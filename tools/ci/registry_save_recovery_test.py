"""Reproduce loss of an ordinary registry save using the production serializer."""
import argparse
from pathlib import Path
import subprocess
import tempfile
from achievement_persistence_test import function

ROOT = Path(__file__).resolve().parents[2]


def run(work):
    work.mkdir(parents=True, exist_ok=True)
    serializer = ROOT / 'src/custom/registry_map.inc'
    (work / 'registry-save.inc').write_text(
        function(serializer, 'void intif_registry_replay(') + '\n' +
        function(serializer, 'int32 intif_saveregistry('))
    binary = work / 'registry-save-test'
    subprocess.run(['g++', '-std=c++17', '-O1', '-g', '-fsanitize=address,undefined',
                    '-fno-sanitize=alignment', '-fno-sanitize-recover=all', '-I' + str(ROOT / 'src'),
                    '-I' + str(work), str(Path(__file__).with_suffix('.cpp')), '-o', str(binary)], check=True)
    result = subprocess.run([str(binary)], capture_output=True, text=True)
    (work / 'result.log').write_text(result.stdout + result.stderr)
    print(result.stdout + result.stderr, end='')
    result.check_returncode()


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path)
    args = parser.parse_args()
    if args.build_dir:
        run(args.build_dir)
    else:
        with tempfile.TemporaryDirectory(prefix='registry-save-') as directory:
            run(Path(directory))
