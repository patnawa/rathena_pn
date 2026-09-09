#!/usr/bin/env python3
# ============================================================================
#  PN  /  DEVELOPMENT TOOLS
#  player_command_permissions_test.py
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/player_command_permissions_test.py
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

"""Check the player command allowlist against native registrations and aliases.

This checks checked-in defaults, not deployment-specific import overrides or
runtime permissions. Script bindings are validated in their own native VM tests.
"""
from pathlib import Path
import re
import yaml

ROOT = Path(__file__).resolve().parents[2]

class UniqueLoader(yaml.SafeLoader):
    pass

def unique_mapping(loader, node, deep=False):
    pairs = loader.construct_pairs(node, deep=deep)
    result = {}
    for key, value in pairs:
        assert key not in result, f'Duplicate YAML key: {key}'
        result[key] = value
    return result

UniqueLoader.add_constructor(yaml.resolver.BaseResolver.DEFAULT_MAPPING_TAG, unique_mapping)
groups = yaml.load((ROOT / 'conf/groups.yml').read_text(), Loader=UniqueLoader)
commands = yaml.load((ROOT / 'conf/atcommands.yml').read_text(), Loader=UniqueLoader)
player = next(row for row in groups['Body'] if row['Id'] == 0)
assert player.get('Level', 0) == 0 and not player.get('Inherit')
assert not any(player.get('CharCommands', {}).values())
assert {key for key, value in player.get('Permissions', {}).items() if value} <= {'can_trade', 'can_party', 'attendance'}
source = (ROOT / 'src/map/atcommand.cpp').read_text()
native = set(re.findall(r'ACMD_DEF(?:R)?\(\s*(\w+)', source))
native.update(re.findall(r'ACMD_DEF2\("([^"]+)"', source))
aliases = {}
for row in commands['Body']:
    for alias in row.get('Aliases', []):
        assert alias not in aliases or aliases[alias] == row['Command'], alias
        aliases[alias] = row['Command']
resolved = [aliases.get(key, key) for key in player['Commands']]
assert len(resolved) == len(set(resolved)), 'Duplicate canonical command/alias grants'
enabled = {aliases.get(key, key) for key, value in player['Commands'].items() if value}
assert enabled <= native, f'Unregistered player commands: {enabled-native}'
# These existing native handlers have administrative or materially different semantics.
for forbidden in 'item item2 zeny alive monster killmonster allowks guild invite storeall stockall dropall storage adjgroup addperm setbattleflag reloadscript warp jumpto'.split():
    assert forbidden not in enabled, f'Unsafe/unreviewed native command exposed: {forbidden}'
for alias, target in {'al':'autoloot', 'ii':'iteminfo', 'mi':'mobinfo', 'wd':'whodrops', 'wi':'whereis', 'time':'servertime', 'return':'load', 'noks':'ksprotection'}.items():
    assert aliases[alias] == target and target in enabled
print(f'PASS: {len(enabled)} registered player commands, exact aliases, no GM permissions or charcommands')
