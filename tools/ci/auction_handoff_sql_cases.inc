#include <fstream>
static int auction_handoff_cases(bool replay=false){
    if(replay){
        std::strcpy(schema_config.auction_db,"auction");std::strcpy(schema_config.mail_db,"mail");std::strcpy(schema_config.mail_attachment_db,"mail_attachments");
        const auto before=state()+result("SELECT * FROM auction")+result("SELECT * FROM mail ORDER BY id")+result("SELECT * FROM mail_attachments ORDER BY id,`index`");
        require(pn_auction_reload(),"fresh process reloads custody");
        std::ifstream file("/evidence/auction-replay.bin",std::ios::binary);
        for(int i=0;i<3;++i){pn_shop::Commit r{};file.read(reinterpret_cast<char*>(&r),sizeof(r));require(file.good(),"read exact replay request");
            require(pn_shop_tosql(r,false)==(i==2?pn_shop::Rejected:pn_shop::Committed),"fresh process resolves exact original receipt");}
        require(before==state()+result("SELECT * FROM auction")+result("SELECT * FROM mail ORDER BY id")+result("SELECT * FROM mail_attachments ORDER BY id,`index`"),"database and process restart replay preserves all newer assets");
        std::cout<<"AUCTION_HANDOFF_RESTART_PASS "<<checks<<std::endl;return 0;
    }
    seed();std::strcpy(schema_config.auction_db,"auction");std::strcpy(schema_config.mail_db,"mail");std::strcpy(schema_config.mail_attachment_db,"mail_attachments");
    sql("DELETE FROM auction");sql("DELETE FROM mail_attachments");sql("DELETE FROM mail");require(pn_auction_reload(),"empty auction cache");
    auto snapshot=[](){return state()+result("SELECT * FROM auction ORDER BY auction_id")+result("SELECT * FROM mail ORDER BY id")+result("SELECT * FROM mail_attachments ORDER BY id,`index`");};
    auto load_items=[](pn_shop::Commit& r){s_storage inv{};require(char_memitemdata_from_sql(&inv,MAX_INVENTORY,r.char_id,TABLE_INVENTORY,0),"load authoritative fixture inventory");memcpy(r.items,inv.u.items_inventory,sizeof(r.items));};
    auto registration=request(pn_shop::AuctionRegister);registration.stock_count=0;registration.counter_after=5;registration.wallet_after=990;
    auto& envelope=*new(&registration.auction) pn_shop::AuctionEnvelope{};envelope.fee=10;
    load_items(registration);
    auction_data listing{};listing.seller_id=registration.char_id;std::strcpy(listing.seller_name,"Shop fixture");std::strcpy(listing.item_name,"Fixture material");
    listing.price=100;listing.buynow=500;listing.hours=1;listing.type=IT_ETC;
    for(auto& it:registration.items)if(it.nameid==503){listing.item=it;listing.item.id=0;listing.item.amount=1;--it.amount;}
    envelope.set_listing(listing);
    require(pn_shop::valid(registration),"registration wire validation");
    auto forged=registration;forged.sequence=90;
    for(auto& it:forged.items)if(it.nameid==503)++it.amount;
    require(pn_shop_tosql(forged,true)==pn_shop::Rejected,"registration cannot retain its listed item in inventory");
    auto departed=registration;departed.sequence=91;
    require(pn_shop_tosql(departed,false)==pn_shop::Rejected,"departed owner cannot initiate fresh custody transfer");
    require(result("SELECT zeny FROM `char` WHERE char_id=99001313")=="1000|\n" && result("SELECT COUNT(*) FROM auction")=="0|\n","rejected registration takes neither item nor fee");
    auto before=snapshot();
    sql("CREATE TRIGGER auction_handoff_fault BEFORE INSERT ON pn_shop_commits FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='receipt failure'");
    require(pn_shop_tosql(registration,true)==pn_shop::Retry,"registration receipt failure is retry");sql("DROP TRIGGER auction_handoff_fault");
    require(snapshot()==before && auction_count(registration.char_id,false)==0,"registration failure preserves fee inventory and cache");
    require(pn_shop_tosql(registration,true)==pn_shop::Committed,"registration commits with debit and receipt");
    require(result("SELECT zeny FROM `char` WHERE char_id=99001313")=="990|\n" && result("SELECT amount FROM inventory WHERE nameid=503")=="19|\n","fee and exactly one item debited");
    require(auction_count(registration.char_id,false)==1 && result("SELECT COUNT(*) FROM auction")=="1|\n","registered listing has SQL and cache custody");
    auto committed=snapshot();require(pn_shop_tosql(registration,false)==pn_shop::Committed && snapshot()==committed,"lost registration ACK replay after departure cannot duplicate or charge twice");
    auto changed=registration;auto changed_listing=changed.auction.listing();changed_listing.price++;changed.auction.set_listing(changed_listing);
    require(pn_shop_tosql(changed,true)==pn_shop::Retry && snapshot()==committed,"altered request cannot reuse registration receipt");
    const uint32 id=std::stoul(result("SELECT auction_id FROM auction"));
    auto bid=request(pn_shop::AuctionBid);bid.account_id=990014;bid.char_id=99001314;bid.wallet_before=777;bid.wallet_after=577;bid.cash_before=bid.cash_after=77;bid.kafra_before=bid.kafra_after=0;
    bid.counter_before=bid.counter_after=9;bid.stock_count=0;auto& b=*new(&bid.auction) pn_shop::AuctionEnvelope{};b.auction_id=id;b.bid=200;load_items(bid);
    require(pn_shop::valid(bid),"bid wire validation");
    before=snapshot();sql("CREATE TRIGGER auction_handoff_fault BEFORE UPDATE ON `char` FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='bid debit failure'");
    require(pn_shop_tosql(bid,true)==pn_shop::Retry,"failed debit is retry");sql("DROP TRIGGER auction_handoff_fault");
    require(snapshot()==before && auction_count(bid.char_id,true)==0,"failed bid cannot change listing or debit wallet");
    require(pn_shop_tosql(bid,true)==pn_shop::Committed,"bid and wallet commit together");
    require(result("SELECT zeny FROM `char` WHERE char_id=99001314")=="577|\n" && result("SELECT buyer_id,price FROM auction")=="99001314|200|\n","bid escrow matches wallet debit");
    committed=snapshot();require(pn_shop_tosql(bid,false)==pn_shop::Committed && snapshot()==committed,"lost bid ACK replay after departure does not repeat payment");
    auto buyout=bid;buyout.sequence=2;buyout.wallet_before=577;buyout.wallet_after=27;buyout.auction.bid=550;
    before=snapshot();sql("CREATE TRIGGER auction_handoff_fault BEFORE INSERT ON pn_shop_commits FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='buyout receipt failure'");
    require(pn_shop_tosql(buyout,true)==pn_shop::Retry,"buyout receipt failure retries");sql("DROP TRIGGER auction_handoff_fault");
    require(snapshot()==before && auction_count(bid.char_id,true)==1,"buyout failure rolls back mails listing and wallet");
    require(pn_shop_tosql(buyout,true)==pn_shop::Committed,"buyout completes atomically");
    require(result("SELECT dest_id,zeny FROM mail ORDER BY dest_id,zeny")=="99001313|500|\n99001314|0|\n99001314|50|\n99001314|200|\n","self outbid and excess refunds preserve full currency");
    require(result("SELECT nameid,amount FROM mail_attachments")=="503|1|\n" && result("SELECT COUNT(*) FROM auction")=="0|\n","buyout transfers item custody once");
    committed=snapshot();require(pn_shop_tosql(buyout,false)==pn_shop::Committed && snapshot()==committed,"buyout receipt replay cannot duplicate mails");
    auto rejected=buyout;rejected.sequence=3;rejected.wallet_before=27;rejected.wallet_after=17;rejected.auction.bid=10;
    require(pn_shop_tosql(rejected,true)==pn_shop::Rejected,"missing listing rejected durably");
    require(result("SELECT zeny FROM `char` WHERE char_id=99001314")=="27|\n","rejected bid takes no payment");
    auto rejected_state=snapshot();require(pn_shop_tosql(rejected,false)==pn_shop::Rejected && snapshot()==rejected_state,"rejection is replayable");
    // A new process reloads only SQL custody; receipts never need the old session.
    inter_auction_sql_final();require(pn_auction_reload(),"restart reload");
    require(pn_shop_tosql(registration,false)==pn_shop::Committed && pn_shop_tosql(buyout,false)==pn_shop::Committed && snapshot()==rejected_state,"restart receipt replay cannot resurrect settled listing");
    std::ofstream replay_file("/evidence/auction-replay.bin",std::ios::binary);
    for(const auto* r:{&registration,&buyout,&rejected})replay_file.write(reinterpret_cast<const char*>(r),sizeof(*r));
    replay_file.close();require(!replay_file.fail(),"write replay requests for fresh process");
    std::cout<<"AUCTION_HANDOFF_SQL_PASS "<<checks<<std::endl;return 0;
}
