import subprocess
import sys
from pathlib import Path

worker=Path(sys.argv[1])
for args, expected in [([], ('usage:',)), (['relative-driver'], ('absolute provider path',)),
                        ([str(worker)+'.missing'], ('no such file','cannot open shared object','cannot load provider'))]:
    r=subprocess.run([worker,*args],text=True,capture_output=True,timeout=5)
    assert r.returncode and any(message in r.stdout.lower() for message in expected),(args,r)

r=subprocess.run([worker,'--seed-key','ptdiag-pats-v408','00','123456'],text=True,capture_output=True,timeout=5)
assert r.returncode == 0 and 'KEY 313DCF' in r.stdout, r

r=subprocess.run([worker,'--seed-key','ptdiag-pats-b299n','00','123456'],text=True,capture_output=True,timeout=5)
assert r.returncode and 'expects level' in r.stdout.lower(), r

r=subprocess.run([worker,'--seed-key-profiles'],text=True,capture_output=True,timeout=5)
assert r.returncode == 0 and 'PROFILE ptdiag-pats-v408' in r.stdout and 'built-in' in r.stdout, r
