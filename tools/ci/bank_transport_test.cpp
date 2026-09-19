// Windows integration proof using the production hook/transport and loopback only.
#include "../../client-patch/account_bank/bank_transport.cpp"
#include <cassert>
#include <thread>
#include <iostream>

static int opened=0, replies=0;
static BankResult last;
static LRESULT CALLBACK test_proc(HWND h,UINT m,WPARAM w,LPARAM l) {
    if(m==BANK_REMOTE_OPEN) { if(bank_current_generation(static_cast<LONG>(w))) ++opened; return 0; }
    if(m==BANK_RESULT) { auto p=reinterpret_cast<BankResult*>(l); last=*p; delete p; ++replies; return 0; }
    return DefWindowProc(h,m,w,l);
}
static void pump_until(int& value,int wanted) {
    auto deadline=GetTickCount64()+5000;
    while(value<wanted && GetTickCount64()<deadline) {
        MSG m; while(PeekMessage(&m,nullptr,0,0,PM_REMOVE)) { TranslateMessage(&m); DispatchMessage(&m); }
        MsgWaitForMultipleObjects(0,nullptr,FALSE,10,QS_ALLINPUT);
    }
    assert(value>=wanted);
}
static SOCKET listener(uint16_t& port) {
    SOCKET s=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP); assert(s!=INVALID_SOCKET);
    sockaddr_in address{}; address.sin_family=AF_INET; address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    assert(bind(s,reinterpret_cast<sockaddr*>(&address),sizeof(address))==0 && listen(s,4)==0);
    int size=sizeof(address); assert(getsockname(s,reinterpret_cast<sockaddr*>(&address),&size)==0);
    port=ntohs(address.sin_port); return s;
}
static SOCKET connected(uint16_t port) {
    SOCKET s=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    sockaddr_in address{}; address.sin_family=AF_INET; address.sin_addr.s_addr=htonl(INADDR_LOOPBACK); address.sin_port=htons(port);
    assert(connect(s,reinterpret_cast<sockaddr*>(&address),sizeof(address))==0); return s;
}
static void read_exact(SOCKET s,char* data,int size) {
    while(size) { int n=recv(s,data,size,0); assert(n>0); data+=n; size-=n; }
}
int main() {
    WSADATA ws; assert(WSAStartup(MAKEWORD(2,2),&ws)==0);
    WNDCLASSW cls{}; cls.lpfnWndProc=test_proc; cls.hInstance=GetModuleHandle(nullptr); cls.lpszClassName=L"PNBankTransportTest";
    RegisterClassW(&cls); HWND window=CreateWindowW(cls.lpszClassName,L"",0,0,0,0,0,HWND_MESSAGE,nullptr,cls.hInstance,nullptr);
    bank_install_transport(window); assert(game_send && game_close);
    SOCKET chars=listener(char_port), maps=listener(map_port);
    SOCKET cc=connected(char_port), cs=accept(chars,nullptr,nullptr);
    char character[17]={0x65,0}; uint32_t account=2000001, one=314159, two=271828, id=150001;
    memcpy(character+2,&account,4); memcpy(character+6,&one,4); memcpy(character+10,&two,4);
    auto symbol=GetProcAddress(GetModuleHandleW(L"ws2_32.dll"),"send");
    SendFn dynamic; static_assert(sizeof(dynamic)==sizeof(symbol)); memcpy(&dynamic,&symbol,sizeof(dynamic));
    assert(dynamic(cc,character,2,0)==2 && dynamic(cc,character+2,15,0)==15);
    char received[64]{}; read_exact(cs,received,17); assert(memcmp(received,character,17)==0 && !bank_authenticated());
    SOCKET mc=connected(map_port), ms=accept(maps,nullptr,nullptr);
    char map[19]={0x36,0x04}; memcpy(map+2,&account,4); memcpy(map+6,&id,4); memcpy(map+10,&one,4);
    assert(dynamic(mc,map,5,0)==5 && dynamic(mc,map+5,14,0)==14);
    read_exact(ms,received,19); assert(memcmp(map,received,19)==0 && bank_authenticated());
    const char chat[]="Fixture : @bank";
    assert(dynamic(mc,chat,11,0)==11 && dynamic(mc,chat+11,sizeof(chat)-11,0)==sizeof(chat)-11);
    read_exact(ms,received,sizeof(chat)); assert(memcmp(chat,received,sizeof(chat))==0 && opened==0);
    pn_bank::Reply snapshot; snapshot.nonce_hi=123; snapshot.nonce_lo=456;
    SOCKET bank_peer=INVALID_SOCKET;
    auto backend=[&](bool truncate,bool push) {
        if(bank_peer==INVALID_SOCKET) bank_peer=accept(maps,nullptr,nullptr);
        SOCKET s=bank_peer; pn_bank::Request r;
        read_exact(s,reinterpret_cast<char*>(&r),sizeof(r));
        assert(r.account_id==account && r.char_id==id && r.login_id1==one && r.login_id2==two);
        assert(r.nonce_hi==123 && r.nonce_lo==456 && r.amount==3 && r.action==pn_bank::BuyDiamond && r.request_id==1);
        pn_bank::Reply response; response.char_id=id; response.result=pn_bank::Saving;
        response.bank=INT64_MAX;response.wallet=INT32_MAX;
        if(push) {
            response.flags=pn_bank::open_panel;
            assert(game_send(s,reinterpret_cast<char*>(&response),sizeof(response),0)==sizeof(response));
            response.flags=0;
        }
        assert(game_send(s,reinterpret_cast<char*>(&response),2,0)==2);
        if(!truncate) assert(game_send(s,reinterpret_cast<char*>(&response)+2,sizeof(response)-2,0)==sizeof(response)-2);
        if(truncate) { game_close(s); bank_peer=INVALID_SOCKET; }
    };
    std::thread good(backend,false,true);
    assert(bank_submit(window,snapshot,pn_bank::BuyDiamond,3,1)); pump_until(replies,1); good.join();
    assert(last.connected && last.state.result==pn_bank::Saving && !last.state.flags && bank_current_generation(last.generation));
    assert(last.state.bank==INT64_MAX && last.state.wallet==INT32_MAX);pump_until(opened,1);
    assert(bank_connection_ready());
    assert(PNGameInputReady()); // Successful character/companion handshake gates turbo.
    pn_bank::Reply push;push.char_id=id;push.result=pn_bank::Ok;push.flags=pn_bank::open_panel;
    assert(game_send(bank_peer,reinterpret_cast<char*>(&push),7,0)==7);
    assert(game_send(bank_peer,reinterpret_cast<char*>(&push)+7,sizeof(push)-7,0)==sizeof(push)-7);
    pump_until(opened,2);assert(replies==1); // Idle pushes do not complete an action.
    std::thread broken(backend,true,false);
    assert(bank_submit(window,snapshot,pn_bank::BuyDiamond,3,1)); pump_until(replies,2); broken.join();
    assert(!last.connected && bank_authenticated());
    assert(!bank_connection_ready());
    assert(!PNGameInputReady());
    closesocket(mc); assert(!bank_authenticated() && !bank_current_generation(last.generation));
    assert(!notify_open(window,push,last.generation));
    assert(!bank_submit(window,snapshot,pn_bank::Deposit,1,2));
    for(auto s:{cc,cs,ms,chars,maps}) closesocket(s);
    DestroyWindow(window); WSACleanup();
    std::cout<<"PASS: real Windows API hooks, fragmented authentication, unmodified game bytes, idle and interleaved open notifications, exact 64-bit balances, authenticated companion exchange, short reply rejection, logout and stale-session rejection\n";
}
