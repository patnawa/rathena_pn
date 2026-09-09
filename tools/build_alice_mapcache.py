# ============================================================================
#  PN  /  DEVELOPMENT TOOLS
#  build_alice_mapcache.py
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/build_alice_mapcache.py
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

"""Install the two original Alice GAT grids into a native cache, preserving other records."""
import argparse
from pathlib import Path
import struct
import zlib

ROOT = Path(__file__).resolve().parents[1]
NAMES = ('1@alice_mad', '2@alice_mad')


def build(cache, maps):
    original = cache.read_bytes()
    size, count = struct.unpack_from('<IH', original)
    assert size == len(original), 'Invalid cache size'
    records = []
    offset = 8
    for _ in range(count):
        name, width, height, length = struct.unpack_from('<12shhi', original, offset)
        end = offset + 20 + length
        assert len(zlib.decompress(original[offset + 20:end])) == width * height
        if name.split(b'\0', 1)[0].decode() not in NAMES:
            records.append(original[offset:end])
        offset = end
    assert offset == len(original), 'Unexpected cache tail'
    for name in NAMES:
        gat = (maps / (name + '.gat')).read_bytes()
        assert gat[:6] == b'GRAT\x01\x02'
        width, height = struct.unpack_from('<II', gat, 6)
        assert len(gat) == 14 + width * height * 20
        cells = bytes(struct.unpack_from('<I', gat, 30 + i * 20)[0] for i in range(width * height))
        assert set(cells) <= {0, 1}, 'Unexpected generated terrain type'
        packed = zlib.compress(cells, 9)
        records.append(struct.pack('<12shhi', name.encode(), width, height, len(packed)) + packed)
    body = b''.join(records)
    return struct.pack('<IH', 8 + len(body), len(records)) + original[6:8] + body


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cache', type=Path, default=ROOT / 'db/map_cache.dat')
    parser.add_argument('--maps', type=Path, default=ROOT / 'client-patch/alice_maze/data')
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    result = build(args.cache, args.maps)
    if args.check:
        assert result == args.cache.read_bytes(), 'Alice map cache is stale'
    else:
        args.cache.write_bytes(result)
    print('Alice map cache matches generated terrain.')
