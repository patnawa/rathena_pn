"""Run bounded, offline bug hunts and preserve every result, including failures.

No server is started, no deployment occurs, and no player data is modified.
Native fixtures use explicit world/network doubles, not rendered playthroughs.
"""
import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import signal
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[2]
SUITES = {
    'transactions': ['equipment_reform_transaction_test.py', 'reform_commit_test.py', 'refine_transaction_test.py',
                     'grade_system_test.py', 'equipment_progression_test.py', 'bank_core_test.py', 'bank_service_test.py'],
    'party': ['episode_party_progression_test.py', 'instance_access_manifest_test.py'],
    'client': ['client_archive_stack_test.py', 'client_preflight_test.py',
               'audit_druid_client_test.py', 'audit_druid_integration_test.py',
               'client_release_audit_test.py',
               'legacy_quest_navigation_test.py', 'client_resource_repair_test.py',
               'bank_item_metadata_test.py'],
    'recovery': ['database_backup_test.py', 'storage_sql_failure_test.py',
                 'scdata_reload_test.py', 'rodex_operation_test.py', 'health_check_test.py',
                 'storage_native_audit_test.py'],
    'combat': ['element_system_test.py', 'hotfix_bonus_regression.py',
               'instance_combat_rules_test.py', 'aquila_cast_time_test.py',
               'sealed_performer_cards_test.py', 'soul_combo_card_audit.py'],
    'release': ['bug_hunt_test.py', 'release_checks_test.py', 'mob_sql_schema_test.py',
                'char_shared_header_dependency_test.py'],
}
NATIVE = {
    'party': ['episode21_finale_flow_test.py', 'episode21_checkpoint_test.py',
              'instance_entry_native_test.py'],
    'transactions': ['rune_tablet_transaction_test.py', 'npc_audit_fashion_test.py'],
}
ACCEPTANCE = {
    'transactions': 'Rendered menus; exact inventory and zeny after disconnect, repeated confirmation, and full inventory.',
    'party': 'Two real clients overlap dialogues, reconnect at checkpoints, change leader, and attempt repeated claims.',
    'client': 'Rendered sprites, descriptions, navigation, and active GRF override results.',
    'recovery': 'Isolated server/host crash during transactions; restore SQL backup and compare persistent state.',
    'combat': 'Controlled full combat encounters measuring damage, stacking, cast time, and cooldown.',
    'release': 'Fresh full build and isolated startup, then comparison with installed server/client hashes.',
}


