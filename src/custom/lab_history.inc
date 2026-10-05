#include <common/runtime_identity.hpp>
// GPL-3.0-or-later. Diagnostic identities, not authentication tokens.
// Explicit field serialization avoids padding, inventory order and timer IDs.
BUILDIN_FUNC(pnlabidentity) {
 map_session_data* sd = nullptr;
 if (!script_rid2sd(sd)) { script_pushconststr(st, ""); return SCRIPT_CMD_SUCCESS; }
 const int mode = script_getnum(st,2);
 std::string value = "pnlab-v1";
 auto add = [&](auto number) { value += ":" + std::to_string(number); };
 if (mode == 0) {
  const auto& c=sd->status;
  add(c.class_); add(c.base_level); add(c.job_level);
  add(c.str); add(c.agi); add(c.vit); add(c.int_); add(c.dex); add(c.luk);
  add(c.pow); add(c.sta); add(c.wis); add(c.spl); add(c.con); add(c.crt);
  for (int slot=0;slot<EQI_MAX;++slot) {
   add(slot); const int i=sd->equip_index[slot];
   if(i<0 || i>=MAX_INVENTORY) { add(0); continue; }
   const auto& it=sd->inventory.u.items_inventory[i];
   add(it.nameid); add(it.equip); add(it.refine); add(it.enchantgrade); add(it.attribute);
   for(auto card:it.card)add(card);
   for(const auto& option:it.option) { add(option.id); add(option.value); add(option.param); }
  }
  for(int i=0;i<MAX_SKILL;++i) if(c.skill[i].id && c.skill[i].lv) {
   add(c.skill[i].id); add(c.skill[i].lv); add(c.skill[i].flag);
  }
 } else if(mode==1) {
  add(sd->sc.option); add(sd->sc.opt1); add(sd->sc.opt2); add(sd->sc.opt3);
  for(int i=SC_NONE+1;i<SC_MAX;++i) if(const auto* sc=sd->sc.getSCE(static_cast<sc_type>(i))) {
   add(i); add(sc->val1); add(sc->val2); add(sc->val3); add(sc->val4);
  }
 } else if(mode==2) {
  script_pushstrcopy(st,pn_runtime_identity::current().c_str());
  return SCRIPT_CMD_SUCCESS;
 } else { script_pushconststr(st, ""); return SCRIPT_CMD_SUCCESS; }
 char digest[33]; MD5_String(value.c_str(),digest); script_pushstrcopy(st,digest);
 return SCRIPT_CMD_SUCCESS;
}
