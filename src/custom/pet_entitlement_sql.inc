// Included by char/int_pet.cpp; joins, never nests, the asset transaction.
#include <custom/pet_entitlement.hpp>
#include <cerrno>
#include <limits>
#include <initializer_list>

namespace pn_pet {
static bool transactional_tables() {
    if(!Sql_InTransaction(sql_handle))return false;
    const char* tables[]={schema_config.pet_db,"pn_pet_entitlements"};
    for(const char* table:tables) {
        if(Sql_Query(sql_handle,"SELECT ENGINE FROM information_schema.TABLES WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME='%s'",table)!=SQL_SUCCESS)return false;
        char* engine=nullptr;
        bool ok=Sql_NextRow(sql_handle)==SQL_SUCCESS && Sql_GetData(sql_handle,0,&engine,nullptr)==SQL_SUCCESS && engine && !strcmpi(engine,"InnoDB");
        Sql_FreeResult(sql_handle);
        if(!ok)return false;
    }
    return true;
}
static bool number(uint32_t column,uint64_t& result) {
    char* value=nullptr;
    if(Sql_GetData(sql_handle,column,&value,nullptr)!=SQL_SUCCESS || !value || !*value || *value=='-')return false;
    char* end=nullptr;errno=0;result=strtoull(value,&end,10);
    return !errno && end!=value && !*end;
}
static bool blob(uint32_t column,void* destination,size_t size) {
    char* value=nullptr;size_t actual=0;
    if(Sql_GetData(sql_handle,column,&value,&actual)!=SQL_SUCCESS || !value || actual!=size)return false;
    memcpy(destination,value,size);return true;
}
Result issue_locked(const Key& key,const Output& output,Entitlement& result) {
    result={};
    if(!valid(key,output))return Result::Conflict;
    if(!transactional_tables())return Result::Error;
    if(Sql_Query(sql_handle,"SELECT id,char_id,pet_id,claimed,payload,egg FROM pn_pet_entitlements WHERE account_id=%u AND nonce_hi=%" PRIu64 " AND nonce_lo=%" PRIu64 " AND sequence=%" PRIu64 " AND ordinal=%u FOR UPDATE",
        key.account_id,key.nonce_hi,key.nonce_lo,key.sequence,static_cast<unsigned>(key.ordinal))!=SQL_SUCCESS)return Result::Error;
    int row=Sql_NextRow(sql_handle);
    if(row==SQL_SUCCESS) {
        uint64_t id=0,owner=0,pet_id=0,claimed=0;Output recorded{};item egg{};
        bool ok=number(0,id)&&number(1,owner)&&number(2,pet_id)&&number(3,claimed)&&
            blob(4,&recorded,sizeof(recorded))&&blob(5,&egg,sizeof(egg));
        Sql_FreeResult(sql_handle);
        if(!ok || !id || !pet_id || pet_id>INT32_MAX || claimed>1)return Result::Error;
        if(owner!=key.char_id || memcmp(&recorded,&output,sizeof(output)))return Result::Conflict;
        result.id=id;result.pet_id=static_cast<uint32_t>(pet_id);result.egg=egg;result.claimed=claimed!=0;
        return result.claimed?Result::AlreadyClaimed:Result::Pending;
    }
    Sql_FreeResult(sql_handle);
    if(row!=SQL_NO_DATA)return Result::Error;
    s_pet pet{};
    pet.class_=output.pet_class;pet.level=output.level;pet.egg_id=output.egg.nameid;
    pet.intimate=output.intimacy;pet.hungry=output.hungry;pet.incubate=1;
    memcpy(pet.name,output.name,sizeof(pet.name));
    if(!inter_pet_tosql(-1,&pet) || pet.pet_id<=0)return Result::Error;
    item egg=output.egg;
    egg.card[0]=egg_marker;egg.card[1]=static_cast<uint32_t>(pet.pet_id)&0xffff;
    egg.card[2]=static_cast<uint32_t>(pet.pet_id)>>16;
    // Same six intimacy tiers as pet_get_card3_intimacy, without map dependency.
    egg.card[3]=(output.intimacy<100?1:output.intimacy<250?2:output.intimacy<750?3:output.intimacy<910?4:5)<<1;
    SqlStmt stmt{*sql_handle};
    if(stmt.Prepare("INSERT INTO pn_pet_entitlements (account_id,char_id,nonce_hi,nonce_lo,sequence,ordinal,pet_id,payload,egg) VALUES (%u,%u,%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%u,%u,?,?)",
        key.account_id,key.char_id,key.nonce_hi,key.nonce_lo,key.sequence,static_cast<unsigned>(key.ordinal),static_cast<uint32_t>(pet.pet_id))!=SQL_SUCCESS ||
       stmt.BindParam(0,SQLDT_BLOB,const_cast<Output*>(&output),sizeof(output))!=SQL_SUCCESS ||
       stmt.BindParam(1,SQLDT_BLOB,&egg,sizeof(egg))!=SQL_SUCCESS || stmt.Execute()!=SQL_SUCCESS)return Result::Error;
    result.id=stmt.LastInsertId();result.pet_id=pet.pet_id;result.egg=egg;
    return result.id?Result::Issued:Result::Error;
}
Result claim_locked(uint32_t account_id,uint32_t char_id,uint64_t id,const item& expected) {
    if(!account_id || !char_id || !id)return Result::Conflict;
    if(!transactional_tables())return Result::Error;
    if(Sql_Query(sql_handle,"SELECT claimed,egg FROM pn_pet_entitlements WHERE id=%" PRIu64 " AND account_id=%u AND char_id=%u FOR UPDATE",id,account_id,char_id)!=SQL_SUCCESS)return Result::Error;
    int row=Sql_NextRow(sql_handle);uint64_t claimed=0;item stored{};
    bool ok=row==SQL_SUCCESS && number(0,claimed) && claimed<=1 && blob(1,&stored,sizeof(stored));
    Sql_FreeResult(sql_handle);
    if(row==SQL_NO_DATA)return Result::Conflict;
    if(!ok)return Result::Error;
    if(memcmp(&stored,&expected,sizeof(stored)))return Result::Conflict;
    if(claimed)return Result::AlreadyClaimed;
    const uint32_t pet_id=stored.card[1]|(stored.card[2]<<16);
    if(stored.card[0]!=egg_marker || !pet_id || pet_id>INT32_MAX)return Result::Error;
    if(Sql_Query(sql_handle,"SELECT pet_id FROM `%s` WHERE pet_id=%u AND egg_id=%u AND incubate=1 AND account_id=0 AND char_id=0 FOR UPDATE",schema_config.pet_db,pet_id,stored.nameid)!=SQL_SUCCESS)return Result::Error;
    row=Sql_NextRow(sql_handle);Sql_FreeResult(sql_handle);
    if(row!=SQL_SUCCESS)return Result::Error;
    if(Sql_Query(sql_handle,"UPDATE pn_pet_entitlements SET claimed=1,claimed_at=CURRENT_TIMESTAMP WHERE id=%" PRIu64 " AND claimed=0",id)!=SQL_SUCCESS || Sql_NumRowsAffected(sql_handle)!=1)return Result::Error;
    return Result::ClaimedNow;
}
Result retire_locked(uint32_t char_id,uint32_t pet_id,uint32_t egg_id) {
    if(!char_id || !pet_id || pet_id>INT32_MAX || !egg_id)return Result::Conflict;
    if(!transactional_tables())return Result::Error;
    // A claimed receipt survives egg consumption. A pending entitlement is not
    // an inventory material and must never be destroyed by this operation.
    if(Sql_Query(sql_handle,"SELECT claimed FROM pn_pet_entitlements WHERE pet_id=%u FOR UPDATE",pet_id)!=SQL_SUCCESS)return Result::Error;
    int row=Sql_NextRow(sql_handle);uint64_t claimed=0;
    bool pending=false,ok=true;
    if(row==SQL_SUCCESS){ok=number(0,claimed)&&claimed<=1;pending=!claimed;}
    else if(row!=SQL_NO_DATA)ok=false;
    Sql_FreeResult(sql_handle);
    if(!ok)return Result::Error;
    if(pending)return Result::Conflict;
    // Look across owners to fail closed on legacy duplicated pet identities.
    // The migration provides an index for this identity lookup.
    if(Sql_Query(sql_handle,"SELECT char_id,nameid,amount,equip,equip_switch FROM `%s` WHERE card0=%u AND card1=%u AND card2=%u FOR UPDATE",
        schema_config.inventory_db,egg_marker,pet_id&0xffff,pet_id>>16)!=SQL_SUCCESS)return Result::Error;
    row=Sql_NextRow(sql_handle);uint64_t owner=0,name=0,amount=0,equip=0,equip_switch=0;
    ok=row==SQL_SUCCESS && number(0,owner)&&number(1,name)&&number(2,amount)&&number(3,equip)&&number(4,equip_switch);
    const int next=ok?Sql_NextRow(sql_handle):SQL_NO_DATA;
    Sql_FreeResult(sql_handle);
    if(row==SQL_NO_DATA)return Result::Conflict;
    if(!ok || (next!=SQL_SUCCESS && next!=SQL_NO_DATA))return Result::Error;
    if(next==SQL_SUCCESS || owner!=char_id || name!=egg_id || amount!=1 || equip || equip_switch)return Result::Conflict;
    if(Sql_Query(sql_handle,"DELETE FROM `%s` WHERE pet_id=%u AND egg_id=%u AND incubate=1 AND account_id=0 AND char_id=0",
        schema_config.pet_db,pet_id,egg_id)!=SQL_SUCCESS)return Result::Error;
    return Sql_NumRowsAffected(sql_handle)==1?Result::RetiredNow:Result::Conflict;
}
}
