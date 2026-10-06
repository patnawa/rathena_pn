#include <custom/shop_state.hpp>
// Copyright (c) rAthena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "mail.hpp"
#include <custom/zeny_arithmetic.hpp>

#include <common/nullpo.hpp>
#include <common/showmsg.hpp>
#include <common/strlib.hpp>
#include <common/timer.hpp>
#include <common/utilities.hpp>

#include "atcommand.hpp"
#include "battle.hpp"
#include "clif.hpp"
#include "date.hpp" // date_get_dayofyear
#include "intif.hpp"
#include "itemdb.hpp"
#include "log.hpp"
#include "pc.hpp"
#include "pet.hpp"

using namespace rathena;

void mail_clear(map_session_data *sd)
{
	int32 i;

	for( i = 0; i < MAIL_MAX_ITEM; i++ ){
		sd->mail.item[i].nameid = 0;
		sd->mail.item[i].index = 0;
		sd->mail.item[i].amount = 0;
	}
	sd->mail.zeny = 0;
	sd->mail.dest_id = 0;

	return;
}

int32 mail_removeitem(map_session_data *sd, int16 flag, int32 idx, int32 amount)
{
	int32 i;

	nullpo_ret(sd);

	idx -= 2;

	if( idx < 0 || idx >= MAX_INVENTORY )
			return false;
	if( amount <= 0 || amount > sd->inventory.u.items_inventory[idx].amount )
			return false;

	ARR_FIND(0, MAIL_MAX_ITEM, i, sd->mail.item[i].index == idx && sd->mail.item[i].nameid > 0);

	if( i == MAIL_MAX_ITEM ){
		return false;
	}

	if( flag ){
		if( battle_config.mail_attachment_price > 0 ){
			if( pc_payzeny( sd, battle_config.mail_attachment_price, LOG_TYPE_MAIL ) ){
				return false;
			}
		}

#if PACKETVER < 20150513
		// With client update packet
		pc_delitem(sd, idx, amount, 1, 0, LOG_TYPE_MAIL);
#else
		// RODEX refreshes the client inventory from the ACK packet
		pc_delitem(sd, idx, amount, 0, 0, LOG_TYPE_MAIL);
#endif
	}else{
		sd->mail.item[i].amount -= amount;

		// Item was removed completely
		if( sd->mail.item[i].amount <= 0 ){
			// Move the rest of the array forward
			for( ; i < MAIL_MAX_ITEM - 1; i++ ){
				if ( sd->mail.item[i + 1].nameid == 0 ){
					break;
				}

				sd->mail.item[i].index = sd->mail.item[i+1].index;
				sd->mail.item[i].nameid = sd->mail.item[i+1].nameid;
				sd->mail.item[i].amount = sd->mail.item[i+1].amount;
			}

			// Zero the rest
			for( ; i < MAIL_MAX_ITEM; i++ ){
				sd->mail.item[i].index = 0;
				sd->mail.item[i].nameid = 0;
				sd->mail.item[i].amount = 0;
			}
		}

#if PACKETVER < 20150513
		clif_additem(sd, idx, amount, 0);
#else
		clif_mail_removeitem(sd, true, idx + 2, amount);
#endif
	}

	return 1;
}

bool mail_removezeny( map_session_data *sd, bool flag ){
	nullpo_retr( false, sd );

	if( sd->mail.zeny > 0 ){
		//Zeny send
		if( flag ){
			std::int64_t zeny;
			if (!pn_zeny::fee_total(sd->mail.zeny, battle_config.mail_zeny_fee, 0, zeny)) return false;

			// It's possible that we don't know what the dest_id is, so it will be 0
			if( pc_payzeny( sd, zeny, LOG_TYPE_MAIL, sd->mail.dest_id ) ){
				return false;
			}
		}else{
			// Update is called by pc_payzeny, so only call it in the else condition
			clif_updatestatus(*sd, SP_ZENY);
		}
	}

	sd->mail.zeny = 0;

	return true;
}

