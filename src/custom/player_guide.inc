// GPL-3.0-or-later. Live character state only; never infer timers from stale SQL.
BUILDIN_FUNC(pnquestremaining) {
 map_session_data* sd=nullptr;
 if(!script_rid2sd(sd)){script_pushint(st,-1);return SCRIPT_CMD_SUCCESS;}
 const int id=script_getnum(st,2);
 for(int i=0;i<sd->num_quests;++i)if(sd->quest_log[i].quest_id==id){
  const int64 expiry=sd->quest_log[i].time;
  script_pushint(st,expiry?std::max<int64>(0,expiry-static_cast<int64>(time(nullptr))):0);
  return SCRIPT_CMD_SUCCESS;
 }
 script_pushint(st,-1);return SCRIPT_CMD_SUCCESS;
}
