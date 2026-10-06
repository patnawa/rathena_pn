// Copyright (c) rAthena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "trade.hpp"
#include <custom/zeny_arithmetic.hpp>

#include <cstdio>
#include <cstring>

#include <common/nullpo.hpp>
#include <common/socket.hpp>

#include "atcommand.hpp"
#include "battle.hpp"
#include "chrif.hpp"
#include "clif.hpp"
#include "intif.hpp"
#include "itemdb.hpp"
#include "log.hpp"
#include "path.hpp"
#include "pc.hpp"
#include "pc_groups.hpp"
#include "storage.hpp"

#define TRADE_DISTANCE 2 ///Max distance from traders to enable a trade to take place.

// A revision binds each companion confirmation to the displayed bilateral offer.
static void trade_revision(map_session_data& sd, map_session_data& other, bool offer_changed = false) {
    const uint64 next = std::max(sd.bank_ui.trade_revision, other.bank_ui.trade_revision) + 1;
    sd.bank_ui.trade_revision = other.bank_ui.trade_revision = next;
    if (offer_changed && (sd.deal.zeny > MAX_ZENY || other.deal.zeny > MAX_ZENY)) {
        sd.state.deal_locked = other.state.deal_locked = 0;
    }
}
// A stale actor must never edit or cancel another actor's newer trade.
static bool trade_reciprocal(const map_session_data& a, const map_session_data& b) {
    return &a != &b && a.trade_partner.id == b.status.account_id &&
        b.trade_partner.id == a.status.account_id &&
        (a.state.trading == b.state.trading) &&
        (!a.state.trading || (a.bank_ui.trade_id &&
            a.bank_ui.trade_id == b.bank_ui.trade_id));
}

static bool trade_wide_required(const map_session_data& sd, const map_session_data& other) {
    return sd.deal.zeny > MAX_ZENY || other.deal.zeny > MAX_ZENY;
}
static void trade_clear_epoch(map_session_data& sd) {
    sd.bank_ui.trade_id = sd.bank_ui.trade_revision = 0;
}

/**
 * Player initiates a trade request.
 * @param sd : player requesting the trade
 * @param target_sd : player requested
 */
void trade_traderequest(map_session_data *sd, map_session_data *target_sd)
{
	nullpo_retv(sd);

	if (map_getmapflag(sd->m, MF_NOTRADE)) {
		clif_displaymessage (sd->fd, msg_txt(sd,272));
		return; //Can't trade in notrade mapflag maps.
	}

	if (target_sd == nullptr || sd == target_sd || sd->state.trading || pc_transaction_pending(sd) || pc_transaction_pending(target_sd)) {
		clif_traderesponse(*sd, TRADE_ACK_CHARNOTEXIST);
		return;
	}

	if (target_sd->npc_id) { // Trade fails if you are using an NPC.
		clif_traderesponse(*sd, TRADE_ACK_FAILED);
		return;
	}

	if (!battle_config.invite_request_check) {
		if (target_sd->guild_invite > 0 || target_sd->party_invite > 0 || target_sd->adopt_invite) {
			clif_traderesponse(*sd, TRADE_ACK_FAILED);
			return;
		}
	}

	if ( sd->trade_partner.id != 0 ) { // If a character tries to trade to another one then cancel the previous one
        trade_tradecancel(sd);
        if (sd->trade_partner.id) return;
	}

	if (target_sd->trade_partner.id != 0) {
		clif_traderesponse(*sd, TRADE_ACK_FAILED); // person is in another trade
		return;
	}

	if (!pc_can_give_items(sd) || !pc_can_give_items(target_sd)) { // check if both GMs are allowed to trade
		clif_displaymessage( sd->fd, msg_txt( sd, 246 ) ); // Your GM level doesn't authorize you to perform this action.
		clif_traderesponse(*sd, TRADE_ACK_FAILED); // GM is not allowed to trade
		return;
	}

	// Players can not request trade from far away, unless they are allowed to use @trade.
	if (!pc_can_use_command(sd, "trade", COMMAND_ATCOMMAND) &&
	    (sd->m != target_sd->m || !check_distance_bl(sd, target_sd, TRADE_DISTANCE))) {
		clif_traderesponse(*sd, TRADE_ACK_TOOFAR);
		return ;
	}

	target_sd->trade_partner.id = sd->status.account_id;
	target_sd->trade_partner.lv = sd->status.base_level;

	sd->trade_partner.id = target_sd->status.account_id;
	sd->trade_partner.lv = target_sd->status.base_level;

	clif_traderequest(*target_sd, sd->status.name);
}


