// Actual character-server lifecycle functions against the disposable SQL fixture.
void mapif_Mail_return(int32,uint32,int32,uint32,uint32);
bool mapif_Mail_delete(int32,uint32,int32,uint32);
int32 mail_timer_sub(int32,mail_inbox_type);
static int mail_lifecycle_cases(const std::string& mode){
    std::strcpy(schema_config.mail_db,"mail");std::strcpy(schema_config.mail_attachment_db,"mail_attachments");
    auto reset=[](){
        pet_seed();std::strcpy(schema_config.mail_db,"mail");std::strcpy(schema_config.mail_attachment_db,"mail_attachments");
        sql("DELETE FROM mail_attachments");sql("DELETE FROM mail");
        sql("INSERT INTO mail(id,send_id,send_name,dest_id,dest_name,title,message,time,status,zeny,type) VALUES(801,99001313,'Shop fixture',99001314,'Other fixture','Return test','Body',1,2,75,0)");
        sql("INSERT INTO mail_attachments(id,`index`,nameid,amount,identify,unique_id,refine,option_id0,option_val0) VALUES(801,0,501,3,1,998877,7,1,23)");
        charserv_config.mail_return_empty=1;
    };
    auto snapshot=[](){return result("SELECT * FROM mail ORDER BY id")+result("SELECT * FROM mail_attachments ORDER BY id,`index`");};
    if(mode=="lifecycle-race-seed"){reset();return 0;}
    if(mode=="lifecycle-race-return"){mapif_Mail_return(0,99001314,801,990014,990013);return 0;}
    if(mode=="lifecycle-race-verify"){
        require(result("SELECT dest_id,zeny,type FROM mail")=="99001313|75|2|\n","concurrent returns produce exactly one returned header");
        require(result("SELECT nameid,amount,unique_id FROM mail_attachments")=="501|3|998877|\n","concurrent returns preserve exactly one attachment");
        std::cout<<"MAIL_RETURN_RACE_PASS"<<std::endl;return 0;
    }
    if(mode=="lifecycle" || mode=="lifecycle-return"){
        reset();auto before=snapshot();
        sql("CREATE TRIGGER lifecycle_fault BEFORE INSERT ON mail FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='return persistence fault'");
        mapif_Mail_return(0,99001314,801,990014,990013);
        const bool preserved=snapshot()==before;
        std::cout<<"MAIL_RETURN_FAILURE_PROBE original_preserved="<<preserved<<std::endl;
        sql("DROP TRIGGER lifecycle_fault");
        require(preserved,"failed return must preserve original mail and all assets");
    }
    if(mode=="lifecycle" || mode=="lifecycle-owner"){
        reset();auto before=snapshot();
        const bool removed=mapif_Mail_delete(0,99001313,801,990013);
        std::cout<<"MAIL_DELETE_OWNER_PROBE removed="<<removed<<" preserved="<<(snapshot()==before)<<std::endl;
        require(!removed && snapshot()==before,"wrong character cannot delete another inbox's mail");
    }
    if(mode=="lifecycle" || mode=="lifecycle-disabled"){
        reset();sql("UPDATE mail SET send_id=0 WHERE id=801");auto before=snapshot();
        mail_timer_sub(-1,MAIL_INBOX_NORMAL);
        std::cout<<"MAIL_NEGATIVE_EXPIRY_PROBE preserved="<<(snapshot()==before)<<std::endl;
        require(snapshot()==before,"nonpositive expiry must not delete system assets");
    }
    if(mode=="lifecycle"){
        for(const std::string& fault:{"BEFORE INSERT ON mail_attachments","BEFORE DELETE ON mail"}){
            reset();auto before=snapshot();
            sql("CREATE TRIGGER lifecycle_fault "+fault+" FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='lifecycle rollback fault'");
            mapif_Mail_return(0,99001314,801,990014,990013);
            require(snapshot()==before,"attachment or original-header deletion failure rolls back entire return");
            sql("DROP TRIGGER lifecycle_fault");
        }
        reset();sql("UPDATE mail SET title=REPEAT('T',39) WHERE id=801");
        sql("INSERT INTO pet(pet_id,class,name,egg_id,incubate) VALUES(77,1002,'Returned egg',9001,1)");
        sql("INSERT INTO mail_attachments(id,`index`,nameid,amount,identify,card0,card1,unique_id) VALUES(801,1,9001,1,1,256,77,12345)");
        const auto pets=result("SELECT * FROM pet");
        mapif_Mail_return(0,99001314,801,990014,990013);
        require(result("SELECT dest_id,send_id,zeny,type,message,LENGTH(title) FROM mail")=="99001313|99001314|75|2|Body|39|\n","return swaps owner and preserves body with maximum title");
        require(result("SELECT nameid,amount,unique_id,refine,option_val0,card1 FROM mail_attachments ORDER BY `index`")=="501|3|998877|7|23|0|\n9001|1|12345|0|0|77|\n","return preserves exact ordinary and pet metadata");
        require(result("SELECT * FROM pet")==pets,"return never recreates or changes pet identity");
        auto returned=snapshot();mapif_Mail_return(0,99001314,801,990014,990013);
        require(snapshot()==returned,"old return retry cannot duplicate assets");
        const int returned_id=std::stoi(result("SELECT id FROM mail"));
        mapif_Mail_return(0,99001313,returned_id,990013,990014);
        require(snapshot()==returned,"returned mail cannot bounce indefinitely");
        reset();sql("DELETE FROM `char` WHERE char_id=99001313");auto orphan=snapshot();
        mapif_Mail_return(0,99001314,801,990014,990013);
        require(snapshot()==orphan,"missing sender retains source assets");
        reset();sql("UPDATE mail SET send_id=0 WHERE id=801");orphan=snapshot();
        mapif_Mail_return(0,99001314,801,990014,0);
        require(snapshot()==orphan,"system mail cannot be manually returned and destroyed");
        // Packet emission is buffered locally; no socket is opened or flushed.
        socket_data wire{};wire.wdata=static_cast<uint8*>(aCalloc(1024*1024,1));wire.max_wdata=1024*1024;wire.flag.server=1;
        require(session[1]==nullptr,"unused fixture transport slot");session[1]=&wire;
        for(int content=0;content<3;++content){
            reset();if(content!=0)sql("UPDATE mail SET zeny=0 WHERE id=801");
            if(content==2)sql("DELETE FROM mail_attachments WHERE id=801");
            const auto before=snapshot();wire.wdata_size=0;
            const bool removed=mapif_Mail_delete(1,99001314,801,990014);
            require(removed==(content==2),"manual deletion requires empty authoritative mail");
            if(content!=2)require(snapshot()==before,"failed manual deletion preserves all assets");
            require(wire.wdata[10]==(content!=2),"delete ACK reports actual outcome");
        }
        session[1]=nullptr;aFree(wire.wdata);
        reset();charserv_config.mail_return_empty=0;sql("DELETE FROM mail_attachments");sql("UPDATE mail SET zeny=0");
        mapif_Mail_return(0,99001314,801,990014,990013);
        require(result("SELECT COUNT(*) FROM mail")=="0|\n","configured empty expiry deletes without creating returned mail");
        reset();mail_timer_sub(1,MAIL_INBOX_NORMAL);
        require(result("SELECT dest_id,type FROM mail")=="99001313|2|\n","normal expiry returns to sender");
        sql("UPDATE mail SET time=1");mail_timer_sub(1,MAIL_INBOX_RETURNED);
        require(result("SELECT COUNT(*) FROM mail")=="0|\n" && result("SELECT COUNT(*) FROM mail_attachments")=="0|\n","configured final expiry removes header and attachment rows together");
        reset();sql("UPDATE mail SET send_id=0");mail_timer_sub(1,MAIL_INBOX_NORMAL);
        require(result("SELECT COUNT(*) FROM mail")=="0|\n" && result("SELECT COUNT(*) FROM mail_attachments")=="0|\n","system expiry retains configured cleanup policy");
    }
    std::cout<<"MAIL_LIFECYCLE_SQL_PASS "<<checks<<std::endl;return 0;
}
