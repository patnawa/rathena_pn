// Load the shipping DLL chain in a disposable Windows process. No game login.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cassert>
#include <cstring>
#include <iostream>
using Callback=BOOL(WINAPI*)(GUID*,LPSTR,LPSTR,LPVOID,HMONITOR);
using Draw=HRESULT(WINAPI*)(Callback,LPVOID,DWORD);
BOOL WINAPI device(GUID*,LPSTR,LPSTR,LPVOID,HMONITOR) { return FALSE; }
BOOL CALLBACK own_panel(HWND window,LPARAM result) {
    DWORD pid=0;GetWindowThreadProcessId(window,&pid);
    wchar_t name[64];GetClassNameW(window,name,64);
    if(pid==GetCurrentProcessId() && !wcscmp(name,L"PNAccountBank")) {
        *reinterpret_cast<HWND*>(result)=window;return FALSE;
    }
    return TRUE;
}
int main() {
    wchar_t file[MAX_PATH]; GetModuleFileNameW(nullptr,file,MAX_PATH);
    auto slash=wcsrchr(file,L'\\'); assert(slash); wcscpy(slash+1,L"FontScale.dll");
    auto module=LoadLibraryW(file); assert(module);
    auto symbol=GetProcAddress(module,"_DirectDrawEnumerateExA@12"); assert(symbol);
    Draw draw; static_assert(sizeof(draw)==sizeof(symbol)); memcpy(&draw,&symbol,sizeof(draw));
    assert(SUCCEEDED(draw(device,nullptr,0)));
    HWND panel=nullptr;
    auto deadline=GetTickCount64()+5000;
    while(GetTickCount64()<deadline) {
        EnumWindows(own_panel,reinterpret_cast<LPARAM>(&panel));
        if(panel && GetModuleHandleW(L"FontScaleOriginal.dll") && GetModuleHandleW(L"BankUI.dll")) break;
        Sleep(10);
    }
    if(!panel) {
        auto bank=GetModuleHandleW(L"BankUI.dll");
        std::cerr<<"Bank module="<<bank<<", original="<<GetModuleHandleW(L"FontScaleOriginal.dll")<<"\n";
        wcscpy(slash+1,L"BankUI.dll");
        if(!bank) { bank=LoadLibraryW(file); std::cerr<<"Direct bank load="<<bank<<", error="<<GetLastError()<<"\n"; }
    }
    assert(panel && !IsWindowVisible(panel));
    wchar_t title[20]; GetWindowTextW(panel,title,20); assert(wcscmp(title,L"Bank")==0);
    assert(!IsWindowEnabled(GetDlgItem(panel,400)) && !IsWindowEnabled(GetDlgItem(panel,401)));
    // Support both the original scaler and the current native reference font.
    const bool native_font=GetProcAddress(GetModuleHandleW(L"FontScaleOriginal.dll"),"PNFontStatus")!=nullptr;
    bool scaled=false;
    while(GetTickCount64()<deadline) {
        auto font=CreateFontW(-10,0,0,0,400,0,0,0,0,0,0,0,0,L"Tahoma");
        LOGFONTW details{}; assert(GetObjectW(font,sizeof(details),&details)); DeleteObject(font);
        if(native_font?(details.lfHeight==-10 && !wcscmp(details.lfFaceName,L"Arial")):details.lfHeight==-11) { scaled=true; break; }
        Sleep(10);
    }
    assert(scaled);
    std::cout<<"PASS: shipping FontScale export forwards DirectDraw, installed font behavior preserved, BankUI loads hidden with unauthenticated transactions disabled\n";
    // Do not FreeLibrary an API-hook DLL while its hooks/threads are active.
}