/**
 * Reply to a trade-request.
 * @param sd : player receiving the trade request answer
 * @param type : answer code
 *  0: Char is too far
 *  1: Character does not exist
 *  2: Trade failed
 *  3: Accept
 *  4: Cancel
 * Weird enough, the client should only send 3/4
 * and the server is the one that can reply 0~2
 */
void trade_tradeack(map_session_data *sd, int32 type)
{
	map_session_data *tsd;

	nullpo_retv(sd);

	if (pc_transaction_pending(sd) || sd->state.trading || !sd->trade_partner.id)
		return; // Already trading or no partner set.

	if ((tsd = map_id2sd(sd->trade_partner.id)) == nullptr) {
		clif_traderesponse(*sd, TRADE_ACK_CHARNOTEXIST);
		sd->trade_partner = {0,0};
		return;
	}

	if (pc_transaction_pending(tsd) || tsd->state.trading || tsd->trade_partner.id != sd->id) {
		clif_traderesponse(*sd, TRADE_ACK_FAILED);
		sd->trade_partner = {0,0};
		return; // Already trading or wrong partner.
	}

	if (type == 4) { // Cancel
		clif_traderesponse(*tsd, TRADE_ACK_CANCEL);
		clif_traderesponse(*sd, TRADE_ACK_CANCEL);
		sd->state.deal_locked = 0;
		sd->trade_partner = {0,0};
		tsd->state.deal_locked = 0;
		tsd->trade_partner = {0,0};
		return;
	}

	if (type != 3)
		return; //If client didn't send accept, it's a broken packet?

	// Players can not request trade from far away, unless they are allowed to use @trade.
	// Check here as well since the original character could had warped.
	if (!pc_can_use_command(sd, "trade", COMMAND_ATCOMMAND) &&
	    (sd->m != tsd->m || !check_distance_bl(sd, tsd, TRADE_DISTANCE))) {
		clif_traderesponse(*sd, TRADE_ACK_TOOFAR);
		sd->trade_partner = {0,0};
		tsd->trade_partner = {0,0};
		return;
	}

	// Check if you can start trade.
	if (sd->npc_id || sd->state.vending || sd->state.buyingstore || sd->state.storage_flag ||
		tsd->npc_id || tsd->state.vending || tsd->state.buyingstore || tsd->state.storage_flag) { // Fail
		clif_traderesponse(*sd, TRADE_ACK_FAILED);
		clif_traderesponse(*tsd, TRADE_ACK_FAILED);
		sd->state.deal_locked = 0;
		sd->trade_partner = {0,0};
		tsd->state.deal_locked = 0;
		tsd->trade_partner = {0,0};
		return;
	}

	// Initiate trade
	sd->state.trading = 1;
	tsd->state.trading = 1;
	memset(&sd->deal, 0, sizeof(sd->deal));
	memset(&tsd->deal, 0, sizeof(tsd->deal));
    static uint64 next_trade = 0;
    if (++next_trade == 0) ++next_trade;
    sd->bank_ui.trade_id = tsd->bank_ui.trade_id = next_trade;
    sd->bank_ui.trade_revision = tsd->bank_ui.trade_revision = 1;
    clif_bank_trade_open(*sd);
    clif_bank_trade_open(*tsd);
	clif_traderesponse(*tsd, static_cast<e_ack_trade_response>( type ));
	clif_traderesponse(*sd, static_cast<e_ack_trade_response>( type ));
}

