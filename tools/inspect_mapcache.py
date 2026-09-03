#!/usr/bin/env python3
"""Print dimensions and walkability around coordinates in an rAthena map cache."""

import argparse
import struct
import zlib


def load_maps(path, wanted):
    data = open(path, "rb").read()
    _, count = struct.unpack_from("<IH", data, 0)
    offset = 8
    result = {}
    for _ in range(count):
        raw_name, width, height, length = struct.unpack_from("<12shhi", data, offset)
        offset += 20
        name = raw_name.split(b"\0", 1)[0].decode("ascii")
        cells = zlib.decompress(data[offset : offset + length])
        offset += length
        if name in wanted:
            result[name] = (width, height, cells)
    return result


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("cache")
    parser.add_argument("maps", nargs="+")
    parser.add_argument("--x", type=int, default=100)
    parser.add_argument("--y", type=int, default=100)
    args = parser.parse_args()

    maps = load_maps(args.cache, set(args.maps))
    for name in args.maps:
        width, height, cells = maps[name]
        cell = cells[args.x + args.y * width] if 0 <= args.x < width and 0 <= args.y < height else -1
        candidates = []
        for y in range(height):
            for x in range(width):
                if cells[x + y * width] in (0, 3):
                    candidates.append((abs(x - args.x) + abs(y - args.y), x, y))
        _, near_x, near_y = min(candidates)
        print(
            f"{name}: {width}x{height}; ({args.x},{args.y}) cell={cell}; "
            f"walkable={cell in (0, 3)}; nearest=({near_x},{near_y})"
        )


if __name__ == "__main__":
    main()