/**
* Attempt to set item or zeny to a mail
* @param sd : player attaching the content
* @param idx 0 - Zeny; >= 2 - Inventory item
* @param amount : amout of zeny or number of item
* @return see enum mail_attach_result in mail.hpp
*/
enum mail_attach_result mail_setitem(map_session_data *sd, int16 idx, int64 amount) {
	if( amount < 0 || pc_istrading(sd) )
		return MAIL_ATTACH_ERROR;

	if( idx == 0 ) { // Zeny Transfer
		if( !pc_can_give_items(sd) )
			return MAIL_ATTACH_UNTRADEABLE;

#if PACKETVER < 20150513
		if( amount > sd->status.zeny )
			amount = sd->status.zeny; // TODO: confirm this behavior for old mail system
#else
		std::int64_t total;
		if( !pn_zeny::fee_total(amount, battle_config.mail_zeny_fee, 0, total) || total > sd->status.zeny )
			return MAIL_ATTACH_ERROR;
#endif

		sd->mail.zeny = amount;
		// clif_updatestatus(*sd, SP_ZENY);
		return MAIL_ATTACH_SUCCESS;
	} else { // Item Transfer
		if (amount <= 0 || amount > MAX_AMOUNT) return MAIL_ATTACH_ERROR;
		int32 i;
#if PACKETVER >= 20150513
		int32 j, total = 0;
#endif

		idx -= 2;

		if( idx < 0 || idx >= MAX_INVENTORY || sd->inventory_data[idx] == nullptr )
			return MAIL_ATTACH_ERROR;

		if (itemdb_ishatched_egg(&sd->inventory.u.items_inventory[idx]))
			return MAIL_ATTACH_ERROR;

		if( sd->inventory.u.items_inventory[idx].equipSwitch ){
			return MAIL_ATTACH_EQUIPSWITCH;
		}

#if PACKETVER < 20150513
		i = 0;
		// Remove existing item
		mail_removeitem(sd, 0, sd->mail.item[i].index + 2, sd->mail.item[i].amount);
#else
		ARR_FIND(0, MAIL_MAX_ITEM, i, sd->mail.item[i].index == idx && sd->mail.item[i].nameid > 0 );
		
		// The same item had already been added to the mail
		if( i < MAIL_MAX_ITEM ){
			// Check if it is stackable
			if( !itemdb_isstackable(sd->mail.item[i].nameid) ){
				return MAIL_ATTACH_ERROR;
			}

			// Check if it exceeds the total amount
			if( ( amount + sd->mail.item[i].amount ) > sd->inventory.u.items_inventory[idx].amount ){
				return MAIL_ATTACH_ERROR;
			}

			// Check if it exceeds the total weight
			if( battle_config.mail_attachment_weight ){
				// Sum up all items to get the current total weight
				for( j = 0; j < MAIL_MAX_ITEM; j++ ){
					if (sd->mail.item[j].nameid == 0)
						continue;
					if (sd->inventory_data[sd->mail.item[j].index] == nullptr) {
						return MAIL_ATTACH_ERROR;
					}
					total += sd->mail.item[j].amount * ( sd->inventory_data[sd->mail.item[j].index]->weight / 10 );
				}

				// Add the newly added weight to the current total
				total += amount * sd->inventory_data[idx]->weight / 10;

				if( total > battle_config.mail_attachment_weight ){
					return MAIL_ATTACH_WEIGHT;
				}
			}

			sd->mail.item[i].amount += amount;

			return MAIL_ATTACH_SUCCESS;
		}else{
			ARR_FIND(0, MAIL_MAX_ITEM, i, sd->mail.item[i].nameid == 0);

			if( i == MAIL_MAX_ITEM ){
				return MAIL_ATTACH_SPACE;
			}

			// Check if it exceeds the total weight
			if( battle_config.mail_attachment_weight ){
				// Only need to sum up all entries until the new entry
				for( j = 0; j < i; j++ ){
					if (sd->inventory_data[sd->mail.item[j].index] == nullptr) {
						return MAIL_ATTACH_ERROR;
					}
					total += sd->mail.item[j].amount * ( sd->inventory_data[sd->mail.item[j].index]->weight / 10 );
				}

				// Add the newly added weight to the current total
				total += amount * sd->inventory_data[idx]->weight / 10;

				if( total > battle_config.mail_attachment_weight ){
					return MAIL_ATTACH_WEIGHT;
				}
			}
		}
#endif

		if( amount > sd->inventory.u.items_inventory[idx].amount )
			return MAIL_ATTACH_ERROR;
		if( !pc_can_give_items(sd) || sd->inventory.u.items_inventory[idx].expire_time
			|| !itemdb_available(sd->inventory.u.items_inventory[idx].nameid)
			|| !itemdb_canmail(&sd->inventory.u.items_inventory[idx],pc_get_group_level(sd))
			|| (sd->inventory.u.items_inventory[idx].bound && !pc_can_give_bounded_items(sd)) )
			return MAIL_ATTACH_UNTRADEABLE;

		sd->mail.item[i].index = idx;
		sd->mail.item[i].nameid = sd->inventory.u.items_inventory[idx].nameid;
		sd->mail.item[i].amount = amount;
		return MAIL_ATTACH_SUCCESS;
	}
}

