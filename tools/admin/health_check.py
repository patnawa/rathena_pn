#!/usr/bin/env python3
"""Read-only service, recent-error, backup-restore and disk checks for the Docker host."""
import argparse
from datetime import datetime, timezone, timedelta
import hashlib
import json
import math
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile

SERVICES = ('rathena-login', 'rathena-char', 'rathena-map', 'rathena-web', 'rathena-db', 'rathena-fluxcp', 'npm')
ERROR = re.compile(r'\[(?:Error|Fatal[^\]]*)\]|AddressSanitizer|runtime error:|\[SQL\]:\s*DB error', re.I)
RECONNECT = re.compile(r'Connection to (?:Char|Login)[ -]Server lost|Unable to resolve (?:char|login)-server', re.I)
METRICS = re.compile(r'^\s*\[Info\]:\s*PN_METRICS (\{.*\})\s*$')


def metrics_status(logs, now, uptime_seconds, max_pending_seconds=30, max_timer_p99_ms=None):
    samples = []
    fields = {'version', 'utc', 'window_ms', 'shop_pending', 'shop_oldest_ms',
              'shop_started', 'shop_committed', 'shop_rejected', 'shop_retry_attempts',
              'shop_refresh_failures', 'shop_busy_refusals'}
    fields.update(prefix + suffix for prefix in ('timer_late', 'dispatch', 'shop_ack', 'shop_complete')
                  for suffix in ('_count', '_p50_ms', '_p95_ms', '_p99_ms', '_max_ms'))
    for line in logs.splitlines():
        match = METRICS.match(line)
        if not match:
            continue
        try:
            data = json.loads(match.group(1))
            if set(data) != fields or data['version'] != 1 or any(type(value) is not int or value < 0 for value in data.values()):
                raise ValueError('Invalid metrics schema')
            if data['shop_pending'] not in (0, 1):
                raise ValueError('Invalid pending count')
            samples.append(data)
        except (ValueError, TypeError, KeyError):
            # Do not fall back to an earlier healthy sample after malformed output.
            return {'passed': False, 'reason': 'Malformed runtime metrics'}
    if not samples:
        return {'passed': uptime_seconds < 120, 'status': 'warming' if uptime_seconds < 120 else 'missing'}
    latest = samples[-1]
    age = now.timestamp() - latest['utc']
    reasons = []
    if age < -5 or age > 150:
        reasons.append('Runtime metrics heartbeat stale or dated in the future')
    if latest['shop_oldest_ms'] > max_pending_seconds * 1000:
        reasons.append('Stock purchase remains pending beyond configured threshold')
    if max_timer_p99_ms is not None and latest['timer_late_p99_ms'] > max_timer_p99_ms:
        reasons.append('Timer lateness exceeds configured p99 threshold')
    return {'passed': not reasons, 'reasons': reasons, 'age_seconds': round(age, 2), 'latest': latest,
            'trend': [{k: sample[k] for k in ('utc', 'timer_late_p99_ms', 'dispatch_p99_ms', 'shop_oldest_ms')}
                      for sample in samples[-15:]]}


def backup_status(directory, now, max_age_hours):
    reports = sorted(directory.glob('ragnarok-*.sql.json'), reverse=True)
    if not reports:
        return {'passed': False, 'reason': 'No database backup report found'}
    report_path = reports[0]
    try:
        data = json.loads(report_path.read_text())
        stamp = datetime.strptime(data['created_utc'], '%Y%m%dT%H%M%S%fZ').replace(tzinfo=timezone.utc)
        age = (now - stamp).total_seconds() / 3600
        archive = report_path.with_suffix('.gz')
        if data.get('passed') is not True or data.get('restore', {}).get('passed') is not True:
            raise ValueError('Latest backup or its restore verification failed')
        if age < -.1 or age > max_age_hours:
            raise ValueError('Latest restored backup is stale or dated in the future')
        if not archive.is_file() or archive.stat().st_size != data['archive_bytes']:
            raise ValueError('Backup archive missing or size does not match its report')
        with archive.open('rb') as stream:
            actual = hashlib.file_digest(stream, 'sha256').hexdigest()
        if actual != data['archive_sha256']:
            raise ValueError('Backup archive checksum mismatch')
        return {'passed': True, 'report': report_path.name, 'age_hours': round(age, 2),
                'restore_verified': True, 'archive_checksum_verified': True}
    except (OSError, ValueError, TypeError, KeyError, AttributeError) as error:
        return {'passed': False, 'report': report_path.name, 'reason': str(error)}


def command(args):
    result = subprocess.run(args, text=True, capture_output=True, timeout=25)
    if result.returncode:
        raise RuntimeError('Command failed: ' + args[0] + ' ' + args[1])
    return result.stdout + (result.stderr if args[:2] == ['docker', 'logs'] else '')


