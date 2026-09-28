#!/usr/bin/env python3
"""Execute actual production ratio bodies and GTB dispatch condition in C++.

Compile with g++ (WSL/Linux). Status/skill lookups are explicit boundary doubles;
the arithmetic and dispatch predicate are extracted unchanged from production.
"""
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
SKILLS = {
    'radiantspear': 'SkillRadiantSpear',
    'imperialcross': 'SkillImperialCross',
    'cannonspear': 'SkillCannonSpear',
    'banishingpoint': 'SkillBanishingPoint',
    'psychicstream': 'SkillPsychicStream',
}

def body(source, marker):
    start = source.index('{', source.index(marker))
    depth = 1
    end = start + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]

cpp = r'''
#include <cstdint>
#include <cstdio>
using int32 = int32_t; using uint16 = uint16_t;
struct status_data { int pow=0, spl=0, str=0; } stats;
struct block_list { int immunity=0; } player, enemy;
using map_session_data = block_list;
struct Damage {};
struct status_change { bool scar=false; bool getSCE(int) const { return scar; } } status;
constexpr int BL_PC=1, SC_SPEAR_SCAR=2, IG_SPEAR_SWORD_M=3, SM_BASH=4;
constexpr int LG_RAYOFGENESIS=2321, NPC_RAYOFGENESIS=999, AG_DEADLY_PROJECTION=888, BF_MAGIC=2;
int mastery=0, bash=0, base_level=100;
#define BL_CAST(type, src) (src)
#define RE_LVL_DMOD(divisor) skillratio = skillratio * base_level / (divisor)
const status_data* status_get_status_data(const block_list&) { return &stats; }
const status_change* status_get_sc(const block_list*) { return &status; }
int pc_checkskill(const map_session_data*, int id) { return id == SM_BASH ? bash : mastery; }
int skill_get_type(int) { return BF_MAGIC; }
int status_isimmune(const block_list* bl) { return bl->immunity; }
int failures=0, checks=0;
void expect(int actual,int expected,const char* label) {
    ++checks; if(actual!=expected) { ++failures; std::printf("FAIL %s actual=%d expected=%d\n",label,actual,expected); }
}
'''
for name, cls in SKILLS.items():
    group = 'mage' if name == 'psychicstream' else 'swordman'
    source = (ROOT/f'src/map/skills/{group}/{name}.cpp').read_text()
    cpp += f'int {name}(uint16 skill_lv) {{ int32 skillratio=100; const block_list* src=&player;\n'
    cpp += body(source, cls+'::calculateSkillRatio') + '\nreturn skillratio; }\n'
source = (ROOT/'src/map/skill.cpp').read_text()
start = source.index('\tif (skill_id &&', source.index('int32 skill_castend_damage_id ('))
condition = source[start:source.index('\n\t{',start)].strip()[4:-1]
cpp += 'bool blocked(int skill_id,const block_list* src,const block_list* bl,int flag) { return '+condition+'; }\n'
cpp += r'''
int main() {
    for (int level : {1,5,10}) for(int power : {0,120}) for(bool scar : {false,true}) {
        stats.pow=power; status.scar=scar; base_level=100; mastery=10; bash=10;
        expect(radiantspear(level),3500+1150*level+500+(scar?7:5)*power+(scar?250*level:0),"Radiant Spear");
        if(level<=5) expect(imperialcross(level),1650+1350*level+250+(scar?8:5)*power+(scar?100+300*level:0),"Imperial Cross");
        stats.str=130;
        if(level<=5) expect(cannonspear(level),250*level+(scar?200*level:0),"Cannon Spear");
        expect(banishingpoint(level),100*level+700+(scar?180*level:0),"Banishing Point");
    }
    status.scar=true; stats.pow=120; base_level=285;
    expect(radiantspear(10),(17500+500+840)*285/100,"Radiant base level");
    base_level=100; stats.spl=120;
    expect(psychicstream(5),21000+1440,"Psychic Stream already correct");
    player.immunity=100; enemy.immunity=0;
    expect(blocked(LG_RAYOFGENESIS,&player,&player,0),false,"GTB wearer may start Genesis splash");
    expect(blocked(LG_RAYOFGENESIS,&player,&enemy,1),false,"normal enemy receives Genesis");
    enemy.immunity=100;
    expect(blocked(LG_RAYOFGENESIS,&player,&enemy,1),true,"GTB enemy remains immune");
    expect(blocked(LG_RAYOFGENESIS,&player,&player,1),true,"recursive self hit remains immune");
    expect(blocked(NPC_RAYOFGENESIS,&player,&player,0),true,"NPC Genesis unchanged");
    expect(blocked(AG_DEADLY_PROJECTION,&player,&enemy,1),false,"existing projection exception retained");
    std::printf("MUHRO_SKILL_BALANCE checks=%d failures=%d\n",checks,failures);
    return failures ? 1 : 0;
}
'''
cpp = '#include <initializer_list>\n' + cpp
with tempfile.TemporaryDirectory(prefix='pn-muhro-skills-') as temp:
    path, executable = Path(temp)/'test.cpp', Path(temp)/'test'
    path.write_text(cpp)
    subprocess.run(['g++','-std=c++17','-fsanitize=address,undefined','-fno-sanitize-recover=all',str(path),'-o',str(executable)],check=True)
    subprocess.run([str(executable)],check=True)
