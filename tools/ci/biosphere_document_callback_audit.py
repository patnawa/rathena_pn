#!/usr/bin/env python3
# ============================================================================
#  PN  /  DEVELOPMENT TOOLS
#  biosphere_document_callback_audit.py
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/biosphere_document_callback_audit.py
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

"""Reviewed dependency gate for Depth-1 documents -> RepPoints6.

Requires the broad source gate AND the existing ba_in01 material/QuestInfo gate.
This is current-content consistency, not a script sandbox or live persistence
attestation. Collection is explicitly unvalidated. There is no autoaccept mode.
"""
from __future__ import annotations

import argparse
import copy
import json
from pathlib import Path
import re
import sys

import biosphere_callback_closure_audit as broad
import biosphere_material_callback_audit as material_gate

ROOT = Path(__file__).resolve().parents[2]
NPC = 'npc/custom/varmundt_biosphere_depth.txt'
DOCUMENT = 1001289
REPUTATION = 6
CONSTANT = 'REPUTATION_BIOSPHERE_DEPTH1'
VARIABLE = 'RepPoints6'
ENGINE = ['src/map/' + path for path in
          ('pc.cpp', 'pc.hpp', 'script.cpp', 'script.hpp', 'script_constants.hpp', 'clif.cpp', 'packets.hpp')]
ENGINE += ['src/common/database.cpp', 'src/common/database.hpp']
# Reviewed complete section hashes, never generated/accepted by the CLI.
PINS = {
    'document': '900df4193e2cf2ea2c1eab70ccaa83020cdad758565df5deef15ab70cbeafc50',
    'reputation': 'ef75e783cbe4231045ce3deff668923486b0dc4f595bc826e409e2db70de9604',
    'constants': '648df3c16e898c0c7bd421def43e406f34b5a828e47f8bdbc11b1489832027fa',
    'engine': '7be371b0e512ebea566716e9a0bec6d297b43b2fcc10ffefd7e5083f58fd0a44',
    'service': '9518354fe151f2d357d32dcdf89301e47a46c1cda14fc8a0a34975a767b11472',
}


def ordered_graph(reader, root, db_type, identity):
    files, graph, rows, active = {}, [], [], set()

    def visit(path):
        resolved = reader.path(path)
        if resolved in active:
            raise ValueError(f'{db_type} import cycle: {path}')
        active.add(resolved)
        text = reader.text(path)
        files[path] = broad.digest(text)
        data = broad._yaml_load(text, path)
        if set(data) - {'Header', 'Body', 'Footer'} or data.get('Header') != {'Type': db_type, 'Version': 1}:
            raise ValueError(f'Unsupported {db_type} schema: {path}')
        body = data.get('Body') or []
        if not isinstance(body, list) or any(not isinstance(row, dict) or identity not in row for row in body):
            raise ValueError(f'Invalid {db_type} body: {path}')
        for row in body:
            allowed = {'Name', 'Value', 'Parameter'} if identity == 'Name' else {'Id', 'Name', 'Variable', 'Minimum', 'Maximum', 'Visibility'}
            if set(row) - allowed:
                raise ValueError(f'Unknown {db_type} record field: {path}')
            if identity == 'Name':
                if not isinstance(row['Name'], str) or not re.fullmatch(r'[A-Za-z_][A-Za-z_0-9]*', row['Name']) or type(row.get('Value')) is not int:
                    raise ValueError(f'Unsupported constant definition: {path}')
                if 'Parameter' in row and type(row['Parameter']) is not bool:
                    raise ValueError(f'Invalid constant parameter type: {path}')
            else:
                if type(row['Id']) is not int:
                    raise ValueError(f'Invalid reputation identity: {path}')
                for key in ('Minimum', 'Maximum'):
                    if key in row and type(row[key]) is not int:
                        raise ValueError(f'Invalid reputation bound type: {path}')
                for key in ('Name', 'Variable', 'Visibility'):
                    if key in row and not isinstance(row[key], str):
                        raise ValueError(f'Invalid reputation string type: {path}')
                if 'Visibility' in row and row['Visibility'] not in ('Always', 'Never', 'Exist'):
                    raise ValueError(f'Unknown reputation generator visibility: {path}')
        rows.extend(copy.deepcopy(body))
        footer = data.get('Footer') or {}
        if not isinstance(footer, dict) or set(footer) - {'Imports'}:
            raise ValueError(f'Unsupported {db_type} footer: {path}')
        imports = footer.get('Imports') or []
        if not isinstance(imports, list):
            raise ValueError(f'Invalid {db_type} imports: {path}')
        for index, entry in enumerate(imports):
            if not isinstance(entry, dict) or set(entry) - {'Path', 'Mode', 'Generator'}:
                raise ValueError(f'Invalid {db_type} import entry: {path}')
            target, mode = entry.get('Path'), entry.get('Mode', 'Renewal')
            reader.path(target)
            if mode not in ('Renewal', 'Prerenewal'):
                raise ValueError(f'Invalid {db_type} mode: {mode}')
            generator = entry.get('Generator')
            if 'Generator' in entry and type(generator) is not bool:
                raise ValueError(f'Invalid {db_type} generator flag: {path}')
            # YamlDatabase defaults shouldLoadGenerator=false in map-server.
            # ANY present Generator flag is skipped (including false): native
            # accepts it only when shouldLoadGenerator && isGenerator is true.
            selected = mode == 'Renewal' and 'Generator' not in entry
            graph.append([path, index, target, mode, generator, selected])
            if selected:
                visit(target)
        active.remove(resolved)

    visit(root)
    return {'root': root, 'files': files, 'graph': graph, 'ordered_records': rows,
            'ordered_record_count': len(rows)}


