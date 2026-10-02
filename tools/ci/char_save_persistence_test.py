"""Exercise the actual character writer and final-save handler with SQL faults.

The native probe supplies SQL/transport boundaries. Real database durability is
verified separately; extracting these production functions is not a SQL proof.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile

from achievement_persistence_test import function

ROOT = Path(__file__).resolve().parents[2]


def run(work, case):
    work.mkdir(parents=True, exist_ok=True)
    source = ROOT / 'src/char/char.cpp'
    helpers = ''
    if 'static bool char_status_tables_lock(' in source.read_text():
        helpers = function(source, 'static bool char_status_tables_lock(')
    (work / 'char-writer.inc').write_text(helpers + function(source, 'int32 char_mmo_char_tosql('))
    (work / 'char-save-handler.inc').write_text(function(
        ROOT / 'src/char/char_mapif.cpp', 'int32 chmapif_parse_reqsavechar('))
    binary = work / 'char-save-persistence'
    subprocess.run(['g++', '-std=c++17', '-O1', '-g',
                    '-fsanitize=address,undefined', '-fno-sanitize=alignment',
                    '-fno-sanitize-recover=all', '-I' + str(ROOT / 'src'), '-I' + str(work),
                    str(Path(__file__).with_suffix('.cpp')), '-o', str(binary)], check=True)
    inputs = [source, ROOT / 'src/char/char_mapif.cpp', Path(__file__), Path(__file__).with_suffix('.cpp')]
    (work / 'inputs.json').write_text(json.dumps({str(p.relative_to(ROOT)):
        hashlib.sha256(p.read_bytes()).hexdigest() for p in inputs}, indent=2) + '\n')
    result = subprocess.run([str(binary), case], capture_output=True, text=True)
    (work / (case + '.log')).write_text(result.stdout + result.stderr)
    print(result.stdout + result.stderr, end='')
    result.check_returncode()


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path)
    parser.add_argument('--case', default='all', choices=[
        'all', 'writer-failure', 'ack-failure', 'atomicity', 'packet-identity'])
    args = parser.parse_args()
    if args.build_dir:
        run(args.build_dir, args.case)
    else:
        with tempfile.TemporaryDirectory(prefix='char-save-') as directory:
            run(Path(directory), args.case)
