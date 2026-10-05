"""Exercise real archive parsing against damaged and overlapping fixtures."""
from pathlib import Path
import os
import struct
import sys
import tempfile
import unittest
import zlib

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from client_release_audit import archive_audit, index, lua_audit


def archive(name=b'data\\same.bmp', payload=b'original', alignment=0, version=0x200, stored=False):
    packed = payload if stored else zlib.compress(payload)
    layout = '<IIIBQ' if version == 0x300 else '<IIIBI'
    table = name + b'\0' + struct.pack(layout, len(packed), len(packed)+alignment, len(payload), 1, 0)
    compressed = zlib.compress(table)
    if version == 0x300:
        header = b'Event Horizon\0' + bytes(16) + struct.pack('<QII', len(packed) - 4, 1, version)
    else:
        header = b'Master of Magic\0' + bytes(14) + struct.pack('<IIII', len(packed), 0, 8, version)
    return header + packed + struct.pack('<II', len(compressed), len(table)) + compressed


class ArchiveAuditTests(unittest.TestCase):
    def fixture(self, files, ini=None):
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        root = Path(directory.name)
        for name, data in files.items():
            (root / name).write_bytes(data)
        (root / 'DATA.INI').write_text(ini or '[Data]\n' + ''.join(
            f'{i}={name}\n' for i, name in enumerate(files)))
        return root

    def test_case_and_slash_variants_report_winner_and_changed_payload(self):
        root = self.fixture({'first.grf': archive(), 'second.grf': archive(b'DATA/same.bmp', b'changed')})
        result, _ = archive_audit(root)
        self.assertFalse(result['errors'])
        self.assertEqual(result['collisions'][0]['winner'], 'first.grf')
        self.assertFalse(result['collisions'][0]['identical'])

    def test_identical_overrides_are_distinguished(self):
        result, _ = archive_audit(self.fixture({'a.grf': archive(), 'b.grf': archive()}))
        self.assertTrue(result['collisions'][0]['identical'])

    def test_omitted_final_alignment_padding_is_valid(self):
        result, _ = archive_audit(self.fixture({'a.grf': archive(alignment=3)}))
        self.assertFalse(result['errors'])

    def test_truncated_index_and_corrupt_payload_fail(self):
        for raw in (archive()[:-1], archive()[:46] + b'bad' + archive()[49:]):
            result, _ = archive_audit(self.fixture({'a.grf': raw}))
            self.assertTrue(result['errors'])

    def test_classic_payload_cannot_bypass_zlib_validation_with_equal_sizes(self):
        result, _ = archive_audit(self.fixture({'a.grf': archive(stored=True)}))
        self.assertTrue(result['errors'])
        self.assertEqual(result['archives'][0]['verified_payloads'], 0)

    def test_modern_stored_and_compressed_payloads_are_supported(self):
        for stored in (False, True):
            with self.subTest(stored=stored):
                result, _ = archive_audit(self.fixture({'a.grf': archive(version=0x300, stored=stored)}))
                self.assertFalse(result['errors'])
                self.assertEqual(result['archives'][0]['verified_payloads'], 1)

    def test_header_and_index_bounds_fail(self):
        raw = bytearray(archive())
        struct.pack_into('<I', raw, 30, 0xffffffff)
        for data in (bytes(raw), b'truncated'):
            root = self.fixture({'a.grf': data})
            with self.assertRaises(ValueError):
                index(root / 'a.grf')

    def test_invalid_stack_fails(self):
        for ini in ('[Data]\n1=a.grf\n', '[Data]\n0=../a.grf\n', '[Data]\n0=a.grf\n1=A.GRF\n'):
            with self.assertRaises(ValueError):
                archive_audit(self.fixture({'a.grf': archive()}, ini))


@unittest.skipUnless(os.environ.get('LUA51'), 'Set LUA51 to a Lua 5.1 executable')
class LuaAuditTests(unittest.TestCase):
    def run_loader(self, code):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp); (root/'SystemEN').mkdir()
            (root/'SystemEN/itemInfo.lua').write_text(code)
            return lua_audit(root, Path(os.environ['LUA51']), {})

    def test_empty_sentinel_registers_without_artwork_error(self):
        result=self.run_loader("tbl={[0]={identifiedResourceName='',unidentifiedResourceName=''}}; "
                               "function main() AddItem(0); return true end")
        self.assertEqual(result['registered_items'], 1)
        self.assertEqual(result['missing_archive_icons'], [])

    def test_declared_but_missing_table_fails(self):
        with self.assertRaisesRegex(ValueError, 'Missing declared table'):
            self.run_loader("tbl={}; ImportTables={'missing'}; function main() return true end")

    def test_registration_failure_is_not_a_pass(self):
        with self.assertRaisesRegex(ValueError, 'registration failed'):
            self.run_loader("tbl={}; function main() return false end")

    def test_duplicate_registration_cannot_hide_a_missing_item(self):
        with self.assertRaisesRegex(ValueError, 'Registered duplicate item'):
            self.run_loader("tbl={[0]={identifiedResourceName='',unidentifiedResourceName=''}, "
                            "[1]={identifiedResourceName='fixture',unidentifiedResourceName='fixture'}}; "
                            "function main() AddItem(0); AddItem(0); return true end")


if __name__ == '__main__':
    unittest.main()
