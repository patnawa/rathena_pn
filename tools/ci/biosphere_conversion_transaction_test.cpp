// ============================================================================
//  PN  /  DEVELOPMENT TOOLS
//  biosphere_conversion_transaction_test.cpp
// ----------------------------------------------------------------------------
//  Project contributions: (C) 2026 PN Development Team
//  License for project contributions: GPL-3.0-or-later; see LICENSE.
//  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/biosphere_conversion_transaction_test.cpp
//  Existing upstream authors, notices and other rights are retained.
// ============================================================================

// GPL-3.0-or-later. Appended to the existing native crown fixture's reviewed
// boundary helpers. This driver executes the exact complete NPC conversion path.
#include "map/npc.hpp"
extern int buildin_getinventoryslots(script_state*);
extern script_data* push_val2(script_stack*,c_op,int64,reg_db*);

namespace {
struct Recipe { std::vector<int> ids, needs; int price, output; };
const Recipe recipes[]={{{1001550},{10},10000,1001552},{{1001551},{10},10000,1001553},
    {{1001552,1001553},{10,10},20000,1001554},{{1001554,6607},{5,5},30000,1001555},
    {{1001555,6608,6755,25866},{5,5,5,3},50000,1001556}};
bool waiting_input=false;
int input_count=0;
unsigned argument_writes=0;
std::vector<int> zeny_log;

void getter_probe(script_code* code,int rid,int character,int expected,bool failure){
    auto* st=script_alloc_state(code,0,rid,0);st->start=0;st->end=0;
    if(character>=0){
        push_val2(st->stack,C_NOP,0,nullptr);push_val2(st->stack,C_NOP,0,nullptr);
        push_val2(st->stack,C_INT,character,nullptr);st->end=3;
    }
    int previous_errors=errors;int result=buildin_getinventoryslots(st);
    check(result==(failure?SCRIPT_CMD_FAILURE:SCRIPT_CMD_SUCCESS),"actual getter native return status");
    const auto* returned=script_getdatatop(st,-1);
    check(returned->type==C_INT&&returned->u.num==expected,"actual getter native pushed value");
    check(errors==previous_errors+(failure?1:0),"exact getter diagnostic count");
    if(failure&&character<0)check(st->state==END,"unattached getter ends native script");
    script_free_state(st);
}

std::unique_ptr<map_session_data> converter(int recipe,int quantity=1) {
    ++cases;errors=0;nums.clear();strings.clear();messages.clear();menu_text.clear();windows.clear();
    closes=unequips=equips=0;zeny_log.clear();input_count=0;waiting_input=false;argument_writes=0;
    auto sd=std::make_unique<map_session_data>();attached=sd.get();
    sd->id=sd->status.account_id=99000001;sd->status.char_id=99000002;sd->type=BL_PC;
    sd->status.base_level=250;sd->status.class_=JOB_ALITEA;sd->class_=MAPID_ALITEA;
    sd->status.zeny=2000000000;sd->status.inventory_slots=MAX_INVENTORY;
    sd->fd=0;sd->state.ignoretimeout=true;sd->npc_idle_timer=INVALID_TIMER;sd->max_weight=2000000000;
    for(auto& index:sd->equip_index)index=-1;
    sd->m=0;setnum("ep17_2_main",33);setnum("RepPoints6",2000);setnum("RepPoints9",-100);
    int index=0;
    for(size_t m=0;m<recipes[recipe].ids.size();++m){
        int left=recipes[recipe].needs[m]*quantity,stack=0;
        while(left){int amount=std::min(left,30000);put(index,recipes[recipe].ids[m],amount);
            // Distinct valid native UID stack keys model aggregated inventory.
            // This does not claim ordinary acquisition of very large batches.
            if(stack++)sd->inventory.u.items_inventory[index].unique_id=1000+index;
            ++index;left-=amount;}
    }
    weight();return sd;
}
int total(const Snapshot& before,int id){int sum=0;for(const auto& it:before.inventory.u.items_inventory)if(it.nameid==id)sum+=it.amount;return sum;}
void flow(script_code* code,int recipe,int quantity,
          const std::function<void()>& at_input={},bool timeout=false,bool original=false){
    run_script(code,0,attached->id,NPC);int answered=0,pauses=0;
    while(attached->st){check(++pauses<25,"conversion dialogue terminates");auto* st=attached->st;
        if(st->state==RERUNLINE){
            if(waiting_input){if(at_input)at_input();waiting_input=false;
                // Execute the real forced-close cleanup, without pretending
                // this fixture drives the secure timer or a client packet.
                if(timeout){pc_close_npc(attached,2);continue;}
                else attached->npc_amount=quantity;
            }else{attached->npc_menu=answered++==0?1:recipe+1;}
        }else if(st->state==CLOSE)st->state=END;
        else check(st->state==STOP,"known actual input/menu/Next suspension");
        run_script_main(st);
    }
    if(!original)check(errors==0,"candidate has no native script errors");
    check(windows.empty()&&unequips==0&&equips==0,"conversion never opens enchant or equipment services");
    check(!attached->state.menu_or_input,"input wait flag cleared");
}
void committed(const Snapshot& before,int recipe,int quantity){
    const auto& r=recipes[recipe];
    check(attached->status.zeny==before.zeny-r.price*quantity,"exact total Zeny charged");
    check(zeny_log==std::vector<int>{-r.price*quantity},"one exact native Zeny log");
    check(count(r.output)==total(before,r.output)+quantity,"exact output total");
    int input_weight=0;
    for(size_t i=0;i<r.ids.size();++i){check(count(r.ids[i])==total(before,r.ids[i])-r.needs[i]*quantity,"exact aggregate input debit including all chunks");input_weight+=r.needs[i]*quantity*10;}
    check(attached->weight==before.weight-input_weight+quantity*10,"exact net native inventory weight");
    for(int i=0;i<MAX_INVENTORY;++i){const auto& old=before.inventory.u.items_inventory[i];const auto& now=attached->inventory.u.items_inventory[i];
        bool material=std::find(r.ids.begin(),r.ids.end(),old.nameid)!=r.ids.end();
        if(now.nameid==old.nameid&&now.nameid){item expected=old;expected.amount=now.amount;
            check(std::memcmp(&expected,&now,sizeof(item))==0,"all retained stack metadata byte-preserved");
            check(material||old.nameid==r.output||old.amount==now.amount,"unrelated stack amount unchanged");
        }else if(now.nameid==r.output){check(material||old.nameid==0,"new output replaces only free/consumed-input cell");
            item expected{};expected.nameid=r.output;expected.amount=quantity;expected.identify=1;
            check(std::memcmp(&expected,&now,sizeof(item))==0,"new output has exact plain getitem metadata");
        }else if(now.nameid==0){check(material||old.nameid==0,"only input cells can be cleared");}
        else check(std::memcmp(&old,&now,sizeof(item))==0,"all unrelated inventory byte-preserved");
        check(now.nameid?(attached->inventory_data[i]&&attached->inventory_data[i]->nameid==now.nameid):attached->inventory_data[i]==nullptr,"inventory data pointer remains consistent");
    }
    check(nums[add_str("RepPoints6")]==2000&&nums[add_str("RepPoints9")]==-100&&nums[add_str("ep17_2_main")]==33,"no reputation/story consumption or new Depth2 gate");
    check(argument_writes==2&&nums[add_str("ARG0")]==0,"actual AG_GET_ITEM argument setup and cleanup");
    check(attached->achievement_data.count==0,"sell-zero output completes no achievement");
}
void set_output(int index,int recipe,int amount,int variant){
    put(index,recipes[recipe].output,amount);auto& it=attached->inventory.u.items_inventory[index];
    if(variant==1)it.bound=2;else if(variant==2)it.unique_id=UINT64_MAX;
    else if(variant==3)it.card[2]=4700;else if(variant==4)it.expire_time=2100000000;
    else if(variant==5){it.id=123;it.identify=0;it.refine=12;it.attribute=1;it.enchantgrade=3;it.favorite=1;it.option[0].id=1;it.option[0].value=23;}
    weight();
}
}
// As with the existing map_id2sd/map_id2nd fixture boundaries, there is no
// global world ID database. Native error reporting may look up its NPC source.
extern "C" block_list* conversion_world(int32) asm("__wrap__Z9map_id2bli");
extern "C" block_list* conversion_world(int32 id){return attached&&attached->id==id?attached:nullptr;}
extern "C" map_session_data* conversion_character(int32) asm("__wrap__Z13map_charid2sdi");
extern "C" map_session_data* conversion_character(int32 id){return attached&&attached->status.char_id==id?attached:nullptr;}
extern "C" void conversion_input(map_session_data&,uint32) asm("__wrap__Z16clif_scriptinputR16map_session_dataj");
extern "C" void conversion_input(map_session_data&,uint32){waiting_input=true;++input_count;}
extern "C" void conversion_zeny(const map_session_data&,e_log_pick_type,uint32,int32) asm("__wrap__Z8log_zenyRK16map_session_data15e_log_pick_typeji");
extern "C" void conversion_zeny(const map_session_data& sd,e_log_pick_type type,uint32 id,int32 amount){check(&sd==attached&&type==LOG_TYPE_SCRIPT&&id==sd.status.char_id,"actual script Zeny log metadata");zeny_log.push_back(amount);}
extern "C" bool conversion_registry(map_session_data*,int64,int64) asm("__wrap__Z14pc_setregistryP16map_session_datall");
extern "C" bool conversion_registry(map_session_data* sd,int64 key,int64 value){check(sd==attached&&std::strcmp(get_str(script_getvarid(key)),"ARG0")==0,"only actual achievement transient ARG0 persistence");nums[key]=value;++argument_writes;return true;}

