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
#define nullpo_ret(sd) assert(sd)
#define nullpo_retv(sd) assert(sd)
#define ARRAYLENGTH(a) (sizeof(a)/sizeof((a)[0]))
struct Storage { struct { int items_storage[10]; } u; int amount=0,max_amount=10,stor_id=0; };
struct map_session_data {
 int m=0,fd=1,npc_id=0,npc_shopid=0;
 struct { int storage_flag=0; bool trading=false,vending=false,buyingstore=false,mail_writing=false; } state;
 Storage storage,premiumStorage;
};
bool restricted=false,allowed=true;
int opened=0;
bool map_getmapflag(int,int) { return restricted; }
bool pc_can_give_items(map_session_data*) { return allowed; }
const char* msg_txt(const map_session_data*,int) { return "test"; }
void clif_displaymessage(int,const char*) {}
void storage_sortitem(int*,size_t) {}
const char* storage_getName(int) { return "storage"; }
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
 std::cout << "256 RODEX state combinations and storage/composer transitions: PASS\n";
}
'''
test = prefix + '\n'.join([
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