/**
 * Check here hacker for duplicate item in trade
 * normal client refuse to have 2 same types of item (except equipment) in same trade window
 * normal client authorise only no equipped item and only from inventory
 * This function could end player connection if too much hack is detected
 * @param sd : player to check
 * @return -1:zeny hack, 0:all fine, 1:item hack
 */
int32 impossible_trade_check(map_session_data *sd)
{
	struct item inventory[MAX_INVENTORY];
	char message_to_gm[200];
	int32 i, index;

	nullpo_retr(1, sd);

	if(sd->deal.zeny > sd->status.zeny) {
		pc_setglobalreg(sd, add_str("ZENY_HACKER"), 1);
		return -1;
	}

	// get inventory of player
	memcpy(&inventory, &sd->inventory.u.items_inventory, sizeof(struct item) * MAX_INVENTORY);

	// remove this part: arrows can be trade and equipped
	// re-added! [celest]
	// remove equipped items (they can not be trade)
	for (i = 0; i < MAX_INVENTORY; i++)
		if (inventory[i].nameid > 0 && inventory[i].equip && !(inventory[i].equip & EQP_AMMO))
			memset(&inventory[i], 0, sizeof(struct item));

	// check items in player inventory
	for(i = 0; i < 10; i++) {
		if (!sd->deal.item[i].amount)
			continue;

		index = sd->deal.item[i].index;
        if (index < 0 || index >= MAX_INVENTORY || sd->deal.item[i].amount < 0 ||
            !inventory[index].nameid || !sd->inventory_data[index] ||
            sd->inventory_data[index]->nameid != inventory[index].nameid) return 1;

		if (inventory[index].amount < sd->deal.item[i].amount) { // if more than the player have -> hack
			sprintf(message_to_gm, msg_txt(sd,538), sd->status.name, sd->status.account_id); // Hack on trade: character '%s' (account: %d) try to trade more items that he has.
			intif_wis_message_to_gm(wisp_server_name, PC_PERM_RECEIVE_HACK_INFO, message_to_gm);
			sprintf(message_to_gm, msg_txt(sd,539), inventory[index].amount, inventory[index].nameid, sd->deal.item[i].amount); // This player has %d of a kind of item (id: %u), and try to trade %d of them.
			intif_wis_message_to_gm(wisp_server_name, PC_PERM_RECEIVE_HACK_INFO, message_to_gm);
			// if we block people
			if (battle_config.ban_hack_trade < 0) {
				chrif_req_login_operation(-1, sd->status.name, CHRIF_OP_LOGIN_BLOCK, 0, 0, 0); // type: 1 - block
				set_eof(sd->fd); // forced to disconnect because of the hack
				// message about the ban
				strcpy(message_to_gm, msg_txt(sd,540)); //  This player has been definitively blocked.
			// if we ban people
			} else if (battle_config.ban_hack_trade > 0) {
				chrif_req_login_operation(-1, sd->status.name, CHRIF_OP_LOGIN_BAN, battle_config.ban_hack_trade*60, 0, 0); // type: 2 - ban (year, month, day, hour, minute, second)
				set_eof(sd->fd); // forced to disconnect because of the hack
				// message about the ban
				sprintf(message_to_gm, msg_txt(sd,507), battle_config.ban_hack_trade); //  This player has been banned for %d minute(s).
			} else
				// message about the ban
				strcpy(message_to_gm, msg_txt(sd,508)); //  This player hasn't been banned (Ban option is disabled).

			intif_wis_message_to_gm(wisp_server_name, PC_PERM_RECEIVE_HACK_INFO, message_to_gm);
			return 1;
		}

		inventory[index].amount -= sd->deal.item[i].amount; // remove item from inventory
	}
	return 0;
}

/**
 * Checks if trade is possible (against zeny limits, inventory limits, etc)
 * @param sd : player 1 trading
 * @param tsd : player 2 trading
 * @return 0:error, 1:success
 */
