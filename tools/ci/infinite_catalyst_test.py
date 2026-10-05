#!/usr/bin/env python3
"""Execute production rental eligibility, cost override and trap recovery guards."""
from pathlib import Path
import re
import subprocess
import tempfile
import yaml
from episode_party_progression_test import scan_to

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT/'src/map/skill.cpp').read_text(encoding='utf-8')
start = source.index('static bool skill_has_rental_catalyst(')
brace = source.index('{', start)
helper = source[start:scan_to(source, brace, '{', '}')+1]
start = source.index('\t// Rental catalysts waive only')
override = source[start:source.index('\n\treturn req;', start)]
guards = []
for filename in ('src/map/skill.cpp', 'src/map/skills/archer/removetrap.cpp', 'src/map/skills/archer/sensitivekeen.cpp'):
    body = (ROOT/filename).read_text(encoding='utf-8')
    for match in re.finditer(r'if\s*\(\s*!(?:group|sg)->state.rental_trap', body):
        begin = body.index('(', match.start())
        guards.append(body[begin+1:scan_to(body, begin, '(', ')')])
assert len(guards) == 4
cpp = r'''
#include <cstdint>
#include <ctime>
#include <cstdio>
using int32=int32_t; using t_itemid=uint32_t;
constexpr int MAX_INVENTORY=8,MAX_SKILL_ITEM_REQUIRE=4;
constexpr int PN_SOUL_TALISMAN_MATERIAL=1000563,ITEMID_TRAP=1065,ITEMID_TRAP_ALLOY=7940;
constexpr int PN_INFINITE_SOUL_TALISMAN=50151,PN_INFINITE_TRAP=50152,PN_INFINITE_ALLOY_TRAP=50153;
struct item { t_itemid nameid=0;int amount=0;time_t expire_time=0; };
struct map_session_data { struct { struct { item items_inventory[MAX_INVENTORY]; } u; } inventory; };
struct requirement { t_itemid itemid[4]={1000563,1065,7940,715};int amount[4]={3,2,1,4};bool rental_trap=false;int sp=30,ap=5; };
struct block_list { int type=1; } owner;
constexpr int BL_PC=1,UNT_USED_TRAPS=2,UNT_ANKLESNARE=3;
block_list* map_id2bl(int) { return &owner; }
struct group_t { struct { bool rental_trap=false; } state;int src_id=1,item_id=1065,unit_id=1,val2=0; };
struct unit_t { int val1=1,val2=0; };
'''
cpp += helper + '\nrequirement waive(map_session_data* sd, requirement req) {\n' + override + '\nreturn req; }\n'
for n, guard in enumerate(guards):
    cpp += f'bool refund{n}(group_t* group,unit_t* unit) {{ auto* sg=group;block_list* src=nullptr;return {guard}; }}\n'
cpp += r'''
int main() {
    int checks=0,failures=0;
    auto check=[&](bool ok) { ++checks;if(!ok){++failures;std::printf("FAIL %d\n",checks);} };
    time_t now=std::time(nullptr);
    int materials[]={1000563,1065,7940};
    for(int kind=0;kind<3;++kind) for(int state=0;state<6;++state) {
        map_session_data sd{};auto& token=sd.inventory.u.items_inventory[7];
        token.nameid=50151+kind;token.amount=1;token.expire_time=now+2592000;
        if(state==1) token.expire_time=now-1;
        if(state==2) token.expire_time=now;
        if(state==3) token.expire_time=0;
        if(state==4) token.amount=0;
        if(state==5) token.nameid=50150;
        for(int m=0;m<3;++m) check(skill_has_rental_catalyst(sd,materials[m],now)==(state==0 && m==kind));
        auto req=waive(&sd,requirement{});
        for(int m=0;m<3;++m) check((req.amount[m]==0)==(state==0 && m==kind));
        check(req.rental_trap==(state==0 && kind!=0));
        check(req.itemid[3]==715 && req.amount[3]==4 && req.sp==30 && req.ap==5);
    }
    group_t group;unit_t unit;
    for(int rental=0;rental<2;++rental) {
        group.state.rental_trap=rental;
        check(refund0(&group,&unit)==!rental);check(refund1(&group,&unit)==!rental);
        check(refund2(&group,&unit)==!rental);check(refund3(&group,&unit)==!rental);
    }
    // Recovery consults the deployed group's snapshot, never current rentals.
    std::printf("INFINITE_CATALYST checks=%d failures=%d\n",checks,failures);return failures?1:0;
}
'''
box = (ROOT/'npc/custom/infinite_catalysts.txt').read_text(encoding='utf-8')
commit = box[box.index('\tif (countitem(50150)'):box.rindex('\n}')]
commit = re.sub(r'mes\s+[^;]+;', '', commit).replace('.@item', 'selected').replace('close;', 'return;')
commit = re.sub(r'(rentitem|delitem) ([^,;]+),\s*([^;]+);', r'\1(\2,\3);', commit)
cpp2 = r'''
#include <cstdio>
int boxes,permanent,rentals,granted,consumed;bool capacity,grant_ok;
int countitem(int id) { return id==50150?boxes:permanent; }
int rentalcountitem(int) { return rentals; }
bool checkweight(int,int) { return capacity; }
void rentitem(int,int seconds) { if(grant_ok && seconds==2592000){++rentals;++granted;} }
void delitem(int,int n) { boxes-=n;consumed+=n; }
'''
cpp2 += 'void commit() { int selected=50151;\n'+commit+'\n}\n'
cpp2 += r'''
int main() {
    int failures=0;
    for(int scenario=0;scenario<6;++scenario) {
        boxes=1;permanent=rentals=granted=consumed=0;capacity=grant_ok=true;
        if(scenario==1) boxes=0;
        if(scenario==2) permanent=1;
        if(scenario==3) rentals=1;
        if(scenario==4) capacity=false;
        if(scenario==5) grant_ok=false;
        commit();
        bool ok=scenario==0?(granted==1 && consumed==1 && boxes==0):(granted==0 && consumed==0);
        if(!ok) ++failures;
    }
    std::printf("CATALYST_BOX scenarios=6 failures=%d\n",failures);return failures?1:0;
}
'''
with tempfile.TemporaryDirectory(prefix='pn-catalysts-') as temp:
    for name, code in [('mechanics', cpp), ('box', cpp2)]:
        path, exe = Path(temp)/(name+'.cpp'), Path(temp)/name
        path.write_text(code, encoding='utf-8')
        subprocess.run(['g++','-std=c++17','-fsanitize=address,undefined','-fno-sanitize-recover=all',str(path),'-o',str(exe)],check=True)
        subprocess.run([str(exe)],check=True)
data = yaml.safe_load((ROOT/'db/import/infinite_catalysts.yml').read_text(encoding='utf-8'))['Body']
assert [i['Id'] for i in data] == [50150,50151,50152,50153]
assert data[0]['Flags']['NoConsume'] is True
for token in data[1:]:
    assert all(token['Trade'][key] for key in ('NoDrop','NoTrade','NoSell','NoCart','NoStorage','NoGuildStorage','NoMail','NoAuction'))
assert 'group->state.rental_trap = rental_trap;' in source
assert '!skill_has_rental_catalyst(*sd, req.itemid[i]) &&' in source
assert not re.search(r'next;|sleep|select\(',box[box.index('\trentitem'):])
print('CATALYST_CONFIG native expiry, restrictions, new-slot checks and no-yield commit verified')