def inspect_service(name, log_minutes, max_pending_seconds=30, max_timer_p99_ms=None):
    try:
        # Select State only: a Docker inspect dump could disclose environment secrets.
        state = json.loads(command(['docker', 'inspect', '--format', '{{json .State}}', name]))
        ready = state.get('Running') is True and not state.get('Paused') and not state.get('Restarting')
        health = state.get('Health', {}).get('Status', 'not-configured')
        ready = ready and health not in ('unhealthy', 'starting') and not state.get('OOMKilled')
        # Docker keeps the previous process's logs across a container restart.
        # A planned shutdown disconnect must not fail a healthy new process.
        # Retain every error within both the lookback and current process lifetime.
        since = datetime.now(timezone.utc) - timedelta(minutes=log_minutes)
        started = datetime.fromisoformat(state['StartedAt'].replace('Z', '+00:00'))
        since = max(since, started)
        logs = command(['docker', 'logs', '--since', since.isoformat(), '--tail', '2000', name])
        clean = re.sub(r'\x1b\[[0-?]*[ -/]*[@-~]', '', logs)
        failures = sum(bool(ERROR.search(line) or RECONNECT.search(line)) for line in clean.splitlines())
        # Only counts are reported; SQL and client log lines may contain private data.
        result = {'passed': ready and failures == 0, 'running': ready,
                  'docker_health': health, 'recent_error_lines': failures}
        if name == 'rathena-map':
            now = datetime.now(timezone.utc)
            result['runtime'] = metrics_status(clean, now, (now-started).total_seconds(),
                                               max_pending_seconds, max_timer_p99_ms)
            result['passed'] = result['passed'] and result['runtime']['passed']
        return result
    except (OSError, ValueError, TypeError, KeyError, AttributeError, subprocess.SubprocessError, RuntimeError) as error:
        return {'passed': False, 'reason': str(error)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--backup-dir', type=Path, default=Path('/app/rathena-database-backups'))
    parser.add_argument('--report', type=Path)
    parser.add_argument('--max-backup-hours', type=float, default=30)
    parser.add_argument('--min-free-gib', type=float, default=5)
    parser.add_argument('--max-disk-percent', type=float, default=90)
    parser.add_argument('--log-minutes', type=int, default=15)
    parser.add_argument('--max-shop-pending-seconds', type=int, default=30)
    parser.add_argument('--max-timer-p99-ms', type=int, help='Optional threshold chosen from a measured load baseline')
    args = parser.parse_args()
    if (not all(math.isfinite(value) for value in (args.max_backup_hours, args.min_free_gib, args.max_disk_percent))
            or args.max_backup_hours <= 0 or args.min_free_gib < 0
            or not 0 < args.max_disk_percent <= 100 or args.log_minutes <= 0
            or args.max_shop_pending_seconds <= 0
            or (args.max_timer_p99_ms is not None and args.max_timer_p99_ms <= 0)):
        parser.error('Invalid health-check thresholds')
    now = datetime.now(timezone.utc)
    report = {'checked_utc': now.isoformat(), 'checks': {}}
    for name in SERVICES:
        report['checks'][name] = inspect_service(name, args.log_minutes, args.max_shop_pending_seconds, args.max_timer_p99_ms)
    report['checks']['backup'] = backup_status(args.backup_dir, now, args.max_backup_hours)
    try:
        usage = shutil.disk_usage(args.backup_dir)
        free = usage.free / 1024**3
        percent = usage.used * 100 / usage.total
        report['checks']['disk'] = {'passed': free >= args.min_free_gib and percent <= args.max_disk_percent,
                                    'free_gib': round(free, 2), 'used_percent': round(percent, 1)}
    except OSError:
        report['checks']['disk'] = {'passed': False, 'reason': 'Backup filesystem unavailable'}
    try:
        active = command(['systemctl', 'is-active', 'rathena-database-backup.timer']).strip() == 'active'
        report['checks']['backup_timer'] = {'passed': active}
    except (OSError, subprocess.SubprocessError, RuntimeError):
        report['checks']['backup_timer'] = {'passed': False, 'reason': 'Backup timer is not active'}
    report['passed'] = all(row['passed'] for row in report['checks'].values())
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True, mode=0o700)
        with tempfile.NamedTemporaryFile(mode='w', dir=args.report.parent, prefix='.health-', delete=False) as stream:
            temporary = Path(stream.name)
            try:
                json.dump(report, stream, indent=2)
                stream.write('\n'); stream.flush(); os.fsync(stream.fileno())
                stream.close()
                os.replace(temporary, args.report)
            finally:
                temporary.unlink(missing_ok=True)
    print(json.dumps(report, indent=2))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
