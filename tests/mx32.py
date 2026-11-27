"""Compare all config-region conversions with the output of the unmodified MX32 V337 GUI."""
from pathlib import Path
import re
import subprocess
import sys
from tempfile import TemporaryDirectory

cli, fixtures = sys.argv[1], Path(sys.argv[2])
profiles = subprocess.check_output([cli, 'mx32-config', '--list'], text=True).splitlines()
assert set(profiles) == {'vid128', 'trid128', 'trid256', 'vidtrid384'}


def parts(path):
    data = path.read_bytes()
    start = data.index(b'}', data.index(b'file_checksum')) + 1
    # Comments include the original filename, producer and wall-clock time.
    header = re.sub(rb'//[^\n]*', b'', data[:start])
    return b' '.join(header.split()), data[start:]


with TemporaryDirectory() as tmp:
    for profile in profiles:
        source = fixtures / profile / 'Sample.h32'
        reference = fixtures / profile / 'Sample.VBF'
        output = Path(tmp) / (profile + '.vbf')
        subprocess.run([cli, 'mx32-config', profile, source, output], check=True)
        assert parts(output) == parts(reference), profile
        assert 'body CRC-32 verified' in subprocess.check_output([cli, 'inspect', reference], text=True)
        roundtrip = Path(tmp) / (profile + '.hex')
        subprocess.run([cli, 'to-hex', output, roundtrip], check=True, capture_output=True)
        assert roundtrip.read_text().splitlines() == source.read_text().splitlines(), profile
        assert subprocess.run([cli, 'mx32-config', profile, source, output], capture_output=True).returncode
    wrong = Path(tmp)/'mismatch.vbf'
    assert subprocess.run([cli,'mx32-config','trid256',fixtures/'vid128/Sample.h32',wrong],capture_output=True).returncode
    assert not wrong.exists()
