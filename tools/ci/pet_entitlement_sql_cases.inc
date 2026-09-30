// Executes linked char/int_pet.o code against disposable InnoDB. The payment
// and inventory orchestration below is a fixture, not a tested map producer.
static pn_pet::Key pet_key(){pn_pet::Key k{};k.account_id=990013;k.char_id=99001313;k.nonce_hi=91;k.nonce_lo=92;k.sequence=1;return k;}
static pn_pet::Output pet_output(){
    pn_pet::Output o{};o.egg.nameid=9001;o.egg.amount=1;o.egg.identify=1;o.egg.bound=2;
    o.egg.refine=3;o.egg.option[0].id=1;o.egg.option[0].value=7;o.egg.unique_id=998877;
    o.pet_class=1002;o.level=1;o.intimacy=250;std::strcpy(o.name,"Poring fixture");return o;
}
static std::string pet_state(bool receipts=true){return state(receipts)+result("SELECT * FROM pet ORDER BY pet_id")+
    result("SELECT id,account_id,char_id,nonce_hi,nonce_lo,sequence,ordinal,pet_id,HEX(payload),HEX(egg),claimed FROM pn_pet_entitlements ORDER BY id");}
static void pet_seed(){seed();sql("DELETE FROM pn_pet_entitlements");sql("DELETE FROM pet");}
static pn_pet::Result pet_paid(const pn_pet::Key& k,const pn_pet::Output& o,pn_pet::Entitlement& e){
    require(Sql_BeginTransaction(sql_handle)==SQL_SUCCESS,"begin pet/payment fixture transaction");
    auto outcome=pn_pet::issue_locked(k,o,e);
    bool ok=outcome==pn_pet::Result::Issued || outcome==pn_pet::Result::Pending || outcome==pn_pet::Result::AlreadyClaimed;
    if(outcome==pn_pet::Result::Issued)ok=Sql_Query(sql_handle,"UPDATE `char` SET zeny=zeny-100 WHERE char_id=%u AND account_id=%u AND zeny>=100",k.char_id,k.account_id)==SQL_SUCCESS && Sql_NumRowsAffected(sql_handle)==1;
    if(Sql_EndTransaction(sql_handle,ok)!=SQL_SUCCESS || !ok)return pn_pet::Result::Error;
    return outcome;
}

