// Observe the game's existing authentication; never change game packet bytes.
// The companion connection uses the same game server and existing login tokens.
#include "bank_client.hpp"
#include "vendor/MinHook/include/MinHook.h"
#include <algorithm>
#include <array>
#include <cstring>
#include <vector>

namespace {
using SendFn = int (WSAAPI*)(SOCKET,const char*,int,int);
using CloseFn = int (WSAAPI*)(SOCKET);
SendFn game_send = nullptr; CloseFn game_close = nullptr;
CRITICAL_SECTION lock;
CRITICAL_SECTION exchange_lock;
SOCKET bank_socket=INVALID_SOCKET;
LONG bank_generation=0;
HWND panel_window = nullptr;
volatile LONG transport_initialized=0;
volatile LONG accepted_generation=0;
uint16_t char_port = 6121, map_port = 5121;
struct Auth { uint32_t account=0, character=0, one=0, two=0; sockaddr_in peer{}; SOCKET socket=INVALID_SOCKET; LONG generation=0; bool ready=false; } auth;
struct Capture { SOCKET socket=INVALID_SOCKET; std::vector<char> start; bool initial=false; };
std::array<Capture, 16> captures;
uint32_t u32(const char* p) { uint32_t value; memcpy(&value,p,4); return value; }
uint16_t u16(const char* p) { uint16_t value; memcpy(&value,p,2); return value; }

void capture_send(SOCKET socket, const char* data, int length) {
    sockaddr_in peer{}; int peer_size=sizeof(peer);
    if (getpeername(socket,reinterpret_cast<sockaddr*>(&peer),&peer_size) || peer.sin_family!=AF_INET) return;
    uint16_t port=ntohs(peer.sin_port);
    if (port!=char_port && port!=map_port) return;
    EnterCriticalSection(&lock);
    auto found=std::find_if(captures.begin(),captures.end(),[&](const Capture& c){return c.socket==socket;});
    if(found==captures.end()) found=std::find_if(captures.begin(),captures.end(),[](const Capture& c){return c.socket==INVALID_SOCKET;});
    if(found==captures.end()) { LeaveCriticalSection(&lock); return; }
    auto& c=*found; c.socket=socket;
    if(!c.initial) {
        int copy=std::min<int>(length,32-static_cast<int>(c.start.size()));
        c.start.insert(c.start.end(),data,data+copy);
        if(port==char_port && c.start.size()>=17) {
            c.initial=true;
            if(u16(c.start.data())==0x0065) {
                auth.account=u32(c.start.data()+2); auth.one=u32(c.start.data()+6); auth.two=u32(c.start.data()+10);
                auth.ready=false; ++auth.generation;
                PostMessage(panel_window,BANK_SESSION,static_cast<WPARAM>(auth.generation),0);
            }
        }
        if(port==map_port && c.start.size()>=19) {
            c.initial=true;
            // 2026 map authentication has fixed field offsets even if the
            // packet's two-byte opcode is obfuscated.
            if(auth.account && u32(c.start.data()+2)==auth.account && u32(c.start.data()+10)==auth.one) {
                auth.character=u32(c.start.data()+6); auth.peer=peer; auth.socket=socket;
                auth.ready=true; ++auth.generation;
                PostMessage(panel_window,BANK_SESSION,static_cast<WPARAM>(auth.generation),0);
            }
        }
    }
    LeaveCriticalSection(&lock);
}
int WSAAPI observed_send(SOCKET socket,const char* data,int length,int flags) {
    int result=game_send(socket,data,length,flags);
    int error=WSAGetLastError();
    if(result>0) capture_send(socket,data,result);
    WSASetLastError(error); return result;
}
int WSAAPI observed_close(SOCKET socket) {
    EnterCriticalSection(&lock);
    for(auto& c:captures) if(c.socket==socket) c=Capture{};
    if(auth.socket==socket) { auth.ready=false; auth.socket=INVALID_SOCKET; ++auth.generation; PostMessage(panel_window,BANK_SESSION,static_cast<WPARAM>(auth.generation),0); }
    LeaveCriticalSection(&lock);
    // Never block the game thread behind a network receive. An active worker
    // closes a stale companion itself; an idle companion can close immediately.
    if(TryEnterCriticalSection(&exchange_lock)) {
        if(bank_socket!=INVALID_SOCKET && !bank_current_generation(bank_generation)) {
            game_close(bank_socket); bank_socket=INVALID_SOCKET;InterlockedExchange(&accepted_generation,0);
        }
        LeaveCriticalSection(&exchange_lock);
    }
    return game_close(socket);
}
struct Work { HWND panel; Auth auth; pn_bank::Request request; };
// Both the request worker and idle receiver hold exchange_lock while reading.
// Notifications can arrive before a transaction reply and never count as its
// acknowledgement. A single deadline also bounds fragmented or excessive input.
bool read_reply(SOCKET socket,pn_bank::Reply& reply,ULONGLONG deadline) {
    int received=0;
    while(received<static_cast<int>(sizeof(reply))) {
        auto now=GetTickCount64(); if(now>=deadline) return false;
        auto remaining=deadline-now;
        timeval timeout{static_cast<long>(remaining/1000),static_cast<long>((remaining%1000)*1000)};
        fd_set readable; FD_ZERO(&readable); FD_SET(socket,&readable);
        if(select(0,&readable,nullptr,nullptr,&timeout)<=0) return false;
        int n=recv(socket,reinterpret_cast<char*>(&reply)+received,sizeof(reply)-received,0);
        if(n<=0) return false;
        received+=n;
    }
    return pn_bank::valid_reply(reply);
}
bool notify_open(HWND panel,const pn_bank::Reply& reply,LONG generation) {
    return reply.flags==pn_bank::open_panel && reply.result!=pn_bank::Unauthorized &&
        bank_current_generation(generation) && PostMessage(panel,BANK_REMOTE_OPEN,static_cast<WPARAM>(generation),0);
}
DWORD WINAPI watch_companion(void*) {
    while(IsWindow(panel_window)) {
        if(TryEnterCriticalSection(&exchange_lock)) {
            if(bank_socket!=INVALID_SOCKET) {
                bool ok=bank_current_generation(bank_generation);
                fd_set readable; FD_ZERO(&readable); FD_SET(bank_socket,&readable); timeval timeout{};
                int ready=ok?select(0,&readable,nullptr,nullptr,&timeout):SOCKET_ERROR;
                if(ready>0) {
                    pn_bank::Reply reply;
                    ok=read_reply(bank_socket,reply,GetTickCount64()+5000);
                    EnterCriticalSection(&lock);
                    ok=ok && reply.char_id==auth.character;
                    LeaveCriticalSection(&lock);
                    ok=ok && notify_open(panel_window,reply,bank_generation);
                } else if(ready<0) ok=false;
                if(!ok) { game_close(bank_socket); bank_socket=INVALID_SOCKET;InterlockedExchange(&accepted_generation,0); }
            }
            LeaveCriticalSection(&exchange_lock);
        }
        Sleep(100);
    }
    return 0;
}
bool exchange(const Work& work,pn_bank::Reply& reply) {
    // Reuse one connection per game session. Opening a socket for every refresh
    // would trip the server's normal flood protection, especially behind NAT.
    EnterCriticalSection(&exchange_lock);
    bool ok=false;
    do {
        if(!bank_current_generation(work.auth.generation)) break;
        if(bank_socket!=INVALID_SOCKET && bank_generation!=work.auth.generation) {
            game_close(bank_socket); bank_socket=INVALID_SOCKET;InterlockedExchange(&accepted_generation,0);
        }
        bool fresh=bank_socket==INVALID_SOCKET;
        if(fresh) bank_socket=::socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
        SOCKET socket=bank_socket;
        if(socket==INVALID_SOCKET) break;
        bank_generation=work.auth.generation;
        if(fresh) {
        u_long nonblock=1; ioctlsocket(socket,FIONBIO,&nonblock);
        int connected=connect(socket,reinterpret_cast<const sockaddr*>(&work.auth.peer),sizeof(work.auth.peer));
        if(connected==SOCKET_ERROR && WSAGetLastError()!=WSAEWOULDBLOCK) break;
        fd_set writable; FD_ZERO(&writable); FD_SET(socket,&writable); timeval timeout{5,0};
        if(select(0,nullptr,&writable,nullptr,&timeout)<=0) break;
        int error=0, error_size=sizeof(error);
        if(getsockopt(socket,SOL_SOCKET,SO_ERROR,reinterpret_cast<char*>(&error),&error_size) || error) break;
        nonblock=0; ioctlsocket(socket,FIONBIO,&nonblock);
        DWORD milliseconds=5000;
        setsockopt(socket,SOL_SOCKET,SO_SNDTIMEO,reinterpret_cast<char*>(&milliseconds),sizeof(milliseconds));
        setsockopt(socket,SOL_SOCKET,SO_RCVTIMEO,reinterpret_cast<char*>(&milliseconds),sizeof(milliseconds));
        }
        int sent=0;
        while(sent<static_cast<int>(sizeof(work.request))) {
            int n=game_send(socket,reinterpret_cast<const char*>(&work.request)+sent,sizeof(work.request)-sent,0);
            if(n<=0) break;
            sent+=n;
        }
        if(sent!=sizeof(work.request)) break;
        ULONGLONG deadline=GetTickCount64()+5000;
        while(read_reply(socket,reply,deadline)) {
            if(reply.char_id!=work.auth.character && reply.result!=pn_bank::Unauthorized) break;
            if(reply.flags) {
                if(!notify_open(work.panel,reply,work.auth.generation)) break;
                continue;
            }
            InterlockedExchange(&accepted_generation,reply.result==pn_bank::Unauthorized?0:work.auth.generation);
            ok=true; break;
        }
    } while(false);
    if(bank_socket!=INVALID_SOCKET && (!ok || !bank_current_generation(work.auth.generation))) {
        game_close(bank_socket); bank_socket=INVALID_SOCKET;InterlockedExchange(&accepted_generation,0);
    }
    LeaveCriticalSection(&exchange_lock);
    return ok;
}
DWORD WINAPI worker(void* argument) {
    auto work=static_cast<Work*>(argument);
    auto result=new BankResult;
    result->generation=work->auth.generation;
    result->connected=exchange(*work,result->state);
    if(!PostMessage(work->panel,BANK_RESULT,0,reinterpret_cast<LPARAM>(result))) delete result;
    SecureZeroMemory(&work->auth,sizeof(work->auth)); SecureZeroMemory(&work->request,sizeof(work->request));
    delete work; return 0;
}
BOOL CALLBACK find_game(HWND window,LPARAM result) {
    DWORD process=0; GetWindowThreadProcessId(window,&process);
    wchar_t name[100]; GetClassNameW(window,name,100);
    RECT area{}; GetClientRect(window,&area);
    if(process==GetCurrentProcessId() && IsWindowVisible(window) && !GetWindow(window,GW_OWNER) &&
        wcscmp(name,L"PNAccountBank") && area.right>=320 && area.bottom>=240) {
        *reinterpret_cast<HWND*>(result)=window; return FALSE;
    }
    return TRUE;
}
}

