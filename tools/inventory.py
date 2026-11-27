"""Inventory the public UI resources of the six known PE32 references, without executing them."""
import argparse
import csv
import hashlib
import re
import subprocess
from collections import defaultdict
from pathlib import Path
import struct
import sys


def word(data, pos):
    return struct.unpack_from('<H', data, pos)[0]


def dword(data, pos):
    return struct.unpack_from('<I', data, pos)[0]


def text(data, pos):
    if word(data, pos) == 0xffff:  # A dialog field can be a resource ordinal.
        return f'#{word(data, pos + 2)}', pos + 4
    end = pos
    while word(data, end):
        end += 2
    return data[pos:end].decode('utf-16le'), end + 2


def resources(data):
    pe = dword(data, 60)
    opt = pe + 24
    section = opt + word(data, pe + 20)
    sections = [struct.unpack_from('<IIII', data, section + 40*i + 8)
                for i in range(word(data, pe + 6))]

    def address(rva):
        return next(offset + rva - start for _, start, size, offset in sections
                    if start <= rva < start + size)

    base = address(dword(data, opt + 112))

    def entries(offset):
        table = base + (offset & 0x7fffffff)
        for i in range(word(data, table + 12) + word(data, table + 14)):
            yield struct.unpack_from('<II', data, table + 16 + 8*i)

    for kind, table in entries(0):
        if kind not in (4, 5):
            continue
        for rid, langs in entries(table):
            if rid & 0x80000000:
                name = base + (rid & 0x7fffffff)
                rid = data[name+2:name+2+2*word(data,name)].decode('utf-16le')
            for _, leaf in entries(langs):
                rva, size = struct.unpack_from('<II', data, base + leaf)
                start = address(rva)
                yield kind, rid, data[start:start + size]


def menu(data):
    extended = word(data, 0) == 1
    pos = 4 + word(data, 2)

    def items(parents):
        nonlocal pos
        while True:
            if extended:
                pos = (pos + 3) & ~3
                cmd, flags = struct.unpack_from('<IH', data, pos + 8)
                popup = flags & 1
                pos += 14
            else:
                flags = word(data, pos)
                popup = flags & 16
                cmd = 0 if popup else word(data, pos + 2)
                pos += 2 if popup else 4
            title, pos = text(data, pos)
            path = parents + [title.replace('&', '').strip()]
            if popup:
                if extended:
                    pos = ((pos + 3) & ~3) + 4
                yield from items(path)
            elif title:
                yield cmd, ' / '.join(path)
            if flags & 128:
                return

    yield from items([])


def callbacks(path, objdump):
    """Resolve only direct DialogBox/CreateDialog calls with literal template and procedure arguments."""
    symbols, calls, slots = {}, set(), {}
    label = re.compile(r'^([0-9a-f]+) <([^>]+)>:$')
    instruction = re.compile(r'^\s*[0-9a-f]+:\s+(\w+)\s+(.*)$')
    with subprocess.Popen([objdump,'-d','--no-show-raw-insn',path], stdout=subprocess.PIPE,
                          text=True, errors='replace') as p:
        for line in p.stdout:
            if m := label.match(line):
                symbols[int(m[1],16)] = m[2]; slots.clear(); continue
            if not (m := instruction.match(line)): continue
            op, args = m[1], m[2].split('#')[0].strip()
            if op.startswith('call'):
                if re.search(r'<_(?:CreateDialogParam|DialogBoxParam)[AW]@20>',args) and slots.get('4') and slots.get('c'):
                    calls.add((slots['4'],slots['c']))
                slots.clear()
            elif op.startswith(('j','ret','push','pop')) or args.endswith(', %esp'):
                slots.clear()
            elif a := re.search(r'(?:, |^)(?:0x)?([4c])\(%esp\)$',args):
                value = re.match(r'^\$0x([0-9a-f]+),',args) if op=='movl' else None
                slots[a[1]] = int(value[1],16) if value else None
        if p.wait(): raise RuntimeError(f'{objdump} failed for {path}')
    return sorted((rid,symbols[addr]) for rid,addr in calls if addr in symbols)


def dialog(data):
    extended = data[:4] == b'\x01\0\xff\xff'
    pos = 26 if extended else 18
    style = dword(data, 12 if extended else 0)
    count = word(data, 16 if extended else 8)
    owner_menu, pos = text(data, pos)
    _, pos = text(data, pos)
    title, pos = text(data, pos)
    if style & 64:  # DS_SETFONT
        _, pos = text(data, pos + (6 if extended else 2))
    controls = []
    classes = ['button', 'edit', 'static', 'listbox', 'scrollbar', 'combobox']
    for _ in range(count):
        pos = (pos + 3) & ~3
        control_style = dword(data, pos + (8 if extended else 0))
        ident = dword(data, pos+20) if extended else word(data, pos+16)
        pos += 24 if extended else 18
        cls, pos = text(data, pos)
        label, pos = text(data, pos)
        extra = word(data, pos)
        pos += max(extra, 2)
        if pos > len(data):
            raise ValueError('Invalid dialog creation data')
        if cls in [f'#{n}' for n in range(128,134)]:
            cls = classes[int(cls[1:])-128]
        cls = cls.lower()
        if cls == 'button' and control_style & 15 == 7:
            cls = 'group'
        elif cls == 'static' and control_style & 0x100:  # SS_NOTIFY can deliver user input.
            cls = 'static-notify'
        controls.append((ident, cls, label))
    return title, owner_menu, controls


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    modes = parser.add_mutually_exclusive_group()
    modes.add_argument('--controls', action='store_true', help='emit the control-level ledger instead')
    modes.add_argument('--callbacks', metavar='LLVM_OBJDUMP', help='map dialog IDs to recovered dialog procedures')
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    rows = defaultdict(list)
    for line in (root / 'reference/SHA256SUMS').read_text().splitlines():
        digest, name = line.split()
        data = (root / name).read_bytes()
        if hashlib.sha256(data).hexdigest() != digest:
            raise ValueError(f'{name}: reference hash mismatch')
        build = Path(name).stem
        if args.callbacks:
            for rid, proc in callbacks(root/name,args.callbacks): rows[build,rid,proc].append(build)
            continue
        for kind, rid, payload in resources(data):
            if kind == 4 and not args.controls:
                for cmd, label in menu(payload):
                    rows['menu', rid, cmd, label].append(build)
            elif kind == 5:
                label, menu_id, controls = dialog(payload)
                if args.controls:
                    # IDs are only unique within a dialog; static controls can even reuse an ID there.
                    seen = defaultdict(int)
                    product = 'MX32' if build.startswith('MX32') else 'PTDIAG'
                    for ident, cls, caption in controls:
                        key = (ident, cls, caption)
                        seen[key] += 1
                        rows[product, rid, label, menu_id, ident, cls, caption, seen[key]].append(build)
                else:
                    rows['dialog', rid, '', label].append(build)
    writer = csv.writer(sys.stdout, lineterminator='\n')
    if args.callbacks:
        writer.writerow(['build','dialog_id','procedure']); writer.writerows(sorted(rows)); return
    writer.writerow((['product','dialog_id','dialog','menu','control_id','class','label','occurrence']
                     if args.controls else ['kind', 'resource_id', 'command_id', 'label']) + ['builds'])
    for key, builds in sorted(rows.items(), key=lambda item: (item[0][0], not isinstance(item[0][1],int), *item[0][1:])):
        writer.writerow([*key, ';'.join(dict.fromkeys(builds))])


if __name__ == '__main__':
    main()
