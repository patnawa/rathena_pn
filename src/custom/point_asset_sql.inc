#include <custom/global_point.hpp>
// Included within the production asset transaction after numeric SQL parsing.
// The original map stream flushes permanent registries before this CAS. Session
// points have no persistent representation; the authenticated map fences the key
// until ACK, and disconnection discards that session's remaining points.
static bool pn_point_validate_locked(const pn_shop::Commit& r,bool& reject) {
    const auto& point=r.point;
    if(!point.scope || point.scope==pn_shop::SessionPoint)return true;
    if(point.scope==pn_shop::GlobalPoint && reject)return true;
    const bool account=point.scope==pn_shop::AccountPoint || point.scope==pn_shop::GlobalPoint;
    std::string table=account?schema_config.acc_reg_num_table:schema_config.char_reg_num_table;
    if(point.scope==pn_shop::GlobalPoint && !pn_global_point::registry_locked(sql_handle,r,table))return false;
    char escaped[sizeof(point.key)*2+1]{};Sql_EscapeStringLen(sql_handle,escaped,point.key,strlen(point.key));
    if(Sql_Query(sql_handle,"SELECT value FROM `%s` WHERE `%s`=%u AND `key`='%s' AND `index`=0 FOR UPDATE",
        table.c_str(),account?"account_id":"char_id",account?r.account_id:r.char_id,escaped)!=SQL_SUCCESS)return false;
    const int row=Sql_NextRow(sql_handle);int64 balance=0;
    const bool valid=row==SQL_NO_DATA || (row==SQL_SUCCESS && pn_shop_number(0,balance));
    Sql_FreeResult(sql_handle);if(!valid)return false;
    if(balance!=point.before)reject=true;
    return true;
}
static bool pn_point_debit_locked(const pn_shop::Commit& r) {
    const auto& point=r.point;
    if(!point.scope || point.scope==pn_shop::SessionPoint)return true;
    const bool account=point.scope==pn_shop::AccountPoint || point.scope==pn_shop::GlobalPoint;
    std::string table=account?schema_config.acc_reg_num_table:schema_config.char_reg_num_table;
    if(point.scope==pn_shop::GlobalPoint && !pn_global_point::registry_locked(sql_handle,r,table))return false;
    char escaped[sizeof(point.key)*2+1]{};Sql_EscapeStringLen(sql_handle,escaped,point.key,strlen(point.key));
    const bool debited=Sql_Query(sql_handle,"UPDATE `%s` SET value=%" PRId64 " WHERE `%s`=%u AND `key`='%s' AND `index`=0 AND value=%" PRId64,
        table.c_str(),point.after,account?"account_id":"char_id",account?r.account_id:r.char_id,escaped,point.before)==SQL_SUCCESS && Sql_NumRowsAffected(sql_handle)==1;
    if(!debited)return false;
    if(point.scope==pn_shop::GlobalPoint)return true; // Its durable barrier records the protected key.
    return Sql_Query(sql_handle,"INSERT IGNORE INTO pn_point_registry_keys(account_id,char_id,point_key) VALUES(%u,%u,'%s')",
        r.account_id,point.scope==pn_shop::CharacterPoint?r.char_id:0,escaped)==SQL_SUCCESS;
}
