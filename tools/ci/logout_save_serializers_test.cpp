// Real replacement-save serializers and real wire structures, with a transport double.
#include <cassert>
#include <cstring>
#include <iostream>
#include <vector>
#include <common/mmo.hpp>
#include <common/socket.hpp>
#include <map/chrif_save.hpp>
struct map_session_data {
	mmo_charstatus status{};
	int num_quests=0;
	quest* quest_log=nullptr;
};
#define nullpo_retr(value,p) if(!(p)) return value
bool pc_transaction_pending(const map_session_data*) { return false; }
bool connected=false;
std::vector<std::vector<uint8>> sent;
bool chrif_save_available() { return connected || chrif_save_capture; }
bool chrif_save_packet(const void* p,size_t n) {
	if(chrif_save_capture) return chrif_save_capture->append(p,n);
	if(!connected)return false;
	sent.emplace_back(static_cast<const uint8*>(p),static_cast<const uint8*>(p)+n);
	return true;
}
#include "logout-serializers.inc"
template<class T> void payload(const std::vector<uint8>& p,uint16 opcode,size_t offset,const T& value) {
	assert(p.size()==offset+sizeof(value));
	assert(RBUFW(p.data(),0)==opcode && RBUFW(p.data(),2)==p.size());
	assert(memcmp(p.data()+offset,&value,sizeof(value))==0);
}
int main() {
	map_session_data sd;sd.status.account_id=7;sd.status.char_id=17;
	quest quests[2]{};quests[0].quest_id=123;quests[0].count[0]=3;quests[1].quest_id=456;
	sd.quest_log=quests;sd.num_quests=2;
	s_storage storage{};storage.type=TABLE_INVENTORY;storage.amount=1;
	storage.u.items_inventory[0].nameid=501;storage.u.items_inventory[0].amount=12;
	s_pet pet{};pet.pet_id=13;pet.intimate=800;
	s_homunculus hom{};hom.hom_id=19;hom.hp=123;
	s_mercenary merc{};merc.mercenary_id=23;merc.hp=456;
	s_elemental ele{};ele.elemental_id=29;ele.hp=789;
	ChrifSaveBuffer capture;
	{
		ChrifSaveCapture scope(capture);
		assert(intif_storage_save(&sd,&storage));
		assert(intif_quest_save(&sd));
		assert(intif_save_petdata(sd.status.account_id,&pet));
		assert(intif_homunculus_requestsave(sd.status.account_id,&hom));
		assert(intif_mercenary_save(&merc));
		assert(intif_elemental_save(&ele));
	}
	assert(!capture.failed && capture.packets.size()==6 && sent.empty());
	payload(capture.packets[0],0x308b,13,storage);
	assert(RBUFL(capture.packets[0].data(),5)==7 && RBUFL(capture.packets[0].data(),9)==17);
	payload(capture.packets[1],0x3061,8,quests);
	assert(RBUFL(capture.packets[1].data(),4)==17);
	payload(capture.packets[2],0x3082,8,pet);
	payload(capture.packets[3],0x3092,8,hom);
	payload(capture.packets[4],0x3073,4,merc);
	payload(capture.packets[5],0x307f,4,ele);
	assert(RBUFL(capture.packets[2].data(),4)==7 && RBUFL(capture.packets[3].data(),4)==7);
	connected=true;
	intif_storage_save(&sd,&storage);intif_quest_save(&sd);intif_save_petdata(7,&pet);
	intif_homunculus_requestsave(7,&hom);intif_mercenary_save(&merc);intif_elemental_save(&ele);
	assert(sent==capture.packets);
	// Capture owns bytes after the producer's live data changes or disappears.
	storage.u.items_inventory[0].amount=999;quests[0].count[0]=999;pet.intimate=1;
	assert(sent==capture.packets);
	connected=false;sent.clear();
	assert(!intif_storage_save(&sd,&storage));assert(!intif_quest_save(&sd));
	assert(sent.empty());
	std::cout<<"PASS six real logout serializers: byte-identical connected/offline capture, owned snapshots and wire identity\n";
}
