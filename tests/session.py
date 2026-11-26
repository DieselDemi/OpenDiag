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
