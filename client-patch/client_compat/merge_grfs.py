"""Merge standard GRF v2 repair overlays in highest-priority-first order."""
import argparse
from pathlib import Path
import struct
import zlib


def read(data):
    if data[:16] != b'Master of Magic\0':
        raise ValueError('Expected a standard GRF v2 repair archive')
    offset, seed, count, version = struct.unpack_from('<IIII', data, 30)
    if version != 0x200 or count < seed + 7:
        raise ValueError('Invalid GRF version/count')
    packed, expanded = struct.unpack_from('<II', data, 46 + offset)
    if 54 + offset + packed != len(data):
        raise ValueError('Invalid GRF table bounds')
    table = zlib.decompress(data[54 + offset:])
    if len(table) != expanded:
        raise ValueError('Invalid GRF table size')
    entries, pos = {}, 0
    for _ in range(count - seed - 7):
        end = table.index(0, pos)
        name = table[pos:end]
        size, aligned, length, flag, start = struct.unpack_from('<IIIBI', table, end + 1)
        pos = end + 18
        if flag != 1 or size != aligned or start + size > offset or name in entries:
            raise ValueError('Invalid or duplicate GRF entry')
        raw = zlib.decompress(data[46 + start:46 + start + size])
        if len(raw) != length:
            raise ValueError('Invalid GRF payload size')
        entries[name] = raw
    if pos != len(table):
        raise ValueError('Extra GRF table bytes')
    return entries


def merge(archives):
    entries = {}
    for archive in archives:
        for name, raw in read(archive).items():
            entries.setdefault(name, raw)
    body, table = bytearray(), bytearray()
    for name, raw in sorted(entries.items()):
        packed = zlib.compress(raw, 9)
        table += name + b'\0' + struct.pack('<IIIBI', len(packed), len(packed), len(raw), 1, len(body))
        body += packed
    packed = zlib.compress(table, 9)
    result = (b'Master of Magic\0' + bytes(14)
              + struct.pack('<IIII', len(body), 0, len(entries) + 7, 0x200)
              + body + struct.pack('<II', len(packed), len(table)) + packed)
    assert read(result) == entries
    return result


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('archives', nargs='+', type=Path)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    args.output.write_bytes(merge([p.read_bytes() for p in args.archives]))
    print('Merged', len(read(args.output.read_bytes())), 'resources into', args.output)
