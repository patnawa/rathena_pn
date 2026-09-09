# ============================================================================
#  PN  /  CLIENT TOOLING
#  build_grf.py
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/client-patch/client_compat/build_grf.py
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

"""Build the reviewed episode resource archive without third-party dependencies."""
import hashlib
import json
from pathlib import Path
import struct
import zlib


def build(root):
    body, table = bytearray(), bytearray()
    rows = json.loads((root / 'assets.json').read_text())
    for row in sorted(rows, key=lambda r: r['archive_path_hex']):
        raw = (root / row['source']).read_bytes()
        assert hashlib.sha256(raw).hexdigest() == row['sha256'], row['source']
        name = bytes.fromhex(row['archive_path_hex'])
        assert name.startswith(b'data\\') and b'..' not in name and b'\0' not in name
        packed = zlib.compress(raw, 9)
        table += name + b'\0' + struct.pack('<IIIBI', len(packed), len(packed), len(raw), 1, len(body))
        body += packed
    packed_table = zlib.compress(table, 9)
    return (b'Master of Magic\0' + bytes(14) + struct.pack('<IIII', len(body), 0, len(rows) + 7, 0x200)
            + body + struct.pack('<II', len(packed_table), len(table)) + packed_table)


if __name__ == '__main__':
    root = Path(__file__).resolve().parent
    target = root / 'client_compat.grf'
    target.write_bytes(build(root))
    print(f'Built {target.name}: {target.stat().st_size} bytes')
