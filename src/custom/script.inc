// Copyright (c) rAthena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

/**
 * Custom Script Commands
 * Place the body of custom script commands in this file.
 * Format:
 *	BUILDIN_FUNC(command_name)
 *	{
 *		<code>;
 *	}
 **/

//BUILDIN_FUNC(example)
//{
//	ShowInfo("buildin_example: It works!\n");
//	script_pushint(st,1);
//	return 0;
//}

// A true result means submitted, not committed. The acknowledgement owns delivery.
BUILDIN_FUNC(getpetreward)
{
	map_session_data* sd=nullptr;
	auto reject=[&](){if(sd)pn_item_use_abort(*sd);script_pushint(st,0);return SCRIPT_CMD_SUCCESS;};
	if(!script_rid2sd(sd))return reject();
	const int64 egg=script_getnum64(st,2),quantity=script_getnum64(st,3);
	if(egg<=0 || egg>UINT32_MAX || quantity<1 || quantity>MAX_INVENTORY ||
	   !pet_db_search(static_cast<t_itemid>(egg),PET_EGG) ||
	   script_hasdata(st,4)!=script_hasdata(st,5))return reject();
	uint32_t costs[MAX_INVENTORY]{};
	if(script_hasdata(st,4)) {
		const int64 material=script_getnum64(st,4);
		int64 remaining=script_getnum64(st,5);
		if(material<=0 || material>UINT32_MAX || remaining<=0)return reject();
		for(int i=0;i<sd->status.inventory_slots && i<MAX_INVENTORY && remaining;++i) {
			const auto& it=sd->inventory.u.items_inventory[i];
			if(it.nameid!=material || it.amount<=0 || it.equip || it.equipSwitch)continue;
			costs[i]=static_cast<uint32_t>(std::min<int64>(remaining,it.amount));remaining-=costs[i];
		}
		if(remaining)return reject();
	}
	if(pn_item_use_active(sd)) {
		item prototype{};prototype.nameid=static_cast<t_itemid>(egg);prototype.amount=1;prototype.identify=1;
		if(!pn_item_use_collect_pet(*sd,prototype,static_cast<uint32>(quantity)))return reject();
		for(int i=0;i<MAX_INVENTORY;++i)if(costs[i] && pc_delitem(sd,i,costs[i],0,0,LOG_TYPE_SCRIPT))return reject();
		script_pushint(st,1);return SCRIPT_CMD_SUCCESS;
	}
	auto request=pn_shop_request(*sd,pn_shop::Asset);
	const std::vector<pn_shop::Grant> grants={{static_cast<uint32_t>(egg),static_cast<uint32_t>(quantity)}};
	script_pushint(st,pn_shop_begin(*sd,request,grants,costs)?1:0);
	return SCRIPT_CMD_SUCCESS;
}
#include "lab_history.inc"
