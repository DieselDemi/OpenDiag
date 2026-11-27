from binascii import crc_hqx
from hashlib import sha256
from pathlib import Path
import struct
import subprocess
import sys
from tempfile import TemporaryDirectory


def run(*args, success=True):
    result = subprocess.run([sys.argv[1], *args], text=True, capture_output=True, timeout=10)
    assert (result.returncode == 0) == success, (args, result)
    return result.stdout


with TemporaryDirectory() as folder:
    base = Path(folder)
    binary, hexfile, vbf = [base / name for name in ['test.bin', 'test.hex', 'test.vbf']]
    binary.write_bytes(b'123456789')
    run('to-hex', binary, hexfile, success=False)
    run('to-hex', binary, hexfile, '1fffe')
    assert hexfile.read_text().splitlines() == [':020000040001F9', ':02FFFE0031329E',
        ':020000040002F8', ':07000000333435363738397F', ':00000001FF']
    assert '9 bytes' in run('inspect', hexfile)
    backup = base/'backup.bin'
    backup.write_bytes(b'\0'*12)
    preview = run('compare',backup,'1fffd',hexfile)
    assert '9 changed bytes' in preview and sha256(b'\0'+b'123456789'+b'\0\0').hexdigest() in preview
    run('compare',backup,'20000',hexfile,success=False)
    assert backup.read_bytes()==bytes(12)
    run('to-hex', binary, hexfile, '1000', success=False)
    assert hexfile.read_text().startswith(':020000040001F9')
    run('to-hex', binary, base/'overflow.hex', 'ffffffff', success=False)
    run('vid',binary,'1',success=False)
    binary.write_bytes(bytes(range(108)))
    for layout, offset in enumerate(('19','58','02')):
        assert f'SPEED_LIMIT_OFFSET @0x{offset}: 0x{offset}' in run('vid',binary,str(layout))
    run('vid',binary,'3',success=False)

    header = b'vbf_version = 2.4;\nheader { // }\n text = {"}"}; data_format_identifier = 0x00; }'
    data = b'123456789'
    block = struct.pack('>II', 0x1fffe, len(data)) + data + struct.pack('>H', crc_hqx(data, 0xffff))
    for sep in (b'', b'\r\n', b'\n'):
        vbf.write_bytes(header + sep + block)
        assert 'CRC-16 verified' in run('inspect', vbf)
    alt = base / 'vbf.hex'
    run('to-hex', vbf, alt)
    assert alt.read_bytes() == hexfile.read_bytes()
    hexfile.write_text(':0100010042BC\n:0100000041BE\n:00000001FF\n')
    assert '2 bytes, 1 range' in run('inspect', hexfile)
    for bad in (header + block[:-1], header + block + block, header + block[:-2] + b'\0\0',
                header.replace(b'0x00', b'0x10') + block):
        vbf.write_bytes(bad); run('inspect', vbf, success=False)
    for bad in (':0100000041BD\n:00000001FF', ':0100000041BE\n', ':0100000041BE\n'*2 + ':00000001FF',
                ':0100000041BE\n:00000001FF\n:0100000041BE', ':010000004XBE\n:00000001FF'):
        hexfile.write_text(bad); run('inspect', hexfile, success=False)
