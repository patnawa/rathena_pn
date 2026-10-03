"""Run exact map shop inter handler with explicit transport/world boundary doubles."""
from pathlib import Path
import argparse
import subprocess
import tempfile
ROOT = Path(__file__).resolve().parents[2]


def run(root, path, source, fixture, modes):
    path.mkdir(parents=True, exist_ok=True)
    source=source.read_text()
    # Only replace production includes; every executable handler line is retained.
    (path/'shop_inter_body.inc').write_text('\n'.join(line for line in source.splitlines() if not line.startswith('#include')))
    binary=path/'shop-inter'
    for mode in modes:
        subprocess.run(['g++','-std=c++17','-O1','-fsanitize=address,undefined','-fno-sanitize-recover=all',
                        *(['-D'+mode] if mode else []),
                        '-I'+str(root/'src'),'-I'+str(path),str(fixture),'-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True)


if __name__ == '__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root',type=Path,default=ROOT,help='Read-only production header root')
    parser.add_argument('--source',type=Path,default=ROOT/'src/custom/shop_inter.inc')
    parser.add_argument('--fixture',type=Path,default=ROOT/'tools/ci/shop_recovery_inter_test.cpp')
    parser.add_argument('--build-dir',type=Path)
    modes=parser.add_mutually_exclusive_group()
    modes.add_argument('--queue',action='store_true')
    modes.add_argument('--cash-log-only',action='store_true',help='Run only the focused ACK currency logging cases')
    args=parser.parse_args()
    modes=['PN_TEST_CASH_LOG_ONLY'] if args.cash_log_only else ['PN_TEST_SHOP_QUEUE'] if args.queue else [None,'PN_TEST_SHOP_QUEUE']
    if args.build_dir:
        run(args.root.resolve(),args.build_dir.resolve(),args.source.resolve(),args.fixture.resolve(),modes)
    else:
        with tempfile.TemporaryDirectory(prefix='pn-shop-inter-') as directory:
            run(args.root.resolve(),Path(directory),args.source.resolve(),args.fixture.resolve(),modes)
