#!/usr/bin/env python3
# ============================================================================
#  PN  /  DEVELOPMENT TOOLS
#  release_checks_test.py
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/release_checks_test.py
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

"""Regression coverage for startup failures that can accompany exit code zero."""
from contextlib import contextmanager
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import MagicMock, patch
import release_checks
from release_checks import startup_errors, candidate_digest


class ReleaseRunnerTest(unittest.TestCase):
    @contextmanager
    def fixture(self, script):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory) / 'candidate'
            ci = root / 'tools/ci'
            ci.mkdir(parents=True)
            (ci / 'fixture.py').write_text(script, encoding='utf-8')
            compat = root / 'client-patch/client_compat'
            compat.mkdir(parents=True)
            (compat / 'validate.py').write_text('print("compatibility passed")', encoding='utf-8')
            report = Path(directory) / 'results/report.json'
            with patch.object(release_checks, 'ROOT', root), \
                 patch.object(release_checks, 'TESTS', ('fixture.py',)), \
                 patch.object(sys, 'platform', 'linux'), \
                 patch.dict(sys.modules, {'yaml': MagicMock()}), \
                 patch.object(sys, 'argv', ['release_checks', '--phase', 'source', '--report', str(report)]):
                yield report

    def test_optimization_cannot_hide_assertion_failure(self):
        with self.fixture('assert False, "regression must execute"') as path, \
             patch.dict(os.environ, {'PYTHONOPTIMIZE': '2'}):
            with self.assertRaises(subprocess.CalledProcessError):
                release_checks.main()
            report = json.loads(path.read_text(encoding='utf-8'))
            self.assertFalse(report['passed'])
            check = report['checks'][-1]
            self.assertEqual(check['name'], 'fixture.py')
            self.assertFalse(check['passed'])
            self.assertIn('regression must execute', Path(check['log']).read_text(encoding='utf-8'))

    def test_failure_retains_output_and_exit_status(self):
        with self.fixture('print("defect evidence"); raise SystemExit(7)') as path:
            with self.assertRaises(subprocess.CalledProcessError):
                release_checks.main()
            report = json.loads(path.read_text(encoding='utf-8'))
            check = report['checks'][-1]
            self.assertEqual(check['status'], 'failed')
            self.assertEqual(check['exit_code'], 7)
            self.assertIn('defect evidence', Path(check['log']).read_text(encoding='utf-8'))
            self.assertFalse(report['passed'])

    def test_timeout_retains_failed_check_and_diagnostic(self):
        with self.fixture('print("not reached")') as path, \
             patch.object(release_checks.subprocess, 'Popen',
                          side_effect=subprocess.TimeoutExpired(['fixture'], 900)):
            with self.assertRaises(subprocess.TimeoutExpired):
                release_checks.main()
            report = json.loads(path.read_text(encoding='utf-8'))
            check = report['checks'][-1]
            self.assertEqual(check['name'], 'fixture.py')
            self.assertEqual(check['status'], 'failed')
            self.assertFalse(check['passed'])
            self.assertIn('timed out', check['error'])
            self.assertIn('timed out', Path(check['log']).read_text(encoding='utf-8'))

    def test_missing_yaml_dependency_still_writes_report(self):
        with self.fixture('print("not reached")') as path, patch.dict(sys.modules, {'yaml': None}):
            with self.assertRaises(ModuleNotFoundError):
                release_checks.main()
            report = json.loads(path.read_text(encoding='utf-8'))
            self.assertFalse(report['passed'])
            self.assertIn('yaml', report['error'])

    def test_interruption_retains_failed_check(self):
        with self.fixture('print("not reached")') as path, \
             patch.object(release_checks.subprocess, 'Popen', side_effect=KeyboardInterrupt):
            with self.assertRaises(KeyboardInterrupt):
                release_checks.main()
            report = json.loads(path.read_text(encoding='utf-8'))
            check = report['checks'][-1]
            self.assertFalse(report['passed'])
            self.assertEqual(check['name'], 'fixture.py')
            self.assertEqual(check['status'], 'failed')
            self.assertEqual(check['error'], 'KeyboardInterrupt')
            self.assertIn('KeyboardInterrupt', Path(check['log']).read_text(encoding='utf-8'))

    def test_successful_source_run_retains_all_logs(self):
        with self.fixture('print("regression passed")') as path:
            release_checks.main()
            report = json.loads(path.read_text(encoding='utf-8'))
            self.assertTrue(report['passed'])
            self.assertEqual(len(report['checks']), 3)
            self.assertTrue(all(check['status'] == 'passed' for check in report['checks']))
            self.assertIn('regression passed', Path(report['checks'][1]['log']).read_text(encoding='utf-8'))
            self.assertIn('compatibility passed', Path(report['checks'][2]['log']).read_text(encoding='utf-8'))

    def test_full_startup_rejects_errors_timeouts_and_candidate_drift(self):
        for failure in ('diagnostic', 'timeout', 'drift'):
            with self.subTest(failure=failure), self.fixture('print("regression passed")') as path:
                binary = release_checks.ROOT / 'map-server'
                binary.write_bytes(b'candidate fixture')
                startup = path.with_name('startup.log')

                def execute(command, stream, timeout):
                    if command[0] != str(binary):
                        return 0
                    stream.write(b'Map Server is now online\n')
                    if failure == 'diagnostic':
                        stream.write(b'[Error]: rejected script\n')
                    elif failure == 'timeout':
                        raise subprocess.TimeoutExpired(command, timeout)
                    else:
                        binary.write_bytes(b'changed candidate')
                    return 0

                with patch.object(sys, 'argv', ['release_checks', '--report', str(path),
                                                '--startup-log', str(startup)]), \
                     patch.object(release_checks, 'FULL_TESTS', ()), \
                     patch.object(release_checks, 'run_process', side_effect=execute):
                    expected = subprocess.TimeoutExpired if failure == 'timeout' else ValueError
                    with self.assertRaises(expected):
                        release_checks.main()
                report = json.loads(path.read_text(encoding='utf-8'))
                self.assertFalse(report['passed'])
                check = report['checks'][-1]
                self.assertEqual(check['name'], 'isolated_startup')
                self.assertFalse(check['passed'])
                self.assertEqual(check['status'], 'failed')
                if failure == 'timeout':
                    self.assertIn('timed out', check['error'])
                    self.assertIn('timed out', startup.read_text(encoding='utf-8'))
                else:
                    self.assertEqual(check['sha256'], hashlib.sha256(startup.read_bytes()).hexdigest())
                    self.assertTrue(check['errors'])