// Mirror the actual alternating delivery order using complete stack identity,
// configured stack caps and each character's usable inventory slots.
int32 trade_check(map_session_data *sd, map_session_data *tsd)
{
    if (!sd || !tsd || sd == tsd || sd->status.inventory_slots > MAX_INVENTORY ||
        tsd->status.inventory_slots > MAX_INVENTORY) return 0;
    if(sd->deal.zeny < 0 || sd->deal.zeny > sd->status.zeny || !pn_zeny::room(tsd->status.zeny, tsd->mail.pending_zeny, sd->deal.zeny) ||
       tsd->deal.zeny < 0 || tsd->deal.zeny > tsd->status.zeny || !pn_zeny::room(sd->status.zeny, sd->mail.pending_zeny, tsd->deal.zeny)) return 0;
    item inventories[2][MAX_INVENTORY];
    memcpy(inventories[0],sd->inventory.u.items_inventory,sizeof(inventories[0]));
    memcpy(inventories[1],tsd->inventory.u.items_inventory,sizeof(inventories[1]));
    map_session_data* players[]={sd,tsd};
    uint64 weights[]={sd->weight,tsd->weight};
    for (int row=0;row<10;++row) for (int side=0;side<2;++side) {
        const auto& offer=players[side]->deal.item[row];
        if (!offer.amount) continue;
        const int n=offer.index,amount=offer.amount,other=1-side;
        if (n<0 || n>=MAX_INVENTORY || amount<0 || amount>MAX_AMOUNT) return 0;
        auto& source=inventories[side][n];
        const auto data=item_db.find(source.nameid);
        if (!data || !players[side]->inventory_data[n] || players[side]->inventory_data[n]!=data.get() ||
            amount>source.amount || (data->stack.inventory && amount>data->stack.amount)) return 0;
        const uint64 weight=static_cast<uint64>(data->weight)*amount;
        if (weights[other]+weight>players[other]->max_weight || weights[side]<weight) return 0;
        int slot=MAX_INVENTORY;
        if (itemdb_isstackable2(data.get()) && !source.expire_time && (!data->flag.guid || source.unique_id)) {
            for (int i=0;i<MAX_INVENTORY;++i) if (compare_item(&inventories[other][i],&source)) {
                if (i>=players[other]->status.inventory_slots || amount>MAX_AMOUNT-inventories[other][i].amount ||
                    (data->stack.inventory && amount>data->stack.amount-inventories[other][i].amount)) return 0;
                slot=i;break;
            }
        }
        if (slot==MAX_INVENTORY) {
            for (int i=0;i<players[other]->status.inventory_slots;++i) if (!inventories[other][i].nameid) {slot=i;break;}
            if (slot==MAX_INVENTORY) return 0;
            inventories[other][slot]=source;inventories[other][slot].amount=0;
            inventories[other][slot].equip=inventories[other][slot].equipSwitch=0;
        }
        inventories[other][slot].amount+=amount;
        source.amount-=amount;if (!source.amount) source={};
        weights[side]-=weight;weights[other]+=weight;
    }
    return 1;
}

/**
 * Adds an item/qty to the trade window
 * @param sd : Player requesting to add stuff to the trade
 * @param index : index of item in inventory
 * @param amount : amount of item to add from index
 */
