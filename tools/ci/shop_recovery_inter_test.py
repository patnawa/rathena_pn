"""Run exact map shop inter handler with explicit transport/world boundary doubles."""
from pathlib import Path
import subprocess
import tempfile
import sys
ROOT = Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix='pn-shop-inter-') as directory:
    path=Path(directory)
    source=(ROOT/'src/custom/shop_inter.inc').read_text()
    # Only replace production includes; every executable handler line is retained.
    (path/'shop_inter_body.inc').write_text('\n'.join(line for line in source.splitlines() if not line.startswith('#include')))
    binary=path/'shop-inter'
    for queue in ([True] if '--queue' in sys.argv else [False, True]):
        subprocess.run(['g++','-std=c++17','-O1','-fsanitize=address,undefined','-fno-sanitize-recover=all',
                        *(['-DPN_TEST_SHOP_QUEUE'] if queue else []),
                        '-I'+str(ROOT/'src'),'-I'+str(path),str(ROOT/'tools/ci/shop_recovery_inter_test.cpp'),'-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True)
