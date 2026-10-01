#!/usr/bin/env python3
# ============================================================================
#  PN  /  DEVELOPMENT TOOLS
#  rodex_operation_test.py
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/rodex_operation_test.py
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

"""Compile actual RODEX operation and storage-open handlers with UI test doubles."""
import argparse
import pathlib
import subprocess
import tempfile

root = pathlib.Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser()
parser.add_argument('--emit', type=pathlib.Path)
parser.add_argument('--cxx', default='g++')
args = parser.parse_args()

def function(path, signature):
    source = (root / path).read_text()
    start = source.index(signature)
    opening = source.index('{', start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]

prefix = r'''
#include <cassert>
#include <cstdint>
#include <iostream>
using int32=int32_t;
#define PACKETVER 20250319
#define MF_NORODEX 1
#define OPT1_BURNING 1
#define nullpo_ret(sd) assert(sd)
#define nullpo_retv(sd) assert(sd)
#define ARRAYLENGTH(a) (sizeof(a)/sizeof((a)[0]))
struct Storage {
 struct { int items_storage[10]; } u;
 struct { int put=0,get=0; } state;
 int amount=0,max_amount=10,stor_id=0; bool status=false;
};
struct map_session_data {
 bool capture_waiting=false;
 struct { uint32_t reward_pending_id=0; } achievement_data;
 int m=0,fd=1,npc_id=0,npc_shopid=0;
 struct { int storage_flag=0;
  bool trading=false,vending=false,buyingstore=false,mail_writing=false;
  bool prevend=false,banking=false,callshop=false,refineui_open=false,stylist_open=false;
  bool inventory_expansion_confirmation=false,barter_open=false,barter_extended_open=false;
  bool laphine_synthesis=false,laphine_upgrade=false,roulette_open=false,enchantgrade_open=false;
  bool item_reform=false,item_reform_save_id=false,item_enchant_index=false;
 } state;
 struct { bool pending=false,applying=false; } bank_ui,mail_companion,pair_commit,shop_commit;
 struct { bool pending=false,applying=false,loading=false; } multi_storage;
 struct { unsigned pending_zeny=0,pending_slots=0,pending_weight=0; } mail;
 struct { int opt1=0; } sc;
 Storage storage,premiumStorage;
};
bool restricted=false,allowed=true;
bool pn_item_use_capture_waiting(const map_session_data* sd){return sd->capture_waiting;}
int opened=0;
bool map_getmapflag(int,int) { return restricted; }
bool pc_can_give_items(map_session_data*) { return allowed; }
const char* msg_txt(const map_session_data*,int) { return "test"; }
void clif_displaymessage(int,const char*) {}
void storage_sortitem(int*,size_t) {}
const char* storage_getName(int) { return "storage"; }
const char* storage_page_name(map_session_data&,int) { return "storage"; }
void clif_storagelist(map_session_data*,int*,size_t,const char*) { ++opened; }
void clif_updatestorageamount(map_session_data&,int,int) {}
'''
suffix = r'''
int main() {
 for(int writing=0;writing<2;++writing) {
  for(int mask=0;mask<128;++mask) {
   map_session_data sd;
   sd.state.mail_writing=writing;
   sd.npc_id=mask&1; sd.npc_shopid=mask&2;
   sd.state.storage_flag=(mask&4)?3:0;
   sd.state.trading=mask&8; sd.state.vending=mask&16; sd.state.buyingstore=mask&32;
   restricted=mask&64;
   assert(mail_invalid_operation(&sd)==(mask!=0));
  }
 }
 restricted=false;
 map_session_data sd;
 assert(!mail_invalid_operation(&sd));
 sd.state.mail_writing=true;
 assert(!mail_invalid_operation(&sd)); // attachments/send in own composer remain valid
 assert(storage_storageopen(&sd)==1 && sd.state.storage_flag==0 && opened==0);
 storage_premiumStorage_open(&sd); // delayed async response cannot overlap writing
 assert(sd.state.storage_flag==0 && opened==0);
 sd.state.mail_writing=false;
 sd.npc_id=123; // NPCs legitimately open storage
 assert(storage_storageopen(&sd)==0 && sd.state.storage_flag==1 && opened==1);
 assert(mail_invalid_operation(&sd));
 sd.npc_id=sd.state.storage_flag=0;
 storage_premiumStorage_open(&sd);
 assert(sd.state.storage_flag==3 && opened==2 && mail_invalid_operation(&sd));
 for(int pending=0;pending<8;++pending) {
  map_session_data blocked;
  blocked.bank_ui.pending=pending==0;
  blocked.multi_storage.pending=pending==1;
  blocked.multi_storage.loading=pending==2;
  blocked.mail_companion.pending=pending==3;
  blocked.pair_commit.pending=pending==4;
  blocked.shop_commit.pending=pending==5;
  blocked.capture_waiting=pending==6;
  blocked.achievement_data.reward_pending_id=pending==7?1:0;
  if(blocked.capture_waiting){assert(mail_invalid_operation(&blocked));assert(pc_transaction_locked(&blocked));}
  assert(pc_cant_act2(&blocked));
  assert(storage_storageopen(&blocked)==1);
  storage_premiumStorage_open(&blocked);
  assert(blocked.state.storage_flag==0 && opened==2);
 }
 for(int pending=0;pending<3;++pending) {
  map_session_data blocked;
  blocked.mail.pending_zeny=pending==0?100:0;
  blocked.mail.pending_slots=pending==1?1:0;
  blocked.mail.pending_weight=pending==2?10:0;
  assert(pc_cant_act2(&blocked));
  assert(storage_storageopen(&blocked)==1);
  storage_premiumStorage_open(&blocked);
  assert(blocked.state.storage_flag==0 && opened==2);
  blocked.mail={};
  assert(!pc_cant_act2(&blocked));
 }
 // Production pending/locked predicates differ during the internal apply
 // phase: financial mutation can proceed, but neither storage UI may open.
 // These are state/handler tests, not asynchronous transaction persistence.
 for(int mask=0;mask<32;++mask) {
  map_session_data blocked;
  blocked.bank_ui.pending=mask&1;
  blocked.multi_storage.pending=mask&2;
  blocked.mail_companion.pending=mask&4;
  blocked.pair_commit.pending=mask&8;
  blocked.shop_commit.pending=mask&16;
  assert(pc_transaction_pending(&blocked)==(mask!=0));
  assert(pc_transaction_locked(&blocked)==(mask!=0));
  blocked.bank_ui.applying=blocked.multi_storage.applying=true;
  blocked.mail_companion.applying=blocked.pair_commit.applying=blocked.shop_commit.applying=true;
  assert(!pc_transaction_locked(&blocked));
  assert(pc_cant_act2(&blocked)==(mask!=0));
  const int before=opened;
  if(mask) {
   assert(storage_storageopen(&blocked)==1);
   storage_premiumStorage_open(&blocked);
   assert(blocked.state.storage_flag==0 && opened==before);
  }
  blocked.bank_ui.pending=blocked.multi_storage.pending=false;
  blocked.mail_companion.pending=blocked.pair_commit.pending=blocked.shop_commit.pending=false;
  assert(!pc_cant_act2(&blocked));
  assert(storage_storageopen(&blocked)==0 && blocked.state.storage_flag==1 && opened==before+1);
 }
 std::cout << "256 RODEX state combinations, storage/composer transitions, 8 pending sources, delayed-claim capacity and 32 pending/applying masks: PASS\n";
}
'''
test = prefix + '\n'.join([
    function('src/map/pc.hpp', 'static inline bool pc_transaction_pending('),
    function('src/map/pc.hpp', 'static inline bool pc_transaction_locked('),
    function('src/map/pc.hpp', 'static bool pc_cant_act2('),
    function('src/map/mail.cpp', 'bool mail_invalid_operation('),
    function('src/map/storage.cpp', 'int32 storage_storageopen('),
    function('src/map/storage.cpp', 'void storage_premiumStorage_open('),
]) + suffix
if args.emit:
    args.emit.write_text(test)
else:
    with tempfile.TemporaryDirectory() as directory:
        path=pathlib.Path(directory)
        (path/'test.cpp').write_text(test)
        subprocess.run([args.cxx,'-std=c++17','-O0','-g','-fsanitize=address',str(path/'test.cpp'),'-o',str(path/'test')],check=True)
        subprocess.run([str(path/'test')],check=True)
