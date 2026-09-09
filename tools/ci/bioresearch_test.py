#!/usr/bin/env python3
# ============================================================================
#  PN  /  DEVELOPMENT TOOLS
#  bioresearch_test.py
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/bioresearch_test.py
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

"""Native EDDA admission, staged encounter, hazards, and atomic reward checks.

Actual NPC/global-function bodies execute in the script VM. Party population,
map cells, movement, timers, mobs and inventory delivery are explicit doubles.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import tempfile
import episode21_encounter_flow_test as native
from episode_party_progression_test import scan_to, npc_body
SOURCE=native.ROOT/'npc/custom/instances/BioresearchLaboratory.txt'

def fixtures(build,pre_fix=False):
    source=SOURCE.read_text(); rows=[]; digest=hashlib.sha256()
    for name in ('F_BioReady','F_BioMove','F_BioBarrier'):
        m=re.search(r'function\s+script\s+'+name+r'\s*\{',source); b=m.end()-1
        rows.append((name,source[b:scan_to(source,b,'{','}')+1],'function'))
    for name in ('Sierra#bio_gate','Sierra#bio_start','Sierra#bio_regroup','Sierra#bio_boss','Sierra#bio_reward','Gas Device#bio','Vision Device#bio'):
        rows.append((name,'{\n'+npc_body(SOURCE,name)+'\n}',''))
    body=npc_body(SOURCE,'#BioControl')
    for label in re.findall(r'(?m)^(On\w+):',body):
        if label=='OnCleared':continue
        rows.append(('#BioControl::'+label,'{\ngoto '+label+';\n'+body+'\n}',''))
    header=['static Case source_cases[] = {']
    for n,(name,body,kind) in enumerate(rows):
        filename=f'bio_{n}.script';(build/filename).write_text(body);digest.update(body.encode())
        header.append('{'+','.join([json.dumps(name),json.dumps(filename),json.dumps(kind),'0','false'])+'},')
    header.append('};');(build/'episode_cases.inc').write_text('\n'.join(header))
    print(json.dumps({'bodies':len(rows),'source_sha256':hashlib.sha256(source.encode()).hexdigest()}),flush=True)
    return digest.hexdigest()

EXTRA=r"""
int party_size=2, selection=1, owned=1, entry_result=0, admissions=0, rolls=0;
int32 offline=0;
bool reuse_on_select=false;
std::map<int32,Move> locations;
std::map<std::string,int> live_mobs;
std::map<std::pair<int,int>,bool> walls;
std::vector<std::pair<int,int>> hazards;
std::vector<int> cleared;
int64 iv(const char* name,int index) { return i64db_i64get(instances.at(1)->regs.vars,reference_uid(add_str(name),index)); }
void iv(const char* name,int index,int value) { i64db_i64put(instances.at(1)->regs.vars,reference_uid(add_str(name),index),value); }
void av(script_state* st,int arg,int index,int64 value) {
 auto* d=script_getdata(st,arg); set_reg_num(st,nullptr,reference_uid(reference_getid(d),reference_getindex(d)+index),get_str(reference_getid(d)),value,reference_getref(d));
}
extern "C" map_session_data* bio_char(int32) asm("__wrap__Z13map_charid2sdi");
extern "C" map_session_data* bio_char(int32 id) { for(auto& p:players) if(p->status.char_id==id && p->id!=offline) return p.get(); return nullptr; }
extern "C" block_list* bio_bl(int32) asm("__wrap__Z9map_id2bli");
extern "C" block_list* bio_bl(int32 id) { for(auto& p:players) if(p->id==id && id!=offline) return p.get(); return id==NPC?&npc:nullptr; }
extern "C" map_session_data* bio_nick(const char*,bool) asm("__wrap__Z11map_nick2sdPKcb");
extern "C" map_session_data* bio_nick(const char* n,bool) { for(auto& p:players) if(!std::strcmp(p->status.name,n) && p->id!=offline) return p.get(); return nullptr; }
"""
WORLD=r"""
    if (command == "select") { if(reuse_on_select) { iv("'bio_admitted",100,0); reuse_on_select=false; } script_pushint(st,selection); }
    else if (command == "is_party_leader") script_pushint(st,st->rid==players[0]->id);
    else if (command == "instance_id") script_pushint(st,owned);
    else if (command == "instance_live_info") script_pushstrcopy(st,"Bioresearch Laboratory");
    else if (command == "instance_create") { owned=1;script_pushint(st,1); }
    else if (command == "instance_enter") { ++admissions;script_pushint(st,entry_result); }
    else if (command == "checkweight2") {
      auto* a=script_getdata(st,2);auto* b=script_getdata(st,3);auto* sd=bio_char(st->rid-99000010+100);
      int n=script_array_highest_key(st,sd,get_str(reference_getid(a)),reference_getref(a));
      int k=script_array_highest_key(st,sd,get_str(reference_getid(b)),reference_getref(b));
      std::map<int,int> bundle;for(int i=0;i<n;++i)bundle[get_val2_num(st,reference_uid(reference_getid(a),i),reference_getref(a))]=get_val2_num(st,reference_uid(reference_getid(b),i),reference_getref(b));
      std::map<int,int> expected=stage("'bio_mode")==1?std::map<int,int>{{25787,2}}:std::map<int,int>{{25786,9},{25787,15},{102571,1},{21051,1}};
      check(n==k && bundle==expected,"inventory preflight covers exact full reward bundle");script_pushint(st,capacity);
    }
    else if (command == "groupranditem") { ++rolls;check(script_getnum(st,2)==IG_BIO_W_BOX && script_getnum(st,3)==6,"weapon uses actual EDDA group and subgroup6");script_pushint(st,21051); }
    else if (command == "getpartymember") {
      check(script_getnum(st,2)==17 && (script_getnum(st,3)==1 || script_getnum(st,3)==2),"party enumeration uses owning party and exact ID kind");
      for(int i=0;i<party_size;++i) av(st,4,i,script_getnum(st,3)==1?players[i]->status.char_id:players[i]->id);
      script_pushint(st,party_size);
    } else if(command=="getmapxy") {
      auto* p=bio_nick(script_getstr(st,6),false);boundary(p!=nullptr,"map lookup only online party member");
      auto loc=locations[p->id]; auto* d=script_getdata(st,2);
      set_reg_str(st,nullptr,reference_getuid(d),get_str(reference_getid(d)),loc.map.c_str(),reference_getref(d));
      av(st,3,0,loc.x);av(st,4,0,loc.y);script_pushint(st,0);
    } else if(command=="warp") {
      int cid=script_hasdata(st,5)?script_getnum(st,5):0;auto* p=cid?bio_char(cid):bio_char(st->rid-99000010+100);
      boundary(p!=nullptr,"movement target online");Move m{p->id,script_getstr(st,2),script_getnum(st,3),script_getnum(st,4)};moves.push_back(m);locations[p->id]=m;
    } else if(command=="monster") { spawned+=script_getnum(st,7);live_mobs[script_getstr(st,8)]+=script_getnum(st,7); }
    else if(command=="mobcount") script_pushint(st,live_mobs[script_getstr(st,3)]);
    else if(command=="setcell") { for(int x=script_getnum(st,3);x<=script_getnum(st,5);++x)for(int y=script_getnum(st,4);y<=script_getnum(st,6);++y)walls[{x,y}]=script_getnum(st,8); }
    else if(command=="sc_start") { int gid=script_getnum(st,7);check(gid==players[0]->id || gid==players[1]->id,"hazard uses account GID, not CID");hazards.emplace_back(script_getnum(st,2),gid); }
    else if(command=="sc_end") { cleared.push_back(script_getnum(st,2)); }
    else if(command=="dispbottom" || command=="setnpctimer" || command=="initnpctimer" || command=="stopnpctimer") { }
    else if (command == "instance_mapname"
"""
MAIN=r"""
extern "C" int __wrap_main(int argc,char** argv) {
 boundary(argc==2,"fixture directory supplied");fixture_dir=argv[1];deny_network();static char name[]="bioresearch-test";SERVER_NAME=name;
 malloc_init();db_init();do_init_database();timer_init();install_world_doubles();do_init_script();save_settings=0;battle_config.atcommand_disable_npc=0;
 npc.id=NPC;npc.type=BL_NPC;npc.instance_id=1;instances[1]=std::make_shared<s_instance_data>();instances.at(1)->state=INSTANCE_BUSY;instances.at(1)->regs.vars=i64db_alloc(DB_OPT_RELEASE_DATA);
 for(int i=0;i<3;++i){auto p=std::make_unique<map_session_data>();p->id=99000010+i;p->type=BL_PC;p->status.account_id=p->id;p->status.char_id=100+i;p->fd=0;p->state.ignoretimeout=true;p->npc_idle_timer=INVALID_TIMER;std::snprintf(p->status.name,NAME_LENGTH,"BioPlayer%d",i);players.emplace_back(std::move(p));}
 for(int id:{16388,16390,16392,16399,16400}){auto q=std::make_shared<s_quest_db>();q->id=id;if(id==16390){q->time=14400;q->time_at=true;}quest_db.put(id,q);}
 for(const auto& c:source_cases){std::ifstream f(fixture_dir+"/"+c.path);const std::string s{std::istreambuf_iterator<char>(f),std::istreambuf_iterator<char>()};auto* code=parse_script(s.c_str(),c.name,1,0);boundary(code!=nullptr,"actual source parses");if(!std::strcmp(c.var,"function"))strdb_put(script_get_userfunc_db(),c.name,code);else codes[c.name]=code;}
 auto setup=[&](){offline=0;reuse_on_select=false;reset();party_size=2;selection=1;owned=1;entry_result=0;admissions=rolls=0;locations.clear();live_mobs.clear();walls.clear();hazards.clear();cleared.clear();stage("'bio_party",17);for(auto& p:players){p->status.party_id=17;p->status.base_level=170;p->battle_status.hp=1000;locations[p->id]={p->id,"1@gol1",197,36};}};
 auto admit=[&](){finish("Sierra#bio_gate",0);finish("Sierra#bio_gate",1);};
 auto eventrun=[&](const std::string& e){current=e;attached=nullptr;run_script(codes.at(e),0,0,NPC);};
 auto drain=[&](){unsigned count=0;while(!events.empty()){boundary(++count<30,"bounded event queue");auto e=events.front();events.erase(events.begin());eventrun(e);}};
 setup();players[0]->status.base_level=169;finish("Sierra#bio_gate");check(admissions==0,"level169 blocked");
 setup();entry_result=1;finish("Sierra#bio_gate");check(q(0,16390)==-1 && !iv("'bio_admitted",100),"failed entry consumes neither cooldown nor admission");
 setup();finish("Sierra#bio_gate");check(q(0,16390)==Q_ACTIVE && iv("'bio_admitted",100),"successful entry starts shared cooldown and CID admission");finish("Sierra#bio_gate");check(admissions==2,"same live CID reenters during cooldown");seed_quest(1,16390);finish("Sierra#bio_gate",1);check(admissions==2,"unadmitted cooldown member cannot borrow party reservation");
 setup();finish("Sierra#bio_gate");reuse_on_select=true;finish("Sierra#bio_gate");check(admissions==1 && q(0,16390)==Q_ACTIVE,"ID reuse during entry dialogue rechecks live CID admission and cooldown");
 setup();admit();offline=players[1]->id;finish("Sierra#bio_start");check(stage("'bio_zone")==0 && errors==0,"offline party blocks start without RID error");offline=0;players[1]->battle_status.hp=0;finish("Sierra#bio_start");check(stage("'bio_zone")==0,"dead member blocks start");
 setup();admit();invoke("Sierra#bio_start");to_close2(0);stage("'bio_zone",1);acknowledge(0);check(stage("'bio_mode")==0,"late start dialogue cannot overwrite active run");
 for(int mode:{1,2}) {
  setup();admit();selection=mode;finish("Sierra#bio_start");selection=1;check(stage("'bio_mode")==mode && stage("'bio_started_count")==2,"selected mode freezes starting roster");
  check(q(0,16388)==Q_COMPLETE && q(1,16388)==Q_COMPLETE,"mode briefing completes introduction for both members");
  iv("'bio_admitted",102,1);auto before_admissions=admissions;finish("Sierra#bio_gate",2);check(admissions==before_admissions,"admitted outsider absent from frozen start cannot reenter");
  drain();
  eventrun("#BioControl::OnTimer10000");check(hazards.size()==2,"zone1 sleep reaches both online party members");hazards.clear();players[1]->status.party_id=99;eventrun("#BioControl::OnTimer10000");check(hazards.size()==1,"hazard excludes changed party member");players[1]->status.party_id=17;
  finish("Gas Device#bio");hazards.clear();eventrun("#BioControl::OnTimer10000");check(hazards.empty(),"gas device disables sleep for current section");
  for(int zone=1;zone<=7;++zone){
   int waves=zone==1||zone==6?2:zone==4||zone==5?4:3;
   for(int wave=0;wave<waves;++wave){
    if(zone==5){
      if(wave==2){offline=0;party_size=2;players[1]->status.party_id=17;locations[players[1]->id]={players[1]->id,"1@gol1",118,22};}
      if(wave==0){party_size=3;finish("Sierra#bio_regroup");check(stage("'bio_waiting")==1,"replacement member cannot enter frozen expedition");party_size=2;}
      if(wave==1){offline=players[1]->id;}
      finish("Sierra#bio_regroup");drain();check(locations[players[0]->id].x==stage("'bio_x"),"containment checkpoint follows active cage");}
    if(zone==4 && wave==0){hazards.clear();cleared.clear();eventrun("#BioControl::OnTimer10000");check(hazards.size()==2 && hazards[0].first==SC_BLIND,"zone4 blindness targets connected members");finish("Vision Device#bio");check(cleared.size()==1 && cleared[0]==SC_BLIND,"vision device actually clears blindness");}
    std::string event="#BioControl::OnWave"+std::to_string(zone);check(live_mobs[event]==(mode==1?1:6),"mode-specific section population");
    eventrun(event);check(stage("'bio_wave")==wave,"live enemies prevent wave advance");live_mobs[event]=0;eventrun(event);auto nextwave=stage("'bio_wave"),nextzone=stage("'bio_zone");eventrun(event);check(stage("'bio_wave")==nextwave&&stage("'bio_zone")==nextzone,"duplicate queued clear cannot advance twice");drain();
   }
  }
  check(std::all_of(walls.begin(),walls.end(),[](const auto& p){return p.second;}),"all cage perimeter cells restored after clear");
  if(mode==2){check(stage("'bio_zone")==8,"Battle reaches locked boss staging");finish("Sierra#bio_gate",1);check(admissions==3,"admitted member can recover before boss lock");offline=players[1]->id;finish("Sierra#bio_boss");check(stage("'bio_boss_started")==1&&live_mobs["#BioControl::OnBossDead"]==1,"one boss starts with eligible roster");check(!iv("'bio_eligible",101),"offline original member excluded from boss lock");offline=0;players[1]->status.party_id=17;finish("Sierra#bio_gate",1);check(admissions==3,"returning original member cannot bypass boss roster lock");finish("Sierra#bio_gate",0);check(admissions==4,"eligible boss member can recover after lock");finish("Sierra#bio_boss");check(live_mobs["#BioControl::OnBossDead"]==1,"boss start is idempotent");eventrun("#BioControl::OnBossDead");check(stage("'bio_zone")==8,"live boss prevents false completion");live_mobs["#BioControl::OnBossDead"]=0;eventrun("#BioControl::OnBossDead");}
  check(stage("'bio_zone")==9,"mode completes all seven zones");if(mode==2){finish("Sierra#bio_reward",1);check(items[players[1]->id].empty(),"original member outside boss lock cannot claim");}finish("Sierra#bio_reward",2);check(items[players[2]->id].empty(),"late participant cannot claim");capacity=false;finish("Sierra#bio_reward");check(items[players[0]->id].empty(),"full inventory grants nothing");auto before=rolls;capacity=true;finish("Sierra#bio_reward");check(rolls==before,"inventory retry never rerolls weapon");check(items[players[0]->id][25787]==(mode==1?2:15),"mode fragment quantity exact");if(mode==2)check(items[players[0]->id][25786]==9&&items[players[0]->id][102571]==1&&items[players[0]->id][21051]==1,"Battle atomic documents token and cached weapon");auto loot=items;quest_delete(players[0].get(),16390);finish("Sierra#bio_reward");check(items==loot,"daily reset cannot reopen instance reward");
 }
 check(errors==0,"no native diagnostics");for(auto& c:codes)script_free_code(c.second);codes.clear();reset();script_free_vars(instances.at(1)->regs.vars);instances.clear();quest_db.clear();players.clear();attached=nullptr;do_final_script();timer_final();db_final();std::printf("BIORESEARCH_RESULT checks=%u failures=%u errors=%u\n",checks,failures,errors);malloc_final();return failures||errors?1:0;
}
"""
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--build-dir',type=Path);p.add_argument('--prepare-only',action='store_true');args=p.parse_args()
    prefix='#include <algorithm>\n#include "map/itemdb.hpp"\n'+native.CPP.split('extern "C" int __wrap_main',1)[0]
    prefix=prefix.replace('namespace {','int64 get_val2_num(script_state*,int64,reg_db*);\nnamespace {',1)
    prefix=prefix.replace('int32 world(script_state* st) {',EXTRA+'\nint32 world(script_state* st) {')
    prefix=prefix.replace('    if (command == "instance_mapname"',WORLD,1)
    prefix=prefix.replace('if (p->id == id) return p.get();','if (p->id == id && id!=offline) return p.get();')
    names=['select','is_party_leader','instance_live_info','instance_create','instance_enter','checkweight2','groupranditem','getpartymember','getmapxy','mobcount','setcell','sc_start','sc_end','dispbottom','setnpctimer','initnpctimer','stopnpctimer']
    prefix=prefix.replace('"getexp", "callfunc"};','"getexp",'+','.join(json.dumps(n) for n in names)+'};')
    native.CPP=prefix+MAIN;native.fixtures=fixtures;native.WRAPPERS+=('_Z13map_charid2sdi','_Z9map_id2bli','_Z11map_nick2sdPKcb')
    def run(d):
        d.mkdir(parents=True,exist_ok=True);native.run(d.resolve(),False,False,args.prepare_only,completion_marker='BIORESEARCH_RESULT ')
    if args.build_dir:run(args.build_dir)
    else:
        with tempfile.TemporaryDirectory(prefix='bioresearch-') as d:run(Path(d))
if __name__=='__main__':main()
