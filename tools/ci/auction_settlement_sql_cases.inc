#include <char/int_auction.hpp>
uint32 auction_create(std::shared_ptr<auction_data>);
int32 auction_count(uint32,bool);
TIMER_FUNC(auction_end_timer);
void mapif_parse_Auction_cancel(int32);
void mapif_parse_Auction_close(int32);
void mapif_parse_Auction_bid(int32);
static int auction_settlement_cases(){
    seed();std::strcpy(schema_config.auction_db,"auction");std::strcpy(schema_config.mail_db,"mail");std::strcpy(schema_config.mail_attachment_db,"mail_attachments");
    sql("DELETE FROM auction");sql("ALTER TABLE auction ENGINE=InnoDB");sql("DELETE FROM mail_attachments");sql("DELETE FROM mail");
    auto a=std::make_shared<auction_data>();a->seller_id=99001313;std::strcpy(a->seller_name,"Shop fixture");a->buyer_id=99001314;std::strcpy(a->buyer_name,"Other fixture");
    a->hours=1;a->price=100;a->buynow=500;a->item.nameid=501;a->item.amount=1;a->item.identify=1;a->item.unique_id=887766;a->item.refine=7;a->item.option[0].id=1;a->item.option[0].value=23;
    require(auction_create(a)>0,"auction fixture created by actual implementation");
    const auto before=result("SELECT * FROM auction");
    sql("CREATE TRIGGER auction_fault BEFORE INSERT ON mail FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='auction delivery fault'");
    delete_timer(a->auction_end_timer,auction_end_timer);
    auction_end_timer(a->auction_end_timer,gettick(),a->auction_id,0);
    const bool retained=result("SELECT * FROM auction")==before;
    std::cout<<"AUCTION_SETTLEMENT_FAILURE_PROBE retained="<<retained<<std::endl;
    sql("DROP TRIGGER auction_fault");
    require(retained,"failed winner delivery retains auction and item custody");
    require(result("SELECT COUNT(*) FROM mail")=="0|\n","failed settlement creates no partial payment");
    auto expire=[](const std::shared_ptr<auction_data>& a){
        const auto timer=a->auction_end_timer;
        if(timer!=INVALID_TIMER)delete_timer(timer,auction_end_timer);
        auction_end_timer(timer,gettick(),a->auction_id,0);
    };
    auto snapshot=[](){return result("SELECT * FROM auction ORDER BY auction_id")+result("SELECT * FROM mail ORDER BY id")+result("SELECT * FROM mail_attachments ORDER BY id,`index`");};
    auto clean=[](){sql("DELETE FROM mail_attachments");sql("DELETE FROM mail");};
    auto create=[&](bool buyer){
        auto next=std::make_shared<auction_data>(*a);next->buyer_id=buyer?99001314:0;next->price=100;
        require(auction_create(next)>0,"new actual auction");return next;
    };
    for(const std::string& fault:{"BEFORE INSERT ON mail_attachments FOR EACH ROW", "BEFORE INSERT ON mail FOR EACH ROW IF NEW.zeny>0 THEN", "BEFORE DELETE ON auction FOR EACH ROW"}){
        const auto before=snapshot();
        const bool conditional=fault.find("IF ")!=std::string::npos;
        sql("CREATE TRIGGER auction_fault "+fault+" SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='settlement fault'"+(conditional?"; END IF":""));
        expire(a);sql("DROP TRIGGER auction_fault");
        require(snapshot()==before,"attachment, second mail or deletion fault rolls back entire settlement");
        require(auction_count(a->seller_id,false)==1 && a->auction_end_timer!=INVALID_TIMER,"failed expiry retains cache and schedules retry");
    }
    expire(a);
    require(result("SELECT COUNT(*) FROM auction")=="0|\n" && auction_count(a->seller_id,false)==0,"successful expiry removes SQL and cache together");
    require(result("SELECT dest_id,zeny FROM mail ORDER BY dest_id")=="99001313|100|\n99001314|0|\n","expiry pays seller and delivers winner once");
    require(result("SELECT nameid,amount,unique_id,refine,option_val0 FROM mail_attachments")=="501|1|887766|7|23|\n","settlement preserves exact item metadata");
    auto delivered=snapshot();expire(a);require(snapshot()==delivered,"expired callback replay cannot duplicate assets");
    clean();auto unbid=create(false);expire(unbid);
    require(result("SELECT dest_id,zeny FROM mail")=="99001313|0|\n","unbid expiry returns item to seller without payment");
    clean();auto nontransactional=create(false);sql("ALTER TABLE auction ENGINE=MyISAM");auto held=snapshot();expire(nontransactional);
    require(snapshot()==held,"nontransactional table refuses settlement without asset changes");
    sql("ALTER TABLE auction ENGINE=InnoDB");expire(nontransactional);clean();
    // Drive actual packet parsers through an in-memory FIFO; no network writes.
    socket_data wire{};wire.wdata=static_cast<uint8*>(aCalloc(1024*1024,1));wire.max_wdata=1024*1024;wire.flag.server=1;
    wire.rdata=static_cast<uint8*>(aCalloc(1024,1));wire.max_rdata=1024;wire.rdata_size=1024;
    require(session[1]==nullptr,"unused auction transport slot");session[1]=&wire;
    auto request=[&](const std::shared_ptr<auction_data>& listing,uint32 owner){
        wire.wdata_size=0;std::memcpy(wire.rdata+2,&owner,4);std::memcpy(wire.rdata+6,&listing->auction_id,4);
    };
    for(bool close:{false,true}){
        clean();auto listing=create(close);request(listing,99001314);held=snapshot();
        if(close)mapif_parse_Auction_close(1);else mapif_parse_Auction_cancel(1);
        require(snapshot()==held && wire.wdata[6]!=0,"nonowner cannot cancel or close listing");
        request(listing,listing->seller_id);
        sql("CREATE TRIGGER auction_fault BEFORE DELETE ON auction FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='terminal fault'");
        if(close)mapif_parse_Auction_close(1);else mapif_parse_Auction_cancel(1);
        require(snapshot()==held && wire.wdata[6]!=0,"failed manual settlement preserves assets and rejects success ACK");
        sql("DROP TRIGGER auction_fault");request(listing,listing->seller_id);
        if(close)mapif_parse_Auction_close(1);else mapif_parse_Auction_cancel(1);
        require(wire.wdata[6]==0 && auction_count(listing->seller_id,false)==0,"manual settlement acknowledges only committed removal");
        require(result("SELECT COUNT(*) FROM mail_attachments")=="1|\n","manual settlement delivers one item");
        delivered=snapshot();request(listing,listing->seller_id);
        if(close)mapif_parse_Auction_close(1);else mapif_parse_Auction_cancel(1);
        require(snapshot()==delivered,"manual settlement replay cannot duplicate delivery");
    }
    auto bid=[&](const std::shared_ptr<auction_data>& listing,int amount){
        wire.wdata_size=0;WBUFL(wire.rdata,4)=99001315;WBUFL(wire.rdata,8)=listing->auction_id;WBUFL(wire.rdata,12)=amount;
        std::strcpy(reinterpret_cast<char*>(wire.rdata+16),"New buyer");mapif_parse_Auction_bid(1);
    };
    for(bool buyout:{false,true}){
        clean();auto listing=create(true);held=snapshot();const int amount=buyout?550:200;
        sql(std::string("CREATE TRIGGER auction_fault BEFORE ")+(buyout?"DELETE":"UPDATE")+" ON auction FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='bid fault'");
        bid(listing,amount);sql("DROP TRIGGER auction_fault");
        require(snapshot()==held && listing->buyer_id==99001314 && listing->price==100,"failed bid rolls back refund mail and preserves cache");
        uint32 refund;std::memcpy(&refund,wire.wdata+6,4);
        require(wire.wdata[10]==0 && refund==amount,"known rollback returns full bid in failure ACK");
        bid(listing,amount);std::memcpy(&refund,wire.wdata+6,4);
        require(wire.wdata[10]==1 && refund==0,"committed bid ACK carries no duplicate refund");
        if(buyout){
            require(result("SELECT dest_id,zeny FROM mail ORDER BY dest_id,zeny")=="99001313|500|\n99001314|100|\n99001315|0|\n99001315|50|\n","buyout atomically pays seller, refunds old buyer and returns excess");
            require(result("SELECT COUNT(*) FROM mail_attachments")=="1|\n" && auction_count(listing->seller_id,false)==0,"buyout delivers one item and removes cache");
        }else{
            require(result("SELECT buyer_id,price FROM auction")=="99001315|200|\n" && listing->buyer_id==99001315 && listing->price==200,"bid persists SQL and cache consistently");
            require(result("SELECT dest_id,zeny FROM mail")=="99001314|100|\n","ordinary outbid refunds prior buyer once");expire(listing);
        }
    }
    session[1]=nullptr;aFree(wire.rdata);aFree(wire.wdata);
    std::cout<<"AUCTION_SETTLEMENT_SQL_PASS "<<checks<<std::endl;return 0;
}
