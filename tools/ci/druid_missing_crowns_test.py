#!/usr/bin/env python3
"""New-only crown/Sky partner definition checks and optional real native VM proof."""
import argparse
from concurrent.futures import ThreadPoolExecutor
from copy import deepcopy
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import yaml

ROOT = Path(__file__).resolve().parents[2]
PACKAGE = ROOT / 'client-patch/druid_missing_crowns'
sys.path.insert(0, str(PACKAGE))
from facts import ITEMS, COMBOS, EXTERNAL_COMBOS, UNRESOLVED
from build import outputs, records, skills, script, require, describe_effects
from audit_enchant_upgrades import renewal_records
from lua51_literal_table import literal_tables

JOB_CONSTANTS = {'Assassin': 'EAJ_SHADOW_CROSS', 'Wizard': 'EAJ_ARCH_MAGE', 'Alchemist': 'EAJ_BIOLO',
    'BardDancer': 'EAJ_TROUBADOURTROUVERE', 'Alitea': 'EAJ_ALITEA', 'Knight': 'EAJ_DRAGON_KNIGHT',
    'Sage': 'EAJ_ELEMENTAL_MASTER', 'KagerouOboro': 'EAJ_SHINKIROSHIRANUI',
    'Rebellion': 'EAJ_NIGHT_WATCH', 'SoulLinker': 'EAJ_SOUL_ASCETIC'}


def digest(path): return hashlib.sha256(path.read_bytes()).hexdigest()


OWNED = {'db/import/druid_missing_crowns.yml', 'db/import/sky_crown_partner_weapons.yml',
         'db/import/druid_missing_crown_combos.yml'}


def import_records(relative, suppressed=False):
    """Observe all import edges, fail even if our file is hidden in wrong mode."""
    visits, rows, stack = [], [], set()
    def walk(relative, enabled=True):
        path = ROOT / relative
        require(path.is_file(), 'Missing import: ' + relative)
        require(relative not in stack, 'Import cycle: ' + relative)
        stack.add(relative)
        data = yaml.load(path.read_text(), Loader=getattr(yaml, 'CSafeLoader', yaml.SafeLoader)) or {}
        if enabled and not (suppressed and relative in OWNED): rows.extend(deepcopy(data.get('Body') or []))
        for entry in data.get('Footer', {}).get('Imports', []):
            child = entry['Path']; active = enabled and entry.get('Mode', 'Renewal') == 'Renewal'
            if child in OWNED:
                visits.append((child, active))
                require(active, 'Own overlay imported in non-Renewal mode: ' + child)
            walk(child, active)
        stack.remove(relative)
    walk(relative)
    return rows, visits


def merge_items(rows):
    result = {}
    def merge(target, row):
        for key, value in row.items():
            if isinstance(value, dict) and isinstance(target.get(key), dict): merge(target[key], value)
            else: target[key] = deepcopy(value)
    for row in rows: merge(result.setdefault(row['Id'], {}), row)
    return result


def merge_combos(rows, items):
    names = {row['AegisName']: id for id, row in items.items()}
    result = {}
    for row in rows:
        groups = row.get('Combos', [])
        # Match native whole-row rejection for any missing identity.
        if any(name not in names for group in groups for name in group['Combo']): continue
        for group in groups:
            key = tuple(sorted(names[name] for name in group['Combo']))
            if row.get('Clear'): result.pop(key, None)
            elif 'Script' in row: result[key] = row['Script']
    return result


