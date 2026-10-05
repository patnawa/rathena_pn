// GPL-3.0-or-later. Actual NPC, catalog parser and transaction functions.
#include "map/npc.hpp"
#include "map/pet.hpp"
#include "map/quest.hpp"
#include "map/map.hpp"
#include "common/nullpo.hpp"
#include "common/utils.hpp"
#include "common/sql.hpp"
#include "common/showmsg.hpp"
#include "custom/retired_tokens.hpp"
#include "recipes.inc"
extern char barter_table[];
extern Sql* mmysql_handle;
// FUNCTIONS

npc_data* shop_npc=nullptr;
unsigned shop_windows=0;
extern "C" npc_data* name_lookup(const char*) asm("__wrap__Z11npc_name2idPKc");
extern "C" npc_data* name_lookup(const char* name){check(!strcmp(name,"barter_alice_equipment"),"actual NPC opens exact catalog");return shop_npc;}
extern "C" void barter_window(map_session_data&,npc_data&) asm("__wrap__Z25clif_barter_extended_openR16map_session_dataR8npc_data");
extern "C" void barter_window(map_session_data& sd,npc_data& nd){check(&nd==shop_npc&&sd.state.callshop,"extended floating barter transport boundary");++shop_windows;}
extern "C" bool wallet_param(map_session_data*,int64,int64) asm("__wrap__Z11pc_setparamP16map_session_datall");
extern "C" bool wallet_param(map_session_data* sd,int64 type,int64 value){check(type==SP_ZENY,"legacy wallet setter boundary");sd->status.zeny=value;return true;}

struct PlayerRelease { void operator()(map_session_data* sd)const {
    if(sd->regs.arrays)sd->regs.arrays->destroy(sd->regs.arrays,script_free_array_db);
    if(sd->regs.vars)script_free_vars(sd->regs.vars);
    delete sd;
} };
std::unique_ptr<map_session_data,PlayerRelease> stock(const Recipe& r,int multiplier=1,bool full=false,bool leftovers=false){
    ++cases; nums.clear();strings.clear();messages.clear();menu_text.clear();closes=0;
    std::unique_ptr<map_session_data,PlayerRelease> sd(new map_session_data());attached=sd.get();
    sd->id=sd->status.account_id=99000001;sd->status.char_id=99000002;sd->type=BL_PC;
    sd->status.uniqueitem_counter=1;sd->status.inventory_slots=full?r.costs.size():MAX_INVENTORY;
    sd->status.zeny=int64(r.zeny)*multiplier+100;sd->max_weight=10000000;
    sd->state.ignoretimeout=true;sd->npc_idle_timer=INVALID_TIMER;
    for(auto& i:sd->equip_index)i=-1;
    int index=0;for(auto cost:r.costs)put(index++,cost.first,cost.second*multiplier+(leftovers?1:0));
    weight();return sd;
}
std::shared_ptr<s_npc_barter_item> entry_for(std::shared_ptr<s_npc_barter> shop,unsigned index){
    auto it=shop->items.find(index);check(it!=shop->items.end(),"all nine loaded recipes");return it->second;
}
e_purchase_result buy(std::shared_ptr<s_npc_barter> shop,unsigned index,uint32 amount=1){
    std::vector<s_barter_purchase> cart={{entry_for(shop,index),amount,nullptr}};
    return audit_npc_barter_purchase(*attached,shop,cart);
}
void callback(npc_data& nd,const Recipe& r){
    nd.id=NPC;nd.type=BL_NPC;quest_npc=&nd;fake_nd=&nd;map_num=1;attached->m=0;map[0].qi_npc={NPC};attached->qi_display.resize(1);
    auto qi=std::make_shared<s_questinfo>();qi->icon=QTYPE_QUEST;qi->color=QMARK_NONE;
    std::string condition="{ if (!$@__SWseen_VAL) { $@__SWseen_VAL=1; $@__SWsafe_VAL=(countitem("+std::to_string(r.costs.begin()->first)+")==0 && countitem("+std::to_string(r.output)+")==1 && Zeny==100); Zeny=0; } achievement_condition(0); }";
    qi->condition=compile(condition,"actual quest-info exchange observer");nd.qi_data.push_back(qi);
}
void clear_callback(){map[0].qi_npc.clear();quest_npc=nullptr;fake_nd=nullptr;}

