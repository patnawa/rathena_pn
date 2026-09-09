#!/usr/bin/env python3
"""Native Alice maze, boss, daily admission and Confused Boy exchange regressions."""
import argparse,hashlib,json,re,tempfile
from pathlib import Path
import episode21_encounter_flow_test as native
import bioresearch_test as bio
from episode_party_progression_test import scan_to,npc_body
SOURCE=native.ROOT/'npc/custom/instances/AliceTwistedMadness.txt'
RECIPES=[{'output': 28902, 'zeny': 5000000, 'materials': {'1001074': 1, '1001076': 50, '1001083': 50}}, {'output': 410233, 'zeny': 5000000, 'materials': {'1001074': 5, '1001076': 150, '1001083': 150}}, {'output': 420210, 'zeny': 30000000, 'materials': {'1001082': 5, '1001074': 15, '1001079': 300, '1001076': 300, '1001083': 300}}, {'output': 420213, 'zeny': 30000000, 'materials': {'1001082': 5, '1001074': 15, '1001079': 300, '1001076': 300, '1001083': 300}}, {'output': 420220, 'zeny': 30000000, 'materials': {'1001082': 5, '1001074': 15, '1001079': 300, '1001076': 300, '1001083': 300}}, {'output': 420269, 'zeny': 30000000, 'materials': {'1001082': 5, '1001074': 15, '1001079': 300, '1001076': 300, '1001083': 300}}, {'output': 480297, 'zeny': 10000000, 'materials': {'1001074': 3, '1001076': 300}}, {'output': 480298, 'zeny': 10000000, 'materials': {'1001074': 3, '1001082': 2, '1001079': 300}}, {'output': 480310, 'zeny': 10000000, 'materials': {'1001074': 3, '1001083': 300}}]

def fixtures(build,pre_fix=False):
 source=SOURCE.read_text();rows=[];digest=hashlib.sha256()
 for name in ('F_AliceTravel','F_AliceMember','F_AliceCorridor','F_AliceLeadership'):
  m=re.search(r'function\s+script\s+'+name+r'\s*\{',source);b=m.end()-1;rows.append((name,source[b:scan_to(source,b,'{','}')+1],'function'))
 for name in re.findall(r'(?m)^\S+\tscript\t([^\t]+)\t',source):
  if name=='#AliceControl':continue
  body=npc_body(SOURCE,name);prefix='goto OnTouch;\n' if re.search(r'(?m)^OnTouch:',body) else ''
  rows.append((name,'{\n'+prefix+body+'\n}',''))
 body=npc_body(SOURCE,'#AliceControl')
 for label in re.findall(r'(?m)^(On\w+):',body):
  if label=='OnBossCleared':continue
  rows.append(('#AliceControl::'+label,'{\ngoto '+label+';\n'+body+'\n}',''))
 h=['static Case source_cases[] = {']
 for i,(name,body,kind) in enumerate(rows):
  filename=f'alice_{i}.script';(build/filename).write_text(body);digest.update(body.encode());h.append('{'+','.join([json.dumps(name),json.dumps(filename),json.dumps(kind),'0','false'])+'},')
 h.append('};\nstruct Recipe { int output,zeny; std::map<int,int> costs; };\nstatic Recipe recipes[] = {')
 for r in RECIPES:h.append('{'+str(r['output'])+','+str(r['zeny'])+',{'+','.join('{'+str(k)+','+str(v)+'}' for k,v in r['materials'].items())+'}},')
 h.append('};');(build/'episode_cases.inc').write_text('\n'.join(h));print(json.dumps({'bodies':len(rows),'source_sha256':hashlib.sha256(source.encode()).hexdigest()}),flush=True)
 return digest.hexdigest()

