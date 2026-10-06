// Character-server settlement. Caller retains cache ownership until COMMIT.
enum class AuctionWrite { Failed, Committed, Missing, Unknown };
static mail_message auction_mail(uint32 recipient,const char* name,int body,int64 zeny,const item* attachment=nullptr){
    mail_message msg{};msg.dest_id=recipient;msg.zeny=zeny;msg.timestamp=time(nullptr);
    safestrncpy(msg.send_name,msg_txt(200),sizeof(msg.send_name));
    safestrncpy(msg.dest_name,name,sizeof(msg.dest_name));
    safestrncpy(msg.title,msg_txt(201),sizeof(msg.title));
    safestrncpy(msg.body,msg_txt(body),sizeof(msg.body));
    if(attachment)msg.item[0]=*attachment;
    return msg;
}
static AuctionWrite auction_write(const auction_data& before,const std::shared_ptr<auction_data>& after,std::vector<mail_message>& mails){
    for(const char* table:{schema_config.auction_db,schema_config.mail_db,schema_config.mail_attachment_db}){
        if(Sql_Query(sql_handle,"SELECT ENGINE FROM information_schema.TABLES WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME='%s'",table)!=SQL_SUCCESS)return AuctionWrite::Failed;
        char* engine=nullptr;
        const bool safe=Sql_NextRow(sql_handle)==SQL_SUCCESS && Sql_GetData(sql_handle,0,&engine,nullptr)==SQL_SUCCESS && engine && !strcmpi(engine,"InnoDB");
        Sql_FreeResult(sql_handle);if(!safe)return AuctionWrite::Failed;
    }
    if(Sql_BeginTransaction(sql_handle)!=SQL_SUCCESS)return AuctionWrite::Failed;
    bool ready=false,missing=false;
    do{
        if(Sql_Query(sql_handle,"SELECT seller_id,buyer_id,price,buynow FROM `%s` WHERE auction_id=%u FOR UPDATE",schema_config.auction_db,before.auction_id)!=SQL_SUCCESS)break;
        const int row=Sql_NextRow(sql_handle);missing=row==SQL_NO_DATA;
        bool same=row==SQL_SUCCESS;const int64 expected[]={before.seller_id,before.buyer_id,before.price,before.buynow};
        for(int i=0;i<4 && same;++i){char* value=nullptr;same=Sql_GetData(sql_handle,i,&value,nullptr)==SQL_SUCCESS && value && strtoll(value,nullptr,10)==expected[i];}
        Sql_FreeResult(sql_handle);if(!same)break;
        bool saved=true;
        for(auto& msg:mails)if(!mail_savemessage_locked(&msg)){saved=false;break;}
        if(!saved)break;
        if(after){if(!auction_save(after))break;}
        else if(Sql_Query(sql_handle,"DELETE FROM `%s` WHERE auction_id=%u",schema_config.auction_db,before.auction_id)!=SQL_SUCCESS || Sql_NumRowsAffected(sql_handle)!=1)break;
        ready=true;
    }while(false);
    if(Sql_EndTransaction(sql_handle,ready)!=SQL_SUCCESS)return AuctionWrite::Unknown;
    if(!ready)return missing?AuctionWrite::Missing:AuctionWrite::Failed;
    for(auto& msg:mails)mapif_Mail_new(&msg);
    return AuctionWrite::Committed;
}
static bool auction_settle(std::shared_ptr<auction_data> auction,int item_text,int payment_text){
    std::vector<mail_message> mails;
    mails.push_back(auction_mail(auction->buyer_id?auction->buyer_id:auction->seller_id,
        auction->buyer_id?auction->buyer_name:auction->seller_name,item_text,0,&auction->item));
    if(auction->buyer_id)mails.push_back(auction_mail(auction->seller_id,auction->seller_name,payment_text,auction->price));
    const auto result=auction_write(*auction,nullptr,mails);
    if(result!=AuctionWrite::Committed && result!=AuctionWrite::Missing)return false;
    auction_delete(auction); // Cache/timer removal only; SQL already committed.
    return true;
}
