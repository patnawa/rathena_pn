#!/usr/bin/env python3
"""Pinned source proposal and isolated native proof for Mandel's supply service.

This runner never installs the proposal.  It accepts only the reviewed runtime
with the 125-owner QuestInfo migration already present, emits a candidate outside
the repository, proves an exact inverse, and executes the genuine script VM,
quest, inventory, reputation, and QuestInfo paths with deterministic daily keys.
"""
from __future__ import annotations

import argparse
from concurrent.futures import ThreadPoolExecutor
import difflib
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys
from unittest import mock

import yaml

from audit_enchant_upgrades import renewal_records
from biosphere_crown_transaction_test import WRAPPERS
import episode20_21_questinfo_migration as qi_migration
import episode21_family_supply_overlay as qi_overlay


ROOT = Path(__file__).resolve().parents[2]
NPC = 'npc/custom/episode21/FamilyReputation.txt'
MAIN = 'npc/custom/episode21/BlackHairedBeast.txt'
DAILY = 'npc/custom/episode21/MysteriousGhostShip.txt'
PREFIX = 'tools/ci/biosphere_crown_transaction_test.cpp'
DRIVER = 'tools/ci/episode21_family_supply_test.cpp'

PRE_QI_SHA = 'd49ef88c4d7b2ef1a16fccff22f0f13aa82bf790c9a6a92a42ef66b4d8c39f17'
CURRENT_SHA = 'c72019f51033d96054c277d952d993a45a68062da9f7b4ab99ee76edde2a9605'
CANDIDATE_SHA = '3e6217fa35f57cc1969810938b523d2970b4982e6ce0c383aefea26a2d266aeb'
MAIN_SHA = 'b00fdd28cd0ea4703bbbfdfb8a3f7336f74d38be02b93eddde30029996593b53'
DAILY_SHA = '011f74d5b6abc6833be6dbb70cd19c7b123ed057a4a9bbe677528aa85f9318a6'
DATA_SHA = {
    'db/re/item_db_etc.yml': '385e14a6ce2a943ee847cc6ae876390cc4aeac484e77491eaf83962f44cac137',
    'db/import/quest_db.yml': '55d75cfefaaabc4d08c774ef95c56332b16648237331764e8a86283530ae0c43',
    'db/import/reputation.yml': '83cace0cda525350959195af10f80c67db1f3ea5154cd48ac4051f0a571e3272',
}


