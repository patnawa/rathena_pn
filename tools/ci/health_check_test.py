"""Behavioral checks for stale, failed, corrupt backups and stopped/erroring services."""
from datetime import datetime, timedelta, timezone
import hashlib
import importlib.util
import io
import json
from pathlib import Path
import sys
import tempfile
import unittest
from contextlib import redirect_stderr
from unittest.mock import patch

spec = importlib.util.spec_from_file_location('health', Path(__file__).resolve().parents[1]/'admin/health_check.py')
health = importlib.util.module_from_spec(spec); spec.loader.exec_module(health)


class HealthTests(unittest.TestCase):
    def metric(self, now, **changes):
        data = {key: 0 for key in ('window_ms', 'shop_pending', 'shop_oldest_ms', 'shop_started',
            'shop_committed', 'shop_rejected', 'shop_retry_attempts', 'shop_refresh_failures', 'shop_busy_refusals')}
        data.update(version=1, utc=int(now.timestamp()))
        data.update({prefix+suffix: 0 for prefix in ('timer_late', 'dispatch', 'shop_ack', 'shop_complete')
                     for suffix in ('_count', '_p50_ms', '_p95_ms', '_p99_ms', '_max_ms')})
        data.update(changes)
        return '[Info]: PN_METRICS '+json.dumps(data)

    def test_running_process_with_stalled_purchase_is_unhealthy(self):
        now = datetime.now(timezone.utc)
        state = {'Running': True, 'StartedAt': (now-timedelta(hours=1)).isoformat()}
        with patch.object(health, 'command', side_effect=[json.dumps(state), self.metric(now, shop_pending=1, shop_oldest_ms=31000)]):
            result = health.inspect_service('rathena-map', 15)
        self.assertTrue(result['running'])
        self.assertFalse(result['passed'])
        self.assertEqual(result['runtime']['latest']['shop_oldest_ms'], 31000)

    def test_metrics_freshness_schema_latency_and_bounded_history(self):
        now = datetime.now(timezone.utc)
        self.assertTrue(health.metrics_status('', now, 30)['passed'])
        self.assertFalse(health.metrics_status('', now, 121)['passed'])
        for stamp in (now-timedelta(seconds=151), now+timedelta(seconds=6)):
            self.assertFalse(health.metrics_status(self.metric(stamp), now, 300)['passed'])
        self.assertFalse(health.metrics_status(self.metric(now, version=True), now, 300)['passed'])
        self.assertFalse(health.metrics_status(self.metric(now)+'\n[Info]: PN_METRICS {bad}', now, 300)['passed'])
        self.assertFalse(health.metrics_status(self.metric(now, timer_late_p99_ms=1001), now, 300, 30, 1000)['passed'])
        result = health.metrics_status('\n'.join(self.metric(now) for _ in range(100)), now, 300)
        self.assertTrue(result['passed'])
        self.assertEqual(len(result['trend']), 15)

    def test_queue_metrics_versions_bounds_and_pending_consistency(self):
        now = datetime.now(timezone.utc)
        legacy = self.metric(now)
        self.assertTrue(health.metrics_status(legacy, now, 300)['passed'])
        for depth in (0, 1, 4, 32):
            with self.subTest(depth=depth):
                queued = self.metric(now, version=2, shop_queue_depth=depth, shop_pending=int(depth > 0))
                result = health.metrics_status(legacy+'\n'+queued, now, 300)
                self.assertTrue(result['passed'])
                self.assertEqual(result['latest']['shop_queue_depth'], depth)
        for changes in (
            {'version': 2},
            {'version': 2, 'shop_queue_depth': 33, 'shop_pending': 1},
            {'version': 2, 'shop_queue_depth': -1},
            {'version': 2, 'shop_queue_depth': True, 'shop_pending': 1},
            {'version': 2, 'shop_queue_depth': 0, 'shop_pending': 1},
            {'version': 2, 'shop_queue_depth': 1, 'shop_pending': 0},
            {'version': 2, 'shop_queue_depth': 0, 'unknown_field': 0},
            {'version': 1, 'shop_queue_depth': 0},
            {'version': 3, 'shop_queue_depth': 0},
        ):
            with self.subTest(changes=changes):
                result = health.metrics_status(legacy+'\n'+self.metric(now, **changes), now, 300)
                self.assertFalse(result['passed'])
                self.assertEqual(result['reason'], 'Malformed runtime metrics')

    def test_nonfinite_thresholds_fail_before_inspecting_services(self):
        for option in ('--max-backup-hours', '--min-free-gib', '--max-disk-percent'):
            for value in ('nan', 'inf', '-inf'):
                with self.subTest(option=option, value=value), \
                     patch.object(sys, 'argv', ['health', f'{option}={value}']), \
                     patch.object(health, 'inspect_service') as inspect, redirect_stderr(io.StringIO()):
                    with self.assertRaises(SystemExit) as error:
                        health.main()
                    self.assertEqual(error.exception.code, 2)
                    inspect.assert_not_called()

    def backup(self, root, now, **overrides):
        path = root/('ragnarok-'+now.strftime('%Y%m%dT%H%M%S%fZ')+'.sql.json')
        archive = path.with_suffix('.gz'); archive.write_bytes(b'verified compressed backup fixture')
        data = {'created_utc': now.strftime('%Y%m%dT%H%M%S%fZ'), 'passed': True,
                'restore': {'passed': True}, 'archive_bytes': archive.stat().st_size,
                'archive_sha256': hashlib.sha256(archive.read_bytes()).hexdigest()}
        data.update(overrides); path.write_text(json.dumps(data))
        return path

    def test_backup_integrity_and_expiration(self):
        now = datetime.now(timezone.utc)
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self.assertFalse(health.backup_status(root, now, 30)['passed'])
            path = self.backup(root, now-timedelta(hours=1))
            self.assertTrue(health.backup_status(root, now, 30)['passed'])
            self.assertFalse(health.backup_status(root, now+timedelta(hours=31), 30)['passed'])
            path.with_suffix('.gz').write_bytes(b'corrupt')
            self.assertFalse(health.backup_status(root, now, 30)['passed'])

    def test_new_failure_cannot_be_hidden_by_older_success(self):
        now = datetime.now(timezone.utc)
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); self.backup(root, now-timedelta(hours=1))
            latest = self.backup(root, now, passed=False)
            self.assertFalse(health.backup_status(root, now, 30)['passed'])
            latest.write_text('{invalid')
            self.assertFalse(health.backup_status(root, now, 30)['passed'])

    def test_restore_failure_future_and_missing_archive(self):
        now = datetime.now(timezone.utc)
        for overrides in ({'restore': {'performed': False}}, {'archive_sha256': '0'*64}):
            with tempfile.TemporaryDirectory() as tmp:
                root = Path(tmp); self.backup(root, now, **overrides)
                self.assertFalse(health.backup_status(root, now, 30)['passed'])
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); path = self.backup(root, now+timedelta(hours=2))
            self.assertFalse(health.backup_status(root, now, 30)['passed'])
            path.with_suffix('.gz').unlink()
            self.assertFalse(health.backup_status(root, now+timedelta(hours=2), 30)['passed'])

    def test_service_states_and_recent_errors(self):
        for state, logs, expected in (
            ({'Running': True}, '[Info]: [SQL]: connected', True),
            ({'Running': False}, '', False),
            ({'Running': True, 'Paused': True}, '', False),
            ({'Running': True, 'Health': {'Status': 'unhealthy'}}, '', False),
            ({'Running': True}, '\x1b[31m[Error]: connection failed', False),
            ({'Running': True}, '[Warning]: Connection to Char Server lost.', False),
            ({'Running': True}, "[Warning]: Unable to resolve char-server 'fixture'; retrying later.", False),
            ({'Running': True}, '[SQL]: DB error - private content', False)):
            state['StartedAt'] = (datetime.now(timezone.utc) - timedelta(hours=1)).isoformat()
            with patch.object(health, 'command', side_effect=[json.dumps(state), logs]):
                result = health.inspect_service('fixture', 15)
                self.assertEqual(result['passed'], expected)
                self.assertNotIn('private content', json.dumps(result))

    def test_missing_or_invalid_start_time_reports_failure(self):
        for fields in ({}, {'StartedAt': None}, {'StartedAt': 'invalid'},
                       {'StartedAt': '2026-09-22T00:00:00'}):
            with self.subTest(fields=fields), patch.object(
                    health, 'command', return_value=json.dumps({'Running': True, **fields})):
                self.assertFalse(health.inspect_service('fixture', 15)['passed'])

    def test_log_window_excludes_previous_process_but_keeps_recent_errors(self):
        now = datetime.now(timezone.utc)
        for started in (now - timedelta(minutes=2), now - timedelta(hours=2)):
            state = {'Running': True, 'StartedAt': started.isoformat()}
            with self.subTest(started=started), patch.object(
                    health, 'command', side_effect=[json.dumps(state), '[Error]: current failure']) as command:
                self.assertFalse(health.inspect_service('fixture', 15)['passed'])
                args = command.call_args.args[0]
                since = datetime.fromisoformat(args[args.index('--since') + 1])
                self.assertLess(abs((since - max(started, now - timedelta(minutes=15))).total_seconds()), 2)

    def test_backup_requires_boolean_success_flags(self):
        now = datetime.now(timezone.utc)
        for overrides in ({'passed': 'false'}, {'passed': 1},
                          {'restore': {'passed': 'false'}}, {'restore': {'passed': 1}}):
            with self.subTest(overrides=overrides), tempfile.TemporaryDirectory() as tmp:
                root = Path(tmp)
                self.backup(root, now, **overrides)
                self.assertFalse(health.backup_status(root, now, 30)['passed'])


if __name__ == '__main__':
    unittest.main()