void trade_tradeadditem(map_session_data *sd, int16 index, int16 amount)
{
	map_session_data *target_sd;
	struct item *item;
	int32 trade_i;
	int32 src_lv, dst_lv;

	nullpo_retv(sd);

    if (pc_transaction_pending(sd)) return;
	if( !sd->state.trading || sd->state.deal_locked > 0 )
		return; // Can't add stuff.

	if( (target_sd = map_id2sd(sd->trade_partner.id)) == nullptr ) {
		trade_tradecancel(sd);
		return;
	}

	if( !amount ) { // Why do this.. ~.~ just send an ack, the item won't display on the trade window.
		clif_tradeitemok(*sd, -2, EXITEM_ADD_SUCCEED); // We pass -2 which will becomes 0 in clif_tradeitemok (Official behavior)
		return;
	}

	// Item checks...
	if( index < 0 || index >= MAX_INVENTORY )
		return;
	if( amount < 0 || amount > sd->inventory.u.items_inventory[index].amount )
		return;

    if (!trade_reciprocal(*sd,*target_sd) || pc_transaction_pending(target_sd)) return;
	item = &sd->inventory.u.items_inventory[index];
    const auto data=item_db.find(item->nameid);
    if (!data || sd->inventory_data[index]!=data.get()) return;
	src_lv = pc_get_group_level(sd);
	dst_lv = pc_get_group_level(target_sd);

	if( !itemdb_cantrade(item, src_lv, dst_lv) && // Can't trade
		(pc_get_partner(sd) != target_sd || !itemdb_canpartnertrade(item, src_lv, dst_lv)) ) { // Can't partner-trade
		clif_displaymessage (sd->fd, msg_txt(sd,260));
		clif_tradeitemok(*sd, index, EXITEM_ADD_FAILED_CLOSED);
		return;
	}

	if (itemdb_ishatched_egg(item))
		return;

	if( item->expire_time ) { // Rental System
		clif_displaymessage (sd->fd, msg_txt(sd,260));
		clif_tradeitemok(*sd, index, EXITEM_ADD_FAILED_CLOSED);
		return;
	}

	if( ((item->bound == BOUND_ACCOUNT || item->bound > BOUND_GUILD) || (item->bound == BOUND_GUILD && sd->status.guild_id != target_sd->status.guild_id)) && !pc_can_give_bounded_items(sd) ) { // Item Bound
		clif_displaymessage(sd->fd, msg_txt(sd,293));
		clif_tradeitemok(*sd, index, EXITEM_ADD_FAILED_CLOSED);
		return;
	}

	if( item->equipSwitch ){
		clif_msg( *sd, MSI_SWAP_EQUIPITEM_UNREGISTER_FIRST );
		return;
	}

	// Locate a trade position
	ARR_FIND( 0, 10, trade_i, sd->deal.item[trade_i].index == index || sd->deal.item[trade_i].amount == 0 );
	if( trade_i == 10 ) { // No space left
		// The client does not allow to add more than 10 items, and will show an error message.
		return;
	}

    const bool existing=sd->deal.item[trade_i].amount>0;
    const int offered=existing?sd->deal.item[trade_i].amount:0;
    amount=std::min<int32>(amount,item->amount-offered);
    if (amount<=0) return;
    // Preserve the configured conservative slot policy. Adjusting one offer
    // row never reserves a second slot.
    bool needs_slot=battle_config.trade_count_stackable==1 || !itemdb_isstackable2(data.get());
    if (!needs_slot) {
        needs_slot=true;
        for (auto& received:target_sd->inventory.u.items_inventory)
            if (!item->expire_time && compare_item(&received,item)) {needs_slot=false;break;}
    }
    if (!existing && needs_slot && pc_inventoryblank(target_sd)<sd->deal.inventory_space+1) {
        clif_tradeitemok(*sd,index,EXITEM_ADD_FAILED_OVERCOUNT);return;
    }
    const uint64 weight=static_cast<uint64>(data->weight)*amount;
    if (static_cast<uint64>(target_sd->weight)+sd->deal.weight+weight>target_sd->max_weight) {
        clif_tradeitemok(*sd,index,EXITEM_ADD_FAILED_OVERWEIGHT);return;
    }
    const auto before=sd->deal;
    sd->deal.item[trade_i].index=index;sd->deal.item[trade_i].amount=offered+amount;
    if (!trade_check(sd,target_sd)) {
        sd->deal=before;clif_tradeitemok(*sd,index,EXITEM_ADD_FAILED_OVERCOUNT);return;
    }
    sd->deal.weight+=weight;
    if (!existing && needs_slot) sd->deal.inventory_space++;
    if (item->bound) sd->state.isBoundTrading |= (1<<item->bound);

	clif_tradeitemok(*sd, index, EXITEM_ADD_SUCCEED); // Return the index as it was received
	trade_revision(*sd, *target_sd, true);
	clif_tradeadditem(sd, target_sd, index+2, amount);
}

