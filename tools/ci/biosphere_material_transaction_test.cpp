// GPL-3.0-or-later. Appended to the existing crown fixture's explicitly tracked
// boundary helpers. Actual scripts/input/inventory/quest/achievement execution;
// world indices, packets, persistence and quest monster identity context doubled.
#include "map/npc.hpp"
#include "map/quest.hpp"
#include "map/mob.hpp"
#include <climits>

namespace {
struct Recipe {std::vector<int> ids,needs,choices;int output,price,access;};
std::vector<Recipe> recipes;
std::map<int,std::unique_ptr<npc_data>> nodes;
std::vector<script_code*> conditions;
std::map<script_code*,std::pair<unsigned,unsigned>> condition_hits;
std::vector<int> zeny_log;
std::vector<std::pair<int,int>> debits;
bool waiting_input=false;
unsigned input_count=0,argument_writes=0,qi_packets=0,nested_checks=0;
struct PlayerDelete {void operator()(map_session_data* sd)const {if(sd->quest_log)aFree(sd->quest_log);delete sd;}};
using Player=std::unique_ptr<map_session_data,PlayerDelete>;
int read_int(ryml::NodeRef n){int v;n>>v;return v;}
std::string read_string(ryml::NodeRef n){std::string v;n>>v;return v;}
std::vector<int> ints(ryml::NodeRef n){std::vector<int> v;for(auto x:n)v.push_back(read_int(x));return v;}

Player converter(int recipe,int quantity=1){
    ++cases;errors=0;nums.clear();strings.clear();messages.clear();menu_text.clear();windows.clear();
    closes=unequips=equips=0;zeny_log.clear();debits.clear();input_count=argument_writes=0;waiting_input=false;
    Player sd(new map_session_data());attached=sd.get();
    sd->id=sd->status.account_id=99000001;sd->status.char_id=99000002;sd->type=BL_PC;
    sd->status.base_level=recipes[recipe].access?250:240;sd->status.class_=JOB_ALITEA;sd->class_=MAPID_ALITEA;
    sd->status.zeny=MAX_ZENY;sd->status.inventory_slots=MAX_INVENTORY;
    sd->fd=0;sd->state.ignoretimeout=true;sd->npc_idle_timer=INVALID_TIMER;sd->max_weight=2000000000;
    for(auto& i:sd->equip_index)i=-1;
    sd->m=0;setnum("ep17_2_main",recipes[recipe].access?33:0);setnum("RepPoints6",-100);setnum("RepPoints9",-100);
    int index=0;
    for(size_t m=0;m<recipes[recipe].ids.size();++m){
        int left=recipes[recipe].needs[m]*quantity,stack=0;
        while(left){int n=std::min(left,30000);check(index<MAX_INVENTORY,"fixture fits physical inventory");put(index,recipes[recipe].ids[m],n);
            if(stack++)sd->inventory.u.items_inventory[index].unique_id=1000+index;
            ++index;left-=n;}
    }
    weight();pc_show_questinfo_reinit(sd.get());check(sd->qi_display.size()==19,"real QuestInfo display initialization");return sd;
}
int total(const Snapshot& old,int id){int sum=0;for(const auto& i:old.inventory.u.items_inventory)if(i.nameid==id)sum+=i.amount;return sum;}
bool success_text(){for(const auto& s:messages)if(s.find("Conversion complete.")==0||s.find("Magical water conversion complete.")==0||s.find("Fusion complete.")==0)return true;return false;}
void flow(script_code* code,int recipe,int quantity,const std::function<void()>& hook={},bool cancel=false,bool old=false,const std::vector<int>& override_choices={}){
    const auto& choices=override_choices.empty()?recipes[recipe].choices:override_choices;
    run_script(code,0,attached->id,NPC);int answered=0,pauses=0;
    while(attached->st){check(++pauses<30,"material dialogue bounded termination");auto* st=attached->st;
        if(st->state==RERUNLINE){
            if(waiting_input){if(hook)hook();waiting_input=false;
                if(cancel){pc_close_npc(attached,2);continue;}attached->npc_amount=quantity;
            }else{check(answered<static_cast<int>(choices.size()),"answer exists for actual source menu");attached->npc_menu=choices[answered++];}
        }else if(st->state==CLOSE)st->state=END;
        else check(st->state==STOP,"known native input/menu/Next suspension");
        run_script_main(st);
    }
    if(!old)check(errors==0,"candidate has no native errors");
    check(!attached->state.menu_or_input,"input/menu wait flag cleared");
    check(windows.empty()&&equips==0&&unequips==0,"material path never enters equipment service");
}
void refused(const Snapshot& before){before.unchanged();check(zeny_log.empty()&&debits.empty(),"refusal never begins payment");check(!success_text(),"refusal never emits success");}
void committed(const Snapshot& before,int recipe,int quantity){
    const auto& r=recipes[recipe];const int cost=r.price*quantity;
    check(attached->status.zeny==before.zeny-cost&&zeny_log==std::vector<int>{-cost},"one exact native Zeny debit");
    check(count(r.output)==total(before,r.output)+quantity,"exact output quantity");
    int removed_weight=0;std::vector<std::pair<int,int>> grouped;
    for(auto [id,n]:debits){check(n>0&&n<=30000,"every actual item debit fits native amount");
        if(!grouped.empty()&&grouped.back().first==id)grouped.back().second+=n;else grouped.emplace_back(id,n);}
    check(grouped.size()==r.ids.size(),"material debit groups unchanged");
    for(size_t m=0;m<r.ids.size();++m){
        check(count(r.ids[m])==total(before,r.ids[m])-r.needs[m]*quantity,"exact complete material debit");
        check(grouped[m]==std::make_pair(r.ids[m],r.needs[m]*quantity),"original debit order and sums");
        removed_weight+=item_db.find(r.ids[m])->weight*r.needs[m]*quantity;
    }
    check(attached->weight==before.weight-removed_weight+item_db.find(r.output)->weight*quantity,"actual mixed material net weight");
    for(int i=0;i<MAX_INVENTORY;++i){const auto& old=before.inventory.u.items_inventory[i];const auto& now=attached->inventory.u.items_inventory[i];
        const bool material=std::find(r.ids.begin(),r.ids.end(),old.nameid)!=r.ids.end();
        if(now.nameid==old.nameid&&now.nameid){item expected=old;expected.amount=now.amount;
            check(!std::memcmp(&expected,&now,sizeof(item)),"remaining metadata byte-identical");
            check(material||old.nameid==r.output||old.amount==now.amount,"unrelated amounts identical");
        }else if(now.nameid==r.output){check(material||!old.nameid,"output only uses free or consumed-input cell");
            item expected{};expected.nameid=r.output;expected.amount=quantity;expected.identify=1;
            check(!std::memcmp(&expected,&now,sizeof(item)),"new plain output never inherits consumed metadata");
        }else if(!now.nameid)check(material||!old.nameid,"only input cells removed");
        else check(!std::memcmp(&old,&now,sizeof(item)),"unrelated inventory byte-identical");
        check(now.nameid?(attached->inventory_data[i]&&attached->inventory_data[i]->nameid==now.nameid):!attached->inventory_data[i],"native inventory data pointers coherent");
    }
    check(nums[add_str("ep17_2_main")]==(r.access?33:0)&&nums[add_str("RepPoints6")]==-100&&nums[add_str("RepPoints9")]==-100,"original access, no story/reputation charge");
    check(argument_writes==2&&nums[add_str("ARG0")]==0&&attached->achievement_data.count==0,"seven actual sell-zero achievements complete none");
    check(success_text(),"successful commit reaches original confirmation");
}
void output_stack(int i,int recipe,int amount,int variant){
    put(i,recipes[recipe].output,amount);auto& it=attached->inventory.u.items_inventory[i];
    if(variant==1)it.bound=BOUND_CHAR;else if(variant==2)it.unique_id=UINT64_MAX;
    else if(variant==3)it.card[2]=4700;else if(variant==4)it.expire_time=2100000000;
    else if(variant==5){it.id=123;it.identify=0;it.refine=12;it.attribute=1;it.enchantgrade=3;it.favorite=1;it.option[0].id=1;it.option[0].value=23;}
    weight();
}
void clear_player_quests(){if(attached->quest_log)aFree(attached->quest_log);attached->quest_log=nullptr;attached->num_quests=attached->avail_quests=0;}
void apply_probe(ryml::NodeRef probe){
    clear_player_quests();auto qs=probe["Quests"];int n=static_cast<int>(qs.num_children());
    if(n)attached->quest_log=(struct quest*)aCalloc(n,sizeof(struct quest));
    attached->num_quests=n;int index=0;
    for(auto row:qs){auto& q=attached->quest_log[index++];q.quest_id=read_int(row["Id"]);q.state=static_cast<e_quest_state>(read_int(row["State"]));
        bool expired,complete;row["Expired"]>>expired;row["CompleteCounts"]>>complete;
        q.time=static_cast<uint32>(time(nullptr)+(expired?-1000:86400));
        if(complete){auto definition=quest_db.find(q.quest_id);check(definition!=nullptr,"actual hunting quest exists");
            for(size_t i=0;i<definition->objectives.size();++i)q.count[i]=definition->objectives[i]->count;}
    }
    for(auto row:probe["Variables"]){auto name=read_string(row["Name"]);int v=read_int(row["Value"]);
        if(name=="BaseLevel")attached->status.base_level=v;else setnum(name.c_str(),v);}
    int slot=MAX_INVENTORY-1;for(auto row:probe["Items"]){put(slot--,read_int(row["Id"]),read_int(row["Amount"]));}weight();
}
}

