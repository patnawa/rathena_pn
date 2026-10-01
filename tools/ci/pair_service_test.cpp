// Execute the production paired SQL handler against transactional fault doubles.
#include <custom/pair_commit.hpp>
#include <array>
#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>
enum {SQL_SUCCESS=0,SQL_ERROR=-1,SQL_NO_DATA=100,SQLDT_BLOB=1};
struct Sql{} handle;Sql* sql_handle=&handle;
struct {const char* char_db="char";const char* inventory_db="inventory";const char* cart_db="cart_inventory";const char* acc_reg_num_table="acc_reg_num";} schema_config;
struct Db {
    int64_t wallet[2]={0,0},bank[2]={0,0};
    int64_t buying_budget=0;
    std::array<item,pn_pair::item_capacity> inventory[2]{};
    std::vector<unsigned char> receipt;
    std::vector<pn_pair::Listing> listings;
} db,backup;
struct Cache {int64_t zeny=0;};std::map<uint32,std::shared_ptr<Cache>> cache;
auto& char_get_chardb(){return cache;}
bool transactional=true;int fail_write=0,writes=0;bool have_row=false;std::string cell;
int strcmpi(const char*a,const char*b){return std::strcmp(a,b);}
int Sql_BeginTransaction(Sql*){backup=db;writes=0;return SQL_SUCCESS;}
int Sql_EndTransaction(Sql*,bool success){if(!success)db=backup;return SQL_SUCCESS;}
int write_step(){return ++writes==fail_write?SQL_ERROR:SQL_SUCCESS;}
int Sql_Query(Sql*,const char* format,...) {
    char buffer[2048];va_list args;va_start(args,format);vsnprintf(buffer,sizeof(buffer),format,args);va_end(args);
    std::string q=buffer;have_row=false;cell.clear();
    if(q.find("SELECT ENGINE")==0){have_row=true;cell=transactional?"InnoDB":"MyISAM";return SQL_SUCCESS;}
    if(q.find("SELECT payload")==0){have_row=!db.receipt.empty();if(have_row)cell.assign(reinterpret_cast<const char*>(db.receipt.data()),db.receipt.size());return SQL_SUCCESS;}
    if(q.find("SELECT account_id")==0 || q.find("SELECT id FROM vendings")==0 || q.find("SELECT id FROM buyingstores")==0){have_row=true;cell="1";return SQL_SUCCESS;}
    if(write_step()!=SQL_SUCCESS)return SQL_ERROR;
    if(q.find("UPDATE `char`")==0) {
        long long value=0;unsigned cid=0,aid=0;
        assert(sscanf(buffer,"UPDATE `char` SET zeny=%lld WHERE char_id=%u AND account_id=%u",&value,&cid,&aid)==3);
        assert(cid==101 || cid==102);db.wallet[cid-101]=value;return SQL_SUCCESS;
    }
    if(q.find("REPLACE INTO")==0) {
        unsigned aid=0;long long value=0;auto values=q.find("VALUES (");assert(values!=std::string::npos);
        assert(sscanf(buffer+values,"VALUES (%u,'#BANKVAULT',0,%lld)",&aid,&value)==2);
        assert(aid==1 || aid==2);db.bank[aid-1]=value;return SQL_SUCCESS;
    }
    if(q.find("DELETE FROM vending_items")==0){db.listings.clear();return SQL_SUCCESS;}
    if(q.find("INSERT INTO vending_items")==0){db.listings.push_back({77,3,9007199254740993LL});return SQL_SUCCESS;}
    if(q.find("UPDATE buyingstores")==0){long long value=0;unsigned id=0;assert(sscanf(buffer,"UPDATE buyingstores SET `limit`=%lld WHERE id=%u",&value,&id)==2);db.buying_budget=value;return SQL_SUCCESS;}
    if(q.find("DELETE FROM buyingstore_items")==0){db.listings.clear();return SQL_SUCCESS;}
    if(q.find("INSERT INTO buyingstore_items")==0){
        unsigned store=0,index=0,id=0,amount=0;long long price=0;
        assert(sscanf(buffer,"INSERT INTO buyingstore_items (buyingstore_id,`index`,item_id,amount,price) VALUES (%u,%u,%u,%u,%lld)",&store,&index,&id,&amount,&price)==5);
        assert(index==1);db.listings.push_back({id,amount,price});return SQL_SUCCESS;
    }
    assert(false && "unhandled production query");return SQL_ERROR;
}
int Sql_NextRow(Sql*){return have_row?SQL_SUCCESS:SQL_NO_DATA;}
int Sql_GetData(Sql*,size_t,char** data,size_t* length){*data=cell.data();if(length)*length=cell.size();return SQL_SUCCESS;}
void Sql_FreeResult(Sql*){have_row=false;cell.clear();}
int char_memitemdata_to_sql(const item* items,int32 count,int32 cid,storage_type,uint8) {
    if(write_step()!=SQL_SUCCESS)return 1;
    assert(cid==101 || cid==102);std::copy(items,items+count,db.inventory[cid-101].begin());return 0;
}
class SqlStmt {
    const unsigned char* bytes=nullptr;size_t length=0;
public:
    explicit SqlStmt(Sql&){}
    int Prepare(const char*,...){return SQL_SUCCESS;}
    int BindParam(size_t,int,void* p,size_t n){bytes=static_cast<const unsigned char*>(p);length=n;return SQL_SUCCESS;}
    int Execute(){if(write_step()!=SQL_SUCCESS)return SQL_ERROR;db.receipt.assign(bytes,bytes+length);return SQL_SUCCESS;}
};
unsigned char incoming[65536]{},outgoing[65536]{};size_t output_size=0;
#define RFIFOW(fd,off) (*reinterpret_cast<uint16*>(incoming+(off)))
#define RFIFOP(fd,off) (incoming+(off))
#define WFIFOHEAD(fd,n) ((void)0)
#define WFIFOP(fd,off) (outgoing+(off))
#define WFIFOSET(fd,n) (output_size=(n))
#include <custom/pair_sql.inc>

