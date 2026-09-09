#!/usr/bin/env python3
# ============================================================================
#  PN  /  DEVELOPMENT TOOLS
#  sealed_performer_cards_test.py
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/sealed_performer_cards_test.py
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

"""Execute narrowly translated card expressions, not a full script-VM/combat test."""
from pathlib import Path
import re
import subprocess
import tempfile
from audit_enchant_upgrades import renewal_records
ROOT=Path(__file__).resolve().parents[2]
items={}
for row in renewal_records(ROOT,'db/item_db.yml'):items.setdefault(row['Id'],{}).update(row)
skills={r['Name']:r for r in renewal_records(ROOT,'db/skill_db.yml') if 'Name' in r}
cpp='''#include <cassert>
#include <string>
int refinement,weapon,vit,granted,flee;
std::string granted_name;
enum {EQI_HAND_R,ITEMINFO_VIEW,W_MUSICAL,W_WHIP,W_STAFF,bVit};
int getrefine(){return refinement;} int getequipid(int){return 0;}
int getiteminfo(int,int){return weapon;} int readparam(int){return vit;}
void grant(const char* name,int level){granted_name=name;granted=level;}
'''
for id,normal,name in [(27213,4560,'BA_POEMBRAGI'),(27219,4566,'DC_FORTUNEKISS')]:
    script=items[id]['Script']
    assert name in items[normal]['Script']
    assert skills[name]['MaxLevel']>=7
    assert skills[name]['Requires']['Weapon']=={'Musical':True,'Whip':True}
    assert not re.search(r'(BA_POEMBRAGI2|DC_FORTUNEKISS2)',script)
    script=script.replace('.@a','a').replace('.@b','b')
    script=re.sub(r'skill "(\w+)",([^;]+);',r'grant("\1",\2);',script)
    script=re.sub(r'bonus bFlee,([^;]+);',r'flee=(\1);',script)
    cpp+=f'void card_{id}(){{int a=0,b=0;\n{script}\n}}\n'
cpp+='int main(){\n'
for id,name in [(27213,'BA_POEMBRAGI'),(27219,'DC_FORTUNEKISS')]:
    for ref in (0,14,15,20):
        for vit in (109,110):
            for weapon in ('W_MUSICAL','W_WHIP','W_STAFF'):
                level=(7 if ref>=15 else 5) if weapon!='W_STAFF' else 0
                flee=(15 if ref>=15 else 10)*(2 if vit>=110 else 1)
                cpp+=f'refinement={ref};vit={vit};weapon={weapon};granted=0;granted_name="";flee=0;card_{id}();assert(granted=={level});assert(flee=={flee});'
                if level:cpp+=f'assert(granted_name=="{name}");'
                cpp+='\n'
cpp+='}\n'
with tempfile.TemporaryDirectory(prefix='sealed-cards-') as directory:
    p=Path(directory)
    (p/'test.cpp').write_text(cpp)
    subprocess.run(['g++','-std=c++17',str(p/'test.cpp'),'-o',str(p/'test')],check=True)
    subprocess.run([str(p/'test')],check=True)
print('PASS: 48 translated-script cases: refine 0/14/15/20, VIT 109/110, instrument/whip/staff; supported skill grants and unchanged FLEE')
