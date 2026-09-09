// ============================================================================
//  PN  /  DEVELOPMENT TOOLS
//  aloot_sets_test.cpp
// ----------------------------------------------------------------------------
//  Project contributions: (C) 2026 PN Development Team
//  License for project contributions: GPL-3.0-or-later; see LICENSE.
//  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/aloot_sets_test.cpp
//  Existing upstream authors, notices and other rights are retained.
// ============================================================================

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>
using int64=int64_t; using uint16=uint16_t; using t_itemid=uint32_t;
constexpr int AUTOLOOTITEM_SIZE=10;
#define safesnprintf std::snprintf
struct State {
    uint16 autoloot=0,autoloottype=0;
    t_itemid autolootid[AUTOLOOTITEM_SIZE]={};
    bool autolooting=false;
    bool operator==(const State& other) const {
        return autoloot==other.autoloot && autoloottype==other.autoloottype && autolooting==other.autolooting && std::equal(std::begin(autolootid),std::end(autolootid),other.autolootid);
    }
};
struct map_session_data { bool vars_ok=true; int account=1; State state; };
std::map<int,std::map<int64,std::string>> registry;
std::vector<std::string> messages;
int writes=0; bool fail_write=false;
int add_str(const char* name) { assert(std::string(name)=="#PNALootSet$"); return 1; }
int64 reference_uid(int id,int slot) { assert(id==1 && slot>=1 && slot<=10); return slot; }
char* pc_readregistry_str(map_session_data* sd,int64 key) {
    auto a=registry.find(sd->account);
    if(a==registry.end()) return nullptr;
    auto v=a->second.find(key);
    return v==a->second.end() || v->second.empty()?nullptr:v->second.data();
}
bool pc_setregistry_str(map_session_data* sd,int64 key,const char* value) {
    ++writes;
    if(fail_write) return false;
    assert(std::string(value).size()<=254);
    registry[sd->account][key]=value; return true;
}
void clif_displaymessage(int,const char* message) { messages.emplace_back(message); }
struct ItemDB { std::set<t_itemid> ids={501,502,503}; const void* find(t_itemid id) { return ids.count(id)?this:nullptr; } } item_db;
// PRODUCTION_HELPER
int main() {
    map_session_data sd;
    sd.state.autoloot=1250; sd.state.autoloottype=1533;
    sd.state.autolootid[0]=501; sd.state.autolootid[9]=502; sd.state.autolooting=true;
    const State initial=sd.state;
    assert(pn_aloot_set_command(sd,0,"save 1 Farming gear",false)==0);
    assert(writes==1 && sd.state==initial);
    const auto saved=registry;
    assert(pn_aloot_set_command(sd,0,"list",false)==0 && registry==saved);
    sd.state={};
    assert(pn_aloot_set_command(sd,0,"1",true)==0 && sd.state==initial);
    assert(registry==saved); // Loading never writes persistent settings.
    map_session_data second_character;
    assert(pn_aloot_set_command(second_character,0,"load 1",false)==0 && second_character.state==initial);
    second_character.account=2;
    assert(pn_aloot_set_command(second_character,0,"1",true)==-1);
    for(const char* input : {"0","11","-1","+1","1.0","1x","99999999999999999999","1 A","1 G","1 C","1 M","1 junk"}) {
        assert(pn_aloot_set_command(sd,0,input,true)==-1 && sd.state==initial);
    }
    for(const char* input : {"save","save 0","save 1 ^FF0000Bad","save 1 abc\ndef","save 1 123456789012345678901234567890123","delete 1 extra","list extra","help extra","unknown 1"}) {
        const auto before=registry;
        assert(pn_aloot_set_command(sd,0,input,false)==-1 && sd.state==initial && registry==before);
    }
    for(const char* corrupt : {
        "2 1250 1533 10 501 0 0 0 0 0 0 0 0 502 BadVersion",
        "1 -1 1533 10 501 0 0 0 0 0 0 0 0 502 BadRate",
        "1 10001 1533 10 501 0 0 0 0 0 0 0 0 502 BadRate",
        "1 1250 2 10 501 0 0 0 0 0 0 0 0 502 BadType",
        "1 1250 1533 9 501 0 0 0 0 0 0 0 502 BadCount",
        "1 1250 1533 10 501 0 0 0 0 0 0 0 0 501 Duplicate",
        "1 1250 1533 10 501 0 0 0 0 0 0 0 0 999 MissingItem",
        "1 1250 1533 10 501 0 0 0 0 0 0 0 0 4294967296 Overflow",
        "1 1250 1533 10 501 0 0 0 0 0 0 0 0 -1 Negative",
        "1 1250 1533 10 501 0 0 0 0 0 0 0 0 502",
        "truncated"
    }) {
        registry[1][1]=corrupt;
        assert(pn_aloot_set_command(sd,0,"1",true)==-1 && sd.state==initial);
    }
    registry=saved;
    item_db.ids.erase(502);
    assert(pn_aloot_set_command(sd,0,"1",true)==-1 && sd.state==initial);
    item_db.ids.insert(502);
    fail_write=true;
    assert(pn_aloot_set_command(sd,0,"save 1 changed",false)==-1 && registry==saved);
    fail_write=false;
    sd.vars_ok=false;
    assert(pn_aloot_set_command(sd,0,"save 1",false)==-1 && registry==saved);
    sd.vars_ok=true;
    assert(pn_aloot_set_command(sd,0,"delete 1",false)==0 && sd.state==initial);
    assert(pn_aloot_set_command(sd,0,"1",true)==-1 && sd.state==initial);
    sd.state={};
    assert(pn_aloot_set_command(sd,0,"save 10",false)==0);
    sd.state=initial;
    assert(pn_aloot_set_command(sd,0,"10",true)==0 && sd.state==State{});
    std::cout << "PASS: account loot set save/load/list/delete, account isolation, strict parsing, corruption rejection, atomic preferences and failed writes\n";
}
