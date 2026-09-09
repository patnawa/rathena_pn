# ============================================================================
#  PN  /  CLIENT TOOLING
#  build_grf.py
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/client-patch/navigation_repair/build_grf.py
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

"""Pack validated navigation tables; generated GRFs are distribution artifacts."""
import argparse
from pathlib import Path
import struct
import zlib


def build(tables: Path, helper: Path) -> bytes:
    entries = {}
    for suffix in ('krpri', 'krsak'):
        for kind in ('map', 'npc', 'mob', 'link', 'npcdistance', 'linkdistance', 'picknpc', 'scroll'):
            raw = (tables / f'navi_{kind}_krpri.lub').read_bytes()
            entries[rf'data\luafiles514\lua files\navigation\navi_{kind}_{suffix}.lub'] = raw
        entries[rf'data\luafiles514\lua files\navigation\navi_f_{suffix}.lub'] = helper.read_bytes()
    body = bytearray()
    index = bytearray()
    for name, raw in sorted(entries.items()):
        if not raw.startswith(b'\x1bLua'):
            raw = raw.replace(b'\r\n', b'\n')
        compressed = zlib.compress(raw, 9)
        index += name.encode('ascii') + b'\0'
        index += struct.pack('<IIIBI', len(compressed), len(compressed), len(raw), 1, len(body))
        body += compressed
    compressed = zlib.compress(index, 9)
    return (b'Master of Magic\0' + bytes(14)
            + struct.pack('<IIII', len(body), 0, len(entries) + 7, 0x200)
            + body + struct.pack('<II', len(compressed), len(index)) + compressed)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('tables', type=Path, help='Validated server-specific navigation table directory')
    parser.add_argument('--output', type=Path, default=Path('navigation_repair.grf'))
    args = parser.parse_args()
    args.output.write_bytes(build(args.tables, Path(__file__).with_name('navi_f.lub')))
    print(f'Built {args.output}: 18 entries, {args.output.stat().st_size:,} bytes')
