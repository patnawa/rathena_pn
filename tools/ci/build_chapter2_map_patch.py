#!/usr/bin/env python3
# ============================================================================
#  PN  /  DEVELOPMENT TOOLS
#  build_chapter2_map_patch.py
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/build_chapter2_map_patch.py
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

"""Materialize Chapter 2 fallback map names in an offline GRF-v2 artifact.

Uses locally supplied, unencrypted map resources; never downloads or installs
assets. Original map bytes remain unchanged, preserving server-cache geometry.
"""
import argparse
import hashlib
import json
import re
import struct
import zlib
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


def build(source, aliases, resource_table):
    redirects = {}
    for line in resource_table.splitlines():
        match = re.fullmatch(rb'([\w@]+\.(?:gat|gnd|rsw))#([\w@]+\.(?:gat|gnd|rsw))#', line.strip())
        if match:
            redirects[match[1].decode()] = match[2].decode()
    entries, report = {}, []
    for line in aliases.splitlines():
        if not line.strip() or line.startswith('//'):
            continue
        target, original, empty = line.strip().split('#')
        if empty or not re.fullmatch(r'[\w@]+\.(gat|gnd|rsw)', target):
            raise ValueError('Invalid alias: ' + line)
        resolved, seen = original, set()
        while resolved in redirects and redirects[resolved] != resolved:
            if resolved in seen:
                raise ValueError('Resource alias cycle: ' + original)
            seen.add(resolved)
            resolved = redirects[resolved]
        payload = (source / resolved).read_bytes()
        magic = {'.gat': b'GRAT', '.gnd': b'GRGN', '.rsw': b'GRSW'}[Path(target).suffix]
        if not payload.startswith(magic):
            raise ValueError('Invalid map signature: ' + resolved)
        if target.endswith('.gat'):
            width, height = struct.unpack_from('<II', payload, 6)
            if len(payload) != 14 + width * height * 20:
                raise ValueError('Invalid GAT dimensions: ' + resolved)
        name = 'data\\' + target
        if name in entries:
            raise ValueError('Duplicate target: ' + target)
        entries[name] = payload
        report.append(dict(target=name, source=resolved, bytes=len(payload),
                           sha256=hashlib.sha256(payload).hexdigest()))
    if not entries or len(entries) % 3:
        raise ValueError('Incomplete map collection')
    for name in entries:
        for extension in ('gat', 'gnd', 'rsw'):
            if name.rsplit('.', 1)[0] + '.' + extension not in entries:
                raise ValueError('Incomplete map triple: ' + name)
    data, table = bytearray(), bytearray()
    for name, payload in sorted(entries.items()):
        packed = zlib.compress(payload, 9)
        table.extend(name.encode('ascii') + b'\0')
        table.extend(struct.pack('<IIIBI', len(packed), len(packed), len(payload), 1, len(data)))
        data.extend(packed)
    packed_table = zlib.compress(table, 9)
    header = b'Master of Magic\0' + bytes(14) + struct.pack('<IIII', len(data), 0, len(entries) + 7, 0x200)
    return header + data + struct.pack('<II', len(packed_table), len(table)) + packed_table, report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-data', type=Path, required=True)
    parser.add_argument('--resource-table', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    aliases = (ROOT / 'client-patch/chapter2/resnametable_chapter2.txt').read_text()
    archive, report = build(args.source_data, aliases, args.resource_table.read_bytes())
    args.output.mkdir(parents=True, exist_ok=False)
    (args.output / 'chapter2_maps.grf').write_bytes(archive)
    result = dict(entries=report, grf_sha256=hashlib.sha256(archive).hexdigest())
    (args.output / 'map-build-report.json').write_text(json.dumps(result, indent=2) + '\n')
    print(f'Built {len(report)} resources; SHA-256 {result["grf_sha256"]}')


if __name__ == '__main__':
    main()
