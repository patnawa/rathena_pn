// PN client-local held-input turbo. GPL-3.0-or-later.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cwchar>
#include <cstdio>
#include <cstring>
#include <string>
#include "turbo_core.hpp"
namespace {
using namespace pn_turbo;
HMODULE module;HWND game,toast;HHOOK keyboard_hook,mouse_hook;
using Ready=BOOL(WINAPI*)();using GameWindow=HWND(WINAPI*)();Ready session_ready;GameWindow game_window;
Core core;Action active;unsigned phase=0;uint64_t phase_at=0,toast_until=0;
wchar_t config_path[MAX_PATH],status_path[MAX_PATH];FILETIME config_time{};
bool configured=false,default_state=false,ready_cached=false,was_ready=false;
unsigned pause_key='P',notice=0;uint64_t cycles=0,last_status=0;
uint64_t smart_requests=0,repeat_requests=0,smart_clicks=0,mouse_accepted=0,mouse_rejected=0,toggles=0;
// Let the game process the skill key before clicking its targeting cursor.
constexpr unsigned smart_key_hold_ms=32,smart_target_wait_ms=50,smart_mouse_hold_ms=32;
using InputSink=UINT(WINAPI*)(UINT,LPINPUT,int);InputSink input_sink=SendInput;
ULONG_PTR tag;volatile LONG initialized=0;
UINT_PTR fast_timer=0,slow_timer=0;
bool modifier(int vk){return (GetAsyncKeyState(vk)&0x8000)!=0;}
bool other_modifiers(){return modifier(VK_CONTROL)||modifier(VK_SHIFT)||modifier(VK_LWIN)||modifier(VK_RWIN);}
bool focused(){return game && GetForegroundWindow()==game && IsWindowVisible(game) && !IsIconic(game);}
bool cursor_inside(){POINT point{};RECT rect{};if(!GetCursorPos(&point)||!GetClientRect(game,&rect))return false;
    HWND hit=WindowFromPoint(point);ScreenToClient(game,&point);return PtInRect(&rect,point) && GetAncestor(hit,GA_ROOT)==game;}
bool base_allowed(){
    if(!ready_cached || !focused())return false;
    GUITHREADINFO info{};info.cbSize=sizeof(info);
    if(GetGUIThreadInfo(GetWindowThreadProcessId(game,nullptr),&info)){
        wchar_t name[80]{};GetClassNameW(info.hwndFocus,name,80);
        if(wcsstr(name,L"Edit")||wcsstr(name,L"EDIT")||info.hwndMenuOwner||(info.hwndCaret&&(info.flags&GUI_CARETBLINKING)))return false;
    }
    return true;
}
bool allowed(Mode mode){return base_allowed()&&!other_modifiers() &&
    (mode==AltRight ? modifier(VK_MENU)&&cursor_inside() : !modifier(VK_MENU) && (mode!=Smart || cursor_inside()));}
unsigned key_code(std::wstring text){
    while(!text.empty() && iswspace(text.back()))text.pop_back();
    while(!text.empty() && iswspace(text.front()))text.erase(0,1);
    for(auto& c:text)c=towupper(c);
    if(text.size()==1 && ((text[0]>='A'&&text[0]<='Z')||(text[0]>='0'&&text[0]<='9')))return text[0];
    if(text.size()>=2 && text[0]=='F'){
        wchar_t* end=nullptr;long n=wcstol(text.c_str()+1,&end,10);
        if(*end==0 && n>=1&&n<=12)return VK_F1+static_cast<unsigned>(n)-1;
    }
    return 0;
}
unsigned delay(const wchar_t* section,const wchar_t* key,unsigned fallback){
    unsigned value=GetPrivateProfileIntW(section,key,fallback,config_path);return value<10?10:(value>5000?5000:value);
}
void reload(){
    WIN32_FILE_ATTRIBUTE_DATA attributes{};
    if(!GetFileAttributesExW(config_path,GetFileExInfoStandard,&attributes))return;
    if(attributes.nFileSizeHigh || attributes.nFileSizeLow>16384)return;
    if(configured && CompareFileTime(&attributes.ftLastWriteTime,&config_time)==0)return;
    config_time=attributes.ftLastWriteTime;
    std::array<Binding,256> next{};unsigned standard=delay(L"General",L"DelayMs",10);bool valid=true;
    for(int group=0;group<2;group++)for(int slot=1;slot<=10;slot++){
        const wchar_t* section=group==0?L"SmartKeys":L"TurboKeys";
        wchar_t field[30],value[40];swprintf(field,30,L"Key%d",slot);
        GetPrivateProfileStringW(section,field,L"",value,40,config_path);
        if(!value[0])continue;
        unsigned key=key_code(value);
        if(!key || next[key].mode!=Off){valid=false;continue;}
        swprintf(field,30,L"Delay%d",slot);next[key]={group==0?Smart:Repeat,delay(section,field,standard)};
    }
    if(GetPrivateProfileIntW(L"AltRightClick",L"Enabled",0,config_path))next[VK_RBUTTON]={AltRight,delay(L"AltRightClick",L"DelayMs",100)};
    wchar_t pause[40];GetPrivateProfileStringW(L"General",L"PauseKey",L"P",pause,40,config_path);
    pause_key=key_code(pause);if(!pause_key){pause_key='P';valid=false;}
    bool next_default=GetPrivateProfileIntW(L"General",L"DefaultState",0,config_path)!=0;
    core.interrupt();core.bindings=next;
    if(!configured || next_default!=default_state)core.enable(next_default);
    default_state=next_default;configured=true;
    if(!valid){core.enable(false);notice=4;}else notice=core.enabled?1:2;
}
INPUT key_input(unsigned key,bool up){INPUT i{};i.type=INPUT_KEYBOARD;i.ki.wScan=static_cast<WORD>(MapVirtualKeyW(key,MAPVK_VK_TO_VSC));
    i.ki.dwFlags=KEYEVENTF_SCANCODE|(up?KEYEVENTF_KEYUP:0);i.ki.dwExtraInfo=tag;return i;}
INPUT mouse_input(bool right,bool up){INPUT i{};i.type=INPUT_MOUSE;i.mi.dwFlags=right?(up?MOUSEEVENTF_RIGHTUP:MOUSEEVENTF_RIGHTDOWN):(up?MOUSEEVENTF_LEFTUP:MOUSEEVENTF_LEFTDOWN);i.mi.dwExtraInfo=tag;return i;}
bool send_one(INPUT input){return input_sink(1,&input,sizeof(input))==1;}
void cancel(){
    // Key/button up completes only an input previously synthesized by us.
    // Physical downs were captured separately and never rewritten as held input.
    if(phase==1)send_one(active.mode==AltRight?mouse_input(true,true):key_input(active.key,true));
    if(phase==2)send_one(mouse_input(false,true));
    phase=0;active={};
}
void pause(unsigned reason){core.enable(false);notice=reason;}
void start_action(Action next,uint64_t now){
    active=next;
    if(send_one(next.mode==AltRight?mouse_input(true,false):key_input(next.key,false))){
        phase=1;phase_at=now+(next.mode==Smart?smart_key_hold_ms:16);
    }else{active={};pause(5);}
}
void advance_action(uint64_t now){
    if(!phase || now<phase_at)return;
    if(phase==1){
        if(!send_one(active.mode==AltRight?mouse_input(true,true):key_input(active.key,true))){pause(5);return;}
        if(active.mode==Smart){phase=3;phase_at=now+smart_target_wait_ms;return;}
    }else if(phase==3){
        if(!send_one(mouse_input(false,false))){pause(5);return;}
        smart_clicks++;phase=2;phase_at=now+smart_mouse_hold_ms;return;
    }else if(!send_one(mouse_input(false,true))){pause(5);return;}
    phase=0;core.completed(active,now);cycles++;active={};
}
LRESULT CALLBACK keyboard(int code,WPARAM message,LPARAM parameter){
    if(code<0)return CallNextHookEx(nullptr,code,message,parameter);
    auto& event=*reinterpret_cast<KBDLLHOOKSTRUCT*>(parameter);
    bool up=(event.flags&LLKHF_UP)!=0;
    if(event.dwExtraInfo==tag){
        if(!up && (!base_allowed()||other_modifiers()||modifier(VK_MENU)))return 1;
        return CallNextHookEx(nullptr,code,message,parameter);
    }
    if(event.flags&LLKHF_INJECTED)return CallNextHookEx(nullptr,code,message,parameter);
    const unsigned key=event.vkCode;
    static bool pause_held=false,pause_captured=false;
    if(key==pause_key){
        if(up){bool swallow=pause_captured;pause_held=pause_captured=false;if(swallow)return 1;}
        else if(!pause_held && modifier(VK_MENU)&&focused()){
            pause_held=pause_captured=true;core.enable(!core.enabled);toggles++;notice=core.enabled?1:2;return 1;
        }else if(pause_captured)return 1;
    }
    if(!up && ready_cached && focused() && (key==VK_RETURN || key==VK_ESCAPE))pause(3);
    if(!up && (key==VK_LCONTROL||key==VK_RCONTROL||key==VK_LSHIFT||key==VK_RSHIFT||key==VK_LMENU||key==VK_RMENU||key==VK_LWIN||key==VK_RWIN))core.interrupt();
    if(key<256 && (core.bindings[key].mode!=Off || core.is_held(key))){
        bool fresh=!up&&!core.is_held(key);
        if(core.event(key,!up,allowed(core.bindings[key].mode),GetTickCount64())){
            if(fresh){if(core.bindings[key].mode==Smart)smart_requests++;else repeat_requests++;}return 1;
        }
    }
    return CallNextHookEx(nullptr,code,message,parameter);
}
LRESULT CALLBACK mouse(int code,WPARAM message,LPARAM parameter){
    if(code<0)return CallNextHookEx(nullptr,code,message,parameter);
    auto& event=*reinterpret_cast<MSLLHOOKSTRUCT*>(parameter);
    if(event.dwExtraInfo==tag){
        bool down=message==WM_LBUTTONDOWN||message==WM_RBUTTONDOWN;
        if(down){if(!allowed(active.mode)||!core.valid(active)){mouse_rejected++;return 1;}mouse_accepted++;}
        return CallNextHookEx(nullptr,code,message,parameter);
    }
    if(event.flags&LLMHF_INJECTED)return CallNextHookEx(nullptr,code,message,parameter);
    if(message==WM_RBUTTONDOWN||message==WM_RBUTTONUP){
        if(core.event(VK_RBUTTON,message==WM_RBUTTONDOWN,allowed(AltRight),GetTickCount64()))return 1;
    }
    return CallNextHookEx(nullptr,code,message,parameter);
}
void tick(){
    uint64_t now=GetTickCount64();ready_cached=session_ready && session_ready();
    if(ready_cached!=was_ready){core.interrupt();was_ready=ready_cached;}
    if(!base_allowed() || other_modifiers()){core.interrupt();cancel();}
    else if(phase){
        if(!allowed(active.mode)||!core.valid(active)){cancel();}
        else advance_action(now);
    }else{
        Action next=core.next(now,base_allowed());
        if(next.mode!=Off && allowed(next.mode)){
            start_action(next,now);
        }
    }
    if(notice && focused()){
        wchar_t key[16],label[160];if(pause_key>=VK_F1&&pause_key<=VK_F12)swprintf(key,16,L"F%u",pause_key-VK_F1+1);else swprintf(key,16,L"%lc",pause_key);
        const wchar_t* format=notice==1?L"Turbo ON  |  Alt+%ls to pause":notice==2?L"Turbo OFF  |  Alt+%ls to enable":notice==3?L"Turbo paused for chat/menu  |  Alt+%ls resumes":notice==4?L"Turbo OFF: check Turbo Setup":L"Turbo paused: input unavailable";
        swprintf(label,160,format,key);
        SetWindowTextW(toast,label);RECT rect{};GetWindowRect(game,&rect);
        SetWindowPos(toast,HWND_TOPMOST,rect.left+24,rect.top+44,365,28,SWP_NOACTIVATE|SWP_SHOWWINDOW);
        toast_until=now+1800;notice=0;
    }
    if(!focused()||now>=toast_until)ShowWindow(toast,SW_HIDE);
    if(now-last_status>2000){
        wchar_t text[700];swprintf(text,700,L"[Turbo]\nBuild=20260919.3\nProcessId=%lu\nTick=%llu\nLoaded=1\nEnabled=%d\nCharacterReady=%d\nFocused=%d\nCursorInside=%d\nInputAllowed=%d\nCompletedCycles=%llu\nToggles=%llu\nSmartHolds=%llu\nRepeatHolds=%llu\nSmartClicks=%llu\nMouseAccepted=%llu\nMouseRejected=%llu\n",GetCurrentProcessId(),static_cast<unsigned long long>(now),core.enabled,ready_cached,focused(),game&&cursor_inside(),base_allowed(),static_cast<unsigned long long>(cycles),static_cast<unsigned long long>(toggles),static_cast<unsigned long long>(smart_requests),static_cast<unsigned long long>(repeat_requests),static_cast<unsigned long long>(smart_clicks),static_cast<unsigned long long>(mouse_accepted),static_cast<unsigned long long>(mouse_rejected));
        HANDLE f=CreateFileW(status_path,GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
        if(f!=INVALID_HANDLE_VALUE){std::string ascii;for(auto* p=text;*p;p++)ascii.push_back(static_cast<char>(*p));DWORD written;WriteFile(f,ascii.data(),static_cast<DWORD>(ascii.size()),&written,nullptr);CloseHandle(f);}last_status=now;
    }
}
DWORD WINAPI run(void*){
    wchar_t path[MAX_PATH];GetModuleFileNameW(module,path,MAX_PATH);wchar_t* slash=wcsrchr(path,L'\\');if(!slash)return 1;
    wcscpy(slash+1,L"PN-Turbo.ini");wcscpy(config_path,path);wcscpy(slash+1,L"PN-Turbo.status.ini");wcscpy(status_path,path);
    tag=0x504e5400u^GetCurrentProcessId();
    HMODULE bank=GetModuleHandleW(L"BankUI.dll");if(!bank)return 2;
    session_ready=reinterpret_cast<Ready>(GetProcAddress(bank,"PNGameInputReady@0"));
    auto found=GetProcAddress(bank,"PNGameWindow@0");static_assert(sizeof(game_window)==sizeof(found));memcpy(&game_window,&found,sizeof(found));if(!session_ready||!game_window)return 3;
    toast=CreateWindowExW(WS_EX_TOOLWINDOW|WS_EX_NOACTIVATE|WS_EX_TRANSPARENT,L"STATIC",L"",WS_POPUP|SS_CENTER|SS_CENTERIMAGE|WS_BORDER,0,0,365,28,nullptr,nullptr,module,nullptr);
    SendMessageW(toast,WM_SETFONT,reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),TRUE);
    EnableWindow(toast,FALSE);
    keyboard_hook=SetWindowsHookExW(WH_KEYBOARD_LL,keyboard,module,0);mouse_hook=SetWindowsHookExW(WH_MOUSE_LL,mouse,module,0);
    if(!keyboard_hook||!mouse_hook){if(keyboard_hook)UnhookWindowsHookEx(keyboard_hook);if(mouse_hook)UnhookWindowsHookEx(mouse_hook);return 4;}
    game=game_window();reload();fast_timer=SetTimer(nullptr,0,10,nullptr);slow_timer=SetTimer(nullptr,0,500,nullptr);InterlockedExchange(&initialized,1);
    MSG message;while(GetMessageW(&message,nullptr,0,0)>0){
        if(message.message==WM_TIMER){
            if(message.wParam==fast_timer)tick();else if(message.wParam==slow_timer){game=game_window();reload();}
        }else{TranslateMessage(&message);DispatchMessageW(&message);}
    }
    cancel();UnhookWindowsHookEx(keyboard_hook);UnhookWindowsHookEx(mouse_hook);DestroyWindow(toast);return 0;
}
}
extern "C" __declspec(dllexport) DWORD WINAPI PNTurboStatus(){return InterlockedCompareExchange(&initialized,0,0);}
BOOL WINAPI DllMain(HINSTANCE instance,DWORD reason,void*){
    if(reason==DLL_PROCESS_ATTACH){module=instance;DisableThreadLibraryCalls(instance);HANDLE thread=CreateThread(nullptr,0,run,nullptr,0,nullptr);if(thread)CloseHandle(thread);}return TRUE;
}
