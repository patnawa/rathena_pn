// Raw floor admission creates only recoverable outputs; the source is ephemeral.
static pn_shop::Commit pet_floor_request(){
 auto r=request(pn_shop::Asset);r.stock_count=0;for(auto& stock:r.stocks)stock={};
 r.wallet_after=r.wallet_before;r.cash_after=r.cash_before;r.kafra_after=r.kafra_before;
 r.items[2]={};r.items[3]={};r.pet_count=2;r.counter_after=r.counter_before+2;
 for(int i=0;i<2;++i){r.pets[i].output=pet_output();r.pets[i].output.egg.unique_id=(uint64_t(r.char_id)<<32)|(r.counter_before+i+1);}
 return r;
}
static int pet_floor_sql_cases(const std::string& mode){
 auto r=pet_floor_request();require(pn_shop::valid(r),"raw floor asset identity valid");
 if(mode=="floor-pet-seed"){
  pet_seed();require(pn_shop_tosql(r,true)==pn_shop::Committed,"seed committed floor admission");
  newer();std::cout<<"PET_FLOOR_RESTART_SEEDED"<<std::endl;return 0;
 }
 if(mode=="floor-pet-restart"){
  auto before=pet_state();require(pn_shop_tosql(r,false)==pn_shop::Committed,"restart detached lookup resolves original receipt");require(pet_state()==before,"restart cannot reinstall stale inventory or duplicate output");
  require(result("SELECT COUNT(*) FROM pn_pet_entitlements WHERE char_id=99001313 AND claimed=0")=="2|\n","original owner retains both recoverable eggs after floor disappears");
  std::cout<<"PET_FLOOR_RESTART_PASS "<<checks<<std::endl;return 0;
 }
 require(mode=="floor-pets","known raw floor mode");pet_seed();auto before=pet_state();
 for(const char* boundary:{"BEFORE INSERT ON pet","BEFORE INSERT ON pn_pet_entitlements","BEFORE UPDATE ON `char`","BEFORE INSERT ON pn_shop_commits"}){
  std::cout<<"PET_FLOOR_FAULT_BOUNDARY "<<boundary<<std::endl;
  sql(std::string("CREATE TRIGGER pet_floor_fault ")+boundary+" FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='floor admission fault'");
  require(pn_shop_tosql(r,true)==pn_shop::Retry,"incomplete floor admission remains unresolved");require(pet_state()==before,"no output or receipt leaks across failed admission");sql("DROP TRIGGER pet_floor_fault");
 }
 sql("CREATE TRIGGER pet_floor_second BEFORE INSERT ON pn_pet_entitlements FOR EACH ROW BEGIN IF NEW.ordinal=1 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='second floor output fault'; END IF; END");
 require(pn_shop_tosql(r,true)==pn_shop::Retry,"second output failure remains unresolved");require(pet_state()==before,"second output failure rolls back first output too");sql("DROP TRIGGER pet_floor_second");
 require(pn_shop_tosql(r,false)==pn_shop::Rejected,"departed owner before commit cancels original receipt");require(result("SELECT COUNT(*) FROM pn_pet_entitlements")=="0|\n","rejected floor source has no reward and may be unlocked");
 pet_seed();const auto inventory_before=result("SELECT * FROM inventory ORDER BY id");
 // Raw eggs become entitlements; unchanged inventory produces no INSERT. Prove
 // that invariant directly instead of requiring an unreachable trigger to fail.
 for(const char* event:{"INSERT","UPDATE","DELETE"})sql(std::string("CREATE TRIGGER pet_floor_no_")+event+" BEFORE "+event+" ON inventory FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='unexpected floor inventory mutation'");
 require(pn_shop_tosql(r,true)==pn_shop::Committed,"two floor eggs admitted atomically without inventory mutation");
 require(result("SELECT * FROM inventory ORDER BY id")==inventory_before,"raw admission retains original inventory byte-for-byte");
 for(const char* event:{"INSERT","UPDATE","DELETE"})sql(std::string("DROP TRIGGER pet_floor_no_")+event);
 require(result("SELECT COUNT(*) FROM pet")=="2|\n" && result("SELECT COUNT(*) FROM pn_pet_entitlements WHERE claimed=0")=="2|\n","one recoverable identity per raw egg");
 require(result("SELECT zeny FROM `char` WHERE char_id=99001313")=="1000|\n","free floor reward charges no payment");
 auto committed=pet_state();require(pn_shop_tosql(r,false)==pn_shop::Committed,"disconnect after commit resolves success");require(pet_state()==committed,"duplicate disconnected lookup creates no second reward");
 auto other=r;other.char_id=99001314;require(pn_shop_tosql(other,false)==pn_shop::Retry,"new character cannot reuse original source identity");require(pet_state()==committed,"wrong-character retry cannot transfer reward");
 std::cout<<"PET_FLOOR_SQL_PASS "<<checks<<"; production Asset receipt rollback/disconnect/replay, ephemeral source tested separately"<<std::endl;return 0;
}
