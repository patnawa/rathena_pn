#!/usr/bin/env python3
"""Align twelve reviewed Chapter 2 alias cells with the installed client GAT.

Only the three compatibility aliases are changed; all other records and cells
are retained. The source's four wall cells were marked water by the old cache.
Use on an offline candidate or during map-server maintenance, after a backup.
"""
import argparse
from pathlib import Path
import struct
import zlib
from build_main_office import cache_records, write_cache

MAPS = ('mu_dun01', 'mu_dun02', 'rgs_dun1')


def repair(path):
    records = cache_records(path)
    changed = 0
    for name in MAPS:
        if name not in records:
            continue
        raw = records[name]
        assert struct.unpack_from('<hh', raw, 12) == (300, 300), ('Map dimensions changed', name)
        cells = bytearray(zlib.decompress(raw[20:]))
        for x in range(176, 180):
            pos = x + 87*300
            assert cells[pos] in (1, 3), ('Unexpected reviewed cell', name, x, cells[pos])
            changed += cells[pos] != 1
            cells[pos] = 1
        packed = zlib.compress(cells)
        records[name] = raw[:16] + struct.pack('<i', len(packed)) + packed
    if changed:
        write_cache(path, records)
    return changed


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('cache', type=Path)
    args = parser.parse_args()
    print(f'Corrected {repair(args.cache)} Chapter 2 cells.')
