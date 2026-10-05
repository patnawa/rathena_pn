#include <custom/shop_commit.hpp>
#include <cassert>
#include <iostream>
#include <vector>
using int32=int32_t;
namespace pn_global_point {constexpr uint16_t ack_packet=0x2745;}
static std::vector<unsigned char> inbound(65536),outbound(65536);
static size_t available=0,consumed=0,sent=0;static bool eof=false,allow=true,has_owner=true;static int approvals=0;
struct Online {int char_server=2;};static Online online;
static pn_shop::Commit captured;
#define RFIFOREST(fd) (available)
#define RFIFOW(fd,n) (*reinterpret_cast<uint16_t*>(inbound.data()+(n)))
#define RFIFOP(fd,n) (inbound.data()+(n))
#define RFIFOSKIP(fd,n) (consumed+=(n),available-=(n))
#define WFIFOHEAD(fd,n) ((void)0)
#define WFIFOP(fd,n) (outbound.data()+(n))
#define WFIFOSET(fd,n) (sent=(n))
static void set_eof(int){eof=true;}
static Online* login_get_online_user(uint32_t account){return has_owner&&account==7?&online:nullptr;}
static void* login_get_accounts_db(){return nullptr;}
static bool mmo_point_barrier(void*,const pn_shop::Commit& r){++approvals;captured=r;return allow;}
#include "point_barrier_body.inc"
int main(){
 pn_shop::Commit r;r.account_id=7;r.char_id=8;r.nonce_hi=9;r.nonce_lo=10;r.sequence=11;
 const size_t size=4+sizeof(r);
 for(int test=0;test<8;++test){
  available=size;consumed=sent=approvals=0;eof=false;allow=true;has_owner=true;online.char_server=2;
  RFIFOW(0,2)=size;memcpy(RFIFOP(0,4),&r,sizeof(r));
  if(test==0)available=3;
  if(test==1)RFIFOW(0,2)--;
  if(test==2)available--;
  if(test==3)has_owner=false;
  if(test==4)online.char_server=3;
  if(test==5)allow=false;
  if(test<3){assert(!logchrif_point_barrier(0,2));assert(!consumed&&!sent&&!approvals);assert(eof==(test==1));continue;}
  assert(logchrif_point_barrier(0,2)==1);assert(consumed==size&&sent==sizeof(pn_shop::Ack));
  pn_shop::Ack ack;memcpy(&ack,outbound.data(),sizeof(ack));
  assert(ack.packet==pn_global_point::ack_packet&&ack.account_id==7&&ack.char_id==8&&ack.nonce_hi==9&&ack.nonce_lo==10&&ack.sequence==11);
  assert(ack.outcome==unsigned(test>=6));assert(approvals==int(test>=5));
  if(approvals)assert(!memcmp(&captured,&r,sizeof(r)));
 }
 std::cout<<"POINT_BARRIER_WIRE_PASS cases=8; exact login FIFO handler, explicit ownership/SQL doubles (no TCP)\n";
}
