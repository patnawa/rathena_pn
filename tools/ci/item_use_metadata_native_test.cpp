#include <custom/item_use.hpp>
#include <map/pet.hpp>
extern "C" int __wrap_main(int argc,char** argv){
 deny_network();static char server[]="item-use-metadata-test";SERVER_NAME=server;
 malloc_init();db_init();do_init_database();timer_init();do_init_script();battle_set_defaults();
 auto data=read(std::string(argv[1])+"/items.yml");auto items=ryml::parse_in_arena(ryml::to_csubstr(data));
 for(auto n:items["Body"])check(item_db.parseBodyNode(n)==1,"item parses");
 item_db.find(501)->type=IT_PETEGG;
 auto pet=std::make_shared<s_pet_db>();pet->class_=1002;pet->EggID=501;pet_db.put(1002,pet);
 for(int id:{60000,60001}){auto group=std::make_shared<s_item_group_db>();group->id=id;auto random=std::make_shared<s_item_group_random>();auto entry=std::make_shared<s_item_group_entry>();entry->nameid=id==60000?503:501;random->data[0]=entry;group->random[0]=random;itemdb_group.put(id,group);}
 strdb_put(script_get_userfunc_db(),"PN_MetaWarp",parse_script("{ warp \"prontera\",150,150; return; }","warp",1,0));
 strdb_put(script_get_userfunc_db(),"PN_MetaGrant",parse_script("{ getitem getarg(0),1; return; }","grant",1,0));
 strdb_put(script_get_userfunc_db(),"PN_MetaRand",parse_script("{ return getarg(rand(getargcount())); }","random",1,0));
 struct Test {const char* script;uint32 required,forbidden;};
 const Test tests[]={
  {"{ getitem 503,1; }",0,PN_ITEM_PET},
  {"{ getitem 501,1; }",PN_ITEM_PET,PN_ITEM_WORLD},
  {"{ getitem \"Yellow_Potion\",1; }",0,PN_ITEM_PET},
  {"{ getitembound 501,1,1; }",PN_ITEM_PET,0},
  {"{ .@id=503; getitem .@id,1; }",0,PN_ITEM_PET},
  {"{ setarray .@ids[0],502,503; .@n=select(\"First:Second\")-1; getitem .@ids[.@n],1; }",PN_ITEM_WORLD,PN_ITEM_PET},
  {"{ setarray .@ids[0],501,503; getitem .@ids[rand(2)],1; }",PN_ITEM_PET,0},
  {"{ getitem #DynamicReward,1; }",PN_ITEM_PET|PN_ITEM_DYNAMIC,0},
  {"{ callfunc \"PN_MetaWarp\"; }",PN_ITEM_WORLD,PN_ITEM_PET},
  {"{ .@f$=\"PN_MetaWarp\"; callfunc .@f$; }",PN_ITEM_WORLD,PN_ITEM_PET},
  {"{ callfunc \"PN_MetaGrant\",503; }",0,PN_ITEM_PET},
  {"{ callfunc \"PN_MetaGrant\",501; }",PN_ITEM_PET,0},
  {"{ getgroupitem 60000; }",0,PN_ITEM_PET},
  {"{ getgroupitem 60001; }",PN_ITEM_PET,0},
  {"{ mes \"Choice\"; next; getitem 501,1; }",PN_ITEM_PET|PN_ITEM_WORLD,0},
  {"{ warp \"prontera\",150,150; }",PN_ITEM_WORLD,PN_ITEM_PET},
  {"{ .@id=503; while(rand(2)){getitem .@id,1; .@id=501;} }",PN_ITEM_PET,0},
  {"{ .@x=getiteminfo(501,2); getitem 503,1; }",0,PN_ITEM_PET},
  {"{ .@id=callfunc(\"PN_MetaRand\",502,503); getitem .@id,1; }",0,PN_ITEM_PET},
  {"{ .@id=callfunc(\"PN_MetaRand\",501,503); getitem .@id,1; }",PN_ITEM_PET,0},
  {"{ .@id=500+select(\"First:Second\"); mes getitemname(.@id); getitem .@id,1; }",PN_ITEM_PET|PN_ITEM_WORLD,0},
  {"{ .@id=502+select(\"First:Second\"); mes getitemname(.@id); rentitem .@id,10; }",PN_ITEM_WORLD,PN_ITEM_PET},
  {"{ getitem 4294967797,1; }",PN_ITEM_PET,0},
  {"{ getordinaryitem #UnknownReward,1; }",0,PN_ITEM_PET},
  {"{ for(.@i=0;.@i<100;.@i++)getordinaryitem .@i,1; }",0,PN_ITEM_PET},
  {"{ .@id=600; while(.@id>0){getitem .@id,1; .@id--;} }",PN_ITEM_PET,0},
 };
 for(const auto& test:tests){++cases;auto* code=parse_script(test.script,"metadata",1,0);check(code!=nullptr,"compiler accepts script");auto effects=script_item_use_effects(code);bool ok=(effects&test.required)==test.required && !(effects&test.forbidden);if(!ok)++errors;printf("ITEM_USE_METADATA case=%u effects=%u %s\n",cases,effects,ok?"PASS":"FAIL");script_free_code(code);}
 printf("ITEM_USE_METADATA cases=%u failures=%u\n",cases,errors);
 itemdb_group.clear();pet_db.clear();pet.reset();item_db.clear();do_final_script();timer_final();db_final();malloc_final();return errors?1:0;
}
