#!/usr/bin/env python3
# ============================================================================
#  PN  /  DEVELOPMENT TOOLS
#  druid_missing_targets_test.py
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/druid_missing_targets_test.py
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

"""Target-only overlay preservation and original-client evidence.

This is an effective-data comparison, not native packet charging or gameplay.
Without --require-import, only the candidate overlay is injected in memory.
The two candidate item fragments are read directly to check IDs/slot viability.
With --require-import, all items and targets must resolve through actual roots.
"""
import argparse
import copy
import hashlib
import json
from pathlib import Path
from unittest.mock import patch

import yaml
from audit_enchant_upgrades import renewal_records, server_recipes
from audit_initial_enchants import client_configuration, server_configuration
from lua51_literal_table import literal_tables

ROOT = Path(__file__).resolve().parents[2]
OVERLAY = 'db/import/druid_missing_targets.yml'
TARGETS = {
    24: {'Solid_Whinger': 510189},
    31: {'Glacier_N_Knife': 510190, 'Glacier_N_Axe': 620056},
    33: {'Repeat_Dagger_AD': 510185},
    47: {'D_Glacier_N_Knife': 510191, 'D_Glacier_N_Axe': 620057},
    63: {'F_Ein_AXE': 520047}, 64: {'Mocadas_Garz': 590104},
    132: {'Time_DM_R_Crown_AT': 400999},
    133: {'Dimen_AT_Knife': 510193, 'Dimen_AT_Axe': 620059},
    147: {'Axe_Furious': 520052, 'Hall_Furious': 590117},
    148: {'FuriousCirclet_AT': 401176},
    164: {'Time_DM_R_Crown_AT': 400999},
    165: {'Sky_Rune_Crown_SHC': 401171, 'Sky_Rune_Crown_AG': 401172,
          'Sky_Rune_Crown_BO': 401173, 'Sky_Rune_Crown_TR': 401174,
          'Sky_Rune_Crown_AT': 401175, 'Sky_Rune_Crown_DK': 401216,
          'Sky_Rune_Crown_EM': 401217, 'Sky_Rune_Crown_SS': 401218,
          'Sky_Rune_Crown_NW': 401219, 'Sky_Rune_Crown_SOA': 401220},
}
UNRESOLVED = {'NP_B_Dagger': 510200, 'SC_B_Axe': 620064,
              'Frontier_R_Crown_AT': 401195}


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def load(path):
    return yaml.load(path.read_text(encoding='utf-8'),
                     Loader=getattr(yaml, 'CSafeLoader', yaml.SafeLoader))


def configuration(include):
    original = Path.read_text

    def read(path, *args, **kwargs):
        text = original(path, *args, **kwargs)
        if path.resolve() == (ROOT / 'db/item_enchant.yml').resolve():
            data = yaml.safe_load(text)
            imports = data['Footer']['Imports']
            imports[:] = [entry for entry in imports if entry['Path'] != OVERLAY]
            if include:
                imports.append({'Path': OVERLAY, 'Mode': 'Renewal'})
            return yaml.safe_dump(data)
        return text

    with patch.object(Path, 'read_text', read):
        return server_configuration(ROOT), server_recipes(ROOT)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--require-import', action='store_true')
    parser.add_argument('--client', type=Path, required=True)
    parser.add_argument('--client-item-names', type=Path, required=True)
    args = parser.parse_args()
    for path, digest in (
        (args.client, '664a6083b051233496611664af4c84d4ccd1c9e6ceb685edac6cacf60feda80d'),
        (args.client_item_names, '2f4f35e157d25548f236dbe7785a3138f4c7ab23367becf9e1f95b958cc3c496'),
    ):
        require(hashlib.sha256(path.read_bytes()).hexdigest() == digest,
                'Original reference changed: ' + str(path))
    document = load(ROOT / OVERLAY)
    expected = {'Header': {'Type': 'ITEM_ENCHANT_DB', 'Version': 1},
                'Body': [{'Id': group, 'TargetItems': dict.fromkeys(names, True)}
                         for group, names in TARGETS.items()]}
    require(document == expected, 'Overlay is not the exact target-only allowlist')
    before, before_upgrades = configuration(False)
    after, after_upgrades = configuration(True)
    require(before_upgrades == after_upgrades, 'An upgrade recipe changed')
    expected_after = copy.deepcopy(before)
    for group, names in TARGETS.items():
        require(group in before, 'New group invented')
        require(before[group]['Targets'].isdisjoint(names), 'Target was not missing')
        expected_after[group]['Targets'].update(names)
    require(after == expected_after, 'Old group, target, price, recipe or reset changed')

    items = {}
    for item in renewal_records(ROOT, 'db/item_db.yml'):
        items.setdefault(item['Id'], {}).update(item)
    if not args.require_import:
        for fragment in ('druid_missing_weapons.yml', 'druid_missing_crowns.yml'):
            for item in load(ROOT / 'db/import' / fragment)['Body']:
                items.setdefault(item['Id'], {}).update(item)
    else:
        imports = load(ROOT / 'db/item_enchant.yml')['Footer']['Imports']
        require([entry for entry in imports if entry['Path'] == OVERLAY] ==
                [{'Path': OVERLAY, 'Mode': 'Renewal'}], 'Missing/duplicate/wrong-mode import')
        require(server_configuration(ROOT) == after, 'Actual target import differs')
    names = literal_tables(args.client_item_names.read_bytes())['ItemDBNameTbl']
    client = client_configuration(args.client, lambda name: name)
    for group, entries in TARGETS.items():
        for name, number in entries.items():
            require(names[name] == number, 'Original numeric identity mismatch: ' + name)
            require(name in client[group]['Targets'], 'Original group lacks target: ' + name)
            require(number in items and items[number]['AegisName'] == name,
                    'Actual item identity unresolved: ' + name)
            for slot in after[group]['Order']:
                require(slot >= items[number].get('Slots', 0),
                        'Enchant order collides with card slots: ' + name)
    for name, number in UNRESOLVED.items():
        require(names[name] == number and number not in items,
                'Unresolved identity disposition changed: ' + name)
        require(all(name not in group['Targets'] for group in after.values()),
                'Unsupported target was enabled: ' + name)
    print(json.dumps({'result': 'PASS', 'mode': 'actual-imports' if args.require_import else 'candidate',
        'unique_items': 24, 'target_bindings': 25, 'existing_groups_extended': 12,
        'all_effective_groups_preserved': len(before),
        'all_ordinary_upgrades_preserved': len(before_upgrades[1]),
        'all_perfect_upgrades_preserved': len(before_upgrades[2]),
        'unresolved_items': UNRESOLVED,
        'scope': 'Effective data and original references, not native packet execution'}, indent=2))


if __name__ == '__main__':
    main()
