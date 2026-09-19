// Included by char/int_storage.cpp. No client-supplied SQL or table names.
#include <custom/bank_commit.hpp>
#include <custom/reserve_sql.inc>

static bool bank_tables_transactional() {
    const char* tables[] = {schema_config.char_db, schema_config.inventory_db,
        schema_config.acc_reg_num_table, "pn_bank_commits"};
    for (const char* table : tables) {
        if (Sql_Query(sql_handle, "SELECT ENGINE FROM information_schema.TABLES WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME='%s'", table) != SQL_SUCCESS)
            return false;
        char* engine = nullptr;
        bool good = Sql_NextRow(sql_handle) == SQL_SUCCESS &&
            Sql_GetData(sql_handle, 0, &engine, nullptr) == SQL_SUCCESS && engine && strcmpi(engine, "InnoDB") == 0;
        Sql_FreeResult(sql_handle);
        if (!good) return false;
    }
    return true;
}

// Acknowledging a repeated commit must never overwrite a newer inventory.
// 1 = committed (including identical retry), 0 = no committed change.
bool bank_tosql(const pn_bank_commit& request, s_storage& inventory) {
    if (request.action == 7) return reserve_tosql(request, inventory);
    if (request.account_id == 0 || request.char_id == 0 || request.request_id == 0 ||
        (request.nonce_hi == 0 && request.nonce_lo == 0) ||
        request.action < pn_bank::Deposit || request.action > pn_bank::SellNote || request.amount <= 0 ||
        request.bank_after < 0 || request.bank_after > MAX_BANK_ZENY ||
        request.wallet_after < 0 || request.wallet_after > MAX_ZENY || inventory.type != TABLE_INVENTORY ||
        !bank_tables_transactional() || Sql_BeginTransaction(sql_handle) != SQL_SUCCESS)
        return false;
    bool success = false, wrote = false;
    do {
        if (Sql_Query(sql_handle, "SELECT `char_id`,`action`,`amount` FROM `pn_bank_commits` WHERE `account_id`=%u AND `nonce_hi`=%" PRIu64 " AND `nonce_lo`=%" PRIu64 " AND `request_id`=%" PRIu64 " FOR UPDATE",
            request.account_id, request.nonce_hi, request.nonce_lo, request.request_id) != SQL_SUCCESS) break;
        int row = Sql_NextRow(sql_handle);
        if (row == SQL_SUCCESS) {
            char *cid = nullptr, *action = nullptr, *amount = nullptr;
            success = Sql_GetData(sql_handle, 0, &cid, nullptr) == SQL_SUCCESS && cid &&
                Sql_GetData(sql_handle, 1, &action, nullptr) == SQL_SUCCESS && action &&
                Sql_GetData(sql_handle, 2, &amount, nullptr) == SQL_SUCCESS && amount &&
                strtoul(cid, nullptr, 10) == request.char_id && strtoul(action, nullptr, 10) == request.action &&
                strtoll(amount, nullptr, 10) == request.amount;
            Sql_FreeResult(sql_handle);
            // A successful old request contains an old snapshot; do not apply it again.
            break;
        }
        Sql_FreeResult(sql_handle);
        if (row != SQL_NO_DATA) break;
        if (Sql_Query(sql_handle, "SELECT `account_id` FROM `%s` WHERE `char_id`=%u AND `account_id`=%u FOR UPDATE",
            schema_config.char_db, request.char_id, request.account_id) != SQL_SUCCESS) break;
        bool owner = Sql_NextRow(sql_handle) == SQL_SUCCESS;
        Sql_FreeResult(sql_handle);
        if (!owner) break;
        if (char_memitemdata_to_sql(inventory.u.items_inventory, MAX_INVENTORY,
            request.char_id, TABLE_INVENTORY, inventory.stor_id) != 0) break;
        if (Sql_Query(sql_handle, "UPDATE `%s` SET `zeny`=%" PRId64 " WHERE `char_id`=%u AND `account_id`=%u",
            schema_config.char_db, request.wallet_after, request.char_id, request.account_id) != SQL_SUCCESS) break;
        if (Sql_Query(sql_handle, "REPLACE INTO `%s` (`account_id`,`key`,`index`,`value`) VALUES (%u,'#BANKVAULT',0,%" PRId64 ")",
            schema_config.acc_reg_num_table, request.account_id, request.bank_after) != SQL_SUCCESS) break;
        if (Sql_Query(sql_handle, "INSERT INTO `pn_bank_commits` (`account_id`,`nonce_hi`,`nonce_lo`,`request_id`,`char_id`,`action`,`amount`,`bank_before`,`bank_after`,`wallet_before`,`wallet_after`) VALUES (%u,%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%u,%u,%" PRId64 ",%" PRId64 ",%" PRId64 ",%" PRId64 ",%" PRId64 ")",
            request.account_id, request.nonce_hi, request.nonce_lo, request.request_id, request.char_id,
            request.action, request.amount, request.bank_before, request.bank_after,
            request.wallet_before, request.wallet_after) != SQL_SUCCESS) break;
        success = wrote = true;
    } while (false);
    if (Sql_EndTransaction(sql_handle, success) != SQL_SUCCESS) success = false;
    // Update only after a NEW committed write. Otherwise a subsequent wallet
    // returning to the old cached value would be skipped by the normal saver.
    if (success && wrote) {
        auto found = char_get_chardb().find(request.char_id);
        if (found != char_get_chardb().end() && found->second)
            found->second->zeny = static_cast<int32>(request.wallet_after);
    }
    return success;
}

static void mapif_parse_BankCommit(int32 fd) {
    if (RFIFOW(fd, 2) != sizeof(pn_bank_commit) + sizeof(s_storage)) return;
    pn_bank_commit request;
    s_storage inventory;
    memcpy(&request, RFIFOP(fd, 0), sizeof(request));
    memcpy(&inventory, RFIFOP(fd, sizeof(request)), sizeof(inventory));
    pn_bank_ack reply;
    reply.account_id = request.account_id; reply.char_id = request.char_id;
    reply.nonce_hi = request.nonce_hi; reply.nonce_lo = request.nonce_lo;
    reply.request_id = request.request_id;
    reply.committed = bank_tosql(request, inventory) ? 1 : 0;
    WFIFOHEAD(fd, sizeof(reply));
    memcpy(WFIFOP(fd, 0), &reply, sizeof(reply));
    WFIFOSET(fd, sizeof(reply));
}
