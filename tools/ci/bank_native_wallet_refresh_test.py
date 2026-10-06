"""Compile real refresh nudge, snapshot/pending logic and the native SP_ZENY branch."""
from pathlib import Path
import ast
import os
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[2]
source=(ROOT/'src/custom/bank_ui.inc').read_text()
snapshot=source[source.index('static pn_bank::Reply pn_bank_snapshot'):source.index('// Reuse the native status handler')]
helper=source[source.index('static void pn_bank_refresh_native_wallet'):source.index('static pn_bank::Result pn_bank_action')]
assert source.count('pn_bank_refresh_native_wallet(sd, request, reply, authenticated);')==1
pc=(ROOT/'src/map/pc.hpp').read_text()
pending=pc[pc.index('static inline bool pc_transaction_pending'):pc.index('static inline bool pc_transaction_locked')]
clif=(ROOT/'src/map/clif.cpp').read_text()
start=clif.index('case SP_ZENY:',clif.index('void clif_updatestatus'))
zeny=clif[start:clif.index('#if PACKETVER',start)]
module=ast.parse((ROOT/'tools/ci/bank_snapshot_test.py').read_text())
stub=next(ast.literal_eval(node.value) for node in module.body if isinstance(node,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='stub' for t in node.targets))
wire=r"""
using int64=int64_t;
constexpr int64 MAX_ZENY=INT32_MAX;
enum _sp {SP_ZENY=20};
unsigned sent=0;int64 last_wire=-1;
void clif_longpar_change(map_session_data&,enum _sp type,int64 value){assert(type==SP_ZENY);++sent;last_wire=value;}
void clif_updatestatus(map_session_data& sd,enum _sp type){switch(type){
"""+zeny+'}}\n'
tests=r"""
void emit(map_session_data* sd,const pn_bank::Request& request,const pn_bank::Reply& reply,bool auth,bool expected){
 unsigned before=sent;int64 wallet=sd?sd->status.zeny:0,bank=sd?sd->bank_vault:0;
 pn_bank_refresh_native_wallet(sd,request,reply,auth);
 assert(sent==before+(expected?1:0));
 if(sd){assert(sd->status.zeny==wallet && sd->bank_vault==bank);if(expected)assert(last_wire==std::min<int64>(wallet,INT32_MAX));}
}
int main(){
 pn_bank::Request request;map_session_data sd;size_t cases=0;
 const int64 wallets[]={0,1,INT32_MAX-1LL,INT32_MAX,INT32_MAX+1LL,9007199254740993LL,INT64_MAX};
 for(int64 wallet:wallets)for(uint32 result=pn_bank::Ok;result<=pn_bank::SaveFailed;++result){
  sd.status.zeny=wallet;auto reply=pn_bank_snapshot(sd);reply.result=result;
  emit(&sd,request,reply,true,result!=pn_bank::Saving && result!=pn_bank::Unauthorized);++cases;
 }
 sd.status.zeny=9007199254740993LL;auto reply=pn_bank_snapshot(sd);
 emit(&sd,request,reply,false,false);emit(nullptr,request,reply,true,false);
 for(uint32 action=pn_bank::Deposit;action<=pn_bank::CollectOffline;++action){request.action=action;emit(&sd,request,reply,true,false);}
 request.action=pn_bank::Refresh;
 for(unsigned flags=1;flags<128;++flags){
  sd.bank_ui.pending=flags&1;sd.pair_commit.pending=flags&2;sd.mail_companion.pending=flags&4;sd.multi_storage.pending=flags&8;
  sd.shop_commit.pending=flags&16;sd.achievement_data.reward_pending_id=(flags&32)?42:0;sd.capture_waiting=flags&64;
  emit(&sd,request,reply,true,false); // Even a stale terminal reply cannot bypass live pending state.
 }
 sd.bank_ui.pending=sd.pair_commit.pending=sd.mail_companion.pending=sd.multi_storage.pending=false;
 sd.shop_commit.pending=sd.capture_waiting=false;sd.achievement_data.reward_pending_id=0;
 auto invalid=reply;invalid.char_id++;emit(&sd,request,invalid,true,false);
 invalid=reply;invalid.nonce_hi++;emit(&sd,request,invalid,true,false);
 invalid=reply;invalid.nonce_lo++;emit(&sd,request,invalid,true,false);
 invalid=reply;invalid.wallet--;emit(&sd,request,invalid,true,false);
 invalid=reply;invalid.wallet=-1;emit(&sd,request,invalid,true,false);
 invalid=reply;invalid.flags=2;emit(&sd,request,invalid,true,false);
 no_bank=true;reply=pn_bank_snapshot(sd);assert(reply.result==pn_bank::Unavailable);emit(&sd,request,reply,true,true);
 sd.mail_companion.pending=true;reply=pn_bank_snapshot(sd);assert(reply.result==pn_bank::Saving);emit(&sd,request,reply,true,false);
 printf("PASS: %zu terminal-result/wallet boundaries; native SP_ZENY projection exact; zero financial mutations; no unauthenticated/pending/action/stale/corrupt emissions\n",cases);
}
"""
with tempfile.TemporaryDirectory(prefix='pn-native-wallet-refresh-') as folder:
 directory=Path(folder);cpp=directory/'test.cpp';binary=directory/('test.exe' if os.name=='nt' else 'test')
 (directory/'bank_protocol.hpp').write_text((ROOT/'src/custom/bank_protocol.hpp').read_text())
 cpp.write_text(stub+wire+pending+snapshot+helper+tests)
 flags=['-std=c++17','-O1']
 if os.name!='nt':flags+=['-fsanitize=undefined','-fno-sanitize-recover=all']
 subprocess.run(['g++',*flags,str(cpp),'-o',str(binary)],check=True)
 subprocess.run([str(binary)],check=True)