def script_body(text, declaration):
    clean, masked = material_gate.lexical_views(text)
    if clean.count(declaration) != 1:
        raise ValueError(f'Missing/ambiguous document-service declaration: {declaration}')
    start = clean.index(declaration)
    opening = masked.find('{', start, masked.find('\n', start))
    if opening < 0:
        raise ValueError('Missing document-service script body')
    return text[opening:material_gate.body_end(masked, opening)]


def _collect(reader, material=None):
    if material is None:
        material = material_gate._collect(reader)
    _, databases = broad.database_graph(reader)
    items = broad.scalar_overlay(databases['db/item_db.yml'], 'Id')
    document = items.get(DOCUMENT)
    if document is None or document.get('AegisName') != 'Bar_D_Docu_1' or document.get('Type') != 'Etc' or document.get('Weight') != 1:
        raise ValueError('Exact Depth-1 research document identity/type/weight changed')
    if any(document.get(field) for field in broad.SCRIPT_FIELDS) or document.get('Locations') or document.get('Stack') or document.get('Flags'):
        raise ValueError('Document acquired a callback/stack/location/behavior flag')
    reputation = ordered_graph(reader, 'db/reputation.yml', 'REPUTATION_DB', 'Id')
    effective = broad.scalar_overlay(reputation['ordered_records'], 'Id')
    selected = effective.get(REPUTATION)
    if selected is None or selected.get('Variable') != VARIABLE or selected.get('Minimum') != -5000 or selected.get('Maximum') != 5000:
        raise ValueError('Reputation 6 variable/bounds changed')
    reputation['selected_record'] = selected
    reputation['selected_ordered_records'] = [row for row in reputation['ordered_records'] if row['Id'] == REPUTATION]
    constants = ordered_graph(reader, 'db/const.yml', 'CONSTANT_DB', 'Name')
    bindings = [row for row in constants['ordered_records'] if row['Name'] == CONSTANT]
    # Native constant registration is not arbitrary last-row-wins overlay.
    if bindings != [{'Name': CONSTANT, 'Value': REPUTATION}]:
        raise ValueError('Exact unique constant binding changed')
    constants['binding'] = bindings[0]
    text = reader.text(NPC)
    declaration = 'ba_in01,292,104,4\tscript\tDepth Research Administrator#bio_d1\t4_EP17_MASTER_A,{'
    access = 'function\tscript\tF_BioDepthQuestAccess\t{'
    return {'schema': 1, 'acceptance': 'UNVALIDATED_COLLECTION',
            'document': {'record': document, 'input_count_per_pair': 2},
            'reputation': reputation, 'constants': constants,
            'engine': {'files': {path: broad.digest(reader.text(path)) for path in ENGINE}},
            'service': {'path': NPC, 'declaration': declaration,
                        'body_sha256': broad.digest(script_body(text, declaration)),
                        'access_body_sha256': broad.digest(script_body(text, access)),
                        'map': 'ba_in01', 'base_level_minimum': 250, 'story_variable': 'ep17_2_main',
                        'story_minimum': 33, 'reputation_per_pair': 3, 'ceiling': 5000,
                        'valid_registry_minimum': -5000, 'valid_registry_maximum': 5000,
                        'final_partial_credit_pair_preserved': True},
            'material': material,
            'live_registry_verified': False, 'deployed_binary_verified': False,
            'graphical_client_verified': False}