def preserve_prior(require_import=False):
    baseline_items, item_visits = import_records('db/item_db.yml', suppressed=True)
    baseline_combos, combo_visits = import_records('db/item_combos.yml', suppressed=True)
    visits = item_visits + combo_visits
    require(len(visits) == len({p for p, _ in visits}), 'Duplicate own overlay import')
    require(not visits or {p for p, _ in visits} == OWNED, 'Partial own overlay imports')
    require(not require_import or len(visits) == 3, 'All three Renewal imports are required')
    old_items = merge_items(baseline_items); old_combos = merge_combos(baseline_combos, old_items)
    additions = records(); item_ids = {r['Id'] for r in additions}
    require(not set(old_items) & item_ids, 'Own IDs existed before scoped overlays')
    names = {r['AegisName']: r['Id'] for r in additions}
    own_keys = {tuple(sorted(names[n] for n in c['names'])) for c in COMBOS}
    raw_old_keys = {tuple(sorted(g['Combo'])) for row in baseline_combos for g in row.get('Combos', [])}
    require(all(tuple(sorted(c['names'])) not in raw_old_keys for c in COMBOS),
            'New set membership overlaps a pre-existing row, even if formerly unresolved')
    candidate_items = merge_items(baseline_items + additions)
    new_combo_rows = yaml.safe_load((ROOT / 'db/import/druid_missing_crown_combos.yml').read_text())['Body']
    candidate_combos = merge_combos(baseline_combos + new_combo_rows, candidate_items)
    require(set(candidate_items) - set(old_items) == item_ids and all(candidate_items[i] == r for i, r in old_items.items()),
            'A previous effective item changed in candidate merge')
    require(all(candidate_combos.get(k) == v for k, v in old_combos.items()), 'A previous effective combo changed')
    # Parent may already have wired the coordinated weapon agent's four sets.
    # Only these reviewed unresolved crosssets may newly resolve as a side effect.
    known = {r['AegisName']: id for id, r in candidate_items.items()}
    external_keys = {tuple(sorted(known[n] for n in c['names'])) for c in EXTERNAL_COMBOS if all(n in known for n in c['names'])}
    require(own_keys <= set(candidate_combos) - set(old_combos) <= own_keys | external_keys,
            'Unexpected new effective set membership')
    if visits:
        active_items = merge_items(import_records('db/item_db.yml')[0])
        active_combos = merge_combos(import_records('db/item_combos.yml')[0], active_items)
        require(active_items == candidate_items and active_combos == candidate_combos,
                'Actual import order differs semantically from additive candidate')
    print(f'PRIOR_DATA_DEEP_PRESERVED items={len(old_items)} combos={len(old_combos)} own_sets=11 imports={len(visits)}')


def validate(require_import=False):
    for path, expected in outputs().items():
        require((ROOT / path).read_text() == expected, 'Generated data/description/provenance drift: ' + path)
    ids = {f['item']['Id'] for f in ITEMS}
    require(len(ids) == 23 and ids.isdisjoint(UNRESOLVED), 'Unreviewed identities')
    # The original active compiled lookup already knows every supported identity.
    names_path = ROOT.parent / 'audit-item-aliases-20260906/data/data/luafiles514/lua files/itemdbnametbl.lub'
    require(digest(names_path) == '2f4f35e157d25548f236dbe7785a3138f4c7ab23367becf9e1f95b958cc3c496', 'Original alias source drift')
    tables = literal_tables(names_path.read_bytes())
    mapping = next(t for t in tables.values() if isinstance(t, dict) and t.get('Sky_Rune_Crown_SHC') == 401171)
    for f in ITEMS:
        row = f['item']; require(mapping.get(row['AegisName']) == row['Id'], 'Original name/ID mismatch')
        require(row['Classes'] == {'Fourth': True} and len(row['Jobs']) == 1, 'New-only trait filter drift')
    # Before import they must be absent; after parent integration they must be
    # byte-for-byte semantic matches, never a partial field overlay of old data.
    current = {}
    occurrences = {id: 0 for id in ids}
    for row in renewal_records(ROOT, 'db/item_db.yml'):
        if row['Id'] in ids:
            occurrences[row['Id']] += 1
            current[row['Id']] = row
    expected = {r['Id']: r for r in records()}
    require(all(n <= 1 for n in occurrences.values()), 'Existing item or duplicate import overlaps new IDs')
    for id, row in current.items(): require(row == expected[id], 'Imported record differs from reviewed complete definition')
    require(not current or len(current) == 23, 'Partial active crown/Sky import requires explicit review')
    skill_db = skills()
    for fact in ITEMS:
        describe_effects(fact['effects'], skill_db)
    for c in COMBOS + EXTERNAL_COMBOS:
        describe_effects(c['effects'], skill_db)
    # Fail-closed renderer negatives: unsupported op/skill/condition/threshold,
    # and player-race exclusions may not silently disappear from descriptions.
    bad = deepcopy(ITEMS[0]['effects']); bad[0]['op'] = 'bUnknown'
    probes = [lambda: describe_effects(bad, skill_db),
              lambda: describe_effects([dict(ITEMS[0]['effects'][3], selector='UNKNOWN_SKILL')], skill_db)]
    for probe in probes:
        try: probe()
        except AssertionError: pass
        else: raise AssertionError('Renderer accepted unreviewed effect')
    preserve_prior(require_import)
    print(f'MISSING_CROWNS_STATIC_OK items=23 combos=11 imported={len(current)} unresolved=1')


