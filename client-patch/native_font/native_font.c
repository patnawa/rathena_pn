/* PN reference font profile. GPL-3.0-or-later.
 * A font-selection correction, with no global scaling or text-layout hooks.
 * The legacy export/file name preserves the installed bank/DirectDraw chain. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../account_bank/vendor/MinHook/include/MinHook.h"

static HMODULE self;
static INIT_ONCE once=INIT_ONCE_STATIC_INIT;
static uintptr_t app_start,app_end;
static volatile LONG applied, ready;
static wchar_t log_path[MAX_PATH];
typedef HFONT (WINAPI *FontA)(int,int,int,int,int,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,LPCSTR);
typedef HFONT (WINAPI *FontW)(int,int,int,int,int,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,LPCWSTR);
static FontA original_a;
static FontW original_w;
static HFONT (WINAPI *original_ia)(const LOGFONTA *);
static HFONT (WINAPI *original_iw)(const LOGFONTW *);
typedef HRESULT (WINAPI *Draw)(void*,void*,DWORD);
static Draw original_draw;

static void log_line(const char *line, BOOL reset) {
    HANDLE f=CreateFileW(log_path,GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,
        reset?CREATE_ALWAYS:OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    if(f==INVALID_HANDLE_VALUE) return;
    if(!reset) SetFilePointer(f,0,NULL,FILE_END);
    DWORD written; WriteFile(f,line,(DWORD)strlen(line),&written,NULL); CloseHandle(f);
}

static BOOL eligible(void *caller, int height, DWORD charset, int rotation, int orientation) {
    uintptr_t p=(uintptr_t)caller;
    return p>=app_start && p<app_end && height>=-16 && height<=16 &&
        (height>=9 || height<=-9) && charset!=SYMBOL_CHARSET && !rotation && !orientation;
}

static void trace(const char *api,int before,int after) {
    LONG count=InterlockedIncrement(&applied);
    if(count<=16) {
        char msg[180]; snprintf(msg,sizeof(msg),"%s: requested height %d -> Arial character height %d, unsmoothed\r\n",api,before,-after);
        log_line(msg,FALSE);
    }
}

/* The supplied 2026 client requests a 14-pixel cell for Basic Information.
 * Runtime label probes confirmed HP/SP/Base Lv./Job Lv. use this font, while
 * pixel comparison proves their reference glyphs are Arial character height 11.
 * Keep this cell-font slot at 11. Regular 13-cell resource numbers keep Arial
 * character height 10. Runtime draws of Kafra Employee and Healer identify
 * the bold 13-cell font as NPC/character names: use character height 11,
 * between the original 10-pixel correction and the oversized 12/13-pixel trials.
 * The 11-cell inventory fonts and explicit -13/-14 requests remain distinct.
 * Positive GDI heights include internal leading. RO's standard 12 request
 * should mean a 12-pixel character, not a smaller character in a 12-pixel cell.
 * Preserve the other 9..16 sizes, bold/underline/italic, and all
 * larger display fonts. The API's own text metrics then match its raster. */
static int character_height(int h,int weight) {
    return h==14 ? -11 : (h==13 ? (weight==FW_BOLD ? -11 : -10) : (h>0 ? -h : h));
}
static void fix_a(LOGFONTA *f) {
    f->lfHeight=character_height(f->lfHeight,f->lfWeight);
    f->lfWidth=0; f->lfCharSet=ANSI_CHARSET; f->lfQuality=NONANTIALIASED_QUALITY;
    f->lfPitchAndFamily=DEFAULT_PITCH|FF_SWISS;
    strcpy(f->lfFaceName,"Arial");
}
static void fix_w(LOGFONTW *f) {
    f->lfHeight=character_height(f->lfHeight,f->lfWeight);
    f->lfWidth=0; f->lfCharSet=ANSI_CHARSET; f->lfQuality=NONANTIALIASED_QUALITY;
    f->lfPitchAndFamily=DEFAULT_PITCH|FF_SWISS;
    wcscpy(f->lfFaceName,L"Arial");
}

