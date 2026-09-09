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
import unittest
from pathlib import Path
import tempfile
from release_checks import startup_errors, candidate_digest


class StartupGateTest(unittest.TestCase):
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
