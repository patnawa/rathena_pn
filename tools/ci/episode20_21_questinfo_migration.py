#!/usr/bin/env python3
# ============================================================================
#  PN  /  DEVELOPMENT TOOLS
#  episode20_21_questinfo_migration.py
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/episode20_21_questinfo_migration.py
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

"""Read-only, exact Episode20/21 questinfo relocation proposal and verifier.

No runtime application, automatic rebaseline, server action, or native gameplay
claim. --emit-patch prints a reviewable patch; applying it requires a separately
authorized action. Only newline-only checkout normalization is accepted.
"""
from __future__ import annotations
import argparse
from collections import Counter
import difflib
import hashlib
import json
from pathlib import Path
import re

import biosphere_callback_closure_audit as base
from biosphere_material_callback_audit import lexical_views, body_end
import episode21_family_supply_overlay as family_overlay

ROOT = Path(__file__).resolve().parents[2]
IMPORT_GRAPH_SHA = 'c1054ed60893c7fd996897328b67b194073a0b66f1231438627119d431cb59b7'
BEFORE = {
    "npc/custom/episode20/Progression.txt": "ab08d7b63804e85aa892e0c315e945f13c37d118af92e58cd3e47d22c0e0c649",
    "npc/custom/episode20/SidesAndDailies.txt": "895e602387edce65bb352cd17e51cbe88e0c5e83e4766bfa8bb08c6e2f5f37d4",
    "npc/custom/episode21/Progression.txt": "0def3b01c11e6302d8d6133db2fe05713a2c10f320ce244f59f7c0ea1b6453d3",
    "npc/custom/episode21/GimliInfiltration.txt": "1b719604835239f32a6e09e5d4367540fafa4fe487a9a13bce0fa3c3c9d64293",
    "npc/custom/episode21/MysteriousGhostShip.txt": "58c336e56c135390cf47f98526f598433ec87f3384213ed6f3f4f46bba229965",
    "npc/custom/episode21/BlackHairedBeast.txt": "40e9f25f960a1cb3b73a7c757ba63167b65d38d7830ceb442fef758feb76e151",
    "npc/custom/episode21/FamilyReputation.txt": "d49ef88c4d7b2ef1a16fccff22f0f13aa82bf790c9a6a92a42ef66b4d8c39f17",
    "npc/custom/episode21/FinalBattle.txt": "3873d72118f6cf83374c445e6891eaf9a79660fec71c5e12b3bb02872e4625f1",
    "npc/custom/episode21/SecretAltar.txt": "866e4c4590b8de634b1e5f78ce5dbf047c0129aabdea5bf0bb9cf04dff4b9022",
    "npc/custom/episode21/SilentSanctuary.txt": "cbd658dd184ecce5a15eedf1774c60d62bb3478c11f8238201f5c42a9cbc27bb",
    "npc/custom/episode21/SideDailies.txt": "1b305582511432b4fc76d737c907240ac7064dc4cc79f8606632938e869ec09c"
}
AFTER = {
    'npc/custom/episode20/Progression.txt': '1fb4e3b719877f7a0bf25fe4f22fea06b181b95289f6ee8f8844a1593b26920f',
    'npc/custom/episode20/SidesAndDailies.txt': '4f43339e03f7ad71165c4b158e83bd5b468bd0b53813443414df3004af1b0e9a',
    'npc/custom/episode21/Progression.txt': '812e3a1e7c5a6ffedb4ca9fbb7a98e5160485cf577d834ed2e97a5e7b367b2b6',
    'npc/custom/episode21/GimliInfiltration.txt': 'cb51fe24b554f896a8702f8cd62430b78e3b52ea7f4e4f640c3c36188bd34273',
    'npc/custom/episode21/MysteriousGhostShip.txt': '011f74d5b6abc6833be6dbb70cd19c7b123ed057a4a9bbe677528aa85f9318a6',
    'npc/custom/episode21/BlackHairedBeast.txt': 'b00fdd28cd0ea4703bbbfdfb8a3f7336f74d38be02b93eddde30029996593b53',
    'npc/custom/episode21/FamilyReputation.txt': 'c72019f51033d96054c277d952d993a45a68062da9f7b4ab99ee76edde2a9605',
    'npc/custom/episode21/FinalBattle.txt': 'd93ccfe36f48cccaf041e15d24c6036cc2fd908afe89103bd5a1dd3924558ace',
    'npc/custom/episode21/SecretAltar.txt': '18b816122fd5a3fbb7e4db0d7efcf9533ecec2ebc799cbd0a8f99c87c87f7970',
    'npc/custom/episode21/SilentSanctuary.txt': 'e5551f3cceeefa1b2dbf2ef85743c58a282152a83526743d9e1439a143292a0e',
    'npc/custom/episode21/SideDailies.txt': 'e031113a8e040385a6d395187d0c00e45f2239e0dd9197299f2c603758cf764c',
}
COUNTS = {
    "npc/custom/episode20/Progression.txt": 51,
    "npc/custom/episode20/SidesAndDailies.txt": 20,
    "npc/custom/episode21/Progression.txt": 26,
    "npc/custom/episode21/GimliInfiltration.txt": 6,
    "npc/custom/episode21/MysteriousGhostShip.txt": 3,
    "npc/custom/episode21/BlackHairedBeast.txt": 9,
    "npc/custom/episode21/FamilyReputation.txt": 1,
    "npc/custom/episode21/FinalBattle.txt": 1,
    "npc/custom/episode21/SecretAltar.txt": 4,
    "npc/custom/episode21/SilentSanctuary.txt": 2,
    "npc/custom/episode21/SideDailies.txt": 2
}
UNCHANGED = {
    'npc/custom/episode20/Fields.txt': '4bfdcb51f8a7b4c1a12b95c4fe862cf1bcd9190a879c3dde1c3b49a90742afab',
    'npc/custom/episode20/Instances.txt': 'db617bf25b4f010eea721e0de1aec5ef0d2ef56a8de7b50704711dd5139d3232',
    'npc/custom/episode21/EquipmentServices.txt': '9bdf26310387b413c4bc65ec550dc42c8851d7e4e059aba5911ec31e23f4f5b5',
    'npc/custom/episode21/FamilyBlessings.txt': '3d8f445842568a6585803a39798bc2f4363c24b95d2b4a6b97918b55532dac08',
    'npc/custom/episode21/FieldMonsters.txt': '49eea98fd03ffa374c7e9dbc1c26b84a2ac780b10cfcc8c0728139a1f2fa528f',
}
SELECTED = sorted([
    'npc/custom/episode20/' + p for p in
    ('Fields.txt', 'Instances.txt', 'Progression.txt', 'SidesAndDailies.txt')
] + [
    'npc/custom/episode21/' + p for p in
    ('BlackHairedBeast.txt', 'EquipmentServices.txt', 'FamilyBlessings.txt',
     'FamilyReputation.txt', 'FieldMonsters.txt', 'FinalBattle.txt',
     'GimliInfiltration.txt', 'MysteriousGhostShip.txt', 'Progression.txt',
     'SecretAltar.txt', 'SideDailies.txt', 'SilentSanctuary.txt')
])
MAP_COUNTS = {
    "icas_in": 14,
    "icas_in2": 3,
    "icecastle": 12,
    "jalbe_in": 2,
    "jor_albe": 4,
    "jor_back1": 2,
    "jor_back2": 1,
    "jor_back3": 1,
    "jor_back4": 1,
    "jor_back5": 2,
    "jor_crk": 2,
    "jor_crk_p": 6,
    "jor_maze": 9,
    "jor_mbase": 16,
    "jor_nest": 1,
    "jor_raise1": 1,
    "jor_root1": 6,
    "jor_root2": 1,
    "jor_safty1": 1,
    "jor_sanct": 8,
    "jor_tail": 1,
    "jor_tmple1": 4,
    "jor_twice": 4,
    "jor_twig": 6,
    "luna_sf1": 4,
    "luna_sf2": 5,
    "mbase_in": 8
}
HELPER_CALLS = {
    'isbegin_quest', 'EP20_MainComplete', 'EP20_DailyCanAccept', 'EP21_Started',
    'EP21_GaebolgComplete', 'EP21_CultComplete', 'EP21_GimliComplete',
    'EP21_GhostShipUnlocked', 'EP21_MainComplete', 'EP21_DailyKey',
}
MAND = ('npc/custom/episode21/FamilyReputation.txt', 'Mandel#ep21_reputation')
BOUNDARY = '\tend;\n\nOnInit:\n'


