// ============================================================================
//  PN  /  DEVELOPMENT TOOLS
//  equipswitch_deletion_test.cpp
// ----------------------------------------------------------------------------
//  Project contributions: (C) 2026 PN Development Team
//  License for project contributions: GPL-3.0-or-later; see LICENSE.
//  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/equipswitch_deletion_test.cpp
//  Existing upstream authors, notices and other rights are retained.
// ============================================================================

// GPL-3.0-or-later. Appended to unchanged crown fixture boundary helpers.
#include <array>

namespace {
struct Notice { int index;uint32 mask;int result; };
std::vector<Notice> switch_adds,switch_removes;
std::vector<int> payments;
bool old_engine=false;
struct Exchange { int input,rune,need,price,output,element,piece; };
const Exchange exchanges[]={
    {450199,1000640,30,300000,450201,0,0},{480144,1000640,20,300000,480145,0,1},{470107,1000640,15,300000,470108,0,2},
    {450199,1000641,30,300000,450200,1,0},{480144,1000641,20,300000,480146,1,1},{470107,1000641,15,300000,470109,1,2},
    {450199,1000642,30,300000,450203,2,0},{480144,1000642,20,300000,480148,2,1},{470107,1000642,15,300000,470111,2,2},
    {450199,1000643,30,300000,450202,3,0},{480144,1000643,20,300000,480147,3,1},{470107,1000643,15,300000,470110,3,2},
    {490297,1001182,30,100000,490299,0,-1},{490297,1001180,30,100000,490300,1,-1},{490297,1001178,30,100000,490301,2,-1}};

auto subject(){
    ++cases;errors=0;nums.clear();strings.clear();messages.clear();menu_text.clear();windows.clear();
    switch_adds.clear();switch_removes.clear();payments.clear();closes=unequips=equips=0;
    auto sd=std::make_unique<map_session_data>();attached=sd.get();
    sd->id=sd->status.account_id=99000001;sd->status.char_id=99000002;sd->type=BL_PC;
    sd->status.base_level=250;sd->status.class_=JOB_DRAGON_KNIGHT;sd->class_=MAPID_DRAGON_KNIGHT;sd->status.sex=SEX_MALE;
    sd->status.zeny=1000000;sd->status.inventory_slots=MAX_INVENTORY;sd->max_weight=1000000;
    sd->state.ignoretimeout=true;sd->npc_idle_timer=INVALID_TIMER;sd->m=0;sd->fd=0;
    for(auto& i:sd->equip_index)i=-1;for(auto& i:sd->equip_switch_index)i=-1;
    check(sd->permissions.none()&&!pc_has_permission(sd.get(),PC_PERM_USE_ALL_EQUIPMENT),"ordinary equipment permissions, no GM bypass");
    return sd;
}
struct FullSnapshot:Snapshot {
    std::array<int16,EQI_MAX> cache,equipped;
    std::array<item_data*,MAX_INVENTORY> data;
    explicit FullSnapshot(){std::copy(std::begin(attached->equip_switch_index),std::end(attached->equip_switch_index),cache.begin());
        std::copy(std::begin(attached->equip_index),std::end(attached->equip_index),equipped.begin());
        std::copy(std::begin(attached->inventory_data),std::end(attached->inventory_data),data.begin());}
    void unchanged_all()const{unchanged();check(std::equal(cache.begin(),cache.end(),std::begin(attached->equip_switch_index)),"switch cache unchanged");
        check(std::equal(equipped.begin(),equipped.end(),std::begin(attached->equip_index)),"equipped cache unchanged");
        check(std::equal(data.begin(),data.end(),std::begin(attached->inventory_data)),"all metadata pointers unchanged");}
};
uint32 register_switch(int index,int id,int requested=0){
    put(index,id,1);weight();auto* sd=attached;
    const auto before_add=switch_adds.size();const uint32 mask=requested?requested:sd->inventory_data[index]->equip;
    check(pc_isequip(sd,index)==ITEM_EQUIP_ACK_OK,"actual class, level, item eligibility permits registration");
    check(pc_equipitem(sd,index,mask,true),"actual native equipment-switch registration succeeds");
    uint32 result=sd->inventory.u.items_inventory[index].equipSwitch;
    check(result&&sd->inventory.u.items_inventory[index].equip==0,"registered but not ordinarily equipped");
    check(switch_adds.size()==before_add+1&&switch_adds.back().result==ITEM_EQUIP_ACK_OK,"exact successful switch registration notification");
    for(int i=0;i<EQI_MAX;++i)if(result&equip_bitmask[i])check(sd->equip_switch_index[i]==index,"actual native registration fills every mask cache entry");
    return result;
}
void cleanup_post(const FullSnapshot& before,int index,uint32 mask){
    for(int i=0;i<EQI_MAX;++i)check(attached->equip_switch_index[i]==(before.cache[i]==index?(old_engine?index:-1):before.cache[i]),"every target cache reference cleared, all unrelated references preserved");
    if(old_engine)check(switch_removes.empty(),"pinned old deletion sends no cleanup acknowledgement");
    else check(switch_removes.size()==1&&switch_removes[0].index==index&&switch_removes[0].mask==mask&&!switch_removes[0].result,"one exact original-mask removal notification");
    check(attached->inventory.u.items_inventory[index].equipSwitch==0,"consumed/replaced cell has no switch mask");
}
void npc_flow(script_code* code,const Exchange& recipe,bool cancel=false){
    std::vector<int> choices=recipe.piece<0?std::vector<int>{2,recipe.element+1,cancel?2:1}:
        std::vector<int>{1,recipe.element+1,recipe.piece+1,cancel?2:1};
    size_t answer=0;int pauses=0;run_script(code,0,attached->id,NPC);
    while(attached->st){check(++pauses<20,"Ellie dialogue terminates");auto* st=attached->st;
        if(st->state==RERUNLINE){check(answer<choices.size(),"actual Ellie menu has supplied choice");attached->npc_menu=choices[answer++];}
        else if(st->state==CLOSE)st->state=END;
        else check(st->state==STOP,"known actual Ellie Next suspension");
        run_script_main(st);
    }
    check(answer==choices.size()&&errors==0,"actual Ellie exchange completes without native script error");
}
}
extern "C" void switch_add(const map_session_data*,uint16,uint32,uint8) asm("__wrap__Z20clif_equipswitch_addPK16map_session_datatjh");
extern "C" void switch_add(const map_session_data* sd,uint16 index,uint32 mask,uint8 result){
    check(sd==attached,"switch add targets actual fixture player");switch_adds.push_back({index,mask,result});
}
extern "C" void switch_remove(const map_session_data*,uint16,uint32,bool) asm("__wrap__Z23clif_equipswitch_removePK16map_session_datatjb");
extern "C" void switch_remove(const map_session_data* sd,uint16 index,uint32 mask,bool failed){
    check(sd==attached&&sd->inventory.u.items_inventory[index].equipSwitch==mask,"native removal notifies while original mask still exists");
    for(int i:sd->equip_switch_index)check(i!=index,"native removal clears all cache entries before notification");
    switch_removes.push_back({index,mask,failed});
}
extern "C" void switch_zeny(const map_session_data&,e_log_pick_type,uint32,int32) asm("__wrap__Z8log_zenyRK16map_session_data15e_log_pick_typeji");
extern "C" void switch_zeny(const map_session_data& sd,e_log_pick_type type,uint32 id,int32 amount){check(&sd==attached&&type==LOG_TYPE_SCRIPT&&id==sd.status.char_id,"native script payment log metadata");payments.push_back(amount);}

