// Actual item scripts, bonus lifecycle, bonus arithmetic, and movement formula.
// The parent fixture supplies only offline player/network/persistence boundaries.
#include "map/status.hpp"
#include "map/skill.hpp"
#include "map/unit.hpp"

static int32 recalc(map_session_data* sd,uint8) {
    sd->base_status={};sd->bonus={};sd->indexed_bonus={};
    sd->base_status.patk=100;sd->base_status.smatk=100;sd->base_status.flee=100;
    pc_bonus_script(sd);
    return 0;
}
extern "C" void recalc_block(block_list*,std::bitset<SCB_MAX>,uint8) asm("__wrap_@STATUS_CALC_SYMBOL@");
extern "C" void recalc_block(block_list* bl,std::bitset<SCB_MAX>,uint8 opt) {
    recalc(BL_CAST(BL_PC,bl),opt);
}

extern "C" int __wrap_main(int argc,char** argv) {
    check(argc==2,"item-script directory supplied");deny_network();
    static char server[]="consumable-booster-test";SERVER_NAME=server;
    malloc_init();db_init();do_init_database();timer_init();do_init_script();battle_set_defaults();
    {
        ++cases;auto sd=std::make_unique<map_session_data>();attached=sd.get();
        sd->id=99000005;sd->type=BL_PC;sd->status.char_id=99000005;
        sd->status.zeny=7654321;sd->state.ignoretimeout=true;sd->dsprate=100;sd->npc_idle_timer=INVALID_TIMER;
        auto* effect=sd->sc.createSCE(SC_LIMIT_POWER_BOOSTER);
        effect->val1=30;effect->val2=1;effect->val3=5;
        auto* code=compile("{"+read(std::string(argv[1])+"/power.txt")+"\nend;}","loaded Power Booster status script");
        walk(code,{});
        check(sd->dsprate==95,"Power Booster reduces SP cost by five percent");
        check(sd->base_status.hit==30&&sd->base_status.flee==30,"Power Booster HIT/FLEE unchanged");
        check(sd->bonus.aspd_add==-10,"Power Booster ASPD unchanged");
        script_free_code(code);sd->sc.deleteSCE(SC_LIMIT_POWER_BOOSTER);attached=nullptr;
    }
    for(int id:{102803,103273,102985,103272}) {
        ++cases;auto sd=std::make_unique<map_session_data>();attached=sd.get();
        sd->id=99000005;sd->type=BL_PC;sd->status.char_id=99000005;
        sd->status.zeny=7654321;sd->state.ignoretimeout=true;sd->npc_idle_timer=INVALID_TIMER;
        sd->ud.skilltimer=INVALID_TIMER;sd->class_=MAPID_NOVICE;
        auto* code=compile("{"+read(std::string(argv[1])+"/"+std::to_string(id)+".txt")+"\nend;}","actual consumable script");
        walk(code,{});
        check(sd->bonus_script.count==1,"consumable applies one effect instead of doing nothing");
        auto* entry=static_cast<s_bonus_script_entry*>(sd->bonus_script.head->data);
        check(entry->flag & BSF_REM_ON_DEAD,"official buff ends on death");
        check(!(entry->flag & BSF_REM_ON_LOGOUT),"relogging does not remove the effect");
        check(entry->tick-gettick()>=1799000&&entry->tick-gettick()<=1800000,"30-minute lifetime");
        if(id==102803||id==103273) {
            for(int p=PARAM_POW;p<=PARAM_CRT;++p)check(sd->indexed_bonus.param_bonus[p]==5,"all six traits gain five");
            check(sd->base_status.patk==110&&sd->base_status.smatk==110,"P.ATK/S.MATK add ten, not ten percent");
            check(sd->patk_rate==0&&sd->smatk_rate==0,"no percentage attack multiplier");
        }else {
            check(sd->base_status.flee==150,"FLEE gains fifty");
            check(sd->bonus.aspd_add==-10,"ASPD gains one");
            check(sd->bonus.speed_rate==-25&&sd->bonus.speed_add_rate==0,"movement uses non-stacking haste");
            check(audited_status_calc_speed(sd.get(),&sd->sc,200)==150,"movement reduces walk delay by 25 percent");
            sd->sc.createSCE(SC_INCREASEAGI)->val1=10;
            check(audited_status_calc_speed(sd.get(),&sd->sc,200)==150,"Increase Agility does not stack movement");
            sd->sc.deleteSCE(SC_INCREASEAGI);
            pc_bonus(sd.get(),SP_SPEED_RATE,25);
            check(audited_status_calc_speed(sd.get(),&sd->sc,200)==150,"Moonlight Flower-style equipment does not stack");
        }
        // Advance the stored and scheduled expiry backwards without a wall-clock wait.
        entry->tick=gettick()+60000;settick_timer(entry->tid,entry->tick);
        walk(code,{});
        check(sd->bonus_script.count==1,"repeat use refreshes without stacking");
        check(get_timer(entry->tid)->tick==entry->tick,"refresh updates expiry metadata and timer together");
        check(entry->tick-gettick()>=1799000,"refreshed expiry metadata retains the full duration");
        recalc(sd.get(),0);
        check(sd->base_status.patk==(id==102803||id==103273?110:100),"repeated use does not add extra attack");
        pc_bonus_script_clear(sd.get(),BSF_REM_ON_DEAD);
        check(sd->bonus_script.count==0&&sd->bonus_script.head==nullptr,"death removes the complete effect");
        check(sd->base_status.patk==100&&sd->base_status.flee==100&&sd->bonus.aspd_add==0,"no bonuses survive death");
        walk(code,{});entry=static_cast<s_bonus_script_entry*>(sd->bonus_script.head->data);
        pc_bonus_script_timer(entry->tid,entry->tick,sd->id,reinterpret_cast<intptr_t>(entry));
        check(sd->bonus_script.count==0&&sd->base_status.patk==100&&sd->base_status.flee==100,"timer expiration removes all effects");
        script_free_code(code);attached=nullptr;
    }
    check(errors==0,"no native script errors");
    do_final_script();timer_final();db_final();malloc_final();
    std::printf("CONSUMABLE_BOOSTER_OK cases=%u assertions=%u\n",cases,assertions);return 0;
}
