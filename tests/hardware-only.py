from pathlib import Path
import subprocess
import sys
import tempfile

app, worker, image = sys.argv[1:]
root = Path(app).resolve().parent
assert Path(app).is_file() and Path(worker).is_file() and Path(image).is_file()
for path in root.iterdir():
    if path.is_file():
        assert not any(name in path.name.lower() for name in ("opendiag-sim", "virtual-ecu", "simulator")), path
with tempfile.TemporaryDirectory() as scratch:
    result = subprocess.run([worker, "relative-provider"], capture_output=True, text=True, timeout=5)
    assert result.returncode and "absolute provider path" in result.stdout.lower(), result
    result = subprocess.run([worker, str(Path(scratch) / "missing-provider")],
                            capture_output=True, text=True, timeout=5)
    assert result.returncode and any(message in result.stdout.lower() for message in
                                     ("no such file", "cannot open shared object", "cannot load provider")), result
    result = subprocess.run([image, "ecu", "status", "unused"], capture_output=True, text=True, timeout=5)
    assert result.returncode, result