def require(ok, message):
    if not ok:
        raise AssertionError(message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def normalized(data):
    """Do not discard a BOM, replace invalid text, or normalize lone CR bytes."""
    return data.replace(b'\r\n', b'\n')


def inspect(text, path):
    """Account for every registration token and its exact static NPC owner."""
    clean, masked = lexical_views(text)
    owners = []
    pattern = r'^([^\s,]+),\d+,\d+,\d+\s+script(?:\([^)]*\))?\s+[^\n]+'
    for h in re.finditer(pattern, masked, re.M):
        header = clean[h.start():h.end()].rstrip()
        start = masked.find('{', h.start(), h.end())
        require(start >= 0, 'Missing source body: ' + header)
        end = body_end(masked, start)
        fields = header.split('\t')
        require(len(fields) == 4, 'Unreviewed NPC header: ' + header)
        owners.append((h, header, fields[2], start, end))
    rows = []
    for q in re.finditer(r'\b(?:questinfo|questinfo_refresh|showevent)\b', masked):
        matches = [o for o in owners if o[3] < q.start() < o[4]]
        require(len(matches) == 1, 'Unowned or multiply owned registration: ' + path)
        h, header, name, start, end = matches[0]
        require(header.split('\t')[1] == 'script', 'Only ordinary static registration owners reviewed')
        statement_end = masked.find(';', q.start(), end)
        require(statement_end >= 0, 'Unterminated registration: ' + name)
        statement = text[q.start():statement_end + 1]
        literal = re.fullmatch(
            r'questinfo\s+(QTYPE_\w+)\s*,\s*(QMARK_\w+)\s*,\s*"([^"\\\n]*)"\s*;',
            statement)
        require(literal is not None, 'Nonliteral/unreviewed registration: ' + name)
        icon, color, condition = literal.groups()
        require(icon in ('QTYPE_QUEST', 'QTYPE_DAILYQUEST') and color == 'QMARK_YELLOW',
                'Changed icon/color contract')
        require(not re.search(r'[.@\x27$]', condition), 'Non-character condition scope')
        calls = set(re.findall(r'\b([A-Za-z_]\w*)\s*\(', condition))
        require(calls <= HELPER_CALLS, 'New condition helper requires manual review')
        line_start = text.rfind('\n', start, q.start()) + 1
        line_end = text.find('\n', statement_end, end)
        require(line_end >= 0 and text[line_start:line_end] == '\t' + statement,
                'Expected exact standalone tab-indented statement')
        labels = re.findall(r'^\s*(\w+):', masked[start:end], re.M)
        rows.append({
            'path': path, 'line': text.count('\n', 0, q.start()) + 1,
            'map': h[1], 'npc': name, 'header': header,
            'icon': icon, 'color': color, 'condition': condition,
            'statement': statement, 'labels': labels,
            'start': start, 'end': end, 'line_start': line_start,
            'line_end': line_end + 1,
        })
    require(len(rows) == COUNTS.get(path, 0), 'Changed per-file registration count: ' + path)
    require(len({r['npc'] for r in rows}) == len(rows), 'Repeated owner in source')
    return rows


def before_labels(row):
    return ['L_Reputation'] if (row['path'], row['npc']) == MAND else []


def transform(path, text):
    """Change only the location of each exact first registration statement."""
    rows = inspect(text, path)
    result = text
    for row in reversed(rows):
        start, end = row['start'], row['end']
        original = text[start:end]
        line = '\t' + row['statement'] + '\n'
        require(row['labels'] == before_labels(row), 'Existing/unreviewed owner label')
        require(original.startswith('{\n' + line), 'Registration is not unique first literal statement')
        # The final literal } is established by the string/comment-aware scanner.
        remaining = '{\n' + original[2 + len(line):]
        candidate = remaining[:-1] + BOUNDARY + line + '\tend;\n}'
        result = result[:start] + candidate + result[end:]
    return result, rows


def inverse(path, text):
    """Reconstruct the genuine original; no approximation or comment stripping."""
    rows = inspect(text, path)
    result = text
    for row in reversed(rows):
        start, end = row['start'], row['end']
        candidate = text[start:end]
        line = '\t' + row['statement'] + '\n'
        suffix = BOUNDARY + line + '\tend;\n}'
        require(row['labels'] == before_labels(row) + ['OnInit'], 'Unexpected initialized-owner labels')
        require(candidate.startswith('{\n') and candidate.endswith(suffix),
                'Changed exact OnInit suffix or missing dialogue terminator')
        require(candidate.count(BOUNDARY) == 1, 'Duplicate initialization boundary')
        remaining = candidate[:-len(suffix)] + '}'
        original = '{\n' + line + remaining[2:]
        result = result[:start] + original + result[end:]
    return result


def pair(path, data):
    """Accept only a whole reviewed before file or its whole reviewed after file."""
    norm = normalized(data)
    text = norm.decode('utf-8')
    digest = sha(norm)
    if digest == BEFORE[path]:
        previous = text
        proposed, rows = transform(path, text)
        require(sha(proposed.encode()) == AFTER[path], 'Unreviewed proposed whole-file result')
        require(inverse(path, proposed) == previous, 'Exact proposed inverse failed')
        return 'before', previous, proposed, rows
    require(digest == AFTER[path], 'Unreviewed source bytes: ' + path)
    proposed = text
    previous = inverse(path, proposed)
    require(sha(previous.encode()) == BEFORE[path], 'Original whole-file reconstruction failed')
    rebuilt, rows = transform(path, previous)
    require(rebuilt == proposed, 'Exact forward reconstruction failed')
    return 'after', previous, proposed, rows


def active_pair(path, data):
    """Pair an installed source while keeping Family transaction edits separate.

    ``pair`` remains the exact QuestInfo-only before/after transform. The Family
    adapter first removes only the complete, hash-pinned transaction overlay;
    arbitrary combined edits therefore cannot enter the migration baseline.
    """
    norm = normalized(data)
    overlay_phase = 'none'
    if path == family_overlay.NPC and sha(norm) != BEFORE[path]:
        norm, overlay_phase = family_overlay.canonicalize_for_questinfo(path, norm)
    phase, previous, proposed, rows = pair(path, norm)
    require(overlay_phase == 'none' or phase == 'after',
            'A Family transaction overlay requires the complete QI migration')
    return phase, previous, proposed, rows, overlay_phase


def verify(root=ROOT, *, expected_phase=None):
    root = Path(root).resolve()
    reader = base.Reader(root)
    graph = base.npc_graph(reader)
    config_pins = {p: sha((root / p).read_bytes()) for p in graph['configs']}
    require(all(base.digest(reader.text(p)) == base.digest(
                normalized((root / p).read_bytes()).decode('utf-8')) for p in graph['configs']),
            'Include file changed between graph traversal and raw snapshot')
    require(base.digest(base.canonical(graph['graph'])) == IMPORT_GRAPH_SHA,
            'Enabled import graph changed; manual review required')
    selected = sorted(p for p in graph['scripts']
                      if p.startswith(('npc/custom/episode20/', 'npc/custom/episode21/')))
    require(selected == SELECTED, 'Changed enabled custom episode membership')
    raw = {p: (root / p).read_bytes() for p in SELECTED}
    raw_pins = {p: sha(v) for p, v in raw.items()}
    pairs, phases, rows = {}, set(), []
    overlay_phases = {}
    for path in SELECTED:
        if path not in BEFORE:
            require(sha(normalized(raw[path])) == UNCHANGED[path],
                    'Unrelated enabled episode source changed: ' + path)
            require(inspect(normalized(raw[path]).decode(), path) == [],
                    'New registration in a previously callback-free episode source')
            continue
        phase, previous, proposed, members, overlay_phase = active_pair(path, raw[path])
        pairs[path] = {'before': previous, 'after': proposed}
        phases.add(phase)
        rows += members
        if path == family_overlay.NPC:
            overlay_phases[path] = overlay_phase
    require(len(phases) == 1, 'Mixed migration state: all eleven files must be reviewed together')
    phase = next(iter(phases))
    require(set(overlay_phases) == {family_overlay.NPC},
            'Unexpected Family overlay inventory')
    require((phase == 'before' and overlay_phases[family_overlay.NPC] == 'none') or
            (phase == 'after' and overlay_phases[family_overlay.NPC] in
             ('qi-only', 'family-transaction')),
            'Family overlay phase is incompatible with the QuestInfo migration phase')
    require(expected_phase is None or expected_phase == phase, 'Unexpected migration phase')
    require(len(rows) == 125 and len({(r['path'], r['npc']) for r in rows}) == 125,
            'Exact owner/registration inventory changed')
    require(dict(Counter(r['map'] for r in rows)) == MAP_COUNTS, 'Exact 27-map inventory changed')
    require(all('@' not in r['map'] for r in rows), 'Instance-map ownership requires separate review')
    require(raw_pins == {p: sha((root / p).read_bytes()) for p in SELECTED},
            'Raw active source changed during read-only verification')
    require(config_pins == {p: sha((root / p).read_bytes()) for p in config_pins},
            'Raw include source changed during read-only verification')
    summary = {
        'result': 'SOURCE_ONLY_' + phase.upper() + '_VERIFIED',
        'scope': '125 static literal registrations, not native gameplay or full-map callback proof',
        'phase': phase, 'files': 11, 'owners': 125, 'maps': 27,
        'family_overlay_phase': overlay_phases[family_overlay.NPC],
        'map_counts': MAP_COUNTS, 'file_counts': COUNTS,
        'before_lf_sha256': BEFORE, 'after_lf_sha256': AFTER,
        'active_raw_sha256': raw_pins, 'import_graph_sha256': IMPORT_GRAPH_SHA,
        'include_raw_sha256': config_pins,
        'inventory': [{k: r[k] for k in ('path', 'line', 'map', 'npc', 'icon', 'color', 'condition')}
                      for r in rows],
    }
    return summary, pairs


def negative_controls(root=ROOT):
    summary, pairs = verify(root)
    positives = negatives = 0
    for path, texts in pairs.items():
        for phase in ('before', 'after'):
            source = texts[phase].encode()
            for data in (source, source.replace(b'\n', b'\r\n')):
                found, before, after, _ = pair(path, data)
                require(found == phase and before == texts['before'] and after == texts['after'],
                        'LF/CRLF positive changed exact source semantics')
                positives += 1
            samples = [
                source + b'// unrelated change\n',
                source.replace(b'QMARK_YELLOW', b'QMARK_NONE', 1),
                source.replace(b'questinfo ', b'questinfo_refresh ', 1),
                source.replace(b'4\tscript\t', b'3\tscript\t', 1),
            ]
            require(all(x != source for x in samples), 'Negative control failed to mutate source')
            for data in samples:
                try:
                    pair(path, data)
                except (AssertionError, UnicodeDecodeError):
                    negatives += 1
                else:
                    raise AssertionError('Accepted non-newline whole-file source change')
        previous, proposed = texts['before'], texts['after']
        first = inspect(previous, path)[0]
        injected = previous[:first['start'] + 2] + 'OnBadInit:\n' + previous[first['start'] + 2:]
        try:
            transform(path, injected)
        except AssertionError:
            negatives += 1
        else:
            raise AssertionError('Accepted an existing owner event label')
        broken = proposed.replace(BOUNDARY, '\nOnInit:\n', 1)
        try:
            reconstructed = inverse(path, broken)
            require(sha(reconstructed.encode()) == BEFORE[path],
                    'A preceding old end cannot substitute for the newly inserted terminator')
        except AssertionError:
            negatives += 1
        else:
            raise AssertionError('Accepted missing explicit dialogue terminator')
    family_after = pairs[family_overlay.NPC]['after'].encode()
    candidate = family_overlay.apply_to_qi_only(family_after)
    overlay_positives = overlay_negatives = 0
    for source, expected_overlay in ((family_after, 'qi-only'),
                                     (candidate, 'family-transaction')):
        for data in (source, source.replace(b'\n', b'\r\n')):
            found, before, after, _, overlay_phase = active_pair(family_overlay.NPC, data)
            require(found == 'after' and before == pairs[family_overlay.NPC]['before'] and
                    after == pairs[family_overlay.NPC]['after'] and
                    overlay_phase == expected_overlay,
                    'Family overlay changed QuestInfo-only pair semantics')
            overlay_positives += 1
    for changed in (
            candidate + b' ',
            candidate.replace(b'questinfo_refresh();', b'questinfo_refresh(1);', 1),
            candidate.replace(b'erasequest .@quest[.@choice];', b'erasequest 17782;', 1),
            candidate.replace(b'@inventorylist_amount[.@match] <= 29990',
                              b'@inventorylist_amount[.@match] <= 29991', 1)):
        try:
            active_pair(family_overlay.NPC, changed)
        except (AssertionError, UnicodeDecodeError):
            overlay_negatives += 1
        else:
            raise AssertionError('Accepted an unknown Family transaction overlay')
    try:
        pair(family_overlay.NPC, candidate)
    except (AssertionError, UnicodeDecodeError):
        overlay_negatives += 1
    else:
        raise AssertionError('QuestInfo-only pair directly accepted transaction edits')
    print(f'EPISODE_QI_SOURCE_CONTROLS_OK: {positives} newline positives, '
          f'{negatives} content/shape negatives, {overlay_positives} overlay forms, '
          f'{overlay_negatives} overlay isolation negatives')
    return {'newline_positive': positives, 'negative': negatives,
            'overlay_positive': overlay_positives,
            'overlay_isolation_negative': overlay_negatives}


def patch(pairs):
    """Print-only apply_patch-compatible unified proposal; never edits a source."""
    chunks = ['*** Begin Patch\n']
    for path, texts in pairs.items():
        diff = list(difflib.unified_diff(texts['before'].splitlines(keepends=True),
                                        texts['after'].splitlines(keepends=True),
                                        fromfile=path, tofile=path, n=3))
        chunks.append('*** Update File: ' + path + '\n')
        # apply_patch uses bare @@ hunk boundaries, not numeric unified offsets.
        chunks.extend('@@\n' if line.startswith('@@ ') else line for line in diff[2:])
    chunks.append('*** End Patch\n')
    return ''.join(chunks)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=ROOT)
    parser.add_argument('--phase', choices=('before', 'after'))
    parser.add_argument('--self-test', action='store_true')
    parser.add_argument('--inventory', action='store_true')
    parser.add_argument('--emit-patch', action='store_true')
    args = parser.parse_args()
    summary, pairs = verify(args.root, expected_phase=args.phase)
    if args.emit_patch:
        require(summary['phase'] == 'before', 'Refusing a duplicate migration proposal')
        require(not args.self_test and not args.inventory, 'Patch stdout must contain only the proposal')
        print(patch(pairs), end='')
        return
    if args.self_test:
        summary['source_controls'] = negative_controls(args.root)
    if not args.inventory:
        summary.pop('inventory')
    print(json.dumps(summary, indent=2))


if __name__ == '__main__':
    main()