bool mail_setattachment(map_session_data *sd, struct mail_message *msg)
{
	int32 i, amount;

	nullpo_retr(false,sd);
	nullpo_retr(false,msg);

	for( i = 0, amount = 0; i < MAIL_MAX_ITEM; i++ ){
		int32 index = sd->mail.item[i].index;

		if( sd->mail.item[i].nameid == 0 || sd->mail.item[i].amount == 0 ){
			memset(&msg->item[i], 0x00, sizeof(struct item));
			continue;
		}

		amount++;

		if( sd->inventory.u.items_inventory[index].nameid != sd->mail.item[i].nameid )
			return false;

		if( sd->inventory.u.items_inventory[index].amount < sd->mail.item[i].amount )
			return false;

		if( sd->weight > sd->max_weight ) // TODO: Why check something weird like this here?
			return false;

		memcpy(&msg->item[i], &sd->inventory.u.items_inventory[index], sizeof(struct item));
		msg->item[i].amount = sd->mail.item[i].amount;
	}

	std::int64_t total;
	if( !pn_zeny::fee_total(sd->mail.zeny, battle_config.mail_zeny_fee, static_cast<int64>(amount) * battle_config.mail_attachment_price, total) || total > sd->status.zeny )
		return false;

	msg->zeny = sd->mail.zeny;

	// Removes the attachment from sender
	for( i = 0; i < MAIL_MAX_ITEM; i++ ){
		if( sd->mail.item[i].nameid == 0 || sd->mail.item[i].amount == 0 ){
			// Exit the loop on the first empty entry
			break;
		}

		mail_removeitem(sd,1,sd->mail.item[i].index + 2,sd->mail.item[i].amount);
	}
	mail_removezeny(sd,true);

	return true;
}

// Runs before legacy attachment deletion or pending-capacity reservations.
bool pn_mail_getattachment_atomic(map_session_data& sd,mail_message& msg,int32 type) {
    if(!(type&MAIL_ATT_ALL))return false;
    // Every item extraction uses this path, including ordinary items and
    // existing encoded eggs. Catalog reload cannot reopen the legacy raw-egg
    // window between attachment deletion and map delivery.
    bool any=false;
    for(const auto& attachment:msg.item)if(attachment.nameid && attachment.amount>0)any=true;
    const bool take_items=(type&MAIL_ATT_ITEM) && any;
    const bool take_zeny=(type&MAIL_ATT_ZENY) && msg.zeny>0;
    if(!take_items && !take_zeny)return true;
    auto request=pn_shop_request(sd,pn_shop::Asset);
    request->mail_id=msg.id;
    if(take_items)memcpy(request->mail_items,msg.item,sizeof(request->mail_items));
    if(take_zeny) {
        if(msg.zeny<0 || sd.status.zeny<0 || msg.zeny>MAX_WALLET_ZENY-sd.status.zeny){clif_mail_getattachment(&sd,&msg,1,MAIL_ATT_ZENY);return true;}
        request->mail_zeny=msg.zeny;
        request->wallet_after+=msg.zeny;
    }
    std::vector<pn_shop::Grant> grants;
    for(const auto& attachment:request->mail_items)if(attachment.nameid && attachment.amount>0) {
        pn_shop::Grant grant{};grant.nameid=attachment.nameid;grant.amount=attachment.amount;grant.prototype=attachment;
        grants.push_back(grant);
    }
    if(!pn_shop_begin(sd,request,grants)) {
        if(take_items)clif_mail_getattachment(&sd,&msg,2,MAIL_ATT_ITEM);
        if(take_zeny)clif_mail_getattachment(&sd,&msg,1,MAIL_ATT_ZENY);
    }
    return true;
}

