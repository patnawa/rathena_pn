#!/usr/bin/env python3
"""Check backup publication and subprocess failures without a database or Docker."""
import gzip
import hashlib
import importlib.util
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

spec = importlib.util.spec_from_file_location('database_backup', Path(__file__).resolve().parents[1] / 'admin/database_backup.py')
backup = importlib.util.module_from_spec(spec)
spec.loader.exec_module(backup)


class BackupTests(unittest.TestCase):
    def test_binary_dump_compresses_and_verifies(self):
        data = b'CREATE TABLE test;\n' + bytes(range(256)) * 5000
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / 'source'; source.write_bytes(data)
            command = [sys.executable, '-c', 'import pathlib,sys;sys.stdout.buffer.write(pathlib.Path(sys.argv[1]).read_bytes())', str(source)]
            with patch.object(backup, 'db_command', return_value=command):
                digest = backup.dump('unused', 'unused', root / 'dump.gz')
            self.assertEqual(digest, hashlib.sha256(data).hexdigest())
            self.assertEqual(gzip.decompress((root / 'dump.gz').read_bytes()), data)

    def test_failed_dump_is_not_published(self):
        with tempfile.TemporaryDirectory() as directory:
            command = [sys.executable, '-c', 'import sys;sys.stdout.write("partial SQL");sys.exit(7)']
            with patch.object(backup, 'db_command', return_value=command), patch.object(sys, 'argv', ['backup', '--output', directory]):
                with self.assertRaisesRegex(RuntimeError, 'dump failed'):
                    backup.main()
            self.assertFalse(list(Path(directory).glob('*.sql.gz')))
            self.assertFalse(list(Path(directory).glob('*.partial')))
            reports = list(Path(directory).glob('*.json'))
            self.assertEqual(len(reports), 1)
            self.assertFalse(json.loads(reports[0].read_text())['passed'])

    def test_uses_locks_for_mixed_engines(self):
        with tempfile.TemporaryDirectory() as directory:
            command = [sys.executable, '-c', 'print("SQL")']
            with patch.object(backup, 'db_command', return_value=command) as call:
                backup.dump('test', 'ragnarok', Path(directory) / 'dump.gz')
            self.assertIn('--lock-all-tables', call.call_args.args[2])
            self.assertNotIn('--single-transaction', call.call_args.args[2])


if __name__ == '__main__':
    unittest.main()
