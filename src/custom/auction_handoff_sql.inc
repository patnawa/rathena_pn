// Included by int_auction.cpp. Wallet, inventory and receipt are owned by shop SQL.
static int auction_inventory_matches(const pn_shop::Commit& r){
    if(Sql_Query(sql_handle,"SELECT id FROM `%s` WHERE char_id=%u FOR UPDATE",schema_config.inventory_db,r.char_id)!=SQL_SUCCESS)return -1;
    Sql_FreeResult(sql_handle);
    s_storage loaded{};
    if(!char_memitemdata_from_sql(&loaded,MAX_INVENTORY,r.char_id,TABLE_INVENTORY,0))return -1;
    std::vector<item> before,after;
    bool removed=r.kind==pn_shop::AuctionBid;
    for(auto it:loaded.u.items_inventory)if(it.nameid){
        it.id=0;
        if(!removed){
            auto one=it;one.amount=1;
            auto expected=r.auction.listing().item;expected.favorite=one.favorite; // UI preference is not auction custody.
            if(!memcmp(&one,&expected,sizeof(item))){--it.amount;removed=true;}
        }
        if(it.amount)before.push_back(it);
    }
    for(auto it:r.items)if(it.nameid){it.id=0;after.push_back(it);}
    auto less=[](const item& a,const item& b){return memcmp(&a,&b,sizeof(item))<0;};
    std::sort(before.begin(),before.end(),less);std::sort(after.begin(),after.end(),less);
    return removed && before.size()==after.size() &&
        (before.empty() || !memcmp(before.data(),after.data(),before.size()*sizeof(item)))?1:0;
}

int pn_auction_apply_locked(const pn_shop::Commit& r,std::vector<mail_message>& mails){
    if(!Sql_InTransaction(sql_handle) || !pn_shop::auction_kind(r.kind))return -1;
    const int inventory=auction_inventory_matches(r);if(inventory!=1)return inventory;
    if(r.kind==pn_shop::AuctionRegister){
        if(Sql_Query(sql_handle,"SELECT COUNT(*) FROM `%s` WHERE seller_id=%u",schema_config.auction_db,r.char_id)!=SQL_SUCCESS)return -1;
        char* count=nullptr;
        const bool read=Sql_NextRow(sql_handle)==SQL_SUCCESS && Sql_GetData(sql_handle,0,&count,nullptr)==SQL_SUCCESS && count;
        const bool room=read && strtol(count,nullptr,10)<5;Sql_FreeResult(sql_handle);
        if(!read)return -1;if(!room)return 0;
        auto listing=std::make_shared<auction_data>(r.auction.listing());
        // Authenticate seller name from SQL, never from the client envelope.
        if(Sql_Query(sql_handle,"SELECT name FROM `%s` WHERE char_id=%u AND account_id=%u",schema_config.char_db,r.char_id,r.account_id)!=SQL_SUCCESS)return -1;
        char* name=nullptr;const bool named=Sql_NextRow(sql_handle)==SQL_SUCCESS && Sql_GetData(sql_handle,0,&name,nullptr)==SQL_SUCCESS && name;
        if(named)safestrncpy(listing->seller_name,name,sizeof(listing->seller_name));Sql_FreeResult(sql_handle);
        if(!named)return -1;
        return auction_insert(listing)?1:-1;
    }
    const uint32 id=r.auction.auction_id;const int32 bid=r.auction.bid;
    if(Sql_Query(sql_handle,"SELECT seller_id,buyer_id,price,buynow,timestamp FROM `%s` WHERE auction_id=%u FOR UPDATE",schema_config.auction_db,id)!=SQL_SUCCESS)return -1;
    const int row=Sql_NextRow(sql_handle);
    auto listing=util::umap_find(auction_db,id);
    bool same=row==SQL_SUCCESS && listing;
    if(same){
        const int64 expected[]={listing->seller_id,listing->buyer_id,listing->price,listing->buynow,listing->timestamp};
        for(int i=0;i<5 && same;++i){char* data=nullptr;same=Sql_GetData(sql_handle,i,&data,nullptr)==SQL_SUCCESS && data && strtoll(data,nullptr,10)==expected[i];}
    }
    Sql_FreeResult(sql_handle);
    if(row==SQL_NO_DATA)return 0;
    if(!same)return -1; // Retry after authoritative cache refresh; never debit stale custody.
    if(listing->seller_id==r.char_id || listing->price>=bid || listing->timestamp<=time(nullptr))return 0;
    if(Sql_Query(sql_handle,"SELECT COUNT(*) FROM `%s` WHERE buyer_id=%u",schema_config.auction_db,r.char_id)!=SQL_SUCCESS)return -1;
    char* count=nullptr;
    const bool counted=Sql_NextRow(sql_handle)==SQL_SUCCESS && Sql_GetData(sql_handle,0,&count,nullptr)==SQL_SUCCESS && count;
    const bool capped=counted && strtol(count,nullptr,10)>=5;Sql_FreeResult(sql_handle);
    if(!counted)return -1;
    if(capped && bid<listing->buynow && listing->buyer_id!=r.char_id)return 0;
    auto next=std::make_shared<auction_data>(*listing);next->buyer_id=r.char_id;next->price=bid;
    if(Sql_Query(sql_handle,"SELECT name FROM `%s` WHERE char_id=%u AND account_id=%u",schema_config.char_db,r.char_id,r.account_id)!=SQL_SUCCESS)return -1;
    char* name=nullptr;const bool named=Sql_NextRow(sql_handle)==SQL_SUCCESS && Sql_GetData(sql_handle,0,&name,nullptr)==SQL_SUCCESS && name;
    if(named)safestrncpy(next->buyer_name,name,sizeof(next->buyer_name));Sql_FreeResult(sql_handle);if(!named)return -1;
    if(listing->buyer_id)mails.push_back(auction_mail(listing->buyer_id,listing->buyer_name,listing->buyer_id==r.char_id?209:208,listing->price));
    if(bid>=listing->buynow){
        mails.push_back(auction_mail(r.char_id,next->buyer_name,210,0,&listing->item));
        mails.push_back(auction_mail(listing->seller_id,listing->seller_name,211,listing->buynow));
        if(bid>listing->buynow)mails.push_back(auction_mail(r.char_id,next->buyer_name,209,bid-listing->buynow));
    }
    for(auto& msg:mails)if(!mail_savemessage_locked(&msg))return -1;
    if(bid<listing->buynow)return auction_save(next)?1:-1;
    return Sql_Query(sql_handle,"DELETE FROM `%s` WHERE auction_id=%u",schema_config.auction_db,id)==SQL_SUCCESS && Sql_NumRowsAffected(sql_handle)==1?1:-1;
}