void pn_mail_asset_result(map_session_data& sd,const pn_shop::Commit& request,bool committed) {
    if(request.kind==pn_shop::MailSend) {
        if(committed) {
            mail_clear(&sd);
            sd.state.mail_writing=false;
            if(battle_config.mail_daily_count) {
                mail_refresh_remaining_amount(&sd);
                auto* count=sd.sc.getSCE(SC_DAILYSENDMAILCNT);
                if(count)sc_start2(&sd,&sd,SC_DAILYSENDMAILCNT,100,date_get_dayofyear(),count->val2+1,INFINITE_TICK);
            }
        }
        clif_Mail_send(&sd,committed?WRITE_MAIL_SUCCESS:WRITE_MAIL_FAILED);
        return;
    }
    if(!request.mail_id)return;
    const bool items=std::any_of(std::begin(request.mail_items),std::end(request.mail_items),[](const item& it){return it.nameid!=0;});
    for(auto& msg:sd.mail.inbox.msg)if(msg.id==request.mail_id) {
        if(committed){if(items)memset(msg.item,0,sizeof(msg.item));if(request.mail_zeny)msg.zeny=0;}
        if(items)clif_mail_getattachment(&sd,&msg,committed?0:2,MAIL_ATT_ITEM);
        if(request.mail_zeny)clif_mail_getattachment(&sd,&msg,committed?0:1,MAIL_ATT_ZENY);
        break;
    }
}

void mail_getattachment(map_session_data* sd, struct mail_message* msg, int64 zeny, struct item* item){
    // All current claims use immutable Asset receipts. A stale legacy reply
    // cannot identify the session that owned it and must not credit this one.
    return;
}

int32 mail_openmail( const map_session_data* sd )
{
	nullpo_ret(sd);

	if( sd->state.storage_flag || sd->state.vending || sd->state.buyingstore || sd->state.trading )
		return 0;

	clif_Mail_window(sd->fd, 0);

	return 1;
}

void mail_deliveryfail(map_session_data *sd, struct mail_message *msg){
	nullpo_retv(sd);
	nullpo_retv(msg);
	// Player sends now settle through MailSend receipts. Legacy/system mail
	// acknowledgements never own a debit on the currently connected session.
	clif_Mail_send(sd, WRITE_MAIL_FAILED);
}

// This function only check if the mail operations are valid
bool mail_invalid_operation( const map_session_data* sd )
{
	if(pc_transaction_pending(sd))return true;
#if PACKETVER < 20150513
	if( !map_getmapflag(sd->m, MF_TOWN) && !pc_can_use_command(sd, "mail", COMMAND_ATCOMMAND) )
	{
		ShowWarning("clif_parse_Mail: char '%s' trying to do invalid mail operations.\n", sd->status.name);
		return true;
	}
#else
	// RODEX transfers must not overlap NPC shops, storage, or trading.
	// mail_writing is intentionally allowed: attaching and sending use this check.
	if( sd->npc_id || sd->npc_shopid || sd->state.storage_flag || sd->state.trading
		|| sd->state.vending || sd->state.buyingstore ){
		return true;
	}

	if( map_getmapflag( sd->m, MF_NORODEX ) ){
		clif_displaymessage( sd->fd, msg_txt( sd, 796 ) ); // You cannot use RODEX on this map.
		return true;
	}
#endif

	return false;
}

