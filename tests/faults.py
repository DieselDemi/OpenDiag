import os
from pathlib import Path
import subprocess
import sys
from tempfile import TemporaryDirectory

with TemporaryDirectory() as tmp:
    for mode, expected, exitcode in [('version','provider reports 05.00',1), ('filter','Flow-control filter failed',1), ('unplug','Receive failed',1),
            ('timeout','No matching PCM response',0), ('negative','ECU negative response 0x31',0),
            ('sequence','OK GDS (D100 = 0x85)',0)]:
        trace = Path(tmp)/mode
        r = subprocess.run(sys.argv[1:], input='ping\nquit\n', capture_output=True, text=True,
                           timeout=8, env=dict(os.environ, OD_TRACE=str(trace), OD_FAULT=mode))
        assert expected in r.stdout and r.returncode == exitcode, (mode,r)
        events = trace.read_text().splitlines()
        wanted = ['open','close'] if mode=='version' else ['open','connect','filter'] + ([] if mode=='filter' else ['tx','stop']) + ['disconnect','close']
        assert events == wanted, events