// These cases execute the production immutable asset transaction, including its
// stock/payment/inventory writes and receipt. Map producer planning is separate.
static int pet_asset_cases(){
    pet_seed();auto purchase=request();purchase.pet_count=2;purchase.counter_after=8;
    for(int i=0;i<2;++i){purchase.pets[i].output=pet_output();
        purchase.pets[i].output.egg.unique_id=(uint64_t(purchase.char_id)<<32)|(6+i);}
    require(pn_shop::valid(purchase),"versioned pet purchase validates");
    auto initial=pet_state();
    auto old=purchase;old.version=1;
    require(pn_shop_tosql(old,true)==pn_shop::Retry,"old protocol version cannot execute new layout");
    require(pet_state()==initial,"wrong version changes nothing");
    auto original_assets=pet_state(false);
    require(pn_shop_tosql(purchase,false)==pn_shop::Rejected,"nonowner pet purchase rejected durably");
    require(pet_state(false)==original_assets,"nonowner rejection creates no pet or payment");
    pet_seed();sql("UPDATE market SET amount=2 WHERE nameid=502");
    original_assets=pet_state(false);
    require(pn_shop_tosql(purchase,true)==pn_shop::Rejected,"stale pet shop stock rejected");
    require(pet_state(false)==original_assets,"stale stock rejection creates no entitlement");
    pet_seed();initial=pet_state();
    for(const std::string& fault:std::vector<std::string>{
        "BEFORE INSERT ON pet FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='pet insert'",
        "BEFORE INSERT ON pn_pet_entitlements FOR EACH ROW BEGIN IF NEW.ordinal=1 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='second pet output'; END IF; END",
        "BEFORE UPDATE ON market FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='stock write'",
        "BEFORE INSERT ON inventory FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='inventory write'",
        "BEFORE UPDATE ON `char` FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='wallet write'",
        "BEFORE INSERT ON acc_reg_num FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='cash write'",
        "BEFORE INSERT ON pn_shop_commits FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='receipt write'"}){
        sql("CREATE TRIGGER pet_asset_fault "+fault);
        require(pn_shop_tosql(purchase,true)==pn_shop::Retry,"pet purchase boundary failure remains retryable");
        require(pet_state()==initial,"pet, stock, inventory, currencies and receipt roll back together");
        sql("DROP TRIGGER pet_asset_fault");
    }
    require(pn_shop_tosql(purchase,true)==pn_shop::Committed,"production pet purchase commits");
    require(result("SELECT zeny,uniqueitem_counter FROM `char` WHERE char_id=99001313")=="800|8|\n","purchase debit/counter committed");
    require(result("SELECT COUNT(*) FROM pet")=="2|\n","purchase creates both pet identities");
    require(result("SELECT claimed,COUNT(*) FROM pn_pet_entitlements GROUP BY claimed")=="0|2|\n","purchase leaves two recoverable outputs");
    require(result("SELECT SUM(amount) FROM market")=="8|\n","pet purchase stock consumed once");
    auto committed=pet_state();
    require(pn_shop_tosql(purchase,false)==pn_shop::Committed,"departed session can resolve original receipt");
    require(pet_state()==committed,"purchase replay does not create or charge twice");
    pn_shop::Commit claim{};claim.length=sizeof(claim);claim.kind=pn_shop::PetClaim;
    claim.account_id=purchase.account_id;claim.char_id=purchase.char_id;
    claim.nonce_hi=purchase.nonce_hi;claim.nonce_lo=purchase.nonce_lo;claim.sequence=2;
    claim.wallet_before=claim.wallet_after=800;claim.cash_before=claim.cash_after=1000;
    claim.kafra_before=claim.kafra_after=200;claim.counter_before=claim.counter_after=8;claim.pet_count=2;
    memcpy(claim.items,purchase.items,sizeof(claim.items));
    require(Sql_BeginTransaction(sql_handle)==SQL_SUCCESS,"read original pet outputs under lock");
    for(uint16_t i=0;i<2;++i){
        pn_pet::Key key{};key.account_id=purchase.account_id;key.char_id=purchase.char_id;
        key.nonce_hi=purchase.nonce_hi;key.nonce_lo=purchase.nonce_lo;key.sequence=purchase.sequence;key.ordinal=i;
        pn_pet::Entitlement entitlement{};
        require(pn_pet::issue_locked(key,purchase.pets[i].output,entitlement)==pn_pet::Result::Pending,"read original claim identity");
        auto& c=claim.pets[i];c.claim_id=entitlement.id;c.inventory_index=4+i;c.output.egg=entitlement.egg;
        claim.items[4+i]=entitlement.egg;
    }
    require(Sql_EndTransaction(sql_handle,true)==SQL_SUCCESS,"finish original output lookup");
    require(pn_shop::valid(claim),"exact two-output claim validates");
    auto duplicate=claim;duplicate.pets[1]=duplicate.pets[0];
    require(!pn_shop::valid(duplicate),"duplicate claim identity/slot rejected before SQL");
    duplicate=claim;duplicate.items[6]=duplicate.items[4];
    require(!pn_shop::valid(duplicate),"same pet cannot appear twice in claimed inventory");
    auto altered=claim;altered.pets[1].output.egg.refine++;altered.items[5].refine++;
    require(pn_shop::valid(altered),"metadata mismatch reaches durable entitlement validation");
    original_assets=pet_state(false);
    require(pn_shop_tosql(altered,true)==pn_shop::Rejected,"second output conflict is a durable rejection");
    require(pet_state(false)==original_assets,"second conflict rolls back first claim marker");
    claim.sequence=3;committed=pet_state();
    sql("CREATE TRIGGER pet_asset_fault BEFORE INSERT ON inventory FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='claim inventory failure'");
    require(pn_shop_tosql(claim,true)==pn_shop::Retry,"claim inventory failure not acknowledged");
    require(pet_state()==committed,"claim markers and inventory roll back together");
    sql("DROP TRIGGER pet_asset_fault");
    require(pn_shop_tosql(claim,true)==pn_shop::Committed,"production claim atomically installs both eggs");
    require(result("SELECT claimed,COUNT(*) FROM pn_pet_entitlements GROUP BY claimed")=="1|2|\n","both outputs durably claimed");
    require(result("SELECT COUNT(*) FROM inventory WHERE char_id=99001313 AND card0=256")=="2|\n","exactly two pet eggs delivered");
    auto second=claim;second.sequence=4;original_assets=pet_state(false);
    require(pn_shop_tosql(second,true)==pn_shop::Rejected,"new request cannot claim an old output twice");
    require(pet_state(false)==original_assets,"duplicate claim does not reinstall inventory");
    // A cancelled/failed tame commits only its reserved lure. Existing pet IDs
    // and ordinary metadata must survive this zero-output ItemUse transaction.
    auto cost_only=claim;cost_only.kind=pn_shop::ItemUse;cost_only.sequence=100;
    cost_only.pet_count=0;for(auto& output:cost_only.pets)output={};
    cost_only.items[1].amount=19;
    require(pn_shop::valid(cost_only),"cost-only ItemUse validates without pet outputs");
    const auto retained_pets=result("SELECT * FROM pet ORDER BY pet_id");
    const auto retained_eggs=result("SELECT id,nameid,amount,refine,bound,unique_id,card0,card1,card2,card3 FROM inventory WHERE char_id=99001313 AND card0=256 ORDER BY id");
    auto before_cost=pet_state();
    sql("CREATE TRIGGER pet_asset_fault BEFORE UPDATE ON inventory FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='lure inventory failure'");
    require(pn_shop_tosql(cost_only,true)==pn_shop::Retry,"cost-only write failure remains retryable");
    require(pet_state()==before_cost,"failed lure debit preserves all inventory pets wallet and receipt");
    sql("DROP TRIGGER pet_asset_fault");
    require(pn_shop_tosql(cost_only,true)==pn_shop::Committed,"cost-only lure debit commits without an output");
    require(result("SELECT amount FROM inventory WHERE char_id=99001313 AND nameid=503")=="19|\n","cost-only commit consumes exactly one lure");
    require(result("SELECT * FROM pet ORDER BY pet_id")==retained_pets && result("SELECT id,nameid,amount,refine,bound,unique_id,card0,card1,card2,card3 FROM inventory WHERE char_id=99001313 AND card0=256 ORDER BY id")==retained_eggs,"cost-only transaction preserves existing pet identities and egg metadata");
    require(result("SELECT refine,bound,unique_id,card0 FROM inventory WHERE id=1")=="10|2|987654321|4001|\n","cost-only transaction preserves unrelated ordinary item metadata");
    auto after_cost=pet_state();
    require(pn_shop_tosql(cost_only,false)==pn_shop::Committed && pet_state()==after_cost,"lost cost-only ACK replay never consumes a second lure");
    sql("UPDATE inventory SET char_id=99001314 WHERE char_id=99001313 AND card0=256");
    newer();auto transferred=pet_state();
    require(pn_shop_tosql(claim,false)==pn_shop::Committed,"original claim receipt resolves after transfer");
    require(pn_shop_tosql(purchase,false)==pn_shop::Committed,"original purchase receipt resolves after transfer");
    require(pet_state()==transferred,"neither replay overwrites transferred eggs or newer balances");
    pet_seed();auto asset=purchase;asset.kind=pn_shop::Asset;asset.stock_count=0;
    asset.cash_after=900;asset.kafra_after=100;asset.items[1].amount=18;
    require(pn_shop_tosql(asset,true)==pn_shop::Committed,"stockless asset grant commits pet outputs with material and currency costs");
    require(result("SELECT amount FROM inventory WHERE char_id=99001313 AND nameid=503")=="18|\n","material debit shares pet commit");
    require(result("SELECT value FROM acc_reg_num WHERE account_id=990013 ORDER BY `key`")=="900|\n100|\n","cash and kafra costs share pet commit");
    std::cout<<"PET_ASSET_COMMIT_PASS "<<checks<<"; real production asset transaction, map planning not certified"<<std::endl;
    return 0;
}
static pn_pet::Result pet_claim(const pn_pet::Key& k,const pn_pet::Entitlement& e){
    require(Sql_BeginTransaction(sql_handle)==SQL_SUCCESS,"begin claim/inventory fixture transaction");
    auto outcome=pn_pet::claim_locked(k.account_id,k.char_id,e.id,e.egg);
    bool ok=outcome==pn_pet::Result::ClaimedNow || outcome==pn_pet::Result::AlreadyClaimed;
    if(outcome==pn_pet::Result::ClaimedNow){
        item inventory[MAX_INVENTORY]{};inventory[0]=e.egg;
        ok=char_memitemdata_to_sql(inventory,MAX_INVENTORY,k.char_id,TABLE_INVENTORY,0)==0;
    }
    if(Sql_EndTransaction(sql_handle,ok)!=SQL_SUCCESS || !ok)return pn_pet::Result::Error;
    return outcome;
}
static int pet_retirement_cases(){
    pet_seed();auto key=pet_key();auto output=pet_output();pn_pet::Entitlement egg{};
    require(pet_paid(key,output,egg)==pn_pet::Result::Issued,"seed pet material");
    require(pet_claim(key,egg)==pn_pet::Result::ClaimedNow,"seed owned material inventory");
    pn_shop::Commit r{};r.length=sizeof(r);r.kind=pn_shop::Asset;
    r.account_id=key.account_id;r.char_id=key.char_id;r.nonce_hi=701;r.nonce_lo=702;r.sequence=1;
    r.wallet_before=900;r.wallet_after=800;r.cash_before=r.cash_after=1000;r.kafra_before=r.kafra_after=200;
    r.counter_before=r.counter_after=5;r.pet_retire_count=1;r.retired_pets[0]={egg.pet_id,egg.egg.nameid};
    r.items[0].nameid=501;r.items[0].amount=1;r.items[0].identify=1;
    require(pn_shop::valid(r),"pet material replacement validates");
    auto invalid=r;invalid.items[1]=egg.egg;
    require(!pn_shop::valid(invalid),"retired egg cannot remain in resulting inventory");
    invalid=r;invalid.pet_retire_count=2;invalid.retired_pets[1]=invalid.retired_pets[0];
    require(!pn_shop::valid(invalid),"pet cannot be consumed twice in one request");
    auto initial=pet_state();
    require(Sql_BeginTransaction(sql_handle)==SQL_SUCCESS,"begin wrong-owner retirement probe");
    require(pn_pet::retire_locked(99001314,egg.pet_id,egg.egg.nameid)==pn_pet::Result::Conflict,"wrong inventory owner cannot retire pet");
    require(Sql_EndTransaction(sql_handle,false)==SQL_SUCCESS,"rollback wrong owner probe");
    require(pet_state()==initial,"wrong owner changes no pet or assets");
    sql("UPDATE pet SET incubate=0,account_id=990013,char_id=99001313 WHERE pet_id="+std::to_string(egg.pet_id));
    auto before=pet_state(false);
    require(pn_shop_tosql(r,true)==pn_shop::Rejected,"hatched pet cannot be consumed as an egg");
    require(pet_state(false)==before,"active pet rejection preserves all assets");
    sql("UPDATE pet SET incubate=1,account_id=0,char_id=0 WHERE pet_id="+std::to_string(egg.pet_id));
    sql("INSERT INTO inventory(char_id,nameid,amount,card0,card1,card2) SELECT 99001314,nameid,amount,card0,card1,card2 FROM inventory WHERE char_id=99001313");
    r.sequence=2;before=pet_state(false);
    require(pn_shop_tosql(r,true)==pn_shop::Rejected,"legacy duplicated pet identity cannot be consumed");
    require(pet_state(false)==before,"duplicate identity rejection preserves both inventories");
    sql("DELETE FROM inventory WHERE char_id=99001314");
    r.sequence=3;initial=pet_state();
    for(const char* target:{"BEFORE DELETE ON pet","BEFORE DELETE ON inventory","BEFORE INSERT ON inventory","BEFORE UPDATE ON `char`","BEFORE INSERT ON pn_shop_commits"}){
        sql(std::string("CREATE TRIGGER pet_retire_fault ")+target+" FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='pet consumption failure'");
        require(pn_shop_tosql(r,true)==pn_shop::Retry,"retirement write failure remains retryable");
        require(pet_state()==initial,"pet, original egg, replacement and payment roll back together");
        sql("DROP TRIGGER pet_retire_fault");
    }
    require(pn_shop_tosql(r,true)==pn_shop::Committed,"owned egg and pet consumed with replacement/payment");
    require(result("SELECT COUNT(*) FROM pet")=="0|\n","consumption does not orphan pet row");
    require(result("SELECT nameid,amount FROM inventory WHERE char_id=99001313")=="501|1|\n","replacement committed instead of consumed egg");
    require(result("SELECT claimed FROM pn_pet_entitlements")=="1|\n","claimed entitlement retained after pet deletion");
    newer();auto later=pet_state();
    require(pn_shop_tosql(r,false)==pn_shop::Committed,"lost consumption acknowledgement resolves after later activity");
    require(pet_state()==later,"consumption replay preserves newer state");
    pn_pet::Entitlement replay{};
    require(pet_paid(key,output,replay)==pn_pet::Result::AlreadyClaimed,"consumed pet's creation cannot replay");
    require(pet_state()==later,"old output replay cannot recreate consumed pet");
    std::cout<<"PET_RETIREMENT_PASS "<<checks<<"; production atomic pet material consumption"<<std::endl;
    return 0;
}

