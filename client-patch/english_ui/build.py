"""Build the English interface overlay from the verified installation snapshot."""
from pathlib import Path
import argparse, hashlib, json, struct, sys, zlib

ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT.parent / 'client_compat'))
from merge_grfs import read, merge

def build():
    assets = {}
    for row in json.loads((ROOT / 'manifest.json').read_text(encoding='utf-8')):
        name = row['path']
        raw = (ROOT / 'files' / name).read_bytes()
        assert hashlib.sha256(raw).hexdigest() == row['sha256'], name
        key = (name if name.startswith('data/') else 'data/' + name).replace('/', '\\').encode('cp949')
        assert key not in assets or assets[key] == raw, name
        assets[key] = raw
    body, table = bytearray(), bytearray()
    for name, raw in sorted(assets.items()):
        packed = zlib.compress(raw, 9)
        table += name + b'\0' + struct.pack('<IIIBI', len(packed), len(packed), len(raw), 1, len(body))
        body += packed
    packed = zlib.compress(table, 9)
    archive = (b'Master of Magic\0' + bytes(14) + struct.pack('<IIII', len(body), 0, len(assets)+7, 0x200)
               + body + struct.pack('<II', len(packed), len(table)) + packed)
    assert read(archive) == assets
    return archive

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--merge', type=Path, help='Existing client_repairs.grf to preserve')
    parser.add_argument('--output', type=Path, help='Output archive; omit for validation only')
    args = parser.parse_args()
    archive = build()
    if args.merge:
        old = args.merge.read_bytes()
        combined = merge([archive, old])
        expected, actual = read(archive), read(combined)
        assert all(actual[k] == v for k, v in read(old).items() if k not in expected)
        archive = combined
    if args.output:
        assert not args.merge or args.output.resolve() != args.merge.resolve(), 'Use a separate output before installation'
        args.output.write_bytes(archive)
    print('PASS: manifest hashes, duplicate aliases, GRF roundtrip and existing resource preservation')
