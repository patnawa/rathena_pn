# ============================================================================
#  PN  /  DEVELOPMENT TOOLS
#  client_archive_stack_test.py
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/client_archive_stack_test.py
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

"""Regression checks for the ten-slot client archive limit and overlay merging."""
from pathlib import Path
import configparser
import struct
import sys
import tempfile
import unittest
import zlib

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT/'client-patch/client_compat'))
from merge_grfs import merge, read
from client_preflight import inspect


def archive(name, payload):
    packed = zlib.compress(payload)
    table = name + b'\0' + struct.pack('<IIIBI', len(packed), len(packed), len(payload), 1, 0)
    index = zlib.compress(table)
    return b'Master of Magic\0' + bytes(14) + struct.pack('<IIII', len(packed), 0, 8, 0x200) + packed + struct.pack('<II', len(index), len(table)) + index


class ClientArchiveStack(unittest.TestCase):
    def test_shipping_archive_list_fits_client_slots(self):
        ini = configparser.ConfigParser()
        ini.read(ROOT/'client-patch/client_compat/DATA.INI')
        keys = sorted(int(key) for key in ini['Data'])
        self.assertEqual(keys, list(range(len(keys))))
        self.assertLessEqual(len(keys), 10)
        self.assertIn('data.grf', ini['Data'].values())

    def test_merging_preserves_payloads_and_priority(self):
        first = archive(b'data\\same.lub', b'high priority')
        second = archive(b'data\\same.lub', b'lower priority')
        korean = b'data\\\xbe\xc6\\other.bmp'
        third = archive(korean, bytes(range(256)))
        self.assertEqual(read(merge([first, second, third])),
                         {b'data\\same.lub': b'high priority', korean: bytes(range(256))})

    def test_truncated_archive_is_rejected(self):
        good = archive(b'data\\test.lub', b'value')
        with self.assertRaises(ValueError):
            merge([good[:-1]])

    def test_base_archive_must_fit_inside_ten_slots(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root/'Ragexe.exe').touch()
            for i in range(10):
                (root/f'patch{i}.grf').write_bytes(b'Master of Magic\0')
            (root/'data.grf').write_bytes(b'Master of Magic\0')
            def check(count):
                names=[f'patch{i}.grf' for i in range(count-1)]+['data.grf']
                (root/'DATA.INI').write_text('[Data]\n'+''.join(f'{i}={name}\n' for i,name in enumerate(names)))
                return inspect(root,'20260219')['issues']
            self.assertEqual(check(10), [])
            self.assertTrue(any('limit exceeded' in error for error in check(11)))


if __name__ == '__main__':
    unittest.main()
