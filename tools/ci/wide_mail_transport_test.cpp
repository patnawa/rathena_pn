#include "../../client-patch/wide_mail/mail_transport.cpp"
#include <cassert>
#include <thread>
#include <iostream>
namespace fixture {BankSession session;bool active=true;int replies=0;MailResult last;}
bool bank_session_snapshot(BankSession& session){session=fixture::session;return fixture::active;}
bool bank_current_generation(LONG generation,bool){return fixture::active&&generation==fixture::session.generation;}
LRESULT CALLBACK proc(HWND hwnd,UINT message,WPARAM w,LPARAM l) {
    if(message==MAIL_RESULT){auto result=reinterpret_cast<MailResult*>(l);fixture::last=*result;delete result;++fixture::replies;return 0;}
    return DefWindowProc(hwnd,message,w,l);
}
void pump(int target){auto deadline=GetTickCount64()+5000;while(fixture::replies<target&&GetTickCount64()<deadline){MSG m;while(PeekMessage(&m,nullptr,0,0,PM_REMOVE))DispatchMessage(&m);Sleep(1);}assert(fixture::replies==target);}
int main() {
    WSADATA ws;assert(!WSAStartup(MAKEWORD(2,2),&ws));SOCKET listener=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    sockaddr_in address{};address.sin_family=AF_INET;address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);assert(!bind(listener,reinterpret_cast<sockaddr*>(&address),sizeof(address))&&!listen(listener,2));int size=sizeof(address);getsockname(listener,reinterpret_cast<sockaddr*>(&address),&size);
    fixture::session={100,200,300,400,address,7};WNDCLASSW type{};type.lpfnWndProc=proc;type.hInstance=GetModuleHandle(nullptr);type.lpszClassName=L"PNMailTransportTest";RegisterClassW(&type);
    HWND window=CreateWindowW(type.lpszClassName,L"",0,0,0,0,0,HWND_MESSAGE,nullptr,type.hInstance,nullptr);SOCKET peer=INVALID_SOCKET;
    auto backend=[&](bool wrong_character) {
        if(peer==INVALID_SOCKET)peer=accept(listener,nullptr,nullptr);
        pn_mail::Request request;size_t received=0;
        while(received<sizeof(request)){int count=recv(peer,reinterpret_cast<char*>(&request)+received,sizeof(request)-received,0);assert(count>0);received+=count;}
        assert(request.length==636&&request.account_id==100&&request.login_id1==300&&request.action==pn_mail::Send&&request.amount==9007199254740993LL&&request.expected_total==9097271247288402LL);
        pn_mail::Reply reply;reply.result=pn_mail::Pending;reply.pending=1;reply.nonce_hi=11;reply.nonce_lo=22;reply.char_id=wrong_character?999:200;reply.wallet=INT64_MAX;reply.amount=request.amount;reply.total=request.expected_total;reply.fee=reply.total-reply.amount;
        assert(send(peer,reinterpret_cast<char*>(&reply),7,0)==7);assert(send(peer,reinterpret_cast<char*>(&reply)+7,sizeof(reply)-7,0)==sizeof(reply)-7);
    };
    pn_mail::Request request;request.action=pn_mail::Send;request.nonce_hi=11;request.nonce_lo=22;request.amount=9007199254740993LL;request.expected_total=9097271247288402LL;std::strcpy(request.recipient,"Fixture Recipient");std::strcpy(request.title,"Exact transfer");
    std::thread good(backend,false);assert(mail_submit(window,request));pump(1);good.join();assert(fixture::last.connected&&fixture::last.state.total==9097271247288402LL);
    std::thread stale(backend,true);assert(mail_submit(window,request));pump(2);stale.join();assert(!fixture::last.connected);
    fixture::active=false;assert(!mail_submit(window,request));mail_forget_session();closesocket(peer);closesocket(listener);DestroyWindow(window);WSACleanup();
    std::cout<<"PASS: exact PZL1 authenticated send framing, fragmented reply, total above2^53, character binding and logout rejection\n";
}
