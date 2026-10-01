void npc_market_tosql(const char *exname, struct npc_item_list *list) {
	SqlStmt stmt{ *mmysql_handle };
	if (SQL_ERROR == stmt.Prepare("REPLACE INTO `%s` (`name`,`nameid`,`price`,`amount`,`flag`) VALUES ('%s','%u','%d','%d','%" PRIu8 "')",
		market_table, exname, list->nameid, list->value, list->qty, list->flag) ||
		SQL_ERROR == stmt.Execute())
		SqlStmt_ShowDebug(stmt);
}

/**
 * Removes persistent NPC Market Data

static int32 npc_buylist_sub(map_session_data* sd, std::vector<s_npc_buy_list>& item_list, npc_data* nd) {
	char npc_ev[EVENT_NAME_LENGTH];
	int32 key_nameid = 0, key_amount = 0;

	// discard old contents
	script_cleararray_pc( sd, "@bought_nameid" );
	script_cleararray_pc( sd, "@bought_quantity" );

	// save list of bought items
	for( int32 i = 0; i < item_list.size(); i++ ){
		script_setarray_pc( sd, "@bought_nameid", i, item_list[i].nameid, &key_nameid );
		script_setarray_pc( sd, "@bought_quantity", i, item_list[i].qty, &key_amount );
	}

	// invoke event
	snprintf(npc_ev, ARRAYLENGTH(npc_ev), "%s::%s", nd->exname, script_config.onbuy_event_name);
	npc_event(sd, npc_ev, 0);

	return 0;
}

/**
 * Shop buylist that the player is attempting to purchase
 * @param sd: Player who attempt to buy
 * @param n: Number of items
 * @param item_list: List of items
 * @return result code for clif_parse_NpcBuyListSend/clif_npc_market_purchase_ack
 */
