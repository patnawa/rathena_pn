#include "../../client-patch/wide_market/market_transport.cpp"
#include <cassert>
#include <thread>
#include <iostream>
namespace fixture {BankSession session;bool active=true;int replies=0;MarketResult last;}
bool bank_session_snapshot(BankSession& session){session=fixture::session;return fixture::active;}
bool bank_current_generation(LONG generation,bool){return fixture::active&&generation==fixture::session.generation;}
LRESULT CALLBACK proc(HWND hwnd,UINT message,WPARAM w,LPARAM l) {
    if(message==MARKET_RESULT){auto result=reinterpret_cast<MarketResult*>(l);fixture::last=*result;delete result;++fixture::replies;return 0;}
    return DefWindowProc(hwnd,message,w,l);
}
void pump(int target){auto deadline=GetTickCount64()+5000;while(fixture::replies<target&&GetTickCount64()<deadline){MSG m;while(PeekMessage(&m,nullptr,0,0,PM_REMOVE))DispatchMessage(&m);Sleep(1);}assert(fixture::replies==target);}
int main() {
    WSADATA ws;assert(!WSAStartup(MAKEWORD(2,2),&ws));
    SOCKET listener=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);sockaddr_in address{};address.sin_family=AF_INET;address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    assert(!bind(listener,reinterpret_cast<sockaddr*>(&address),sizeof(address))&&!listen(listener,2));int size=sizeof(address);getsockname(listener,reinterpret_cast<sockaddr*>(&address),&size);
    fixture::session={100,200,300,400,address,7};
    WNDCLASSW type{};type.lpfnWndProc=proc;type.hInstance=GetModuleHandle(nullptr);type.lpszClassName=L"PNMarketTransportTest";RegisterClassW(&type);
    HWND window=CreateWindowW(type.lpszClassName,L"",0,0,0,0,0,HWND_MESSAGE,nullptr,type.hInstance,nullptr);
    SOCKET peer=INVALID_SOCKET;
    auto backend=[&](int fault) {
        if(peer==INVALID_SOCKET)peer=accept(listener,nullptr,nullptr);
        pn_market::Request request;size_t received=0;
        while(received<sizeof(request)){int count=recv(peer,reinterpret_cast<char*>(&request)+received,sizeof(request)-received,0);assert(count>0);received+=count;}
        assert(request.length==128&&request.account_id==100&&request.login_id1==300&&request.action==pn_market::Purchase);
        assert((request.expected_price==9007199254740993LL||request.expected_price==INT64_MAX)&&request.quantity==3&&request.revision==17);
        pn_market::Reply reply;reply.result=pn_market::Saving;reply.request_id=request.request_id+(fault==2);reply.nonce_hi=fault==1?99:11;reply.nonce_lo=22;reply.wallet=INT64_MAX;reply.count=1;
        reply.entries[0].item_id=501;reply.entries[0].quantity=3;reply.entries[0].price=request.expected_price;
        assert(send(peer,reinterpret_cast<char*>(&reply),7,0)==7);
        assert(send(peer,reinterpret_cast<char*>(&reply)+7,sizeof(reply)-7,0)==sizeof(reply)-7);
    };
    pn_market::Request request;request.action=pn_market::Purchase;request.nonce_hi=11;request.nonce_lo=22;request.quantity=3;request.expected_price=9007199254740993LL;request.revision=17;
    std::thread first(backend,0);assert(market_submit(window,request));pump(1);first.join();
    assert(fixture::last.connected&&fixture::last.state.result==pn_market::Saving&&fixture::last.state.wallet==INT64_MAX&&fixture::last.state.entries[0].price==9007199254740993LL);
    for(int i=0;i<300;++i){
        request.expected_price=INT64_MAX;
        const int target=fixture::replies+1;
        std::thread repeated(backend,0);assert(market_submit(window,request));pump(target);repeated.join();
        assert(fixture::last.connected&&fixture::last.state.entries[0].price==INT64_MAX);
    }
    const int completed=fixture::replies;
    std::thread stale(backend,1);assert(market_submit(window,request));pump(completed+1);stale.join();assert(!fixture::last.connected);
    closesocket(peer);peer=INVALID_SOCKET;
    std::thread wrong_id(backend,2);assert(market_submit(window,request));pump(completed+2);wrong_id.join();assert(!fixture::last.connected);
    fixture::active=false;assert(!market_submit(window,request));
    market_forget_session();closesocket(peer);closesocket(listener);DestroyWindow(window);WSACleanup();
    std::cout<<"PASS: authenticated PMK1 framing, 300 exact INT64_MAX exchanges, fragmented catalog reply, persistent socket, stale nonce and logout rejection\n";
}
