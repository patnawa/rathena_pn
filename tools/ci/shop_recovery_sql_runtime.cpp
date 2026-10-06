// Real SQL and character inventory persistence; current map ownership is the
// sole explicit boundary supplied to the production shop SQL function.
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include "char/char.hpp"
#include "char/inter.hpp"
#include "char/int_achievement.hpp"
#include "char/int_storage.hpp"
#include "common/malloc.hpp"
#include "common/sql.hpp"
#include "common/strlib.hpp"
#include "common/timer.hpp"
#include "custom/shop_commit.hpp"
#include "custom/shop_sql.inc"
#include "custom/pet_entitlement.hpp"
#define mmysql_handle sql_handle
#include "shop-admission.inc"
#undef mmysql_handle

static int checks=0;
static void require(bool ok,const char* label){if(!ok){std::cerr<<"FAIL: "<<label<<std::endl;std::abort();}++checks;}
static void sql(const std::string& text){require(Sql_QueryStr(sql_handle,text.c_str())==SQL_SUCCESS,text.c_str());}
static std::string result(const std::string& query){
    sql(query);std::string value;int code;
    while((code=Sql_NextRow(sql_handle))==SQL_SUCCESS){
        for(unsigned i=0;i<Sql_NumColumns(sql_handle);++i){char* data=nullptr;size_t size=0;
            require(Sql_GetData(sql_handle,i,&data,&size)==SQL_SUCCESS,"read SQL cell");
            value+=data?std::string(data,size):"NULL";value+='|';}
        value+='\n';
    }
    require(code==SQL_NO_DATA,"read all SQL rows");Sql_FreeResult(sql_handle);return value;
}
static void connect_db(){
    sql_handle=Sql_Malloc();
    require(Sql_Connect(sql_handle,"root","shop-fixture-only","shop-recovery-db",3306,"shop_recovery_probe")==SQL_SUCCESS,"connect disposable SQL");
    require(result("SELECT DATABASE()") == "shop_recovery_probe|\n","fixture DB guard");
    std::strcpy(schema_config.inventory_db,"inventory");std::strcpy(schema_config.char_db,"char");
    std::strcpy(schema_config.acc_reg_num_table,"acc_reg_num");
    std::strcpy(schema_config.pet_db,"pet");
    std::strcpy(schema_config.achievement_table,"achievement");
}
static std::string state(bool receipts=true){
    std::string value=result("SELECT char_id,account_id,zeny,uniqueitem_counter FROM `char` ORDER BY char_id")+
        result("SELECT * FROM inventory ORDER BY id")+
        result("SELECT * FROM acc_reg_num ORDER BY account_id,`key`,`index`")+
        result("SELECT * FROM achievement ORDER BY char_id,id")+
        result("SELECT * FROM market ORDER BY name,nameid")+result("SELECT * FROM barter ORDER BY name,`index`")+
        result("SELECT * FROM sales ORDER BY nameid");
    if(receipts)value+=result("SELECT account_id,nonce_hi,nonce_lo,sequence,HEX(payload),outcome FROM pn_shop_commits ORDER BY account_id,sequence")+
        result("SELECT * FROM pn_purchase_history ORDER BY account_id,nonce_hi,nonce_lo,sequence");
    return value;
}
static pn_shop::Commit request(uint32 kind=pn_shop::Market){
    pn_shop::Commit r{};r.length=sizeof(r);r.kind=kind;r.account_id=990013;r.char_id=99001313;
    r.nonce_hi=101;r.nonce_lo=202;r.sequence=1;r.wallet_before=1000;r.wallet_after=kind==pn_shop::Sale?1000:800;
    r.cash_before=1000;r.cash_after=kind==pn_shop::Sale?800:1000;r.kafra_before=r.kafra_after=200;
    r.counter_before=5;r.counter_after=6;r.stock_count=2;
    auto& keep=r.items[0];keep.id=1;keep.nameid=1201;keep.amount=1;keep.identify=1;keep.refine=10;keep.bound=2;keep.unique_id=987654321;keep.card[0]=4001;
    auto& material=r.items[1];material.id=2;material.nameid=503;material.amount=kind==pn_shop::Barter?18:20;material.identify=1;
    for(unsigned i=0;i<2;++i){auto& out=r.items[2+i];out.nameid=501+i;out.amount=1;out.identify=1;
        auto& stock=r.stocks[i];stock.key=kind==pn_shop::Barter?i:501+i;stock.before=5;stock.after=4;
        if(kind==pn_shop::Sale){stock.sale_start=1700000000;stock.sale_end=2000000000;}
        else std::strcpy(stock.name,"fixture-shop");
        if(kind==pn_shop::Market)stock.price=100;
    }
    return r;
}
static void seed(){
    char_get_chardb().clear();auto cached=std::make_shared<mmo_charstatus>();cached->zeny=1000;cached->uniqueitem_counter=5;char_get_chardb()[99001313]=cached;
    sql("DROP TRIGGER IF EXISTS shop_fault");sql("DROP TRIGGER IF EXISTS shop_crash");sql("DROP TRIGGER IF EXISTS shop_progression_fault");
    for(const char* table:{"pn_purchase_history","pn_shop_commits","inventory","acc_reg_num","achievement","market","barter","sales","char"})sql(std::string("DELETE FROM `")+table+"`");
    sql("INSERT INTO `char` (char_id,account_id,name,zeny,uniqueitem_counter) VALUES (99001313,990013,'Shop fixture',1000,5),(99001314,990014,'Other fixture',777,9)");
    sql("INSERT INTO inventory(id,char_id,nameid,amount,identify,refine,bound,unique_id,card0) VALUES (1,99001313,1201,1,1,10,2,987654321,4001),(2,99001313,503,20,1,0,0,0,0)");
    sql("INSERT INTO acc_reg_num(account_id,`key`,`index`,`value`) VALUES (990013,'#CASHPOINTS',0,1000),(990013,'#KAFRAPOINTS',0,200),(990014,'#CASHPOINTS',0,77)");
    sql("INSERT INTO market(name,nameid,price,amount,flag) VALUES ('fixture-shop',501,100,5,0),('fixture-shop',502,100,5,0)");
    sql("INSERT INTO barter(name,`index`,amount) VALUES ('fixture-shop',0,5),('fixture-shop',1,5)");
    sql("INSERT INTO sales(nameid,`start`,`end`,amount) VALUES (501,FROM_UNIXTIME(1700000000),FROM_UNIXTIME(2000000000),5),(502,FROM_UNIXTIME(1700000000),FROM_UNIXTIME(2000000000),5)");
}
static void committed(const pn_shop::Commit& r){
    auto cached=char_get_chardb().find(r.char_id);
    require(cached!=char_get_chardb().end() && cached->second->zeny==r.wallet_after && cached->second->uniqueitem_counter==6,"new commit updates existing character cache");
    require(result("SELECT zeny,uniqueitem_counter FROM `char` WHERE char_id=99001313")==std::to_string(r.wallet_after)+"|6|\n","wallet/counter committed");
    require(result("SELECT nameid,amount FROM inventory WHERE nameid IN (501,502) ORDER BY nameid")=="501|1|\n502|1|\n","outputs committed once");
    require(result("SELECT refine,bound,unique_id,card0 FROM inventory WHERE id=1")=="10|2|987654321|4001|\n","unrelated item metadata preserved");
    require(result("SELECT zeny FROM `char` WHERE char_id=99001314")=="777|\n","other character unchanged");
    require(result("SELECT amount FROM inventory WHERE nameid=503")==std::string(r.kind==pn_shop::Barter?"18|\n":"20|\n"),"materials committed");
    require(result("SELECT value FROM acc_reg_num WHERE account_id=990013 AND `key`='#CASHPOINTS'")==std::to_string(r.cash_after)+"|\n","cash committed");
    std::string q=r.kind==pn_shop::Market?"SELECT amount FROM market ORDER BY nameid":r.kind==pn_shop::Barter?"SELECT amount FROM barter ORDER BY `index`":"SELECT amount FROM sales ORDER BY nameid";
    require(result(q)=="4|\n4|\n","both stock rows committed");
    require(result("SELECT COUNT(*) FROM pn_shop_commits WHERE outcome=1")=="1|\n","one committed receipt");
    require(result("SELECT account_id,char_id,kind,outcome FROM pn_purchase_history")=="990013|99001313|"+std::to_string(r.kind)+"|1|\n","history is scoped to both owners and committed once");
    const auto encoded=result("SELECT details FROM pn_purchase_history");
    const auto history=nlohmann::json::parse(encoded.substr(0,encoded.size()-2));
    require(history.at("zeny").get<int64>()==r.wallet_after-r.wallet_before && history.at("cash").get<int64>()==r.cash_after-r.cash_before,"history contains actual committed currency deltas");
    std::map<int,int> expected={{501,1},{502,1}};if(r.kind==pn_shop::Barter)expected[503]=-2;
    std::map<int,int> actual;for(const auto& it:history.at("items"))actual[it[0].get<int>()]=it[1].get<int>();
    require(actual==expected,"history separates output/material net changes from unchanged inventory");
}
static void newer(){
    sql("UPDATE `char` SET zeny=333,uniqueitem_counter=99 WHERE char_id=99001313");
    sql("UPDATE inventory SET amount=7 WHERE nameid=501");
    sql("UPDATE market SET amount=2 WHERE nameid=501");
    sql("UPDATE acc_reg_num SET value=123 WHERE account_id=990013 AND `key`='#CASHPOINTS'");
}
#include "pet_entitlement_sql_cases.inc"
#include "pet_mail_sql_cases.inc"
#include "mail_lifecycle_sql_cases.inc"
#include "pet_floor_sql_cases.inc"
#include "point_asset_sql_cases.inc"
#include "point_global_sql_cases.inc"
#include "shop_progression_sql_cases.inc"

