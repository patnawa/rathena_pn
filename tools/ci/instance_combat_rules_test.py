#!/usr/bin/env python3
"""Compile actual map-rule helpers and delivery guards; no full combat-engine simulation.

Requires g++. Exercises both Renewal definitions, all damage channels, delayed
phase changes and cap boundaries. The surrounding world/HP delivery is a double.
"""
import argparse
from pathlib import Path
import re
import subprocess
import tempfile
from mob_matk_range_test import block
ROOT=Path(__file__).resolve().parents[2]

def fixture(negative=False):
 s=(ROOT/'src/map/battle.cpp').read_text()
 helpers='\n'.join(block(s,signature) for signature in ('static int32 battle_capped_resistance(', 'static bool battle_mode_blocks_damage(', 'static bool battle_strict_damage_blocked('))
 delivery=block(s,'int32 battle_damage(')
 guard=delivery[delivery.index('if (battle_strict_damage_blocked'):delivery.index('\n\tif (target == nullptr)')]
 calc=block(s,'struct Damage battle_calc_attack(')
 calcguard=block(calc,'if (battle_strict_damage_blocked')
 cardfix=block(s,'int32 battle_calc_cardfix(')
 assert cardfix.count('100 - cap_resistance(')==29
 assert 'const int32 resistance_cap = tsd ? map_getmapflag(target->m, MF_RESISTANCECAP) : 0;' in cardfix
 # Every defensive subtraction is capped except explicit status reductions.
 for expression in re.findall(r'cardfix = cardfix \* \(100 - (.*?)\) / 100;',cardfix):
  assert expression.startswith('cap_resistance(') or 'tsc->getSCE(' in expression,expression
 constants=[]
 for path,prefix in [('src/common/mmo.hpp','MD_IGNORE'),('src/map/battle.hpp','BF_')]:
  text=(ROOT/path).read_text()
  for name,value in re.findall(r'\b('+prefix+r'\w+)\s*=\s*(0x[0-9a-fA-F]+)',text):constants.append(f'constexpr int {name}={value};')
 if negative: helpers=helpers.replace('std::min(resistance, cap)','resistance').replace('&& map_getmapflag(target->m, MF_STRICTDAMAGE) > 0','&& false')
 return r'''#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <initializer_list>
using int32=int32_t;using uint32=uint32_t;
constexpr int BL_MOB=1,BL_PC=2,MF_STRICTDAMAGE=1,ATK_BLOCK=0;
struct Status { uint32 mode=0; };
struct block_list { int type=BL_MOB,m=0; Status status; struct {bool immune_attack=false;} ud; };
using mob_data=block_list;
bool enabled=false;
int map_getmapflag(int,int){return enabled;}
const Status* status_get_status_data(const block_list& b){return &b.status;}
'''+ '\n'.join(constants)+'\n'+helpers+r'''
int delivered=0;
int commit(block_list* target,int attack_type){
'''+guard+r'''
 ++delivered;return 1;
}
struct Damage {int flag;long long damage=100,damage2=17;int dmg_lv=1;};
Damage calculate(block_list* target,int flag){ Damage d{flag};
'''+calcguard+r'''
 return d;
}
int checks=0,failures=0;
void check(bool v){++checks;if(!v)++failures;}
int main(){
 for(int cap:{0,1,50,100}) for(int resist:{-100,-1,0,1,49,50,51,99,100,150})
  check(battle_capped_resistance(resist,cap)==(cap && resist>cap?cap:resist));
 // These are separate categories: fifty percent race and size leaves25%, not50%.
 check((100-battle_capped_resistance(90,50))*(100-battle_capped_resistance(80,50))==2500);
 const int channels[]={BF_WEAPON|BF_SHORT,BF_WEAPON|BF_LONG,BF_MAGIC|BF_LONG,BF_MISC|BF_LONG};
 const uint32 modes[]={MD_IGNOREMELEE,MD_IGNORERANGED,MD_IGNOREMAGIC,MD_IGNOREMISC};
 block_list mob;
 for(int mask=0;mask<16;++mask){mob.status.mode=0;for(int n=0;n<4;++n)if(mask&(1<<n))mob.status.mode|=modes[n];
  for(int c=0;c<4;++c)for(int kind:{BF_SKILL,BF_NORMAL})for(bool flag:{false,true}){
   enabled=flag;bool blocked=flag&&(mask&(1<<c));int channel=channels[c]|kind;
   check(battle_strict_damage_blocked(&mob,channel)==blocked);
   auto d=calculate(&mob,channel);check((d.damage==0&&d.damage2==0)==blocked);
   int before=delivered;commit(&mob,channel);check(delivered==before+(blocked?0:1));
  }
 }
 enabled=true;mob.status.mode=MD_IGNOREMAGIC;auto old=calculate(&mob,BF_WEAPON|BF_SHORT);check(old.damage>0);
 mob.status.mode=MD_IGNOREMELEE;int before=delivered;commit(&mob,BF_WEAPON|BF_SHORT);check(before==delivered);
 mob.status.mode=0;mob.ud.immune_attack=true;before=delivered;commit(&mob,BF_MAGIC|BF_LONG);check(before==delivered);
 enabled=false;commit(&mob,BF_MAGIC|BF_LONG);check(delivered==before+1);enabled=true;
 mob.type=BL_PC;check(!battle_strict_damage_blocked(&mob,BF_WEAPON|BF_SHORT));
 check(!battle_strict_damage_blocked(nullptr,BF_MAGIC));
 printf("INSTANCE_COMBAT_RULES checks=%d failures=%d\n",checks,failures);return failures?1:0;
}
'''

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--build-dir',type=Path);p.add_argument('--prepare-only',action='store_true');p.add_argument('--negative',action='store_true');a=p.parse_args()
 def run(out):
  out.mkdir(parents=True,exist_ok=True);src=out/'instance_combat_rules.cpp';src.write_text(fixture(a.negative))
  if a.prepare_only:return
  for renewal in (False,True):
   exe=out/('rules-re' if renewal else 'rules-pre');cmd=['g++','-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=undefined','-fno-sanitize-recover=all']
   if renewal:cmd+=['-DRENEWAL']
   subprocess.run(cmd+[str(src),'-o',str(exe)],check=True);result=subprocess.run([str(exe)])
   if a.negative:assert result.returncode!=0,'negative control unexpectedly passed'
   else:result.check_returncode()
 if a.build_dir:run(a.build_dir.resolve())
 else:
  with tempfile.TemporaryDirectory(prefix='instance-combat-') as d:run(Path(d))
if __name__=='__main__':main()
