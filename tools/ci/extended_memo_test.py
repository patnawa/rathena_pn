#!/usr/bin/env python3
"""Exercise production memo capacity, recording, selection and quest data."""
from pathlib import Path
import re
import struct
import subprocess
import tempfile
import zlib
import yaml
from episode_party_progression_test import scan_to

ROOT=Path(__file__).resolve().parents[2]
def function(source,signature):
    start=source.index(signature);brace=source.index('{',start)
    return source[start:scan_to(source,brace,'{','}')+1]
pc=(ROOT/'src/map/pc.cpp').read_text(encoding='utf-8')
skill=(ROOT/'src/map/skill.cpp').read_text(encoding='utf-8')
portal=(ROOT/'src/map/skills/acolyte/warpportal.cpp').read_text(encoding='utf-8')
helper=function(pc,'int32 pc_memo_slots(')
record=function(pc,'bool pc_memo(')
cast=function(portal,'void SkillWarpPortal::castendPos2(')
selection=skill[skill.index('\t\t\tconst int32 destinations = pc_memo_slots'):skill.index('\n\n\t\t\tif(!skill_check_condition_castend',skill.index('\t\t\tconst int32 destinations = pc_memo_slots'))]
reset=pc[pc.index('\t// The missionary reward is skill progress'):pc.index('\n\tsd->status.skill_point += skill_point;',pc.index('\t// The missionary reward is skill progress'))]
cpp=r'''
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <vector>
using int32=int32_t;using int64=int64_t;using uint8=uint8_t;using uint16=uint16_t;using t_tick=int64_t;
#define PACKETVER_MAIN_NUM 20250401
constexpr int MAX_MEMOPOINTS=6,MAPID_CARDINAL=1,MAPID_INQUISITOR=2,MF_NOMEMO=1,MF_NOWARPTO=2,PC_PERM_WARP_ANYWHERE=1,NOTIFY_MAPINFO_CANT_MEMO=1,AL_WARP=27,WARPPOINT_NOT_LEARNED=2,WARPPOINT_LOW_LEVEL=3,WARPPOINT_SUCCESS=0,SC_CURSEDCIRCLE_ATKER=1,SKILL_NOCONSUME_REQ=1,BL_PC=1,HAVEQUEST=0;
struct s_point_str { char map[16]={};int x=0,y=0; };
struct block_list {};
struct map_session_data: block_list {
 int m=0,fd=0,x=10,y=20,class_=MAPID_CARDINAL,level=4;
 struct { s_point_str memo_point[6],save_point; } status;
 std::map<std::string,int64> vars;
};
int last_message=0,deleted_quests=0;bool blocked=false;std::string current_map="prontera";
struct map_data { int instance_id=0; } mapdata;
int64 pc_readglobalreg(map_session_data* sd,const char* key) { return sd->vars[key]; }
void pc_setglobalreg(map_session_data* sd,const char* key,int64 value) { sd->vars[key]=value; }
const char* add_str(const char* name) { return name; }
bool map_getmapflag(int,int) { return blocked; }
bool pc_has_permission(map_session_data*,int) { return false; }
void clif_skill_teleportmessage(map_session_data&,int n) { last_message=n; }
int pc_checkskill(map_session_data* sd,int) { return sd->level; }
void clif_skill_memomessage(map_session_data&,int n) { last_message=n; }
const char* map_mapid2mapname(int) { return current_map.c_str(); }
map_data* map_getmapdata(int) { return &mapdata; }
void clif_displaymessage(int,const char*) {}
const char* msg_txt(map_session_data*,int) { return "message"; }
void safestrncpy(char* target,const char* source,size_t n) { std::strncpy(target,source,n);target[n-1]=0; }
int quest_check(map_session_data*,int,int) { return 0; }
void quest_delete(map_session_data*,int) { ++deleted_quests; }
#define nullpo_ret(p) if(!(p)) return false
#define ARR_FIND(start,end,i,predicate) for(i=start;i<end;++i) if(predicate) break
#define BL_CAST(type,p) static_cast<map_session_data*>(p)
struct status_change { void* getSCE(int) { return nullptr; } };
status_change* status_get_sc(block_list*) { return nullptr; }
void status_change_end(block_list*,int) {}
std::vector<std::string> offered;
void clif_skill_warppoint(map_session_data&,int,int,std::vector<std::string>& maps) { offered=maps; }
class SkillWarpPortal { public:int getSkillId()const {return AL_WARP;} void castendPos2(block_list*,int32,int32,uint16,t_tick,int32&)const; };
'''
cpp+=helper+'\n'+record+'\n'+cast+'\nvoid reset(map_session_data* sd) {\n'+reset+'\n}\n'
cpp+='bool select_destination(map_session_data* sd,int lv,const char* mapname) { const s_point_str* p[7];p[0]=&sd->status.save_point;for(int m=0;m<6;++m)p[m+1]=&sd->status.memo_point[m];int i,x=-1,y=-1;\n#define skill_failed(sd) return false\n'+selection+'\nreturn x>=0 && y>=0;\n#undef skill_failed\n}\n'
cpp+=r'''
int main() {
 int checks=0,failures=0;auto check=[&](bool ok){++checks;if(!ok){++failures;std::printf("FAIL %d\n",checks);}};
 map_session_data sd;std::strcpy(sd.status.save_point.map,"save");
 for(int job=1;job<=3;++job) for(int level=1;level<=4;++level) for(int extra=-1;extra<=4;++extra) {
  sd.class_=job;sd.level=level;sd.vars["PN_MemoExtra"]=extra;
  int expected=std::max(0,level-1)+(level==4 && job<=2?std::clamp(extra,0,3):0);
  check(pc_memo_slots(&sd,level)==expected);
 }
 sd.class_=MAPID_CARDINAL;sd.level=4;sd.vars["PN_MemoExtra"]=3;
 for(int m=0;m<6;++m) {current_map="map"+std::to_string(m);check(pc_memo(&sd,m));}
 int flag=0;SkillWarpPortal portal;portal.castendPos2(&sd,10,20,4,0,flag);
 check(offered.size()==7 && offered[6]=="map5" && flag==SKILL_NOCONSUME_REQ);
 check(select_destination(&sd,4,"map5"));check(!select_destination(&sd,3,"map5"));check(!select_destination(&sd,4,"forged"));
 sd.vars["PN_MemoExtra"]=0;check(!select_destination(&sd,4,"map5"));check(!pc_memo(&sd,3));
 sd.vars["PN_MemoExtra"]=3;current_map="map5";check(pc_memo(&sd,-1));
 check(std::strcmp(sd.status.memo_point[0].map,"map5")==0 && std::strcmp(sd.status.memo_point[5].map,"map4")==0);
 current_map="new";blocked=true;check(!pc_memo(&sd,-1));check(std::strcmp(sd.status.memo_point[0].map,"map5")==0);blocked=false;
 mapdata.instance_id=1;check(!pc_memo(&sd,-1));check(std::strcmp(sd.status.memo_point[0].map,"map5")==0);mapdata.instance_id=0;
 sd.vars["PN_MissionStage"]=4;sd.vars["PN_MissionDone"]=3;reset(&sd);
 check(pc_memo_slots(&sd,4)==3 && sd.vars["PN_MissionStage"]==0 && deleted_quests==15);
 for(int m=3;m<6;++m)check(sd.status.memo_point[m].map[0]==0);
 std::printf("EXTENDED_MEMO checks=%d failures=%d\n",checks,failures);return failures?1:0;
}
'''
with tempfile.TemporaryDirectory(prefix='pn-memo-') as temp:
    path,exe=Path(temp)/'test.cpp',Path(temp)/'test';path.write_text(cpp)
    subprocess.run(['g++','-std=c++17','-fsanitize=address,undefined','-fno-sanitize-recover=all',str(path),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
quests={q['Id']:q for q in yaml.safe_load((ROOT/'db/import/quest_db.yml').read_text(encoding='utf-8'))['Body']}
for q,mob,m in [(16592,'BRUTAL_MURDERER','nif_dun01'),(16596,'ABYSSMAN','ein_dun03')]:
    target=quests[q]['Targets'][0]
    assert target['Count']==100 and target['Location']==m and target['MapMobTargets']=={mob:True}
script=(ROOT/'npc/custom/extended_memo.txt').read_text(encoding='utf-8')
positions=re.findall(r'^(prontera|umbala|einbroch|nif_dun01|ein_dun03),(\d+),(\d+),4\tscript',script,re.M)
cells={}
for filename in ('db/map_cache.dat','db/re/map_cache.dat'):
    raw=(ROOT/filename).read_bytes();pos=8
    for _ in range(struct.unpack_from('<H',raw,4)[0]):
        name,w,h,size=struct.unpack_from('<12shhi',raw,pos);pos+=20;name=name.split(b'\0')[0].decode()
        if name in {p[0] for p in positions}:cells[name]=(w,zlib.decompress(raw[pos:pos+size]))
        pos+=size
for m,x,y in positions:
    w,data=cells[m];assert data[int(y)*w+int(x)] in (0,3,6),(m,x,y)
assert len(positions)==9
assert 'if( flag&2 || !skill_point ) return skill_point;' in pc[:pc.index('// The missionary reward')]
print('MEMO_QUEST two map-specific 100-kill objectives, six distinct rescues, nine walkable NPCs')
