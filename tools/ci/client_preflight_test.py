"""Malformed client installations must produce diagnostics, never a false pass."""
import struct
import tempfile
import unittest
from pathlib import Path

from client_preflight import inspect


class ClientPreflightTests(unittest.TestCase):
    def check(self, ini, header=None, executable='Ragexe.exe'):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            (root / executable).touch()
            (root / 'DATA.INI').write_text(ini, encoding='utf-8')
            (root / 'data.grf').write_bytes(header if header is not None else
                b'Master of Magic\0' + bytes(26) + struct.pack('<I', 0x200))
            return inspect(root, '20260219')['issues']

    def test_valid_header(self):
        self.assertEqual(self.check('[Data]\n0=data.grf\n'), [])

    def test_empty_archives(self):
        self.assertTrue(self.check('[Data]\n'))

    def test_truncated_header(self):
        self.assertTrue(self.check('[Data]\n0=data.grf\n', b'Master of Magic\0'))

    def test_wrong_version(self):
        self.assertTrue(self.check('[Data]\n0=data.grf\n',
            b'Master of Magic\0' + bytes(26) + struct.pack('<I', 0x999)))

    def test_malformed_ini_reports_issues(self):
        for ini in ('[Data]\nx=data.grf\n', '[Data]\n0=data.grf\n0=data.grf\n',
                    '[Data]\n0=../data.grf\n', '[Data]\n0=%bad%.grf\n'):
            with self.subTest(ini=ini):
                self.assertTrue(self.check(ini))

    def test_setup_executable_does_not_replace_game(self):
        self.assertTrue(self.check('[Data]\n0=data.grf\n', executable='Setup.exe'))

    def test_oversized_archive_priority_reports_issue(self):
        issues = self.check('[Data]\n' + '9' * 5000 + '=data.grf\n')
        self.assertTrue(any('priority' in issue.lower() for issue in issues))


if __name__ == '__main__':
    unittest.main()
