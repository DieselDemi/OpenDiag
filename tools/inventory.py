"""Print the menu commands and dialog captions of the six known PE32 references as CSV."""
import csv
import hashlib
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


def main():
    root = Path(__file__).resolve().parents[1]
    rows = defaultdict(list)
    for line in (root / 'reference/SHA256SUMS').read_text().splitlines():
        digest, name = line.split()
        data = (root / name).read_bytes()
        if hashlib.sha256(data).hexdigest() != digest:
            raise ValueError(f'{name}: reference hash mismatch')
        build = Path(name).stem
        for kind, rid, payload in resources(data):
            if kind == 4:
                for cmd, label in menu(payload):
                    rows['menu', rid, cmd, label].append(build)
            else:
                pos = 26 if payload[:4] == b'\x01\0\xff\xff' else 18
                for _ in range(3):  # menu, window class, caption
                    label, pos = text(payload, pos)
                rows['dialog', rid, '', label].append(build)
    writer = csv.writer(sys.stdout, lineterminator='\n')
    writer.writerow(['kind', 'resource_id', 'command_id', 'label', 'builds'])
    for key, builds in sorted(rows.items()):
        writer.writerow([*key, ';'.join(dict.fromkeys(builds))])


if __name__ == '__main__':
    main()
