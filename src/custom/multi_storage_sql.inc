// Included by char/int_storage.cpp. Owner-checked, atomic personal storage persistence.
// PN contributions: GPL-3.0-or-later.
#include <custom/multi_storage_protocol.hpp>

static bool storage_engine_is_transactional(const char* table) {
    if (Sql_Query(sql_handle,"SELECT ENGINE FROM information_schema.TABLES WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME='%s'",table) != SQL_SUCCESS) return false;
    char* engine = nullptr;
    bool good = Sql_NextRow(sql_handle) == SQL_SUCCESS &&
        Sql_GetData(sql_handle,0,&engine,nullptr) == SQL_SUCCESS && engine && strcmpi(engine,"InnoDB") == 0;
    Sql_FreeResult(sql_handle);return good;
}
static bool storage_account_owns_character(uint32 account, uint32 character, bool lock) {
    if (Sql_Query(sql_handle,"SELECT `char_id` FROM `%s` WHERE `account_id`=%u AND `char_id`=%u%s",
        schema_config.char_db,account,character,lock?" FOR UPDATE":"") != SQL_SUCCESS) return false;
    bool found = Sql_NextRow(sql_handle) == SQL_SUCCESS;Sql_FreeResult(sql_handle);return found;
}
static int storage_page_paid_sql(uint32 account, uint8 page) {
    if (!pn_storage::managed(page) || pn_storage::free_page(page)) return true;
    if (Sql_Query(sql_handle,"SELECT `value` FROM `%s` WHERE `account_id`=%u AND `key`='%s' AND `index`=%u",
        schema_config.acc_reg_num_table,account,pn_storage::paid_key,page) != SQL_SUCCESS) return -1;
    char* value = nullptr;
    const int row = Sql_NextRow(sql_handle);
    bool paid = row == SQL_SUCCESS && Sql_GetData(sql_handle,0,&value,nullptr) == SQL_SUCCESS && value && strcmp(value,"1") == 0;
    Sql_FreeResult(sql_handle);return row == SQL_SUCCESS || row == SQL_NO_DATA ? (paid ? 1 : 0) : -1;
}
static bool personal_storage_schema_ready() {
    if (!storage_engine_is_transactional(schema_config.cart_db) ||
        !storage_engine_is_transactional("pn_storage_commits")) return false;
    for (const auto& entry : interServerDb)
        if (!storage_engine_is_transactional(entry.second->table)) return false;
    return true;
}