e_purchase_result npc_buylist( map_session_data* sd, std::vector<s_npc_buy_list>& item_list ){
	npc_data* nd;
	struct npc_item_list *shop = nullptr;
	int64 z;
	int32 j,k,skill,new_;
	int64 w;
	std::vector<int32> market_index( item_list.size() );

	nullpo_retr(e_purchase_result::PURCHASE_FAIL_COUNT, sd);

	nd = npc_checknear(sd,map_id2bl(sd->npc_shopid));
	if( nd == nullptr )
		return e_purchase_result::PURCHASE_FAIL_COUNT;
	if( nd->subtype != NPCTYPE_SHOP && nd->subtype != NPCTYPE_MARKETSHOP )
		return e_purchase_result::PURCHASE_FAIL_COUNT;
	if( item_list.empty() || item_list.size() > MAX_INVENTORY ){
		return e_purchase_result::PURCHASE_FAIL_COUNT;
	}

	z = 0;
	w = 0;
	new_ = 0;

	shop = nd->u.shop.shop_item;

	for( const auto& entry : item_list ){
		if( entry.qty <= 0 || entry.qty > MAX_AMOUNT )
			return e_purchase_result::PURCHASE_FAIL_COUNT;
	}
	// process entries in buy list, one by one
	for( int32 i = 0; i < item_list.size(); ++i ){
		t_itemid nameid;
		uint16 amount;
		int32 value;

		// find this entry in the shop's sell list
		ARR_FIND( 0, nd->u.shop.count, j,
			item_list[i].nameid == shop[j].nameid || //Normal items
			item_list[i].nameid == itemdb_viewid(shop[j].nameid) //item_avail replacement
		);

		if( j == nd->u.shop.count )
			return e_purchase_result::PURCHASE_FAIL_COUNT; // no such item in shop

#if PACKETVER >= 20131223
		if (nd->subtype == NPCTYPE_MARKETSHOP) {
			if (shop[j].qty >= 0 && item_list[i].qty > shop[j].qty)
				return e_purchase_result::PURCHASE_FAIL_COUNT;
			market_index[i] = j;
		}
#endif

		amount = item_list[i].qty;
		nameid = item_list[i].nameid = shop[j].nameid; //item_avail replacement
		// Independent preflight cannot reserve the same stack or stock twice.
		for( int32 previous = 0; previous < i; ++previous ){
			if( item_list[previous].nameid == nameid )
				return e_purchase_result::PURCHASE_FAIL_COUNT;
		}
		value = shop[j].value;

		std::shared_ptr<item_data> id = item_db.find(nameid);

		if( !id )
			return e_purchase_result::PURCHASE_FAIL_COUNT; // item no longer in itemdb

		if( !itemdb_isstackable2(id.get()) && amount > 1 ) { //Exploit? You can't buy more than 1 of equipment types o.O
			ShowWarning("Player %s (%d:%d) sent a hexed packet trying to buy %d of nonstackable item %u!\n",
				sd->status.name, sd->status.account_id, sd->status.char_id, amount, nameid);
			amount = item_list[i].qty = 1;
		}

		if( nd->master_nd ) { // Script-controlled shops decide by themselves, what can be bought and for what price.
			continue;
		}

		switch( pc_checkadditem(sd,nameid,amount) ) {
			case CHKADDITEM_EXIST:
				break;

			case CHKADDITEM_NEW:
				new_ += id->inventorySlotNeeded(amount);
				break;

			case CHKADDITEM_OVERAMOUNT:
				return e_purchase_result::PURCHASE_FAIL_WEIGHT;
		}

		if (npc_shop_discount(nd))
			value = pc_modifybuyvalue(sd,value);

		if (value < 0 || (value && amount > (MAX_WALLET_ZENY-z)/value)) return e_purchase_result::PURCHASE_FAIL_MONEY;
		z += static_cast<int64>(value) * amount;
		w += static_cast<int64>( itemdb_weight(nameid) ) * amount;
	}

	if (nd->master_nd){ //Script-based shops.
		npc_buylist_sub(sd,item_list,nd->master_nd);
		return e_purchase_result::PURCHASE_SUCCEED;
	}

	if (z > sd->status.zeny)
		return e_purchase_result::PURCHASE_FAIL_MONEY;	// Not enough Zeny

	if( w + sd->weight > sd->max_weight )
		return e_purchase_result::PURCHASE_FAIL_WEIGHT;	// Too heavy
	if( pc_inventoryblank(sd) < new_ )
		return e_purchase_result::PURCHASE_FAIL_COUNT;	// Not enough space to store items

	if (pc_payzeny(sd, z, LOG_TYPE_NPC)) return e_purchase_result::PURCHASE_FAIL_MONEY;

	for( int32 i = 0; i < item_list.size(); ++i ) {
		t_itemid nameid = item_list[i].nameid;
		uint16 amount = item_list[i].qty;

#if PACKETVER >= 20131223
		if (nd->subtype == NPCTYPE_MARKETSHOP) {
			j = market_index[i];

			if( shop[j].qty >= 0 ){
				if (amount > shop[j].qty)
					return e_purchase_result::PURCHASE_FAIL_COUNT;
				shop[j].qty -= amount;
				npc_market_tosql(nd->exname, &shop[j]);
			}
		}
#endif

		if (itemdb_type(nameid) == IT_PETEGG)
			pet_create_egg(sd, nameid);
		else {
			uint16 get_amt = amount;

			if ((itemdb_search(nameid))->flag.guid)
				get_amt = 1;

			for (k = 0; k < amount; k += get_amt) {
				struct item item_tmp;
				memset(&item_tmp, 0, sizeof(item_tmp));
				item_tmp.nameid = nameid;
				item_tmp.identify = 1;

				pc_additem(sd,&item_tmp,get_amt,LOG_TYPE_NPC);
			}
		}
	}

	// custom merchant shop exp bonus
	if( battle_config.shop_exp > 0 && z > 0 && (skill = pc_checkskill(sd,MC_DISCOUNT)) > 0 ) {
		uint16 sk_idx = skill_get_index(MC_DISCOUNT);
		if( sd->status.skill[sk_idx].flag >= SKILL_FLAG_REPLACED_LV_0 )
			skill = sd->status.skill[sk_idx].flag - SKILL_FLAG_REPLACED_LV_0;

		if( skill > 0 ) {
			z = static_cast<int64>(std::min<double>(MAX_ZENY, z * (double)skill * (double)battle_config.shop_exp/10000.));
			if( z < 1 )
				z = 1;
			pc_gainexp(sd,nullptr,0,(int32)z, 0);
		}
	}

	return e_purchase_result::PURCHASE_SUCCEED;
}

/// npc_selllist for script-controlled shops

