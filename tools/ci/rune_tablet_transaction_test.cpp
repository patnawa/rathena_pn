// ============================================================================
//  PN  /  DEVELOPMENT TOOLS
//  rune_tablet_transaction_test.cpp
// ----------------------------------------------------------------------------
//  Project contributions: (C) 2026 PN Development Team
//  License for project contributions: GPL-3.0-or-later; see LICENSE.
//  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/rune_tablet_transaction_test.cpp
//  Existing upstream authors, notices and other rights are retained.
// ============================================================================

// Appended to the crown fixture's explicit transport/world boundary prefix.
// Production service/catalog bodies and native parser, registry and inventory
// code execute unchanged. Bonus stat recalculation is a recorded boundary.
#include "common/ers.hpp"
#include <regex>

extern "C" block_list* rune_world(int32) asm("__wrap__Z9map_id2bli");
extern "C" block_list* rune_world(int32 id){return attached&&attached->id==id?static_cast<block_list*>(attached):nullptr;}

namespace {
struct RuneDelete { void operator()(map_session_data* sd) const {
    check(!sd->st,"no suspended Rune dialogue at teardown");
    if(sd->regs.arrays)sd->regs.arrays->destroy(sd->regs.arrays,script_free_array_db);
    sd->regs.vars->destroy(sd->regs.vars,script_reg_destroy);delete sd;
}};
using RunePlayer=std::unique_ptr<map_session_data,RuneDelete>;
RunePlayer rune_player() {
    ++cases;errors=0;messages.clear();menu_text.clear();closes=unequips=equips=0;
    RunePlayer sd(new map_session_data());attached=sd.get();
    sd->id=sd->status.account_id=99000001;sd->status.char_id=99000002;sd->type=BL_PC;
    sd->status.zeny=7654321;sd->status.inventory_slots=MAX_INVENTORY;
    sd->fd=0;sd->state.ignoretimeout=true;sd->npc_idle_timer=INVALID_TIMER;sd->max_weight=100000000;
    for(auto& i:sd->equip_index)i=-1;for(auto& i:sd->equip_switch_index)i=-1;
    sd->regs.vars=i64db_alloc(DB_OPT_BASE);sd->vars_ok=true;return sd;
}
int64 reg(const char* name,int index=0) {return pc_readregistry(attached,reference_uid(add_str(name),index));}
void reg(const char* name,int index,int64 value) {check(pc_setregistry(attached,reference_uid(add_str(name),index),value),"native persistent write");}
void load_functions(const std::string& path) {
    auto source=read(path);std::regex declaration("function[\\t ]+script[\\t ]+([A-Za-z0-9_]+)[\\t ]+\\{");
    for(std::sregex_iterator i(source.begin(),source.end(),declaration),end;i!=end;++i) {
        std::string name=(*i)[1];auto* code=compile(body(source,(*i).str()),name.c_str());
        strdb_put(script_get_userfunc_db(),name.c_str(),code);
    }
}
void invoke(const std::string& command,const std::vector<int>& choices={},const std::function<void(int,int)>& hook={}) {
    auto* code=compile("{ "+command+" end; }","Rune production entry");
    walk(code,choices,hook);script_free_code(code);
}
void stock(const char* kind,int id,int level=0) {
    invoke(std::string("callfunc \"PN_RT_Cost\",\"")+kind+"\","+std::to_string(id)+","+std::to_string(level)+";");
    int n=pc_readreg(attached,add_str("@RT_CostCount"));
    for(int i=0;i<n;++i)put(i,pc_readreg(attached,reference_uid(add_str("@RT_CostIds"),i)),pc_readreg(attached,reference_uid(add_str("@RT_CostAmounts"),i)));
    weight();
}
void clear_items(){attached->inventory={};for(auto& d:attached->inventory_data)d=nullptr;weight();}
}

