from binascii import crc_hqx
from hashlib import sha256
from zlib import crc32
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
    payload = hexfile.read_bytes()
    for preamble in [b'  FILE NAME > SIMPLE.HEX\n  LAST DATA ADDRESS > 00020006\n',b'file data > Specified KAM Region\r\n']:
        hexfile.write_bytes(preamble+payload);assert '9 bytes' in run('inspect',hexfile)
    hexfile.write_bytes(b'unknown contents\n'+payload);run('inspect',hexfile,success=False)
    hexfile.write_bytes(payload+b'file data > Specified KAM Region\n');run('inspect',hexfile,success=False)
    hexfile.write_text(':0100010042BC\n:0100000041BE\n:00000001FF\n')
    assert '2 bytes, 1 range' in run('inspect', hexfile)
    for bad in (header + block[:-1], header + block + block, header + block[:-2] + b'\0\0',
                header.replace(b'0x00', b'0x10') + block):
        vbf.write_bytes(bad); run('inspect', vbf, success=False)
    crc_header=header[:-1]+f'file_checksum = 0x{crc32(block):08X}; }}'.encode()
    vbf.write_bytes(crc_header+block);assert 'body CRC-32 verified' in run('inspect',vbf)
    vbf.write_bytes(crc_header+block[:3]+bytes([block[3]^1])+block[4:])
    corrupt=subprocess.run([sys.argv[1],'inspect',vbf],text=True,capture_output=True,timeout=10)
    assert corrupt.returncode and 'file checksum mismatch' in corrupt.stderr,corrupt
    for bad in (':0100000041BD\n:00000001FF', ':0100000041BE\n', ':0100000041BE\n'*2 + ':00000001FF',
                ':0100000041BE\n:00000001FF\n:0100000041BE', ':010000004XBE\n:00000001FF'):
        hexfile.write_text(bad); run('inspect', hexfile, success=False)
    binary.write_bytes(bytes.fromhex('FFFE 0002 7FFF 8000'))
    assert run('table',binary,'1000','2','2','s16','motorola','csv','1000').splitlines()==['(address),col1,col2','(00001000),-2,2','(00001004),32767,-32768']
    assert '(00000000),65279' in run('table',binary,'0','1','1','u16','intel','csv')
    run('table',binary,'0','1','41','u8','intel','csv',success=False)
    binary.write_bytes(struct.pack('<f',-1.25));assert '-1.25' in run('table',binary,'0','1','1','f32','intel','fixed')
