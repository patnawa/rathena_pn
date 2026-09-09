#!/usr/bin/env python3
# ============================================================================
#  PN  /  DEVELOPMENT TOOLS
#  episode18_gudra_transaction_test.py
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/episode18_gudra_transaction_test.py
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

"""Exact Gudra-only transform/inverse and isolated native proof.

The active source must be either the reviewed original or installed candidate.
The immutable pre-install commit supplies the original only when validating the
candidate's exact inverse; no worktree content is silently rebaselined.
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

ROOT = Path(__file__).resolve().parents[2]
NPC = 'npc/re/quests/quests_18.txt'
DECLARATION = 'wolfvill,61,170,5\tscript\tFolklorist Gudra#ep18\t4_F_SHABBY,{'
ORIGINAL_RAW_SHA = '50d771b504dfa2eafefcbafee9680eebdadfc2edc08e21428051aebb2dc78b0f'
ORIGINAL_SHA = '5f0b2af75840828f4cac5377dd1c0a5fd368fa0d1d612a4fb9a0a449707d5926'
PROPOSED_SHA = '8d3af9e9418e085c84338973df576689fa267bc6c62f0cc4a9be482c7d3002fe'
BASELINE_COMMIT = '43251f97df8d5104af7f9fbb5605aad7c829dadd'


def require(ok, message):
    if not ok:
        raise AssertionError(message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def normalize(raw):
    result = raw.replace(b'\r\n', b'\n')
    require(b'\r' not in result, 'Only LF/CRLF source forms are supported')
    return result


HELPER = '''
// Exact plain grants for Gudra's two pinned Etc items. A hand-in may use the
// slot released by the actual one-Note delitem selection. No script can yield
// or mutate these resources in the reviewed wolfvill commit callback closure.
L_GudraPlain:
	.@output = getarg(0);
	.@amount = getarg(1);
	.@debit = getarg(2);
	if ((.@output != 1000405 && .@output != 1000408) || .@amount < 1 || .@amount > 30000 || (.@debit != 0 && .@debit != 1))
		return false;
	if (.@debit && .@output != 1000405)
		return false;
	// Both current definitions are zero-weight, no-GUID, ordinary stackable Etc.
	if (getiteminfo(1000405,ITEMINFO_WEIGHT) != 0 || getiteminfo(1000408,ITEMINFO_WEIGHT) != 0 || Weight > MaxWeight)
		return false;
	.@slots = getinventoryslots();
	if (.@slots < 1 || .@slots > MAX_INVENTORY)
		return false;
	getinventorylist;
	.@note = -1;
	.@fallback = -1;
	.@match = -1;
	// getinventorylist visits valid positive inventory rows in ascending index.
	for (.@i = 0; .@i < @inventorylist_count; ++.@i) {
		if (@inventorylist_idx[.@i] < .@slots)
			++.@occupied;
		if (.@debit && @inventorylist_id[.@i] == 1000408) {
			if (.@fallback < 0)
				.@fallback = .@i;
			if (.@note < 0 && @inventorylist_equip[.@i] == 0 && @inventorylist_refine[.@i] == 0 && @inventorylist_card1[.@i] == 0 && @inventorylist_card2[.@i] == 0 && @inventorylist_card3[.@i] == 0 && @inventorylist_card4[.@i] == 0)
				.@note = .@i;
		}
		if (.@match >= 0 || @inventorylist_id[.@i] != .@output)
			continue;
		if (@inventorylist_bound[.@i] != 0 || @inventorylist_expire[.@i] != 0 || @inventorylist_uniqueid$[.@i] != "0" || @inventorylist_card1[.@i] != 0 || @inventorylist_card2[.@i] != 0 || @inventorylist_card3[.@i] != 0 || @inventorylist_card4[.@i] != 0)
			continue;
		.@match = .@i;
	}
	if (.@debit) {
		if (.@note < 0)
			.@note = .@fallback;
		if (.@note < 0)
			return false;
		if (@inventorylist_idx[.@note] < .@slots && @inventorylist_amount[.@note] == 1)
			--.@occupied;
	}
	if (.@match >= 0) {
		// A failing first compatible native stack never falls back to a free slot.
		return @inventorylist_idx[.@match] < .@slots && @inventorylist_amount[.@match] + .@amount <= 30000;
	}
	return .@occupied < .@slots;
'''


def capacity(output, amount, debit, indent):
    return (f'{indent}if (!callsub(L_GudraPlain,{output},{amount},{debit})) {{\n'
            f'{indent}\tmes "Please make room for the item before continuing.";\n'
            f'{indent}\tclose;\n{indent}}}\n')


def fresh(conditions, indent):
    expression = 'strcharinfo(3) != "wolfvill" || ep18_main < 36'
    if conditions:
        expression += ' || ' + ' || '.join(conditions)
    return (f'{indent}if ({expression}) {{\n'
            f'{indent}\tmes "Your quest state has changed. Please speak to me again.";\n'
            f'{indent}\tclose;\n{indent}}}\n')


def _transform_original(raw):
    """Return genuine whole original, candidate and exact inverse edit receipt."""
    before = normalize(raw)
    require(sha(before) == ORIGINAL_SHA, 'Entire frozen original Gudra file changed')
    from biosphere_document_callback_audit import script_body
    source = before.decode('utf-8')
    original_body = script_body(source, DECLARATION)
    changed = original_body
    edits = []

    def change(old, new):
        nonlocal changed
        require(changed.count(old) == 1, 'Ambiguous Gudra-only edit: ' + old[:90])
        changed = changed.replace(old, new, 1)
        edits.append({'before': old, 'after': new})

    top = changed[changed.index('\tif (checkweight'):changed.index('\tif (ep18_main')]
    change(top, '')
    for first in (True, False):
        ids = (16551, 16552, 16553) if first else (16555, 16556, 16557)
        stage = 16554 if first else 16558
        checks = [f'isbegin_quest({stage}) != 0']
        checks += [f'isbegin_quest({i}) != 0' for i in ids]
        if not first:
            checks += ['isbegin_quest(16554) != 2', 'checkquest(16559,PLAYTIME) != -1']
        old = ''.join(f'\t\t\tsetquest {i};\n' for i in ids)
        change(old, fresh(checks, '\t\t\t') + capacity(1000408, 1, 0, '\t\t\t') + old)

    # Existing recovery grants are exactly these three blocks. Add an explicit
    # branch identifier to make every subsequent inverse substitution unique.
    recovery = '\t\tif (countitem(1000408) < 1) {\n\t\t\tgetitem 1000408,1;\t// Ep18_Recording_Note\n\t\t}\n'
    require(changed.count(recovery) == 3, 'Exactly three original Note recovery blocks')
    for label in ('first stories', 'daily stories', 'daily hand-in'):
        replacement = ('\t\t// Recover the Note without resetting ' + label + '.\n'
                       '\t\tif (countitem(1000408) < 1) {\n' +
                       fresh([], '\t\t\t') + capacity(1000408, 1, 0, '\t\t\t') +
                       '\t\t\tgetitem 1000408,1;\t// Ep18_Recording_Note\n\t\t}\n')
        # This known three-site sequence is guarded by both counts and inverse.
        require(changed.count(recovery) == 3 - len([e for e in edits if e.get('recovery')]), 'Recovery sequence changed')
        changed = changed.replace(recovery, replacement, 1)
        edits.append({'before': recovery, 'after': replacement, 'recovery': True})
    first_case = '\tcase 1:\n\t\tmes "[Folklorist Gudra]";\n\t\tmes "Dinar and Amira, did you come to tell stories to Granny Shannina?";'
    change(first_case, '\tcase 1:\n\t\t// The completed first stories also retain their lost-Note recovery.\n'
           '\t\tif (countitem(1000408) < 1) {\n' + fresh([], '\t\t\t') +
           capacity(1000408, 1, 0, '\t\t\t') +
           '\t\t\tgetitem 1000408,1;\t// Ep18_Recording_Note\n\t\t}\n' + first_case.split('\n', 1)[1])

    first_checks = ['isbegin_quest(16554) != 1', 'checkquest(16559,PLAYTIME) != -1'] + [f'isbegin_quest({i}) != 2' for i in (16551, 16552, 16553)]
    first_debit = '\t\tdelitem 1000408,1;\t// Ep18_Recording_Note\n\t\tcompletequest 16554;'
    note_check = '\t\tif (countitem(1000408) < 1) {\n\t\t\tmes "The Recording Note is missing. Please speak to me again.";\n\t\t\tclose;\n\t\t}\n'
    change(first_debit, fresh(first_checks, '\t\t') + note_check + capacity(1000405, 20, 1, '\t\t') + first_debit)
    daily_checks = ['isbegin_quest(16554) != 2', 'isbegin_quest(16558) != 1', 'checkquest(16559,PLAYTIME) != -1']
    daily_checks += [f'isbegin_quest({i}) != 2' for i in (16555, 16556, 16557)]
    daily_debit = '\t\tdelitem 1000408,1;\t// Ep18_Recording_Note\n\t\terasequest 16555;'
    change(daily_debit, fresh(daily_checks, '\t\t') + note_check +
           '\t\t// Preserve the bonus determined after the original +30 reputation.\n'
           '\t\t.@reward_amount = 3 + (get_reputation_points(REPUTATION_EP18) >= 4970);\n' +
           capacity(1000405, '.@reward_amount', 1, '\t\t') + daily_debit)
    change('\nOnInit:\n', HELPER + '\nOnInit:\n')

    after = source.replace(original_body, changed, 1).encode('utf-8')
    reverse = changed
    for edit in reversed(edits):
        require(reverse.count(edit['after']) == 1 if edit['after'] else True, 'Unique inverse candidate edit')
        if not edit['after']:
            # The removed global check occupied the start of the original body.
            require(reverse.startswith('{\n\tif (ep18_main'), 'Original opening anchor')
            reverse = reverse.replace('{\n', '{\n' + edit['before'], 1)
        else:
            reverse = reverse.replace(edit['after'], edit['before'], 1)
    require(reverse == original_body, 'Every proposed Gudra edit has an exact inverse')
    require(after.decode().replace(changed, reverse, 1).encode() == before, 'Whole-file inverse equality')
    require(source.count(original_body) == 1, 'Unique original NPC body')
    require(sha(after) == PROPOSED_SHA, 'Reviewed proposed candidate changed; explicit review required')
    return before, after, edits


def transformation(raw):
    """Accept only either exact endpoint and always return the genuine pair."""
    current = normalize(raw)
    digest = sha(current)
    if digest == ORIGINAL_SHA:
        return _transform_original(raw)
    require(digest == PROPOSED_SHA, 'Active Gudra source is neither reviewed endpoint')
    baseline = subprocess.check_output(
        ['git', 'show', f'{BASELINE_COMMIT}:{NPC}'], cwd=ROOT)
    require(sha(normalize(baseline)) == ORIGINAL_SHA,
            'Immutable pre-install Git source does not match reviewed original')
    before, after, edits = _transform_original(baseline)
    require(after == current, 'Installed candidate is not the exact forward transform')
    return before, after, edits


def source_controls(raw):
    before, after, _ = transformation(raw)
    require(transformation(before.replace(b'\n', b'\r\n'))[1] == after, 'LF/CRLF positive equivalence')
    require(transformation(after)[0] == before, 'Installed-candidate exact inverse endpoint')
    bad_sources = (before + b' ', before.replace(b'ep18_main', b'ep18_main_bad', 1),
                   before.replace(b'\n', b'\r', 1), after + b' ',
                   after.replace(b'L_GudraPlain', b'L_GudraPlain_bad', 1))
    for bad in bad_sources:
        try:
            transformation(bad)
        except AssertionError:
            pass
        else:
            raise AssertionError('Non-newline source change accepted')


def prepare(directory, root=ROOT):
    directory = directory.resolve()
    require(directory != root and root not in directory.parents, 'Artifacts must be outside repository')
    raw = (root / NPC).read_bytes()
    before, after, edits = transformation(raw)
    source_controls(raw)
    directory.mkdir(parents=True, exist_ok=True)
    (directory / 'before.txt').write_bytes(before)
    (directory / 'after.txt').write_bytes(after)
    diff = ''.join(difflib.unified_diff(before.decode().splitlines(True), after.decode().splitlines(True), fromfile=NPC, tofile=NPC + '.PROPOSED'))
    (directory / 'candidate.diff').write_text(diff, encoding='utf-8')
    receipt = {'result': 'EXACT_SOURCE_PAIR', 'runtime_raw_sha256': sha(raw),
               'original_normalized_sha256': sha(before), 'candidate_normalized_sha256': sha(after),
               'active_phase': 'candidate' if sha(normalize(raw)) == PROPOSED_SHA else 'original',
               'baseline_commit': BASELINE_COMMIT,
               'edit_count': len(edits), 'edits': edits, 'inverse': 'exact whole-file equality',
               'runtime_unchanged': raw == (root / NPC).read_bytes()}
    (directory / 'proposal.json').write_text(json.dumps(receipt, indent=2) + '\n', encoding='utf-8')
    print('GUDRA_PROPOSAL_OK: ' + json.dumps({k: v for k, v in receipt.items() if k != 'edits'}), flush=True)
    return raw, before, after


def prepare_callbacks(directory, manifest):
    from audit_enchant_upgrades import renewal_records
    def effective(path):
        result = {}
        for row in renewal_records(ROOT, path):
            result.setdefault(row['Id'], {}).update(row)
        return result
    def emit(name, rows):
        (directory / (name + '.yml')).write_text(yaml.safe_dump({'Body': rows}, sort_keys=False), encoding='utf-8')
    groups = {}
    probes = []
    for row in manifest['questinfo']['registrations']:
        groups.setdefault(row['npc'], []).append(row)
        expression = row['condition'].split('||', 1)[0]
        qs, variables, items = {}, {}, {}
        for ident, value in re.findall(r'isbegin_quest\((\d+)\)\s*==\s*(-?\d+)', expression):
            if int(value):
                qs[int(ident)] = {'Id': int(ident), 'State': int(value), 'CompleteCounts': False}
        for ident, mode, value in re.findall(r'checkquest\((\d+),(HUNTING|PLAYTIME)\)\s*==\s*(-?\d+)', expression):
            if int(value) != -1:
                require(mode == 'HUNTING' and int(value) == 2, 'Explicit current condition true-probe grammar')
                qs[int(ident)] = {'Id': int(ident), 'State': 1, 'CompleteCounts': True}
        for name, value in re.findall(r'\b(BaseLevel|ep18_main|ep19_main)\s*(?:==|>=)\s*(\d+)', expression):
            variables[name] = int(value)
        for ident, operator, value in re.findall(r'countitem\((\d+)\)\s*(>=|>)\s*(\d+)', expression):
            items[int(ident)] = int(value) + (operator == '>')
        probes.append({'Condition': row['condition'], 'Quests': list(qs.values()),
                       'Variables': [{'Name': n, 'Value': v} for n, v in variables.items()],
                       'Items': [{'Id': i, 'Amount': v} for i, v in items.items()]})
    require(len(groups) == 35 and len(probes) == 72, 'Exact wolfvill native fixture coverage')
    emit('questinfo', [{'Owner': name, 'Script': '{\n' + '\n'.join(r['statement'] for r in rows) + '\nend;\n}',
                        'Conditions': [r['condition'] for r in rows]} for name, rows in groups.items()])
    emit('qi-probes', probes)
    quests = manifest['quests']['referenced_ordered_records']
    emit('quests', quests)
    mob_names = {t['Mob'] for q in quests for key in ('Targets', 'Drops') for t in q.get(key, []) if 'Mob' in t}
    mobs = {r['AegisName']: r for r in effective('db/mob_db.yml').values() if r.get('AegisName') in mob_names}
    require(set(mobs) == mob_names, 'All real quest monster identities resolved')
    emit('quest-mob-identities', [{'Id': mobs[n]['Id'], 'AegisName': n} for n in sorted(mobs)])
    items = effective('db/item_db.yml')
    by_name = {r['AegisName']: r['Id'] for r in items.values() if 'AegisName' in r}
    ids = {1000405, 1000408, 7110, 517, 999, 1000406}
    ids |= {by_name[t['Item']] for q in quests for t in q.get('Drops', []) if 'Item' in t}
    emit('items', [items[i] for i in sorted(ids)])
    emit('reputation', manifest['reputation']['ordered_records'])
    emit('achievements', manifest['achievements']['get_item_records'])


def check_output(result, mode):
    result.check_returncode()
    text = re.sub(r'\x1b\[[0-9;]*m', '', result.stdout + '\n' + result.stderr)
    require(text.count('Memory manager: No memory leaks found.') == 1, 'Explicit clean native allocator teardown')
    require(not re.search(r'AddressSanitizer|UndefinedBehaviorSanitizer|runtime error:|(?:invalid|double) free|Memory manager:(?! No memory leaks found\.)', text, re.I), 'Sanitizer/allocator diagnostic')
    marker = 'GUDRA_ORIGINAL_OK' if mode == 'original' else 'GUDRA_NATIVE_OK'
    require(text.count(marker) == 1, 'Exact native completion marker')
    residual = text
    if mode == 'original':
        for token, count in (
            ('[Error]: buildin_getitem: Failed to add the item to player.', 3),
            ("[Warning]: Script command 'getitem' returned failure.", 3),
            ('[Error]: buildin_delitem: failed to delete 1 items (AID=99000001 item_id=1000408).', 1),
            ("[Warning]: Script command 'delitem' returned failure.", 1)):
            require(residual.count(token) == count, 'Exact original diagnostic count: ' + token)
            residual = residual.replace(token, '')
    require(not re.search(r'\[(?:Error|Warning)\]|fatal error', residual, re.I), 'Unexpected zero-exit diagnostic')
    return text


def output_controls():
    good = 'GUDRA_NATIVE_OK cases=1 assertions=1 nested_checks=1 achievement_checks=7\nMemory manager: No memory leaks found.\n'
    check_output(subprocess.CompletedProcess([], 0, good, ''), 'candidate')
    samples = [good.replace('Memory manager: No memory leaks found.', '')]
    samples += [good + s for s in ('[Error]: unexpected', '[Warning]: unexpected', 'AddressSanitizer: bad',
                                  'runtime error: bad', 'Memory manager: invalid pointer', 'double free')]
    for text in samples:
        try:
            check_output(subprocess.CompletedProcess([], 0, text, ''), 'candidate')
        except AssertionError:
            pass
        else:
            raise AssertionError('Zero-exit native diagnostic accepted')
    print('GUDRA_OUTPUT_CONTROLS_OK: 7 fail-closed controls', flush=True)


def verify_binding(binding, sources, executable):
    require(binding['sources'] == sources, 'Retained native source/header binding changed')
    require(binding['executable_sha256'] == sha(executable.read_bytes()), 'Retained executable bytes changed')
    require(binding['link_inputs_sha256'] == {p: sha(Path(p).read_bytes()) for p in binding['link_inputs_sha256']}, 'Retained support/scoped link input bytes changed')


def artifact_controls():
    exe = Path('/gudra-artifact-control-executable')
    support = '/gudra-artifact-control-support'
    data = {str(exe): b'executable', support: b'support'}
    sources = {'source': 'exact'}
    binding = {'sources': sources, 'executable_sha256': sha(data[str(exe)]),
               'link_inputs_sha256': {support: sha(data[support])}}
    with mock.patch.object(Path, 'read_bytes', lambda p: data[str(p)]):
        verify_binding(binding, sources, exe)
        for changed in ('source', 'executable', 'support'):
            altered = {'source': 'changed'} if changed == 'source' else sources
            key = str(exe) if changed == 'executable' else support
            if changed != 'source':
                data[key] += b'changed'
            try:
                verify_binding(binding, altered, exe)
            except AssertionError:
                pass
            else:
                raise AssertionError('Changed artifact accepted: ' + changed)
            if changed != 'source':
                data[key] = data[key][:-7]
    print('GUDRA_ARTIFACT_CONTROLS_OK: 3 in-memory rejection controls', flush=True)


def native(directory, inputs, *, prepare_only=False, run_unvalidated=False, object_cache=None):
    from episode18_gudra_callback_audit import collect_evidence, validate as gate
    from biosphere_crown_transaction_test import WRAPPERS
    raw, before, after = inputs
    manifest = collect_evidence(ROOT) if prepare_only or run_unvalidated else gate(ROOT)
    prepare_callbacks(directory, manifest)
    prefix_path = 'tools/ci/biosphere_crown_transaction_test.cpp'
    driver_path = 'tools/ci/episode18_gudra_transaction_test.cpp'
    prefix = (ROOT / prefix_path).read_text().split('extern "C" int __wrap_main(', 1)[0]
    def replace(old, new):
        nonlocal prefix
        require(prefix.count(old) == 1, 'Exact inherited boundary adapter drift')
        prefix = prefix.replace(old, new, 1)
    replace('extern "C" npc_data* npc_lookup(int32){return nullptr;}', '')
    replace('extern "C" void crown_log(const map_session_data*,e_log_pick_type,int32,const item*){}', '')
    replace('extern "C" void error(const char* f,...){++errors;va_list a;va_start(a,f);std::vfprintf(stderr,f,a);va_end(a);}',
            'extern "C" void error(const char* f,...){++errors;std::fputs("[Error]: ",stderr);va_list a;va_start(a,f);std::vfprintf(stderr,f,a);va_end(a);}')
    combined = directory / 'combined_gudra_test.cpp'
    combined.write_text(prefix + '\n' + (ROOT / driver_path).read_text(), encoding='utf-8')
    production = ['src/map/' + p + '.cpp' for p in ('pc', 'script', 'itemdb', 'clif', 'achievement', 'quest')] + ['src/common/malloc.cpp']
    dependencies = [prefix_path, driver_path, 'tools/ci/episode18_gudra_transaction_test.py',
                    'tools/ci/episode18_gudra_callback_audit.py', 'tools/ci/biosphere_document_callback_audit.py',
                    'tools/ci/biosphere_crown_transaction_test.py', 'tools/ci/biosphere_material_callback_audit.py',
                    'tools/ci/biosphere_callback_closure_audit.py', 'tools/ci/audit_enchant_upgrades.py']
    headers = sorted(p.relative_to(ROOT).as_posix() for tree in ('src', '3rdparty') for p in (ROOT / tree).rglob('*') if p.is_file() and p.suffix in ('.h', '.hpp', '.inl', '.tcc'))
    hashes = {p: sha((ROOT / p).read_bytes()) for p in production + dependencies + headers}
    header_sha = sha(json.dumps({p: hashes[p] for p in headers}, sort_keys=True).encode())
    san = ['-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-omit-frame-pointer']
    flags = ['g++', '-std=c++17', '-O0', '-g', '-DPACKETVER=20260219', '-fno-strict-aliasing'] + san
    flags += ['-I' + p for p in ('src', '3rdparty/libconfig', '3rdparty/rapidyaml/src', '3rdparty/rapidyaml/ext/c4core/src', '3rdparty/json/include', '/usr/include/mysql')]
    def compile_one(source):
        path = Path(source)
        out = directory / (path.stem + '.o')
        signature = {'source': str(source), 'sha256': sha(path.read_bytes()), 'flags': flags, 'headers_sha256': header_sha}
        sidecar = out.with_suffix('.source.json')
        candidates = [out]
        if object_cache:
            candidates.append(object_cache / out.name)
        for candidate in candidates:
            meta = candidate.with_suffix('.source.json')
            saved = json.loads(meta.read_text()) if meta.is_file() else {}
            if candidate.is_file() and {k: v for k, v in saved.items() if k != 'object_sha256'} == signature and saved.get('object_sha256') == sha(candidate.read_bytes()):
                if candidate != out:
                    shutil.copyfile(candidate, out)
                    shutil.copyfile(meta, sidecar)
                require(sha(out.read_bytes()) == saved['object_sha256'], 'Cached sanitizer object changed during verified copy')
                print('Reusing exact source/header/flags/object-bound sanitizer object ' + str(source), flush=True)
                return out
        print('Fresh compile ' + str(source), flush=True)
        subprocess.run(flags + ['-c', str(source), '-o', str(out)], cwd=ROOT, check=True)
        require(signature['sha256'] == sha(path.read_bytes()), 'Source changed during compile')
        sidecar.write_text(json.dumps({**signature, 'object_sha256': sha(out.read_bytes())}, indent=2) + '\n')
        return out
    with ThreadPoolExecutor(max_workers=2) as pool:
        fresh = list(pool.map(compile_one, production + [str(combined)]))
    require(hashes == {p: sha((ROOT / p).read_bytes()) for p in hashes}, 'Compiled input changed')
    excluded = {p.name for p in fresh}
    objects = sorted(p for p in (ROOT / 'src/map/obj').rglob('*.o') if p.name not in excluded)
    libs = [ROOT / p for p in ('src/common/obj/common.a', '3rdparty/libconfig/obj/libconfig.a', '3rdparty/rapidyaml/obj/ryml.a')]
    require(objects and all(p.is_file() for p in libs), 'Real built support objects/libraries required')
    wrappers = [w for w in WRAPPERS if w not in (
        '_Z17pc_show_questinfoP16map_session_data', '_Z15pc_readregistryPK16map_session_datal',
        '_Z11pc_readreg2PK16map_session_dataPKc', '_Z28achievement_update_objectiveP16map_session_data19e_achievement_grouphz')]
    wrappers += ['_Z9map_id2bli', '_Z10run_scriptP11script_codeiii', '_Z20clif_reputation_typeRK16map_session_datall',
                 '_Z27achievement_check_conditionP11script_codeP16map_session_data',
                 '_Z21clif_quest_show_eventPK16map_session_dataPK10block_list17e_questinfo_types21e_questinfo_markcolor',
                 '_Z10pc_gainexpP16map_session_dataP10block_listmmh', '_Z14clif_quest_addPK16map_session_dataPK5quest',
                 '_Z27clif_quest_update_objectivePK16map_session_dataPK5quest', '_Z17clif_quest_deletePK16map_session_datai',
                 '_Z24clif_quest_update_statusPK16map_session_dataib', '_Z10chrif_saveP16map_session_datai',
                 '_Z19clif_disp_overhead_PK10block_listPKc11send_target']
    exe = directory / 'episode18_gudra_transaction_test'
    link_inputs = {str(p): sha(p.read_bytes()) for p in fresh + objects + libs}
    subprocess.run(['g++'] + san + ['-o', str(exe)] + [str(p) for p in fresh + objects + libs] + ['-Wl,--wrap=' + w for w in wrappers] +
                   ['-lz', '-ldl', '-lmysqlclient', '-lzstd', '-lssl', '-lcrypto', '-lresolv', '-lm'], cwd=ROOT, check=True)
    require(link_inputs == {p: sha(Path(p).read_bytes()) for p in link_inputs}, 'Link input changed during linking')
    binding = {'sources': hashes, 'executable_sha256': sha(exe.read_bytes()), 'link_inputs_sha256': link_inputs}
    (directory / 'build.json').write_text(json.dumps(binding, indent=2) + '\n')
    verify_binding(binding, hashes, exe)
    if prepare_only:
        print('GUDRA_BUILD_ONLY: no accepted gate or native execution claimed', flush=True)
        return
    outputs = {}
    for mode in ('candidate', 'original'):
        result = subprocess.run([str(exe), str(directory), mode], cwd=ROOT, capture_output=True, text=True, timeout=180)
        for stream in ('stdout', 'stderr'):
            (directory / (mode + '.' + stream + '.txt')).write_text(getattr(result, stream))
        print(result.stdout, end='', flush=True)
        print(result.stderr, end='', file=sys.stderr, flush=True)
        outputs[mode] = check_output(result, mode)
    final = collect_evidence(ROOT) if run_unvalidated else gate(ROOT)
    require(final == manifest, 'Scoped callback evidence changed during native proof')
    require(raw == (ROOT / NPC).read_bytes(), 'Runtime changed during proof')
    require(hashes == {p: sha((ROOT / p).read_bytes()) for p in hashes}, 'Source/header changed during proof')
    verify_binding(binding, hashes, exe)
    counts = {}
    for mode, text in outputs.items():
        match = re.search(r'GUDRA_(?:NATIVE|ORIGINAL)_OK cases=(\d+) assertions=(\d+) nested_checks=(\d+) achievement_checks=(\d+)', text)
        require(match is not None, 'Exact native coverage counters required')
        counts[mode] = dict(zip(('cases', 'assertions', 'nested_checks', 'achievement_checks'), map(int, match.groups())))
    require(counts['candidate']['cases'] == 416 and counts['original']['cases'] == 7, 'Exact complete candidate/original fixture case inventory')
    receipt = {'result': 'UNVALIDATED_NATIVE_TEST' if run_unvalidated else 'PASS',
               'callback_manifest_sha256': sha(json.dumps(manifest, sort_keys=True).encode()),
               'runtime_raw_sha256': sha(raw), 'original_normalized_sha256': sha(before), 'candidate_normalized_sha256': sha(after),
               'source_hashes': hashes, 'build_binding_sha256': sha((directory / 'build.json').read_bytes()),
               'executable_sha256': binding['executable_sha256'], 'link_inputs_sha256': link_inputs,
               'fixture_sha256': {p.name: sha(p.read_bytes()) for p in sorted(directory.iterdir()) if p.suffix == '.yml' or p.name in ('before.txt', 'after.txt', 'combined_gudra_test.cpp')},
               'native_outputs': {m: {s: sha((directory / (m + '.' + s + '.txt')).read_bytes()) for s in ('stdout', 'stderr')} for m in outputs},
               'counts': counts,
               'asan_ubsan': True, 'network': 'kernel denied',
               'boundary': 'Actual NPC/select/Next/inventory/loaded registry/quest mutation/35-owner72-QI/7item conditions; pc_gainexp records exact unchanged arguments only. World/packets/transport/transient @ registers explicitly doubled; no full level-up or ordinary party-pickup execution claim.'}
    (directory / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print('GUDRA_NATIVE_RECEIPT: ' + receipt['result'], flush=True)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--proposal-dir', type=Path, required=True)
    parser.add_argument('--native', action='store_true')
    parser.add_argument('--prepare-only', action='store_true')
    parser.add_argument('--run-unvalidated', action='store_true', help='Development execution only; never accepted gate/PASS receipt')
    parser.add_argument('--object-cache', type=Path)
    args = parser.parse_args()
    require(not (args.prepare_only and args.run_unvalidated), 'Preparation-only and unvalidated execution are distinct modes')
    output_controls()
    artifact_controls()
    directory = args.proposal_dir.resolve()
    inputs = prepare(directory)
    if args.native:
        native(directory, inputs, prepare_only=args.prepare_only, run_unvalidated=args.run_unvalidated,
               object_cache=args.object_cache.resolve() if args.object_cache else None)