static HFONT WINAPI hooked_a(int h,int w,int e,int o,int weight,DWORD italic,DWORD underline,DWORD strike,
    DWORD cs,DWORD out,DWORD clip,DWORD quality,DWORD pitch,LPCSTR name) {
    if(eligible(__builtin_return_address(0),h,cs,e,o)) {
        int before=h; h=character_height(h,weight); w=0; cs=ANSI_CHARSET; quality=NONANTIALIASED_QUALITY;
        pitch=DEFAULT_PITCH|FF_SWISS; name="Arial"; trace("CreateFontA",before,h);
    }
    return original_a(h,w,e,o,weight,italic,underline,strike,cs,out,clip,quality,pitch,name);
}
static HFONT WINAPI hooked_w(int h,int w,int e,int o,int weight,DWORD italic,DWORD underline,DWORD strike,
    DWORD cs,DWORD out,DWORD clip,DWORD quality,DWORD pitch,LPCWSTR name) {
    if(eligible(__builtin_return_address(0),h,cs,e,o)) {
        int before=h; h=character_height(h,weight); w=0; cs=ANSI_CHARSET; quality=NONANTIALIASED_QUALITY;
        pitch=DEFAULT_PITCH|FF_SWISS; name=L"Arial"; trace("CreateFontW",before,h);
    }
    return original_w(h,w,e,o,weight,italic,underline,strike,cs,out,clip,quality,pitch,name);
}
static HFONT WINAPI hooked_ia(const LOGFONTA *f) {
    if(f && eligible(__builtin_return_address(0),f->lfHeight,f->lfCharSet,f->lfEscapement,f->lfOrientation)) {
        LOGFONTA copy=*f; fix_a(&copy); trace("CreateFontIndirectA",f->lfHeight,copy.lfHeight); return original_ia(&copy);
    }
    return original_ia(f);
}
static HFONT WINAPI hooked_iw(const LOGFONTW *f) {
    if(f && eligible(__builtin_return_address(0),f->lfHeight,f->lfCharSet,f->lfEscapement,f->lfOrientation)) {
        LOGFONTW copy=*f; fix_w(&copy); trace("CreateFontIndirectW",f->lfHeight,copy.lfHeight); return original_iw(&copy);
    }
    return original_iw(f);
}
static BOOL CALLBACK initialize(PINIT_ONCE unused,void *arg,void **context) {
    (void)unused;(void)arg;(void)context;
    GetModuleFileNameW(self,log_path,MAX_PATH);
    wchar_t *slash=wcsrchr(log_path,L'\\'); if(slash) wcscpy(slash+1,L"FontFix.log");
    app_start=(uintptr_t)GetModuleHandleW(NULL);
    IMAGE_DOS_HEADER *dos=(IMAGE_DOS_HEADER*)app_start;
    IMAGE_NT_HEADERS *nt=(IMAGE_NT_HEADERS*)(app_start+dos->e_lfanew);
    app_end=app_start+nt->OptionalHeader.SizeOfImage;
    wchar_t path[MAX_PATH]; GetSystemDirectoryW(path,MAX_PATH); wcscat(path,L"\\ddraw.dll");
    HMODULE dd=LoadLibraryW(path);
    if(dd) original_draw=(Draw)(void*)GetProcAddress(dd,"DirectDrawEnumerateExA");
    log_line("PN font profile 6: Arial; bold NPC/character names 11px; inventory 11px; panel labels 11px; regular resource values 10px; body 12px.\r\n",TRUE);
    if(MH_Initialize()!=MH_OK) {log_line("ERROR: hook initialization failed.\r\n",FALSE);return TRUE;}
    if(MH_CreateHookApi(L"gdi32.dll","CreateFontA",hooked_a,(void**)&original_a)!=MH_OK ||
       MH_CreateHookApi(L"gdi32.dll","CreateFontW",hooked_w,(void**)&original_w)!=MH_OK ||
       MH_CreateHookApi(L"gdi32.dll","CreateFontIndirectA",hooked_ia,(void**)&original_ia)!=MH_OK ||
       MH_CreateHookApi(L"gdi32.dll","CreateFontIndirectW",hooked_iw,(void**)&original_iw)!=MH_OK ||
       MH_EnableHook(MH_ALL_HOOKS)!=MH_OK) {
        MH_DisableHook(MH_ALL_HOOKS); MH_Uninitialize();
        log_line("ERROR: native font correction unavailable.\r\n",FALSE); return TRUE;
    }
    InterlockedExchange(&ready,1);log_line("Ready: four font-creation APIs; bank and system fonts retain their own settings.\r\n",FALSE);
    return TRUE;
}
__declspec(dllexport) HRESULT WINAPI DirectDrawEnumerateExA(void *callback,void *context,DWORD flags) {
    InitOnceExecuteOnce(&once,initialize,NULL,NULL);
    return original_draw?original_draw(callback,context,flags):E_FAIL;
}
__declspec(dllexport) DWORD WINAPI PNFontStatus(void) {return (DWORD)InterlockedCompareExchange(&ready,0,0);}
static DWORD WINAPI start(void *unused) {(void)unused;InitOnceExecuteOnce(&once,initialize,NULL,NULL);return 0;}
BOOL WINAPI DllMain(HINSTANCE module,DWORD reason,void *reserved) {
    (void)reserved;
    if(reason==DLL_PROCESS_ATTACH) {self=module;DisableThreadLibraryCalls(module);HANDLE t=CreateThread(NULL,0,start,NULL,0,NULL);if(t)CloseHandle(t);}
    return TRUE;
}