extern "C" int __wrap_main(int argc,char** argv){
    check(argc==3,"explicit artifact directory and mode");const bool original=std::string(argv[2])=="original";
    const bool getter_missing=std::string(argv[2])=="getter-missing";
    check(original||getter_missing||std::string(argv[2])=="candidate","known isolated mode");deny_network();
    static char server[]="biosphere-conversion-transaction-test";SERVER_NAME=server;
    malloc_init();db_init();do_init_database();timer_init();do_init_script();battle_set_defaults();
    battle_config.atcommand_disable_npc=0;battle_config.feature_achievement=1;
    std::strcpy(::map[0].name,"ba_chess");std::strcpy(::map[1].name,"prontera");
    check(::map[0].qi_npc.empty(),"native ba_chess QuestInfo loop is empty");
    auto fake=std::make_unique<npc_data>();fake->id=NPC;fake_nd=fake.get();
    const std::string dir=argv[1];
    auto text=read(dir+"/items.yml");auto item_rows=ryml::parse_in_arena(ryml::to_csubstr(text));
    for(auto row:item_rows["Body"])check(item_db.parseBodyNode(row)==1,"actual material record parses");
    for(const auto& r:recipes){auto data=item_db.find(r.output);check(data&&data->type==IT_ETC&&data->weight==10&&data->value_sell==0&&!data->flag.guid&&!data->flag.autoequip&&!data->stack.inventory,"actual plain output invariant");}
    text=read(dir+"/reputation.yml");auto rep_rows=ryml::parse_in_arena(ryml::to_csubstr(text));
    for(auto row:rep_rows["Body"])check(reputation_db.parseBodyNode(row)==1,"actual reputation parses");
    text=read(dir+"/achievements.yml");auto achievement_rows=ryml::parse_in_arena(ryml::to_csubstr(text));
    for(auto row:achievement_rows["Body"])check(achievement_db.parseBodyNode(row)==1,"actual AG_GET_ITEM definition parses");
    check(achievement_db.size()==7,"seven actual native output conditions loaded");
    const auto source=read(dir+(original?"/before.txt":"/after.txt"));
    auto* access=compile(body(source,"F_BioDepthQuestAccess"),"actual-conversion-access");
    strdb_put(script_get_userfunc_db(),"F_BioDepthQuestAccess",access);
    auto* npc=compile(body(source,"Abyss Researcher#bio_d2"),original?"pinned-original-conversion":"actual-conversion-NPC");
    if(getter_missing){
        auto* probe=compile("{ end; }","actual-getter-direct-probe");
        {auto sd=converter(0);Snapshot before;
            getter_probe(probe,sd->id,99009999,-1,true);before.unchanged();}
        attached=nullptr;++cases;errors=0;
        getter_probe(probe,0,-1,-1,true);script_free_code(probe);
    }else if(original){
        for(int r=0;r<5;++r){auto sd=converter(r);flow(npc,r,1,[&](){sd->status.zeny=recipes[r].price-1;},false,true);
            check(errors==1&&zeny_log.empty(),"original stale-Zeny fails instead of granting free output");
            for(int id:recipes[r].ids)check(count(id)==0,"original consumed inputs before rejected Zeny");
            check(count(recipes[r].output)==0&&sd->status.zeny==recipes[r].price-1,"original partial payment output/Zeny postconditions");}
        for(int r=2;r<5;++r){auto sd=converter(r);int last=recipes[r].ids.back();
            flow(npc,r,1,[&](){int i=pc_search_inventory(sd.get(),last);--sd->inventory.u.items_inventory[i].amount;weight();},false,true);
            check(errors==1&&zeny_log.empty(),"original later-material abort expected");
            for(size_t i=0;i+1<recipes[r].ids.size();++i)check(count(recipes[r].ids[i])==0,"original earlier materials already deleted");
            check(count(last)==recipes[r].needs.back()-1&&count(recipes[r].output)==0,"original failing material and output preserved");}
    }else{
        // Actual parser registration and optional character lookup, then direct
        // native return-value probes. Only the world character index is doubled.
        {auto sd=converter(0);sd->status.inventory_slots=37;Snapshot before;
            auto* probe=compile("{ @CapacityA=getinventoryslots(); @CapacityB=getinventoryslots(99000002); end; }","actual-getter-VM");
            run_script(probe,0,sd->id,0);check(!sd->st&&errors==0,"actual getter VM terminates cleanly");
            check(nums[add_str("@CapacityA")]==37&&nums[add_str("@CapacityB")]==37,"both registered getter forms return allowed slots");
            getter_probe(probe,0,sd->status.char_id,37,false);before.unchanged();script_free_code(probe);}
        for(int capacity:{0,1,MAX_INVENTORY}){auto sd=converter(0);sd->status.inventory_slots=capacity;Snapshot before;
            auto* probe=compile("{ end; }","actual-getter-raw-capacity");getter_probe(probe,sd->id,-1,capacity,false);before.unchanged();script_free_code(probe);}
        // All five recipes, native signed-amount boundaries, multiple chunks,
        // the 30000 output limit and ordinary resource/price ceilings.
        for(int r=0;r<5;++r)for(int q:{1,2,3000,3001,3220,3276,3277,6000,6553,6554,10000,30000}){
            auto sd=converter(r,q);Snapshot before;flow(npc,r,q);check(input_count==1,"real input executed");committed(before,r,q);}
        for(int r=0;r<5;++r)for(int q:{0,-1,2,INT32_MAX}){auto sd=converter(r);Snapshot before;flow(npc,r,q);before.unchanged();check(zeny_log.empty(),"invalid input never charges");}
        for(int r=0;r<5;++r){auto sd=converter(r,30001);Snapshot before;flow(npc,r,30001);before.unchanged();check(zeny_log.empty(),"output cap refuses rather than silently clamps");}
        for(int r=0;r<5;++r){
            auto sd=converter(r);std::unique_ptr<Snapshot> changed;
            flow(npc,r,1,[&](){sd->status.zeny=recipes[r].price-1;changed=std::make_unique<Snapshot>();});changed->unchanged();
            check(zeny_log.empty(),"stale Zeny refuses all material payment");
        }
        for(int r=0;r<5;++r)for(size_t mat=0;mat<recipes[r].ids.size();++mat){
            auto sd=converter(r,6554);std::unique_ptr<Snapshot> changed;
            flow(npc,r,6554,[&](){int i=pc_search_inventory(sd.get(),recipes[r].ids[mat]);--sd->inventory.u.items_inventory[i].amount;weight();changed=std::make_unique<Snapshot>();});
            changed->unchanged();check(zeny_log.empty(),"all-input preflight precedes every debit chunk");
        }
        for(int r=0;r<5;++r)for(int what=0;what<4;++what){auto sd=converter(r);Snapshot before;
            flow(npc,r,1,[&](){if(what==0)sd->status.base_level=249;else if(what==1)setnum("ep17_2_main",32);else if(what==2)setnum("RepPoints6",1999);else sd->m=1;});
            before.unchanged();check(zeny_log.empty(),"current original access and location checked after input");}
        for(int r=0;r<5;++r){auto sd=converter(r);Snapshot before;flow(npc,r,1,{},true);before.unchanged();check(zeny_log.empty(),"actual forced-close cleanup consumes nothing");}
        for(int r=0;r<5;++r){auto sd=converter(r);Snapshot before;
            flow(npc,r,1,[&](){sd->max_weight=sd->weight+9;});before.unchanged();}
        for(int r=0;r<5;++r){auto sd=converter(r);sd->max_weight=sd->weight+10;Snapshot before;flow(npc,r,1);committed(before,r,1);}
        // Full accessible inventory, including deterioration during input.
        for(int r=0;r<5;++r){auto sd=converter(r);sd->status.inventory_slots=recipes[r].ids.size();Snapshot before;flow(npc,r,1);before.unchanged();}
        for(int r=0;r<5;++r){auto sd=converter(r);std::unique_ptr<Snapshot> changed;
            flow(npc,r,1,[&](){sd->status.inventory_slots=recipes[r].ids.size();changed=std::make_unique<Snapshot>();});changed->unchanged();}
        // Match binding/UID/card/expiry exactly. Ignore fields native stacking
        // ignores, including refine/options/identify/favorite/grade.
        for(int r=0;r<5;++r)for(int variant=1;variant<=5;++variant)for(int room=0;room<2;++room){
            auto sd=converter(r,2);int n=recipes[r].ids.size();set_output(n,r,20,variant);sd->status.inventory_slots=n+1+room;Snapshot before;flow(npc,r,2);
            if(room||variant==5)committed(before,r,2);else before.unchanged();
        }
        for(int r=0;r<5;++r){auto sd=converter(r,2);int n=recipes[r].ids.size();set_output(n,r,30000,1);set_output(n+1,r,100,0);sd->status.inventory_slots=n+2;Snapshot before;flow(npc,r,2);committed(before,r,2);}
        for(int r=0;r<5;++r){auto sd=converter(r,2);int n=recipes[r].ids.size();set_output(n,r,29999,0);set_output(n+1,r,100,0);Snapshot before;flow(npc,r,2);before.unchanged();}
        for(int r=0;r<5;++r){auto sd=converter(r,2);int n=recipes[r].ids.size();set_output(n+2,r,100,0);sd->status.inventory_slots=n+1;Snapshot before;flow(npc,r,2);before.unchanged();}
        for(int r=0;r<5;++r){auto sd=converter(r,2);int n=recipes[r].ids.size();set_output(n+2,r,100,1);sd->status.inventory_slots=n+1;Snapshot before;flow(npc,r,2);committed(before,r,2);}
        for(int r=0;r<5;++r){auto sd=converter(r,2);
            for(size_t i=0;i<recipes[r].ids.size();++i)sd->inventory.u.items_inventory[i].bound=BOUND_CHAR;
            Snapshot before;flow(npc,r,1);committed(before,r,1);}
        // Old recipe menu Leave remains cancellation. Its numeric input is not
        // reached, and the full inventory is unchanged.
        {auto sd=converter(0);Snapshot before;flow(npc,5,1);before.unchanged();check(input_count==0,"original recipe Leave choice preserved");}
    }
    attached=nullptr;script_free_code(npc);item_db.clear();reputation_db.clear();achievement_db.clear();
    nums.clear();strings.clear();fake_nd=nullptr;fake.reset();do_final_script();timer_final();db_final();malloc_final();
    std::printf(getter_missing?"CONVERSION_GETTER_FAILURES_OK cases=%u assertions=%u\n":original?"CONVERSION_OLD_FAILURES_OK cases=%u assertions=%u\n":"CONVERSION_NATIVE_OK cases=%u assertions=%u\n",cases,assertions);
    return 0;
}
