// Real character asset transaction: point debit and pet output share COMMIT.
static int point_asset_cases(){
    std::strcpy(schema_config.char_reg_num_table,"char_reg_num");
    for(auto scope:{pn_shop::CharacterPoint,pn_shop::AccountPoint,pn_shop::SessionPoint}) {
        pet_seed();sql("DELETE FROM char_reg_num");sql("DELETE FROM pn_point_registry_keys");
        const char* key=scope==pn_shop::CharacterPoint?"Points":scope==pn_shop::AccountPoint?"#Points":"@Points";
        const std::string table=scope==pn_shop::CharacterPoint?"char_reg_num":"acc_reg_num";
        const std::string owner=scope==pn_shop::CharacterPoint?"char_id=99001313":"account_id=990013";
        if(scope!=pn_shop::SessionPoint)sql("INSERT INTO "+table+"("+(scope==pn_shop::CharacterPoint?std::string("char_id"):std::string("account_id"))+",`key`,`index`,value) VALUES("+(scope==pn_shop::CharacterPoint?std::string("99001313"):std::string("990013"))+",'"+key+"',0,5000000000)");
        auto r=request(pn_shop::Asset);r.stock_count=0;for(auto& stock:r.stocks)stock={};
        r.wallet_after=r.wallet_before;r.cash_after=r.cash_before;r.kafra_after=r.kafra_before;
        r.point.scope=scope;std::strcpy(r.point.key,key);r.point.before=5000000000LL;r.point.after=r.point.before-100;
        r.pet_count=1;r.counter_after=r.counter_before+1;r.pets[0].output=pet_output();
        auto snapshot=[&](bool receipts=true){return pet_state(receipts)+result("SELECT * FROM char_reg_num ORDER BY char_id,`key`,`index`")+result("SELECT * FROM pn_point_registry_keys ORDER BY account_id,char_id,point_key");};
        require(pn_shop::valid(r),"point-funded request validates");auto initial=snapshot();
        if(scope==pn_shop::CharacterPoint){sql("ALTER TABLE char_reg_num ENGINE=MyISAM");require(pn_shop_tosql(r,true)==pn_shop::Retry,"nontransactional character points refused");require(snapshot()==initial,"engine guard preserves assets");sql("ALTER TABLE char_reg_num ENGINE=InnoDB");}
        std::vector<std::string> faults={"BEFORE INSERT ON pet","BEFORE INSERT ON pn_pet_entitlements","BEFORE INSERT ON inventory","BEFORE UPDATE ON `char`","BEFORE INSERT ON pn_shop_commits"};
        if(scope!=pn_shop::SessionPoint){faults.push_back("BEFORE UPDATE ON "+table);faults.push_back("BEFORE INSERT ON pn_point_registry_keys");}
        for(const auto& fault:faults){
            sql("CREATE TRIGGER point_asset_fault "+fault+" FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='point asset fault'");
            require(pn_shop_tosql(r,true)==pn_shop::Retry,"point/payment/receipt write failure remains retryable");
            require(snapshot()==initial,"point balance, pets, inventory and receipt roll back together");
            sql("DROP TRIGGER point_asset_fault");
        }
        if(scope!=pn_shop::SessionPoint){
            auto stale=r;stale.sequence=901;stale.point.before++;stale.point.after++;
            auto before=snapshot(false);require(pn_shop_tosql(stale,true)==pn_shop::Rejected,"stale persistent balance rejects");require(snapshot(false)==before,"CAS rejection preserves assets");
        }
        require(pn_shop_tosql(r,true)==pn_shop::Committed,"point purchase commits once");
        if(scope!=pn_shop::SessionPoint)require(result("SELECT value FROM "+table+" WHERE "+owner+" AND `key`='"+key+"'")=="4999999900|\n","full 64-bit point balance debited exactly");
        require(pn_global_point::protected_key(sql_handle,r.account_id,r.char_id,key)==(scope!=pn_shop::SessionPoint),"only persistent committed point keys retain owner fence");
        require(result("SELECT COUNT(*) FROM pn_pet_entitlements")=="1|\n","one recoverable pet output");
        newer();auto after=snapshot();require(pn_shop_tosql(r,false)==pn_shop::Committed,"departed owner resolves original point receipt");require(snapshot()==after,"retry cannot restore old state or debit twice");
        auto changed=r;changed.point.after--;require(pn_shop_tosql(changed,true)==pn_shop::Retry,"different debit under same identity rejected");require(snapshot()==after,"altered replay changes nothing");
    }
    std::cout<<"POINT_ASSET_SQL_PASS "<<checks<<"; character/account/session point asset receipts and atomicity"<<std::endl;return 0;
}
