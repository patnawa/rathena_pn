// PN preparation safety policy. GPL-3.0-or-later.
#ifndef PN_PREPARATION_POLICY_HPP
#define PN_PREPARATION_POLICY_HPP
#include <string>
#include <cstring>
#include <cstdint>
namespace pn_preparation {
template<class Item> bool protected_item(const Item& it) {
    if (!it.nameid || it.amount<=0 || !it.identify || it.equip || it.equipSwitch ||
        it.favorite || it.refine || it.bound || it.expire_time || it.attribute || it.enchantgrade || it.unique_id) return true;
    for(auto value:it.card)if(value)return true;
    for(const auto& value:it.option)if(value.id || value.value || value.param)return true;
    return false;
}
// Item identity excludes mutable position, quantity and favorite status. A zero
// GUID is safe only when exactly one matching fingerprint remains in inventory.
template<class Item> std::string identity(Item it) {
    it.id=0;it.amount=0;it.equip=0;it.equipSwitch=0;it.favorite=0;
    const auto* bytes=reinterpret_cast<const unsigned char*>(&it);
    static const char hex[]="0123456789abcdef";std::string out;out.reserve(sizeof(it)*2);
    for(size_t i=0;i<sizeof(it);++i){out+=hex[bytes[i]>>4];out+=hex[bytes[i]&15];}
    return out;
}
inline bool supply(uint32_t id) {
    switch(id){case 501:case 502:case 503:case 504:case 505:case 506:case 601:case 602:return true;default:return false;}
}
}
#endif
