// Ephemeral raw-drop source reservation; only the reward receipt is persistent.
#include <custom/pet_floor.hpp>
#include <unordered_map>
namespace {
struct PnPetFloorClaim {
 std::shared_ptr<const pn_shop::Commit> request;
 int32 floor_id=0;
 uint32 picker_account=0,picker_character=0;
 bool mvp=false,party_notice=false;
 item prototype{};
 uint16 mob_id=0;
};
std::unordered_map<uint64_t,PnPetFloorClaim> pn_pet_floor_claims;
}
bool pn_pet_floor_raw(const item& egg){
 const auto data=item_db.find(egg.nameid);
 return egg.card[0]!=CARD0_PET && data && (data->type==IT_PETEGG || pet_db_search(egg.nameid,PET_EGG));
}
bool pn_pet_floor_take(map_session_data& picker,flooritem_data& floor,bool mvp){
 if(!pn_pet_floor_raw(floor.item) || floor.pet_claim_token || floor.item.amount<1)return false;
 auto* party=party_search(picker.status.party_id);
 const auto admit=[&](map_session_data* recipient)->int32 {
  auto request=pn_shop_request(*recipient,pn_shop::Asset);pn_shop::Grant grant;
  grant.nameid=floor.item.nameid;grant.amount=floor.item.amount;grant.prototype=floor.item;
  if(!pn_shop_begin(*recipient,request,{grant}))return ADDITEM_INVALID;
  const auto immutable=recipient->shop_commit.request;
  if(!immutable)return ADDITEM_INVALID; // pn_shop_begin must publish the submitted identity.
  floor.pet_claim_token=immutable->sequence;
  PnPetFloorClaim claim;claim.request=immutable;claim.floor_id=floor.id;
  claim.picker_account=picker.status.account_id;claim.picker_character=picker.status.char_id;
  claim.mvp=mvp;claim.prototype=floor.item;claim.mob_id=floor.mob_id;
  claim.party_notice=!mvp && party && battle_config.party_show_share_picker && battle_config.show_picker_item_type&(1<<itemdb_type(floor.item.nameid));
  pn_pet_floor_claims.emplace(static_cast<uint64_t>(immutable->sequence),std::move(claim));return 0;
 };
 return mvp?admit(&picker)==0:party_share_loot_with(party,&picker,&floor.item,floor.first_get_charid,admit,false)==0;
}
void pn_pet_floor_removed(const flooritem_data& floor){
 auto found=pn_pet_floor_claims.find(floor.pet_claim_token);
 if(found!=pn_pet_floor_claims.end() && found->second.floor_id==floor.id)found->second.floor_id=0;
}
std::shared_ptr<const pn_shop::Commit> pn_pet_floor_pending(uint32_t account,uint64_t sequence){
 auto found=pn_pet_floor_claims.find(sequence);
 return found!=pn_pet_floor_claims.end() && found->second.request->account_id==account?found->second.request:nullptr;
}
bool pn_pet_floor_ack(const pn_shop::Ack& reply){
 const uint64_t sequence=reply.sequence;
 auto found=pn_pet_floor_claims.find(sequence);
 if(found==pn_pet_floor_claims.end())return false;
 const auto& r=*found->second.request;
 if(reply.account_id!=r.account_id || reply.char_id!=r.char_id || reply.nonce_hi!=r.nonce_hi || reply.nonce_lo!=r.nonce_lo ||
    (reply.outcome!=pn_shop::Committed && reply.outcome!=pn_shop::Rejected))return false;
 auto claim=std::move(found->second);pn_pet_floor_claims.erase(found);
 auto* floor=BL_CAST(BL_ITEM,map_id2bl(claim.floor_id));
 if(floor && floor->pet_claim_token!=reply.sequence)floor=nullptr;
 if(floor)floor->pet_claim_token=0;
 if(reply.outcome==pn_shop::Rejected)return true; // Original floor remains collectible.
 auto* picker=map_id2sd(claim.picker_account);
 if(picker && picker->status.char_id==claim.picker_character && floor){unit_stop_attack(picker);clif_takeitem(*picker,*floor);}
 auto* recipient=map_id2sd(r.account_id);
 if(recipient && recipient->status.char_id==r.char_id){
  if(claim.mvp)clif_mvp_item(recipient,claim.prototype.nameid);
  if(claim.party_notice)clif_party_show_picker(recipient,&claim.prototype);
  const auto data=item_db.find(claim.prototype.nameid);
  if(claim.mob_id && data && data->flag.broadcast)intif_broadcast_obtain_special_item(recipient,claim.prototype.nameid,claim.mob_id,ITEMOBTAIN_TYPE_MONSTER_ITEM);
 }
 if(floor)map_clearflooritem(floor);
 return true;
}