class StartupGateTest(unittest.TestCase):
    def test_character_and_login_binary_drift(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'map-server').write_bytes(b'map')
            for name in ('char-server', 'login-server', 'web-server'):
                with self.subTest(name=name):
                    before = candidate_digest(root)
                    binary = root / name
                    binary.write_bytes(b'first')
                    added = candidate_digest(root)
                    self.assertNotEqual(before, added)
                    binary.write_bytes(b'second')
                    self.assertNotEqual(added, candidate_digest(root))

    def test_ready_plain_and_colored(self):
        for text in ("Server is 'ready' and listening", "Server is '\x1b[32mready\x1b[0m' and listening", 'Map Server is now online'):
            with self.subTest(text=text):
                self.assertEqual(startup_errors(text), [])

    def test_empty_and_truncated_logs_fail(self):
        for text in ('', 'Loading NPCs...', "Server is 'rea"):
            with self.subTest(text=text):
                self.assertTrue(startup_errors(text))

    def test_ready_does_not_hide_errors(self):
        for error in ('[Error] invalid NPC', '[Fatal Error] SQL failed', '[SQL]: DB error - invalid table', '[Error]: [SQL]: Failed to connect', '[Info]: [SQL]: DB error - invalid table', '[SQL]: Access denied for user', '\x1b[31m[Error]\x1b[0m invalid item', 'AddressSanitizer: invalid read', 'runtime error: invalid shift', 'Segmentation fault'):
            with self.subTest(error=error):
                self.assertTrue(startup_errors(error + '\nMap Server is now online'))

    def test_sql_connection_announcements_are_not_diagnostics(self):
        log = ('[Info]: [SQL]: Connecting to the Log Database ragnarok At progression-test-db...\n'
               '[Status]: [SQL]: Successfully connected...\n'
               "Server is 'ready' and listening")
        self.assertEqual(startup_errors(log), [])

    def test_candidate_drift_includes_local_overrides_and_binary(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'map-server').write_bytes(b'candidate binary')
            override = root / 'conf/import/map_conf.txt'
            override.parent.mkdir(parents=True)
            override.write_text('map_port: 5121')
            before = candidate_digest(root)
            override.write_text('map_port: 5122')
            changed = candidate_digest(root)
            self.assertNotEqual(before, changed)
            (root / 'map-server').write_bytes(b'another binary')
            self.assertNotEqual(changed, candidate_digest(root))


if __name__ == '__main__':
    unittest.main()
