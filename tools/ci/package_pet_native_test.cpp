#include <custom/shop_state.hpp>
#include <custom/retired_tokens.hpp>
#include <map/pet.hpp>
#include <map/mob.hpp>
#include <map/packets_struct.hpp>
#include <common/utils.hpp>
#include <common/nullpo.hpp>
#include <common/strlib.hpp>
#include <map/atcommand.hpp>
const char* fixture_parent_cmd=nullptr;
using namespace rathena;
PACKET_CZ_USE_PACKAGEITEM packet{};
int mail_acks=0,mail_error=0;
void fixture_mail_ack(map_session_data*,mail_message*,int error,int){++mail_acks;mail_error=error;}
bool allow_commit=true;
std::shared_ptr<pn_shop::Commit> captured;
bool audit_plan(const map_session_data&,pn_shop::Commit&,const std::vector<pn_shop::Grant>&,const uint32_t*,std::vector<pn_shop::Event>&,uint32_t&);
bool fixture_begin(map_session_data& sd,std::shared_ptr<pn_shop::Commit> request,const std::vector<pn_shop::Grant>& grants,const uint32_t* costs=nullptr){
 if(!allow_commit)return false;
 std::vector<pn_shop::Event> events;uint32_t weight=0;
 if(!audit_plan(sd,*request,grants,costs,events,weight))return false;
 captured=request;return true;
}
// PRODUCTION
extern "C" int __wrap_main(int argc,char** argv){
 deny_network();static char server[]="package-pet-test";SERVER_NAME=server;
 malloc_init();db_init();do_init_database();timer_init();do_init_script();battle_set_defaults();
 auto data=read(std::string(argv[1])+"/items.yml");auto items=ryml::parse_in_arena(ryml::to_csubstr(data));
 for(auto n:items["Body"])check(item_db.parseBodyNode(n)==1,"item parses");
 item_db.find(501)->type=IT_PETEGG;
 auto pet=std::make_shared<s_pet_db>();pet->class_=1002;pet->EggID=501;pet->intimate=250;pet_db.put(1002,pet);
 auto mob=std::make_shared<s_mob_db>();mob->id=1002;mob->lv=1;mob->jname="Package pet";mob_db.put(1002,mob);
 for(int mode=0;mode<8;++mode){
  ++cases;captured.reset();allow_commit=mode!=1;
  auto sd=std::make_unique<map_session_data>();attached=sd.get();sd->type=BL_PC;
  sd->status.account_id=99001;sd->status.char_id=99000002;sd->status.uniqueitem_counter=10;
  sd->status.inventory_slots=1;sd->status.base_level=200;sd->status.zeny=1000;sd->max_weight=1000000;
  for(auto& index:sd->equip_index)index=-1;
  put(0,502,mode==2?2:1);weight();const auto before=sd->inventory;const auto before_weight=sd->weight;
  auto package=std::make_shared<s_item_package>();package->item_id=502;
  auto group=std::make_shared<s_item_package_group>();group->groupIndex=1;
  auto reward=std::make_shared<s_item_package_item>();reward->item_id=501;reward->amount=mode==3?3:1;reward->refine=2;reward->grade=1;
  if(mode==5)reward->amount=0;
  group->items[501]=reward;
  if(mode==4 || mode==6){auto other=std::make_shared<s_item_package_item>();other->item_id=503;other->amount=mode==6?2:1;other->rentalhours=mode==6?1:0;group->items[503]=other;}
  package->groups[1]=group;item_package_db.put(502,package);
  packet={};packet.AID=sd->status.account_id;packet.index=2;packet.itemID=502;packet.BoxIndex=1;
  if(mode==7)packet.AID++;
  audit_package(0,sd.get());
  const bool success=mode==0 || mode==2 || mode==3 || mode==4;
  bool ok=bool(captured)==success && sd->weight==before_weight && !memcmp(&before,&sd->inventory,sizeof(before)) && sd->status.uniqueitem_counter==10;
  if(captured){
   ok=ok && captured->pet_count==(mode==3?3:1) && captured->kind==pn_shop::Asset;
   for(int i=0;i<captured->pet_count;++i)ok=ok && captured->pets[i].output.egg.refine==2 && captured->pets[i].output.egg.enchantgrade==1;
   if(mode==2)ok=ok && captured->items[0].nameid==502 && captured->items[0].amount==1;
   else if(mode==4)ok=ok && captured->items[0].nameid==503 && captured->items[0].amount==1;
   else ok=ok && !captured->items[0].nameid;
  }
  if(!ok)++errors;printf("PACKAGE_PET mode=%d %s\n",mode,ok?"PASS":"FAIL");
  item_package_db.clear();
 }
 printf("PACKAGE_PET cases=%u failures=%u\n",cases,errors);
 const unsigned package_errors=errors;
 for(int mode=0;mode<8;++mode){
  captured.reset();allow_commit=mode!=1;mail_acks=mail_error=0;
  auto sd=std::make_unique<map_session_data>();attached=sd.get();sd->type=BL_PC;
  sd->status.account_id=99001;sd->status.char_id=99000002;sd->status.uniqueitem_counter=10;
  sd->status.inventory_slots=1;sd->status.zeny=1000;sd->max_weight=1000000;
  put(0,502,1);weight();if(mode>=4)sd->status.inventory_slots=2;
  auto& msg=sd->mail.inbox.msg[0];msg.id=701;msg.zeny=75;
  msg.item[0].nameid=501;msg.item[0].amount=mode==2?2:1;msg.item[0].identify=1;msg.item[0].bound=2;
  if(mode==4){msg.item[1].nameid=503;msg.item[1].amount=1;msg.item[1].identify=1;}
  if(mode==5){msg.item[1].nameid=501;msg.item[1].amount=1;msg.item[1].identify=1;msg.item[1].card[0]=CARD0_PET;msg.item[1].card[1]=42;msg.item[1].unique_id=12345;}
  if(mode==6)msg.item[0].nameid=503;
  if(mode==7){msg.item[0].card[0]=CARD0_PET;msg.item[0].card[1]=43;msg.item[0].unique_id=54321;}
  const auto before=sd->inventory;const auto before_mail=msg;
  bool handled=audit_mail(*sd,msg,mode==3?MAIL_ATT_ALL:MAIL_ATT_ITEM);
  bool ok=handled && bool(captured)==(mode!=1) && !memcmp(&before,&sd->inventory,sizeof(before)) && !memcmp(&before_mail,&msg,sizeof(msg));
  if(captured){
   ok=ok && captured->mail_id==701 && captured->pet_count==(mode>=6?0:mode==2?2:1) && captured->wallet_after==(mode==3?1075:1000) && captured->mail_zeny==(mode==3?75:0);
   if(mode==5)ok=ok && captured->items[1].card[1]==42 && captured->items[1].unique_id==12345;
   audit_mail_result(*sd,*captured,true);
   ok=ok && !msg.item[0].nameid && mail_acks==(mode==3?2:1) && !mail_error && msg.zeny==(mode==3?0:75);
  }else ok=ok && mail_acks==1 && mail_error==2;
  if(!ok)++errors;printf("MAIL_PET mode=%d %s\n",mode,ok?"PASS":"FAIL");
 }
 printf("MAIL_PET cases=8 failures=%u\n",errors-package_errors);
 const unsigned mail_errors=errors;
 for(int mode=0;mode<7;++mode){
  captured.reset();allow_commit=mode!=1;
  auto sd=std::make_unique<map_session_data>();attached=sd.get();sd->type=BL_PC;
  sd->status.account_id=99001;sd->status.char_id=99000002;sd->status.uniqueitem_counter=10;
  sd->status.inventory_slots=mode==3?2:1;sd->status.zeny=1000;sd->max_weight=1000000;
  put(0,502,1);weight();const auto before=sd->inventory;
  int code;
  if(mode<=3)code=fixture_admin_item(0,sd.get(),mode==2?"@itembound":"@item",mode==2?"501 2 2":mode==3?"501:503 1":"501 2");
  else if(mode<=5)code=fixture_admin_item2(0,sd.get(),"@item2",mode==4?"501 2 1 0 0 0 0 0 0":"501 1 1 0 0 1 0 0 0");
  else code=fixture_admin_makeegg(0,sd.get(),"@makeegg","501");
  const bool success=mode!=1 && mode!=5;
  bool ok=(code==0)==success && bool(captured)==success && !memcmp(&before,&sd->inventory,sizeof(before));
  if(captured){ok=ok && captured->pet_count==((mode==3 || mode==6)?1:2);if(mode==2)ok=ok && captured->pets[0].output.egg.bound==2;}
  if(!ok)++errors;printf("ADMIN_PET mode=%d %s\n",mode,ok?"PASS":"FAIL");
 }
 printf("ADMIN_PET cases=7 failures=%u\n",errors-mail_errors);
 captured.reset();attached=nullptr;pet_db.clear();mob_db.clear();pet.reset();mob.reset();item_db.clear();
 do_final_script();timer_final();db_final();malloc_final();return errors?1:0;
}
