"""Exercise the exact market restoration handler with poisoned allocation.

NPC lookup and SQL writes are recorded doubles; this is not a database test.
"""
from pathlib import Path
import argparse
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, default=ROOT / 'src/map/npc.cpp')
    source = parser.parse_args().source.read_text()
    start = source.index('static int32 npc_market_checkall_sub(')
    end = source.index('\n/**', start)
    with tempfile.TemporaryDirectory(prefix='pn-market-restore-') as directory:
        path = Path(directory)
        (path / 'market_restore_body.inc').write_text(source[start:end])
        binary = path / 'market-restore'
        subprocess.run(['g++', '-std=c++17', '-O1', '-g',
                        '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                        '-I' + str(path), str(ROOT / 'tools/ci/market_restore_test.cpp'),
                        '-o', str(binary)], check=True)
        subprocess.run([str(binary)], check=True)


if __name__ == '__main__':
    main()