extern "C" npc_data* npc_lookup(int32 id){auto it=nodes.find(id);return it==nodes.end()?nullptr:it->second.get();}
extern "C" block_list* material_world(int32) asm("__wrap__Z9map_id2bli");
extern "C" block_list* material_world(int32 id){if(attached&&attached->id==id)return attached;return npc_lookup(id);}
extern "C" void crown_log(const map_session_data* sd,e_log_pick_type type,int32 amount,const item* it){
    check(sd==attached&&type==LOG_TYPE_SCRIPT,"actual item log source");if(amount<0)debits.emplace_back(it->nameid,-amount);
}
extern "C" void material_input(map_session_data&,uint32) asm("__wrap__Z16clif_scriptinputR16map_session_dataj");
extern "C" void material_input(map_session_data&,uint32){waiting_input=true;++input_count;}
extern "C" void material_zeny(const map_session_data&,e_log_pick_type,uint32,int32) asm("__wrap__Z8log_zenyRK16map_session_data15e_log_pick_typeji");
extern "C" void material_zeny(const map_session_data& sd,e_log_pick_type type,uint32 id,int32 amount){check(&sd==attached&&type==LOG_TYPE_SCRIPT&&id==sd.status.char_id,"native Zeny log context");zeny_log.push_back(amount);}
extern "C" bool material_registry(map_session_data*,int64,int64) asm("__wrap__Z14pc_setregistryP16map_session_datall");
extern "C" bool material_registry(map_session_data* sd,int64 key,int64 value){check(sd==attached&&std::strcmp(get_str(script_getvarid(key)),"ARG0")==0,"only transient achievement ARG0 persistence");nums[key]=value;++argument_writes;return true;}
extern "C" void qi_packet(const map_session_data*,const block_list*,e_questinfo_types,e_questinfo_markcolor) asm("__wrap__Z21clif_quest_show_eventPK16map_session_dataPK10block_list17e_questinfo_types21e_questinfo_markcolor");
extern "C" void qi_packet(const map_session_data* sd,const block_list* nd,e_questinfo_types,e_questinfo_markcolor){check(sd==attached&&nodes.count(nd->id),"actual quest marker packet destination");++qi_packets;}
extern "C" bool real_condition(script_code*,map_session_data*) asm("__real__Z27achievement_check_conditionP11script_codeP16map_session_data");
extern "C" bool checked_condition(script_code*,map_session_data*) asm("__wrap__Z27achievement_check_conditionP11script_codeP16map_session_data");
extern "C" bool checked_condition(script_code* code,map_session_data* sd){
    check(sd==attached,"native callback attached player");auto* old=sd->st;int rid=old?old->rid:0;Snapshot before;
    std::vector<struct quest> oldquests;if(sd->num_quests)oldquests.assign(sd->quest_log,sd->quest_log+sd->num_quests);
    bool value=real_condition(code,sd);before.unchanged();check(sd->st==old&&(!old||old->rid==rid),"actual nested VM restores caller state and RID");
    check(sd->num_quests==static_cast<int>(oldquests.size())&&(!sd->num_quests||!std::memcmp(sd->quest_log,oldquests.data(),oldquests.size()*sizeof(struct quest))),"native condition never changes quest state");
    if(condition_hits.count(code)){auto& hit=condition_hits[code];(value?hit.second:hit.first)++;}++nested_checks;return value;
}

