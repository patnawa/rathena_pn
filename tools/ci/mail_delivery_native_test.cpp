// GPL-3.0-or-later. Actual RODEX map delivery paths with explicit persistence boundary.
#include <map/mail.hpp>
#include <map/storage.hpp>
#include <map/pc_groups.hpp>
#include <custom/shop_state.hpp>
namespace { bool began=false;std::shared_ptr<pn_shop::Commit> captured;size_t grant_count=0;unsigned acks=0; }
extern "C" bool submit_boundary(map_session_data&,std::shared_ptr<pn_shop::Commit>,std::vector<pn_shop::Event>,uint32) asm("__wrap__Z14pn_shop_submitR16map_session_dataSt10shared_ptrIN7pn_shop6CommitEESt6vectorINS2_5EventESaIS6_EEj");
extern "C" bool submit_boundary(map_session_data& sd,std::shared_ptr<pn_shop::Commit> request,std::vector<pn_shop::Event> events,uint32){request->nonce_hi=1;request->sequence=1;check(pn_shop::valid(*request),"native planned request satisfies wire validation");began=true;captured=std::move(request);grant_count=events.size();sd.shop_commit.pending=true;return true;}
extern "C" void mail_ack(map_session_data*,mail_send_result) asm("__wrap__Z14clif_Mail_sendP16map_session_data16mail_send_result");
extern "C" void mail_ack(map_session_data*,mail_send_result){++acks;}
extern "C" void claim_ack(map_session_data*,mail_message*,uint8,mail_attachment_type) asm("__wrap__Z23clif_mail_getattachmentP16map_session_dataP12mail_messageh20mail_attachment_type");
extern "C" void claim_ack(map_session_data*,mail_message*,uint8,mail_attachment_type){++acks;}
namespace {
RunePlayer actor(){auto sd=rune_player();sd->group=std::make_shared<s_player_group>();sd->permissions.set(PC_PERM_TRADE);sd->rental_timer=INVALID_TIMER;actors={sd.get()};attached=sd.get();began=false;captured.reset();grant_count=acks=0;return sd;}
mail_message message(int id=7001){mail_message msg{};msg.id=id;msg.send_id=123;return msg;}
void zeny_only(){
    auto sd=actor();auto msg=message();msg.zeny=500;
    const bool handled=pn_mail_getattachment_atomic(*sd,msg,MAIL_ATT_ZENY);
    std::printf("MAIL_ZENY_ATOMIC_PROBE handled=%d began=%d grants=%zu\n",handled,began,grant_count);std::fflush(stdout);
    check(handled&&began&&captured&&captured->mail_id==msg.id&&captured->mail_zeny==500&&captured->wallet_before==7654321&&captured->wallet_after==7654821&&grant_count==0,"zeny-only claim uses the durable mail asset transaction");
}
void legacy_reply(){
    auto sd=actor();auto msg=message();sd->mail.pending_zeny=0;const auto wallet=sd->status.zeny;item items[MAIL_MAX_ITEM]{};
    mail_getattachment(sd.get(),&msg,100,items);
    std::printf("MAIL_LEGACY_PROBE pending=%lld wallet=%lld\n",static_cast<long long>(sd->mail.pending_zeny),static_cast<long long>(sd->status.zeny));std::fflush(stdout);
    check(sd->mail.pending_zeny>=0&&sd->status.zeny==wallet,"unreserved legacy reply cannot poison pending Zeny or credit a new session");
}
void send_failure(){
    auto sd=actor();sd->status.inventory_slots=1;battle_config.mail_attachment_price=2500;battle_config.mail_zeny_fee=0;battle_config.mail_daily_count=0;put(0,909,10);weight();
    check(mail_setitem(sd.get(),2,10)==MAIL_ATTACH_SUCCESS,"mail attachment selected");const auto wallet=sd->status.zeny;
    mail_send(sd.get(),"Recipient","Attachment test","body",4);
    check(began&&captured&&captured->kind==pn_shop::MailSend,"send uses durable inventory/mail transaction");
    check(count(909)==10&&sd->status.zeny==wallet,"unacknowledged send keeps original assets");
    item filler{};filler.nameid=501;filler.identify=1;filler.amount=1;
    check(pc_additem(sd.get(),&filler,1,LOG_TYPE_NONE)!=ADDITEM_SUCCESS,"pending send fences competing delivery");
    sd->shop_commit={};pn_mail_asset_result(*sd,*captured,false);
    std::printf("MAIL_FAILURE_PROBE restored=%d filler=%d\n",count(909),count(501));std::fflush(stdout);
    check(count(909)==10&&sd->status.zeny==wallet,"failed send preserves original item and exact fee");
}
void claim_controls(){
    for(int mode=0;mode<7;++mode){auto sd=actor();auto& msg=sd->mail.inbox.msg[0];msg=message();msg.zeny=100;msg.item[0].nameid=909;msg.item[0].amount=3;msg.item[0].identify=1;
        int type=MAIL_ATT_ZENY;
        if(mode==0)sd->status.zeny=INT64_MAX-100;
        if(mode==1)sd->status.zeny=INT64_MAX-99;
        if(mode==2)sd->mail.pending_zeny=1;
        if(mode==3)sd->bank_ui.pending=true;
        if(mode==4)type=MAIL_ATT_ITEM;
        if(mode==5)type=MAIL_ATT_ALL;
        if(mode==6){sd->status.inventory_slots=1;put(0,501,1);weight();}
        check(pn_mail_getattachment_atomic(*sd,msg,type),"claim dispatch owns requested assets");
        const bool expected=mode!=1&&mode!=2&&mode!=3;
        check(began==expected,"claims honor wallet headroom and pending locks; money needs no slot");
        if(!began)continue;
        auto before=msg;pn_mail_asset_result(*sd,*captured,false);check(!memcmp(&before,&msg,sizeof(msg)),"rejected claim preserves cached attachment");
        pn_mail_asset_result(*sd,*captured,true);
        check(msg.item[0].nameid==(type&MAIL_ATT_ITEM?0:909)&&msg.zeny==(type&MAIL_ATT_ZENY?0:100),"claim clears only the committed asset type");
    }
}
void send_controls(){
    for(int mode=0;mode<14;++mode){auto sd=actor();battle_config.mail_daily_count=0;battle_config.mail_attachment_price=25;battle_config.mail_zeny_fee=10;
        put(0,909,10);weight();sd->mail.item[0].index=0;sd->mail.item[0].nameid=909;sd->mail.item[0].amount=3;sd->mail.zeny=100;
        const char* title="Test";
        if(mode==0)title="";
        if(mode==1)sd->mail.item[0].index=-1;
        if(mode==2)sd->mail.item[0].index=MAX_INVENTORY;
        if(mode==3)sd->mail.item[0].amount=-1;
        if(mode==4)sd->mail.item[0].amount=11;
        if(mode==5)sd->inventory_data[0]=nullptr;
        if(mode==6)sd->mail.item[1]=sd->mail.item[0];
        if(mode==7)sd->status.zeny=134;
        if(mode==8)sd->bank_ui.pending=true;
        if(mode==9)sd->inventory.u.items_inventory[0].expire_time=2100000000;
        if(mode==10)sd->inventory.u.items_inventory[0].bound=BOUND_ACCOUNT;
        if(mode==11)sd->mail.zeny=INT64_MAX;
        if(mode==12)sd->inventory.u.items_inventory[0].option[0].value=23;
        if(mode==13){sd->mail.item[0]={};sd->mail.zeny=0;}
        const auto before=sd->inventory;const auto wallet=sd->status.zeny;
        mail_send(sd.get(),"Recipient",title,"text",4);
        check(began==(mode>=12),"send validates original selection, wallet, metadata and busy state");
        check(!memcmp(&before,&sd->inventory,sizeof(before))&&sd->status.zeny==wallet,"send admission never debits assets before receipt");
        if(mode==12){check(captured->items[0].amount==7&&captured->wallet_after==wallet-135,"partial send plans exact source and fees");check(captured->outgoing.attachments[0].option[0].value==23,"send retains item metadata");}
    }
}
}
extern "C" int __wrap_main(int argc,char** argv){
    check(argc==2||argc==3,"artifact input");deny_network();static char name[]="mail-test";SERVER_NAME=name;
    malloc_init();db_init();do_init_database();timer_init();do_init_script();battle_set_defaults();save_settings=0;map_num=1;map[0].instance_id=0;map[0].initMapFlags();
    num_reg_ers=ers_new(sizeof(script_reg_num),"mail:num",(ERSOptions)(ERS_OPT_CLEAN|ERS_OPT_FLEX_CHUNK));str_reg_ers=ers_new(sizeof(script_reg_str),"mail:str",(ERSOptions)(ERS_OPT_CLEAN|ERS_OPT_FLEX_CHUNK));pc_set_reg_load(false);
    auto data=read(std::string(argv[1])+"/items.yml");auto tree=ryml::parse_in_arena(ryml::to_csubstr(data));for(auto n:tree["Body"])check(item_db.parseBodyNode(n)==1,"effective metadata parses");
    if(argc==2||!strcmp(argv[2],"zeny"))zeny_only();
    if(argc==2||!strcmp(argv[2],"legacy"))legacy_reply();
    if(argc==2||!strcmp(argv[2],"failure"))send_failure();
    if(argc==2){claim_controls();send_controls();}
    check(!errors,"no script errors");actors.clear();attached=nullptr;captured.reset();item_db.clear();do_final_script();ers_destroy(num_reg_ers);ers_destroy(str_reg_ers);num_reg_ers=str_reg_ers=nullptr;timer_final();db_final();malloc_final();
    std::printf("MAIL_DELIVERY_NATIVE_OK cases=%u assertions=%u\n",cases,assertions);return 0;
}