/**
 * Adds the specified amount of zeny to the trade window
 * This function will check if the player have enough money to do so
 * And if the target player have enough space for that money
 * @param sd : Player who's adding zeny
 * @param amount : zeny amount
 */
void trade_tradeaddzeny(map_session_data* sd, int64 amount)
{
	map_session_data* target_sd;

	nullpo_retv(sd);

    if (pc_transaction_pending(sd)) return;
	if( !sd->state.trading || sd->state.deal_locked > 0 )
		return; //Can't add stuff.

	if( (target_sd = map_id2sd(sd->trade_partner.id)) == nullptr ) {
		trade_tradecancel(sd);
		return;
	}

    if (!trade_reciprocal(*sd,*target_sd) || pc_transaction_pending(target_sd)) return;

	if( amount < 0 || amount > sd->status.zeny || !pn_zeny::room(target_sd->status.zeny, target_sd->mail.pending_zeny, amount) ) { // invalid values, no appropriate packet for it => abort
		trade_tradecancel(sd);
		return;
	}

    const bool previously_wide = trade_wide_required(*sd, *target_sd);
    sd->deal.zeny = amount;
    trade_revision(*sd, *target_sd, true);
    if (previously_wide) sd->state.deal_locked = target_sd->state.deal_locked = 0;
    // The stock item window cannot represent a wide offer. Its financial
    // confirmation path is disabled while the companion owns such an offer.
    clif_tradeadditem(sd, target_sd, 0, amount > MAX_ZENY ? 0 : static_cast<int32>(amount));
    if (amount > MAX_ZENY) {
        clif_displaymessage(sd->fd, "Review the full Zeny offer and confirm in Wallet & Bank.");
        clif_displaymessage(target_sd->fd, "Review the full Zeny offer and confirm in Wallet & Bank.");
    }
}

/**
 * 'Ok' button on the trade window is pressed.
 * @param sd : Player that pressed the button
 */
void trade_tradeok(map_session_data *sd, bool wide)
{
	map_session_data *target_sd;

    nullpo_retv(sd);
    if (pc_transaction_pending(sd)) return;
	if(sd->state.deal_locked || !sd->state.trading)
		return;

	if ((target_sd = map_id2sd(sd->trade_partner.id)) == nullptr) {
		trade_tradecancel(sd);
		return;
	}

    if (!trade_reciprocal(*sd,*target_sd) || pc_transaction_pending(target_sd)) return;
    if (!wide && trade_wide_required(*sd, *target_sd)) {
        clif_displaymessage(sd->fd, "Confirm this Zeny offer in Wallet & Bank."); return;
    }
    sd->state.deal_locked = 1;
    trade_revision(*sd, *target_sd);
	clif_tradeitemok(*sd, -2, EXITEM_ADD_SUCCEED); // We pass -2 which will becomes 0 in clif_tradeitemok (Official behavior)
	clif_tradedeal_lock( *sd, false );
	clif_tradedeal_lock( *target_sd, true );
}

/**
 * 'Cancel' is pressed. (or trade was force-cancelled by the code)
 * @param sd : Player that pressed the button
 */
void trade_tradecancel(map_session_data *sd)
{
    nullpo_retv(sd);
    if (sd->pair_commit.pending) return;
    auto* other=map_id2sd(sd->trade_partner.id);
    if (other && !trade_reciprocal(*sd,*other)) other=nullptr;
    if (other && other->pair_commit.pending) return;
    for (auto* actor:{sd,other}) {
        if (!actor) continue;
        if (actor->state.trading) for (const auto& offer:actor->deal.item)
            if (offer.amount>0 && offer.index>=0 && offer.index<MAX_INVENTORY)
                clif_additem(actor,offer.index,offer.amount,0);
        if (actor->deal.zeny) clif_updatestatus(*actor,SP_ZENY);
        actor->state.isBoundTrading=actor->state.deal_locked=actor->state.trading=0;
        actor->trade_partner={0,0};memset(&actor->deal,0,sizeof(actor->deal));
        trade_clear_epoch(*actor);clif_tradecancelled(*actor);
    }
}

