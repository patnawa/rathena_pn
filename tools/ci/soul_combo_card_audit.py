#!/usr/bin/env python3
# ============================================================================
#  PN  /  DEVELOPMENT TOOLS
#  soul_combo_card_audit.py
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/soul_combo_card_audit.py
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

"""Read-only database reference audit and native on-skill probability regression.

This is not a combat simulator or proof of every card's gameplay semantics.
"""
from pathlib import Path
import re
import subprocess
import tempfile
from audit_enchant_upgrades import renewal_records

ROOT=Path(__file__).resolve().parents[2]

def main():
    items={}
    for row in renewal_records(ROOT,'db/item_db.yml'):
        items.setdefault(row['Id'],{}).update(row)
    names={r['AegisName'] for r in items.values()}
    skills={r['Name'] for r in renewal_records(ROOT,'db/skill_db.yml') if 'Name' in r}
    constants=(ROOT/'src/map/script_constants.hpp').read_text()
    bonuses=set(re.findall(r'export_constant2\("(b\w+)"\s*,',constants))
    assert 'bMatk' in bonuses and 'bSkillAtk' in bonuses
    bonuses={b.lower() for b in bonuses}
    cards=[r for r in items.values() if r.get('Type')=='Card']
    failures=[]
    scripts=[]
    for row in cards:
        for key in ('Script','EquipScript','UnEquipScript'):
            if row.get(key):scripts.append((str(row['Id']),row[key]))
    combo_count=0
    soa={}
    for row in renewal_records(ROOT,'db/item_combos.yml'):
        combo_count+=1
        for entry in row.get('Combos',[]):
            for name in entry['Combo']:
                if name not in names:failures.append(f'combo item missing: {name}')
            if 'Time_DM_R_Crown_SOA' in entry['Combo'] or 'Frontier_R_Crown_SOA' in entry['Combo']:
                soa[tuple(entry['Combo'])]=row.get('Script','')
        scripts.append((f'combo {combo_count}',row.get('Script','')))
    for label,script in scripts:
        # Include embedded autobonus scripts, whose quotes are escaped in YAML.
        script=script.replace('\\"','"')
        for bonus in re.findall(r'\bbonus[2-5]?\s+(b\w+)\s*[,;]',script):
            if bonus.lower() not in bonuses:failures.append(f'{label}: unknown bonus {bonus}')
        for skill in re.findall(r'"([A-Z][A-Z0-9]*_[A-Z0-9_]+)"',script):
            # These prefixes are player/monster skill IDs, not constants or labels.
            if skill.split('_')[0] in {s.split('_')[0] for s in skills} and skill not in skills:
                failures.append(f'{label}: unknown skill {skill}')
    dimension=soa[('Dimen_SOA_Stick','Time_DM_R_Crown_SOA')]
    frontier=soa[('Frontier_R_Crown_SOA','Frontier_SOA_Staff')]
    assert 'getskilllv("SOA_TALISMAN_OF_RED_PHOENIX")' in dimension
    assert '"SOA_TALISMAN_OF_BLUE_DRAGON","SOA_TALISMAN_OF_RED_PHOENIX"' in dimension
    assert 'ENCHANTGRADE_A' in dimension
    assert '.@sum>=24' in frontier and '60000,"SOA_CIRCLE_OF_DIRECTIONS_AND_ELEMENTALS"' in frontier
    assert '\\"SOA_TALISMAN_OF_BLUE_DRAGON\\",\\"SOA_TALISMAN_OF_WHITE_TIGER\\",5,1000' in frontier
    assert '\\"SOA_TALISMAN_OF_RED_PHOENIX\\",\\"SOA_TALISMAN_OF_BLACK_TORTOISE\\",5,1000' in frontier
    print(f'Audited {len(cards)} cards/enchant records, {combo_count} combos, {len(scripts)} scripts')
    print('PASS: Soul Ascetic Dimensions/Frontier trigger names, grade/skill gates and duration')
    if failures:
        print('\n'.join(sorted(set(failures))[:50]))
        raise AssertionError(f'{len(set(failures))} reference problems')
    print('PASS: card/combo bonus constants and skill/item references; no exceptions')
    source=(ROOT/'src/map/skill.cpp').read_text()
    body=source.split('int32 skill_onskillusage(',1)[1].split('\n}\n',1)[0]
    checks=re.findall(r'if \((rnd_value\(0, \d+\) >= it->rate)\)',body)
    assert len(checks)==2, 'Review player and pet on-skill probability checks'
    cpp='#include <cassert>\nstruct Bonus{int rate;};\nint draw; int rnd_value(int low,int high){assert(draw>=low && draw<=high);return draw;}\nint main(){Bonus bonus;auto it=&bonus;\n'
    for check in checks:
        upper=int(re.search(r'0, (\d+)',check)[1])
        cpp+=f'for(int rate: {{0,1,500,999,1000}}){{ it->rate=rate; int hits=0; for(draw=0;draw<={upper};draw++){{if(!({check}))hits++;}} assert(hits*1000==rate*({upper}+1));}}\n'
    cpp+='}\n'
    cpp='#include <initializer_list>\n'+cpp
    with tempfile.TemporaryDirectory(prefix='soul-proc-') as directory:
        path=Path(directory)
        (path/'test.cpp').write_text(cpp)
        subprocess.run(['g++','-std=c++17',str(path/'test.cpp'),'-o',str(path/'test')],check=True)
        subprocess.run([str(path/'test')],check=True)
    print('PASS: native production probability predicates at 0%, 0.1%, 50%, 99.9%, 100%')

if __name__=='__main__':main()
