#!/usr/bin/env python3
# ============================================================================
#  PN  /  CLIENT TOOLING
#  run_native_bonus_vm_test.py
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/client-patch/enchant_target_metadata/run_native_bonus_vm_test.py
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

"""Fresh real-VM crown bonus proof; isolated generated artifacts only, no server startup."""
import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import yaml

ROOT = Path(__file__).resolve().parents[2]
PACKAGE = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT / 'tools/ci'))
from effects import effective, provenance, require


def validate_output(completed):
    """Do not let allocator warnings with zero exit masquerade as a pass."""
    completed.check_returncode()
    output = re.sub(r'\x1b\[[0-9;]*m', '', completed.stdout + '\n' + completed.stderr)
    require(output.count('NATIVE_CROWN_BONUS_VM_OK executions=44205') == 1,
            'Actual VM did not reach its unique completion marker')
    require(output.count('Memory manager: No memory leaks found.') == 1,
            'Native allocator did not explicitly report clean teardown')
    require(not re.search(r'\[(?:error|warning)\]|AddressSanitizer|UndefinedBehaviorSanitizer|'
                          r'runtime error:|(?:invalid|double) free|'
                          r'Memory manager:(?! No memory leaks found\.)', output, re.I),
            'Native allocator/error/sanitizer diagnostics found despite completion marker')


def run(build):
    manifest = json.loads((PACKAGE / 'manifest.json').read_text())
    ids = {row['id'] for row in manifest['items']}
    items, combos, skills = effective(ROOT, ids)
    require(provenance(ids, items, combos, skills) == manifest['effect_provenance'], 'Effective source drift')
    after = combos[(401117, 500134)]
    needle = 'bSkillAtk,"WL_CHAINLIGHTNING",.@sum*2;'
    require(after.count(needle) == 1, 'Expected one fixed combo key')
    before = after.replace(needle, 'bSkillAtk,"WL_CHAINLIGHTNING_ATK",.@sum*2;')
    for name, script in [('cardinal', items[401118]['Script']), ('hyper_before', before), ('hyper_after', after)]:
        (build / (name + '.script')).write_text('{\n' + script + '}\n')
        print(name + '_sha256=' + hashlib.sha256(script.encode()).hexdigest(), flush=True)
    skill_names = ('CD_ARBITRIUM', 'CD_ARBITRIUM_ATK', 'CD_FRAMEN', 'HN_NAPALM_VULCAN_STRIKE',
                   'WL_CHAINLIGHTNING', 'WL_CHAINLIGHTNING_ATK')
    fixture = [{key: skills[name][key] for key in ('Id', 'Name', 'Description', 'MaxLevel')} for name in skill_names]
    (build / 'skills.yml').write_text(yaml.safe_dump(fixture))
    sources = ['src/map/pc.cpp', 'src/map/skill.cpp', 'src/map/script.cpp', 'src/common/malloc.cpp', 'src/map/clif.cpp',
               str(PACKAGE.relative_to(ROOT) / 'native_bonus_vm_test.cpp')]
    hashes = {source: hashlib.sha256((ROOT / source).read_bytes()).hexdigest() for source in sources}
    objects = sorted(path for path in (ROOT / 'src/map/obj').rglob('*.o')
                     if path.name not in {'pc.o', 'skill.o', 'script.o', 'clif.o'})
    libraries = [ROOT / p for p in ('src/common/obj/common.a', '3rdparty/libconfig/obj/libconfig.a', '3rdparty/rapidyaml/obj/ryml.a')]
    require(objects and all(p.is_file() for p in libraries), 'Previously built local Linux support objects are required')
    sanitizer = ['-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-omit-frame-pointer']
    includes = ['src', '3rdparty/libconfig', '3rdparty/rapidyaml/src', '3rdparty/rapidyaml/ext/c4core/src',
                '3rdparty/json/include', '/usr/include/mysql']
    flags = ['g++', '-std=c++17', '-O0', '-g', '-DPACKETVER=20260219', '-fno-strict-aliasing'] + sanitizer
    flags += ['-I' + p for p in includes]
    def compile_one(source):
        output = build / (Path(source).stem + '.o')
        print('Fresh compile ' + source + ' sha256=' + hashes[source], flush=True)
        subprocess.run(flags + ['-c', source, '-o', str(output)], cwd=ROOT, check=True)
        return output
    with ThreadPoolExecutor(max_workers=2) as pool:
        fresh = list(pool.map(compile_one, sources))
    require(hashes == {s: hashlib.sha256((ROOT / s).read_bytes()).hexdigest() for s in sources},
            'Production source changed during compilation; rerun against a stable snapshot')
    executable = build / 'native_bonus_vm_test'
    wrappers = ('main', '_Z9map_id2sdi', '_Z9map_id2ndi', '_Z11mapreg_initv', '_Z12mapreg_finalv',
                '_Z17npc_event_dequeueP16map_session_datab', '_Z9ShowErrorPKcz')
    command = ['g++'] + sanitizer + ['-o', str(executable)] + [str(p) for p in fresh + objects + libraries]
    command += ['-Wl,--wrap=' + name for name in wrappers]
    command += ['-lz', '-ldl', '-lmysqlclient', '-lzstd', '-lssl', '-lcrypto', '-lresolv', '-lm']
    subprocess.run(command, cwd=ROOT, check=True)
    completed = subprocess.run([str(executable), str(build)], cwd=ROOT, capture_output=True, text=True, timeout=60)
    print(completed.stdout, end=''); print(completed.stderr, end='', file=sys.stderr)
    validate_output(completed)
    print(json.dumps({'result': 'PASS', 'fresh_source_hashes': hashes,
                      'vm_executions': 44205, 'asan_ubsan': True, 'network': 'kernel denied',
                      'unrelated_link_objects': 'previous local build; not a complete fresh server build'}, indent=2))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, help='Explicit isolated Linux artifact directory')
    args = parser.parse_args()
    if args.build_dir:
        build = args.build_dir.resolve()
        require(build != ROOT and ROOT not in build.parents, 'Keep generated artifacts outside the repository')
        build.mkdir(parents=True, exist_ok=True)
        run(build)
    else:
        with tempfile.TemporaryDirectory(prefix='rathena-crown-bonus-vm-') as directory:
            run(Path(directory))
