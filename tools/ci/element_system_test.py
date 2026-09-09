#!/usr/bin/env python3
# ============================================================================
#  PN  /  DEVELOPMENT TOOLS
#  element_system_test.py
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/element_system_test.py
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

"""Validate effective Renewal attributes and execute unchanged battle/status helpers.

Requires PyYAML and g++. World, status storage and skill transport are explicit
test doubles; this does not exercise full weapon/cardfix/skill pipelines.
"""
from pathlib import Path
import re
import subprocess
import tempfile
import yaml

ROOT = Path(__file__).resolve().parents[2]
ELEMENTS = 'Neutral Water Earth Fire Wind Poison Holy Dark Ghost Undead'.split()

def function(source, signature):
    start = source.index(signature)
    brace = source.index('{', start)
    depth = 1
    end = brace + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]

def load(path, matrix, visited):
    assert path not in visited, f'Recursive/duplicate attribute import: {path}'
    visited.add(path)
    if not path.exists():
        assert path == ROOT / 'db/import/attr_fix.yml', path
        return
    doc = yaml.safe_load(path.read_text())
    assert doc['Header'] == {'Type': 'ATTRIBUTE_DB', 'Version': 1}
    levels = set()
    for row in doc.get('Body', []) or []:
        level = row['Level']
        assert level in range(1, 5) and level not in levels, (path, level)
        levels.add(level)
        assert set(row) <= set(ELEMENTS) | {'Level'}
        for attack, defenses in row.items():
            if attack == 'Level':
                continue
            assert set(defenses) <= set(ELEMENTS)
            for defense, ratio in defenses.items():
                assert type(ratio) is int and -100 <= ratio <= 200
                matrix[level-1][ELEMENTS.index(attack)][ELEMENTS.index(defense)] = ratio
    for entry in doc.get('Footer', {}).get('Imports', []):
        if entry.get('Mode', 'Renewal') == 'Renewal':
            load(ROOT / entry['Path'], matrix, visited)

matrix = [[[100] * 11 for _ in range(11)] for _ in range(4)]
base = yaml.safe_load((ROOT / 'db/re/attr_fix.yml').read_text())['Body']
assert len(base) == 4
for row in base:
    assert set(row) == set(ELEMENTS) | {'Level'}
    assert all(set(row[element]) == set(ELEMENTS) for element in ELEMENTS)
load(ROOT / 'db/attr_fix.yml', matrix, set())
# Explicit directional/level sentinels catch transposition and accidental legacy tables.
assert [m[0][8] for m in matrix] == [90, 70, 50, 0]
assert [m[1][3] for m in matrix] == [150, 175, 200, 200]
assert [m[3][1] for m in matrix] == [90, 80, 70, 60]
assert [m[3][3] for m in matrix] == [25, 0, 0, 0]
assert [m[6][9] for m in matrix] == [125, 150, 175, 200]
battle = (ROOT / 'src/map/battle.cpp').read_text()
status = (ROOT / 'src/map/status.cpp').read_text()
production = function(battle, 'int64 battle_attr_fix(')
production += '\n' + function(status, 'int16 AttributeDatabase::getAttribute(')
production += '\n' + function(status, 'static unsigned char status_calc_element(block_list *bl, status_change *sc, int32 element)\n{')
production += '\n' + function(status, 'static unsigned char status_calc_element_lv(block_list *bl, status_change *sc, int32 lv)\n{')
symbols = sorted(set(re.findall(r'\bSC_[A-Z_]+\b', production)))
fixture = (ROOT / 'tools/ci/element_system_test.cpp').read_text()
fixture = fixture.replace('// STATUS_ENUM', 'enum { ' + ','.join(symbols) + ' };')
fixture = fixture.replace('// TABLE_DATA', 'int16 attr_fix_table[4][11][11] = ' + str(matrix).replace('[', '{').replace(']', '}') + ';')
fixture = fixture.replace('// PRODUCTION_FUNCTIONS', production)
with tempfile.TemporaryDirectory(prefix='pn-element-test-') as temp:
    cpp, exe = Path(temp) / 'test.cpp', Path(temp) / 'test'
    cpp.write_text(fixture)
    subprocess.run(['g++', '-std=c++17', '-Wall', '-Wextra', '-Werror', '-Wno-unused-parameter', '-fsanitize=address,undefined', '-fno-sanitize-recover=all', str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
