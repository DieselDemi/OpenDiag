import subprocess
import sys
from pathlib import Path

worker=Path(sys.argv[1])
for args, expected in [([], ('usage:',)), (['relative-driver'], ('absolute provider path',)),
                       ([str(worker)+'.missing'], ('no such file','cannot open shared object','cannot load provider'))]:
    r=subprocess.run([worker,*args],text=True,capture_output=True,timeout=5)
    assert r.returncode and any(message in r.stdout.lower() for message in expected),(args,r)
