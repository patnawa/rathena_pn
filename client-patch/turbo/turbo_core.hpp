// PN held-key turbo scheduler. GPL-3.0-or-later. No Windows or network dependency.
#pragma once
#include <array>
#include <cstdint>
namespace pn_turbo {
enum Mode { Off, Repeat, Smart, AltRight };
struct Binding { Mode mode=Off; unsigned delay=10; };
struct Action { unsigned key=0; Mode mode=Off; };
class Core {
    std::array<bool,256> held{},captured{},blocked{};
    std::array<uint64_t,256> due{};
    unsigned cursor=0;
public:
    std::array<Binding,256> bindings{};
    bool enabled=false;
    void interrupt(){for(unsigned k=0;k<256;k++)if(held[k])blocked[k]=true;}
    void enable(bool value){enabled=value;interrupt();}
    bool event(unsigned key,bool down,bool allowed,uint64_t now){
        if(key>=256)return false;
        if(!down){bool swallow=captured[key];held[key]=captured[key]=blocked[key]=false;return swallow;}
        if(held[key])return captured[key];
        held[key]=true;
        if(bindings[key].mode!=Off && enabled && allowed){captured[key]=true;due[key]=now;return true;}
        blocked[key]=true;return false;
    }
    bool is_held(unsigned key)const{return key<256 && held[key];}
    Action next(uint64_t now,bool allowed){
        if(!allowed || !enabled){interrupt();return {};}
        for(unsigned n=0;n<256;n++){
            unsigned key=(cursor+n)%256;
            if(held[key] && captured[key] && !blocked[key] && bindings[key].mode!=Off && now>=due[key]){
                cursor=(key+1)%256;due[key]=now+bindings[key].delay;return {key,bindings[key].mode};
            }
        }
        return {};
    }
    bool valid(Action a)const{return enabled && a.key<256 && held[a.key] && captured[a.key] && !blocked[a.key];}
    void completed(Action a,uint64_t now){if(a.key<256)due[a.key]=now+bindings[a.key].delay;}
};
}
