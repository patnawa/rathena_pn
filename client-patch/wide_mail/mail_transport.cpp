// PZL1 companion shares the existing DLL's session observer, never its socket.
#include "mail_client.hpp"
#include <cstring>

namespace {
SRWLOCK exchange_lock=SRWLOCK_INIT;
SOCKET mail_socket=INVALID_SOCKET;
LONG socket_generation=0;
struct Work {HWND panel;BankSession session;pn_mail::Request request;};
void disconnect() {
    if(mail_socket!=INVALID_SOCKET)closesocket(mail_socket);
    mail_socket=INVALID_SOCKET;socket_generation=0;
}
bool exchange(const Work& work,pn_mail::Reply& reply) {
    AcquireSRWLockExclusive(&exchange_lock);
    bool success=false;
    do {
        if(!bank_current_generation(work.session.generation))break;
        if(mail_socket!=INVALID_SOCKET && socket_generation!=work.session.generation)disconnect();
        if(mail_socket==INVALID_SOCKET) {
            mail_socket=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
            if(mail_socket==INVALID_SOCKET)break;
            socket_generation=work.session.generation;
            u_long nonblock=1;ioctlsocket(mail_socket,FIONBIO,&nonblock);
            if(connect(mail_socket,reinterpret_cast<const sockaddr*>(&work.session.peer),sizeof(work.session.peer))==SOCKET_ERROR && WSAGetLastError()!=WSAEWOULDBLOCK)break;
            fd_set writable;FD_ZERO(&writable);FD_SET(mail_socket,&writable);timeval limit{5,0};
            if(select(0,nullptr,&writable,nullptr,&limit)<=0)break;
            int error=0;int length=sizeof(error);
            if(getsockopt(mail_socket,SOL_SOCKET,SO_ERROR,reinterpret_cast<char*>(&error),&length)||error)break;
            nonblock=0;ioctlsocket(mail_socket,FIONBIO,&nonblock);
            DWORD timeout=5000;setsockopt(mail_socket,SOL_SOCKET,SO_SNDTIMEO,reinterpret_cast<char*>(&timeout),sizeof(timeout));
            setsockopt(mail_socket,SOL_SOCKET,SO_RCVTIMEO,reinterpret_cast<char*>(&timeout),sizeof(timeout));
        }
        size_t sent=0;
        while(sent<sizeof(work.request)) {
            int count=send(mail_socket,reinterpret_cast<const char*>(&work.request)+sent,sizeof(work.request)-sent,0);
            if(count<=0)break;
            sent+=count;
        }
        if(sent!=sizeof(work.request))break;
        const auto deadline=GetTickCount64()+5000;size_t received=0;
        while(received<sizeof(reply)) {
            auto now=GetTickCount64();if(now>=deadline)break;
            auto remaining=deadline-now;timeval timeout{static_cast<long>(remaining/1000),static_cast<long>((remaining%1000)*1000)};
            fd_set readable;FD_ZERO(&readable);FD_SET(mail_socket,&readable);
            if(select(0,&readable,nullptr,nullptr,&timeout)<=0)break;
            int count=recv(mail_socket,reinterpret_cast<char*>(&reply)+received,sizeof(reply)-received,0);
            if(count<=0)break;
            received+=count;
        }
        success=received==sizeof(reply) && valid_mail_reply(reply) && bank_current_generation(work.session.generation);
        if(success&&reply.result!=pn_mail::Denied)
            success=reply.nonce_hi==work.request.nonce_hi&&reply.nonce_lo==work.request.nonce_lo&&reply.char_id==work.session.character;
    }while(false);
    if(!success)disconnect();
    ReleaseSRWLockExclusive(&exchange_lock);return success;
}
DWORD WINAPI worker(void* argument) {
    auto work=static_cast<Work*>(argument);auto result=new MailResult;
    result->generation=work->session.generation;result->connected=exchange(*work,result->state);
    if(!PostMessage(work->panel,MAIL_RESULT,0,reinterpret_cast<LPARAM>(result)))delete result;
    SecureZeroMemory(&work->session,sizeof(work->session));SecureZeroMemory(&work->request,sizeof(work->request));delete work;return 0;
}
}
bool mail_submit(HWND panel,pn_mail::Request request) {
    auto work=new Work;work->panel=panel;
    if(!bank_session_snapshot(work->session)){delete work;return false;}
    request.account_id=work->session.account;request.char_id=work->session.character;
    request.login_id1=work->session.login_one;request.login_id2=work->session.login_two;
    work->request=request;
    HANDLE thread=CreateThread(nullptr,0,worker,work,0,nullptr);
    if(!thread){SecureZeroMemory(work,sizeof(*work));delete work;return false;}
    CloseHandle(thread);return true;
}
void mail_forget_session() {
    if(TryAcquireSRWLockExclusive(&exchange_lock)){disconnect();ReleaseSRWLockExclusive(&exchange_lock);}
}