extern "C" int __wrap_main(int argc,char** argv) {
    check(argc==2,"artifact directory supplied");deny_network();
    static char server[]="rune-tablet-transaction-test";SERVER_NAME=server;
    malloc_init();db_init();do_init_database();timer_init();do_init_script();battle_set_defaults();
    battle_config.atcommand_disable_npc=0;
    num_reg_ers=ers_new(sizeof(script_reg_num),"rune:num",(ERSOptions)(ERS_OPT_CLEAN|ERS_OPT_FLEX_CHUNK));
    str_reg_ers=ers_new(sizeof(script_reg_str),"rune:str",(ERSOptions)(ERS_OPT_CLEAN|ERS_OPT_FLEX_CHUNK));pc_set_reg_load(false);
    auto raw=read(std::string(argv[1])+"/items.yml");auto tree=ryml::parse_in_arena(ryml::to_csubstr(raw));
    for(auto row:tree["Body"])check(item_db.parseBodyNode(row)==1,"real catalog item metadata parses");
    load_functions("npc/custom/rune_tablet/data.txt");load_functions("npc/custom/rune_tablet/services.txt");
    strdb_put(script_get_userfunc_db(),"PN_RT_Refresh",compile("{ @refreshes++; return; }","explicit stat refresh boundary"));
    // Registration: successful permanent unlock, repeat no-debit, cancel,
    // and stale final-confirmation inventory. First Greenhouse rune is1263026.
    for(int mode=0;mode<4;++mode){auto sd=rune_player();stock("piece",1263026);Snapshot before;
        invoke("callfunc \"PN_RT_Register\",1260008;",{1,mode==1?2:1},[&](int,int chosen){
            if(mode==2&&chosen==1){clear_items();}
        });
        if(mode==0||mode==3){check(reg("#PNRTPiece",26)==1,"account rune flag committed");
            Snapshot after;invoke("callfunc \"PN_RT_Register\",1260008;",{1});after.unchanged();}
        else {check(reg("#PNRTPiece",26)==0,"refused registration never writes flag");if(mode==1)before.unchanged();}
    }
    // Activation is paid once per character. Switching paid tablets is free.
    for(int mode=0;mode<4;++mode){auto sd=rune_player();stock("activate",1260008);
        if(mode!=2)reg("#PNRTPiece",26,1);
        if(mode==1){reg("PNRTPaid",8,1);reg("PNRTActive",0,1260000);}
        Snapshot before;
        invoke("callfunc \"PN_RT_SetMenu\",1260008;",mode==2?std::vector<int>{2,6}:std::vector<int>{2,mode==3?2:1,6});
        if(mode==0||mode==1)check(reg("PNRTPaid",8)==1&&reg("PNRTActive")==1260008,"paid or free activation equips exact tablet");
        else check(!reg("PNRTPaid",8)&&!reg("PNRTActive"),"missing piece or cancellation cannot activate");
        if(mode!=0)before.unchanged();
    }
    // Upgrade uses actual rand builtin at success/failure/pity boundaries.
    for(int mode=0;mode<5;++mode){auto sd=rune_player();reg("PNRTPaid",8,1);stock("upgrade",1260008,1);
        if(mode==2)reg("PNRTPity",8,99999);
        if(mode==3)reg("PNRTLevel",8,15);
        Snapshot before;seed_roll(mode==0?0:99999,100000);
        invoke("callfunc \"PN_RT_Upgrade\",1260008;",{mode==4?2:1});
        if(mode==0||mode==2)check(reg("PNRTLevel",8)==1&&reg("PNRTPity",8)==0,"success increments once and clears pity");
        else if(mode==1)check(reg("PNRTLevel",8)==0&&reg("PNRTPity",8)==4000,"failure keeps level and adds exact pity step");
        else before.unchanged();
    }
    // Claim: full inventory and incompatible metadata must never consume claim.
    for(int mode=0;mode<7;++mode){auto sd=rune_player();reg("PNRTPaid",8,1);
        if(mode==1){sd->status.inventory_slots=1;put(0,1001282,1);}
        if(mode>=2&&mode<=5){sd->status.inventory_slots=1;put(0,103366,1);auto& out=sd->inventory.u.items_inventory[0];
            if(mode==2)out.bound=BOUND_CHAR;if(mode==3)out.unique_id=UINT64_MAX;if(mode==4)out.card[0]=4700;if(mode==5)out.expire_time=2100000000;}
        weight();Snapshot before;
        invoke("callfunc \"PN_RT_Claim\",1260008;",{1,mode==6?2:1});
        if(mode==0){check(reg("#PNRTClaims",8)==1&&count(103366)==1,"exact activation reward granted and marked once");
            Snapshot after;invoke("callfunc \"PN_RT_Claim\",1260008;");after.unchanged();}
        else {check(!reg("#PNRTClaims",8),"capacity/cancel does not claim");before.unchanged();}
    }
    // Model the existing account-registry transport boundary across character
    // login: only the saved account claim is loaded into a fresh alt registry.
    int64 saved_account_claim=0;
    {auto sd=rune_player();reg("PNRTPaid",8,1);invoke("callfunc \"PN_RT_Claim\",1260008;",{1,1});
        saved_account_claim=reg("#PNRTClaims",8);check(saved_account_claim==1,"claim uses account registry namespace");}
    {auto alt=rune_player();alt->status.char_id=99000004;pc_set_reg_load(true);reg("#PNRTClaims",8,saved_account_claim);pc_set_reg_load(false);
        reg("PNRTPaid",8,1);Snapshot before;invoke("callfunc \"PN_RT_Claim\",1260008;");before.unchanged();
        check(count(103366)==0&&reg("#PNRTClaims",8)==1,"different character on same account cannot reclaim account milestone");}
    // Imprint instance identity: native uint64 maximum is preserved as string.
    for(int mode=0;mode<12;++mode){auto sd=rune_player();put(0,1862,1);sd->inventory.u.items_inventory[0].unique_id=UINT64_MAX;put(1,1001282,10);weight();
        std::unique_ptr<Snapshot> before;
        invoke("callfunc \"PN_RT_Print\";",{1,mode==10?2:1},[&](int,int chosen){
            if(chosen!=1||before)return;auto& gear=sd->inventory.u.items_inventory[0];
            if(mode==1)gear.unique_id=UINT64_MAX-1;if(mode==2)gear.refine=1;if(mode==3)gear.card[0]=4700;
            if(mode==4)gear.enchantgrade=1;if(mode==5)gear.option[0].id=1;if(mode==6)gear.bound=BOUND_CHAR;
            if(mode==7)gear.expire_time=2100000000;if(mode==8)gear.equip=EQP_HAND_R;
            if(mode==9){sd->inventory.u.items_inventory[1].amount=9;weight();}
            if(mode==11)sd->status.inventory_slots=2;
            before=std::make_unique<Snapshot>();
        });
        check(before!=nullptr,"imprint confirmation reached");
        if(mode==0)check(count(1862)==0&&count(1001282)==0&&count(1001374)==1,"exact equipment and10 runes consumed for exact printed output");
        else before->unchanged();
    }
    // Shop input disappears at confirmation: no output and no partial debit.
    for(int mode=0;mode<3;++mode){auto sd=rune_player();put(0,1001282,150);weight();
        invoke("callfunc \"PN_RT_Shop\";",{1,mode==2?2:1},[&](int,int chosen){if(mode==1&&chosen==1)clear_items();});
        check(count(102767)==(mode==0?1:0),"shop only grants on validated commit");
        check(count(1001282)==(mode==2?150:0),"shop exact debit or cancel preservation");
    }
    // Sealed card whitelist and deterministic rune-stone decomposition.
    {auto sd=rune_player();put(0,4480,1);weight();invoke("callfunc \"PN_RT_Seal\";",{1,1});check(count(4480)==0&&count(1001594)==1,"whitelisted sealed card becomes one Rune Seal");}
    {auto sd=rune_player();put(0,4001,1);weight();Snapshot before;invoke("callfunc \"PN_RT_Seal\";");before.unchanged();}
    {auto sd=rune_player();put(0,1001595,30);weight();invoke("callfunc \"PN_RT_Decompose\";",{1,2,1});check(count(1001595)==0&&count(1001283)==30,"native30-stone recipe gives exact30 Perfect Runes");}
    // Differential capacity proof against actual native pc_additem: conservative
    // refusals are allowed, but every approved plain output must really fit.
    for(int variant=0;variant<7;++variant)for(int amount:{1,30,30000})for(int existing:{0,1,29999,30000})for(int slots:{1,2}){
        auto sd=rune_player();sd->status.inventory_slots=slots;
        if(existing){put(0,1001283,existing);auto& out=sd->inventory.u.items_inventory[0];
            if(variant==1)out.bound=BOUND_CHAR;if(variant==2)out.unique_id=UINT64_MAX;
            if(variant==3)out.card[2]=4700;if(variant==4)out.expire_time=2100000000;
            if(variant==5){out.refine=12;out.option[0].id=1;}if(variant==6){put(1,1001283,1);}}
        weight();
        invoke("@prediction=callfunc(\"PN_RT_CanGrantOne\",1001283,"+std::to_string(amount)+");");
        if(pc_readreg(sd.get(),add_str("@prediction"))){item out{};out.nameid=1001283;out.identify=1;
            check(pc_additem(sd.get(),&out,amount,LOG_TYPE_SCRIPT)==ADDITEM_SUCCESS,"every positive capacity prediction matches real native grant");}
    }
    attached=nullptr;item_db.clear();do_final_script();ers_destroy(num_reg_ers);ers_destroy(str_reg_ers);num_reg_ers=str_reg_ers=nullptr;
    timer_final();db_final();malloc_final();
    std::printf("RUNE_NATIVE_OK cases=%u assertions=%u\n",cases,assertions);return 0;
}
