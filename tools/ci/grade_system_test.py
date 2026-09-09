#!/usr/bin/env python3
# ============================================================================
#  PN  /  DEVELOPMENT TOOLS
#  grade_system_test.py
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/grade_system_test.py
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

"""Actual grading handlers, compiled with explicit packet/inventory/DB doubles.

Also checks effective grading settings and reagent references. No production
server or SQL/network connection is started. Requires g++, PyYAML.
"""
from pathlib import Path
import re
import subprocess
import tempfile
import yaml
from audit_enchant_upgrades import renewal_records
from episode_party_progression_test import scan_to

ROOT=Path(__file__).resolve().parents[2]
source=(ROOT/'src/map/clif.cpp').read_text()
def extract(name):
    begin=source.index(name+'(')
    begin=source.rfind('\n',0,begin)+1
    brace=source.index('{',begin)
    return source[begin:scan_to(source,brace,'{','}')+1]
helper='\n'.join(extract(name) for name in ('clif_enchantgrade_target_valid','clif_parse_enchantgrade_add','clif_parse_enchantgrade_start'))

records={}
for row in renewal_records(ROOT,'db/item_db.yml'):
    records.setdefault(row['Id'],{}).update(row)
items={row['AegisName']:row for row in records.values() if 'AegisName' in row}
types=yaml.safe_load((ROOT/'db/re/enchantgrade.yml').read_text())['Body']
override=yaml.safe_load((ROOT/'db/import/enchantgrade.yml').read_text())
assert not override.get('Body'),'Grading overrides need an explicit audit merge'
grades=options=0
for kind in types:
    assert kind['Type'] in ('Armor','Weapon')
    for level in kind['Levels']:
        assert (kind['Type'],level['Level']) in (('Armor',2),('Weapon',5))
        assert [g['Grade'] for g in level['Grades']]==['None','D','C','B']
        for grade in level['Grades']:
            grades+=1
            catalyst=grade.get('Catalyst',{})
            assert catalyst['Item'] in items
            assert 0<catalyst['AmountPerStep']*catalyst['MaximumSteps']<=30000
            assert 0<=catalyst['ChanceIncrease']<=10000
            for chance in grade['Chances']:
                assert 0<=chance['Refine']<=20 and 0<=chance['Chance']<=10000
            for option in grade['Options']:
                options+=1;assert option['Item'] in items
                assert 0<option.get('Amount',1)<=30000
                assert 0<=option.get('Zeny',0)<=2147483647
                assert 0<=option.get('BreakingRate',0)<=10000
                assert 0<=option.get('DowngradeAmount',0)<=20
                item=items[option['Item']]
                assert item.get('Type') in ('Etc','Usable')
                if option['Item']==catalyst['Item']:
                    assert option.get('Amount',1)+catalyst['AmountPerStep']*catalyst['MaximumSteps']<=30000
print(f'GRADE_DB_OK: {grades} grade stages / {options} cost options, all reagent IDs resolved')
fixture=(ROOT/'tools/ci/grade_system_test.cpp').read_text()
with tempfile.TemporaryDirectory(prefix='pn-grade-') as temp:
    cpp,exe=Path(temp)/'test.cpp',Path(temp)/'test'
    cpp.write_text(fixture.replace('// PRODUCTION_HANDLERS',helper))
    subprocess.run(['g++','-std=c++17','-Wall','-Wextra','-Werror','-Wno-sign-compare','-fsanitize=address,undefined','-fno-sanitize-recover=all',str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