/**
* Attempt to send mail
* @param sd Sender
* @param dest_name Destination name
* @param title Mail title
* @param body_msg Mail message
* @param body_len Message's length
*/
void mail_send(map_session_data *sd, const char *dest_name, const char *title, const char *body_msg, int32 body_len) {
	nullpo_retv(sd);
    if(mail_invalid_operation(sd))return;
    auto fail=[&](){clif_Mail_send(sd,WRITE_MAIL_FAILED);};
    if(!dest_name || !*dest_name || !title || !*title || body_len<0 ||
       (body_len && !body_msg) || !pc_can_give_items(sd) || DIFF_TICK(sd->cansendmail_tick,gettick())>0) {fail();return;}
    if(battle_config.mail_daily_count) {
        mail_refresh_remaining_amount(sd);
        auto* count=sd->sc.getSCE(SC_DAILYSENDMAILCNT);
        if(!count || count->val2>=battle_config.mail_daily_count){clif_Mail_send(sd,WRITE_MAIL_FAILED_CNT);return;}
    }
    auto request=pn_shop_request(*sd,pn_shop::MailSend);
    auto& envelope=*new(&request->outgoing) pn_shop::MailEnvelope{};
    safestrncpy(envelope.recipient,dest_name,sizeof(envelope.recipient));
    safestrncpy(envelope.title,title,sizeof(envelope.title));
    if(body_len)safestrncpy(envelope.body,body_msg,std::min<size_t>(static_cast<size_t>(body_len)+1,sizeof(envelope.body)));
    envelope.zeny=sd->mail.zeny;
    envelope.fee_percent=battle_config.mail_zeny_fee;
    envelope.attachment_price=battle_config.mail_attachment_price;
    uint32 required[MAX_INVENTORY]{};uint32 count=0;
    for(int i=0;i<MAIL_MAX_ITEM;++i) {
        const auto& selected=sd->mail.item[i];
        if(!selected.nameid && !selected.amount)continue;
        const int index=selected.index;
        if(index<0 || index>=sd->status.inventory_slots || index>=MAX_INVENTORY ||
           selected.amount<=0 || selected.amount>MAX_AMOUNT || required[index]){fail();return;}
        const auto& source=sd->inventory.u.items_inventory[index];
        auto info=item_db.find(source.nameid);
        if(!info || info.get()!=sd->inventory_data[index] || source.nameid!=selected.nameid ||
           source.amount<selected.amount || source.equip || source.equipSwitch || source.expire_time ||
           itemdb_ishatched_egg(&source) || !itemdb_available(source.nameid) || !itemdb_canmail(&source,pc_get_group_level(sd)) ||
           (source.bound && !pc_can_give_bounded_items(sd))){fail();return;}
        auto& output=envelope.attachments[i];output=source;output.id=0;output.amount=selected.amount;
        required[index]=selected.amount;++count;
    }
    int64 total=0;
    if(!pn_zeny::fee_total(envelope.zeny,envelope.fee_percent,static_cast<int64>(count)*envelope.attachment_price,total) || total>sd->status.zeny){fail();return;}
    request->wallet_after-=total;
    // Keep the inventory and wallet intact until the mail header, attachments,
    // debit and receipt have committed together. Rejection needs no refund.
    if(!pn_shop_begin(*sd,request,{},required)){fail();return;}
    sd->cansendmail_tick=gettick()+battle_config.mail_delay;
}

void mail_refresh_remaining_amount( map_session_data* sd ){
	int32 doy = date_get_dayofyear();

	nullpo_retv(sd);

	// If it was not yet started or it was started on another day
	if( sd->sc.getSCE(SC_DAILYSENDMAILCNT) == nullptr || sd->sc.getSCE(SC_DAILYSENDMAILCNT)->val1 != doy ){
		sc_start2( sd, sd, SC_DAILYSENDMAILCNT, 100, doy, 0, INFINITE_TICK );
	}
}