extern "C" int __wrap_main(int argc,char** argv){
    check(argc==2||argc==3,"explicit inputs");deny_network();static char server[]="alice-exchange-test";SERVER_NAME=server;
    malloc_init();db_init();do_init_database();timer_init();do_init_script();battle_set_defaults();save_settings=0;
    auto text=read(std::string(argv[1])+"/items.yml");auto items=ryml::parse_in_arena(ryml::to_csubstr(text));
    for(auto n:items["Body"])check(item_db.parseBodyNode(n)==1,"effective metadata parses");
    auto* npc=compile(body(read(std::string(argv[1])+"/Alice.txt"),"dali,70,100,4\tscript\tConfused Boy#alice_exchange\t4_M_KID1,{"),"actual Confused Boy");
    if(argc==3){
        auto sd=stock(recipes[0],1,!strcmp(argv[2],"slots"));npc_data nd{};
        if(!strcmp(argv[2],"callback"))callback(nd,recipes[0]);
        walk(npc,{1,1});
        std::printf("LEGACY_PROBE case=%s output=%d zeny=%lld callback_seen=%lld callback_safe=%lld\n",argv[2],count(recipes[0].output),(long long)sd->status.zeny,(long long)nums[add_str("$@__SWseen_VAL")],(long long)nums[add_str("$@__SWsafe_VAL")]);std::fflush(stdout);
        if(!strcmp(argv[2],"slots"))check(count(recipes[0].output)==1&&sd->status.zeny==100,"exchange fits after costs release slots");
        else check(nums[add_str("$@__SWsafe_VAL")]==1,"callbacks observe fully settled exchange");
        clear_callback();
    }else{
        text=read(std::string(argv[1])+"/barters.yml");auto catalog=ryml::parse_in_arena(ryml::to_csubstr(text));
        for(auto n:catalog["Body"])check(barter_db.parseBodyNode(n)==1,"real barter catalog parser");
        auto shop=barter_db.find("barter_alice_equipment");check(shop&&shop->items.size()==9,"exact complete catalog");
        npc_data window{};window.id=99000004;window.type=BL_NPC;window.subtype=NPCTYPE_BARTER;window.u.barter.extended=true;shop_npc=&window;
        {auto sd=stock(recipes[0]);Snapshot s;walk(npc,{2});s.unchanged();check(shop_windows==0,"leaving opens no shop");}
        {auto sd=stock(recipes[0]);Snapshot s;walk(npc,{1});s.unchanged();check(shop_windows==1,"actual native callshop opens extended window");check(sd->npc_shopid==window.id,"actual native callshop selects catalog NPC");check(!sd->state.callshop,"native end consumes one-time callshop flag");}
        for(unsigned index=0;index<9;++index){const auto& r=recipes[index];
            for(int mode=0;mode<9;++mode){
                auto sd=stock(r,mode==6?2:1,mode==1||mode==2,mode==2||mode==7);
                if(mode==3)sd->max_weight=item_db.find(r.output)->weight;
                if(mode==4){put(r.costs.size(),501,1);weight();sd->max_weight=item_db.find(r.output)->weight+item_db.find(501)->weight-1;}
                if(mode==5)sd->status.zeny=r.zeny-1;
                if(mode==8)sd->shop_commit.pending=true;
                if(mode==7){auto& it=sd->inventory.u.items_inventory[0];it.bound=2;it.favorite=1;it.card[1]=123;}
                const item saved=sd->inventory.u.items_inventory[0];Snapshot before;
                auto result=buy(shop,index,mode==6?2:1);
                if(mode==2||mode==4||mode==5||mode==8){check(result!=e_purchase_result::PURCHASE_SUCCEED,"invalid exchange refused");before.unchanged();}
                else{check(result==e_purchase_result::PURCHASE_SUCCEED,"exact valid exchange succeeds");check(count(r.output)==(mode==6?2:1)&&sd->status.zeny==100,"exact output and payment");
                    for(auto cost:r.costs)check(count(cost.first)==(mode==7?1:0),"exact material consumption");
                    if(mode==7){item expected=saved;expected.amount=1;check(!memcmp(&expected,&sd->inventory.u.items_inventory[0],sizeof(item)),"surviving material metadata preserved");}
                    else{Snapshot after;check(buy(shop,index)!=e_purchase_result::PURCHASE_SUCCEED,"replay cannot duplicate output");after.unchanged();}
                }
                sd->shop_commit.pending=false;
            }
            for(auto cost:r.costs){auto sd=stock(r);for(auto& it:sd->inventory.u.items_inventory)if(it.nameid==cost.first)--it.amount;weight();Snapshot s;check(buy(shop,index)!=e_purchase_result::PURCHASE_SUCCEED,"each missing material refuses");s.unchanged();}
            {auto sd=stock(r);sd->inventory.u.items_inventory[0].favorite=1;battle_config.hide_fav_sell=1;Snapshot s;check(buy(shop,index)!=e_purchase_result::PURCHASE_SUCCEED,"configured favorite protection respected");s.unchanged();battle_config.hide_fav_sell=0;}
            {auto sd=stock(r);npc_data nd{};callback(nd,r);check(buy(shop,index)==e_purchase_result::PURCHASE_SUCCEED,"quest callback exchange succeeds");check(nums[add_str("$@__SWseen_VAL")]&&nums[add_str("$@__SWsafe_VAL")],"actual callback sees completed payment/input/output");check(sd->status.zeny==0&&count(r.output)==1,"callback changes wallet only after settlement");clear_callback();}
        }
        for(uint32 amount:{0u,UINT32_MAX}){auto sd=stock(recipes[0]);Snapshot s;check(buy(shop,0,amount)!=e_purchase_result::PURCHASE_SUCCEED,"invalid packet quantity refused");s.unchanged();}
        for(bool short_material:{false,true}){
            Recipe batch{0,0,{}};for(const auto& r:recipes){batch.zeny+=r.zeny;for(auto cost:r.costs)batch.costs[cost.first]+=cost.second;}
            auto sd=stock(batch);std::vector<s_barter_purchase> cart;for(unsigned i=0;i<9;++i)cart.push_back({entry_for(shop,i),1,nullptr});
            if(short_material){--sd->inventory.u.items_inventory[0].amount;weight();}Snapshot before;
            auto result=audit_npc_barter_purchase(*sd,shop,cart);
            if(short_material){check(result!=e_purchase_result::PURCHASE_SUCCEED,"whole cart checks combined shared material costs");before.unchanged();}
            else{check(result==e_purchase_result::PURCHASE_SUCCEED&&sd->status.zeny==100,"nine-recipe cart pays exact combined Zeny");for(const auto& r:recipes)check(count(r.output)==1,"nine-recipe cart grants each exact output once");for(auto cost:batch.costs)check(count(cost.first)==0,"combined shared materials consumed exactly once");}
        }
        barter_db.clear();shop_npc=nullptr;
    }
    script_free_code(npc);attached=nullptr;item_db.clear();do_final_script();timer_final();db_final();
    std::printf("ALICE_EXCHANGE_NATIVE_OK cases=%u assertions=%u recipes=9\n",cases,assertions);malloc_final();return 0;
}