def sha256(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def identity(client_root=None):
    revision = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip()
    dirty = subprocess.check_output(['git', 'status', '--porcelain'], cwd=ROOT, text=True).splitlines()
    # Hash all current inputs, including ignored imports and untracked source.
    inputs = {}
    for directory in ('src', 'db', 'npc', 'conf', 'sql-files', 'tools', 'client-patch', '.github/workflows'):
        for path in sorted((ROOT / directory).rglob('*')):
            if path.is_file() and not any(p in ('obj', '__pycache__') for p in path.parts) and path.suffix not in ('.o', '.a', '.pyc'):
                inputs[path.relative_to(ROOT).as_posix()] = sha256(path)
    binaries = {name: sha256(ROOT / name) if (ROOT / name).is_file() else None
                for name in ('map-server', 'char-server', 'login-server', 'web-server')}
    result = {'git_commit': revision, 'working_tree_changes': dirty, 'input_sha256': inputs,
              'server_binary_sha256': binaries, 'binary_source_match': 'not established by this offline run'}
    if client_root:
        from client_preflight import inspect
        result['client'] = inspect(client_root, '20260219', hash_archives=True)
        result['client']['loose_file_sha256'] = {
            p.relative_to(client_root).as_posix(): sha256(p)
            for name in ('System', 'SystemEN', 'data', 'NavigationData', 'tools/client')
            for p in sorted((client_root / name).rglob('*')) if p.is_file()}
        result['client']['core_sha256'] = {name: sha256(client_root / name)
            for name in ('Ragexe.exe', 'DATA.INI') if (client_root / name).is_file()}
    return result


def save(report, output):
    (output / 'report.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    lines = ['# Bug hunt results', '', f"Started: {report['started_utc']}", '',
             f"Offline checks passed: {report['offline_checks_passed']}", '',
             '| Area | Check | Result | Evidence |', '|---|---|---|---|']
    for row in report['checks']:
        lines.append(f"| {row['area']} | {row['name']} | {row['status']} | [{row['log']}]({row['log']}) |")
    if report.get('identity_errors'):
        lines += ['', '## Identity capture failures', '']
        lines += [f"- {error['stage']}: {error['error']}" for error in report['identity_errors']]
    if report.get('finished_utc') and not report['stable_inputs']:
        lines += ['', 'Input stability was not established; this run cannot pass.']
    lines += ['', '## Acceptance still required', '']
    lines += [f'- **{area}:** {task}' for area, task in ACCEPTANCE.items()]
    lines += ['', 'Passing fixtures do not close the acceptance items above. See identity.json for exact local inputs.', '']
    (output / 'dashboard.md').write_text('\n'.join(lines), encoding='utf-8')


def capture_identity(report, output, client_root, stage):
    try:
        snapshot = identity(client_root)
        filename = 'identity.json' if stage == 'initial' else 'identity-final.json'
        (output / filename).write_text(json.dumps(snapshot, indent=2) + '\n', encoding='utf-8')
        return snapshot
    except (OSError, subprocess.SubprocessError, UnicodeError) as exc:
        report['identity_errors'].append({'stage': stage, 'error': str(exc)})
        return None


def run_check(command, stream, timeout=900, environment=None):
    # A fixture may launch a compiler or native executable. Stop its whole
    # process group on timeout/interruption before running any later checks.
    check_environment = os.environ.copy()
    check_environment.update(environment or {})
    # Several fixtures use Python assertions as their regression checks.
    check_environment.pop('PYTHONOPTIMIZE', None)
    with subprocess.Popen(command, cwd=ROOT, stdout=stream, stderr=subprocess.STDOUT,
                          env=check_environment,
                          start_new_session=os.name == 'posix') as process:
        try:
            return process.wait(timeout=timeout)
        except BaseException:
            if os.name == 'posix':
                try:
                    os.killpg(process.pid, signal.SIGKILL)
                except ProcessLookupError:
                    pass
            else:
                process.kill()
            process.wait()
            raise


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True, help='New evidence directory outside the repository')
    parser.add_argument('--native', action='store_true', help='Also compile native VM fixtures; requires Linux map support objects')
    parser.add_argument('--client-root', type=Path)
    parser.add_argument('--lua', type=Path, help='Lua 5.1 executable for the installed quest loader')
    parser.add_argument('--area', choices=list(SUITES), action='append')
    args = parser.parse_args()
    if sys.platform != 'linux':
        parser.error('Run under Linux/WSL; native handler checks require GCC and sanitizers')
    output = args.output.resolve()
    if output.is_relative_to(ROOT):
        parser.error('Evidence must be outside the repository to avoid fingerprinting its own output')
    output.mkdir(parents=True, exist_ok=False)
    (output / 'logs').mkdir()
    report = {'started_utc': datetime.now(timezone.utc).isoformat(), 'checks': [],
              'offline_checks_passed': False, 'acceptance_complete': False,
              'stable_inputs': False, 'identity_errors': []}
    save(report, output)
    client_root = args.client_root.resolve() if args.client_root else None
    baseline = capture_identity(report, output, client_root, 'initial')
    if baseline is None:
        report['finished_utc'] = datetime.now(timezone.utc).isoformat()
        save(report, output)
        return 1
    for area in dict.fromkeys(args.area or SUITES):
        for name in SUITES[area] + (NATIVE.get(area, []) if args.native else []):
            log = 'logs/' + Path(name).stem + '.log'
            row = {'area': area, 'name': name, 'log': log, 'status': 'running',
                   'command': [sys.executable, str(ROOT / 'tools/ci' / name)],
                   'evidence_level': 'native VM fixture' if name in NATIVE.get(area, []) else 'source or isolated handler fixture'}
            report['checks'].append(row)
            if args.lua:
                row['environment'] = {'LUA51': str(args.lua.resolve())}
            if name == 'rune_tablet_transaction_test.py':
                row['command'] += ['--build-dir', str(output / 'native-rune')]
            if name in ('legacy_quest_navigation_test.py', 'client_resource_repair_test.py',
                        'bank_item_metadata_test.py'):
                if not args.lua or not args.client_root:
                    row.update(status='blocked', error='Requires --lua and --client-root')
                    (output / log).write_text(row['error'] + '\n', encoding='utf-8')
                    save(report, output)
                    continue
                row['command'] += ['--lua', str(args.lua.resolve()), '--client', str(args.client_root.resolve())]
                if name in ('client_resource_repair_test.py', 'bank_item_metadata_test.py'):
                    row['command'].append('--require-installed')
            save(report, output)
            start = time.monotonic()
            with (output / log).open('wb') as stream:
                try:
                    returncode = run_check(row['command'], stream, environment=row.get('environment'))
                    row.update(status='passed' if returncode == 0 else 'failed', exit_code=returncode)
                except (OSError, subprocess.TimeoutExpired) as exc:
                    row.update(status='failed', error=str(exc))
                    stream.write(('\nRunner error: ' + str(exc) + '\n').encode('utf-8'))
            row['seconds'] = round(time.monotonic() - start, 2)
            print(area, name, row['status'], flush=True)
            save(report, output)
    final = capture_identity(report, output, client_root, 'final')
    keys = ['git_commit', 'input_sha256', 'server_binary_sha256']
    if client_root:
        keys.append('client')
    report['stable_inputs'] = final is not None and all(final[key] == baseline[key] for key in keys)
    report['offline_checks_passed'] = report['stable_inputs'] and all(r['status'] == 'passed' for r in report['checks']) and not baseline.get('client', {}).get('issues')
    report['finished_utc'] = datetime.now(timezone.utc).isoformat()
    save(report, output)
    return 0 if report['offline_checks_passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
