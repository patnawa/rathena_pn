// Production mail-bound Asset SQL. This fixture does not simulate network ACKs.
static int pet_mail_cases(){
    pet_seed();
    std::strcpy(schema_config.mail_db,"mail");std::strcpy(schema_config.mail_attachment_db,"mail_attachments");
    charserv_config.mail_retrieve=0;
    sql("DELETE FROM mail_attachments");sql("DELETE FROM mail");
    sql("INSERT INTO mail(id,dest_id,status,zeny) VALUES(701,99001313,2,75)");
    sql("INSERT INTO mail_attachments(id,`index`,nameid,amount,identify,refine,bound,option_id0,option_val0,unique_id) VALUES(701,0,9001,2,1,3,2,1,7,998877),(701,4,503,3,1,0,0,0,0,0),(701,7,501,1,1,0,0,0,0,0)");
    auto r=request(pn_shop::Asset);r.stock_count=0;for(auto& stock:r.stocks)stock={};
    r.wallet_after=r.wallet_before+75;r.cash_after=r.cash_before;r.kafra_after=r.kafra_before;
    r.mail_id=701;r.mail_zeny=75;r.mail_items[0]=pet_output().egg;r.mail_items[0].amount=2;
    r.mail_items[1].nameid=503;r.mail_items[1].amount=3;r.mail_items[1].identify=1;
    r.mail_items[2].nameid=501;r.mail_items[2].amount=1;r.mail_items[2].identify=1;
    r.pet_count=2;r.counter_after=r.counter_before+2;
    for(int i=0;i<2;++i){r.pets[i].output=pet_output();r.pets[i].output.egg.unique_id=(uint64(r.char_id)<<32)|(r.counter_before+i);}
    r.items[1].amount+=3;r.items[3]={};
    require(pn_shop::valid(r),"mail-bound pet batch validates");
    auto mailstate=[&](bool receipts=true){return pet_state(receipts)+result("SELECT * FROM mail ORDER BY id")+result("SELECT * FROM mail_attachments ORDER BY id,`index`");};
    auto departed=r;departed.sequence=99;auto original=mailstate(false);
    require(pn_shop_tosql(departed,false)==pn_shop::Rejected,"departed map owner cannot extract mail");
    require(mailstate(false)==original,"wrong session preserves complete mail and inventory");
    auto initial=mailstate();
    for(const std::string& fault:std::vector<std::string>{"BEFORE DELETE ON mail_attachments","BEFORE UPDATE ON mail","BEFORE INSERT ON pet","BEFORE INSERT ON pn_pet_entitlements","BEFORE UPDATE ON inventory","BEFORE INSERT ON inventory","BEFORE UPDATE ON `char`","BEFORE INSERT ON pn_shop_commits"}){
        sql("CREATE TRIGGER mail_pet_fault "+fault+" FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='mail pet write fault'");
        require(pn_shop_tosql(r,true)==pn_shop::Retry,"mail extraction write failure remains retryable");
        require(mailstate()==initial,"mail attachment, zeny, inventory, pets and receipt roll back together");
        sql("DROP TRIGGER mail_pet_fault");
    }
    for(int mode=0;mode<5;++mode){
        auto changed=r;changed.sequence=100+mode;
        if(mode==0)changed.mail_items[0].option[0].value++;
        if(mode==1)changed.mail_items[0].amount++;
        if(mode==2){changed.mail_zeny++;changed.wallet_after++;}
        if(mode==3)sql("UPDATE mail SET dest_id=99001314 WHERE id=701");
        if(mode==4)sql("UPDATE mail SET status=1 WHERE id=701");
        auto before=mailstate(false);
        require(pn_shop_tosql(changed,true)==pn_shop::Rejected,"stale metadata/quantity/zeny/owner/read status rejects");
        require(mailstate(false)==before,"mail conflict preserves all attachments and assets");
        sql("UPDATE mail SET dest_id=99001313,status=2 WHERE id=701");
    }
    require(pn_shop_tosql(r,true)==pn_shop::Committed,"mail batch commits with durable eggs and mixed ordinary output");
    require(result("SELECT COUNT(*) FROM mail_attachments WHERE id=701")=="0|\n","source attachments consumed once");
    require(result("SELECT zeny FROM mail WHERE id=701")=="0|\n","combined attached zeny consumed once");
    require(result("SELECT zeny FROM `char` WHERE char_id=99001313")=="1075|\n","attached zeny credited atomically");
    require(result("SELECT COUNT(*) FROM pn_pet_entitlements WHERE claimed=0")=="2|\n","every raw mailed egg remains recoverable");
    require(result("SELECT amount FROM inventory WHERE nameid=503 AND char_id=99001313")=="23|\n","ordinary attachment shares commit");
    newer();auto final=mailstate();
    require(pn_shop_tosql(r,false)==pn_shop::Committed,"lost mail ACK returns receipt after owner departure");
    require(mailstate()==final,"mail retry cannot restore old inventory or duplicate outputs");
    r.sequence=999;
    require(pn_shop_tosql(r,true)==pn_shop::Rejected,"different request cannot extract an empty mail twice");
    for(int encoded=0;encoded<2;++encoded){
        pet_seed();sql("DELETE FROM mail_attachments");sql("DELETE FROM mail");
        sql("INSERT INTO mail(id,dest_id,status) VALUES(702,99001313,2)");
        auto transfer=request(pn_shop::Asset);transfer.stock_count=0;for(auto& stock:transfer.stocks)stock={};
        transfer.mail_id=702;transfer.wallet_after=transfer.wallet_before;transfer.counter_after=transfer.counter_before;
        transfer.items[2]={};transfer.items[3]={};
        auto& attachment=transfer.mail_items[0];attachment.nameid=encoded?9001:501;attachment.amount=1;attachment.identify=1;
        if(encoded){
            sql("INSERT INTO pet(pet_id,class,name,egg_id,incubate) VALUES(77,1002,'Mailed existing',9001,1)");
            attachment.card[0]=pn_pet::egg_marker;attachment.card[1]=77;attachment.unique_id=12345;
        }
        transfer.items[2]=attachment;
        sql("INSERT INTO mail_attachments(id,`index`,nameid,amount,identify,card0,card1,unique_id) VALUES(702,0,"+std::to_string(attachment.nameid)+",1,1,"+std::to_string(attachment.card[0])+","+std::to_string(attachment.card[1])+","+std::to_string(attachment.unique_id)+")");
        if(encoded){
            sql("UPDATE pet SET incubate=0 WHERE pet_id=77");auto before=mailstate(false);
            require(pn_shop_tosql(transfer,true)==pn_shop::Rejected,"active pet identity cannot be delivered as mailed egg");
            require(mailstate(false)==before,"active pet rejection retains attachment");
            sql("UPDATE pet SET incubate=1 WHERE pet_id=77");transfer.sequence++;
        }
        require(pn_shop_tosql(transfer,true)==pn_shop::Committed,"ordinary or existing encoded egg mail commits without new pet creation");
        require(result("SELECT COUNT(*) FROM pn_pet_entitlements")=="0|\n","existing mail identity never creates a second entitlement");
        if(encoded)require(result("SELECT card1,unique_id FROM inventory WHERE nameid=9001")=="77|12345|\n","existing mailed egg keeps pet and item identities");
    }
    // Zeny-only claim leaves the independently claimable item rows untouched.
    pet_seed();sql("DELETE FROM mail_attachments");sql("DELETE FROM mail");
    sql("INSERT INTO mail(id,dest_id,status,zeny) VALUES(703,99001313,2,75)");
    sql("INSERT INTO mail_attachments(id,`index`,nameid,amount,identify) VALUES(703,0,501,2,1)");
    auto money=request(pn_shop::Asset);money.stock_count=0;for(auto& stock:money.stocks)stock={};
    money.mail_id=703;money.mail_zeny=75;money.wallet_after=1075;money.counter_after=money.counter_before;
    money.items[2]={};money.items[3]={};
    require(pn_shop::valid(money),"Zeny-only receipt retains original protocol frame size and validates");
    auto initial_money=mailstate();
    for(const std::string& fault:{"BEFORE UPDATE ON mail","BEFORE UPDATE ON `char`","BEFORE INSERT ON pn_shop_commits"}) {
        sql("CREATE TRIGGER mail_money_fault "+fault+" FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='mail money fault'");
        require(pn_shop_tosql(money,true)==pn_shop::Retry,"Zeny claim failure retries");
        require(mailstate()==initial_money,"Zeny claim rolls back wallet, mail and receipt together");
        sql("DROP TRIGGER mail_money_fault");
    }
    require(pn_shop_tosql(money,true)==pn_shop::Committed,"Zeny-only claim commits");
    require(result("SELECT zeny FROM mail WHERE id=703")=="0|\n" && result("SELECT zeny FROM `char` WHERE char_id=99001313")=="1075|\n","exact Zeny transfer");
    require(result("SELECT amount FROM mail_attachments WHERE id=703")=="2|\n","Zeny claim preserves items");
    auto settled_money=mailstate();require(pn_shop_tosql(money,false)==pn_shop::Committed && mailstate()==settled_money,"lost Zeny ACK replays exactly once after departure");

    // Outgoing attachments and fees share the same transaction/receipt as the
    // recipient mail. Rejection has no removed assets to refund.
    pet_seed();sql("DELETE FROM mail_attachments");sql("DELETE FROM mail");
    auto send=request(pn_shop::MailSend);send.stock_count=0;for(auto& stock:send.stocks)stock={};
    auto& envelope=*new(&send.outgoing) pn_shop::MailEnvelope{};
    std::strcpy(envelope.recipient,"Other fixture");std::strcpy(envelope.title,"Durable send");std::strcpy(envelope.body,"Real SQL mail test");
    envelope.zeny=100;envelope.attachment_price=25;
    envelope.attachments[0]=send.items[1];envelope.attachments[0].id=0;envelope.attachments[0].amount=3;
    send.items[1].amount=17;send.items[2]={};send.items[3]={};send.wallet_after=875;send.counter_after=send.counter_before;
    require(pn_shop::valid(send),"outgoing envelope validates without changing packed frame size");
    auto initial_send=mailstate();
    for(const std::string& fault:{"BEFORE INSERT ON mail","BEFORE INSERT ON mail_attachments","BEFORE UPDATE ON inventory","BEFORE UPDATE ON `char`","BEFORE INSERT ON pn_shop_commits"}) {
        sql("CREATE TRIGGER mail_send_fault "+fault+" FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='mail send fault'");
        require(pn_shop_tosql(send,true)==pn_shop::Retry,"outgoing SQL failure retries");
        require(mailstate()==initial_send,"outgoing mail, inventory, wallet and receipt roll back together");
        sql("DROP TRIGGER mail_send_fault");
    }
    auto missing=send;missing.sequence=99;std::strcpy(missing.outgoing.recipient,"Missing fixture");
    auto before_missing=mailstate(false);
    require(pn_shop_tosql(missing,true)==pn_shop::Rejected && mailstate(false)==before_missing,"missing recipient rejects without debit or inventory loss");
    require(pn_shop_tosql(send,true)==pn_shop::Committed,"outgoing attachments and fees commit");
    require(result("SELECT dest_id,zeny FROM mail")=="99001314|100|\n","recipient receives exact mail amount");
    require(result("SELECT nameid,amount FROM mail_attachments")=="503|3|\n","recipient receives exact attachment");
    require(result("SELECT amount FROM inventory WHERE id=2")=="17|\n" && result("SELECT zeny FROM `char` WHERE char_id=99001313")=="875|\n","sender inventory and exact fee debit commit");
    auto settled_send=mailstate();require(pn_shop_tosql(send,false)==pn_shop::Committed && mailstate()==settled_send,"outgoing lost ACK cannot send or charge twice");
    // Existing eggs move their identity; they must never be retired or minted.
    pet_seed();sql("DELETE FROM mail_attachments");sql("DELETE FROM mail");
    sql("INSERT INTO pet(pet_id,class,name,egg_id,incubate,account_id,char_id) VALUES(77,1002,'Outgoing egg',9001,1,990013,99001313)");
    sql("INSERT INTO inventory(id,char_id,nameid,amount,identify,card0,card1,unique_id) VALUES(9,99001313,9001,1,1,256,77,12345)");
    auto eggsend=send;eggsend.outgoing.attachments[0]={};auto& egg=eggsend.outgoing.attachments[0];
    egg.nameid=9001;egg.amount=1;egg.identify=1;egg.card[0]=pn_pet::egg_marker;egg.card[1]=77;egg.unique_id=12345;
    eggsend.items[1].amount=20;
    auto original_egg=mailstate();
    sql("CREATE TRIGGER outgoing_pet_fault BEFORE UPDATE ON pet FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='outgoing pet fault'");
    require(pn_shop_tosql(eggsend,true)==pn_shop::Retry && mailstate()==original_egg,"pet detach failure preserves mail and sender egg");
    sql("DROP TRIGGER outgoing_pet_fault");
    auto active=eggsend;active.sequence=100;sql("UPDATE pet SET incubate=0 WHERE pet_id=77");auto active_before=mailstate(false);
    require(pn_shop_tosql(active,true)==pn_shop::Rejected && mailstate(false)==active_before,"active pet cannot be mailed");
    sql("UPDATE pet SET incubate=1 WHERE pet_id=77");
    require(pn_shop_tosql(eggsend,true)==pn_shop::Committed,"existing pet transfers with outgoing mail");
    require(result("SELECT account_id,char_id,incubate FROM pet WHERE pet_id=77")=="0|0|1|\n","mailed pet is detached and incubated");
    require(result("SELECT card0,card1,unique_id FROM mail_attachments")=="256|77|12345|\n" && result("SELECT COUNT(*) FROM inventory WHERE nameid=9001")=="0|\n","same pet identity moves out of sender inventory");
    require(result("SELECT COUNT(*) FROM pn_pet_entitlements")=="0|\n","moving pet never creates an entitlement");
    for(int text_only=0;text_only<2;++text_only){
        pet_seed();sql("DELETE FROM mail_attachments");sql("DELETE FROM mail");
        auto plain=send;plain.items[1].amount=20;plain.outgoing.attachments[0]={};plain.outgoing.zeny=text_only?0:100;
        plain.wallet_after=text_only?1000:900;
        require(pn_shop_tosql(plain,true)==pn_shop::Committed,"text or Zeny-only outgoing mail commits");
        require(result("SELECT COUNT(*) FROM mail")=="1|\n" && result("SELECT COUNT(*) FROM mail_attachments")=="0|\n","attachment-free message is persisted once");
    }
    std::cout<<"RODEX_DURABLE_SQL_PASS zeny_claim outgoing_send rollback retry metadata pet_transfer text_only"<<std::endl;
    std::cout<<"PET_MAIL_ASSET_PASS "<<checks<<"; production SQL mail consumption and entitlement atomicity"<<std::endl;
    return 0;
}