def material_collection(manifest):
    value = copy.deepcopy(manifest)
    value.pop('broad_manifest_sha256', None)
    value.pop('profile', None)
    value['acceptance'] = 'UNVALIDATED_COLLECTION'
    return value


def _check(manifest):
    fields = set(PINS) | {'schema', 'acceptance', 'material', 'live_registry_verified', 'deployed_binary_verified', 'graphical_client_verified'}
    if set(manifest) != fields or manifest.get('schema') != 1 or manifest.get('acceptance') != 'UNVALIDATED_COLLECTION' or any(
            manifest.get(key) is not False for key in ('live_registry_verified', 'deployed_binary_verified', 'graphical_client_verified')):
        raise ValueError('Invalid document evidence schema/provenance')
    material_gate._check(material_collection(manifest['material']))
    for section, expected in PINS.items():
        actual = broad.digest(broad.canonical(manifest[section]))
        if actual != expected:
            raise ValueError(f'Document {section} changed; manual review required: expected {expected}, actual {actual}')
    return manifest


def collect_evidence(root=ROOT):
    """Unvalidated fixture inventory; neither prerequisite gate is accepted."""
    return _collect(broad.Reader(root))


def verify_additional_sources(reader, manifest, material):
    """Reread dependencies outside the broad source manifest, not its cache."""
    for inventory in (manifest['reputation'], manifest['constants'],
                      material['achievements'], material['quests']):
        for path, pin in inventory['files'].items():
            if broad.digest(reader.text(path)) != pin:
                raise ValueError(f'Document dependency changed during validation: {path}')


def validate(root=ROOT, *, profile=None):
    """Both prerequisite gates are unconditional, before scoped acceptance."""
    broad_manifest = broad.validate(root, profile=profile)
    material_manifest = material_gate.validate(root, profile=profile)
    if material_manifest['broad_manifest_sha256'] != broad.digest(broad.canonical(broad_manifest)):
        raise ValueError('Broad/material source snapshots differ')
    reader = broad.Reader(root)
    result = _check(_collect(reader, material_manifest))
    expected = {**broad_manifest['engine']['files'], **broad_manifest['databases']['files'],
                **broad_manifest['npcs']['configs'], **broad_manifest['npcs']['scripts'],
                **material_manifest['achievements']['files'], **material_manifest['quests']['files'],
                **result['reputation']['files'], **result['constants']['files']}
    for path, text in reader.cache.items():
        if broad.digest(text) != expected.get(path):
            raise ValueError(f'Document source changed between validation reads: {path}')
    verify_additional_sources(broad.Reader(root), result, material_manifest)
    broad.verify_manifest(root, broad_manifest, profile=profile)
    result['broad_manifest_sha256'] = broad.digest(broad.canonical(broad_manifest))
    result['material_manifest_sha256'] = broad.digest(broad.canonical(material_manifest))
    result['profile'] = profile
    result['acceptance'] = 'REVIEWED_CONTENT_MATCH'
    return result


