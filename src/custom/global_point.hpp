// Login/character adapter for a shared authoritative InnoDB database.
// No point debit occurs here: approval only proves the ordered login barrier.
#ifndef PN_GLOBAL_POINT_HPP
#define PN_GLOBAL_POINT_HPP
#include <common/sql.hpp>
#include <custom/shop_commit.hpp>
#include <string>
#include <vector>
#include <cstring>
namespace pn_global_point {
constexpr uint16_t request_packet=0x2744,ack_packet=0x2745;
constexpr unsigned timeout_seconds=60;
inline std::string where(const pn_shop::Commit& r) {
 return "account_id="+std::to_string(r.account_id)+" AND nonce_hi="+std::to_string(r.nonce_hi)+" AND nonce_lo="+std::to_string(r.nonce_lo)+" AND sequence="+std::to_string(r.sequence);
}
inline bool identifier(const std::string& name) {
 if(name.empty()||name.size()>32)return false;
 for(unsigned char c:name)if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'))return false;
 return true;
}
inline bool transactional(Sql* handle,const char* table) {
 if(!identifier(table) || Sql_Query(handle,"SELECT ENGINE FROM information_schema.TABLES WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME='%s'",table)!=SQL_SUCCESS)return false;
 char* value=nullptr;const bool ok=Sql_NextRow(handle)==SQL_SUCCESS && Sql_GetData(handle,0,&value,nullptr)==SQL_SUCCESS && value && !strcmp(value,"InnoDB");
 Sql_FreeResult(handle);return ok;
}
inline bool pending(Sql* handle,uint32_t account,const char* key=nullptr) {
 char escaped[65]{};if(key)Sql_EscapeStringLen(handle,escaped,key,strlen(key));
 const std::string filter=key?std::string("state=1 AND point_key='")+escaped+"'":"state<2";
 if(Sql_Query(handle,"SELECT account_id FROM pn_global_point_barriers WHERE account_id=%u AND %s LIMIT 1",account,filter.c_str())!=SQL_SUCCESS)return true;
 const int row=Sql_NextRow(handle);Sql_FreeResult(handle);return row!=SQL_NO_DATA;
}
// Terminal barrier rows remain a durable fence against displaced registry writers.
inline bool known_global_key(Sql* handle,uint32_t account,const char* key) {
 char escaped[65]{};if(strlen(key)>31)return true;Sql_EscapeStringLen(handle,escaped,key,strlen(key));
 if(Sql_Query(handle,"SELECT 1 FROM pn_global_point_barriers WHERE account_id=%u AND point_key='%s' LIMIT 1",account,escaped)!=SQL_SUCCESS)return true;
 const int row=Sql_NextRow(handle);Sql_FreeResult(handle);return row!=SQL_NO_DATA;
}
inline bool protected_key(Sql* handle,uint32_t account,uint32_t character,const char* key) {
 if(key[0]=='#' && key[1]=='#')return known_global_key(handle,account,key);
 char escaped[65]{};if(strlen(key)>31)return true;Sql_EscapeStringLen(handle,escaped,key,strlen(key));
 if(Sql_Query(handle,"SELECT 1 FROM pn_point_registry_keys WHERE account_id=%u AND char_id=%u AND point_key='%s' LIMIT 1",account,key[0]=='#'?0:character,escaped)!=SQL_SUCCESS)return true;
 const int row=Sql_NextRow(handle);Sql_FreeResult(handle);return row!=SQL_NO_DATA;
}
inline bool matches(Sql* handle,const pn_shop::Commit& r,unsigned column) {
 char* data=nullptr;size_t length=0;
 return Sql_GetData(handle,column,&data,&length)==SQL_SUCCESS && data && length==sizeof(r) && !memcmp(data,&r,sizeof(r));
}
inline bool state(Sql* handle,unsigned column,int& value) {
 char* data=nullptr;if(Sql_GetData(handle,column,&data,nullptr)!=SQL_SUCCESS || !data || !data[0] || data[1] || data[0]<'0' || data[0]>'2')return false;
 value=data[0]-'0';return true;
}
inline bool prepare(Sql* handle,const pn_shop::Commit& r) {
 if(r.point.scope!=pn_shop::GlobalPoint || !pn_shop::valid(r) || !transactional(handle,"pn_global_point_barriers") || Sql_BeginTransaction(handle)!=SQL_SUCCESS)return false;
 bool ok=false;
 do {
  // Never recreate admission fences for an already finished immutable request.
  if(Sql_Query(handle,"SELECT payload FROM pn_shop_commits WHERE %s FOR UPDATE",where(r).c_str())!=SQL_SUCCESS)break;
  const int receipt=Sql_NextRow(handle);Sql_FreeResult(handle);if(receipt!=SQL_NO_DATA)break;
  if(Sql_Query(handle,"SELECT payload,state FROM pn_global_point_barriers WHERE %s FOR UPDATE",where(r).c_str())!=SQL_SUCCESS)break;
  const int row=Sql_NextRow(handle);int phase=-1;
  if(row==SQL_SUCCESS)ok=matches(handle,r,0)&&state(handle,1,phase)&&phase<2;
  Sql_FreeResult(handle);if(row==SQL_SUCCESS)break;if(row!=SQL_NO_DATA)break;
  char key[65]{};Sql_EscapeStringLen(handle,key,r.point.key,strlen(r.point.key));
  SqlStmt stmt{*handle};
  ok=stmt.Prepare("INSERT INTO pn_global_point_barriers(account_id,nonce_hi,nonce_lo,sequence,char_id,point_key,payload) VALUES(%u,%llu,%llu,%llu,%u,'%s',?)",r.account_id,(unsigned long long)r.nonce_hi,(unsigned long long)r.nonce_lo,(unsigned long long)r.sequence,r.char_id,key)==SQL_SUCCESS &&
     stmt.BindParam(0,SQLDT_BLOB,const_cast<pn_shop::Commit*>(&r),sizeof(r))==SQL_SUCCESS && stmt.Execute()==SQL_SUCCESS;
 }while(false);
 return Sql_EndTransaction(handle,ok)==SQL_SUCCESS && ok;
}
inline bool approve(Sql* handle,const pn_shop::Commit& r,const char* registry_table) {
 if(r.point.scope!=pn_shop::GlobalPoint || !pn_shop::valid(r) || !transactional(handle,"pn_global_point_barriers") ||
    !transactional(handle,registry_table) || Sql_BeginTransaction(handle)!=SQL_SUCCESS)return false;
 bool ok=false;
 do {
  // The intent must already exist through the character SQL handle. A separate
  // login database cannot approve it, so split-database configurations fail closed.
  if(Sql_Query(handle,"SELECT payload,state,registry_table FROM pn_global_point_barriers WHERE %s FOR UPDATE",where(r).c_str())!=SQL_SUCCESS)break;
  int phase=-1;char* table=nullptr;
  ok=Sql_NextRow(handle)==SQL_SUCCESS && matches(handle,r,0) && state(handle,1,phase) && phase<2 && Sql_GetData(handle,2,&table,nullptr)==SQL_SUCCESS && table && (phase==0 || !strcmp(table,registry_table));
  Sql_FreeResult(handle);if(!ok)break;
  ok=Sql_Query(handle,"UPDATE pn_global_point_barriers SET state=1,registry_table='%s' WHERE %s AND state<2",registry_table,where(r).c_str())==SQL_SUCCESS;
 }while(false);
 return Sql_EndTransaction(handle,ok)==SQL_SUCCESS && ok;
}
inline bool registry_locked(Sql* handle,const pn_shop::Commit& r,std::string& table) {
 if(!Sql_InTransaction(handle) || Sql_Query(handle,"SELECT payload,state,registry_table FROM pn_global_point_barriers WHERE %s FOR UPDATE",where(r).c_str())!=SQL_SUCCESS)return false;
 int phase=-1;char* value=nullptr;
 bool ok=Sql_NextRow(handle)==SQL_SUCCESS && matches(handle,r,0) && state(handle,1,phase) && phase==1 && Sql_GetData(handle,2,&value,nullptr)==SQL_SUCCESS && value;
 if(ok)table=value;Sql_FreeResult(handle);
 return ok && identifier(table) && transactional(handle,table.c_str());
}
inline bool finish_locked(Sql* handle,const pn_shop::Commit& r) {
 return r.point.scope!=pn_shop::GlobalPoint || (Sql_InTransaction(handle) &&
  Sql_Query(handle,"UPDATE pn_global_point_barriers SET state=2 WHERE %s AND state<2",where(r).c_str())==SQL_SUCCESS);
}
inline bool expire(Sql* handle) {
 // A coordinated binary upgrade cannot resume an older raw Commit layout.
 // Once its admission timeout expires, terminalize it exactly like a canceled
 // intent so one legacy/corrupt row cannot poison every later expiry batch.
 if(Sql_Query(handle,"UPDATE pn_global_point_barriers SET state=2 WHERE state<2 AND created_at<DATE_SUB(NOW(),INTERVAL %u SECOND) AND OCTET_LENGTH(payload)<>%u",timeout_seconds,static_cast<unsigned>(sizeof(pn_shop::Commit)))!=SQL_SUCCESS)return false;
 if(Sql_Query(handle,"SELECT payload FROM pn_global_point_barriers WHERE state<2 AND created_at<DATE_SUB(NOW(),INTERVAL %u SECOND) LIMIT 32",timeout_seconds)!=SQL_SUCCESS)return false;
 std::vector<pn_shop::Commit> requests;int row;
 while((row=Sql_NextRow(handle))==SQL_SUCCESS) {
  char* bytes=nullptr;size_t length=0;
  if(Sql_GetData(handle,0,&bytes,&length)!=SQL_SUCCESS || !bytes || length!=sizeof(pn_shop::Commit)){Sql_FreeResult(handle);return false;}
  pn_shop::Commit r;memcpy(&r,bytes,sizeof(r));if(!pn_shop::valid(r)||r.point.scope!=pn_shop::GlobalPoint){Sql_FreeResult(handle);return false;}
  requests.push_back(r);
 }
 Sql_FreeResult(handle);if(row!=SQL_NO_DATA)return false;
 for(const auto& r:requests) {
  if(Sql_BeginTransaction(handle)!=SQL_SUCCESS)return false;bool ok=false;
  do {
   // Same lock order as the asset transaction. A concurrent commit wins or is
   // rejected before payment; expiry never refunds an already committed debit.
   if(Sql_Query(handle,"SELECT payload FROM pn_shop_commits WHERE %s FOR UPDATE",where(r).c_str())!=SQL_SUCCESS)break;
   const int receipt=Sql_NextRow(handle);bool same=receipt==SQL_SUCCESS && matches(handle,r,0);Sql_FreeResult(handle);
   if(receipt!=SQL_NO_DATA && !same)break;
   if(Sql_Query(handle,"SELECT payload,state FROM pn_global_point_barriers WHERE %s AND created_at<DATE_SUB(NOW(),INTERVAL %u SECOND) FOR UPDATE",where(r).c_str(),timeout_seconds)!=SQL_SUCCESS)break;
   int phase=-1;ok=Sql_NextRow(handle)==SQL_SUCCESS && matches(handle,r,0)&&state(handle,1,phase);Sql_FreeResult(handle);if(!ok)break;
   if(phase==2)break;
   if(receipt==SQL_NO_DATA) {
    SqlStmt stmt{*handle};ok=stmt.Prepare("INSERT INTO pn_shop_commits(account_id,nonce_hi,nonce_lo,sequence,outcome,payload) VALUES(%u,%llu,%llu,%llu,%u,?)",r.account_id,(unsigned long long)r.nonce_hi,(unsigned long long)r.nonce_lo,(unsigned long long)r.sequence,pn_shop::Rejected)==SQL_SUCCESS &&
      stmt.BindParam(0,SQLDT_BLOB,const_cast<pn_shop::Commit*>(&r),sizeof(r))==SQL_SUCCESS && stmt.Execute()==SQL_SUCCESS;
   }
   if(ok)ok=finish_locked(handle,r);
  }while(false);
  if(Sql_EndTransaction(handle,ok)!=SQL_SUCCESS || !ok)return false;
 }
 return true;
}
}
#endif
