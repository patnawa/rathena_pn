"""A failing check must not hide later results or produce a passing report."""
import json
import os
from pathlib import Path
import signal
import subprocess
import sys
import tempfile
import time
import unittest
from unittest.mock import patch

import bug_hunt


class BugHuntTests(unittest.TestCase):
    def run_fixture(self, folder, stamps, *arguments):
        root = Path(folder) / 'repo'
        ci = root / 'tools/ci'
        ci.mkdir(parents=True)
        (ci / 'pass.py').write_text('import os\nprint("check executed")\nprint(os.environ.get("LUA51", ""))',
                                   encoding='utf-8')
        output = Path(folder) / 'evidence'
        with patch.object(bug_hunt, 'ROOT', root), \
             patch.object(bug_hunt, 'SUITES', {'release': ['pass.py']}), \
             patch.object(bug_hunt, 'identity', side_effect=stamps) as capture, \
             patch.object(bug_hunt.sys, 'platform', 'linux'), \
             patch.object(bug_hunt.sys, 'argv', ['bug_hunt', '--output', str(output), *arguments]):
            result = bug_hunt.main()
        return result, json.loads((output / 'report.json').read_text(encoding='utf-8')), output, capture

    def stamp(self, client_hash='original'):
        return {'git_commit': 'test', 'input_sha256': {}, 'server_binary_sha256': {},
                'client': {'issues': [], 'loose_file_sha256': {'data/test.lua': client_hash}}}

    def test_changed_client_cannot_pass(self):
        with tempfile.TemporaryDirectory() as folder:
            client = Path(folder) / 'client'
            result, report, output, capture = self.run_fixture(
                folder, [self.stamp(), self.stamp('changed')], '--client-root', str(client))
            self.assertEqual(result, 1)
            self.assertFalse(report['stable_inputs'])
            self.assertFalse(report['offline_checks_passed'])
            self.assertEqual(capture.call_args.args, (client.resolve(),))
            self.assertEqual(json.loads((output / 'identity-final.json').read_text(encoding='utf-8')),
                             self.stamp('changed'))

    def test_unchanged_client_can_pass(self):
        with tempfile.TemporaryDirectory() as folder:
            result, report, _, _ = self.run_fixture(
                folder, [self.stamp(), self.stamp()], '--client-root', str(Path(folder) / 'client'))
            self.assertEqual(result, 0)
            self.assertTrue(report['stable_inputs'])
            self.assertTrue(report['offline_checks_passed'])

    def test_timeout_is_logged_and_reported(self):
        with tempfile.TemporaryDirectory() as folder, \
             patch.object(bug_hunt, 'run_check', side_effect=subprocess.TimeoutExpired('fixture', 900)):
            result, report, output, _ = self.run_fixture(folder, [self.stamp(), self.stamp()])
            self.assertEqual(result, 1)
            self.assertEqual(report['checks'][0]['status'], 'failed')
            self.assertIn('timed out', (output / 'logs/pass.log').read_text(encoding='utf-8'))

    def test_lua_option_reaches_environment_based_fixtures(self):
        with tempfile.TemporaryDirectory() as folder:
            lua = Path(folder) / 'lua5.1'
            result, report, output, _ = self.run_fixture(
                folder, [self.stamp(), self.stamp()], '--lua', str(lua))
            self.assertEqual(result, 0)
            self.assertEqual(report['checks'][0]['environment']['LUA51'], str(lua.resolve()))
            self.assertIn(str(lua.resolve()), (output / 'logs/pass.log').read_text(encoding='utf-8'))

    def test_inherited_optimization_cannot_disable_assertions(self):
        with tempfile.TemporaryDirectory() as folder, patch.dict(os.environ, {'PYTHONOPTIMIZE': '2'}):
            log = Path(folder) / 'assertions.log'
            with log.open('wb') as stream:
                result = bug_hunt.run_check([sys.executable, '-c', 'assert False, "checks must run"'], stream)
            self.assertNotEqual(result, 0)
            self.assertIn('checks must run', log.read_text(encoding='utf-8'))

    @unittest.skipUnless(os.name == 'posix' and Path('/proc').is_dir(), 'Linux process-group regression')
    def test_timeout_stops_native_descendant(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            pid_file = root / 'child.pid'
            child = root / 'child.py'
            child.write_text('import os, pathlib, sys, time\n'
                             'pathlib.Path(sys.argv[1]).write_text(str(os.getpid()))\n'
                             'time.sleep(60)\n', encoding='utf-8')
            parent = root / 'parent.py'
            parent.write_text('import subprocess, sys, time\n'
                              'subprocess.Popen([sys.executable, sys.argv[1], sys.argv[2]])\n'
                              'time.sleep(60)\n', encoding='utf-8')
            child_pid = None
            try:
                with (root / 'fixture.log').open('wb') as stream:
                    with self.assertRaises(subprocess.TimeoutExpired):
                        bug_hunt.run_check([sys.executable, str(parent), str(child), str(pid_file)],
                                           stream, timeout=2)
                self.assertTrue(pid_file.is_file(), 'Fixture did not start its descendant')
                child_pid = int(pid_file.read_text())
                status = Path(f'/proc/{child_pid}/stat')
                deadline = time.monotonic() + 2
                while status.exists():
                    try:
                        state = status.read_text().split(') ', 1)[1].split()[0]
                    except FileNotFoundError:
                        break
                    if state == 'Z':
                        break
                    if time.monotonic() >= deadline:
                        self.fail('Timed-out fixture left its native descendant running')
                    time.sleep(0.02)
            finally:
                if child_pid is not None:
                    try:
                        os.kill(child_pid, signal.SIGKILL)
                    except ProcessLookupError:
                        pass

    def test_identity_failure_preserves_completed_checks(self):
        with tempfile.TemporaryDirectory() as folder:
            result, report, _, _ = self.run_fixture(folder, [self.stamp(), OSError('input removed')])
            self.assertEqual(result, 1)
            self.assertEqual(report['checks'][0]['status'], 'passed')
            self.assertFalse(report['offline_checks_passed'])
            self.assertIn('finished_utc', report)
            self.assertEqual(report['identity_errors'], [{'stage': 'final', 'error': 'input removed'}])

    def test_initial_identity_failure_leaves_a_report(self):
        with tempfile.TemporaryDirectory() as folder:
            result, report, _, _ = self.run_fixture(
                folder, [subprocess.CalledProcessError(128, ['git', 'rev-parse', 'HEAD'])])
            self.assertEqual(result, 1)
            self.assertFalse(report['offline_checks_passed'])
            self.assertEqual(report['checks'], [])
            self.assertEqual(report['identity_errors'][0]['stage'], 'initial')
            self.assertIn('finished_utc', report)

    def test_duplicate_area_does_not_overwrite_evidence(self):
        with tempfile.TemporaryDirectory() as folder:
            result, report, _, _ = self.run_fixture(
                folder, [self.stamp(), self.stamp()], '--area', 'release', '--area', 'release')
            self.assertEqual(result, 0)
            self.assertEqual(len(report['checks']), 1)

    def test_failure_keeps_later_evidence(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder) / 'repo'
            ci = root / 'tools/ci'
            ci.mkdir(parents=True)
            (ci / 'fail.py').write_text('print("reproduced defect"); raise SystemExit(7)')
            (ci / 'pass.py').write_text('print("later check executed")')
            output = Path(folder) / 'evidence'
            stamp = {'git_commit': 'test', 'input_sha256': {}, 'server_binary_sha256': {}}
            with patch.object(bug_hunt, 'ROOT', root), patch.object(bug_hunt, 'SUITES', {'release': ['fail.py', 'pass.py']}), \
                 patch.object(bug_hunt, 'identity', return_value=stamp), \
                 patch.object(bug_hunt.sys, 'platform', 'linux'), \
                 patch.object(bug_hunt.sys, 'argv', ['bug_hunt', '--output', str(output)]):
                self.assertEqual(bug_hunt.main(), 1)
            report = json.loads((output / 'report.json').read_text())
            self.assertEqual([r['status'] for r in report['checks']], ['failed', 'passed'])
            self.assertEqual(report['checks'][0]['exit_code'], 7)
            self.assertFalse(report['offline_checks_passed'])
            self.assertFalse(report['acceptance_complete'])
            self.assertIn('later check executed', (output / 'logs/pass.log').read_text())


if __name__ == '__main__':
    unittest.main()