/**
 * Execute the trade
 * lock sd and tsd trade data, execute the trade, clear, then save players
 * @param sd : Player that has click on trade button
 */
void trade_tradecommit(map_session_data *sd, bool wide)
{
	map_session_data *tsd;
	int32 trade_i;

	nullpo_retv(sd);
	if (pc_transaction_pending(sd)) return;

	if (!sd->state.trading || !sd->state.deal_locked) //Locked should be 1 (pressed ok) before you can press trade.
		return;

	if ((tsd = map_id2sd(sd->trade_partner.id)) == nullptr) {
		trade_tradecancel(sd);
		return;
	}

    if (!trade_reciprocal(*sd,*tsd) || pc_transaction_pending(tsd)) {
        trade_tradecancel(sd);return;
    }
    if (!wide && trade_wide_required(*sd, *tsd)) {
        clif_displaymessage(sd->fd, "Confirm this Zeny offer in Wallet & Bank."); return;
    }
    sd->state.deal_locked = 2;
    trade_revision(*sd, *tsd);

	if (tsd->state.deal_locked < 2)
		return; //Not yet time for trading.

	// Now is a good time (to save on resources) to check that the trade can indeed be made and it's not exploitable.
	// check exploit (trade more items that you have)
	if (impossible_trade_check(sd)) {
		trade_tradecancel(sd);
		return;
	}

	// check exploit (trade more items that you have)
	if (impossible_trade_check(tsd)) {
		trade_tradecancel(tsd);
		return;
	}

	// check for full inventory (can not add traded items)
	if (!trade_check(sd,tsd)) { // check the both players
		trade_tradecancel(sd);
		return;
	}

	// Lock both actors before any mutation; persistence owns the whole pair.
	if (!pn_pair_begin(*sd,*tsd,pn_pair::Trade)) { trade_tradecancel(sd); return; }
    {
    PcItemDeliveryScope delivery_a(*sd),delivery_b(*tsd);
	// trade is accepted and correct.
	for( trade_i = 0; trade_i < 10; trade_i++ ) {
		int32 n;
		unsigned char flag = 0;

		if (sd->deal.item[trade_i].amount) {
			n = sd->deal.item[trade_i].index;

			flag = pc_additem(tsd, &sd->inventory.u.items_inventory[n], sd->deal.item[trade_i].amount,LOG_TYPE_TRADE);
            if (flag != ADDITEM_SUCCESS || pc_delitem(sd,n,sd->deal.item[trade_i].amount,1,6,LOG_TYPE_TRADE)) {
                delivery_a.cancel();delivery_b.cancel();pn_pair_abort(*sd,*tsd);trade_tradecancel(sd);return;
            }
			sd->deal.item[trade_i].index = 0;
			sd->deal.item[trade_i].amount = 0;
		}

		if (tsd->deal.item[trade_i].amount) {
			n = tsd->deal.item[trade_i].index;

			flag = pc_additem(sd, &tsd->inventory.u.items_inventory[n], tsd->deal.item[trade_i].amount,LOG_TYPE_TRADE);
            if (flag != ADDITEM_SUCCESS || pc_delitem(tsd,n,tsd->deal.item[trade_i].amount,1,6,LOG_TYPE_TRADE)) {
                delivery_a.cancel();delivery_b.cancel();pn_pair_abort(*sd,*tsd);trade_tradecancel(sd);return;
            }
			tsd->deal.item[trade_i].index = 0;
			tsd->deal.item[trade_i].amount = 0;
		}
	}

	if( sd->deal.zeny ) {
		pc_payzeny(sd ,sd->deal.zeny, LOG_TYPE_TRADE, tsd->status.char_id);
		pc_getzeny(tsd,sd->deal.zeny,LOG_TYPE_TRADE, sd->status.char_id);


	}

	if ( tsd->deal.zeny) {
		pc_payzeny(tsd,tsd->deal.zeny,LOG_TYPE_TRADE, sd->status.char_id);
		pc_getzeny(sd ,tsd->deal.zeny,LOG_TYPE_TRADE, tsd->status.char_id);

	}

    } // Flush grant/quest callbacks only after both inventories and wallets settle.
    pn_pair_submit(*sd,*tsd);
}

