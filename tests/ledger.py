import collections
import csv
from pathlib import Path
import runpy

ref = Path(__file__).resolve().parents[1]
ledger = list(runpy.run_path(str(ref/'tools/ledger.py'))['ledger']())
assert len({r['key'] for r in ledger}) == len(ledger)
assert all(r['workflow'] != 'needs-triage' and r['expected'] and r['gate'] for r in ledger)
assert any('Read Memory Segment' in r['label'] and '2293' not in r['builds'] for r in ledger)
assert any(r['parent']=='120' and '_PIDDMRMonitor@16' in r['dialog_procedures'] for r in ledger)
with (ref/'reference/controls.csv').open(encoding='utf-8',newline='') as f:
    counts = collections.Counter(b for r in csv.DictReader(f) for b in r['builds'].split(';'))
assert counts['PTDIAG_2293_MyCANIC_FD']==3014 and counts['PTDIAG_1980_CarDAQ']==2186
