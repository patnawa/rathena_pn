"""Compile the production reward bound and conversion branch with boundary fixtures."""
from pathlib import Path
import subprocess, tempfile
ROOT=Path(__file__).resolve().parents[2]
source=(ROOT/'src/map/itemdb.cpp').read_text()
helpers=source[source.index('static int64 itemdb_token_group_bound'):source.index('/** [Cydh]\n* Gives item(s)')]
start=source.index('\tif (pn_tokens::retired(data->nameid))',source.index('void ItemGroupDatabase::pc_get_itemgroup_sub'))
award=source[start:source.index('\n\titem tmp = {};',start)]
cpp=r"""
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <memory>
#include <map>
#include <cstdio>
#include "retired_tokens.hpp"
using int64=int64_t;using uint16=uint16_t;using t_itemid=uint32_t;
constexpr int64 MAX_WALLET_ZENY=INT64_MAX;
enum {IG_LIBRA_SCROLL=1,IG_MS_VIRGO_SCROLL,IG_SCROLL_OF_DEATH,IG_SCROLL_OF_LIFE,IG_MERCURY_SCROLL,GROUP_ALGORITHM_ALL,GROUP_ALGORITHM_RANDOM};
struct Entry {uint32_t nameid;uint16_t amount;};
struct Subgroup {int algorithm=GROUP_ALGORITHM_RANDOM;std::map<int,std::shared_ptr<Entry>> data;};
struct Group {std::map<int,std::shared_ptr<Subgroup>> random;};
struct Database {std::map<int,std::shared_ptr<Group>> values;std::shared_ptr<Group> find(int id){return values[id];}} itemdb_group;
struct Session {struct {int64 zeny=0;uint32_t char_id=10;}status;struct{int64 pending_zeny=0;}mail;};
constexpr int LOG_TYPE_SCRIPT=0;int errors=0;
void ShowError(const char*,uint32_t){++errors;}
void pc_getzeny(Session* s,int64 amount,int){assert(amount>=0 && amount<=INT64_MAX-s->status.zeny);s->status.zeny+=amount;}
"""+helpers+'\nvoid award(Session& sd,std::shared_ptr<Entry> data) {\n'+award+'\n}\n'+r"""
int main(){
 assert(pn_tokens::value(6024)==499000000 && pn_tokens::value(12781)==998000 && !pn_tokens::retired(501));
 assert(itemdb_token_box_bound(1)==0);
 const int boxes[]={16673,17141,17233,17234,17240};
 for(int id=1;id<=5;++id){
  auto group=std::make_shared<Group>();auto sub=std::make_shared<Subgroup>();
  sub->data[0]=std::make_shared<Entry>(Entry{6024,2});sub->data[1]=std::make_shared<Entry>(Entry{12781,3});group->random[0]=sub;itemdb_group.values[id]=group;
  assert(itemdb_token_box_bound(boxes[id-1])==998000000);
  sub->algorithm=GROUP_ALGORITHM_ALL;assert(itemdb_token_box_bound(boxes[id-1])==1000994000);
  group->random[1]=sub;assert(itemdb_token_box_bound(boxes[id-1])==2001988000);
 }
 for(int id:{6024,12781})for(int amount:{1,2,65535}){
  auto entry=std::make_shared<Entry>(Entry{static_cast<uint32_t>(id),static_cast<uint16_t>(amount)});
  int64 credit=pn_tokens::value(id)*amount;
  Session s;s.status.zeny=INT64_MAX-credit-1;s.mail.pending_zeny=1;
  award(s,entry);assert(s.status.zeny==INT64_MAX-1);
  s.status.zeny=INT64_MAX-credit;s.mail.pending_zeny=1;int before=errors;
  award(s,entry);assert(s.status.zeny==INT64_MAX-credit && errors==before+1);
 }
 puts("PASS: production group bounds, all five box mappings, random/all/subgroup totals, exact token values, signed64 and pending-mail edges");
}
"""
# Every configured diamond reward must have a pre-consumption box guard.
groups=[];current=''
for line in (ROOT/'db/re/item_group_db.yml').read_text().splitlines():
 if line.startswith('  - Group:'):current=line.split(': ')[1]
 if 'Item: 17Carat_Dia' in line:groups.append(current)
assert set(groups)=={'LIBRA_SCROLL','MS_VIRGO_SCROLL','SCROLL_OF_DEATH','SCROLL_OF_LIFE','MERCURY_SCROLL'} and len(groups)==5
assert 'getitem 6024' not in (ROOT/'npc/re/merchants/diamond.txt').read_text()
pc=(ROOT/'src/map/pc.cpp').read_text();assert pc.index('itemdb_token_box_bound(nameid)')<pc.index('int32 pc_useitem(')
with tempfile.TemporaryDirectory() as directory:
 path=Path(directory);(path/'test.cpp').write_text(cpp);(path/'retired_tokens.hpp').write_text((ROOT/'src/custom/retired_tokens.hpp').read_text())
 subprocess.run(['g++','-std=c++17','-O1','-fsanitize=undefined','-fno-sanitize-recover=all',str(path/'test.cpp'),'-o',str(path/'test')],check=True)
 subprocess.run([str(path/'test')],check=True)