void bank_install_transport(HWND panel) {
    panel_window=panel; InitializeCriticalSection(&lock); InitializeCriticalSection(&exchange_lock);
    wchar_t file[MAX_PATH]; GetModuleFileNameW(nullptr,file,MAX_PATH);
    auto slash=wcsrchr(file,L'\\'); if(slash) wcscpy(slash+1,L"BankUI.ini");
    char_port=static_cast<uint16_t>(GetPrivateProfileIntW(L"Bank",L"CharacterPort",6121,file));
    map_port=static_cast<uint16_t>(GetPrivateProfileIntW(L"Bank",L"MapPort",5121,file));
    // This executable resolves imports dynamically. Hook the documented API
    // entry points, using the same library as the existing font extension.
    if(MH_Initialize()!=MH_OK) return;
    if(MH_CreateHookApi(L"ws2_32.dll","send",reinterpret_cast<void*>(observed_send),reinterpret_cast<void**>(&game_send))!=MH_OK ||
        MH_CreateHookApi(L"ws2_32.dll","closesocket",reinterpret_cast<void*>(observed_close),reinterpret_cast<void**>(&game_close))!=MH_OK) return;
    if(MH_EnableHook(MH_ALL_HOOKS)!=MH_OK) return;
    HANDLE thread=CreateThread(nullptr,0,watch_companion,nullptr,0,nullptr);
    if(thread) {CloseHandle(thread);InterlockedExchange(&transport_initialized,1);}
}
bool bank_authenticated() { EnterCriticalSection(&lock); bool ready=auth.ready; LeaveCriticalSection(&lock); return ready; }
bool bank_current_generation(LONG generation,bool active) { EnterCriticalSection(&lock); bool same=(!active || auth.ready) && generation==auth.generation; LeaveCriticalSection(&lock); return same; }
bool bank_connection_ready() {
    // The UI timer must not wait behind a transaction receive.
    if(!TryEnterCriticalSection(&exchange_lock)) return true;
    bool ready=bank_socket!=INVALID_SOCKET && bank_current_generation(bank_generation);
    LeaveCriticalSection(&exchange_lock); return ready;
}
bool bank_submit(HWND panel,const pn_bank::Reply& state,uint32_t action,int64_t amount,uint64_t sequence) {
    auto work=new Work; work->panel=panel;
    EnterCriticalSection(&lock); work->auth=auth; LeaveCriticalSection(&lock);
    if(!work->auth.ready) { delete work; return false; }
    work->request.account_id=work->auth.account; work->request.char_id=work->auth.character;
    work->request.login_id1=work->auth.one; work->request.login_id2=work->auth.two;
    work->request.nonce_hi=state.nonce_hi; work->request.nonce_lo=state.nonce_lo;
    work->request.request_id=sequence; work->request.action=action; work->request.amount=amount;
    HANDLE thread=CreateThread(nullptr,0,worker,work,0,nullptr);
    if(!thread) { SecureZeroMemory(&work->auth,sizeof(work->auth)); SecureZeroMemory(&work->request,sizeof(work->request)); delete work; return false; }
    CloseHandle(thread); return true;
}
HWND bank_find_game_window() { HWND result=nullptr; EnumWindows(find_game,reinterpret_cast<LPARAM>(&result)); return result; }
// Input extensions receive no identities, tokens, balances or socket handles.
extern "C" __declspec(dllexport) BOOL WINAPI PNGameInputReady() {
    if(!InterlockedCompareExchange(&transport_initialized,0,0))return FALSE;
    LONG generation=InterlockedCompareExchange(&accepted_generation,0,0);
    return generation && bank_current_generation(generation) && !IsWindowVisible(panel_window);
}
extern "C" __declspec(dllexport) HWND WINAPI PNGameWindow() {return bank_find_game_window();}