static int pet_cases(const std::string& mode){
    using R=pn_pet::Result;
    auto key=pet_key();auto output=pet_output();pn_pet::Entitlement entitlement{};
    if(mode=="pet-race-a" || mode=="pet-race-b"){
        sql(std::string("INSERT INTO pn_pet_test_barrier(worker) VALUES (")+(mode=="pet-race-a"?"1)":"2)"));
        bool ready=false;
        for(int i=0;i<400;++i){if(result("SELECT COUNT(*) FROM pn_pet_test_barrier")=="2|\n"){ready=true;break;}std::this_thread::sleep_for(std::chrono::milliseconds(25));}
        require(ready,"concurrent pet producers reached barrier");
        auto outcome=R::Error;int retries=0;
        for(;retries<20;++retries){outcome=pet_paid(key,output,entitlement);if(outcome!=R::Error)break;std::this_thread::sleep_for(std::chrono::milliseconds(50));}
        require(outcome==R::Issued || outcome==R::Pending,"concurrent creation resolves to new or existing output");
        std::cout<<"PET_RACE_PASS new="<<(outcome==R::Issued)<<" retries="<<retries<<std::endl;return 0;
    }
    if(mode=="pet-verify-race"){
        require(result("SELECT COUNT(*) FROM pet")=="1|\n","concurrent requests create exactly one pet");
        require(result("SELECT COUNT(*) FROM pn_pet_entitlements")=="1|\n","concurrent requests create exactly one entitlement");
        require(result("SELECT zeny FROM `char` WHERE char_id=99001313")=="900|\n","concurrent retry debits once");
        std::cout<<"PET_CONCURRENCY_PASS "<<checks<<std::endl;return 0;
    }
    if(mode=="pet-verify-rollback"){
        sql("DROP TRIGGER IF EXISTS pet_crash");
        require(result("SELECT COUNT(*) FROM pet")=="0|\n","crashed pet insertion rolled back");
        require(result("SELECT COUNT(*) FROM pn_pet_entitlements")=="0|\n","crashed entitlement insertion rolled back");
        require(result("SELECT zeny FROM `char` WHERE char_id=99001313")=="1000|\n","crashed payment rolled back");
        require(pet_paid(key,output,entitlement)==R::Issued,"original output can retry after database crash");
        require(result("SELECT zeny FROM `char` WHERE char_id=99001313")=="900|\n","post-crash retry charges once");
        std::cout<<"PET_CRASH_RECOVERY_PASS "<<checks<<std::endl;return 0;
    }
    if(mode=="pet-verify-restart"){
        auto before=pet_state();
        require(pet_paid(key,output,entitlement)==R::Pending,"pending output survives database/process restart");
        require(pet_state()==before,"restart replay does not debit again");
        require(pet_claim(key,entitlement)==R::ClaimedNow,"restarted entitlement can be claimed");
        std::cout<<"PET_RESTART_PASS "<<checks<<std::endl;return 0;
    }
    pet_seed();
    if(mode=="pet-seed-race"){
        sql("DROP TABLE IF EXISTS pn_pet_test_barrier");
        sql("CREATE TABLE pn_pet_test_barrier(worker INT PRIMARY KEY) ENGINE=InnoDB");
        std::cout<<"PET_RACE_SEEDED"<<std::endl;return 0;
    }
    if(mode=="pet-crash"){
        sql("CREATE TRIGGER pet_crash BEFORE UPDATE ON `char` FOR EACH ROW DO SLEEP(30)");
        std::cout<<"PET_CRASH_READY"<<std::endl;
        auto outcome=pet_paid(key,output,entitlement);
        require(outcome==R::Error,"database kill never acknowledges pet payment");
        std::cout<<"PET_CRASH_RETURN 0"<<std::endl;return 0;
    }
    if(mode=="pet-seed-restart"){
        require(pet_paid(key,output,entitlement)==R::Issued,"seed durable unclaimed pet");
        std::cout<<"PET_RESTART_SEEDED"<<std::endl;return 0;
    }
    require(mode=="pet-core","known pet test mode");
    auto initial=pet_state();
    require(pn_pet::issue_locked(key,output,entitlement)==R::Error,"creation outside atomic transaction refused");
    require(pet_state()==initial,"missing transaction changes no data");
    require(Sql_BeginTransaction(sql_handle)==SQL_SUCCESS,"begin nesting check");
    sql("UPDATE `char` SET zeny=1 WHERE char_id=99001313");
    require(Sql_BeginTransaction(sql_handle)==SQL_ERROR,"nested begin cannot implicitly commit payment");
    require(Sql_InTransaction(sql_handle),"outer transaction still active");
    require(Sql_EndTransaction(sql_handle,false)==SQL_SUCCESS,"rollback original transaction");
    require(pet_state()==initial,"nested begin did not commit outer work");
    for(const char* table:{"pet","pn_pet_entitlements"}){
        sql(std::string("ALTER TABLE ")+table+" ENGINE=MyISAM");
        require(pet_paid(key,output,entitlement)==R::Error,"nontransactional pet/ledger refused");
        require(pet_state()==initial,"engine refusal leaves payment and output untouched");
        sql(std::string("ALTER TABLE ")+table+" ENGINE=InnoDB");
    }
    for(const std::string& target:{std::string("BEFORE INSERT ON pet"),std::string("BEFORE INSERT ON pn_pet_entitlements"),std::string("BEFORE UPDATE ON `char`")}){
        sql("CREATE TRIGGER pet_fault "+target+" FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='pet fixture failure'");
        require(pet_paid(key,output,entitlement)==R::Error,"pet, ledger or payment write failure surfaced");
        require(pet_state()==initial,"failed joint transaction rolls back pet, ledger and debit");
        sql("DROP TRIGGER pet_fault");
    }
    for(int field=0;field<7;++field){
        auto bad=output;if(field==0)bad.version++;if(field==1)bad.egg.card[0]=0x0100;
        if(field==2)bad.egg.amount=2;if(field==3)bad.intimacy=1001;if(field==4)bad.hungry=-1;
        if(field==5)bad.egg.equip=1;if(field==6)memset(bad.name,'x',sizeof(bad.name));
        require(!pn_pet::valid(key,bad),"malformed output rejected");
        require(pet_paid(key,bad,entitlement)==R::Error,"malformed request cannot issue");
        require(pet_state()==initial,"malformed input changes nothing");
    }
    require(pet_paid(key,output,entitlement)==R::Issued,"pet and debit commit together");
    require(entitlement.egg.bound==2 && entitlement.egg.refine==3 && entitlement.egg.option[0].value==7 && entitlement.egg.unique_id==998877,"egg metadata preserved");
    require(entitlement.egg.card[0]==pn_pet::egg_marker && entitlement.egg.card[3]==6 &&
        (entitlement.egg.card[1]|(entitlement.egg.card[2]<<16))==entitlement.pet_id,"egg encodes exact durable pet identity");
    require(result("SELECT zeny FROM `char` WHERE char_id=99001313")=="900|\n","payment committed exactly once");
    require(result("SELECT COUNT(*) FROM pet")=="1|\n","one pet row");
    auto issued=pet_state();pn_pet::Entitlement replay{};
    require(pet_paid(key,output,replay)==R::Pending && replay.id==entitlement.id && replay.pet_id==entitlement.pet_id,"identical creation replay returns original identity");
    require(pet_state()==issued,"creation replay never charges or creates twice");
    for(int field=0;field<3;++field){
        auto changed=output;auto wrong=key;
        if(field==0)changed.egg.bound++;if(field==1)changed.intimacy++;if(field==2)wrong.char_id++;
        require(pet_paid(wrong,changed,replay)==R::Error,"altered payload or character conflicts with original output");
        require(pet_state()==issued,"conflicting replay leaves original entitlement intact");
    }
    auto wrong=key;wrong.char_id++;
    require(pet_claim(wrong,entitlement)==R::Error,"other character cannot claim");
    require(pet_state()==issued,"wrong character claim changes nothing");
    auto changed=entitlement;changed.egg.refine++;
    require(pet_claim(key,changed)==R::Error,"claim cannot change promised metadata");
    require(pet_state()==issued,"altered claim changes nothing");
    for(const char* target:{"BEFORE UPDATE ON pn_pet_entitlements","BEFORE INSERT ON inventory"}){
        sql(std::string("CREATE TRIGGER pet_fault ")+target+" FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='claim fixture failure'");
        require(pet_claim(key,entitlement)==R::Error,"claim/inventory failure surfaced");
        require(pet_state()==issued,"failed inventory claim leaves entitlement pending");
        sql("DROP TRIGGER pet_fault");
    }
    require(pet_claim(key,entitlement)==R::ClaimedNow,"claim and inventory commit together");
    require(result("SELECT nameid,amount,card0 FROM inventory WHERE char_id=99001313")=="9001|1|256|\n","one real inventory egg persisted");
    sql("UPDATE inventory SET char_id=99001314 WHERE char_id=99001313");
    auto transferred=pet_state();
    require(pet_claim(key,entitlement)==R::AlreadyClaimed,"claim replay after transfer returns terminal state");
    require(pet_paid(key,output,replay)==R::AlreadyClaimed,"create replay after transfer remains terminal");
    require(pet_state()==transferred,"replay cannot restore transferred inventory or charge again");
    key.ordinal=1;output.egg.unique_id++;
    require(pet_paid(key,output,replay)==R::Issued && replay.pet_id!=entitlement.pet_id,"second ordinal has independent pet identity");
    require(result("SELECT COUNT(*) FROM pet")=="2|\n","multi-output identity does not collapse eggs");
    sql("DELETE FROM pet WHERE pet_id="+std::to_string(replay.pet_id));
    auto orphaned=pet_state();
    require(pet_claim(key,replay)==R::Error,"missing pet identity cannot produce a ghost egg");
    require(pet_state()==orphaned,"missing pet does not consume the pending entitlement");
    std::cout<<"PET_ENTITLEMENT_CORE_PASS "<<checks<<"; actual char SQL helpers, fixture payment/claim orchestration; producer integration not certified"<<std::endl;
    return 0;
}
