"""File-backed programming acceptance: compare bytes, interrupt real processes, recover without input media."""
import hashlib
import json
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

cli, fixtures = Path(sys.argv[1]).resolve(), Path(sys.argv[2])


def invoke(action, workspace, *args, error=None):
    cmd = [cli, 'ecu', action, workspace, *args]
    result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
    if error is not None:
        assert result.returncode and error.lower() in result.stderr.lower(), (cmd, result)
        return
    assert result.returncode == 0, (cmd, result)
    return json.loads(result.stdout)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def records(base, data):
    def record(addr, kind, payload):
        raw = bytes([len(payload), addr >> 8, addr & 255, kind]) + payload
        return ':' + (raw + bytes([-sum(raw) & 255])).hex().upper() + '\n'
    out = []
    while data:
        out.append(record(0, 4, (base >> 16).to_bytes(2, 'big')))
        size = min(64, 65536-(base & 65535), len(data))
        out.append(record(base & 65535, 0, data[:size]))
        data, base = data[size:], base+size
    return (''.join(out) + ':00000001FF\n').encode()


with tempfile.TemporaryDirectory() as tmp:
    root = Path(tmp)
    media = root/'Original media'; shutil.copytree(fixtures, media)
    ws = root/'virtual PCM'; flash = ws/'flash.bin'
    initial = bytearray(range(128))
    initial[:8], initial[16:24], initial[72:76] = b'SIM-A\0\0\0', b'TEST-VID', b'TRID'
    expected = bytearray(initial)
    expected[4], expected[8:16], expected[64:72], expected[76:80] = ord('B'), b'NEW-DATA', b'UPDATE-B', b'TEST'
    s = invoke('create', ws, media/'device.json')
    assert s['reportedSoftware']=='SIM-A' and s['bootable'] and flash.read_bytes()==initial
    invoke('create',ws,media/'device.json',error='existing paths')
    invalid_profile=json.loads((media/'device.json').read_text());invalid_profile['retain'][0]['address']='1000'
    invalid_path=media/'bad-device.json';invalid_path.write_text(json.dumps(invalid_profile))
    invoke('create',root/'invalid target',invalid_path,error='ID field');assert not (root/'invalid target').exists()
    for key, value, message in [('part','wrong','does not match'), ('from','wrong','does not match'),
            ('to','wrong','software ID'), ('unexpected',False,'unknown field'), ('minMv',14000,'voltage')]:
        recipe = json.loads((media/'pcm.json').read_text()); recipe.pop('lookup'); recipe[key] = value
        p = media/'bad.json'; p.write_text(json.dumps(recipe)); invoke('prepare',ws,p,error=message)
    recipe = json.loads((media/'generic.json').read_text()); recipe['loader']['sha256']='0'*64
    p.write_text(json.dumps(recipe)); invoke('prepare',ws,p,error='sha-256')
    recipe['loader'] = recipe['primary']; p.write_text(json.dumps(recipe)); invoke('prepare',ws,p,error='lookup table match')
    recipe.pop('lookup'); p.write_text(json.dumps(recipe)); invoke('prepare',ws,p,error='outside')
    assert invoke('status',ws)['phase']=='idle' and flash.read_bytes()==initial and not list(ws.glob('job-*'))

    s=invoke('prepare',ws,media/'generic.json'); job=s['job']; assets=ws/('job-'+job['id'])
    assert s['bootable'] and job['sectors']==2 and job['retainedDifferences']==12 and job['afterSha256']==sha(expected)
    assert (assets/'loader.bin').read_bytes()==b'OpenDiag fixture'+bytes(48)
    assert (assets/'backup.bin').read_bytes()==initial
    media.rename(root/'removed media') # Subsequent commands must use only the verified local checkpoint.
    assert invoke('step',ws)['phase']=='erasing'
    if len(sys.argv)>4:
        cmd=[sys.argv[3],sys.argv[4],'--virtual',str(ws)]
        stopped=subprocess.run(cmd,input='module\nquit\n',text=True,capture_output=True,timeout=10)
        assert stopped.returncode and 'cannot boot' in stopped.stdout,stopped
    invoke('prepare',ws,fixtures/'pcm.json',error='current job')
    invoke('export',ws,root/'partial.bin',error='not bootable')
    assert not (root/'partial.bin').exists()
    s=invoke('step',ws); assert not s['bootable'] and s['cursor']==1 and flash.read_bytes()[:32]==b'\xff'*32
    kit=root/'Recovery Kit';invoke('recovery-kit',ws,kit)
    replacement=root/'replacement';assert invoke('create',replacement,kit/'device.json')['reportedSoftware']=='SIM-A'
    assert (replacement/'flash.bin').read_bytes()==initial
    invoke('voltage',ws,'8000'); invoke('run',ws,error='voltage')
    assert invoke('status',ws)['cursor']==1
    invoke('voltage',ws,'13600'); assert invoke('step',ws)['phase']=='programming'
    # Power loss after some writes but before advancing the sector checkpoint: repeat idempotently.
    with flash.open('r+b') as f: f.write(expected[:9])
    assert invoke('step',ws)['cursor']==1
    assert invoke('step',ws)['phase']=='verifying' and flash.read_bytes()==expected
    assert invoke('step',ws)['phase']=='resetting'
    with flash.open('r+b') as f: f.seek(72); f.write(b'BAD!')
    invoke('step',ws,error='sha-256')
    assert invoke('status',ws)['phase']=='resetting'
    # A damaged candidate or missing loader must not prevent restoration. A damaged backup must.
    (assets/'loader.bin').unlink(); (assets/'desired.bin').write_bytes(b'corrupt')
    (assets/'backup.bin').write_bytes(b'corrupt')
    failed_bank=flash.read_bytes(); invoke('restore',ws,error='sha-256'); assert flash.read_bytes()==failed_bank
    (assets/'backup.bin').write_bytes(initial)
    s=invoke('restore',ws); assert s['phase']=='restored' and s['reportedSoftware']=='SIM-A' and flash.read_bytes()==initial
    # Simulate an interruption after the restore-intent commit and before the atomic bank replacement.
    journal=json.loads((ws/'state.json').read_bytes());journal.update(phase='restoring',cursor=0)
    (ws/'state.json').write_text(json.dumps(journal));flash.write_bytes(failed_bank)
    assert invoke('run',ws)['phase']=='restored' and flash.read_bytes()==initial
    for mode in ['pcm','generic']:
        invoke('prepare',ws,fixtures/(mode+'.json')); s=invoke('run',ws)
        assert s['phase']=='complete' and s['reportedSoftware']=='SIM-B' and s['installedSha256']==sha(expected)
        if len(sys.argv)>4:
            diag=subprocess.Popen(cmd,stdin=subprocess.PIPE,stdout=subprocess.PIPE,text=True)
            try:
                assert 'file-backed virtual ECU' in diag.stdout.readline() and diag.stdout.readline().startswith('OK Connected')
                invoke('restore',ws,error='locked')
                answer=diag.communicate('module\nvoltage\nquit\n',timeout=10)[0]
                assert not diag.returncode and 'OK SIM-B' in answer and '13600 mV' in answer,answer
            finally:
                if diag.poll() is None: diag.kill();diag.wait()
        export=root/(mode+'.bin'); invoke('export',ws,export);assert export.read_bytes()==expected
        invoke('export',ws,export,error='')
        backup=root/(mode+'.backup');invoke('backup',ws,backup);assert backup.read_bytes()==initial
        assert invoke('restore',ws)['reportedSoftware']=='SIM-A'

    # Transfer personal data using addressed HEX, independently of the containing flash sectors.
    media=root/'removed media'; saved=media/'personal.hex';invoke('save-config',ws,saved,'VID')
    cfg=media/'exported.cfg';invoke('kam-layout',ws,cfg);assert ': 1010  8' in cfg.read_text()
    kam=media/'KAM.hex';invoke('save-kam',ws,cfg,kam);assert 'Specified KAM Regions' in kam.read_text()
    assert '12 bytes, 2 range(s)' in subprocess.check_output([cli,'inspect',kam],text=True)
    for version in ['1980','2200','2201','2262','2293']:
        original=Path(__file__).resolve().parents[1]/f'reference/golden/PTDIAG-{version}-KAM.cfg'
        output=media/f'KAM-{version}.hex';invoke('save-kam',ws,original,output)
        assert output.read_bytes()==kam.read_bytes(),version
    raw=media/'one.bin';invoke('read-block',ws,'1010','08',raw);assert raw.read_bytes()==b'TEST-VID'
    invoke('read-block',ws,'107f','02',root/'invalid.bin',error='outside');assert not (root/'invalid.bin').exists()
    other=bytearray(initial);other[16:24]=b'NEW--VID';(media/'unit.bin').write_bytes(other)
    profile=json.loads((media/'device.json').read_text());profile['baseline']=dict(file='unit.bin',sha256=sha(other))
    (media/'unit.json').write_text(json.dumps(profile));unit=root/'new virtual module';invoke('create',unit,media/'unit.json')
    recipe=json.loads((media/'pcm.json').read_text());recipe.pop('lookup');recipe.update(mode='configuration',to='SIM-A',primary=dict(file=saved.name,sha256=sha(saved.read_bytes())))
    p=media/'config-job.json';p.write_text(json.dumps(recipe));s=invoke('prepare',unit,p);assert s['job']['restoredRanges']==['VID']
    invoke('run',unit);assert (unit/'flash.bin').read_bytes()==initial
    invoke('restore',unit);recipe['primary']=dict(file=kam.name,sha256=sha(kam.read_bytes()));p.write_text(json.dumps(recipe));invoke('prepare',unit,p);invoke('run',unit);assert (unit/'flash.bin').read_bytes()==initial
    profile['retain']=dict(file=cfg.name,sha256=sha(cfg.read_bytes()));(media/'unit.json').write_text(json.dumps(profile)); imported=invoke('create',root/'KAM imported',media/'unit.json');assert [r['bytes'] for r in imported['profile']['retain']]==[8,4]
    incomplete=media/'bad-config.hex';incomplete.write_bytes(records(0x1011,b'1234'));recipe['primary']=dict(file=incomplete.name,sha256=sha(incomplete.read_bytes()));p.write_text(json.dumps(recipe));invoke('prepare',unit,p,error='complete named')
    # Explicitly mapped, partial raw block; neighboring bytes must survive sector erasure.
    recipe.update(mode='block', primary=dict(file=raw.name,sha256=sha(raw.read_bytes()),address='1061'))
    p.write_text(json.dumps(recipe));invoke('prepare',unit,p);invoke('run',unit)
    changed=bytearray(initial);changed[97:105]=b'TEST-VID';assert (unit/'flash.bin').read_bytes()==changed
    recipe['primary'].pop('address');p.write_text(json.dumps(recipe));invoke('prepare',unit,p,error='address')

    # Kill the actual automatic programmer. A new process must acquire the stale lock and recover the bank.
    media=root/'interrupt';media.mkdir(); profile=json.loads((fixtures/'device.json').read_text())
    data=bytes(initial)+bytes(65536-len(initial)); (media/'full.bin').write_bytes(data)
    profile.update(bytes=len(data),sectorBytes=256,baseline=dict(file='full.bin',sha256=sha(data)))
    profile.pop('ram');(media/'device.json').write_text(json.dumps(profile));ws2=root/'interruptible ECU'
    invoke('create',ws2,media/'device.json'); recipe=json.loads((fixtures/'pcm.json').read_text())
    candidate=bytearray(b'\x55'*len(data));candidate[:8]=expected[:8]; encoded=records(0x1000,candidate)
    (media/'update.hex').write_bytes(encoded);recipe.pop('lookup');recipe['primary']['sha256']=sha(encoded);(media/'job.json').write_text(json.dumps(recipe))
    invoke('prepare',ws2,media/'job.json')
    proc=subprocess.Popen([cli,'ecu','run',ws2],stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
    try:
        assert proc.stderr.readline().strip()=='erasing';proc.kill();proc.wait(timeout=5)
    finally:
        if proc.poll() is None: proc.kill();proc.wait()
    s=invoke('status',ws2);assert s['phase'] in ('erasing','programming') and not s['bootable']
    shutil.rmtree(media)
    candidate[16:24]=initial[16:24];candidate[72:76]=initial[72:76]
    assert invoke('run',ws2)['installedSha256']==sha(candidate)
    assert invoke('restore',ws2)['installedSha256']==sha(data)
