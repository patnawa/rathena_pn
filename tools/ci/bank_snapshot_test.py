"""Exercise the production snapshot and pending predicate; includes a failing prior-body control."""
from pathlib import Path
import json
import os
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parents[2]
source=(ROOT/'src/custom/bank_ui.inc').read_text()
snapshot=source[source.index('static pn_bank::Reply pn_bank_snapshot'):source.index('// Reuse the native status handler')]
pc=(ROOT/'src/map/pc.hpp').read_text()
pending=pc[pc.index('static inline bool pc_transaction_pending'):pc.index('static inline bool pc_transaction_locked')]
prior=snapshot.replace('const bool saving = pc_transaction_pending(&sd);','const bool saving = sd.bank_ui.pending || sd.pair_commit.pending;').replace('if (!saving && (!battle_config.feature_banking || map_getmapflag(sd.m, MF_NOBANK) || !chrif_isconnected()))','if (!battle_config.feature_banking || map_getmapflag(sd.m, MF_NOBANK) || !chrif_isconnected())')
assert prior!=snapshot
stub=r"""
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include "bank_protocol.hpp"
using uint64=uint64_t;using uint32=uint32_t;
struct Pending {bool pending=false;};
struct Bank:Pending {uint64_t nonce_hi=123,nonce_lo=456,request_id=789,trade_id=0,trade_revision=0;uint32_t result=pn_bank::SaveFailed,collected_characters=7,skipped_characters=2;};
struct map_session_data {
 struct {uint32_t char_id=10,account_id=20;int64_t zeny=9007199254740993LL;char name[24]="fixture";}status;
 Bank bank_ui;Pending pair_commit,multi_storage,mail_companion,shop_commit;bool capture_waiting=false;
 int64_t bank_vault=100000000000LL;int m=0;
 struct{uint32_t id=0;}trade_partner;
 struct{bool trading=false;uint32_t deal_locked=0;}state;
 struct{int64_t zeny=0;}deal;
};
struct{bool feature_banking=true;}battle_config;
bool pn_item_use_capture_waiting(const map_session_data* sd){return sd->capture_waiting;}
constexpr int MF_NOBANK=1;
bool no_bank=false,connected=true;
int rnd(){return 1;}
bool map_getmapflag(int,int){return no_bank;}
bool chrif_isconnected(){return connected;}
map_session_data* map_id2sd(uint32_t){return nullptr;}
void safestrncpy(char* out,const char* in,size_t n){std::strncpy(out,in,n);out[n-1]=0;}
"""
tests=r"""
int main(){
 static_assert(pn_bank::version==3 && sizeof(pn_bank::Request)==80 && sizeof(pn_bank::Reply)==208,"Wire ABI changed");
 size_t cases=0;
 for(unsigned availability=0;availability<8;++availability){
  battle_config.feature_banking=(availability&1)!=0;no_bank=(availability&2)!=0;connected=(availability&4)!=0;
  for(unsigned flags=0;flags<16;++flags){
   map_session_data sd;
   sd.bank_ui.pending=(flags&1)!=0;sd.pair_commit.pending=(flags&2)!=0;
   sd.mail_companion.pending=(flags&4)!=0;sd.multi_storage.pending=(flags&8)!=0;
   const auto reply=pn_bank_snapshot(sd);
   const auto expected=flags?pn_bank::Saving:((!battle_config.feature_banking||no_bank||!connected)?pn_bank::Unavailable:pn_bank::SaveFailed);
   assert(reply.result==expected);
   assert(reply.wallet==9007199254740993LL && reply.bank==100000000000LL);
   assert(reply.flags==0 && reply.reserved==0 && pn_bank::valid_reply(reply));
   assert(reply.request_id==789 && reply.counts[0]==7 && reply.counts[1]==2);
   assert(sd.bank_ui.result==pn_bank::SaveFailed && sd.bank_ui.request_id==789);
   ++cases;
  }
 }
 battle_config.feature_banking=true;no_bank=false;connected=true;
 map_session_data sd;
 sd.capture_waiting=true;assert(pc_transaction_pending(&sd));assert(pn_bank_snapshot(sd).result==pn_bank::Saving);sd.capture_waiting=false;
 auto failed=pn_bank_snapshot(sd);assert(failed.result==pn_bank::SaveFailed);
 sd.status.zeny+=123;auto later=pn_bank_snapshot(sd);
 assert(later.result==pn_bank::SaveFailed && later.wallet==failed.wallet+123 && pn_bank::valid_reply(later));
 no_bank=true;auto restricted=pn_bank_snapshot(sd);
 assert(restricted.result==pn_bank::Unavailable && restricted.wallet==later.wallet && pn_bank::valid_reply(restricted));
 sd.bank_ui.pending=true;sd.status.zeny=-1;auto corrupt=pn_bank_snapshot(sd);assert(!pn_bank::valid_reply(corrupt));
 printf("PASS: %zu pending/availability cases; retained failures use live wallet; NOBANK snapshots remain valid; corrupt wallet refused; v3/80/208/flags unchanged\n",cases);
}
"""
with tempfile.TemporaryDirectory(prefix='pn-bank-snapshot-') as folder:
 directory=Path(folder)
 (directory/'bank_protocol.hpp').write_text((ROOT/'src/custom/bank_protocol.hpp').read_text())
 for label,body,success in [('prior',prior,False),('current',snapshot,True)]:
  cpp=directory/(label+'.cpp');binary=directory/(label+('.exe' if os.name=='nt' else ''))
  cpp.write_text(stub+pending+body+tests)
  flags=['-std=c++17','-O1']
  if os.name!='nt':flags+=['-fsanitize=undefined','-fno-sanitize-recover=all']
  subprocess.run(['g++',*flags,str(cpp),'-o',str(binary)],check=True)
  result=subprocess.run([str(binary)],text=True,capture_output=True)
  if success:
   assert result.returncode==0,result.stdout+result.stderr
   print(result.stdout,end='')
  else:
   assert result.returncode!=0,'Prior implementation unexpectedly passed the regression'
   print('PASS: prior production snapshot fails the pending/availability regression')
