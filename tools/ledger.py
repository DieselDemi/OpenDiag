"""Generate the PTDIAG behavioral worklist from public resources; no EXEs required."""
import argparse
import csv
from collections import defaultdict
from hashlib import sha256
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1] / 'reference'


def rows(name):
    with (ROOT / name).open(encoding='utf-8', newline='') as stream:
        return list(csv.DictReader(stream))


def resources():
    workflows = rows('workflows.csv')
    dialogs = {k:r for r in workflows for k in r['dialog_ids'].split()}
    main = {k:r for r in workflows for k in r['main_commands'].split()}
    verified = {(r['kind'],r['parent'],r['item']):r for r in rows('behavior-evidence.csv')}
    procs = defaultdict(set)
    for r in rows('dialog-procedures.csv'):
        procs[r['build'],r['dialog_id']].add(r['procedure'])
    owners = defaultdict(set)

    def entry(kind,parent,item,label,builds,ids,cls='',occurrence='1'):
        wf = main.get(item) if kind=='menu' and parent=='102' else None
        if kind=='menu' and parent=='221': wf=dialogs['131']  # Older VID editor popup menu.
        groups = {dialogs[i]['workflow']:dialogs[i] for i in ids if i in dialogs}
        if wf is None and len(groups)==1: wf = next(iter(groups.values()))
        key = f'{kind}:{parent}:{item}'
        variant=sha256('\0'.join([cls,label,occurrence]).encode()).hexdigest()[:12]
        evidence=verified.get((kind,parent,item),{})
        return dict(key=key+':'+variant,workflow=wf['workflow'] if wf else 'needs-triage',
                    kind=kind,parent=parent,item=item,role=cls,label=label,builds=';'.join(builds),
                    dialog_procedures=';'.join(sorted({p for b in builds for i in ids for p in procs[b,i]})),
                    status=evidence.get('status','unverified'),coverage=evidence.get('coverage',''),
                    acceptance_test=evidence.get('test',''),
                    prerequisites=wf['inputs_or_prerequisites'] if wf else '',
                    expected=wf['expected_result'] if wf else '',
                    gate=wf['qualification'] if wf else 'Recover behavior from original')

    for r in rows('controls.csv'):
        if r['product']!='PTDIAG': continue
        ids=[r['dialog_id']]; builds=r['builds'].split(';')
        if r['menu'].startswith('#'):
            for b in builds: owners[b,r['menu'][1:]].update(ids)
        # Captions and decorative controls remain in controls.csv, not the behavior denominator.
        if r['class'] not in ('static','group'):
            yield entry('control',ids[0],r['control_id'],r['label'],builds,ids,r['class'],r['occurrence'])
    for r in rows('inventory.csv'):
        builds=[b for b in r['builds'].split(';') if b.startswith('PTDIAG_')]
        if not builds or r['kind']!='menu': continue
        parent=r['resource_id']; ids={i for b in builds for i in owners[b,parent]}
        yield entry('menu',parent,r['command_id'],r['label'],builds,ids)


def ledger():
    result={}
    for r in resources():
        previous=result.setdefault(r['key'],r)
        if previous is not r:
            # Dialog caption or attached menu changed across builds; the control did not.
            for field in ('builds','dialog_procedures'):
                previous[field]=';'.join(sorted(set((previous[field]+';'+r[field]).split(';'))-{''}))
    yield from result.values()


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--workflow');parser.add_argument('--older-only',action='store_true')
    args=parser.parse_args(); writer=None
    for row in ledger():
        if writer is None:
            writer=csv.DictWriter(sys.stdout,fieldnames=row,lineterminator='\n');writer.writeheader()
        if args.workflow and row['workflow']!=args.workflow: continue
        if args.older_only and '2293' in row['builds']: continue
        writer.writerow(row)