// Only the matching durable pair acknowledgement can complete native trades.
void trade_pair_completed(map_session_data& a,map_session_data& b) {
    for(auto* sd:{&a,&b}) {
        sd->state.deal_locked=0;sd->trade_partner={0,0};sd->state.trading=0;sd->state.isBoundTrading=0;
        memset(&sd->deal,0,sizeof(sd->deal));trade_clear_epoch(*sd);
        if(sd->bank_ui.action==pn_bank::TradeCommit)sd->bank_ui.result=pn_bank::Ok;
        clif_tradecompleted(*sd);
    }
}

// Only reached through the authenticated companion. Item contents are still
// reviewed in the native item window; all full-width Zeny confirmations bind
// both that item revision and the explicit partner/amount snapshot.
pn_bank::Result trade_wide_action(map_session_data& sd, const pn_bank::Request& request) {
    auto* other = map_id2sd(sd.trade_partner.id);
    if (!sd.state.trading || !other || !other->state.trading ||
        other->trade_partner.id != sd.status.account_id || !request.trade_id ||
        request.trade_id != sd.bank_ui.trade_id || request.trade_id != other->bank_ui.trade_id)
        return pn_bank::Stale;
    if (sd.pair_commit.pending || other->pair_commit.pending) return pn_bank::Saving;
    if (request.action == pn_bank::TradeCancel) {
        if (request.amount) return pn_bank::Invalid;
        trade_tradecancel(&sd);return pn_bank::Ok;
    }
    if (request.trade_revision != sd.bank_ui.trade_revision || request.trade_revision != other->bank_ui.trade_revision)
        return pn_bank::Stale;
    if (pc_transaction_pending(&sd) || pc_transaction_pending(other) || !chrif_isconnected() ||
        pc_isdead(&sd) || pc_isdead(other) || sd.state.warping || other->state.warping ||
        sd.m != other->m || (!pc_can_use_command(&sd,"trade",COMMAND_ATCOMMAND) && !check_distance_bl(&sd,other,TRADE_DISTANCE)))
        return pn_bank::Busy;
    switch (request.action) {
    case pn_bank::TradeSetOffer:
        if (sd.state.deal_locked || request.amount < 0) return pn_bank::Invalid;
        if (request.amount > sd.status.zeny) return pn_bank::Funds;
        if (!pn_zeny::room(other->status.zeny,other->mail.pending_zeny,request.amount)) return pn_bank::Limit;
        if (request.amount > MAX_ZENY && (!session_isValid(other->bank_ui.companion_fd) || session[other->bank_ui.companion_fd]->flag.eof)) return pn_bank::Busy;
        trade_tradeaddzeny(&sd,request.amount);return pn_bank::Ok;
    case pn_bank::TradeLock:
        if (request.amount || sd.state.deal_locked) return pn_bank::Invalid;
        trade_tradeok(&sd,true);return sd.state.deal_locked == 1 ? pn_bank::Ok : pn_bank::Invalid;
    case pn_bank::TradeCommit:
        if (request.amount || sd.state.deal_locked != 1 || other->state.deal_locked < 1) return pn_bank::Invalid;
        if (!trade_check(&sd,other)) return pn_bank::Limit;
        trade_tradecommit(&sd,true);return sd.pair_commit.pending ? pn_bank::Saving : pn_bank::Ok;
    default: return pn_bank::Invalid;
    }
}
