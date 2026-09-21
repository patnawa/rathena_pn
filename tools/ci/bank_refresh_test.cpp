// Shipping panel + shipping socket hooks, using an isolated loopback bank.
// Delayed replies exercise the real timer/worker/message delivery race.
#define PN_BANK_UI_TEST
#include "../../client-patch/account_bank/bank_ui.cpp"
#include "../../client-patch/account_bank/bank_transport.cpp"
#include <atomic>
#include <iostream>
#include <thread>

template<class Predicate> static void pump_until(Predicate ready) {
    const auto deadline=GetTickCount64()+7000;
    do {
        MSG message;
        while(PeekMessage(&message,nullptr,0,0,PM_REMOVE)) { TranslateMessage(&message); DispatchMessage(&message); }
        if(ready()) return;
        MsgWaitForMultipleObjects(0,nullptr,FALSE,10,QS_ALLINPUT);
    } while(GetTickCount64()<deadline);
    assert(false && "Loopback bank timed out");
}
static SOCKET listener(uint16_t& port) {
    SOCKET socket=::socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);assert(socket!=INVALID_SOCKET);
    sockaddr_in address{};address.sin_family=AF_INET;address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    assert(!bind(socket,reinterpret_cast<sockaddr*>(&address),sizeof(address)) && !listen(socket,4));
    int size=sizeof(address);assert(!getsockname(socket,reinterpret_cast<sockaddr*>(&address),&size));
    port=ntohs(address.sin_port);return socket;
}
static SOCKET connect_to(uint16_t port) {
    SOCKET socket=::socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    sockaddr_in address{};address.sin_family=AF_INET;address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);address.sin_port=htons(port);
    assert(!connect(socket,reinterpret_cast<sockaddr*>(&address),sizeof(address)));return socket;
}
static void read_exact(SOCKET socket,void* target,int size) {
    auto bytes=static_cast<char*>(target);
    while(size) { const int count=recv(socket,bytes,size,0);assert(count>0);bytes+=count;size-=count; }
}
static void send_reply(SOCKET socket,const pn_bank::Reply& reply) {
    assert(pn_bank::valid_reply(reply));
    auto bytes=reinterpret_cast<const char*>(&reply);int remaining=sizeof(reply);
    while(remaining) {
        const int count=game_send(socket,bytes,std::min(remaining,17),0);assert(count>0);bytes+=count;remaining-=count;
    }
}
int main() {
    WSADATA ws;assert(!WSAStartup(MAKEWORD(2,2),&ws));instance=GetModuleHandle(nullptr);
    WNDCLASSW type{};type.lpfnWndProc=window_proc;type.hInstance=instance;type.lpszClassName=L"PNAccountBank";
    assert(RegisterClassW(&type));
    panel=CreateWindowExW(WS_EX_TOOLWINDOW,type.lpszClassName,L"",WS_POPUP|WS_BORDER|WS_CLIPCHILDREN,0,0,panel_width+2,panel_height+2,nullptr,nullptr,instance,nullptr);
    assert(panel);bank_install_transport(panel);assert(game_send && game_close);
    SOCKET chars=listener(char_port),maps=listener(map_port);
    SOCKET cc=connect_to(char_port),cs=accept(chars,nullptr,nullptr);
    const uint32_t account=2000001,one=314159,two=271828,character=150001;
    char login[17]={0x65,0};memcpy(login+2,&account,4);memcpy(login+6,&one,4);memcpy(login+10,&two,4);
    assert(send(cc,login,sizeof(login),0)==sizeof(login));char scratch[19];read_exact(cs,scratch,sizeof(login));
    SOCKET mc=connect_to(map_port),ms=accept(maps,nullptr,nullptr);
    char enter[19]={0x36,0x04};memcpy(enter+2,&account,4);memcpy(enter+6,&character,4);memcpy(enter+10,&one,4);
    assert(send(mc,enter,sizeof(enter),0)==sizeof(enter));read_exact(ms,scratch,sizeof(enter));assert(bank_authenticated());
    pn_bank::Reply snapshot;snapshot.result=pn_bank::Ok;snapshot.char_id=character;
    snapshot.nonce_hi=123;snapshot.nonce_lo=456;snapshot.bank=2000000000;snapshot.wallet=1000000;
    snapshot.max_deposit=snapshot.wallet;snapshot.max_withdraw=snapshot.bank;
    for(int i=0;i<2;++i) { snapshot.counts[i]=snapshot.max_sell[i]=10;snapshot.max_buy[i]=snapshot.bank/snapshot.buy[i]; }
    SOCKET peer=INVALID_SOCKET;std::atomic<int> financial{0};std::atomic<bool> received{false};
    HANDLE gate=CreateEvent(nullptr,TRUE,FALSE,nullptr);assert(gate);
    auto backend=[&](uint32_t action,pn_bank::Reply response,bool delayed) {
        if(peer==INVALID_SOCKET) peer=accept(maps,nullptr,nullptr);
        pn_bank::Request request;read_exact(peer,&request,sizeof(request));
        assert(request.action==action && request.account_id==account && request.char_id==character);
        assert(request.login_id1==one && request.login_id2==two);
        if(action!=pn_bank::Refresh) {
            ++financial;assert(request.amount==1 && request.request_id==1);
            assert(request.nonce_hi==123 && request.nonce_lo==456);
        } else assert(!request.amount && !request.request_id);
        received=true;
        if(delayed) assert(WaitForSingleObject(gate,7000)==WAIT_OBJECT_0);
        send_reply(peer,response);
    };
    std::thread initial(backend,pn_bank::Refresh,snapshot,false);
    pump_until([] { return verified && !busy; });initial.join();
    for(int i=2;i<6;++i) assert(IsWindowEnabled(actions[i]));
    select_action(pn_bank::BuyNote);
    // Timer request is genuinely in flight when the native Buy click arrives.
    received=false;std::thread refresh(backend,pn_bank::Refresh,snapshot,true);
    last_refresh=0;SendMessage(panel,WM_TIMER,1,0);pump_until([&] { return received.load(); });
    assert(busy && refreshing && IsWindowEnabled(actions[4]));
    SendMessage(actions[4],BM_CLICK,0,0);assert(queued_action==pn_bank::BuyNote && financial==0);
    SendMessage(actions[4],BM_CLICK,0,0);SendMessage(panel,WM_COMMAND,404,0);
    set_amount(2,2);assert(queued_amount==1); // Preserve the actual clicked quantity.
    SetEvent(gate);refresh.join();
    auto saving=snapshot;saving.result=pn_bank::Saving;saving.request_id=1;
    std::thread purchase(backend,pn_bank::BuyNote,saving,false);
    pump_until([] { return state.result==pn_bank::Saving; });purchase.join();
    assert(financial==1 && queued_action==pn_bank::Refresh);
    assert(receipt.empty());
    for(auto control:actions) assert(!IsWindowEnabled(control));
    snapshot.bank-=snapshot.buy[1];snapshot.max_withdraw=snapshot.bank;++snapshot.counts[1];++snapshot.max_sell[1];snapshot.request_id=1;
    for(int i=0;i<2;++i) snapshot.max_buy[i]=snapshot.bank/snapshot.buy[i];
    std::thread committed(backend,pn_bank::Refresh,snapshot,false);
    submit(pn_bank::Refresh);pump_until([&] { return !busy && state.result==pn_bank::Ok && state.bank==snapshot.bank; });committed.join();
    assert(financial==1 && state.counts[1]==11 && IsWindowEnabled(actions[4]) && IsWindowEnabled(actions[5]));
    assert(receipt.find(L"Saved: Bought 1 Ticket.")==0);
    select_action(pn_bank::SellNote);
    // A logout between an explicit click and its balance reply cancels it.
    received=false;ResetEvent(gate);std::thread logout_refresh(backend,pn_bank::Refresh,snapshot,true);
    submit(pn_bank::Refresh);pump_until([&] { return received.load(); });
    SendMessage(actions[5],BM_CLICK,0,0);assert(queued_action==pn_bank::SellNote);
    ShowWindow(panel,SW_SHOWNOACTIVATE);assert(IsWindowVisible(panel));
    closesocket(mc);pump_until([] { return !verified && !busy && queued_action==pn_bank::Refresh; });
    assert(!IsWindowVisible(panel));
    SetEvent(gate);logout_refresh.join();pump_until([] { return !bank_connection_ready(); });
    assert(financial==1 && !verified && queued_action==pn_bank::Refresh);
    assert(!IsWindowVisible(panel));
    for(auto control:actions) assert(!IsWindowEnabled(control));
    for(auto socket:{peer,cc,cs,ms,chars,maps}) closesocket(socket);
    CloseHandle(gate);DestroyWindow(panel);
    std::cout<<"PASS: shipping UI + real hooked authentication + delayed fragmented loopback replies; timer refresh keeps Buy enabled; native click queues once and preserves quantity; pending save blocks actions; committed purchase restores Buy/Sell; logout cancels queued sale and stale reply\n";
}
