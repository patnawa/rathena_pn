#!/usr/bin/env python3
# ============================================================================
#  PN  /  DEVELOPMENT TOOLS
#  episode20_21_questinfo_native_test.py
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/episode20_21_questinfo_native_test.py
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

"""Exact relocated declarations through native parser, OnInit, click and unload.

Generated fixtures only. No server, runtime mutation, broad-gate repin, or full
gameplay claim. Existing accepted native support objects require exact producer,
artifact and every engine/header source hash. Noncompiled earlier test/gate
sources are not dependencies of this independent fixture and are not reused.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import yaml

import episode20_21_questinfo_migration as migration
import episode21_family_supply_overlay as family_overlay
from garden_legacy_gate_test import WRAPS as GARDEN_WRAPS

ROOT = Path(__file__).resolve().parents[2]
BUILD_SHA = '077f5fd1c2ab24909c3d959b8f6292a9ec491290401d4a985b98938948dce0a8'
RECEIPT_SHA = 'bce8640ee1cd73b9fc91c9076f523762fd5223b99443059b0a0cec784514c9f1'
DRIVER = 'tools/ci/episode20_21_questinfo_native_test.cpp'
RUNNER = 'tools/ci/episode20_21_questinfo_native_test.py'
TOOLS = [DRIVER, RUNNER, 'tools/ci/episode20_21_questinfo_migration.py',
         'tools/ci/episode21_family_supply_overlay.py',
         'tools/ci/garden_legacy_gate_test.py', 'tools/ci/biosphere_material_callback_audit.py',
         'tools/ci/biosphere_callback_closure_audit.py']
HELPERS = {'EP20_MainComplete', 'EP20_DailyCanAccept', 'EP21_Started',
           'EP21_GaebolgComplete', 'EP21_CultComplete', 'EP21_GimliComplete',
           'EP21_GhostShipUnlocked', 'EP21_MainComplete', 'EP21_DailyKey',
           'EP21_QuestInRange', 'EP20_SyncStep', 'EP20_EffectiveStep', 'EP20_QuestInRange'}
WRAPS = (*GARDEN_WRAPS, '_Z27achievement_check_conditionP11script_codeP16map_session_data',
         '_Z21clif_quest_show_eventPK16map_session_dataPK10block_list17e_questinfo_types21e_questinfo_markcolor')


def sha(data):
    return hashlib.sha256(data).hexdigest()


def require(ok, message):
    if not ok:
        raise AssertionError(message)


def frozen(pins, read=None):
    if read is None:
        read = lambda p: Path(p).read_bytes()
    actual = {str(p): sha(read(p)) for p in pins}
    require(pins == actual, 'Frozen input/artifact drift: ' + ', '.join(p for p in pins if pins[p] != actual[p]))


def fixtures(build):
    summary, pairs = migration.verify(ROOT, expected_phase='after')
    functions = {}
    for path, pair in pairs.items():
        text = pair['after']
        _, masked = migration.lexical_views(text)
        for match in re.finditer(r'^function\tscript\t(\w+)\t\{', masked, re.M):
            if match[1] not in HELPERS:
                continue
            end = migration.body_end(masked, masked.index('{', match.start()))
            require(match[1] not in functions, 'Duplicate required helper')
            functions[match[1]] = text[match.start():end] + '\n'
    require(set(functions) == HELPERS, 'Exact Horuru and condition function closure required')
    calls = set()
    for name, text in functions.items():
        calls.update(re.findall(r'callfunc\s*\(?\s*"(EP\w+)"', text))
    require(calls <= HELPERS, 'Unreviewed transitive helper dependency')
    (build / 'helpers.txt').write_text('\n'.join(functions.values()))
    rows, phases = [], {'old': [], 'fixed': []}
    view_ids = set()
    for path, pair in pairs.items():
        for old in migration.inspect(pair['before'], path):
            for phase, key in (('old', 'before'), ('fixed', 'after')):
                members = migration.inspect(pair[key], path)
                found = [r for r in members if r['npc'] == old['npc']]
                require(len(found) == 1, 'Unique complete owner extraction')
                row = found[0]
                phases[phase].append(pair[key][pair[key].rfind('\n', 0, row['start']) + 1:row['end']] + '\n')
            source = pair['before'][old['line_end']:old['end']]
            if old['npc'] == 'Horuru#ep20_start':
                prefix = '\tcallfunc "EP20_SyncStep";\n'
                require(source.startswith(prefix), 'Exact Horuru prefix dependency')
                source = source[len(prefix):]
            match = re.match(r'\tmes "([^"\\\n]*)";', source)
            require(match, 'Every bounded click must stop at the literal first message')
            geometry = old['header'].split('\t')[0].split(',')
            view = old['header'].split('\t')[3].split(',')[0]
            if view.isdecimal():
                require(20000 <= int(view) < 30000, 'Review any other numeric view range')
                view_ids.add(int(view))
            rows.append('\t'.join((old['map'], old['npc'], geometry[1], geometry[2],
                                   old['icon'], old['color'], match[1])))
    require(len(rows) == 125 and all(len(v) == 125 for v in phases.values()), 'Full owner inventory')
    (build / 'owners.tsv').write_text('\n'.join(rows) + '\n')
    for phase, declarations in phases.items():
        (build / (phase + '.txt')).write_text('\n'.join(declarations))
    records = yaml.safe_load((ROOT / 'db/import/mob_db.yml').read_text())['Body']
    identities = []
    for view in sorted(view_ids):
        selected = [r for r in records if r['Id'] == view]
        require(len(selected) == 1, 'Exact existing mob-backed view identity')
        identities.append({k: selected[0][k] for k in ('Id', 'AegisName', 'Name')})
    (build / 'views.yml').write_text(yaml.safe_dump(identities, sort_keys=False))
    return summary, {name: sha(text.encode()) for name, text in functions.items()}


def output(result, phase):
    result.check_returncode()
    clean = re.sub(r'\x1b\[[0-9;]*[A-Za-z]', '', result.stdout)
    require(not result.stderr, 'Native stderr must be empty')
    require(clean.count('Memory manager: No memory leaks found.') == 1, 'Explicit clean allocator teardown')
    require(not re.search(r'\[(?:Error|Warning|Fatal)\]|AddressSanitizer|UndefinedBehaviorSanitizer|runtime error:|(?:invalid|double) free|Memory manager:(?! No memory leaks found\.)', clean, re.I), 'Native diagnostic')
    found = re.findall(r'EPISODE_QI_NATIVE_OK phase=(\w+) owners=(\d+) maps=(\d+) cycles=(\d+) clicks=(\d+) assertions=(\d+) failures=(\d+) errors=(\d+) condition_calls=(\d+)', clean)
    require(len(found) == 1 and found[0][0] == phase, 'Unique phase completion')
    owners, maps, cycles, clicks, assertions, failures, errors, conditions = map(int, found[0][1:])
    require((owners, maps, cycles, clicks, failures, errors) == (125, 27, 2, 500, 0, 0), 'Exact complete native lifecycle')
    require((assertions, conditions) == ((6216, 274) if phase == 'fixed' else (4964, 0)), 'Exact native registration and condition matrix')
    return dict(zip(('owners', 'maps', 'cycles', 'clicks', 'assertions', 'failures', 'errors', 'condition_calls'),
                    (owners, maps, cycles, clicks, assertions, failures, errors, conditions)))


def controls():
    good = ('Memory manager: No memory leaks found.\n'
            'EPISODE_QI_NATIVE_OK phase=fixed owners=125 maps=27 cycles=2 clicks=500 assertions=6216 failures=0 errors=0 condition_calls=274\n')
    output(subprocess.CompletedProcess([], 0, good, ''), 'fixed')
    bad = [good.replace('Memory manager: No memory leaks found.', ''), good + good,
           good.replace('clicks=500', 'clicks=499'), good.replace('assertions=6216', 'assertions=6215'),
           good.replace('condition_calls=274', 'condition_calls=273'), good.replace('phase=fixed', 'phase=old')]
    bad += [good + message for message in ('[Error]: unexpected', '[Warning]: unexpected',
            '[Fatal]: unexpected', 'runtime error: bad', 'AddressSanitizer: bad',
            'UndefinedBehaviorSanitizer: bad', 'Memory manager: invalid pointer', 'double free')]
    for text in bad:
        try:
            output(subprocess.CompletedProcess([], 0, text, ''), 'fixed')
        except AssertionError:
            pass
        else:
            raise AssertionError('Accepted incomplete/diagnostic zero-exit output')
    for code, stderr in ((0, 'unexpected stderr'), (1, '')):
        try:
            output(subprocess.CompletedProcess([], code, good, stderr), 'fixed')
        except (AssertionError, subprocess.CalledProcessError):
            pass
        else:
            raise AssertionError('Accepted failing exit or unexpected stderr')
    blobs = {p: p.encode() for p in ('source', 'header', 'object', 'archive', 'executable', 'fixture', 'producer')}
    pins = {p: sha(b) for p, b in blobs.items()}
    frozen(pins, blobs.__getitem__)
    for p in blobs:
        changed = {**blobs, p: blobs[p] + b'changed'}
        try:
            frozen(pins, changed.__getitem__)
        except AssertionError:
            pass
        else:
            raise AssertionError('Accepted mutated source/artifact')
    installed = (ROOT / family_overlay.NPC).read_bytes()
    canonical, _ = family_overlay.canonicalize_for_questinfo(
        family_overlay.NPC, installed)
    candidate = family_overlay.apply_to_qi_only(canonical)
    overlay_acceptances = overlay_refusals = 0
    for source, expected_overlay in ((canonical, 'qi-only'),
                                     (candidate, 'family-transaction')):
        for data in (source, source.replace(b'\n', b'\r\n')):
            phase, before, after, rows, overlay_phase = migration.active_pair(
                family_overlay.NPC, data)
            require(phase == 'after' and overlay_phase == expected_overlay and
                    len(rows) == 1 and sha(before.encode()) == migration.BEFORE[family_overlay.NPC] and
                    sha(after.encode()) == migration.AFTER[family_overlay.NPC],
                    'Installed Family overlay changed QuestInfo-only reconstruction')
            overlay_acceptances += 1
    for changed in (
            candidate + b' ',
            candidate.replace(b'questinfo_refresh();', b'questinfo_refresh(1);', 1),
            candidate.replace(b'@inventorylist_amount[.@match] <= 29990',
                              b'@inventorylist_amount[.@match] <= 29991', 1)):
        try:
            migration.active_pair(family_overlay.NPC, changed)
        except (AssertionError, UnicodeDecodeError):
            overlay_refusals += 1
        else:
            raise AssertionError('Accepted an unknown installed Family overlay')
    print('EPISODE_QI_GUARDS_OK: 16 output/exit, 7 frozen-input refusals, '
          f'{overlay_acceptances} overlay forms and {overlay_refusals} overlay refusals', flush=True)
    return {'output_exit_refusals': 16, 'frozen_input_refusals': 7,
            'family_overlay_acceptances': overlay_acceptances,
            'family_overlay_refusals': overlay_refusals}


def run(build, retained, reuse=False):
    build, retained = build.resolve(), retained.resolve()
    require(build != ROOT and ROOT not in build.parents, 'Build outside repository')
    require(not build.exists() or reuse, 'Default final proof requires a fresh build directory')
    guard_results = controls()
    require(sha((retained / 'build.json').read_bytes()) == BUILD_SHA and
            sha((retained / 'receipt.json').read_bytes()) == RECEIPT_SHA, 'Exact accepted retained producer')
    binding = json.loads((retained / 'build.json').read_text())
    engine = {p: pin for p, pin in binding['sources'].items() if p.startswith(('src/', '3rdparty/'))}
    require(len(engine) == 1645, 'Exact retained engine/header dependency inventory')
    for path, pin in engine.items():
        require(sha((ROOT / path).read_bytes()) == pin, 'Retained engine/header source drift: ' + path)
    frozen(binding['link_inputs_sha256'])
    source_pins = {str(ROOT / p): sha((ROOT / p).read_bytes()) for p in [*engine, *TOOLS, *migration.SELECTED,
                   'db/const.yml', 'db/import/const.yml', 'db/import/zero_cell_const.yml', 'db/import/mob_db.yml']}
    build.mkdir(parents=True, exist_ok=True)
    summary, helper_hashes = fixtures(build)
    source_pins.update({str(ROOT / p): pin for p, pin in summary['include_raw_sha256'].items()})
    fixture_paths = [build / p for p in ('helpers.txt', 'owners.tsv', 'old.txt', 'fixed.txt', 'views.yml')]
    fixture_pins = {str(p): sha(p.read_bytes()) for p in fixture_paths}
    require(all(len(os.path.relpath(p, ROOT).encode()) < 80 for p in fixture_paths), 'Native path-key limit')
    san = ['-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-omit-frame-pointer']
    flags = ['g++', '-std=c++17', '-O0', '-g', '-DPACKETVER=20260219', '-fno-strict-aliasing'] + san
    flags += ['-I' + p for p in ('src', '3rdparty/libconfig', '3rdparty/rapidyaml/src', '3rdparty/rapidyaml/ext/c4core/src', '3rdparty/json/include', '/usr/include/mysql')]
    fresh = []
    for source in ('src/map/script.cpp', 'src/common/malloc.cpp', DRIVER):
        target = build / (Path(source).stem + '.o')
        metadata = target.with_suffix('.source.json')
        code_pins = {p: source_pins[str(ROOT / p)] for p in engine}
        code_pins[source] = source_pins[str(ROOT / source)]
        previous = json.loads(metadata.read_text()) if reuse and metadata.exists() else None
        if previous and target.exists() and previous == {'sources': code_pins, 'object_sha256': sha(target.read_bytes())}:
            print('Reuse exact source/header/object bound sanitizer object: ' + source, flush=True)
        else:
            print('Fresh ASan/UBSan compile: ' + source, flush=True)
            subprocess.run(flags + ['-c', source, '-o', str(target)], cwd=ROOT, check=True)
            frozen(source_pins)
            metadata.write_text(json.dumps({'sources': code_pins, 'object_sha256': sha(target.read_bytes())}, indent=2) + '\n')
        fresh.append(target)
    excluded = {'script.o', 'malloc.o', 'npc.o', 'combined_document_test.o'}
    support = [Path(p) for p in binding['link_inputs_sha256'] if Path(p).name not in excluded]
    require(len(support) == len(binding['link_inputs_sha256']) - 4, 'Exact replacement of three native objects and old driver')
    inputs = fresh + support
    artifact_pins = {str(p): sha(p.read_bytes()) for p in inputs}
    executable = build / 'episode_qi_native_test'
    subprocess.run(['g++'] + san + ['-o', str(executable)] + list(artifact_pins) +
                   ['-Wl,--wrap=' + p for p in WRAPS] +
                   ['-lz', '-ldl', '-lmysqlclient', '-lzstd', '-lssl', '-lcrypto', '-lresolv', '-lm'], cwd=ROOT, check=True)
    artifact_pins[str(executable)] = sha(executable.read_bytes())
    artifact_pins.update(fixture_pins)
    artifact_pins[str(retained / 'build.json')] = BUILD_SHA
    artifact_pins[str(retained / 'receipt.json')] = RECEIPT_SHA
    results = {}
    for phase in ('fixed', 'old'):
        frozen(source_pins); frozen(artifact_pins)
        args = [str(executable)] + [os.path.relpath(build / p, ROOT) for p in ('helpers.txt', phase + '.txt', 'owners.tsv')] + [phase, os.path.relpath(build / 'views.yml', ROOT)]
        result = subprocess.run(args, cwd=ROOT, text=True, capture_output=True, timeout=120)
        (build / (phase + '.stdout.txt')).write_text(result.stdout)
        (build / (phase + '.stderr.txt')).write_text(result.stderr)
        print(result.stdout, flush=True); print(result.stderr, flush=True)
        results[phase] = output(result, phase)
        frozen(source_pins); frozen(artifact_pins)
    migration.verify(ROOT, expected_phase='after')
    receipt = {'result': 'NATIVE_QI_LIFECYCLE_PASS', 'native': results, 'source_only_migration': summary,
               'guard_controls': guard_results,
               'sources': source_pins, 'frozen_artifacts': artifact_pins, 'helper_sha256': helper_hashes,
               'fresh_sanitized': ['src/map/script.cpp', 'src/common/malloc.cpp', 'src/map/npc.cpp', DRIVER],
               'retained_support': 'Exact accepted document producer and all 1645 engine/header sources; old test drivers/gates not used',
               'outputs': {p.name: sha(p.read_bytes()) for p in build.glob('*.txt')},
               'boundaries': 'Native parser, OnInit, click, unload and condition VM; explicit world/packet doubles; first-message termination, empty-story loaded registry incl. real Horuru SyncStep; not full condition truth tables, real client UI, spatial collision, rewards or live gameplay; duplicate OnInit without unload intentionally duplicates conditions'}
    (build / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print('NATIVE_QI_LIFECYCLE_PASS ' + sha((build / 'receipt.json').read_bytes()), flush=True)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, required=True)
    parser.add_argument('--retained-document-build', type=Path, required=True)
    parser.add_argument('--reuse-objects', action='store_true')
    args = parser.parse_args()
    run(args.build_dir, args.retained_document_build, args.reuse_objects)
