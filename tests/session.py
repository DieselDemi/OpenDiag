import subprocess
import sys
from pathlib import Path
from tempfile import TemporaryDirectory

worker, provider = sys.argv[1:]
result = subprocess.run([worker, provider], input='ping\nmodule\nvin\ndtc\nrpm\nquit\n',
                        text=True, capture_output=True, timeout=10, check=True)
assert [s for s in result.stdout.splitlines() if s.startswith('OK ')] == ['OK Connected: PCM 7E0/7E8, ISO 15765, 500 kbit/s',
    'OK GGDS (D100 = 0x01)', 'OK SIM-PCM-001', 'OK TEST0000000000001', 'OK P0133', 'OK 800 rpm'], result
assert 'REQUEST 7E0 22 D1 00\nRESPONSE 7E8 62 D1 00 01' in result.stdout, result
for tx, rx in [('7E1','7E9'),('18DA10F1','18DAF110')]:
    r=subprocess.run([worker,provider,tx,rx,'500000'],input='did:1165\nfreeze:0C\nford-dtc\ncvn\nquit\n',text=True,capture_output=True,timeout=8,check=True)
    assert f'REQUEST {tx} 22 11 65\nRESPONSE {rx} 62 11 65 0C 80\nOK raw=0C80' in r.stdout and '013300 status 0x08' in r.stdout and 'OK 12345678' in r.stdout,r
    with TemporaryDirectory() as tmp:
        cap=Path(tmp)/'session.log';cap.write_text(r.stdout);subprocess.run([worker,'--replay',cap],capture_output=True,check=True)
assert subprocess.run([worker,provider,'7DF','7E8','500000'],capture_output=True,timeout=5).returncode
for width, ident in [('11','320'),('29','18FE0000')]:
    r=subprocess.run([worker,provider,'--can',width,'500000'],input='poll\nquit\n',text=True,capture_output=True,timeout=5,check=True)
    assert f'FRAME 0 {ident} 00E0' in r.stdout and r.stdout.count('FRAME ')==2 and 'REQUEST' not in r.stdout,r
modules=Path(__file__).resolve().parents[1]/'reference/examples/modules.cfg'
r=subprocess.run([worker,'--dtc-scan',provider,modules,'08'],capture_output=True,text=True,check=True,timeout=10)
assert r.stdout.count('MODULE ')==5 and r.stdout.count('SKIP ')==3 and 'REQUEST 7E1 19 02 08' in r.stdout and r.stdout.count('013300 status 0x08')==2,r
r=subprocess.run([worker,provider],input='scan:01\nscan:02\nscan:06\nscan:09\nquit\n',capture_output=True,text=True,check=True,timeout=10)
assert 'ITEM obd:05 raw 78; 80 C' in r.stdout and 'ITEM mode6:01 raw 01 01' in r.stdout and 'TEST0000000000001' in r.stdout and r.stdout.count('OK OBD scan:')==4,r
missing = subprocess.run([worker, provider + '.missing'], text=True, capture_output=True, timeout=5)
assert missing.returncode and missing.stdout.startswith('ERR '), missing
with TemporaryDirectory() as tmp:
    log = Path(tmp)/'capture.log'
    log.write_text('\n'.join('2026-11-26T00:00:00.000Z  '+s for s in result.stdout.splitlines()))
    replay = subprocess.run([worker,'--replay',log],check=True,capture_output=True,text=True)
    assert replay.stdout.splitlines()[-2:] == ['dtc: P0133','rpm: 800 rpm'], replay
    for bad in ('REQUEST 7E0 22 D1 00','RESPONSE 7E8 62 D1 00 01',
                'REQUEST 7E0 22 D1 00\nRESPONSE 7E8 62 F1 88 41', 'REQUEST 7E0 22 XX 00'):
        log.write_text(bad)
        assert subprocess.run([worker,'--replay',log],capture_output=True).returncode, bad