bool personal_storage_tosql(const pn_storage_request& request, const item* source, const item* page) {
    const bool unlock = request.action == pn_storage::Unlock, cart = request.action == pn_storage::Cart;
    auto table = interServerDb.find(request.page);
    if (!request.account_id || !request.char_id || !request.sequence || (!request.nonce_hi && !request.nonce_lo) ||
        request.reserved || request.action < pn_storage::Inventory || request.action > pn_storage::Unlock || !table || !source ||
        (!unlock && !page) || request.wallet_before < 0 || request.wallet_before > MAX_WALLET_ZENY ||
        request.wallet_after < 0 || request.wallet_after > MAX_WALLET_ZENY ||
        (unlock && (!pn_storage::purchasable(request.page) || request.wallet_before < pn_storage::expansion_price ||
                    request.wallet_after < request.wallet_before-pn_storage::expansion_price)) ||
        !storage_engine_is_transactional(schema_config.char_db) ||
        !storage_engine_is_transactional(cart ? schema_config.cart_db : schema_config.inventory_db) ||
        !storage_engine_is_transactional(schema_config.acc_reg_num_table) ||
        !storage_engine_is_transactional(table->table) || !storage_engine_is_transactional("pn_storage_commits") ||
        Sql_BeginTransaction(sql_handle) != SQL_SUCCESS) return false;
    bool success = false, wrote = false;
    do {
        if (Sql_Query(sql_handle,"SELECT `char_id`,`page`,`action` FROM `pn_storage_commits` WHERE `account_id`=%u AND `nonce_hi`=%" PRIu64 " AND `nonce_lo`=%" PRIu64 " AND `request_id`=%" PRIu64 " FOR UPDATE",
            request.account_id,request.nonce_hi,request.nonce_lo,request.sequence) != SQL_SUCCESS) break;
        int row = Sql_NextRow(sql_handle);
        if (row == SQL_SUCCESS) {
            char *cid=nullptr,*id=nullptr,*action=nullptr;
            success = Sql_GetData(sql_handle,0,&cid,nullptr) == SQL_SUCCESS && cid &&
                Sql_GetData(sql_handle,1,&id,nullptr) == SQL_SUCCESS && id &&
                Sql_GetData(sql_handle,2,&action,nullptr) == SQL_SUCCESS && action &&
                strtoul(cid,nullptr,10) == request.char_id && strtoul(id,nullptr,10) == request.page && strtoul(action,nullptr,10) == request.action;
            Sql_FreeResult(sql_handle);break; // A retry acknowledges; it never reapplies its old arrays.
        }
        Sql_FreeResult(sql_handle);
        if (row != SQL_NO_DATA || !storage_account_owns_character(request.account_id,request.char_id,true)) break;
        const int available = storage_page_paid_sql(request.account_id,request.page);
        if (available < 0 || (unlock ? available != 0 : available != 1)) break;
        if (char_memitemdata_to_sql(source,cart?MAX_CART:MAX_INVENTORY,request.char_id,cart?TABLE_CART:TABLE_INVENTORY,0) != 0) break;
        if (!unlock && char_memitemdata_to_sql(page,MAX_STORAGE,
            request.page == pn_storage::character ? request.char_id : request.account_id,TABLE_STORAGE,request.page) != 0) break;
        if (Sql_Query(sql_handle,"UPDATE `%s` SET `zeny`=%" PRId64 " WHERE `char_id`=%u AND `account_id`=%u",
            schema_config.char_db,request.wallet_after,request.char_id,request.account_id) != SQL_SUCCESS) break;
        if (unlock && Sql_Query(sql_handle,"REPLACE INTO `%s` (`account_id`,`key`,`index`,`value`) VALUES (%u,'%s',%u,1)",
            schema_config.acc_reg_num_table,request.account_id,pn_storage::paid_key,request.page) != SQL_SUCCESS) break;
        if (Sql_Query(sql_handle,"INSERT INTO `pn_storage_commits` (`account_id`,`nonce_hi`,`nonce_lo`,`request_id`,`char_id`,`page`,`action`,`wallet_before`,`wallet_after`) VALUES (%u,%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%u,%u,%u,%" PRId64 ",%" PRId64 ")",
            request.account_id,request.nonce_hi,request.nonce_lo,request.sequence,request.char_id,request.page,request.action,request.wallet_before,request.wallet_after) != SQL_SUCCESS) break;
        success = wrote = true;
    } while (false);
    if (Sql_EndTransaction(sql_handle,success) != SQL_SUCCESS) success = false;
    if (success && wrote) {
        auto cached = char_get_chardb().find(request.char_id);
        if (cached != char_get_chardb().end() && cached->second) cached->second->zeny = request.wallet_after;
    }
    return success;
}
static pn_storage_reply personal_storage_reply(const pn_storage_request& request) {
    pn_storage_reply reply;
    reply.account_id=request.account_id;reply.char_id=request.char_id;
    reply.nonce_hi=request.nonce_hi;reply.nonce_lo=request.nonce_lo;reply.sequence=request.sequence;reply.page=request.page;
    return reply;
}
static void mapif_parse_StoragePageLoad(int32 fd) {
    pn_storage_request request;memcpy(&request,RFIFOP(fd,0),sizeof(request));
    auto reply = personal_storage_reply(request);s_storage storage{};storage.stor_id=request.page;
    bool valid = request.length==sizeof(request) && !request.reserved && request.account_id && request.char_id &&
        request.sequence && (request.nonce_hi || request.nonce_lo) && request.page && request.action<=STOR_MODE_ALL &&
        interServerDb.find(request.page) && storage_account_owns_character(request.account_id,request.char_id,false) &&
        storage_page_paid_sql(request.account_id,request.page) == 1;
    if (valid) reply.success=storage_fromsql(request.page==pn_storage::character?request.char_id:request.account_id,&storage)?1:0;
    storage.state.put=(request.action&STOR_MODE_PUT)?1:0;storage.state.get=(request.action&STOR_MODE_GET)?1:0;
    reply.length=sizeof(reply)+sizeof(storage);
    WFIFOHEAD(fd,reply.length);memcpy(WFIFOP(fd,0),&reply,sizeof(reply));
    memcpy(WFIFOP(fd,sizeof(reply)),&storage,sizeof(storage));WFIFOSET(fd,reply.length);
}
static void mapif_parse_StorageCommit(int32 fd) {
    if (RFIFOW(fd,2)<sizeof(pn_storage_request)) return;
    pn_storage_request request;memcpy(&request,RFIFOP(fd,0),sizeof(request));
    const size_t source_bytes=(request.action==pn_storage::Cart?MAX_CART:MAX_INVENTORY)*sizeof(item);
    const size_t page_bytes=request.action==pn_storage::Unlock?0:MAX_STORAGE*sizeof(item);
    if (request.length!=sizeof(request)+source_bytes+page_bytes) return;
    // Copy packed wire arrays into aligned native containers before passing SQL helpers.
    s_storage source{},page{};
    memcpy(source.u.items_storage,RFIFOP(fd,sizeof(request)),source_bytes);
    if (page_bytes) memcpy(page.u.items_storage,RFIFOP(fd,sizeof(request)+source_bytes),page_bytes);
    auto reply=personal_storage_reply(request);reply.packet=0x3894;
    reply.success=personal_storage_tosql(request,source.u.items_storage,page_bytes?page.u.items_storage:nullptr)?1:0;
    WFIFOHEAD(fd,sizeof(reply));memcpy(WFIFOP(fd,0),&reply,sizeof(reply));WFIFOSET(fd,sizeof(reply));
}