EXTRA=bio.EXTRA+r"""
std::map<int64,int64> server_registers;
extern "C" bool alice_mapset(int64,int64) asm("__wrap__Z13mapreg_setregll");
extern "C" bool alice_mapset(int64 id,int64 value){server_registers[id]=value;return true;}
extern "C" int64 alice_mapread(int64) asm("__wrap__Z14mapreg_readregl");
extern "C" int64 alice_mapread(int64 id){return server_registers[id];}
int64 clock_now=100000;
int chance=50;int32 leader=99000010;
std::vector<int> choices;
std::vector<std::tuple<int,int,int64>> changes;
bool drain_wallet=false, steal_leader=false;int leadership_changes=0, expected_weight_output=0;
extern "C" bool alice_setparam(map_session_data*,int64,int64) asm("__wrap__Z11pc_setparamP16map_session_datall");
extern "C" bool alice_setparam(map_session_data* sd,int64 type,int64 value) { boundary(type==SP_ZENY,"only wallet parameter mutation doubled");sd->status.zeny=value;return true; }
"""
WORLD=r"""
    if(command=="select") { int value=1;if(!choices.empty()){value=choices.front();choices.erase(choices.begin());}if(reuse_on_select){iv("'alice_admitted",100,0);reuse_on_select=false;}if(steal_leader){leader=players[2]->id;steal_leader=false;}if(drain_wallet && choices.empty())bio_char(st->rid-99000010+100)->status.zeny=0;script_pushint(st,value); }
    else if(command=="is_party_leader") script_pushint(st,st->rid==leader);
    else if(command=="party_changeleader"){check(script_getnum(st,2)==17,"leadership change uses owning party");leader=bio_char(script_getnum(st,3))->id;++leadership_changes;script_pushint(st,1);}
    else if(command=="instance_id")script_pushint(st,owned);
    else if(command=="instance_live_info")script_pushstrcopy(st,"Alice Twisted Madness");
    else if(command=="instance_create"){owned=1;script_pushint(st,1);}
    else if(command=="instance_enter"){++admissions;script_pushint(st,entry_result);}
    else if(command=="gettimetick")script_pushint(st,clock_now);
    else if(command=="rand")script_pushint(st,script_hasdata(st,3)?script_getnum(st,2):script_getnum(st,2)==100?chance:0);
    else if(command=="getpartymember"){
      check(script_getnum(st,2)==17,"party enumeration uses owning party");for(int i=0;i<party_size;++i)av(st,4,i,script_getnum(st,3)==1?players[i]->status.char_id:players[i]->id);script_pushint(st,party_size);
    } else if(command=="getmapxy"){
      auto* p=bio_nick(script_getstr(st,6),false);boundary(p!=nullptr,"map lookup only online member");auto loc=locations[p->id];auto* d=script_getdata(st,2);set_reg_str(st,nullptr,reference_getuid(d),get_str(reference_getid(d)),loc.map.c_str(),reference_getref(d));av(st,3,0,loc.x);av(st,4,0,loc.y);script_pushint(st,0);
    } else if(command=="warp"){
      int cid=script_hasdata(st,5)?script_getnum(st,5):0;auto* p=cid?bio_char(cid):bio_char(st->rid-99000010+100);boundary(p!=nullptr,"warp targets online character");Move m{p->id,script_getstr(st,2),script_getnum(st,3),script_getnum(st,4)};moves.push_back(m);locations[p->id]=m;
    } else if(command=="monster"){spawned+=script_getnum(st,7);live_mobs[script_getstr(st,8)]+=script_getnum(st,7);script_pushint(st,90000000+spawned);}
    else if(command=="mobcount")script_pushint(st,live_mobs[script_getstr(st,3)]);
    else if(command=="getunitdata")av(st,3,UMOB_MODE,MD_CANMOVE|MD_CANATTACK|MD_AGGRESSIVE);
    else if(command=="setunitdata")changes.emplace_back(script_getnum(st,2),script_getnum(st,3),script_getnum64(st,4));
    else if(command=="unitexists")script_pushint(st,1);
    else if(command=="killmonster")live_mobs[script_getstr(st,3)]=0;
    else if(command=="countitem")script_pushint(st,items[st->rid][script_getnum(st,2)]);
    else if(command=="delitem"){int id=script_getnum(st,2),n=script_getnum(st,3);boundary(items[st->rid][id]>=n,"exchange cannot overdraw materials");items[st->rid][id]-=n;}
    else if(command=="getitemname")script_pushstrcopy(st,"Item");
    else if(command=="checkweight"){check(script_getnum(st,2)==expected_weight_output&&script_getnum(st,3)==1,"exchange preflight checks actual selected output");script_pushint(st,capacity);}
    else if(command=="checkweight2"){
      auto* a=script_getdata(st,2);auto* b=script_getdata(st,3);auto* sd=bio_char(st->rid-99000010+100);int n=script_array_highest_key(st,sd,get_str(reference_getid(a)),reference_getref(a));int k=script_array_highest_key(st,sd,get_str(reference_getid(b)),reference_getref(b));std::map<int,int> bundle;
      for(int i=0;i<n;++i)bundle[get_val2_num(st,reference_uid(reference_getid(a),i),reference_getref(a))]=get_val2_num(st,reference_uid(reference_getid(b),i),reference_getref(b));
      std::map<int,int> expected{{1001074,1}};if(stage("'alice_mode")==2)expected[1001082]=1+stage("'alice_fast_clear");check(n==k&&bundle==expected,"clear reward preflight covers entire exact bundle");script_pushint(st,capacity);
    } else if(command=="dispbottom"||command=="setnpctimer"||command=="initnpctimer"||command=="stopnpctimer"){}
    else if(command=="instance_mapname"
"""
MAIN=r"""
extern "C" int __wrap_main(int argc,char** argv){
 boundary(argc==2,"fixture directory supplied");fixture_dir=argv[1];deny_network();static char name[]="alice-test";SERVER_NAME=name;malloc_init();db_init();do_init_database();timer_init();install_world_doubles();do_init_script();save_settings=0;battle_config.atcommand_disable_npc=0;npc.id=NPC;npc.type=BL_NPC;npc.instance_id=1;instances[1]=std::make_shared<s_instance_data>();instances.at(1)->state=INSTANCE_BUSY;instances.at(1)->regs.vars=i64db_alloc(DB_OPT_RELEASE_DATA);
 for(int i=0;i<3;++i){auto p=std::make_unique<map_session_data>();p->id=99000010+i;p->type=BL_PC;p->status.account_id=p->id;p->status.char_id=100+i;p->fd=0;p->state.ignoretimeout=true;p->npc_idle_timer=INVALID_TIMER;std::snprintf(p->status.name,NAME_LENGTH,"AlicePlayer%d",i);players.emplace_back(std::move(p));}
 auto qdb=std::make_shared<s_quest_db>();qdb->id=62090;qdb->time=14400;qdb->time_at=true;quest_db.put(62090,qdb);
 for(const auto& c:source_cases){std::ifstream f(fixture_dir+"/"+c.path);const std::string s{std::istreambuf_iterator<char>(f),std::istreambuf_iterator<char>()};auto* code=parse_script(s.c_str(),c.name,1,0);boundary(code!=nullptr,"actual Alice source parses");if(!std::strcmp(c.var,"function"))strdb_put(script_get_userfunc_db(),c.name,code);else codes[c.name]=code;}
 auto setup=[&](int mode){offline=0;reuse_on_select=false;reset();choices.clear();changes.clear();party_size=2;owned=1;entry_result=0;admissions=0;clock_now=100000;chance=50;leader=players[0]->id;drain_wallet=steal_leader=false;leadership_changes=0;expected_weight_output=0;live_mobs.clear();locations.clear();stage("'alice_party",17);stage("'alice_mode",mode);stage("'alice_created",clock_now);for(auto& p:players){p->status.party_id=17;p->status.base_level=175;p->status.zeny=0;p->battle_status.hp=1000;locations[p->id]={p->id,"1@alice_mad",110,110};}};
 auto eventrun=[&](const std::string& e){current=e;attached=nullptr;run_script(codes.at(e),0,0,NPC);};
 auto drain=[&](){unsigned n=0;while(!events.empty()){boundary(++n<20,"bounded actual event queue");auto e=events.front();events.erase(events.begin());eventrun(e);}};
 auto admit=[&](){for(int i=0;i<party_size;++i)finish("Alice#mad_entry",i);};
 setup(1);players[0]->status.base_level=174;finish("Alice#mad_entry");check(admissions==0,"level174 blocked");
 setup(1);entry_result=1;finish("Alice#mad_entry");check(q(0,62090)==-1&&!iv("'alice_admitted",100),"failed entry uses no daily admission");
 setup(1);finish("Alice#mad_entry");check(q(0,62090)==Q_ACTIVE&&iv("'alice_admitted",100),"successful admission records exact CID and daily quest");finish("Alice#mad_entry");check(admissions==2,"same instance permits reentry");seed_quest(1,62090);finish("Alice#mad_entry",1);check(admissions==2,"other member cannot borrow admission during cooldown");
 setup(1);finish("Alice#mad_entry");reuse_on_select=true;finish("Alice#mad_entry");check(admissions==1&&q(0,62090)==Q_ACTIVE,"reused numeric instance ID cannot borrow stale admission");
 setup(1);admit();offline=players[1]->id;finish("Alice#mad_start");check(!stage("'alice_started")&&errors==0,"offline member blocks first briefing safely");offline=0;invoke("Alice#mad_start");to_close2(0);stage("'alice_started",1);acknowledge(0);check(stage("'alice_count")==0,"late start dialogue cannot replace roster");
 setup(1);admit();finish("Alice#mad_start");drain();check(iv("'alice_solution",100)==1,"per-character solution initialized once");finish("Final Passage#mad");check(iv("'alice_phase",100)==0,"return portal cannot skip first maze");finish("Maze Door#mad2");check(iv("'alice_phase",100)==1,"door enters corridor");finish("Corridor Exit#mad1");check(iv("'alice_phase",100)==0,"wrong door returns to first maze");finish("Maze Door#mad1");chance=0;finish("Corridor Exit#mad1");check(iv("'alice_phase",100)==2,"corridor can enter mini detour");finish("Detour Exit#mad");check(iv("'alice_phase",100)==1&&locations[players[0]->id].x==12,"detour preserves original corridor");chance=50;finish("Corridor Exit#mad1");check(iv("'alice_phase",100)==3&&locations[players[0]->id].x==72,"correct door reaches sixth corridor");finish("Corridor Exit#mad6");finish("Clock Gate#mad");check(iv("'alice_phase",100)==4,"clock gate requires both switches");finish("Clock Switch#mad1");finish("Clock Gate#mad");check(iv("'alice_phase",100)==4,"one clock switch insufficient");finish("Clock Switch#mad2");finish("Clock Gate#mad");check(iv("'alice_phase",100)==5,"both switches open second maze");finish("Second Maze Exit#mad");finish("Chess Exit#mad");finish("Final Passage#mad");check(iv("'alice_phase",100)==9&&locations[players[0]->id].x==350,"Normal maze reaches boss chamber");
 setup(2);admit();finish("Alice#mad_start");drain();iv("'alice_phase",100,8);finish("Hidden Passage#mad1");check(iv("'alice_phase",100)==8,"wrong hidden passage does not count");finish("Hidden Passage#mad0");check(iv("'alice_phase",100)==9 && iv("'alice_reached",100),"correct hidden passage records arrival once");choices={1};finish("Alice#mad_leadership");check(leader==players[1]->id&&leadership_changes==1,"current leader can promote online starting member");finish("Alice#mad_leadership",0);check(leadership_changes==1,"nonleader cannot promote");steal_leader=true;choices={1};finish("Alice#mad_leadership",1);check(leadership_changes==1,"leadership revalidated after selection");
 for(int mode:{1,2}){
  setup(mode);party_size=3;admit();finish("Alice#mad_start");drain();iv("'alice_phase",100,9);locations[players[0]->id]={players[0]->id,"1@alice_mad",350,310};iv("'alice_phase",101,9);iv("'alice_phase",102,9);
  if(mode==2){iv("'alice_reached",100,1);finish("Alice#mad_boss");check(!stage("'alice_boss_started"),"one of three cannot trigger hard quorum");iv("'alice_reached",101,1);offline=players[1]->id;finish("Alice#mad_boss");check(!stage("'alice_boss_started"),"historical offline arrival does not satisfy quorum");offline=0;finish("Alice#mad_boss");check(!stage("'alice_boss_started"),"historical arrival outside boss staging does not satisfy quorum");locations[players[1]->id]={players[1]->id,"1@alice_mad",350,310};players[1]->battle_status.hp=0;}
  finish("Alice#mad_boss");drain();check(stage("'alice_boss_started")==1,"eligible leader starts boss after quorum");check(iv("'alice_eligible",102)==1,"Hard recall includes connected remaining member");
  int64 firstmode=MD_CANMOVE|MD_CANATTACK|MD_AGGRESSIVE;if(mode==2)firstmode|=MD_IGNOREMAGIC|MD_IGNORERANGED|MD_IGNOREMISC;bool firstfound=false;for(const auto& c:changes)if(std::get<1>(c)==UMOB_MODE&&std::get<2>(c)==firstmode)firstfound=true;check(firstfound,"first boss uses exact intended channel mask");
  if(mode==2){changes.clear();eventrun("#AliceControl::OnTimer1000");check(std::get<2>(changes.back())==1,"dead participant shields boss");offline=players[1]->id;changes.clear();eventrun("#AliceControl::OnTimer1000");check(std::get<2>(changes.back())==1&&errors==0,"dead logout cannot remove shield or query offline HP");offline=0;players[1]->battle_status.hp=1000;changes.clear();eventrun("#AliceControl::OnTimer1000");check(std::get<2>(changes.back())==0,"revival releases shield");offline=players[1]->id;changes.clear();eventrun("#AliceControl::OnTimer1000");check(std::get<2>(changes.back())==1,"alive disconnect also shields between death sampling ticks");offline=0;eventrun("#AliceControl::OnTimer1000");for(int i=0;i<30;++i)eventrun("#AliceControl::OnTimer1000");check(live_mobs["#AliceControl::OnCatDead"]>=4,"Cheshire reinforcements repeat");live_mobs["#AliceControl::OnCatDead"]=11;stage("'alice_cat_tick",14);eventrun("#AliceControl::OnTimer1000");check(live_mobs["#AliceControl::OnCatDead"]==12,"eleven cats spawn only one to respect alive cap");}
  int phases=mode==1?1:3;
  for(int phase=1;phase<=phases;++phase){int64 expected=MD_CANMOVE|MD_CANATTACK|MD_AGGRESSIVE;if(mode==2)expected|=phase==1?MD_IGNOREMAGIC|MD_IGNORERANGED|MD_IGNOREMISC:phase==2?MD_IGNOREMELEE|MD_IGNORERANGED|MD_IGNOREMISC:MD_IGNOREMELEE|MD_IGNOREMAGIC|MD_IGNOREMISC;bool found=false;for(const auto& c:changes)if(std::get<1>(c)==UMOB_MODE&&std::get<2>(c)==expected)found=true;/* phase1 mode may precede death probes */if(phase>1||mode==1)check(found,"boss phase permits exactly intended damage channel");std::string e="#AliceControl::OnBossDead"+std::to_string(phase);eventrun(e);check(!stage("'alice_complete"),"live boss blocks fake clear");live_mobs[e]=0;eventrun(e);auto next=stage("'alice_boss_phase");eventrun(e);check(stage("'alice_boss_phase")==next,"duplicate death callback cannot advance twice");drain();}
  check(stage("'alice_complete")==1,"all required phases finish encounter");capacity=false;finish("Alice#mad_reward");check(items[players[0]->id].empty(),"full inventory blocks entire clear reward");capacity=true;finish("Alice#mad_reward");check(items[players[0]->id][1001074]==1,"clear grants exactly one chain");if(mode==2)check(items[players[0]->id][1001082]==2,"fast Hard gives regular plus timed stone");auto loot=items;quest_delete(players[0].get(),62090);clock_now+=86400;finish("Alice#mad_reward");check(items==loot,"daily reset cannot repeat clear reward");
 }
 setup(2);stage("'alice_boss_phase",3);stage("'alice_boss_alive",1);clock_now+=1201;eventrun("#AliceControl::OnTimer1000");check(stage("'alice_bonus_expired")==1,"twenty-minute warning follows reservation time");eventrun("#AliceControl::OnBossDead3");check(!stage("'alice_fast_clear"),"late clear cannot receive timed bonus");iv("'alice_eligible",100,1);finish("Alice#mad_reward");check(items[players[0]->id][1001082]==1,"late Hard still receives regular one-stone reward");finish("Alice#mad_reward",1);check(items[players[1]->id].empty(),"nonparticipant cannot claim clear rewards");
 for(unsigned r=0;r<sizeof(recipes)/sizeof(recipes[0]);++r){auto recipe=recipes[r];
  auto stock=[&](){setup(1);for(const auto& cost:recipe.costs)items[players[0]->id][cost.first]=cost.second;players[0]->status.zeny=recipe.zeny;expected_weight_output=recipe.output;choices={int(r)+1,1};};
  stock();capacity=false;auto before=items;finish("Confused Boy#alice_exchange");check(items==before&&players[0]->status.zeny==recipe.zeny,"full inventory exchange consumes nothing");
  for(const auto& cost:recipe.costs){stock();--items[players[0]->id][cost.first];before=items;finish("Confused Boy#alice_exchange");check(items==before&&players[0]->status.zeny==recipe.zeny,"missing each material rejects atomically");}
  stock();players[0]->status.zeny=recipe.zeny-1;before=items;finish("Confused Boy#alice_exchange");check(items==before&&players[0]->status.zeny==recipe.zeny-1,"insufficient zeny rejects atomically");
  stock();choices={int(r)+1,2};before=items;finish("Confused Boy#alice_exchange");check(items==before&&players[0]->status.zeny==recipe.zeny,"cancel preserves all exchange costs");
  stock();drain_wallet=true;before=items;finish("Confused Boy#alice_exchange");check(items==before&&players[0]->status.zeny==0,"wallet rechecked after final confirmation");
  stock();finish("Confused Boy#alice_exchange");check(items[players[0]->id][recipe.output]==1&&players[0]->status.zeny==0,"recipe grants exact output and charges exact zeny");for(const auto& cost:recipe.costs)check(items[players[0]->id][cost.first]==0,"recipe consumes exact material amount");
 }
 check(errors==0,"no native script or quest errors");for(auto& c:codes)script_free_code(c.second);codes.clear();offline=0;reset();script_free_vars(instances.at(1)->regs.vars);instances.clear();quest_db.clear();players.clear();attached=nullptr;do_final_script();timer_final();db_final();std::printf("ALICE_RESULT checks=%u failures=%u errors=%u\n",checks,failures,errors);malloc_final();return failures||errors?1:0;
}
"""
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--build-dir',type=Path);p.add_argument('--prepare-only',action='store_true');args=p.parse_args()
 prefix='#include <algorithm>\n#include "map/itemdb.hpp"\n'+native.CPP.split('extern "C" int __wrap_main',1)[0]
 prefix=prefix.replace('namespace {','int64 get_val2_num(script_state*,int64,reg_db*);\nnamespace {',1)
 prefix=prefix.replace('int32 world(script_state* st) {',EXTRA+'\nint32 world(script_state* st) {')
 prefix=prefix.replace('    if (command == "instance_mapname"',WORLD,1)
 prefix=prefix.replace('if (p->id == id) return p.get();','if (p->id == id && id!=offline) return p.get();')
 names=['select','is_party_leader','party_changeleader','instance_live_info','instance_create','instance_enter','gettimetick','rand','getpartymember','getmapxy','mobcount','getunitdata','setunitdata','unitexists','killmonster','countitem','delitem','getitemname','checkweight2','dispbottom','setnpctimer','initnpctimer','stopnpctimer']
 prefix=prefix.replace('"getexp", "callfunc"};','"getexp",'+','.join(json.dumps(n) for n in names)+'};')
 native.CPP=prefix+MAIN;native.fixtures=fixtures;native.WRAPPERS+=('_Z13map_charid2sdi','_Z9map_id2bli','_Z11map_nick2sdPKcb','_Z11pc_setparamP16map_session_datall','_Z13mapreg_setregll','_Z14mapreg_readregl')
 def run(d):
  d.mkdir(parents=True,exist_ok=True);native.run(d.resolve(),False,False,args.prepare_only,completion_marker='ALICE_RESULT ')
 if args.build_dir:run(args.build_dir)
 else:
  with tempfile.TemporaryDirectory(prefix='alice-') as d:run(Path(d))
if __name__=='__main__':main()
