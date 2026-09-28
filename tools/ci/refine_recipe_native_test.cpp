// Native script VM + native effective refine recipe lookup; outbound UI is wrapped.
unsigned refine_windows=0;
extern "C" void refine_window(map_session_data*) asm("__wrap__Z18clif_refineui_openP16map_session_data");
extern "C" void refine_window(map_session_data* sd){++refine_windows;sd->state.refineui_open=true;}
extern "C" int __wrap_main(int argc,char** argv) {
    check(argc==2,"fixture path supplied");deny_network();
    static char server[]="refine-recipe-test";SERVER_NAME=server;
    malloc_init();db_init();do_init_database();timer_init();do_init_script();battle_set_defaults();
    const std::string directory=argv[1];
    auto text=read(directory+"/items.yml");auto tree=ryml::parse_in_arena(ryml::to_csubstr(text));
    for(auto row:tree["Body"])check(item_db.parseBodyNode(row)==1,"ore metadata parses");
    refine_db.load();
    auto* hammer=compile(read(directory+"/rate0.txt"),"actual Mighty Hammer success expression");
    auto* basta=compile(read(directory+"/rate1.txt"),"actual Basta success expression");
    int scales[2];std::istringstream(read(directory+"/scales.txt"))>>scales[0]>>scales[1];
    unsigned mismatches=0;
    for(int category=0;category<7;++category)for(int level=7;level<20;++level){
        ++cases;auto sd=std::make_unique<map_session_data>();attached=sd.get();
        sd->id=sd->status.account_id=99000001;sd->status.char_id=99000002;sd->type=BL_PC;
        sd->state.ignoretimeout=true;sd->npc_idle_timer=INVALID_TIMER;
        for(auto& i:sd->equip_index)i=-1;
        auto data=std::make_shared<item_data>();data->nameid=400999;data->ename="Refine fixture";
        data->type=category<2?IT_ARMOR:IT_WEAPON;
        data->armor_level=category+1;data->weapon_level=category-1;data->equip=EQP_HEAD_TOP;
        item_db.put(data->nameid,data);put(0,data->nameid,1,true);
        sd->inventory.u.items_inventory[0].refine=level;
        auto recipe=refine_db.findLevelInfo(*data,sd->inventory.u.items_inventory[0]);
        check(recipe!=nullptr,"native effective level exists");
        auto found=recipe->costs.find(REFINE_COST_HD);
        auto cost=found==recipe->costs.end()?nullptr:found->second;
        if(cost){
            run_script(level<10?hammer:basta,0,sd->id,NPC);
            int actual=nums[add_str("@answer")]*10000/scales[level<10?0:1];
            if(actual!=cost->chance){++mismatches;std::printf("RATE_MISMATCH category=%d current=+%d HD=%d NPC=%d\n",category,level,cost->chance,actual);}
            check(!sd->st,"rate expression terminates");
        }
        attached=nullptr;nums.clear();
    }
    {
        ++cases;auto sd=std::make_unique<map_session_data>();attached=sd.get();
        sd->id=sd->status.account_id=99000001;sd->status.char_id=99000002;sd->type=BL_PC;
        sd->status.zeny=7654321;sd->state.ignoretimeout=true;sd->npc_idle_timer=INVALID_TIMER;
        battle_config.feature_refineui=1;
        // The fallback branch references this registered user function at parse
        // time; the native-UI path must never execute it.
        strdb_put(script_get_userfunc_db(),"F_getpositionname",compile("{ return \"unused fallback\"; }","fallback declaration"));
        auto* shadow=compile(body(read(directory+"/shadow.txt"),"::ShadowBlacksmith"),"actual Shadow Blacksmith");
        walk(shadow,{});
        check(refine_windows==1&&sd->state.refineui_open,"Shadow Blacksmith opens native service instead of dead-end referral");
        script_free_code(shadow);attached=nullptr;
    }
    script_free_code(hammer);script_free_code(basta);refine_db.clear();item_db.clear();
    strings.clear();do_final_script();timer_final();db_final();malloc_final();
    std::printf("REFINE_RECIPE_NATIVE cases=%u mismatches=%u assertions=%u\n",cases,mismatches,assertions);
    return mismatches?1:0;
}
