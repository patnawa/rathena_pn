#!/usr/bin/env python3
"""Byte-preservation and actual native NPC gate lifecycle regression (Linux/WSL).

Only generated build fixtures are written. This program never installs/rewrites
the production Garden file. --candidate-from-live tests an uninstalled exact
candidate; the default reconstructs the hash-pinned old live source from the
hash-pinned installed candidate, so a clean checkout needs no sibling snapshot.
CRLF is normalized to LF only in the active test view; raw source bytes are
never rewritten and must remain unchanged throughout the run's checkpoints.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]
LIVE = ROOT.parent / 'biosphere-live-callback-drift-20260906/garden_of_time.txt'
SOURCE = ROOT / 'npc/re/quests/garden_of_time.txt'
LOCAL_SHA = '2ad1160409c13f73326790995cabac7ab8359443f0e5f32ba9569aad5c56dd91'
LIVE_SHA = '0505f6c6980642ef05c132652278ed42da06a294c977c00a4e44c2b94125431f'
CANDIDATE_SHA = '3e5a98758e70be050a0b61b3d23e3d0a3dae1c45e3729cc1988dd2c60fa0f301'
WRAPS = (
    'main', '_Z11mapreg_initv', '_Z12mapreg_finalv', '_Z9ShowErrorPKcz',
    '_Z9map_id2sdi', '_Z9map_id2ndi', '_Z9map_id2bli', '_Z13map_charid2sdi',
    '_Z15map_blid_existsi', '_Z17map_mapname2mapidPKc', '_Z17mapindex_name2idxPKcS0_',
    '_Z10map_addnpcsP8npc_data', '_Z12map_addblockP10block_list',
    '_Z12map_delblockP10block_list', '_Z11map_addiddbP10block_list', '_Z11map_deliddbP10block_list',
    '_Z11mapit_alloc12e_mapitflags7bl_type', '_Z10mapit_freeP13s_mapiterator',
    '_Z11mapit_firstP13s_mapiterator', '_Z10mapit_nextP13s_mapiterator', '_Z12mapit_existsP13s_mapiterator',
    '_Z16map_foreachinmapPFiP10block_listP13__va_list_tagEsiz',
    '_Z14map_foreachnpcPFiP8npc_dataP13__va_list_tagEz',
    '_Z10clif_spawnPK10block_listb', '_Z24clif_changeoption_targetPK10block_listS1_',
    '_Z19clif_clearunit_areaRK10block_list8clr_type',
    '_Z14clif_scriptmesRK16map_session_datajPKc', '_Z16clif_scriptcloseRK16map_session_dataj',
    '_Z10run_scriptP11script_codeiii',
)


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def candidate(live: bytes) -> bytes:
    assert sha(live) == LIVE_SHA, 'Preserved live snapshot drift'
    assert len(live) == 67395 and b'\r' not in live and live.count(b'\n') == 1735
    assert [(i, c) for i, c in enumerate(live) if c >= 128] == [(23806, 0xa1), (23807, 0xd8)]
    result = live
    for x, number in ((158, 1), (173, 2)):
        old = f'\nt_garden,{x},235,2\tscript(CLOAKED)\tDimensional Prison#{number}\tGATE_SKYBLUE,{{\n'.encode('ascii')
        new = old.replace(b'script(CLOAKED)', b'script(DISABLED)')
        assert result.count(old) == 1, 'Ambiguous active gate declaration'
        result = result.replace(old, new)
    assert len(result) == 67397 and sha(result) == CANDIDATE_SHA
    before, after = live.splitlines(keepends=True), result.splitlines(keepends=True)
    differences = [(i + 1, a, b) for i, (a, b) in enumerate(zip(before, after)) if a != b]
    assert len(before) == len(after) and [x[0] for x in differences] == [983, 1039]
    assert all(a.replace(b'script(CLOAKED)', b'script(DISABLED)') == b for _, a, b in differences)
    print('BYTE_PRESERVATION: exact live baseline; only active declarations 983/1039 change; A1D8 retained; no transcoding', flush=True)
    return result


def normalized_candidate(active: bytes) -> bytes:
    normalized = active.replace(b'\r\n', b'\n')
    assert sha(normalized) == CANDIDATE_SHA, 'Installed Garden source drift beyond CRLF/LF checkout differences'
    return normalized


def original_live(active: bytes) -> bytes:
    """Invert only the approved flags, then prove the ENTIRE original identity."""
    restored = normalized_candidate(active)
    for x, number in ((158, 1), (173, 2)):
        current = f'\nt_garden,{x},235,2\tscript(DISABLED)\tDimensional Prison#{number}\tGATE_SKYBLUE,{{\n'.encode('ascii')
        previous = current.replace(b'script(DISABLED)', b'script(CLOAKED)')
        assert restored.count(current) == 1, 'Ambiguous active declaration during inverse reconstruction'
        restored = restored.replace(current, previous)
    assert sha(restored) == LIVE_SHA, 'Inverse reconstruction is not the hash-pinned original live source'
    return restored


def confirm_snapshot(restored: bytes, independent: bytes):
    assert sha(independent) == LIVE_SHA and independent == restored, 'Independent live snapshot disagrees with reconstructed original'


def newline_self_test(fixed: bytes) -> dict:
    """Positive Windows checkout view and fail-closed non-newline controls."""
    crlf = fixed.replace(b'\n', b'\r\n')
    assert normalized_candidate(fixed) == fixed
    assert normalized_candidate(crlf) == fixed
    old = original_live(fixed)
    assert original_live(crlf) == old and sha(old) == LIVE_SHA
    assert bytes(c for c in crlf if c >= 128) == bytes(c for c in fixed if c >= 128) == b'\xa1\xd8'
    rejected = (
        lambda: normalized_candidate(crlf.replace(b'GATE_SKYBLUE', b'GATE_SKYRED', 1)),
        lambda: normalized_candidate(crlf.replace(b'\xa1\xd8', b'\xa1\xd9', 1)),
        lambda: normalized_candidate(fixed + b'\r'),
        lambda: confirm_snapshot(old, old.replace(b'\n', b'\r\n')),
    )
    for operation in rejected:
        try:
            operation()
        except AssertionError:
            continue
        raise AssertionError('Newline-only acceptance incorrectly accepted a non-newline change or transcoded snapshot')
    print('NEWLINE_VALIDATION_PASS: 4 in-memory positives; 4 rejection controls; A1D8 preserved; raw snapshot remains exact', flush=True)
    return {'positive_checks': 4, 'negative_checks': 4, 'synthetic_crlf_raw_sha256': sha(crlf)}


def baseline(active: bytes, draft: bool, snapshot_path: Path) -> tuple[bytes, dict]:
    if draft:
        assert snapshot_path.is_file(), '--candidate-from-live requires an explicit existing live snapshot'
        live = snapshot_path.read_bytes()
        assert sha(live) == LIVE_SHA, 'Preserved live snapshot drift'
        construction = 'explicit_live_snapshot'
    else:
        live = original_live(active)
        construction = 'inverse_only_two_active_declarations_with_full_original_sha256'
    confirmed = snapshot_path.exists()
    if confirmed:
        confirm_snapshot(live, snapshot_path.read_bytes())
    print(f'OLD_SOURCE: {construction}; SHA256 {sha(live)}; independent snapshot confirmed={confirmed}', flush=True)
    return live, {
        'construction': construction,
        'old_live_source_sha256': sha(live),
        'independent_snapshot_confirmed': confirmed,
        'independent_snapshot_path': str(snapshot_path) if confirmed else None,
    }


def declaration(source: bytes, name: bytes) -> bytes:
    pattern = re.compile(rb'(?m)^t_garden,[^\n]+\tscript\([^\n]+\)\t' + re.escape(name) + rb'\t[^\n]+\{\n')
    matches = list(pattern.finditer(source))
    assert len(matches) == 1, f'Exactly one active declaration required: {name!r}'
    start = matches[0].start()
    brace = source.index(b'{', start)
    depth, quote, escaped, line_comment, block_comment = 0, False, False, False, False
    i = brace
    while i < len(source):
        c, following = source[i], source[i + 1:i + 2]
        if line_comment:
            if c == 10: line_comment = False
        elif block_comment:
            if c == 42 and following == b'/': block_comment = False; i += 1
        elif quote:
            if escaped: escaped = False
            elif c == 92: escaped = True
            elif c == 34: quote = False
        elif c == 47 and following == b'/': line_comment = True; i += 1
        elif c == 47 and following == b'*': block_comment = True; i += 1
        elif c == 34: quote = True
        elif c == 123: depth += 1
        elif c == 125:
            depth -= 1
            if depth == 0:
                assert source[i + 1:i + 2] == b'\n'
                return source[start:i + 2]
        i += 1
    raise AssertionError('Unbalanced source declaration')


def fixtures(build: Path, draft: bool, snapshot_path: Path, raw_active: bytes) -> tuple[dict[str, Path], dict, dict]:
    active = raw_active.replace(b'\r\n', b'\n')
    live, provenance = baseline(raw_active, draft, snapshot_path)
    fixed = candidate(live)
    newline_proof = newline_self_test(fixed)
    if draft:
        assert sha(raw_active) == LOCAL_SHA or sha(active) == CANDIDATE_SHA, 'Unexpected current source while preparing candidate'
    else:
        assert active == fixed, 'Installed Garden source is not the reviewed byte-preserving candidate'
    selected = fixed if draft else active
    names = [b'Dimensional Prison#1', b'Dimensional Prison#2']
    result = {}
    for key, source in (('fixed', selected), ('old', live)):
        payload = b'\n'.join(declaration(source, name) for name in names)
        path = build / f'garden_{key}_gates.txt'
        path.write_bytes(payload)
        result[key] = path
        print(f'FIXTURE {key}: source SHA256 {sha(source)}; exact complete gate declarations SHA256 {sha(payload)}', flush=True)
    # These are the two exact named calls in the live completed-quest reveal
    # branch. The surrounding quest/story scene is explicitly not executed.
    reveal = b'{\n'
    for name in names:
        call = b'\t\tcloakoffnpcself( "' + name + b'" );\n'
        assert live.count(call) == 2 and selected.count(call) == 2
        reveal += call
    reveal += b'end;\n}\n'
    result['reveal'] = build / 'garden_reveal.txt'
    result['reveal'].write_bytes(reveal)
    timer = b'{\n'
    for relative in ('npc/custom/instances/LakeOfFire.txt', 'npc/custom/instances/HallOfLife.txt'):
        content = (ROOT / relative).read_bytes().replace(b'\r\n', b'\n')
        start = content.index(b'\nOnTimer1000:\n') + len(b'\nOnTimer1000:\n')
        end = content.index(b'\tend;\n}', start)
        section = content[start:end]
        assert section.count(b'disablenpc ') == 2
        timer += section
    timer += b'end;\n}\n'
    result['timers'] = build / 'garden_existing_suppression.txt'
    result['timers'].write_bytes(timer)
    for name, path in result.items():
        assert len(os.path.relpath(path, ROOT).encode()) < 80, f'Fixture path exceeds native NPC path-key bound: {name}'
    return result, provenance, newline_proof


def run(build: Path, draft: bool, reuse: bool, snapshot_path: Path):
    raw_active = SOURCE.read_bytes()
    normalized_active = raw_active.replace(b'\r\n', b'\n')
    build.mkdir(parents=True, exist_ok=True)
    paths, provenance, newline_proof = fixtures(build, draft, snapshot_path, raw_active)
    assert SOURCE.read_bytes() == raw_active, 'Raw active source changed during fixture preparation'
    print(f'ACTIVE_SOURCE raw SHA256 {sha(raw_active)}; LF-test-view SHA256 {sha(normalized_active)}; CRLF pairs normalized in memory={raw_active.count(bytes([13, 10]))}', flush=True)
    objects = sorted(p for p in (ROOT / 'src/map/obj').rglob('*.o') if p.name not in ('script.o', 'npc.o'))
    libraries = [ROOT / p for p in ('src/common/obj/common.a', '3rdparty/libconfig/obj/libconfig.a', '3rdparty/rapidyaml/obj/ryml.a')]
    assert objects and all(p.is_file() for p in libraries), 'Existing Linux map support objects required'
    sanitizer = ['-fsanitize=address,undefined', '-fno-sanitize-recover=all']
    flags = ['g++', '-std=c++17', '-O0', '-g', '-DPACKETVER=20260219', '-fno-strict-aliasing', '-fno-omit-frame-pointer'] + sanitizer
    flags += ['-I' + p for p in ('src', '3rdparty/libconfig', '3rdparty/rapidyaml/src', '3rdparty/rapidyaml/ext/c4core/src', '3rdparty/json/include', '/usr/include/mysql')]
    compiled = []
    for source in ('src/map/script.cpp', 'src/common/malloc.cpp', 'tools/ci/garden_legacy_gate_test.cpp'):
        target = build / (Path(source).stem + '.o')
        extra = ['src/map/npc.cpp'] if source.endswith('garden_legacy_gate_test.cpp') else []
        pin = '\n'.join(f'{p} {sha((ROOT / p).read_bytes())}' for p in [source] + extra)
        receipt = target.with_suffix('.sources')
        if reuse and target.is_file() and receipt.is_file() and receipt.read_text() == pin:
            print('Reusing hash-identical fresh test object ' + source, flush=True)
        else:
            print('Compiling current ' + pin, flush=True)
            subprocess.run(flags + ['-c', source, '-o', str(target)], cwd=ROOT, check=True)
            assert pin == '\n'.join(f'{p} {sha((ROOT / p).read_bytes())}' for p in [source] + extra), 'Source changed during compilation'
            receipt.write_text(pin)
        compiled.append(target)
    executable = build / 'garden_legacy_gate_test'
    subprocess.run(['g++'] + sanitizer + ['-o', str(executable)] + [str(p) for p in compiled + objects + libraries] +
                   ['-Wl,--wrap=' + name for name in WRAPS] + ['-lz', '-ldl', '-lmysqlclient', '-lzstd', '-lssl', '-lcrypto', '-lresolv', '-lm'], cwd=ROOT, check=True)
    results = {}
    for label in ('fixed', 'old'):
        assert SOURCE.read_bytes() == raw_active, 'Raw active source changed during build/run'
        arguments = [str(executable)] + [os.path.relpath(paths[k], ROOT) for k in (label, 'reveal', 'timers')]
        completed = subprocess.run(arguments, cwd=ROOT, text=True, capture_output=True, timeout=60)
        output = completed.stdout + completed.stderr
        print(f'{label.upper()} native lifecycle:\n' + output, end='', flush=True)
        (build / f'{label}.stdout.log').write_text(completed.stdout)
        (build / f'{label}.stderr.log').write_text(completed.stderr)
        assert not re.search(r'AddressSanitizer|UndefinedBehaviorSanitizer|runtime error:|invalid.free|\[Error\]|\[Warning\]|memory leak|overflow|underflow', output.replace('No memory leaks found.', ''), re.I), 'Native diagnostic'
        assert 'Memory manager: No memory leaks found.' in output, 'Missing clean allocator teardown'
        match = re.search(r'GARDEN_NATIVE_RESULT cases=(\d+) assertions=(\d+) failures=(\d+) errors=(\d+) gate_vm_calls=(\d+)', output)
        assert match, 'Missing native completion receipt'
        cases, assertions, failures, errors, calls = map(int, match.groups())
        assert errors == 0 and cases == 20 and assertions == 132
        if label == 'fixed': assert completed.returncode == 0 and failures == 0 and calls == 4
        else: assert completed.returncode == 2 and failures == 68 and calls == 20
        results[label] = tuple(map(int, match.groups()))
    assert results['fixed'][:2] == results['old'][:2], 'Positive and old-source controls must execute the same assertions'
    assert SOURCE.read_bytes() == raw_active, 'Raw active source changed during native proof'
    (build / 'receipt.json').write_text(json.dumps({
        'candidate_from_live_not_installed': draft,
        'source_raw_sha256': sha(raw_active),
        'source_normalized_lf_sha256': sha(normalized_active),
        'source_raw_unchanged_at_all_checkpoints': True,
        'active_test_view_normalization': 'CRLF to LF byte replacement only; no decoding/transcoding or source write',
        'active_crlf_pairs_normalized_in_memory': raw_active.count(b'\r\n'),
        'newline_self_test': newline_proof,
        'old_live_baseline': provenance,
        'sources': {p: sha((ROOT / p).read_bytes()) for p in (
            'src/map/npc.cpp', 'src/map/script.cpp', 'src/common/malloc.cpp',
            'tools/ci/garden_legacy_gate_test.cpp', 'tools/ci/garden_legacy_gate_test.py')},
        'fixture_sha256': {name: sha(path.read_bytes()) for name, path in paths.items()},
        'native_columns': ['cases', 'assertions', 'failures', 'errors', 'gate_vm_calls'],
        'native_results': results,
        'sanitizers': 'address,undefined; no recovery or suppression',
        'boundaries': 'Private NPC registries; explicit world registration/iteration and packet recorders; no transport, rendering, SQL, full quest scene, instance policy, or reward execution',
    }, indent=2) + '\n')
    print('PASS: exact byte preservation; real NPC declaration/disable/reveal/click/unload/reparse; old live source fails hardening assertions; no production writes', flush=True)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, default=ROOT.parent / 'garden-legacy-native-20260906')
    parser.add_argument('--candidate-from-live', action='store_true', help='Test exact in-memory candidate before production installation')
    parser.add_argument('--live-snapshot', type=Path, default=LIVE,
                        help='Optional independent original snapshot; required only with --candidate-from-live')
    parser.add_argument('--reuse-unchanged-test-objects', action='store_true', help='Reuse only hash-identical sanitized test objects; default recompiles all')
    arguments = parser.parse_args()
    run(arguments.build_dir.resolve(), arguments.candidate_from_live, arguments.reuse_unchanged_test_objects,
        arguments.live_snapshot.resolve())