extern "C" int __wrap_main(int argc,char** argv){
    malloc_init();timer_init();connect_db();const std::string mode=argc>1?argv[1]:"normal";
    auto r=request();
    if(mode.rfind("lifecycle",0)==0)return mail_lifecycle_cases(mode);
    if(mode.rfind("global-",0)==0)return global_point_cases(mode);
    if(mode.rfind("floor-pet",0)==0)return pet_floor_sql_cases(mode);
    if(mode=="point-assets")return point_asset_cases();
    if(mode=="pet-assets")return pet_asset_cases();
    if(mode=="pet-mail")return pet_mail_cases();
    if(mode=="pet-retirement")return pet_retirement_cases();
    if(mode.rfind("pet-",0)==0)return pet_cases(mode);
    if(mode=="progression")return progression_sql_cases();
    if(mode=="queued") {
        for(auto kind:{pn_shop::Market,pn_shop::Barter,pn_shop::Sale}) {
            seed();std::vector<pn_shop::Commit> queued;
            for(unsigned i=0;i<6;++i) {
                auto offer=request(kind);offer.account_id=1000+i;offer.char_id=2000+i;offer.items[0]={};offer.items[1]={};
                sql("INSERT INTO `char`(account_id,char_id,name,zeny,uniqueitem_counter) VALUES("+std::to_string(offer.account_id)+","+
                    std::to_string(offer.char_id)+",'Queued "+std::to_string(i)+"',1000,5)");
                sql("INSERT INTO acc_reg_num(account_id,`key`,`index`,value) VALUES("+std::to_string(offer.account_id)+",'#CASHPOINTS',0,1000),("+
                    std::to_string(offer.account_id)+",'#KAFRAPOINTS',0,200)");
                queued.push_back(offer); // Every buyer saw the same initial five units.
            }
            for(unsigned i=0;i<queued.size();++i) {
                auto& offer=queued[i];require(pn_shop_stock_rebase(offer),"queued admission reads authoritative stock");
                if(i<5)require(offer.stocks[0].before==5-i && offer.stocks[0].after==4-i,"rebased quantity preserves requested debit");
                const auto outcome=i<5?pn_shop::Committed:pn_shop::Rejected;
                require(pn_shop_tosql(offer,true)==outcome,"queued stock commits until exhausted then rejects without payment");
                const auto settled=state();require(pn_shop_tosql(offer,true)==outcome,"queued immutable receipt replay");
                require(state()==settled,"queued retry cannot repeat payment or delivery");
                require(result("SELECT zeny FROM `char` WHERE char_id="+std::to_string(offer.char_id))==std::to_string(i<5?offer.wallet_after:offer.wallet_before)+"|\n","queue charges only successful buyer");
            }
            const std::string table=kind==pn_shop::Market?"market":kind==pn_shop::Barter?"barter":"sales";
            require(result("SELECT SUM(amount) FROM "+table)=="0|\n","all five stock units consumed once");
            require(result("SELECT SUM(amount) FROM inventory WHERE nameid=501")=="5|\n","five total deliveries");
        }
        std::cout<<"SHOP_QUEUED_SQL_PASS "<<checks<<" checks; all three stock kinds, five buyers, exhaustion and immutable receipts"<<std::endl;
        return 0;
    }
    if(mode=="race-a" || mode=="race-b"){
        const bool second=mode=="race-b";
        r.stock_count=1;r.stocks[0].before=1;r.stocks[0].after=0;r.wallet_after=900;r.items[3]={};
        if(second){r.account_id=990014;r.char_id=99001314;r.items[0].id=11;r.items[1].id=12;}
        sql(std::string("INSERT INTO pn_shop_test_barrier(worker) VALUES (")+(second?"2)":"1)"));
        bool ready=false;
        for(int i=0;i<400;++i){if(result("SELECT COUNT(*) FROM pn_shop_test_barrier")=="2|\n"){ready=true;break;}std::this_thread::sleep_for(std::chrono::milliseconds(25));}
        require(ready,"both independent buyer connections reached barrier");
        uint32 outcome=pn_shop::Retry;int retries=0;
        for(;retries<20;++retries){outcome=pn_shop_tosql(r,true);if(outcome!=pn_shop::Retry)break;std::this_thread::sleep_for(std::chrono::milliseconds(50));}
        require(outcome==pn_shop::Committed || outcome==pn_shop::Rejected,"concurrent buyer resolves to durable outcome");
        std::cout<<"SHOP_RACE_OUTCOME account="<<r.account_id<<" outcome="<<outcome<<" retries="<<retries<<std::endl;return 0;
    }
    if(mode=="verify-race"){
        sql("DROP TRIGGER IF EXISTS shop_race_delay");
        require(result("SELECT outcome,COUNT(*) FROM pn_shop_commits GROUP BY outcome ORDER BY outcome")=="1|1|\n2|1|\n","exactly one concurrent winner and one durable loser");
        require(result("SELECT amount FROM market WHERE nameid=501")=="0|\n","final stock unit neither underflows nor reappears");
        require(result("SELECT amount FROM market WHERE nameid=502")=="5|\n","unrelated stock remains untouched");
        require(result("SELECT SUM(amount) FROM inventory WHERE nameid=501")=="1|\n","one total reward across both buyers");
        for(uint32 account:{990013u,990014u}){
            auto outcome=result("SELECT outcome FROM pn_shop_commits WHERE account_id="+std::to_string(account));
            bool winner=outcome=="1|\n";require(winner || outcome=="2|\n","buyer has terminal receipt");
            uint32 cid=account==990013?99001313:99001314;
            require(result("SELECT zeny,uniqueitem_counter FROM `char` WHERE char_id="+std::to_string(cid))==std::string(winner?"900|6|\n":"1000|5|\n"),"only winner pays and advances item counter");
            require(result("SELECT COALESCE(SUM(amount),0) FROM inventory WHERE nameid=501 AND char_id="+std::to_string(cid))==std::string(winner?"1|\n":"0|\n"),"loser receives no grant");
            require(result("SELECT nameid,amount FROM inventory WHERE nameid IN (503,1201) AND char_id="+std::to_string(cid)+" ORDER BY nameid")=="503|20|\n1201|1|\n","both buyers keep original items");
            auto before=state();auto retry=request();retry.stock_count=1;retry.stocks[0].before=1;retry.stocks[0].after=0;retry.wallet_after=900;retry.items[3]={};
            if(account==990014){retry.account_id=account;retry.char_id=cid;retry.items[0].id=11;retry.items[1].id=12;}
            require(pn_shop_tosql(retry,false)==(winner?pn_shop::Committed:pn_shop::Rejected),"both concurrent receipts replay immutably");require(state()==before,"concurrent outcome retry changes nothing");
        }
        std::cout<<"SHOP_CONCURRENCY_PASS "<<checks<<std::endl;return 0;
    }
    if(mode=="verify-restart"){
        auto cached=std::make_shared<mmo_charstatus>();cached->zeny=777;char_get_chardb()[r.char_id]=cached;
        auto before=state();require(pn_shop_tosql(r,false)==pn_shop::Committed,"durable receipt across process/DB restart");
        require(!char_get_chardb().count(r.char_id),"restart receipt evicts stale character cache");
        require(state()==before,"replay cannot overwrite newer state after restart");
        require(result("SELECT zeny,uniqueitem_counter FROM `char` WHERE char_id=99001313")=="333|99|\n","newer wallet survived restart");
        require(result("SELECT amount FROM inventory WHERE nameid=501")=="7|\n","newer inventory survived restart");
        std::cout<<"SHOP_RESTART_PASS "<<checks<<std::endl;return 0;
    }
    if(mode=="verify-rollback"){
        auto cached=std::make_shared<mmo_charstatus>();cached->zeny=1000;cached->uniqueitem_counter=5;char_get_chardb()[r.char_id]=cached;
        sql("DROP TRIGGER IF EXISTS shop_crash");
        require(result("SELECT zeny,uniqueitem_counter FROM `char` WHERE char_id=99001313")=="1000|5|\n","uncommitted wallet rolled back on crash");
        require(result("SELECT nameid,amount FROM inventory ORDER BY nameid")=="503|20|\n1201|1|\n","uncommitted inventory rolled back on crash");
        require(result("SELECT amount FROM market ORDER BY nameid")=="5|\n5|\n","both stock writes rolled back on crash");
        require(result("SELECT COUNT(*) FROM pn_shop_commits")=="0|\n","no uncommitted receipt after crash");
        require(pn_shop_tosql(r,true)==pn_shop::Committed,"retry after crash commits once");committed(r);
        std::cout<<"SHOP_CRASH_RECOVERY_PASS "<<checks<<std::endl;return 0;
    }
    seed();
    if(mode=="wire"){
        // Exercise the actual char handler, FIFO macros and acknowledgement
        // writer. Transport delivery is in-memory; no game socket is opened.
        constexpr int fd=42;require(session[fd]==nullptr,"unused fixture FIFO slot");
        std::vector<uint8> input(sizeof(pn_shop::Commit)),output(FIFOSIZE_SERVERLINK);
        socket_data fifo{};fifo.flag.server=1;fifo.rdata=input.data();fifo.wdata=output.data();
        fifo.max_rdata=input.size();fifo.max_wdata=output.size();session[fd]=&fifo;
        auto dispatch=[&](const pn_shop::Commit& sent){
            std::memcpy(input.data(),&sent,sizeof(sent));fifo.rdata_size=sizeof(sent);fifo.rdata_pos=0;fifo.wdata_size=0;
            mapif_parse_ShopCommit(fd);pn_shop::Ack ack{};
            if(fifo.wdata_size){require(fifo.wdata_size==sizeof(ack),"exact acknowledgement wire length");std::memcpy(&ack,output.data(),sizeof(ack));
                require(ack.packet==0x3898 && ack.account_id==sent.account_id && ack.char_id==sent.char_id && ack.nonce_hi==sent.nonce_hi && ack.nonce_lo==sent.nonce_lo && ack.sequence==sent.sequence,"acknowledgement identity survives wire");}
            return ack;
        };
        for(int invalid=0;invalid<5;++invalid){
            seed();char_get_onlinedb().clear();map_server[0].fd=fd;
            if(invalid){auto online=std::make_shared<online_char_data>(r.account_id);online->char_id=r.char_id;online->server=0;
                if(invalid==1)online->server=-1;if(invalid==2)online->server=MAX_MAP_SERVERS;
                if(invalid==3)online->char_id++;if(invalid==4)map_server[0].fd=fd+1;
                char_get_onlinedb()[r.account_id]=online;}
            auto before=state(false);auto ack=dispatch(r);
            require(ack.outcome==pn_shop::Rejected,"wire handler rejects nonowner session");require(state(false)==before,"nonowner wire request leaves player/stock intact");
            require(result("SELECT outcome FROM pn_shop_commits")=="2|\n","wire rejection receipt persisted");
        }
        seed();char_get_onlinedb().clear();auto online=std::make_shared<online_char_data>(r.account_id);online->char_id=r.char_id;online->server=0;
        char_get_onlinedb()[r.account_id]=online;map_server[0].fd=fd;
        for(int invalid=0;invalid<6;++invalid){auto bad=r;
            if(invalid==0)bad.length--;if(invalid==1)bad.length++;if(invalid==2)bad.packet++;
            if(invalid==3)bad.nonce_hi=bad.nonce_lo=0;if(invalid==4)bad.stock_count=0;if(invalid==5)bad.stock_count=pn_shop::stock_capacity+1;
            auto before=state();auto ack=dispatch(bad);
            if(invalid<2)require(fifo.wdata_size==0,"wrong declared frame length produces no ACK");
            else require(ack.outcome==pn_shop::Retry,"invalid payload cannot produce success ACK");
            require(state()==before,"malformed wire input changes no durable state");
        }
        auto ack=dispatch(r);require(ack.outcome==pn_shop::Committed,"authenticated owning map wire request commits");committed(r);
        newer();auto before=state();char_get_onlinedb().clear();ack=dispatch(r);
        require(ack.outcome==pn_shop::Committed,"departed owner's exact wire receipt can resolve");require(state()==before,"wire receipt retry preserves later state");
        session[fd]=nullptr;char_get_onlinedb().clear();map_server[0].fd=-1;
        std::cout<<"SHOP_WIRE_PASS "<<checks<<" (real char handler/FIFO/SQL; no TCP or outer dispatcher)"<<std::endl;return 0;
    }
    if(mode=="seed-race"){
        sql("DROP TRIGGER IF EXISTS shop_race_delay");sql("DROP TABLE IF EXISTS pn_shop_test_barrier");
        sql("CREATE TABLE pn_shop_test_barrier(worker INT PRIMARY KEY) ENGINE=InnoDB");
        sql("UPDATE market SET amount=1 WHERE nameid=501");
        sql("UPDATE `char` SET zeny=1000,uniqueitem_counter=5 WHERE char_id=99001314");
        sql("REPLACE INTO acc_reg_num(account_id,`key`,`index`,value) VALUES (990014,'#CASHPOINTS',0,1000),(990014,'#KAFRAPOINTS',0,200)");
        sql("INSERT INTO inventory(id,char_id,nameid,amount,identify,refine,bound,unique_id,card0) VALUES (11,99001314,1201,1,1,10,2,987654321,4001),(12,99001314,503,20,1,0,0,0,0)");
        sql("CREATE TRIGGER shop_race_delay BEFORE UPDATE ON market FOR EACH ROW DO SLEEP(1)");
        std::cout<<"SHOP_RACE_SEEDED"<<std::endl;return 0;
    }
    if(mode=="crash"){
        sql("CREATE TRIGGER shop_crash BEFORE INSERT ON pn_shop_commits FOR EACH ROW DO SLEEP(30)");
        std::cout<<"CRASH_READY"<<std::endl;auto outcome=pn_shop_tosql(r,true);
        std::cout<<"CRASH_RETURN "<<(outcome==pn_shop::Committed)<<std::endl;
        require(outcome!=pn_shop::Committed,"crashed transaction not acknowledged committed");return 0;
    }
    if(mode=="seed-restart"){
        require(pn_shop_tosql(r,true)==pn_shop::Committed,"seed durable receipt");committed(r);newer();
        std::cout<<"SHOP_RESTART_SEEDED"<<std::endl;return 0;
    }
    require(mode=="normal","known mode");
    for(uint32 kind:{pn_shop::Market,pn_shop::Barter,pn_shop::Sale}){
        r=request(kind);seed();auto before=state();
        const std::string table=kind==pn_shop::Market?"market":kind==pn_shop::Barter?"barter":"sales";
        for(const std::string& target:std::vector<std::string>{"char","inventory","acc_reg_num",table,"pn_shop_commits","pn_purchase_history"}){
            sql("ALTER TABLE `"+target+"` ENGINE=MyISAM");
            require(pn_shop_tosql(r,true)!=pn_shop::Committed,"nontransactional table rejected");
            require(state()==before,"engine rejection changes no state");sql("ALTER TABLE `"+target+"` ENGINE=InnoDB");
        }
        const std::string key=kind==pn_shop::Barter?"`index`":"nameid";
        std::vector<std::string> faults={
            "BEFORE UPDATE ON "+table+" FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='first stock fault'",
            "BEFORE INSERT ON inventory FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='inventory fault'",
            "BEFORE UPDATE ON `char` FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='wallet fault'",
            "BEFORE UPDATE ON "+table+" FOR EACH ROW BEGIN IF NEW."+key+"="+(kind==pn_shop::Barter?"1":"502")+" THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='second stock fault'; END IF; END",
            "BEFORE INSERT ON pn_shop_commits FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='receipt fault'"};
        faults.push_back("BEFORE INSERT ON acc_reg_num FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='cash fault'");
        faults.push_back("BEFORE INSERT ON pn_purchase_history FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='history fault'");
        for(const auto& fault:faults){
            sql("CREATE TRIGGER shop_fault "+fault);
            require(pn_shop_tosql(r,true)!=pn_shop::Committed,"injected write failure not committed");
            require(state()==before,"first/middle/final write rolls back every player and stock row");
            require(char_get_chardb().at(r.char_id)->zeny==1000,"failed write cannot advance character cache");
            sql("DROP TRIGGER shop_fault");
        }
        auto owner_before=state(false);
        require(pn_shop_tosql(r,false)==pn_shop::Rejected,"new request from wrong map owner rejected");
        require(state(false)==owner_before,"wrong owner leaves player and stock untouched");
        require(result("SELECT outcome FROM pn_shop_commits")=="2|\n","wrong owner rejection is durable");seed();
        require(pn_shop_tosql(r,true)==pn_shop::Committed,"successful multirow purchase");committed(r);
        require(std::stoul(result("SELECT OCTET_LENGTH(payload) FROM pn_shop_commits"))<sizeof(r)/10,"real receipt stored compactly");
        auto replace_receipt=[&](const void* data,size_t size){
            SqlStmt stmt{*sql_handle};
            require(stmt.Prepare("UPDATE pn_shop_commits SET payload=?")==SQL_SUCCESS &&
                stmt.BindParam(0,SQLDT_BLOB,const_cast<void*>(data),size)==SQL_SUCCESS && stmt.Execute()==SQL_SUCCESS,"install fixture receipt encoding");
        };
        replace_receipt(&r,sizeof(r));auto legacy=state();
        require(pn_shop_tosql(r,false)==pn_shop::Committed,"historical raw receipt accepted after owner change");
        require(state()==legacy,"raw replay leaves every asset and receipt unchanged");
        auto encoded=pn_shop_receipt_codec::encode(&r,sizeof(r));
        for(int damage=0;damage<4;++damage){
            auto broken=encoded;
            if(damage==0)broken[4]=2;
            if(damage==1)broken.pop_back();
            if(damage==2)broken.push_back(0);
            if(damage==3)broken.back()^=1;
            replace_receipt(broken.data(),broken.size());auto corrupt=state();
            require(pn_shop_tosql(r,true)==pn_shop::Retry,"corrupt or unsupported receipt cannot acknowledge or reapply");
            require(state()==corrupt,"corrupt receipt failure leaves all SQL state unchanged");
        }
        replace_receipt(encoded.data(),encoded.size());
        newer();auto after=state();
        require(pn_shop_tosql(r,true)==pn_shop::Committed,"lost ACK retries return receipt");require(state()==after,"retry does not replay stale snapshots");
        require(!char_get_chardb().count(r.char_id),"receipt replay evicts old character cache");
        Sql_Free(sql_handle);connect_db();require(pn_shop_tosql(r,false)==pn_shop::Committed,"receipt survives new connection and changed map owner");require(state()==after,"new connection retry leaves newer state untouched");
        for(int field=0;field<4;++field){auto changed=r;if(field==0)changed.items[2].amount++;if(field==1)changed.wallet_after--;if(field==2)changed.stocks[0].after--;if(field==3)changed.char_id++;
            require(pn_shop_tosql(changed,true)!=pn_shop::Committed,"changed payload rejected under same identity");require(state()==after,"changed payload cannot mutate state");}
        seed();sql("UPDATE "+table+" SET amount=3 WHERE "+key+"="+(kind==pn_shop::Barter?"1":"502"));before=state(false);
        require(pn_shop_tosql(r,true)==pn_shop::Rejected,"stale durable stock rejected");require(state(false)==before,"stale second stock leaves first stock and player intact");
        require(result("SELECT outcome FROM pn_shop_commits")=="2|\n","stale stock rejection is durable");
        sql("UPDATE "+table+" SET amount=5 WHERE "+key+"="+(kind==pn_shop::Barter?"1":"502"));before=state();
        require(pn_shop_tosql(r,true)==pn_shop::Rejected,"rejected identity cannot purchase later restored stock");require(state()==before,"rejected receipt replay unchanged");
    }
    seed();r=request(pn_shop::Sale);r.stock_count=1;r.stocks[0].before=1;r.stocks[0].after=0;
    sql("UPDATE sales SET amount=1 WHERE nameid=501");
    require(pn_shop_tosql(r,true)==pn_shop::Committed,"sale final unit commits");
    require(result("SELECT amount FROM sales WHERE nameid=501")=="0|\n","exhausted sale retained as zero stock");
    auto final=state();require(pn_shop_tosql(r,false)==pn_shop::Committed,"exhausted sale retry acknowledged");require(state()==final,"exhausted sale retry cannot restore stock");
    std::cout<<"SHOP_SQL_PASS "<<checks<<" checks; real InnoDB rollback, receipts, stale stock, metadata, multirow purchase, sale exhaustion"<<std::endl;
    Sql_Free(sql_handle);sql_handle=nullptr;return 0;
}