extern "C" int __wrap_main(int argc,char** argv){
    check(argc==3,"explicit build path and mode");bool original=std::string(argv[2])=="original";
    check(original||std::string(argv[2])=="candidate","known native mode");deny_network();
    static char server[]="biosphere-material-transaction-test";SERVER_NAME=server;
    malloc_init();db_init();do_init_database();timer_init();do_init_script();battle_set_defaults();
    battle_config.atcommand_disable_npc=0;battle_config.feature_achievement=1;
    std::strcpy(::map[0].name,"ba_in01");std::strcpy(::map[1].name,"prontera");
    auto fake=std::make_unique<npc_data>();fake->id=NPC;fake->type=BL_NPC;fake->m=0;fake_nd=fake.get();nodes.emplace(NPC,std::move(fake));
    const std::string dir=argv[1];
    auto text=read(dir+"/recipes.yml");auto recipe_rows=ryml::parse_in_arena(ryml::to_csubstr(text));
    for(auto row:recipe_rows["Body"])recipes.push_back({ints(row["Materials"]),ints(row["Needs"]),ints(row["Choices"]),read_int(row["Output"]),read_int(row["Price"]),read_int(row["Access"])});
    check(recipes.size()==41,"all 41 native menu routes");
    text=read(dir+"/items.yml");auto item_rows=ryml::parse_in_arena(ryml::to_csubstr(text));
    for(auto row:item_rows["Body"])check(item_db.parseBodyNode(row)==1,"actual material/ancillary item definition parses");
    for(const auto& r:recipes){auto data=item_db.find(r.output);check(data&&data->type==IT_ETC&&(data->weight==1||data->weight==10)&&!data->value_sell&&!data->flag.guid&&!data->flag.autoequip&&!data->stack.inventory,"native plain output invariants");}
    text=read(dir+"/quest-mob-identities.yml");auto mob_rows=ryml::parse_in_arena(ryml::to_csubstr(text));
    for(auto row:mob_rows["Body"]){auto mob=std::make_shared<s_mob_db>();mob->id=read_int(row["Id"]);mob->sprite=read_string(row["AegisName"]);mob_db.put(mob->id,mob);}
    text=read(dir+"/quests.yml");auto quest_rows=ryml::parse_in_arena(ryml::to_csubstr(text));
    for(auto row:quest_rows["Body"])check(quest_db.parseBodyNode(row)==1,"actual quest definition and ordered overrides parse");
    check(quest_db.size()==36,"36 effective native quest dependencies");
    text=read(dir+"/achievements.yml");auto ach_rows=ryml::parse_in_arena(ryml::to_csubstr(text));
    for(auto row:ach_rows["Body"])check(achievement_db.parseBodyNode(row)==1,"actual Get_Item condition parses");
    check(achievement_db.size()==7,"seven actual native achievement conditions");
    text=read(dir+"/questinfo.yml");auto qi_rows=ryml::parse_in_arena(ryml::to_csubstr(text));int nodeid=NPC+1;
    for(auto row:qi_rows["Body"]){auto node=std::make_unique<npc_data>();node->id=nodeid;node->type=BL_NPC;node->m=0;
        nodes.emplace(nodeid,std::move(node));auto* registration=compile(read_string(row["Script"]),"actual-ba-in01-questinfo-statements");
        run_script(registration,0,0,nodeid);check(errors==0,"actual QuestInfo builtin registration succeeds");script_free_code(registration);
        auto* nd=nodes[nodeid++].get();check(nd->qi_data.size()==row["Conditions"].num_children(),"exact conditions per original NPC owner");
        for(auto& qi:nd->qi_data){conditions.push_back(qi->condition);condition_hits[qi->condition]={0,0};}
    }
    check(::map[0].qi_npc.size()==19&&conditions.size()==33,"nonempty actual 19-NPC/33-condition ba_in01 registry");
    const auto quests=read(dir+(original?"/quests-before.txt":"/quests-after.txt"));
    const auto depth=read(dir+(original?"/depth-before.txt":"/depth-after.txt"));
    auto* omega_access=compile(body(read(dir+"/access.txt"),"function\tscript\tF_BiosphereAccess"),"actual-Omega-access");
    auto* ellie_access=compile(body(depth,"function\tscript\tF_BioDepthQuestAccess"),"actual-Ellie-access");
    strdb_put(script_get_userfunc_db(),"F_BiosphereAccess",omega_access);strdb_put(script_get_userfunc_db(),"F_BioDepthQuestAccess",ellie_access);
    if(!original){auto* helper=compile(body(quests,"function\tscript\tF_BiosphereMaterialCommit"),"actual-material-helper");strdb_put(script_get_userfunc_db(),"F_BiosphereMaterialCommit",helper);}
    auto* omega=compile(body(quests,"Omega#biosphere_materials"),original?"pinned-original-Omega":"actual-Omega");
    auto* ellie=compile(body(depth,"Ellie#bio_d1_fusion"),original?"pinned-original-Ellie":"actual-Ellie");
    auto npc_for=[&](int r){return recipes[r].access?ellie:omega;};
    if(original){
        for(int r=0;r<41;++r){auto sd=converter(r);flow(npc_for(r),r,1,[&](){sd->status.zeny=recipes[r].price-1;},false,true);
            check(errors==1&&zeny_log.empty(),"original stale-Zeny failure exactly once");
            for(int id:recipes[r].ids)check(!count(id),"original consumes materials before rejected Zeny");
            check(!count(recipes[r].output)&&sd->status.zeny==recipes[r].price-1,"no free output from stale Zeny");}
        for(int r=14;r<17;++r)for(int missing=1;missing<5;++missing){auto sd=converter(r);
            flow(npc_for(r),r,1,[&](){--sd->inventory.u.items_inventory[missing].amount;weight();},false,true);
            check(errors==1&&zeny_log.empty(),"original later-material error");for(int m=0;m<missing;++m)check(!count(recipes[r].ids[m]),"original prior ingredients lost");
            check(!count(recipes[r].output),"original incomplete recipe creates no output");}
        for(int r=0;r<41;++r){int q=recipes[r].needs[0]==10?3277:6554;auto sd=converter(r,q);Snapshot before;flow(npc_for(r),r,q,{},false,true);
            check(errors==0&&count(recipes[r].output)==q,"original signed narrowing still creates output");
            for(int id:recipes[r].ids)check(count(id)==total(before,id),"original negative narrowed debit consumes nothing");}
        for(int r:{0,14,17}){auto sd=converter(r,2);int n=recipes[r].ids.size();output_stack(n,r,20,1);sd->status.inventory_slots=n+1;Snapshot before;
            flow(npc_for(r),r,1,{},false,true);check(errors==1&&sd->status.zeny==before.zeny-recipes[r].price,"original pays before incompatible output failure");
            check(count(recipes[r].output)==20,"original failure creates no plain output");}
        for(int r:{0,14,17}){auto sd=converter(r);Snapshot before;flow(npc_for(r),r,0,{},false,true);check(errors==0&&count(recipes[r].output)==1,"original invalid zero silently buys one");}
    }else{
        for(int r=0;r<41;++r)for(int q:{1,2,3000,3001,3276,3277,6000,6001,6553,6554,10000,21474,30000}){
            if(int64(q)*recipes[r].price>MAX_ZENY)continue;auto sd=converter(r,q);Snapshot before;flow(npc_for(r),r,q);check(input_count==1,"actual numeric input executed");committed(before,r,q);}
        for(int r=0;r<41;++r)for(int q:{0,-1,2,INT32_MAX}){auto sd=converter(r);Snapshot before;flow(npc_for(r),r,q);refused(before);}
        for(int r=0;r<41;++r){int q=recipes[r].price==100000?21475:30001;auto sd=converter(r,q);Snapshot before;flow(npc_for(r),r,q);refused(before);}
        for(int r=0;r<41;++r){auto sd=converter(r);std::unique_ptr<Snapshot> changed;flow(npc_for(r),r,1,[&](){sd->status.zeny=recipes[r].price-1;changed=std::make_unique<Snapshot>();});refused(*changed);}
        for(int r=0;r<41;++r)for(size_t m=0;m<recipes[r].ids.size();++m){auto sd=converter(r,6554);std::unique_ptr<Snapshot> changed;
            flow(npc_for(r),r,6554,[&](){int i=pc_search_inventory(sd.get(),recipes[r].ids[m]);--sd->inventory.u.items_inventory[i].amount;weight();changed=std::make_unique<Snapshot>();});refused(*changed);}
        for(int r=0;r<41;++r)for(int what=0;what<(recipes[r].access?3:2);++what){auto sd=converter(r);Snapshot before;
            flow(npc_for(r),r,1,[&](){if(what==0)sd->status.base_level=recipes[r].access?249:239;else if(what==1)sd->m=1;else setnum("ep17_2_main",32);});refused(before);}
        for(int r=0;r<41;++r){auto sd=converter(r);Snapshot before;flow(npc_for(r),r,1,{},true);refused(before);}
        for(int r=0;r<41;++r){auto sd=converter(r);Snapshot before;flow(npc_for(r),r,1,[&](){sd->max_weight=sd->weight+item_db.find(recipes[r].output)->weight-1;});refused(before);}
        for(int r=0;r<41;++r){auto sd=converter(r);sd->max_weight=sd->weight+item_db.find(recipes[r].output)->weight;Snapshot before;flow(npc_for(r),r,1);committed(before,r,1);}
        for(int r=0;r<41;++r)for(int capacity:{0,MAX_INVENTORY+1}){auto sd=converter(r);Snapshot before;flow(npc_for(r),r,1,[&](){sd->status.inventory_slots=capacity;});refused(before);}
        for(int r=0;r<41;++r){auto sd=converter(r);Snapshot before;flow(npc_for(r),r,1,[&](){sd->status.inventory_slots=recipes[r].ids.size();});refused(before);}
        for(int r:{0,1,14,15,16,17,18,19})for(int variant=1;variant<=5;++variant)for(int room=0;room<2;++room){auto sd=converter(r,2);int n=recipes[r].ids.size();output_stack(n,r,20,variant);sd->status.inventory_slots=n+1+room;Snapshot before;flow(npc_for(r),r,1);
            if(room||variant==5)committed(before,r,1);else refused(before);}
        for(int r=0;r<41;++r){auto sd=converter(r,2);int n=recipes[r].ids.size();output_stack(n,r,30000,1);output_stack(n+1,r,100,0);sd->status.inventory_slots=n+2;Snapshot before;flow(npc_for(r),r,1);committed(before,r,1);}
        for(int r=0;r<41;++r){auto sd=converter(r,2);int n=recipes[r].ids.size();output_stack(n,r,30000,0);output_stack(n+1,r,100,0);Snapshot before;flow(npc_for(r),r,1);refused(before);}
        for(int r=0;r<41;++r){auto sd=converter(r,2);int n=recipes[r].ids.size();output_stack(n+2,r,100,0);sd->status.inventory_slots=n+1;Snapshot before;flow(npc_for(r),r,1);refused(before);}
        for(int r=0;r<41;++r){auto sd=converter(r,2);int n=recipes[r].ids.size();output_stack(n+2,r,100,1);sd->status.inventory_slots=n+1;Snapshot before;flow(npc_for(r),r,1);committed(before,r,1);}
        for(int r=0;r<41;++r){auto sd=converter(r,2);for(size_t i=0;i<recipes[r].ids.size();++i)sd->inventory.u.items_inventory[i].bound=BOUND_CHAR;Snapshot before;flow(npc_for(r),r,1);committed(before,r,1);}
        for(const std::vector<int> choices:std::vector<std::vector<int>>{{3},{1,8},{1,1,3},{2,4},{255}}){auto sd=converter(0);Snapshot before;flow(omega,0,1,{},false,false,choices);refused(before);check(!input_count,"Omega menu cancellation avoids numeric input");}
        for(const std::vector<int> choices:std::vector<std::vector<int>>{{9},{1,4},{255}}){auto sd=converter(17);Snapshot before;flow(ellie,17,1,{},false,false,choices);refused(before);check(!input_count,"Ellie menu cancellation avoids numeric input");}
        // Source-derived exact condition TRUE states and independent all-false
        // states; direct real callbacks plus full native map traversal.
        text=read(dir+"/qi-probes.yml");auto probes=ryml::parse_in_arena(ryml::to_csubstr(text));size_t i=0;
        for(auto probe:probes["Body"]){auto sd=converter(0);sd->status.base_level=1;setnum("ep17_2_main",0);Snapshot before;
            check(!checked_condition(conditions[i],sd.get()),"actual condition false under absent-quest baseline");before.unchanged();
            apply_probe(probe);Snapshot ready;check(checked_condition(conditions[i],sd.get()),"source-derived state reaches actual condition true");ready.unchanged();pc_show_questinfo(sd.get());++i;}
        check(i==33,"all 33 original condition true/false probes");
        for(auto [code,hits]:condition_hits)check(hits.first&&hits.second,"every native QuestInfo condition saw both outcomes");
        {auto sd=converter(0);sd->qi_display.pop_back();unsigned before=nested_checks;pc_show_questinfo(sd.get());check(nested_checks==before,"wrong display size genuinely skips native loop");
            pc_show_questinfo_reinit(sd.get());check(nested_checks==before&&sd->qi_display.size()==19,"actual reinit initializes display without executing conditions");
            pc_show_questinfo(sd.get());check(nested_checks>before,"initialized display permits actual callback execution");}
        // Reject malformed direct helper arguments without invoking any charge.
        for(const char* args:{"", "1,1000640,20000,2,1,1000636,10", "0,1000640,20000,0,1,1000636,10", "30001,1000640,20000,0,1,1000636,10",
                "1,0,20000,0,1,1000636,10", "1,2147483648,20000,0,1,1000636,10", "1,1000640,1,0,1,1000636,10", "1,1000640,20000,0,2,1000636,10",
                "1,1000640,20000,0,1,0,10", "1,1000640,20000,0,1,1000640,10", "1,1000640,20000,0,1,1000636,0", "1,1000640,20000,0,1,1000636,11",
                "1,1001186,20000,0,5,1001138,5,1001138,5,1001140,5,1001141,5,1001185,5"}){
            auto sd=converter(0);Snapshot before;std::string source="{ @HelperResult=callfunc(\"F_BiosphereMaterialCommit\"";if(*args)source+=","+std::string(args);source+="); end; }";
            auto* probe=compile(source,"actual-material-helper-invalid-arguments");run_script(probe,0,sd->id,NPC);check(!sd->st&&errors==0&&nums[add_str("@HelperResult")]==0,"helper rejects invalid domain");refused(before);script_free_code(probe);}
    }
    attached=nullptr;script_free_code(omega);script_free_code(ellie);conditions.clear();condition_hits.clear();
    ::map[0].qi_npc.clear();nodes.clear();fake_nd=nullptr;quest_db.clear();mob_db.clear();item_db.clear();achievement_db.clear();
    nums.clear();strings.clear();do_final_script();timer_final();db_final();malloc_final();
    std::printf(original?"MATERIAL_OLD_FAILURES_OK cases=%u assertions=%u nested_checks=%u\n":"MATERIAL_NATIVE_OK cases=%u assertions=%u nested_checks=%u\n",cases,assertions,nested_checks);
    return 0;
}