def negative_controls(root=ROOT, *, profile=None):
    """Full positive validation brackets deliberate in-memory refusals only."""
    accepted = validate(root, profile=profile)
    reader = broad.Reader(root)
    baseline = _check(_collect(reader))
    results = []

    def reject(label, changes):
        injected = broad.Reader(root, changes)
        injected.cache, injected.paths = reader.cache, reader.paths
        try:
            _check(_collect(injected))
        except ValueError:
            results.append(label)
        else:
            raise AssertionError(f'Negative document control accepted: {label}')

    import yaml

    def changed(path, operation):
        value = broad._yaml_load(reader.text(path), path)
        operation(value)
        return {path: yaml.safe_dump(value, sort_keys=False)}

    def changed_row(path, key, identity, operation):
        return changed(path, lambda value: operation(next(row for row in value['Body'] if row.get(key) == identity)))

    item = 'db/re/item_db_etc.yml'
    rep = 'db/import/reputation.yml'
    const = 'db/import/const.yml'
    reject('document weight changed', changed_row(item, 'Id', DOCUMENT, lambda row: row.update(Weight=10)))
    reject('document callback added', changed_row(item, 'Id', DOCUMENT, lambda row: row.update(Script='RepPoints6 = 5000;')))
    reject('document trade policy changed', changed_row(item, 'Id', DOCUMENT, lambda row: row['Trade'].update(NoTrade=False)))
    reject('document identity removed', changed(item, lambda value: value.update(Body=[row for row in value['Body'] if row['Id'] != DOCUMENT])))
    for key, value in (('Variable', 'RepPoints9'), ('Minimum', -5001), ('Maximum', 5001)):
        reject('reputation ' + key + ' changed', changed_row(rep, 'Id', REPUTATION, lambda row, key=key, value=value: row.update({key: value})))
    reject('reputation bound wrong type', changed_row(rep, 'Id', REPUTATION, lambda row: row.update(Maximum='5000')))
    reject('reputation unknown record schema', changed_row(rep, 'Id', REPUTATION, lambda row: row.update(Unknown=True)))
    reject('constant changed', changed_row(const, 'Name', CONSTANT, lambda row: row.update(Value=9)))
    reject('constant became parameter', changed_row(const, 'Name', CONSTANT, lambda row: row.update(Parameter=True)))
    reject('duplicate constant binding', changed(const, lambda value: value['Body'].append({'Name': CONSTANT, 'Value': REPUTATION})))
    reject('missing constant import', {const: None})
    reject('missing reputation import', {rep: None})
    root_rep = 'db/reputation.yml'
    reject('reputation import disabled by mode', {
        root_rep: reader.text(root_rep).replace('Mode: Renewal', 'Mode: Prerenewal', 1)})
    reject('reputation selected import added', {
        root_rep: reader.text(root_rep) + '  - Path: db/import/document_negative.yml\n',
        'db/import/document_negative.yml': 'Header:\n  Type: REPUTATION_DB\n  Version: 1\nBody:\n  - Id: 6\n    Maximum: 5001\n'})
    reject('reputation import cycle', {root_rep: reader.text(root_rep) + '  - Path: db/reputation.yml\n'})
    reject('constant import escapes root', {'db/const.yml': reader.text('db/const.yml') + '  - Path: ../outside.yml\n'})
    reject('unknown import schema', changed(root_rep, lambda value: value['Footer']['Imports'][0].update(Unknown=True)))
    reject('generator flag wrong type', changed('db/re/reputation.yml', lambda value: value['Footer']['Imports'][0].update(Generator='true')))
    reject('skipped generator edge changed', changed('db/re/reputation.yml', lambda value: value['Footer']['Imports'][0].update(Generator=False)))
    reject('unknown header schema', changed(root_rep, lambda value: value['Header'].update(Unreviewed=1)))
    pc = 'src/map/pc.cpp'
    needle = '\t\tsd->vars_dirty = true;'
    if needle not in reader.text(pc):
        raise AssertionError('Native registry mutation anchor missing')
    reject('native registry dirty flag removed', {pc: reader.text(pc).replace(needle, '\t\t/* injected removal */', 1)})
    packet = 'src/map/clif.cpp'
    needle = '\tentry.points = points;'
    if needle not in reader.text(packet):
        raise AssertionError('Native reputation packet anchor missing')
    reject('native reputation packet changed', {packet: reader.text(packet).replace(needle, '\tentry.points = 0;', 1)})
    builtin = 'src/map/script.cpp'
    needle = 'BUILDIN_FUNC(add_reputation_points)'
    reject('native reputation builtin registration renamed', {builtin: reader.text(builtin).replace(needle, needle + '_unreviewed', 1)})
    reject('native import semantics changed', {
        'src/common/database.hpp': reader.text('src/common/database.hpp').replace('shouldLoadGenerator{false}', 'shouldLoadGenerator{true}', 1)})
    body = script_body(reader.text(NPC), baseline['service']['declaration'])
    modified = body.replace('add_reputation_points REPUTATION_BIOSPHERE_DEPTH1,.@gain;', 'add_reputation_points REPUTATION_BIOSPHERE_DEPTH2,.@gain;', 1)
    if modified == body:
        raise AssertionError('Document service mutation anchor missing')
    reject('document service reward changed', {NPC: reader.text(NPC).replace(body, modified, 1)})
    qi = baseline['material']['questinfo']['registrations'][0]
    reject('inherited QuestInfo condition mutates reputation', {
        qi['path']: reader.text(qi['path']).replace(qi['statement'], qi['statement'].replace(qi['condition'], '(RepPoints6 = 5000)'), 1)})
    for label, operation in (
            ('tampered collected reputation', lambda value: value['reputation']['selected_record'].update(Maximum=5001)),
            ('unexpected evidence schema', lambda value: value.update(Unreviewed=True)),
            ('false live registry attestation', lambda value: value.update(live_registry_verified=True))):
        value = copy.deepcopy(baseline)
        operation(value)
        try:
            _check(value)
        except ValueError:
            results.append(label)
        else:
            raise AssertionError('Tampered evidence accepted: ' + label)

    # These are reread-policy controls, not acceptance via injected snapshots.
    for path in (rep, const, 'db/re/achievement_db.yml', 'db/re/quest_db.yml'):
        try:
            verify_additional_sources(broad.Reader(root, {path: reader.text(path) + '\n# source drift\n'}), baseline, baseline['material'])
        except ValueError as exc:
            if 'changed during validation: ' + path not in str(exc):
                raise
            results.append('late dependency source drift: ' + path)
        else:
            raise AssertionError('Late dependency drift accepted: ' + path)

    # Deliberately failing prerequisites must prevent entering later stages.
    # Public positive validation above/below remains real and unconditional.
    from unittest.mock import patch
    for requested in (None, 'live-20260906'):
        with patch.object(broad, 'validate', side_effect=ValueError('required broad failure')) as broad_mock:
            with patch.object(material_gate, 'validate', side_effect=AssertionError('material entered before broad acceptance')):
                try:
                    validate(root, profile=requested)
                except ValueError as exc:
                    if str(exc) != 'required broad failure':
                        raise
                else:
                    raise AssertionError('Broad rejection bypassed')
            broad_mock.assert_called_once_with(root, profile=requested)
        results.append(f'mandatory broad gate/profile forwarding: {requested}')
        with patch.object(broad, 'validate', return_value={}) as broad_mock:
            with patch.object(material_gate, 'validate', side_effect=ValueError('required material failure')) as material_mock:
                with patch.object(sys.modules[__name__], '_collect', side_effect=AssertionError('scope entered before material acceptance')):
                    try:
                        validate(root, profile=requested)
                    except ValueError as exc:
                        if str(exc) != 'required material failure':
                            raise
                    else:
                        raise AssertionError('Material rejection bypassed')
                broad_mock.assert_called_once_with(root, profile=requested)
                material_mock.assert_called_once_with(root, profile=requested)
        results.append(f'mandatory material gate/profile forwarding: {requested}')
    try:
        validate(root, profile='unreviewed-profile')
    except ValueError as exc:
        if 'Unknown callback closure profile' not in str(exc):
            raise
        results.append('unknown profile rejected')
    else:
        raise AssertionError('Unknown profile accepted')
    if validate(root, profile=profile) != accepted:
        raise AssertionError('Source changed during negative controls')
    return results


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=ROOT)
    parser.add_argument('--profile', choices=sorted(broad.PROFILES))
    parser.add_argument('--collect-only', action='store_true')
    parser.add_argument('--manifest', action='store_true')
    parser.add_argument('--negative-controls', action='store_true')
    args = parser.parse_args()
    if args.collect_only and (args.profile or args.negative_controls):
        parser.error('--collect-only cannot claim a profile or run acceptance controls')
    try:
        result = collect_evidence(args.root) if args.collect_only else validate(args.root, profile=args.profile)
        if args.collect_only:
            print('UNVALIDATED COLLECTION: broad/material/document gates NOT accepted.', file=sys.stderr)
        if args.negative_controls:
            print(json.dumps({'negative_controls_rejected': negative_controls(args.root, profile=args.profile),
                              'pins': PINS, 'profile': args.profile}, sort_keys=True, indent=2))
        elif args.collect_only or args.manifest:
            print(json.dumps(result, sort_keys=True, indent=2))
        else:
            print('PASS: both mandatory prerequisite gates and exact document/reputation dependencies unchanged')
            print(json.dumps({'pins': PINS, 'profile': args.profile, 'material_manifest_sha256': result['material_manifest_sha256']}, sort_keys=True))
    except (ValueError, OSError, AssertionError) as exc:
        print(f'FAIL: {exc}', file=sys.stderr)
        return 1
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