def require(ok, message):
    if not ok:
        raise AssertionError(message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def normalize(raw):
    result = raw.replace(b'\r\n', b'\n')
    require(b'\r' not in result, 'Only LF/CRLF source forms are reviewed')
    return result


def apply_edits(current):
    return qi_overlay.apply_to_qi_only(current.encode()).decode('utf-8')


def inverse(candidate):
    return qi_overlay.remove_from_overlay(candidate.encode()).decode('utf-8')


def transformation(raw):
    active = normalize(raw)
    digest = sha(active)
    if digest == CURRENT_SHA:
        current_bytes = active
        supplied_candidate = None
    else:
        require(digest == CANDIDATE_SHA,
                'Family source is neither the exact QI-only baseline nor exact final candidate')
        supplied_candidate = active.decode('utf-8')
        current_bytes = inverse(supplied_candidate).encode()
        require(sha(current_bytes) == CURRENT_SHA,
                'Installed family candidate does not invert to the QI-only baseline')
    phase, pre_qi, migrated, rows = qi_migration.pair(NPC, current_bytes)
    require(phase == 'after' and migrated.encode() == current_bytes,
            'The reviewed 125-owner OnInit migration is not the current input')
    require(sha(pre_qi.encode()) == PRE_QI_SHA and len(rows) == 1,
            'Exact pre-migration family source was not reconstructed')
    candidate = apply_edits(migrated)
    require(inverse(candidate) == migrated, 'Every family proposal edit must invert exactly')
    require(sha(candidate.encode()) == CANDIDATE_SHA,
            'Reviewed full candidate changed; update requires explicit source review')
    if supplied_candidate is not None:
        require(candidate == supplied_candidate,
                'Forward rebuild does not reproduce the installed family candidate')

    current_oninit = migrated[migrated.index('\nOnInit:'):]
    candidate_oninit = candidate[candidate.index('\nOnInit:'):]
    require(current_oninit == candidate_oninit,
            'Family proposal changed the existing OnInit registration suffix')
    require(candidate.count('questinfo QTYPE_DAILYQUEST,QMARK_YELLOW,"EP21_MainComplete() && EP21_Supply_Daily != EP21_DailyKey()";') == 1,
            'Exact one-time Mandel marker registration changed')
    require(candidate.count('questinfo_refresh();') == 1,
            'Exactly one post-marker refresh is required')
    require(candidate.count('.@key = callfunc("EP21_DailyKey");') == 2,
            'Initial display check and one fresh delivery key are required')
    for pattern in (
        r'setarray \.@rep\[1\],[^;]+;', r'setarray \.@item\[1\],[^;]+;',
        r'setarray \.@quest\[1\],[^;]+;', r'setarray \.@family\$\[1\],[^;]+;',
        r'setarray \.@material\$\[1\],[^;]+;',
    ):
        require(re.findall(pattern, migrated) == re.findall(pattern, candidate),
                'Family mapping/economy array changed')
    for statement in ('delitem .@item[.@i],10;', 'getitem 1001618,10;',
                      'add_reputation_points .@rep[.@i],100;'):
        require(migrated.count(statement) == candidate.count(statement) == 1,
                'Debit/reward/reputation contract changed')
    require(migrated.count('checkweight(1001618,10)') == 1 and
            candidate.count('checkweight(1001618,10)') == 0 and
            candidate.count('callsub(L_EP21VoucherCapacity)') == 1,
            'Exactly the ID-only voucher preflight must be replaced')
    require(candidate.count('getinventorylist;') == 1 and
            candidate.count('@inventorylist_uniqueid$[.@i] != "0"') == 1 and
            candidate.count('@inventorylist_amount[.@match] <= 29990') == 1,
            'Exact plain-voucher identity and maximum-stack guard changed')
    return current_bytes, candidate.encode(), pre_qi.encode()


def source_controls(raw):
    current, candidate, pre_qi = transformation(raw)
    require(transformation(current.replace(b'\n', b'\r\n')) == (current, candidate, pre_qi),
            'LF/CRLF positive equivalence changed')
    require(transformation(candidate) == (current, candidate, pre_qi) and
            transformation(candidate.replace(b'\n', b'\r\n')) == (current, candidate, pre_qi),
            'Installed-candidate LF/CRLF positive equivalence changed')
    for data, phase in ((current, 'qi-only'), (current.replace(b'\n', b'\r\n'), 'qi-only'),
                        (candidate, 'family-transaction'),
                        (candidate.replace(b'\n', b'\r\n'), 'family-transaction')):
        canonical, found = qi_overlay.canonicalize_for_questinfo(NPC, data)
        require(canonical == current and found == phase,
                'Fail-closed QuestInfo overlay canonicalization changed')
    negatives = [
        current + b' ',
        current.replace(b'1001618,10', b'1001618,11', 1),
        current.replace(b'OnInit:', b'OnLoad:', 1),
        current.replace(b'QMARK_YELLOW', b'QMARK_NONE', 1),
        current.replace(b'EP21_SupplyFamily = 0;', b'EP21_SupplyFamily = 8;', 1),
    ]
    for changed in negatives:
        try:
            transformation(changed)
        except (AssertionError, UnicodeDecodeError):
            pass
        else:
            raise AssertionError('A non-newline source change was accepted')
    for changed in (
        candidate + b' ',
        candidate.replace(b'questinfo_refresh();', b'questinfo_refresh(1);', 1),
        candidate.replace(b'erasequest .@quest[.@choice];', b'erasequest 17782;', 1),
        candidate.replace(b'@inventorylist_amount[.@match] <= 29990', b'@inventorylist_amount[.@match] <= 29991', 1),
    ):
        try:
            reconstructed = inverse(changed.decode())
            require(reconstructed.encode() == current, 'Changed candidate inverse mismatch')
            require(sha(changed) == CANDIDATE_SHA, 'Changed candidate hash mismatch')
        except (AssertionError, UnicodeDecodeError):
            pass
        else:
            raise AssertionError('A changed family candidate was accepted')
    print('FAMILY_SOURCE_CONTROLS_OK: 4 phase/newline forms, 9 fail-closed mutations, QI overlay adapter', flush=True)


def effective(path):
    result = {}
    for row in renewal_records(ROOT, path):
        result.setdefault(row['Id'], {}).update(row)
    return result


def validate():
    raw = (ROOT / NPC).read_bytes()
    current, candidate, pre_qi = transformation(raw)
    source_controls(raw)
    require(sha((ROOT / MAIN).read_bytes()) == MAIN_SHA, 'EP21_MainComplete source changed')
    require(sha((ROOT / DAILY).read_bytes()) == DAILY_SHA, 'Reviewed fixed daily helper source changed')
    for path, digest in DATA_SHA.items():
        require(sha((ROOT / path).read_bytes()) == digest, 'Pinned family data source changed: ' + path)
    items = effective('db/item_db.yml')
    quests = effective('db/quest_db.yml')
    reputations = effective('db/reputation.yml')
    item_ids = {1001618, 1001629, 1001648, 1001639, 1001637, 1001646, 1001642, 1001645}
    quest_ids = set(range(17781, 17789)) | {18360}
    rep_ids = set(range(13, 20))
    require(item_ids <= items.keys() and quest_ids <= quests.keys() and rep_ids <= reputations.keys(),
            'Effective family identity is incomplete')
    expected_variables = ['REP_EP21', 'REP_EP21_Nerius', 'REP_EP21_Heine',
                          'REP_EP21_Lugenburg', 'REP_EP21_Walter',
                          'REP_EP21_Wigner', 'REP_EP21_Richard']
    for index, ident in enumerate(range(13, 20)):
        row = reputations[ident]
        require(row['Variable'] == expected_variables[index] and row['Minimum'] == -1000 and row['Maximum'] == 1000,
                'Family reputation identity/range changed')
    require(items[1001618].get('Weight', 0) == 0,
            'Voucher weight premise changed')
    for ident in item_ids - {1001618}:
        require(items[ident].get('Weight') == 10, 'Supply material weight premise changed')
    evidence = {
        'runtime_raw_sha256': sha(raw), 'current_lf_sha256': sha(current),
        'candidate_lf_sha256': sha(candidate), 'pre_qi_lf_sha256': sha(pre_qi),
        'runtime_phase': 'candidate' if sha(normalize(raw)) == CANDIDATE_SHA else 'current',
        'main_source_sha256': MAIN_SHA, 'daily_source_sha256': DAILY_SHA,
        'data_source_sha256': DATA_SHA,
        'item_records': [items[i] for i in sorted(item_ids)],
        'quest_records': [quests[i] for i in sorted(quest_ids)],
        'reputation_records': [reputations[i] for i in sorted(rep_ids)],
    }
    print('FAMILY_SOURCE_OK: exact current-or-candidate/OnInit migration/inverse/data identities pinned', flush=True)
    return raw, current, candidate, pre_qi, evidence


def prepare(directory, inputs):
    directory = directory.resolve()
    require(directory != ROOT and ROOT not in directory.parents,
            'Family artifacts must remain outside the repository')
    directory.mkdir(parents=True, exist_ok=True)
    raw, current, candidate, pre_qi, evidence = inputs
    (directory / 'current.txt').write_bytes(current)
    (directory / 'candidate.txt').write_bytes(candidate)
    (directory / 'pre-questinfo-migration.txt').write_bytes(pre_qi)
    diff = ''.join(difflib.unified_diff(
        current.decode().splitlines(True), candidate.decode().splitlines(True),
        fromfile=NPC + '.CURRENT', tofile=NPC + '.PROPOSED'))
    (directory / 'candidate.diff').write_text(diff, encoding='utf-8')
    (directory / 'proposal.json').write_text(json.dumps({
        'result': 'PROPOSED_SOURCE_ONLY', **{k: evidence[k] for k in (
            'runtime_raw_sha256', 'current_lf_sha256', 'candidate_lf_sha256',
            'pre_qi_lf_sha256', 'runtime_phase', 'main_source_sha256', 'daily_source_sha256',
            'data_source_sha256')},
        'edit_count': len(qi_overlay.EDITS), 'inverse': 'exact whole-file equality',
        'questinfo_migration': 'preserved exact OnInit suffix and reconstructed exact pre-migration source',
        'runtime_unchanged': raw == (ROOT / NPC).read_bytes(),
    }, indent=2) + '\n', encoding='utf-8')
    return directory


def emit_fixtures(directory, evidence):
    def emit(name, rows):
        (directory / (name + '.yml')).write_text(
            yaml.safe_dump({'Body': rows}, sort_keys=False), encoding='utf-8')
    emit('items', evidence['item_records'])
    emit('quests', evidence['quest_records'])
    emit('reputation', evidence['reputation_records'])
    (directory / 'main-source.txt').write_bytes((ROOT / MAIN).read_bytes())
    (directory / 'daily-source.txt').write_bytes((ROOT / DAILY).read_bytes())
    marker = '{ questinfo QTYPE_DAILYQUEST,QMARK_YELLOW,"EP21_MainComplete() && EP21_Supply_Daily != EP21_DailyKey()"; end; }\n'
    (directory / 'marker.txt').write_text(marker, encoding='utf-8')


def check_output(result, mode):
    result.check_returncode()
    output = re.sub(r'\x1b\[[0-9;]*m', '', result.stdout + '\n' + result.stderr)
    require(output.count('Memory manager: No memory leaks found.') == 1,
            'Explicit clean native allocator teardown required')
    require(not re.search(r'AddressSanitizer|UndefinedBehaviorSanitizer|runtime error:|(?:invalid|double) free|Memory manager:(?! No memory leaks found\.)', output, re.I),
            'Sanitizer/allocator diagnostic')
    marker = 'FAMILY_CURRENT_DEFECTS_OK' if mode == 'current' else 'FAMILY_CANDIDATE_OK'
    require(output.count(marker) == 1, 'Exact native family marker missing')
    require(not re.search(r'\[(?:Error|Warning)\]|fatal error', output, re.I),
            'Unexpected zero-exit native diagnostic')
    return output


def output_controls():
    good = 'FAMILY_CANDIDATE_OK cases=1 assertions=1 marker_checks=1 expected_current_errors=0\nMemory manager: No memory leaks found.\n'
    check_output(subprocess.CompletedProcess([], 0, good, ''), 'candidate')
    bad = [good.replace('Memory manager: No memory leaks found.', '')]
    bad += [good + token for token in ('[Error]: bad', '[Warning]: bad', 'AddressSanitizer: bad',
                                       'runtime error: bad', 'Memory manager: invalid pointer', 'double free')]
    for output in bad:
        try:
            check_output(subprocess.CompletedProcess([], 0, output, ''), 'candidate')
        except AssertionError:
            pass
        else:
            raise AssertionError('Zero-exit family diagnostic was accepted')
    print('FAMILY_OUTPUT_CONTROLS_OK: 7 fail-closed outputs', flush=True)


def verify_binding(binding, sources, executable):
    require(binding['sources'] == sources, 'Retained family source/header binding changed')
    require(binding['executable_sha256'] == sha(executable.read_bytes()),
            'Retained family executable bytes changed')
    require(binding['link_inputs_sha256'] == {
        path: sha(Path(path).read_bytes()) for path in binding['link_inputs_sha256']},
        'Retained family support/scoped link input changed')


def artifact_controls():
    executable = Path('/family-artifact-control-executable')
    support = '/family-artifact-control-support'
    data = {str(executable): b'executable', support: b'support'}
    sources = {'source': 'exact'}
    binding = {'sources': sources, 'executable_sha256': sha(data[str(executable)]),
               'link_inputs_sha256': {support: sha(data[support])}}
    with mock.patch.object(Path, 'read_bytes', lambda path: data[str(path)]):
        verify_binding(binding, sources, executable)
        for changed in ('source', 'executable', 'support'):
            altered = {'source': 'changed'} if changed == 'source' else sources
            key = str(executable) if changed == 'executable' else support
            if changed != 'source':
                data[key] += b'changed'
            try:
                verify_binding(binding, altered, executable)
            except AssertionError:
                pass
            else:
                raise AssertionError('Changed family artifact accepted: ' + changed)
            if changed != 'source':
                data[key] = data[key][:-7]
    print('FAMILY_ARTIFACT_CONTROLS_OK: 3 source/executable/support mutations', flush=True)


def native(directory, inputs, object_cache=None):
    raw, current, candidate, pre_qi, evidence = inputs
    emit_fixtures(directory, evidence)
    prefix = (ROOT / PREFIX).read_text().split('extern "C" int __wrap_main(', 1)[0]

    def replace(old, new=''):
        nonlocal prefix
        require(prefix.count(old) == 1, 'Exact inherited family boundary adapter drift')
        prefix = prefix.replace(old, new, 1)

    replace('extern "C" npc_data* npc_lookup(int32){return nullptr;}')
    replace('extern "C" void crown_log(const map_session_data*,e_log_pick_type,int32,const item*){}')
    replace('extern "C" void error(const char* f,...){++errors;va_list a;va_start(a,f);std::vfprintf(stderr,f,a);va_end(a);}')
    replace('extern "C" int64 switch_read(int64 key){return nums[key];}')
    combined = directory / 'combined_family_test.cpp'
    combined.write_text(prefix + '\n' + (ROOT / DRIVER).read_text(), encoding='utf-8')

    production = ['src/map/' + name + '.cpp' for name in
                  ('pc', 'script', 'itemdb', 'clif', 'achievement', 'quest')]
    production += ['src/common/malloc.cpp']
    dependencies = [PREFIX, DRIVER, 'tools/ci/episode21_family_supply_test.py',
                    'tools/ci/episode21_family_supply_overlay.py',
                    'tools/ci/episode20_21_questinfo_migration.py',
                    'tools/ci/biosphere_crown_transaction_test.py',
                    'tools/ci/biosphere_callback_closure_audit.py',
                    'tools/ci/biosphere_material_callback_audit.py',
                    'tools/ci/audit_enchant_upgrades.py']
    headers = sorted(path.relative_to(ROOT).as_posix()
                     for tree in ('src', '3rdparty')
                     for path in (ROOT / tree).rglob('*')
                     if path.is_file() and path.suffix in ('.h', '.hpp', '.inl', '.tcc'))
    tracked = production + dependencies + headers
    hashes = {path: sha((ROOT / path).read_bytes()) for path in tracked}
    header_sha = sha(json.dumps({path: hashes[path] for path in headers}, sort_keys=True).encode())
    sanitizers = ['-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-omit-frame-pointer']
    flags = ['g++', '-std=c++17', '-O0', '-g', '-DPACKETVER=20260219',
             '-fno-strict-aliasing'] + sanitizers
    flags += ['-I' + path for path in
              ('src', '3rdparty/libconfig', '3rdparty/rapidyaml/src',
               '3rdparty/rapidyaml/ext/c4core/src', '3rdparty/json/include', '/usr/include/mysql')]

    def compile_one(source):
        path = Path(source)
        output = directory / (path.stem + '.o')
        sidecar = output.with_suffix('.source.json')
        signature = {'source': str(source), 'sha256': sha(path.read_bytes()),
                     'flags': flags, 'headers_sha256': header_sha}
        choices = [output]
        if object_cache:
            choices.append(object_cache / output.name)
        for choice in choices:
            metadata = choice.with_suffix('.source.json')
            saved = json.loads(metadata.read_text()) if metadata.is_file() else {}
            if (choice.is_file() and
                    {key: value for key, value in saved.items() if key != 'object_sha256'} == signature and
                    saved.get('object_sha256') == sha(choice.read_bytes())):
                if choice != output:
                    shutil.copyfile(choice, output)
                    shutil.copyfile(metadata, sidecar)
                require(saved['object_sha256'] == sha(output.read_bytes()),
                        'Verified cached family object changed during copy')
                print('Reusing exact family sanitizer object ' + str(source), flush=True)
                return output
        print('Fresh compile ' + str(source), flush=True)
        subprocess.run(flags + ['-c', str(source), '-o', str(output)], cwd=ROOT, check=True)
        require(signature['sha256'] == sha(path.read_bytes()), 'Family source changed during compile')
        sidecar.write_text(json.dumps({**signature, 'object_sha256': sha(output.read_bytes())}, indent=2) + '\n')
        return output

    with ThreadPoolExecutor(max_workers=2) as pool:
        fresh = list(pool.map(compile_one, production + [str(combined)]))
    require(hashes == {path: sha((ROOT / path).read_bytes()) for path in tracked},
            'Compiled family input changed')
    excluded = {path.name for path in fresh}
    objects = sorted(path for path in (ROOT / 'src/map/obj').rglob('*.o')
                     if path.name not in excluded)
    libraries = [ROOT / path for path in
                 ('src/common/obj/common.a', '3rdparty/libconfig/obj/libconfig.a',
                  '3rdparty/rapidyaml/obj/ryml.a')]
    require(objects and all(path.is_file() for path in libraries),
            'Real built support objects/libraries are required')
    wrappers = [name for name in WRAPPERS if name not in (
        '_Z9pc_setregP16map_session_datall', '_Z10pc_readregPK16map_session_datal',
        '_Z12pc_setregstrP16map_session_datalPKc', '_Z13pc_readregstrPK16map_session_datal',
        '_Z15pc_readregistryPK16map_session_datal', '_Z11pc_readreg2PK16map_session_dataPKc',
        '_Z17pc_show_questinfoP16map_session_data')]
    wrappers += [
        '_Z9map_id2bli', '_Z20clif_reputation_typeRK16map_session_datall',
        '_Z27achievement_check_conditionP11script_codeP16map_session_data',
        '_Z21clif_quest_show_eventPK16map_session_dataPK10block_list17e_questinfo_types21e_questinfo_markcolor',
        '_Z14clif_quest_addPK16map_session_dataPK5quest',
        '_Z27clif_quest_update_objectivePK16map_session_dataPK5quest',
        '_Z17clif_quest_deletePK16map_session_datai',
        '_Z24clif_quest_update_statusPK16map_session_dataib',
        '_Z10chrif_saveP16map_session_datai',
    ]
    executable = directory / 'episode21_family_supply_test'
    link_inputs = {str(path): sha(path.read_bytes()) for path in fresh + objects + libraries}
    command = ['g++'] + sanitizers + ['-o', str(executable)]
    command += [str(path) for path in fresh + objects + libraries]
    command += ['-Wl,--wrap=' + name for name in wrappers]
    command += ['-lz', '-ldl', '-lmysqlclient', '-lzstd', '-lssl', '-lcrypto', '-lresolv', '-lm']
    subprocess.run(command, cwd=ROOT, check=True)
    require(link_inputs == {path: sha(Path(path).read_bytes()) for path in link_inputs},
            'Family link input changed during linking')
    binding = {'sources': hashes, 'executable_sha256': sha(executable.read_bytes()),
               'link_inputs_sha256': link_inputs}
    (directory / 'build.json').write_text(json.dumps(binding, indent=2) + '\n')
    verify_binding(binding, hashes, executable)

    outputs = {}
    for mode in ('candidate', 'current'):
        result = subprocess.run([str(executable), str(directory), mode], cwd=ROOT,
                                capture_output=True, text=True, timeout=180)
        for stream in ('stdout', 'stderr'):
            (directory / (mode + '.' + stream + '.txt')).write_text(getattr(result, stream))
        print(result.stdout, end='', flush=True)
        print(result.stderr, end='', file=sys.stderr, flush=True)
        outputs[mode] = check_output(result, mode)

    final = validate()
    require(final[4] == evidence, 'Family source/data evidence changed during native proof')
    require(raw == (ROOT / NPC).read_bytes(), 'Family runtime changed during proof')
    require(hashes == {path: sha((ROOT / path).read_bytes()) for path in tracked},
            'Family source/header changed during native proof')
    verify_binding(binding, hashes, executable)
    counts = {}
    for mode, output in outputs.items():
        found = re.search(r'FAMILY_(?:CANDIDATE|CURRENT_DEFECTS)_OK cases=(\d+) assertions=(\d+) marker_checks=(\d+) expected_current_errors=(\d+)', output)
        require(found is not None, 'Exact native family coverage counters required')
        counts[mode] = dict(zip(('cases', 'assertions', 'marker_checks', 'expected_current_errors'),
                                map(int, found.groups())))
    require(counts == {
        'candidate': {'cases': 82, 'assertions': 24140, 'marker_checks': 283,
                      'expected_current_errors': 0},
        'current': {'cases': 6, 'assertions': 611, 'marker_checks': 15,
                    'expected_current_errors': 2},
    }, 'Exact family candidate and current-defect case inventories required')
    receipt = {
        'result': 'PASS', 'counts': counts,
        'runtime_raw_sha256': sha(raw), 'current_lf_sha256': sha(current),
        'candidate_lf_sha256': sha(candidate), 'pre_qi_lf_sha256': sha(pre_qi),
        'source_hashes': hashes, 'build_binding_sha256': sha((directory / 'build.json').read_bytes()),
        'executable_sha256': binding['executable_sha256'],
        'link_inputs_sha256': link_inputs,
        'fixture_sha256': {path.name: sha(path.read_bytes()) for path in sorted(directory.iterdir())
                           if path.suffix == '.yml' or path.name in
                           ('current.txt', 'candidate.txt', 'pre-questinfo-migration.txt',
                            'main-source.txt', 'daily-source.txt', 'marker.txt',
                            'combined_family_test.cpp')},
        'native_output_sha256': {mode: {stream: sha((directory / (mode + '.' + stream + '.txt')).read_bytes())
                                        for stream in ('stdout', 'stderr')} for mode in outputs},
        'asan_ubsan': True, 'network': 'kernel denied',
        'clock_boundary': 'Deterministic actual-VM EP21_DailyKey replacement; production helper bytes pinned and independently covered by episode21_daily_clock_test',
        'boundary': 'Actual Mandel/select/Next/inventory/loaded character registry/quest/reputation/QuestInfo VM. World, SQL persistence/reconnect, logs, packet serialization, and clock acquisition are explicit boundaries.',
    }
    (directory / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print('FAMILY_NATIVE_RECEIPT: PASS', flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--proposal-dir', type=Path, required=True)
    parser.add_argument('--native', action='store_true')
    parser.add_argument('--object-cache', type=Path)
    args = parser.parse_args()
    output_controls()
    artifact_controls()
    inputs = validate()
    directory = prepare(args.proposal_dir, inputs)
    if args.native:
        native(directory, inputs, args.object_cache.resolve() if args.object_cache else None)


if __name__ == '__main__':
    main()