def check_native_output(result):
    result.check_returncode()
    output = re.sub(r'\x1b\[[0-9;]*m', '', result.stdout + '\n' + result.stderr)
    require(output.count('NATIVE_MISSING_CROWNS_OK items=23 combos=11 executions=89733') == 1,
            'Native completion marker missing or duplicated')
    require(output.count('Memory manager: No memory leaks found.') == 1, 'Missing clean allocator teardown')
    require(not re.search(r'\[(?:error|warning)\]|AddressSanitizer|UndefinedBehaviorSanitizer|runtime error:|'
                          r'(?:invalid|double) free|Memory manager:(?! No memory leaks found\.)', output, re.I),
            'Native warning/error/allocator/sanitizer diagnostics despite exit status')


def native(build):
    build = build.resolve()
    require(build != ROOT and ROOT not in build.parents, 'Artifacts must stay outside repository')
    build.mkdir(parents=True, exist_ok=True)
    item_map = {f['item']['AegisName']: f['item']['Id'] for f in ITEMS}
    fixture = {'Items': [{'Id': f['item']['Id'], 'Index': 0 if f['item']['Type'] == 'Armor' else 1,
                         'Jobs': [JOB_CONSTANTS[j] for j in f['item']['Jobs']], 'Effects': f['effects']} for f in ITEMS],
               'Combos': [{'Head': item_map[c['names'][0]], 'Weapon': item_map[c['names'][1]],
                           'Effects': c['effects']} for c in COMBOS]}
    class NoAliases(yaml.SafeDumper):
        def ignore_aliases(self, data): return True
    (build / 'facts.yml').write_text(yaml.dump(fixture, Dumper=NoAliases))
    all_effects = [e for f in ITEMS + COMBOS for e in f['effects']]
    wanted = {'AG_ASTRAL_STRIKE_ATK', 'TR_ROSEBLOSSOM_ATK', 'AT_QUILL_SPEAR_S'}
    for e in all_effects:
        if e['op'] in ('bSkillAtk', 'bSkillCooldown'): wanted.add(e['selector'])
        if e['op'] == 'bAutoSpellOnSkill': wanted.update(e['selector'].split('|')[:2])
        if 'learned' in e['conditions']: wanted.add(e['conditions']['learned'])
    source_skills = skills()
    skill_fixture = [{k: v for k, v in source_skills[n].items() if k in ('Id', 'Name', 'Description', 'MaxLevel', 'Cooldown')}
                     for n in sorted(wanted)]
    (build / 'skills.yml').write_text(yaml.safe_dump(skill_fixture))
    prefix_path = ROOT / 'client-patch/enchant_target_metadata/native_bonus_vm_test.cpp'
    prefix = prefix_path.read_text().split('extern "C" int __wrap_main(', 1)[0]
    driver_path = ROOT / 'tools/ci/druid_missing_crowns_test.cpp'
    driver = build / 'combined_native_test.cpp'
    driver.write_text(prefix + '\n' + driver_path.read_text())
    production = ['src/map/pc.cpp', 'src/map/skill.cpp', 'src/map/script.cpp',
                  'src/map/clif.cpp', 'src/map/itemdb.cpp', 'src/common/malloc.cpp']
    tracked = production + [str(driver_path.relative_to(ROOT)), str(prefix_path.relative_to(ROOT)),
        'db/import/druid_missing_crowns.yml', 'db/import/sky_crown_partner_weapons.yml',
        'db/import/druid_missing_crown_combos.yml', 'client-patch/druid_missing_crowns/facts.py']
    hashes = {p: digest(ROOT / p) for p in tracked}
    excluded = {Path(p).stem + '.o' for p in production}
    objects = sorted(p for p in (ROOT / 'src/map/obj').rglob('*.o') if p.name not in excluded)
    libraries = [ROOT / p for p in ('src/common/obj/common.a', '3rdparty/libconfig/obj/libconfig.a', '3rdparty/rapidyaml/obj/ryml.a')]
    require(objects and all(p.is_file() for p in libraries), 'Local native support objects missing')
    sanitizer = ['-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-omit-frame-pointer']
    flags = ['g++', '-std=c++17', '-O0', '-g', '-DPACKETVER=20260219', '-fno-strict-aliasing'] + sanitizer
    flags += ['-I' + p for p in ('src', '3rdparty/libconfig', '3rdparty/rapidyaml/src',
             '3rdparty/rapidyaml/ext/c4core/src', '3rdparty/json/include', '/usr/include/mysql')]
    def compile_one(source):
        path = Path(source); output = build / (path.stem + '.o')
        print('Fresh compile ' + str(source), flush=True)
        subprocess.run(flags + ['-c', str(source), '-o', str(output)], cwd=ROOT, check=True)
        return output
    fresh = [compile_one(str(driver))]
    with ThreadPoolExecutor(max_workers=2) as pool:
        fresh += list(pool.map(compile_one, production))
    require(hashes == {p: digest(ROOT / p) for p in tracked}, 'Source changed during native compilation')
    executable = build / 'druid_missing_crowns_test'
    wrappers = ('main', '_Z9map_id2sdi', '_Z9map_id2ndi', '_Z11mapreg_initv', '_Z12mapreg_finalv',
                '_Z17npc_event_dequeueP16map_session_datab', '_Z9ShowErrorPKcz')
    command = ['g++'] + sanitizer + ['-o', str(executable)] + [str(p) for p in fresh + objects + libraries]
    command += ['-Wl,--wrap=' + s for s in wrappers]
    command += ['-lz', '-ldl', '-lmysqlclient', '-lzstd', '-lssl', '-lcrypto', '-lresolv', '-lm']
    subprocess.run(command, cwd=ROOT, check=True)
    result = subprocess.run([str(executable), str(build)], cwd=ROOT, capture_output=True, text=True, timeout=120)
    print(result.stdout, end=''); print(result.stderr, end='', file=sys.stderr)
    check_native_output(result)
    receipt = {'result': 'PASS', 'items': 23, 'combos': 11, 'executions': 89733,
               'fresh_source_hashes': hashes, 'asan_ubsan': True, 'network': 'kernel denied',
               'unrelated_link_objects': 'existing local support objects, not a full fresh server build'}
    (build / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(json.dumps(receipt, indent=2))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--native-build-dir', type=Path)
    parser.add_argument('--require-import', action='store_true', help='Fail unless all three reviewed overlays are active once in Renewal')
    args = parser.parse_args()
    validate(args.require_import)
    if args.native_build_dir: native(args.native_build_dir)
