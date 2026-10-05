#include <custom/item_use.hpp>
#include <map/pet.hpp>
extern "C" int __wrap_main(int argc,char** argv){
 deny_network();static char server[]="item-use-catalog";SERVER_NAME=server;
 malloc_init();db_init();do_init_database();timer_init();do_init_script();battle_set_defaults();
 const std::string dir=argv[1];
 auto data=read(dir+"/items.yml");auto items=ryml::parse_in_arena(ryml::to_csubstr(data));for(auto node:items["Body"])check(item_db.parseBodyNode(node)==1,"catalog item metadata parses");
 std::ifstream eggfile(dir+"/eggs.txt");uint32 egg=0,petclass=10000;while(eggfile>>egg){auto pet=std::make_shared<s_pet_db>();pet->class_=petclass;pet->EggID=egg;pet_db.put(petclass++,pet);}
 data=read(dir+"/groups.yml");auto groups=ryml::parse_in_arena(ryml::to_csubstr(data));for(auto node:groups["Body"])check(itemdb_group.parseBodyNode(node)==1,"catalog group membership parses");
 std::ifstream functionfile(dir+"/functions.tsv");std::string name,path;std::vector<std::pair<std::string,std::string>> functions;
 while(functionfile>>name>>path){functions.emplace_back(name,path);strdb_put(script_get_userfunc_db(),name.c_str(),parse_script("{ return; }",name.c_str(),1,0));}
 for(const auto& function:functions){auto source=read(dir+"/"+function.second);const unsigned before=errors;auto* code=parse_script(source.c_str(),function.first.c_str(),1,SCRIPT_RETURN_EMPTY_SCRIPT);auto* old=static_cast<script_code*>(strdb_get(script_get_userfunc_db(),function.first.c_str()));
  if(!code || errors!=before){printf("COMPILE_FAILURE function %s file=%s errors=%u\n",function.first.c_str(),function.second.c_str(),errors-before);strdb_remove(script_get_userfunc_db(),function.first.c_str());if(code)script_free_code(code);}
  else strdb_put(script_get_userfunc_db(),function.first.c_str(),code);script_free_code(old);
 }
 std::ifstream scriptfile(dir+"/scripts.tsv");uint32 id;std::vector<uint32> scripts;
 while(scriptfile>>id>>path){auto source=read(dir+"/"+path);const unsigned before=errors;auto* code=parse_script(source.c_str(),path.c_str(),1,SCRIPT_RETURN_EMPTY_SCRIPT);if(!code || errors!=before)printf("COMPILE_FAILURE item %u file=%s errors=%u\n",id,path.c_str(),errors-before);item_db.find(id)->script=code;scripts.push_back(id);}
 for(auto ident:scripts)printf("ITEM_EFFECT %u %u\n",ident,script_item_use_effects(item_db.find(ident)->script));
 itemdb_group.clear();pet_db.clear();item_db.clear();do_final_script();timer_final();db_final();malloc_final();return errors?1:0;
}
