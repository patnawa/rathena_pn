"""Exercise exact production pet request/reply bodies with explicit transport/world doubles.

No SQL/server runs. The harness exposes async delivery contract failures rather
than claiming character-server persistence or socket integration coverage.
"""
from pathlib import Path
import argparse
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]

def function(source, signature):
    start = source.index(signature)
    opening = source.index('{', start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]

def run(out, source, known_failures=False):
    src = source.read_text()
    if 'bool pet_create_egg(' not in src:
        raise SystemExit('Legacy bool producer has been removed. This historical defect probe requires --source pointing to the pre-migration pet.cpp; current acceptance uses durable producer/native/SQL suites.')
    bodies = '\n'.join(function(src, name) for name in (
        'bool pet_create_egg(', 'bool pet_get_egg('))
    driver = (ROOT/'tools/ci/pet_async_delivery_audit.cpp').read_text().replace('// PRODUCTION FUNCTIONS', bodies)
    (out/'pet.cpp').write_text(driver)
    binary = out/'pet-test'
    subprocess.run(['g++', '-std=c++17', '-O1', '-g', '-fsanitize=address,undefined',
                    '-fno-sanitize-recover=all', str(out/'pet.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary)] + (['--known-failures'] if known_failures else []), check=True)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, default=ROOT/'src/map/pet.cpp')
    parser.add_argument('--build-dir', type=Path)
    parser.add_argument('--known-failures', action='store_true', help='Audit only: require the four documented defects to reproduce; never a release acceptance test')
    args = parser.parse_args()
    if args.build_dir:
        args.build_dir.mkdir(parents=True, exist_ok=True)
        run(args.build_dir.resolve(), args.source.resolve(), args.known_failures)
    else:
        with tempfile.TemporaryDirectory(prefix='pn-pet-async-') as directory:
            run(Path(directory), args.source.resolve(), args.known_failures)
