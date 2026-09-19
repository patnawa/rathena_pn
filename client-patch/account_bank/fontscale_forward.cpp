// Keep the installed font hooks and add the account-bank panel. GPL-3.0-or-later.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cwchar>
#include <cstring>
namespace {
HMODULE self;
INIT_ONCE once=INIT_ONCE_STATIC_INIT;
using DrawFn=HRESULT(WINAPI*)(void*,void*,DWORD);
DrawFn draw=nullptr;
BOOL CALLBACK initialize(PINIT_ONCE,void*,void**) {
    wchar_t path[MAX_PATH]; GetModuleFileNameW(self,path,MAX_PATH);
    auto slash=wcsrchr(path,L'\\'); if(!slash) return TRUE;
    wcscpy(slash+1,L"FontScaleOriginal.dll");
    HMODULE original=LoadLibraryW(path);
    if(original) {
        auto symbol=GetProcAddress(original,"_DirectDrawEnumerateExA@12");
        static_assert(sizeof(draw)==sizeof(symbol)); memcpy(&draw,&symbol,sizeof(draw));
    }
    wcscpy(slash+1,L"BankUI.dll"); LoadLibraryW(path);
    wcscpy(slash+1,L"PNTurbo.dll"); LoadLibraryW(path);
    return TRUE;
}
DWORD WINAPI start(void*) { InitOnceExecuteOnce(&once,initialize,nullptr,nullptr); return 0; }
}
extern "C" HRESULT WINAPI ForwardDirectDraw(void* callback,void* context,DWORD flags) {
    InitOnceExecuteOnce(&once,initialize,nullptr,nullptr);
    return draw ? draw(callback,context,flags) : E_FAIL;
}
BOOL WINAPI DllMain(HINSTANCE module,DWORD reason,void*) {
    if(reason==DLL_PROCESS_ATTACH) {
        self=module; DisableThreadLibraryCalls(module);
        HANDLE thread=CreateThread(nullptr,0,start,nullptr,0,nullptr); if(thread) CloseHandle(thread);
    }
    return TRUE;
}
