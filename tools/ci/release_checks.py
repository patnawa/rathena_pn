#!/usr/bin/env python3
# ============================================================================
#  PN  /  DEVELOPMENT TOOLS
#  release_checks.py
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/release_checks.py
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

"""Fail closed on database syntax, focused regressions, and isolated startup errors."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import signal
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[2]
TESTS = (
    'interserver_reconnect_test.py',
    'bank_core_test.py',
    'bank_service_test.py',
    'release_checks_test.py',
    'bug_hunt_test.py',
    'scdata_reload_test.py', 'rodex_operation_test.py',
    'hotfix_bonus_regression.py', 'fly_wing_rental_regression.py',
    'aquila_cast_time_test.py',
    'dimension_autocast_runtime_test.py',
    'equipment_progression_test.py',
      'equipment_reform_transaction_test.py',
      'reform_commit_test.py',
    'refine_transaction_test.py',
    'episode_party_progression_test.py',
    'database_backup_test.py',
    'health_check_test.py',
    'main_office_test.py',
    'storage_sql_failure_test.py',
    'storage_native_audit_test.py',
    'mob_sql_schema_test.py',
    'client_archive_stack_test.py',
    'client_release_audit_test.py',
    'dynamic_reward_audit_test.py',
    'client_preflight_test.py',
    'instance_access_manifest_test.py',
    'instance_geometry_test.py',
    'warper_destination_test.py',
    'bioresearch_geometry_test.py',
    'alice_geometry_test.py',
    'alice_database_test.py',
    'kro_285_progression_test.py',
    'instance_combat_rules_test.py',
)
FULL_TESTS = (
    'npc_audit_fashion_test.py', 'chapter1_protection_test.py', 'instance_entry_native_test.py',
    'episode21_finale_flow_test.py', 'episode21_checkpoint_test.py',
    'mob_matk_range_test.py', 'immortal_instance_test.py',
    'instance_warper_test.py', 'airship_briefing_test.py',
    'bioresearch_test.py', 'alice_test.py',
)


def candidate_digest(root):
    """Include local import overrides as well as tracked candidate inputs."""
    digest = hashlib.sha256()
    files = [root / 'map-server']
    files.extend(root / name for name in ('char-server', 'login-server', 'web-server')
                 if (root / name).is_file())
    for directory in ('src', 'db', 'npc', 'conf', 'sql-files', 'tools', 'client-patch'):
        files.extend(p for p in (root / directory).rglob('*') if p.is_file()
                     and not any(part in ('obj', '__pycache__') for part in p.parts)
                     and p.suffix not in ('.o', '.a', '.pyc'))
    for path in sorted(files):
        digest.update(path.relative_to(root).as_posix().encode() + b'\0')
        digest.update(hashlib.sha256(path.read_bytes()).digest())
    return digest.hexdigest()


def startup_errors(text):
    text = re.sub(r'\x1b\[[0-?]*[ -/]*[@-~]', '', text)
    errors = []
    for line in text.splitlines():
        failure = re.search(r'\[Error\]|\[Fatal[^\]]*\]|AddressSanitizer|runtime error:|Segmentation fault', line, re.I)
        # ShowSQL emits a primary [SQL] severity. Connection announcements
        # instead use [Info]: [SQL]: or [Status]: [SQL]: and are not errors.
        severity = re.search(r'\[(Info|Status|Notice|Warning|Debug|SQL)\]:', line, re.I)
        sql_failure = severity and severity.group(1).lower() == 'sql'
        sql_db_error = re.search(r'\[SQL\]:\s*DB error\b', line, re.I)
        if failure or sql_failure or sql_db_error:
            errors.append(line)
    if "Server is 'ready' and listening" not in text and 'Map Server is now online' not in text:
        errors.append('Map server readiness marker missing')
    return errors


def save_report(report, path):
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_name(path.name + '.tmp')
    temporary.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    temporary.replace(path)


def run_process(command, stream, timeout):
    environment = os.environ.copy()
    # Python fixtures use assertions; an inherited optimization setting must
    # never turn a failing regression into a successful release check.
    environment.pop('PYTHONOPTIMIZE', None)
    with subprocess.Popen(command, cwd=ROOT, stdout=stream, stderr=subprocess.STDOUT,
                          env=environment, start_new_session=os.name == 'posix') as process:
        try:
            return process.wait(timeout=timeout)
        except BaseException:
            # Native fixtures spawn compiler/test children. Stop those as well
            # before returning control or recording a timeout/interruption.
            if os.name == 'posix':
                try:
                    os.killpg(process.pid, signal.SIGKILL)
                except ProcessLookupError:
                    pass
            else:
                process.kill()
            process.wait()
            raise


def run_logged_check(name, command, report, report_path, log_path, timeout=900, check=True):
    row = {'name': name, 'passed': False, 'status': 'running',
           'command': command, 'log': str(log_path)}
    report['checks'].append(row)
    save_report(report, report_path)
    started = time.monotonic()
    print(f'RUN: {name} (log: {log_path})', flush=True)
    try:
        log_path.parent.mkdir(parents=True, exist_ok=True)
        with log_path.open('wb') as stream:
            try:
                returncode = run_process(command, stream, timeout)
            except BaseException as exc:
                stream.write(('\nRunner error: ' + (str(exc) or type(exc).__name__) + '\n').encode('utf-8'))
                raise
        row.update(passed=returncode == 0, status='passed' if returncode == 0 else 'failed',
                   exit_code=returncode)
        if check and returncode:
            raise subprocess.CalledProcessError(returncode, command)
    except BaseException as exc:
        row.update(passed=False, status='failed', error=str(exc) or type(exc).__name__)
        raise
    finally:
        row['seconds'] = round(time.monotonic() - started, 2)
        save_report(report, report_path)
    return row


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--phase', choices=['source', 'full'], default='full')
    parser.add_argument('--startup-log', type=Path, help='Output log for the isolated map-server invocation; required for full')
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    if args.phase == 'full' and not args.startup_log:
        parser.error('--startup-log is required for the full release gate')
    if sys.platform != 'linux':
        parser.error('Run on Linux: the native regression tests require GCC and sanitizers')
    args.report = args.report.resolve()
    if args.startup_log:
        args.startup_log = args.startup_log.resolve()
        if args.startup_log == args.report:
            parser.error('--startup-log and --report must be different files')
    checks = []
    report = {'phase': args.phase, 'passed': False, 'checks': checks}
    save_report(report, args.report)
    try:
        if args.phase == 'full':
            report['candidate_sha256'] = candidate_digest(ROOT)
        yaml_check = {'name': 'database_yaml_syntax', 'passed': False, 'status': 'running', 'files': 0}
        checks.append(yaml_check)
        save_report(report, args.report)
        import yaml
        count = 0
        for path in sorted((ROOT / 'db').rglob('*.yml')):
            yaml_check['file'] = path.relative_to(ROOT).as_posix()
            with path.open(encoding='utf-8-sig') as stream:
                list(yaml.load_all(stream, Loader=yaml.CSafeLoader if hasattr(yaml, 'CSafeLoader') else yaml.SafeLoader))
            count += 1
            yaml_check['files'] = count
        yaml_check.pop('file', None)
        yaml_check.update(passed=True, status='passed')
        tests = TESTS + (FULL_TESTS if args.phase == 'full' else ())
        commands = [(name, [sys.executable, str(ROOT / 'tools/ci' / name)]) for name in tests]
        commands.append(('client_compat_assets', [sys.executable,
                         str(ROOT / 'client-patch/client_compat/validate.py'), '--assets-only']))
        log_directory = args.report.parent / (args.report.stem + '-logs')
        for index, (name, command) in enumerate(commands, 1):
            run_logged_check(name, command, report, args.report,
                             log_directory / f'{index:02d}-{Path(name).stem}.log')
        if args.phase == 'full':
            startup = run_logged_check('isolated_startup', [str(ROOT / 'map-server'), '--run-once'],
                                       report, args.report, args.startup_log, timeout=300, check=False)
            startup.update(passed=False, status='running')
            content = args.startup_log.read_bytes()
            errors = startup_errors(content.decode('utf-8', errors='replace'))
            if startup['exit_code']:
                errors.append(f"map-server exited with status {startup['exit_code']}")
            if candidate_digest(ROOT) != report['candidate_sha256']:
                errors.append('Candidate files changed during validation; rerun on a stable build')
            startup.update(passed=not errors, status='failed' if errors else 'passed',
                           sha256=hashlib.sha256(content).hexdigest(), errors=errors)
            if errors:
                raise ValueError('\n'.join(errors))
        report['passed'] = True
    except BaseException as exc:
        report['error'] = str(exc) or type(exc).__name__
        for row in checks:
            if row.get('status') == 'running':
                row.update(passed=False, status='failed', error=report['error'])
        raise
    finally:
        save_report(report, args.report)
    print(f"PASS: {args.phase} release checks ({len(checks)} checks). Report: {args.report}")


if __name__ == '__main__':
    main()
