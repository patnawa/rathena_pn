"""Exercise production pair SQL boundaries under undefined-behavior sanitizer."""
from pathlib import Path
import subprocess
import tempfile
root=Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix='pn-pair-service-') as folder:
    binary=Path(folder)/'pair-service'
    subprocess.run(['g++','-std=c++17','-O1','-fsanitize=undefined','-fno-sanitize-recover=all',
                    '-I'+str(root/'src'),str(root/'tools/ci/pair_service_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