extern "C" int __wrap_main(int argc,char** argv){
    check(argc==3,"explicit artifact directory and mode");old_engine=std::string(argv[2])!="fixed";
    bool old_oob=std::string(argv[2])=="old-oob";deny_network();
    static char name[]="equipswitch-deletion-test";SERVER_NAME=name;
    malloc_init();db_init();do_init_database();timer_init();do_init_script();battle_set_defaults();
    battle_config.atcommand_disable_npc=0;battle_config.feature_achievement=0;
    const std::string dir=argv[1];auto text=read(dir+"/items.yml");auto rows=ryml::parse_in_arena(ryml::to_csubstr(text));
    for(auto row:rows["Body"])check(item_db.parseBodyNode(row)==1,"exact effective item record parses natively");
    if(old_oob){auto sd=subject();std::fprintf(stderr,"EXPECTED_OLD_OOB_BEGIN pc_delitem index=%d MAX_INVENTORY=%d\n",MAX_INVENTORY,MAX_INVENTORY);
        pc_delitem(sd.get(),MAX_INVENTORY,1,0,0,LOG_TYPE_SCRIPT);check(false,"old native out-of-bounds must be diagnosed");}
    // Real single-slot, two-handed, multi-head and accessory registrations.
    for(int id:{450199,480144,470107,490297,1151,2224})for(int flags=0;flags<8;++flags){
        auto sd=subject();uint32 mask=register_switch(0,id);
        int other=id==490297?450199:490297;register_switch(3,other);switch_removes.clear();
        FullSnapshot before;check(pc_delitem(sd.get(),0,1,flags,0,LOG_TYPE_SCRIPT)==0,"full native deletion succeeds");
        cleanup_post(before,0,mask);check(sd->inventory_data[0]==nullptr,"exhausted data pointer cleared");
        check(std::memcmp(&before.inventory.u.items_inventory[3],&sd->inventory.u.items_inventory[3],sizeof(item))==0,"unrelated switch item metadata preserved");
        check(sd->weight==before.weight-item_db.find(id)->weight,"exact full deletion weight");
    }
    // Defensive generic partial-depletion fixture: stacked equipment amount is
    // not claimed to be ordinary acquisition; it exercises the amount branch.
    for(int flags=0;flags<8;++flags){auto sd=subject();register_switch(0,1151);
        sd->inventory.u.items_inventory[0].amount=2;weight();FullSnapshot before;item expected=before.inventory.u.items_inventory[0];expected.amount=1;
        check(pc_delitem(sd.get(),0,1,flags,0,LOG_TYPE_SCRIPT)==0,"partial native deletion succeeds");
        check(std::memcmp(&expected,&sd->inventory.u.items_inventory[0],sizeof(item))==0,"partial deletion preserves every other item field");
        check(std::equal(before.cache.begin(),before.cache.end(),std::begin(sd->equip_switch_index))&&switch_removes.empty(),"partial deletion preserves complete registration without packet");}
    // All EQI cache positions, including defensive duplicate references outside
    // the visible mask, must be removed by the generic helper's equality scan.
    {auto sd=subject();uint32 mask=register_switch(0,450199);for(auto& i:sd->equip_switch_index)i=0;
        FullSnapshot before;check(pc_delitem(sd.get(),0,1,0,0,LOG_TYPE_SCRIPT)==0,"all-cache-entries defensive fixture deletes");cleanup_post(before,0,mask);}
    for(int flags=0;flags<8;++flags){auto sd=subject();put(0,450199,1);weight();check(pc_delitem(sd.get(),0,1,flags,0,LOG_TYPE_SCRIPT)==0,"nonregistered item deletes");check(switch_removes.empty(),"no switch mask sends no removal packet");}
    for(int which=0;which<8;++which){if(old_engine&&(which==1||which==2))continue;auto sd=subject();register_switch(0,450199);
        int index=0,amount=1;if(which==0)index=-1;else if(which==1)index=MAX_INVENTORY;else if(which==2)index=INT_MAX;
        else if(which==3)index=2;else if(which==4)amount=0;else if(which==5)amount=-1;else if(which==6)amount=2;
        else sd->inventory_data[0]=nullptr;
        FullSnapshot before;check(pc_delitem(sd.get(),index,amount,0,0,LOG_TYPE_SCRIPT)==1,"failed native deletion rejects");before.unchanged_all();check(switch_removes.empty(),"rejection sends no removal");}
    auto source=read(dir+"/npc.txt");auto access=read(dir+"/access.txt");
    strdb_put(script_get_userfunc_db(),"F_BiosphereAccess",compile(body(access,"F_BiosphereAccess"),"actual-Biosphere-access"));
    auto* ellie=compile(body(source,"Ellie#biosphere_equipment"),"actual-Ellie-equipment");
    for(const auto& r:exchanges){auto sd=subject();uint32 mask=register_switch(0,r.input);register_switch(3,r.input==490297?450199:490297);
        // All ordinary base items remain unrefined/cardless for first-pass
        // delitem preference; binding/favorite/options prove metadata isolation.
        auto& base=sd->inventory.u.items_inventory[0];base.bound=BOUND_CHAR;base.favorite=1;base.option[0].id=1;base.option[0].value=23;
        put(1,r.rune,r.need+2);sd->inventory.u.items_inventory[1].bound=BOUND_CHAR;weight();switch_removes.clear();
        FullSnapshot before;uint32 next_uid=sd->status.uniqueitem_counter;npc_flow(ellie,r);
        cleanup_post(before,0,mask);check(payments==std::vector<int>{-r.price}&&sd->status.zeny==before.zeny-r.price,"unchanged exact Ellie Zeny price");
        item expected{};expected.nameid=r.output;expected.amount=1;expected.identify=1;expected.unique_id=(uint64(sd->status.char_id)<<32)|next_uid;
        check(std::memcmp(&expected,&sd->inventory.u.items_inventory[0],sizeof(item))==0,"consumed cell reused by exact fresh output metadata and native UID");
        for(int i=1;i<MAX_INVENTORY;++i){item expected=before.inventory.u.items_inventory[i];if(i==1)expected.amount-=r.need;
            check(std::memcmp(&expected,&sd->inventory.u.items_inventory[i],sizeof(item))==0,"every surviving rune/unrelated item field preserved");}
        check(sd->weight==before.weight-item_db.find(r.input)->weight-r.need*item_db.find(r.rune)->weight+item_db.find(r.output)->weight,"exact native exchange weight");
        check(sd->inventory_data[0]==item_db.find(r.output).get(),"replacement data pointer is new output");
    }
    for(const auto& r:exchanges){auto sd=subject();register_switch(0,r.input);put(1,r.rune,r.need+2);weight();FullSnapshot before;npc_flow(ellie,r,true);before.unchanged_all();check(payments.empty()&&switch_removes.empty(),"Ellie cancellation keeps registration and materials");}
    attached=nullptr;script_free_code(ellie);item_db.clear();nums.clear();strings.clear();do_final_script();timer_final();db_final();malloc_final();
    std::printf(old_engine?"EQUIPSWITCH_OLD_REPRO_OK cases=%u assertions=%u\n":"EQUIPSWITCH_FIXED_OK cases=%u assertions=%u\n",cases,assertions);return 0;
}
