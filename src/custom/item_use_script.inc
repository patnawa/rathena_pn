// Interpret compiled argument expressions, never source text or item ID lists.
namespace {
struct PnScriptValue {
 enum Kind { Unknown, Values, Ref, Function, Args } kind=Unknown;
 std::vector<int64> ids; std::vector<std::string> texts; std::string name;
 static PnScriptValue number(int64 n){PnScriptValue v;v.kind=Values;v.ids.push_back(n);return v;}
 static PnScriptValue string(std::string s){PnScriptValue v;v.kind=Values;v.texts.push_back(std::move(s));return v;}
};
static PnScriptValue pn_value_union(PnScriptValue a,const PnScriptValue& b){
 if(a.kind!=PnScriptValue::Values || b.kind!=PnScriptValue::Values)return {};
 for(auto n:b.ids)if(std::find(a.ids.begin(),a.ids.end(),n)==a.ids.end())a.ids.push_back(n);
 for(const auto& s:b.texts)if(std::find(a.texts.begin(),a.texts.end(),s)==a.texts.end())a.texts.push_back(s);
 return a.ids.size()+a.texts.size()>128?PnScriptValue{}:a;
}
static bool pn_metadata_pet(int64 id){
 const auto narrowed=static_cast<t_itemid>(id);if(!narrowed)return false;
 const auto data=item_db.find(narrowed);
 return (data && data->type==IT_PETEGG) || pet_db_search(static_cast<int32>(narrowed),PET_EGG)!=nullptr;
}
static uint32 pn_reward_effect(PnScriptValue value,bool group){
 if(value.kind!=PnScriptValue::Values)return PN_ITEM_PET|PN_ITEM_DYNAMIC;
 for(const auto& name:value.texts){if(group)return PN_ITEM_PET|PN_ITEM_DYNAMIC;const auto data=item_db.searchname(name.c_str());if(data)value.ids.push_back(data->nameid);}
 for(auto id:value.ids){
  if(!group){if(pn_metadata_pet(id))return PN_ITEM_PET;continue;}
  const auto data=itemdb_group.find(static_cast<uint16>(id));if(!data)continue;
  for(const auto& subgroup:data->random)for(const auto& entry:subgroup.second->data)if(pn_metadata_pet(entry.second->nameid))return PN_ITEM_PET;
 }
 return 0;
}
static uint32 pn_builtin_effect(const std::string& name){
 const char* b=name.c_str();uint32 e=0;
 if(name=="makepet" || name=="pet" || name=="catchpet" || name=="getpetreward")e|=PN_ITEM_PET;
 if(!strncmp(b,"warp",4) || name=="unitwarp" || name=="instance_enter" || name=="savepoint" || name=="kick" || name=="kickall" ||
 name=="equip" || name=="unequip" || name=="mes" || name=="next" || name=="input" || name=="select" || name=="menu" ||
 !strncmp(b,"sc_start",8) || name=="sc_end" || name=="heal" || name=="itemheal" || name=="percentheal" ||
 !strncmp(b,"statusup",8) || !strncmp(b,"bonus",5) || name=="skill" || name=="getexp" || name=="setquest" ||
 name=="completequest" || name=="erasequest" || name=="sleep" || name=="sleep2" || name=="doevent" || name=="donpcevent")e|=PN_ITEM_WORLD;
 if(name=="atcommand")e|=PN_ITEM_PET|PN_ITEM_WORLD|PN_ITEM_DYNAMIC;
 return e;
}
static uint32 pn_script_effects(const script_code* code,std::vector<const script_code*>& seen,const std::vector<PnScriptValue>& parameters,PnScriptValue* returned=nullptr){
 if(!code || !code->script_buf)return 0;
 if(std::find(seen.begin(),seen.end(),code)!=seen.end())return PN_ITEM_PET|PN_ITEM_WORLD|PN_ITEM_DYNAMIC;
 seen.push_back(code);std::map<std::string,PnScriptValue> locals;uint32 effects=0;
 // Union all assignments and repeat to cover backward edges. Unknowns stay
 // unknown; exceeding this bound refuses certification as ordinary-only.
 for(unsigned pass=0;pass<16;++pass){
  bool changed=false,has_return=false;std::vector<std::string> evolving;PnScriptValue return_value;effects=0;int32 pos=0;std::vector<PnScriptValue> stack;
  auto read=[&](PnScriptValue v){if(v.kind!=PnScriptValue::Ref)return v;auto it=locals.find(v.name);return it==locals.end()?PnScriptValue{}:it->second;};
  while(pos<code->script_size){
   const auto op=get_com(code->script_buf,&pos);
   if(op==C_INT){stack.push_back(PnScriptValue::number(get_num(code->script_buf,&pos)));continue;}
   if(op==C_STR){const int32 start=pos;while(pos<code->script_size && code->script_buf[pos])++pos;if(pos>=code->script_size){effects|=PN_ITEM_PET|PN_ITEM_DYNAMIC;break;}stack.push_back(PnScriptValue::string(reinterpret_cast<const char*>(code->script_buf+start)));++pos;continue;}
   if(op==C_POS || op==C_USERFUNC_POS){pos+=3;stack.emplace_back();continue;}
   if(op==C_NAME){
    if(pos+3>code->script_size){effects|=PN_ITEM_PET|PN_ITEM_DYNAMIC;break;}
    const uint32 id=GETVALUE(code->script_buf,pos);pos+=3;PnScriptValue v;
    if(id<static_cast<uint32>(str_num)){v.name=get_str(id);v.kind=str_data[id].type==C_FUNC?PnScriptValue::Function:PnScriptValue::Ref;}stack.push_back(v);continue;
   }
   if(op==C_ARG){PnScriptValue v;v.kind=PnScriptValue::Args;stack.push_back(v);continue;}
   if(op==C_EOL){stack.clear();continue;}
   if(op==C_REF || op==C_NOP)continue;
   if(op==C_FUNC){
    size_t marker=stack.size();while(marker && stack[marker-1].kind!=PnScriptValue::Args)--marker;
    if(marker<2 || stack[marker-2].kind!=PnScriptValue::Function){stack.clear();effects|=PN_ITEM_PET|PN_ITEM_DYNAMIC;continue;}
    const auto name=stack[marker-2].name;std::vector<PnScriptValue> args(stack.begin()+marker,stack.end());stack.resize(marker-2);
    auto arg=[&](size_t i){return i<args.size()?read(args[i]):PnScriptValue{};};PnScriptValue result;effects|=pn_builtin_effect(name);
    if(name=="getitem" || name=="getitem2" || name=="getitem3" || name=="getitem4" ||
       name=="getitembound" || name=="getitembound2" || name=="getitembound3" || name=="getitembound4" ||
       name=="rentitem" || name=="rentitem2" || name=="rentitem3" || name=="rentitem4" || name=="getnameditem")effects|=pn_reward_effect(arg(0),false);
    if(name=="getgroupitem" || name=="getrandgroupitem")effects|=pn_reward_effect(arg(0),true);
    if(name=="getelementofarray" && !args.empty())result=args[0];
    if(name=="set" || name=="setr" || name=="setarray"){
     result=arg(1);
     if(args.empty() || args[0].kind!=PnScriptValue::Ref ||
        (args[0].name.compare(0,2,".@")!=0 && args[0].name!="Zeny" && args[0].name!="#CASHPOINTS" && args[0].name!="#KAFRAPOINTS"))effects|=PN_ITEM_WORLD;
     if(!args.empty() && args[0].kind==PnScriptValue::Ref && args[0].name.compare(0,2,".@")==0){
      if(name=="setarray")for(size_t i=2;i<args.size();++i)result=pn_value_union(result,arg(i));
      auto old=locals.find(args[0].name);auto value=old==locals.end()?result:pn_value_union(old->second,result);
      if(old==locals.end() || old->second.kind!=value.kind || old->second.ids!=value.ids || old->second.texts!=value.texts){locals[args[0].name]=value;evolving.push_back(args[0].name);changed=true;}
     }
    }
    if(name=="getarg"){
     auto index=arg(0);if(index.kind==PnScriptValue::Values && index.ids.size()==1 && index.texts.empty() && index.ids[0]>=0){size_t i=static_cast<size_t>(index.ids[0]);result=i<parameters.size()?parameters[i]:arg(1);}
     else if(!parameters.empty()){result=parameters[0];for(size_t i=1;i<parameters.size();++i)result=pn_value_union(result,parameters[i]);if(args.size()>1)result=pn_value_union(result,arg(1));}
    }
    if(name=="getargcount")result=PnScriptValue::number(parameters.size());
    if(name=="select"){
     size_t choices=0;bool known=true;
     for(size_t i=0;i<args.size();++i){const auto value=arg(i);if(value.kind!=PnScriptValue::Values || !value.ids.empty() || value.texts.size()!=1){known=false;break;}choices+=1+std::count(value.texts[0].begin(),value.texts[0].end(),':');}
     if(known && choices && choices<=128){result.kind=PnScriptValue::Values;for(size_t i=1;i<=choices;++i)result.ids.push_back(i);}
    }
    if(name=="return"){const auto value=arg(0);return_value=has_return?pn_value_union(return_value,value):value;has_return=true;}
    if(name=="callfunc"){
     auto target=arg(0);
     if(target.kind!=PnScriptValue::Values || !target.ids.empty() || target.texts.empty())effects|=PN_ITEM_PET|PN_ITEM_WORLD|PN_ITEM_DYNAMIC;
     else {bool first=true;for(const auto& function:target.texts){
      const auto* callee=static_cast<script_code*>(strdb_get(userfunc_db,function.c_str()));std::vector<PnScriptValue> passed;
      for(size_t i=1;i<args.size();++i)passed.push_back(arg(i));PnScriptValue value;
      effects|=callee?pn_script_effects(callee,seen,passed,&value):PN_ITEM_PET|PN_ITEM_DYNAMIC;
      result=first?value:pn_value_union(result,value);first=false;
     }}
    }
    if(name=="consumeitem"){
     auto value=arg(0);if(value.kind!=PnScriptValue::Values)effects|=PN_ITEM_PET|PN_ITEM_WORLD|PN_ITEM_DYNAMIC;
     else{for(const auto& s:value.texts){const auto data=item_db.searchname(s.c_str());if(data)value.ids.push_back(data->nameid);}for(auto id:value.ids){const auto data=item_db.find(static_cast<t_itemid>(id));if(data)effects|=pn_script_effects(data->script,seen,{});}}
    }
    // Builtins can write through reference arguments (input, copyarray, etc.).
    // Keep only explicitly understood assignments and known read-only calls.
    // Indirect setd can address any local, so invalidate all local knowledge.
    const bool readonly=name=="getordinaryitem" || name=="getitemname" || name=="countitem" || name=="rentalcountitem" || name=="checkweight" || name=="getnameditem" || name=="getelementofarray" || name=="getarraysize" || name=="getarg" || name=="return" ||
       name=="getitem" || name=="getitem2" || name=="getitem3" || name=="getitem4" ||
       name=="getitembound" || name=="getitembound2" || name=="getitembound3" || name=="getitembound4" ||
       name=="rentitem" || name=="rentitem2" || name=="rentitem3" || name=="rentitem4" ||
       name=="getgroupitem" || name=="getrandgroupitem" || name=="consumeitem";
    if(name!="set" && name!="setr" && name!="setarray" && !readonly){
     for(size_t i=name=="callfunc"?1:0;i<args.size();++i){const auto& value=args[i];if(value.kind==PnScriptValue::Ref){auto found=locals.find(value.name);if(found!=locals.end() && found->second.kind!=PnScriptValue::Unknown){found->second={};changed=true;}}}
    }
    if(name=="setd"){effects|=PN_ITEM_WORLD;for(auto& local:locals)if(local.second.kind!=PnScriptValue::Unknown){local.second={};changed=true;}}
    stack.push_back(result);continue;
   }
   // Arithmetic does not retain a literal operand as a falsely known ID.
   size_t count=op==C_OP3?3:(op==C_NEG || op==C_NOT || op==C_LNOT?1:2);PnScriptValue result;
   if(op==C_OP3 && stack.size()>=3)result=pn_value_union(read(stack[stack.size()-2]),read(stack.back()));
   if((op==C_ADD || op==C_SUB) && stack.size()>=2){
    const auto a=read(stack[stack.size()-2]),b=read(stack.back());
    if(a.kind==PnScriptValue::Values && b.kind==PnScriptValue::Values && a.texts.empty() && b.texts.empty() && a.ids.size()*b.ids.size()<=128){
     result.kind=PnScriptValue::Values;bool overflow=false;
     for(auto x:a.ids)for(auto y:b.ids){
      if(op==C_SUB){if(y==INT64_MIN){overflow=true;continue;}y=-y;}
      if((y>0 && x>INT64_MAX-y) || (y<0 && x<INT64_MIN-y)){overflow=true;continue;}
      const auto value=x+y;if(std::find(result.ids.begin(),result.ids.end(),value)==result.ids.end())result.ids.push_back(value);
     }
     if(overflow)result={};
    }
   }
   if(stack.size()>=count)stack.resize(stack.size()-count);else stack.clear();stack.push_back(result);
  }
  if(!changed){if(returned)*returned=return_value;seen.pop_back();return effects;}
  // Widen growing loop induction values before the final pass. Unknown reward
  // arguments still require a pet transaction; ordinary-only calls do not.
  if(pass==14)for(const auto& name:evolving)locals[name]={};
 }
 seen.pop_back();return effects|PN_ITEM_PET|PN_ITEM_DYNAMIC;
}
}
uint32 script_item_use_effects(const script_code* code){std::vector<const script_code*> seen;return pn_script_effects(code,seen,{});}