e_purchase_result npc_barter_purchase( map_session_data& sd, std::shared_ptr<s_npc_barter> barter, std::vector<s_barter_purchase>& purchases ){
	uint64 requiredZeny = 0;
	uint64 requiredWeight = 0;
	uint64 reducedWeight = 0;
	uint32 requiredSlots = 0;
	uint32 requiredItems[MAX_INVENTORY] = { 0 };

	// Validate untrusted quantities before narrowing or multiplying them.
	if( purchases.empty() || purchases.size() > MAX_INVENTORY || pc_transaction_locked( &sd ) ){
		return e_purchase_result::PURCHASE_FAIL_EXCHANGE_FAILED;
	}
	for( const auto& purchase : purchases ){
		if( !purchase.item || purchase.amount == 0 || purchase.amount > MAX_AMOUNT ){
			return e_purchase_result::PURCHASE_FAIL_COUNT;
		}
	}

	for( s_barter_purchase& purchase : purchases ){
		purchase.data = item_db.find( purchase.item->nameid ).get();

		if( purchase.data == nullptr ){
			return e_purchase_result::PURCHASE_FAIL_EXCHANGE_FAILED;
		}

		uint32 amount = purchase.amount;

		if( purchase.item->stockLimited && purchase.item->stock < amount ){
			return e_purchase_result::PURCHASE_FAIL_STOCK_EMPTY;
		}

		char result = pc_checkadditem( &sd, purchase.item->nameid, amount );

		if( result == CHKADDITEM_OVERAMOUNT ){
			return e_purchase_result::PURCHASE_FAIL_COUNT;
		}else if( result == CHKADDITEM_NEW ){
			requiredSlots += purchase.data->inventorySlotNeeded( amount );
		}

		requiredZeny += static_cast<uint64>( purchase.item->price ) * amount;
		requiredWeight += static_cast<uint64>( purchase.data->weight ) * amount;

		for( const auto& requirementPair : purchase.item->requirements ){
			std::shared_ptr<s_npc_barter_requirement> requirement = requirementPair.second;
			std::shared_ptr<item_data> id = item_db.find(requirement->nameid);

			if( id == nullptr ){
				return e_purchase_result::PURCHASE_FAIL_EXCHANGE_FAILED;
			}

			if( itemdb_isstackable2( id.get() ) ){
				int32 j;

				for( j = 0; j < MAX_INVENTORY; j++ ){
					if( sd.inventory.u.items_inventory[j].nameid == requirement->nameid ){
						// Equipped items are not taken into account
						if( sd.inventory.u.items_inventory[j].equip != 0 ){
							continue;
						}

						// Items in equip switch are not taken into account
						if( sd.inventory.u.items_inventory[j].equipSwitch != 0 ){
							continue;
						}

						// Server is configured to hide favorite items on selling
						if( battle_config.hide_fav_sell && sd.inventory.u.items_inventory[j].favorite != 0 ){
							continue;
						}

						// Actually stackable items should never be refinable, but who knows...
						if( requirement->refine >= 0 && sd.inventory.u.items_inventory[j].refine != requirement->refine ){
							// Refine does not match, continue with next item
							continue;
						}

						// Found a match, accumulate required amount
						requiredItems[j] += requirement->amount * amount;

						// Check if there are still enough items available
						if( requiredItems[j] > sd.inventory.u.items_inventory[j].amount ){
							return e_purchase_result::PURCHASE_FAIL_GOODS;
						}

						// Cancel the loop
						break;
					}
				}

				// Required item not found
				if( j == MAX_INVENTORY ){
					return e_purchase_result::PURCHASE_FAIL_GOODS;
				}
			}else{
				for( int32 i = 0; i < (requirement->amount * amount); i++ ){
					int32 j;

					for( j = 0; j < MAX_INVENTORY; j++ ){
						if( sd.inventory.u.items_inventory[j].nameid == requirement->nameid ){
							// Equipped items are not taken into account
							if( sd.inventory.u.items_inventory[j].equip != 0 ){
								continue;
							}

							// Items in equip switch are not taken into account
							if( sd.inventory.u.items_inventory[j].equipSwitch != 0 ){
								continue;
							}

							// Server is configured to hide favorite items on selling
							if( battle_config.hide_fav_sell && sd.inventory.u.items_inventory[j].favorite != 0 ){
								continue;
							}

							// If necessary, check if the refine rate matches
							if( requirement->refine >= 0 && sd.inventory.u.items_inventory[j].refine != requirement->refine ){
								// Refine does not match, continue with next item
								continue;
							}

							// Found a match, since it is not stackable, check if it was already taken
							if( requiredItems[j] > 0 ){
								// Item was already taken, try to find another match
								continue;
							}

							// Mark it as taken
							requiredItems[j] = 1;

							// Cancel the loop
							break;
						}
					}

					// Required item not found
					if( j == MAX_INVENTORY ){
						// Maybe the refine level did not match
						if( requirement->refine >= 0 ){
							int32 refine;

							// Try to find a higher refine level, going from the next lowest to the highest possible
							for( refine = requirement->refine + 1; refine <= MAX_REFINE; refine++ ){
								for( j = 0; j < MAX_INVENTORY; j++ ){
									if( sd.inventory.u.items_inventory[j].nameid == requirement->nameid ){
										// Equipped items are not taken into account
										if( sd.inventory.u.items_inventory[j].equip != 0 ){
											continue;
										}

										// Items in equip switch are not taken into account
										if(	sd.inventory.u.items_inventory[j].equipSwitch != 0 ){
											continue;
										}

										// Server is configured to hide favorite items on selling
										if( battle_config.hide_fav_sell && sd.inventory.u.items_inventory[j].favorite != 0 ){
											continue;
										}

										// If necessary, check if the refine rate matches
										if( requirement->refine >= 0 && sd.inventory.u.items_inventory[j].refine != refine ){
											// Refine does not match, continue with next item
											continue;
										}

										// Found a match, since it is not stackable, check if it was already taken
										if( requiredItems[j] > 0 ){
											// Item was already taken, try to find another match
											continue;
										}

										// Mark it as taken
										requiredItems[j] = 1;

										// Cancel the loop
										break;
									}
								}

								// If a match was found, make sure to cancel the loop
								if( j < MAX_INVENTORY ){
									// Cancel the loop
									break;
								}
							}

							// No matching entry found
							if( refine > MAX_REFINE ){
								return e_purchase_result::PURCHASE_FAIL_GOODS;
							}
						}else{
							return e_purchase_result::PURCHASE_FAIL_GOODS;
						}
					}
				}
			}

			reducedWeight += static_cast<uint64>( purchase.amount ) * requirement->amount * id->weight;
		}
	}

	// Check if there is enough Zeny
	if( sd.status.zeny < requiredZeny ){
		return e_purchase_result::PURCHASE_FAIL_MONEY;
	}

	// Check if there is enough Weight Limit
	if( ( sd.weight + requiredWeight - reducedWeight ) > sd.max_weight ){
		return e_purchase_result::PURCHASE_FAIL_WEIGHT;
	}

	if( pc_inventoryblank( &sd ) < requiredSlots ){
		return e_purchase_result::PURCHASE_FAIL_COUNT;
	}

	for( int32 i = 0; i < MAX_INVENTORY; i++ ){
		if( requiredItems[i] > 0 ){
			if( pc_delitem( &sd, i, requiredItems[i], 8, 0, LOG_TYPE_BARTER ) != 0 ){
				return e_purchase_result::PURCHASE_FAIL_EXCHANGE_FAILED;
			}
		}
	}

	if( pc_payzeny( &sd, static_cast<int64>( requiredZeny ), LOG_TYPE_BARTER ) != 0 ){
		return e_purchase_result::PURCHASE_FAIL_MONEY;
	}

	for( s_barter_purchase& purchase : purchases ){
		if( purchase.item->stockLimited ){
			purchase.item->stock -= purchase.amount;

			if( Sql_Query( mmysql_handle, "REPLACE INTO `%s` (`name`,`index`,`amount`) VALUES ( '%s', '%hu', '%hu' )", barter_table, barter->name.c_str(), purchase.item->index, purchase.item->stock ) != SQL_SUCCESS ){
				Sql_ShowDebug( mmysql_handle );
				return e_purchase_result::PURCHASE_FAIL_EXCHANGE_FAILED;
			}
		}

		if( itemdb_isstackable2( purchase.data ) ){
			struct item it = {};

			it.nameid = purchase.item->nameid;
			it.identify = true;

			if( pc_additem( &sd, &it, purchase.amount, LOG_TYPE_BARTER ) != ADDITEM_SUCCESS ){
				return e_purchase_result::PURCHASE_FAIL_EXCHANGE_FAILED;
			}
		}else{
			if( purchase.data->type == IT_PETEGG ){
				for( int32 i = 0; i < purchase.amount; i++ ){
					if( !pet_create_egg( &sd, purchase.item->nameid ) ){
						return e_purchase_result::PURCHASE_FAIL_EXCHANGE_FAILED;
					}
				}
			}else{
				for( int32 i = 0; i < purchase.amount; i++ ){
					struct item it = {};

					it.nameid = purchase.item->nameid;
					it.identify = true;
					it.refine = purchase.item->refine;

					if( pc_additem( &sd, &it, 1, LOG_TYPE_BARTER ) != ADDITEM_SUCCESS ){
						return e_purchase_result::PURCHASE_FAIL_EXCHANGE_FAILED;
					}
				}
			}
		}
	}

	// Material deletion must not run quest scripts between payment steps.
	pc_show_questinfo( &sd );
	return e_purchase_result::PURCHASE_SUCCEED;
}


//Atempt to remove an npc