pn_pair::Commit example() {
    pn_pair::Commit r;r.length=sizeof(r);r.sequence=1;
    for(unsigned i=0;i<2;++i) {
        auto& s=r.side[i];s.account_id=i+1;s.char_id=i+101;s.nonce_hi=i+10;s.nonce_lo=i+20;
        s.wallet_before=s.wallet_after=i?100:9007199254740993LL;
        s.bank_before=s.bank_after=INT64_MAX;
        s.items[0].nameid=i?502:501;s.items[0].amount=i?7:3;
    }
    r.side[0].wallet_after-=9007199254740000LL;r.side[1].wallet_after+=9007199254740000LL;
    return r;
}
void seed(const pn_pair::Commit& r) {
    db={};fail_write=0;
    for(unsigned i=0;i<2;++i){db.wallet[i]=r.side[i].wallet_before;db.bank[i]=r.side[i].bank_before;db.inventory[i][0].nameid=600+i;db.inventory[i][0].amount=9;}
}
int main() {
    auto r=example();assert(pn_pair::conserved(r));
    // Four near-INT64_MAX balances require a wide sum even though each is valid.
    auto boundary=r;for(auto& s:boundary.side)s.wallet_before=s.wallet_after=s.bank_before=s.bank_after=INT64_MAX;
    assert(pn_pair::conserved(boundary));boundary.side[0].wallet_after--;assert(!pn_pair::conserved(boundary));
    auto invalid=r;invalid.side[0].wallet_after++;assert(!pn_pair_tosql(invalid));
    invalid=r;invalid.side[1].wallet_after=-1;assert(!pn_pair_tosql(invalid));
    seed(r);transactional=false;assert(!pn_pair_tosql(r)&&db.receipt.empty());transactional=true;
    // Every write failure, including the receipt after both inventories/wallets,
    // rolls back the entire pair. A retry then applies exactly once.
    for(int failure=1;failure<=5;++failure) {
        seed(r);auto before=db;fail_write=failure;
        assert(!pn_pair_tosql(r));assert(db.receipt.empty());
        for(int i=0;i<2;++i){assert(db.wallet[i]==before.wallet[i]);assert(db.inventory[i][0].nameid==before.inventory[i][0].nameid);}
        fail_write=0;assert(pn_pair_tosql(r));
        for(int i=0;i<2;++i){assert(db.wallet[i]==r.side[i].wallet_after);assert(db.inventory[i][0].nameid==r.side[i].items[0].nameid);}
        // Lost acknowledgement: do not apply the old snapshot over later state.
        db.wallet[0]=123;db.inventory[0][0].amount=20;assert(pn_pair_tosql(r));assert(db.wallet[0]==123&&db.inventory[0][0].amount==20);
        auto tampered=r;tampered.side[0].items[0].amount++;assert(!pn_pair_tosql(tampered));assert(db.wallet[0]==123);
    }
    r=example();r.kind=pn_pair::Vending;r.fee=10;r.vending_id=77;r.listing_count=1;r.listings[0]={77,3,9007199254740993LL};
    r.side[1].bank_before=1;r.side[1].bank_after=9007199254740000LL-9;r.side[1].wallet_after=r.side[1].wallet_before;
    assert(pn_pair::conserved(r));
    for(int failure=1;failure<=8;++failure) {
        seed(r);db.listings.push_back({77,5,9007199254740993LL});fail_write=failure;
        assert(!pn_pair_tosql(r));assert(db.bank[1]==1&&db.listings[0].amount==5&&db.receipt.empty());
        fail_write=0;assert(pn_pair_tosql(r));assert(db.bank[1]==r.side[1].bank_after&&db.listings.size()==1&&db.listings[0].amount==3);
    }
    r=example();r.kind=pn_pair::Buying;r.vending_id=88;r.listing_count=2;r.buying_budget=9007199254740000LL;
    r.listings[0]={501,0,9007199254740993LL};r.listings[1]={502,3,9007199254740993LL};
    assert(pn_pair::conserved(r));
    for(int failure=1;failure<=8;++failure) {
        seed(r);db.buying_budget=INT64_MAX;db.listings.push_back({501,1,9007199254740993LL});fail_write=failure;
        assert(!pn_pair_tosql(r));assert(db.buying_budget==INT64_MAX&&db.listings[0].cart_id==501&&db.receipt.empty());
        for(int i=0;i<2;++i){assert(db.wallet[i]==r.side[i].wallet_before);assert(db.inventory[i][0].amount==9);}
        fail_write=0;assert(pn_pair_tosql(r));
        assert(db.buying_budget==r.buying_budget&&db.listings.size()==1&&db.listings[0].cart_id==502);
        db.buying_budget=123;db.listings.clear();assert(pn_pair_tosql(r));assert(db.buying_budget==123&&db.listings.empty());
        auto changed=r;++changed.buying_budget;assert(!pn_pair_tosql(changed));
    }
    seed(r);memcpy(incoming,&r,sizeof(r));mapif_parse_PairCommit(1);pn_pair::Ack ack;memcpy(&ack,outgoing,sizeof(ack));
    assert(output_size==sizeof(ack)&&ack.committed==1&&ack.sequence==r.sequence&&ack.nonce_hi==r.side[0].nonce_hi);
    std::cout<<"PASS: production paired SQL conservation, every write-fault rollback, exact retry, changed-payload rejection, stale-snapshot protection, vending bank/cart/listing atomicity, buying inventories/budget/sparse-listings atomicity and acknowledgement ABI\n";
}
