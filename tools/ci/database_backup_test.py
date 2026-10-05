#!/usr/bin/env python3
# ============================================================================
#  PN  /  DEVELOPMENT TOOLS
#  database_backup_test.py
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/database_backup_test.py
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

"""Check backup publication and subprocess failures without a database or Docker."""
import gzip
import hashlib
import importlib.util
import json
from pathlib import Path
import sys
import tempfile
import unittest
from subprocess import CompletedProcess
from unittest.mock import patch

spec = importlib.util.spec_from_file_location('database_backup', Path(__file__).resolve().parents[1] / 'admin/database_backup.py')
backup = importlib.util.module_from_spec(spec)
spec.loader.exec_module(backup)


class BackupTests(unittest.TestCase):
    def test_backup_waits_for_database_before_dumping(self):
        with tempfile.TemporaryDirectory() as directory:
            command = [sys.executable, '-c', 'print("SQL")']
            ready = [CompletedProcess([], 1), CompletedProcess([], 1), CompletedProcess([], 0)]
            with patch.object(backup, 'db_command', return_value=command), \
                 patch.object(backup.subprocess, 'run', side_effect=ready) as probe, \
                 patch.object(backup.time, 'sleep'), \
                 patch.object(sys, 'argv', ['backup', '--output', directory]):
                backup.main()
            self.assertEqual(probe.call_count, 3, 'Backup must tolerate an initializing database')
            reports = list(Path(directory).glob('*.json'))
            self.assertTrue(json.loads(reports[0].read_text())['passed'])

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
            with patch.object(backup, 'db_command', return_value=command), \
                 patch.object(backup, 'wait_database'), \
                 patch.object(sys, 'argv', ['backup', '--output', directory]):
                with self.assertRaisesRegex(RuntimeError, 'dump failed'):
                    backup.main()
            self.assertFalse(list(Path(directory).glob('*.sql.gz')))
            self.assertFalse(list(Path(directory).glob('*.partial')))
            reports = list(Path(directory).glob('*.json'))
            self.assertEqual(len(reports), 1)
            self.assertFalse(json.loads(reports[0].read_text())['passed'])

    def test_database_timeout_never_dumps_or_publishes(self):
        with tempfile.TemporaryDirectory() as directory:
            with patch.object(backup.subprocess, 'run', return_value=CompletedProcess([], 1)), \
                 patch.object(backup.time, 'monotonic', side_effect=[0, 0, 121]), \
                 patch.object(backup, 'dump') as dump, \
                 patch.object(sys, 'argv', ['backup', '--output', directory]):
                with self.assertRaisesRegex(RuntimeError, 'did not become ready'):
                    backup.main()
            dump.assert_not_called()
            self.assertFalse(list(Path(directory).glob('*.sql.gz*')))
            reports = list(Path(directory).glob('*.json'))
            data = json.loads(reports[0].read_text())
            self.assertFalse(data['passed'])
            self.assertEqual(data['failure_stage'], 'database-readiness')
            self.assertEqual(data['error_type'], 'RuntimeError')

    def test_readiness_queries_keep_password_out_of_argv(self):
        with patch.object(backup.subprocess, 'run', return_value=CompletedProcess([], 0)) as query:
            backup.wait_database('fixture-db')
        args = query.call_args.args[0]
        self.assertIn('SELECT 1', args)
        self.assertIn('--connect-timeout=5', args)
        self.assertIn('MYSQL_PWD', args[5])
        self.assertFalse(any(arg.startswith('--password=') for arg in args))
        self.assertLessEqual(query.call_args.kwargs['timeout'], 10)

    def test_uses_locks_for_mixed_engines(self):
        with tempfile.TemporaryDirectory() as directory:
            command = [sys.executable, '-c', 'print("SQL")']
            with patch.object(backup, 'db_command', return_value=command) as call:
                backup.dump('test', 'ragnarok', Path(directory) / 'dump.gz')
            self.assertIn('--lock-all-tables', call.call_args.args[2])
            self.assertNotIn('--single-transaction', call.call_args.args[2])


if __name__ == '__main__':
    unittest.main()
