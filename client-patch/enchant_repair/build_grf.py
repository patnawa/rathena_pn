"""Build the enchantment patch from its three source tables (Python 3, no dependencies)."""
from pathlib import Path
import struct
import zlib


def build(root: Path) -> bytes:
    entries = {
        r'data\luafiles514\lua files\Enchant\EnchantList.lub': 'EnchantList.lub',
        r'data\luafiles514\lua files\ItemDBNameTbl.lub': 'ItemDBNameTbl.lub',
        r'data\luafiles514\lua files\datainfo\LapineUpgradeBox.lub': 'LapineUpgradeBox.lub',
    }
    body = bytearray()
    table = bytearray()
    for name, source in sorted(entries.items()):
        raw = (root / 'source' / source).read_bytes()
        # Normalize text checkouts; compiled Lua must remain byte-for-byte intact.
        if not raw.startswith(b'\x1bLua'):
            raw = raw.replace(b'\r\n', b'\n')
        packed = zlib.compress(raw, 9)
        table += name.encode('ascii') + b'\0'
        table += struct.pack('<IIIBI', len(packed), len(packed), len(raw), 1, len(body))
        body += packed
    packed_table = zlib.compress(table, 9)
    header = b'Master of Magic\0' + bytes(14)
    header += struct.pack('<IIII', len(body), 0, len(entries) + 7, 0x200)
    return header + body + struct.pack('<II', len(packed_table), len(table)) + packed_table


if __name__ == '__main__':
    root = Path(__file__).resolve().parent
    output = root / 'enchant_repair.grf'
    temporary = output.with_suffix('.grf.tmp')
    try:
        temporary.write_bytes(build(root))
        temporary.replace(output)
    finally:
        if temporary.exists():
            temporary.unlink()
    print(f'Built {output} ({output.stat().st_size:,} bytes; 3 entries)')
