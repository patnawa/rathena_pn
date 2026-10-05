// Independent native HUD support for the pinned Basic Info wallet render path.
#include "bank_client.hpp"
#include "native_zeny.hpp"
#include <wincrypt.h>
#include <cstring>
#include <string>
#include <algorithm>
#include <cstdio>
#include <cstdarg>
#include <vector>
#include "../account_bank/vendor/MinHook/include/MinHook.h"

namespace native_zeny_detail {
struct Snapshot {LONG generation=0;int64_t wallet=0;bool valid=false;};
SRWLOCK cache_lock=SRWLOCK_INIT;
Snapshot cache;
uintptr_t image_base=0;
volatile LONG executable_verified=0;
BOOL (WINAPI* original_text)(HDC,int,int,LPCWSTR,int)=TextOutW;
BOOL (WINAPI* original_measure)(HDC,LPCWSTR,int,LPSIZE)=GetTextExtentPoint32W;
volatile LONG diagnostics=0;
void trace(const char* format,...) {
    if(!InterlockedCompareExchange(&diagnostics,0,0))return;
    static SRWLOCK lock=SRWLOCK_INIT;static unsigned count=0;
    AcquireSRWLockExclusive(&lock);
    if(count++>=256){ReleaseSRWLockExclusive(&lock);return;}
    wchar_t path[MAX_PATH]{};HMODULE module=nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&trace),&module);
    GetModuleFileNameW(module,path,MAX_PATH);auto slash=wcsrchr(path,L'\\');if(slash)wcscpy(slash+1,L"PNNativeZeny.log");
    char message[1200]{};va_list args;va_start(args,format);vsnprintf(message,sizeof(message)-3,format,args);va_end(args);strcat(message,"\r\n");
    const HANDLE file=CreateFileW(path,FILE_APPEND_DATA,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file!=INVALID_HANDLE_VALUE){DWORD written;WriteFile(file,message,DWORD(strlen(message)),&written,nullptr);CloseHandle(file);}
    ReleaseSRWLockExclusive(&lock);
}


// Captured normal/AP BasicInfo constructor, resize and draw chains own their
// surfaces. No addresses from another executable build are used here.
constexpr int row_height=15, old_ap_height=149, new_ap_height=164;
struct RowContext {void* owner=nullptr;uintptr_t caller=0;int height=0,length=0;wchar_t source[48]{};};
thread_local RowContext active_row;
bool numeric_wallet_suffix(const wchar_t* source,int length);
volatile LONG layout_installed=0;
using NativeCtor=void* (__attribute__((thiscall)) *)(void*);
using NativeResize=void (__attribute__((thiscall)) *)(void*,int);
using NativeRightText=void (__attribute__((thiscall)) *)(void*,int,int,const char*,int,int,int,int,int);
NativeCtor original_ctor=nullptr;
NativeResize original_resize=nullptr;
NativeRightText original_right_text=nullptr;
using NativeDraw=void (__attribute__((thiscall)) *)(void*);
NativeDraw original_draw=nullptr;
volatile LONG layout_attempt=0;

bool code_matches(uintptr_t rva,const BYTE* expected,size_t length) {
    BYTE actual[64]{};SIZE_T read=0;
    return length<=sizeof(actual)&&ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(image_base+rva),actual,length,&read)&&read==length&&!memcmp(actual,expected,length);
}
struct CodeGuard {uintptr_t rva;std::vector<BYTE> bytes;};
const std::vector<CodeGuard>& layout_guards() {
    static const std::vector<CodeGuard> guards={
        {0x597250,{0x55,0x8b,0xec,0x6a,0xff}},
        {0x5972b7,{0xc7,0x86,0xb8,0,0,0,0x86,0,0,0}},
        {0x5972d5,{0xc7,0x86,0x14,1,0,0,0x95,0,0,0}},
        {0x5972f3,{0xc7,0x86,0x08,1,0,0,0x86,0,0,0}},
        {0x5974f0,{0x55,0x8b,0xec,0x6a,0xff}},
        {0x597c0d,{0xc3}},
        {0x598810,{0x55,0x8b,0xec,0x81,0xec,0xdc,0,0,0}},
        {0x599219,{0x8b,0x83,0xf8,0,0,0,0x8b,0xcb,0x83,0xc0,0x67}},
        {0x59922c,{0xe8,0x0f,0xd2,0x0e,0}},
        {0x5992b8,{0x8b,0x43,0x14,0x2b,0xc7,0x83,0xe8,0x0a,0x50}},
        {0x5992c1,{0xe8,0x7a,0xd1,0x0e,0}},
        {0x5992d6,{0xc3}},
        {0x5981a0,{0x55,0x8b,0xec,0x51,0x8b,0x45,0x08,0x56,0x57,0x8b,0xf9}},
        {0x59824f,{0x8b,0x8f,0xb8,0,0,0,0x3b,0xc1,0x0f,0x85,0x0a,1,0,0}},
        {0x59825d,{0x8b,0x07,0x51,0xff,0x77,0x14,0x8b,0xcf,0xff,0x50,0x04}},
        {0x5979db,{0x8b,0x8f,0xf8,0,0,0,0x0f,0x43,0x55,0x80,0x83,0xc1,0x67}},
        {0x5979fd,{0xe8,0x3e,0xea,0x0e,0}},
        {0x597b38,{0x2b,0x85,0x4c,0xff,0xff,0xff}},
        {0x597b4a,{0xe8,0xf1,0xe8,0x0e,0}},
        {0x686440,{0x55,0x8b,0xec,0x6a,0xff}},
        {0x6864d2,{0x8b,0x45,0x08,0x8d,0x4d,0xcc,0x2b,0x45,0xec}},
        {0x686505,{0xc2,0x20,0}}
    };return guards;
}
bool layout_code_verified() {
    for(const auto& guard:layout_guards())if(!code_matches(guard.rva,guard.bytes.data(),guard.bytes.size()))return false;
    return true;
}
int field(void* object,size_t offset) {int value;memcpy(&value,static_cast<BYTE*>(object)+offset,sizeof(value));return value;}
void set_field(void* object,size_t offset,int value) {memcpy(static_cast<BYTE*>(object)+offset,&value,sizeof(value));}
int object_layout(void* object) {
    if(!object||static_cast<uint32_t>(field(object,0))!=image_base+0xd46f5c)return 0;
    const int offset=field(object,0xf8),compact=field(object,0xb4);
    const int old_height=offset==28&&compact==71&&field(object,0x110)==71?149:
        offset==13&&compact==55&&field(object,0x104)==55?134:0;
    if(!old_height)return 0;
    const int expanded=field(object,0xb8),saved=field(object,old_height==149?0x114:0x108);
    return (expanded==old_height||expanded==old_height+row_height)&&
        (saved==old_height||saved==old_height+row_height)?old_height:0;
}
bool expand_object(void* object) {
    const int old_height=object_layout(object);if(!old_height)return false;
    set_field(object,0xb8,old_height+row_height);set_field(object,old_height==149?0x114:0x108,old_height+row_height);return true;
}
void* __attribute__((fastcall)) hooked_ctor(void* object,void*) {
    void* result=original_ctor(object);
    if(InterlockedCompareExchange(&layout_installed,0,0)&&expand_object(result))trace("layout constructor expanded height=%d",field(result,0xb8));
    return result;
}
void __attribute__((fastcall)) hooked_resize(void* object,void*,int height) {
    const int old_height=InterlockedCompareExchange(&layout_installed,0,0)?object_layout(object):0;
    const bool supported=old_height&&(height==field(object,0xb4)||height==old_height||height==old_height+row_height);
    if(!supported) {original_resize(object,height);return;}
    const size_t saved_offset=old_height==149?0x114:0x108;
    const int before_height=field(object,0x18),expanded=field(object,0xb8),saved=field(object,saved_offset);
    expand_object(object);if(height==old_height)height=old_height+row_height;
    // Native allocation and child/button-strip layout retain their own method.
    original_resize(object,height);
    if(field(object,0x18)!=height) {
        const bool old_owner=before_height==old_height;
        set_field(object,0xb8,old_owner?old_height:expanded);
        set_field(object,saved_offset,old_owner?old_height:saved);
        if(field(object,0x18)!=before_height)original_resize(object,before_height);
        trace("layout resize failed; prior dimensions restored");
    }
}
void __attribute__((fastcall)) hooked_draw(void* object,void*) {
    // A late install may encounter an already-created owner. Migrate only at
    // its native draw entry on the game thread, before its text calls.
    const int old_height=InterlockedCompareExchange(&layout_installed,0,0)?object_layout(object):0;
    if(old_height&&field(object,0x14)==220&&field(object,0x18)==old_height) {
        hooked_resize(object,nullptr,old_height);
        if(field(object,0x18)==old_height+row_height)trace("layout existing owner migrated height=%d",old_height+row_height);
    }
    original_draw(object);
}
int route_row(void* object,uintptr_t caller,int& right,int& y) {
    const int old_height=InterlockedCompareExchange(&layout_installed,0,0)?object_layout(object):0;
    if(!old_height||field(object,0x18)!=old_height+row_height||field(object,0x14)!=220)return 0;
    const bool wallet=caller==uintptr_t(old_height==149?0x597a02:0x599231);
    const bool weight=caller==uintptr_t(old_height==149?0x597b4f:0x5992c6);
    if(wallet) {y+=row_height;return old_height+row_height;}
    if(weight)right=field(object,0x14)-5;
    return 0;
}
RowContext row_context(void* object,uintptr_t caller,int height,const char* text,int count) {
    RowContext next;
    if(!height||!text||count<0||count>=48)return next;
    int length=count;
    if(!length) {while(length<48&&text[length])++length;if(length>=48)return next;}
    for(int i=0;i<length;++i) {const unsigned char c=static_cast<unsigned char>(text[i]);if(!c||c>127)return {};next.source[i]=c;}
    if(!numeric_wallet_suffix(next.source,length))return {};
    next.owner=object;next.caller=caller;next.height=height;next.length=length;return next;
}
bool row_context_valid(const RowContext& row) {
    if(!InterlockedCompareExchange(&layout_installed,0,0)||!row.height||!row.owner||row.length<=0||row.length>=48||!numeric_wallet_suffix(row.source,row.length))return false;
    const int layout=object_layout(row.owner);
    return layout&&row.height==layout+row_height&&field(row.owner,0x18)==row.height&&field(row.owner,0x14)==220&&
        row.caller==uintptr_t(layout==149?0x597a02:0x599231);
}
bool row_source_matches(const wchar_t* text,int count) {
    return text&&count==active_row.length&&count>0&&count<48&&!wmemcmp(text,active_row.source,count);
}
void __attribute__((fastcall)) hooked_right_text(void* object,void*,int right,int y,const char* text,int count,int font,int height,int color,int extra) {
    const auto caller=reinterpret_cast<uintptr_t>(__builtin_return_address(0))-image_base;
    const RowContext prior_row=active_row;
    const int row=route_row(object,caller,right,y);
    // An unrelated nested right-text call must clear the outer authorization.
    // Scope the token to this exact native caller, owner, dimensions and text.
    active_row=row_context(object,caller,row,text,count);
    original_right_text(object,right,y,text,count,font,height,color,extra);
    active_row=prior_row;
}
bool install_layout_hooks() {
    if(InterlockedCompareExchange(&layout_installed,0,0))return true;
    if(InterlockedCompareExchange(&layout_attempt,1,0))return false;
    if(!layout_code_verified()){trace("layout waiting: decoded instruction guards do not match");InterlockedExchange(&layout_attempt,0);return false;}
    void* targets[]={reinterpret_cast<void*>(image_base+0x597250),reinterpret_cast<void*>(image_base+0x5981a0),reinterpret_cast<void*>(image_base+0x686440),reinterpret_cast<void*>(image_base+0x598810)};
    void* hooks[]={reinterpret_cast<void*>(hooked_ctor),reinterpret_cast<void*>(hooked_resize),reinterpret_cast<void*>(hooked_right_text),reinterpret_cast<void*>(hooked_draw)};
    void** originals[]={reinterpret_cast<void**>(&original_ctor),reinterpret_cast<void**>(&original_resize),reinterpret_cast<void**>(&original_right_text),reinterpret_cast<void**>(&original_draw)};
    size_t created=0;
    for(;created<std::size(targets);++created)if(MH_CreateHook(targets[created],hooks[created],originals[created])!=MH_OK)break;
    if(created!=std::size(targets)) {while(created)MH_RemoveHook(targets[--created]);trace("layout disabled: hook creation failed");return false;}
    for(auto target:targets)MH_QueueEnableHook(target);
    if(MH_ApplyQueued()!=MH_OK){
        // A queued activation can fail after enabling a subset. Behavior stays
        // gated off until all succeed. Retain trampolines even after disabling:
        // a callback already in flight must be able to return safely.
        for(auto target:targets)MH_DisableHook(target);
        trace("layout disabled: hook activation failed; passthrough trampolines retained");return false;
    }
    InterlockedExchange(&layout_installed,1);trace("layout installed: pinned normal149/AP164 independent right-aligned Weight and Zeny rows");return true;
}

bool candidate_text(const wchar_t* source,int length) {
    static constexpr wchar_t capped[]=L" | Zeny: 2,147,483,647";
    return source&&length==int(std::size(capped)-1)&&!wmemcmp(source,capped,length);
}
bool numeric_wallet_suffix(const wchar_t* source,int length) {
    if(!source||length<6||length>40)return false;
    int at=0;while(at<length&&(source[at]==L' '||source[at]==L'|'))++at;
    if(at+4>=length||wmemcmp(source+at,L"Zeny",4))return false;
    at+=4;while(at<length&&(source[at]==L' '||source[at]==L':'))++at;
    bool digit=false;
    for(;at<length;++at) {if(source[at]>=L'0'&&source[at]<=L'9')digit=true;else if(source[at]!=L',')return false;}
    return digit;
}
std::wstring full_row_text(int64_t wallet) {
    if(wallet<0)return L"Zeny: --";
    auto number=std::to_wstring(wallet);
    for(int at=int(number.size())-3;at>0;at-=3)number.insert(at,1,L',');
    return L"Zeny: "+number;
}
bool separate_surface(HDC dc,BITMAP& bitmap) {
    return GetObject(GetCurrentObject(dc,OBJ_BITMAP),sizeof(bitmap),&bitmap)==sizeof(bitmap)&&bitmap.bmWidth==220&&(bitmap.bmHeight==149||bitmap.bmHeight==164);
}
bool row_clip_available(HDC dc,int top,int bottom) {
    RECT bounds{};const int kind=GetClipBox(dc,&bounds);
    if(kind==ERROR||kind==NULLREGION||bounds.left>0||bounds.top>top||bounds.right<220||bounds.bottom<bottom) {
        trace("layout row clipped kind=%d bounds=%ld,%ld,%ld,%ld expected=0,%d,220,%d",kind,bounds.left,bounds.top,bounds.right,bounds.bottom,top,bottom);return false;
    }
    HRGN clip=CreateRectRgn(0,0,0,0),required=CreateRectRgn(0,top,220,bottom);
    if(!clip||!required){if(clip)DeleteObject(clip);if(required)DeleteObject(required);return false;}
    const int clipped=GetClipRgn(dc,clip);
    const bool complete=clipped==0||(clipped==1&&CombineRgn(required,required,clip,RGN_DIFF)==NULLREGION);
    DeleteObject(required);DeleteObject(clip);return complete;
}
BOOL draw_separate_row(HDC dc,int y,int64_t wallet,bool& changed) {
    changed=false;BITMAP bitmap{};SIZE size{};const auto text=full_row_text(wallet);
    if(!separate_surface(dc,bitmap)||y!=bitmap.bmHeight-18||!original_measure(dc,text.data(),int(text.size()),&size)||size.cx<=0||size.cx>210||size.cy<=0||y+size.cy>bitmap.bmHeight-2)return FALSE;
    if(!row_clip_available(dc,y,bitmap.bmHeight))return FALSE;
    const int saved=SaveDC(dc);if(!saved)return FALSE;
    if(GetGraphicsMode(dc)==GM_ADVANCED) {XFORM t{};if(!GetWorldTransform(dc,&t)||t.eM11!=1||t.eM22!=1||t.eM12||t.eM21||t.eDx||t.eDy){RestoreDC(dc,saved);return FALSE;}}
    // Preserve the original skin's bottom and side borders. Extend its plain
    // interior, then draw natural-size Zeny with the native five-pixel right
    // margin, matching Weight. Do not broaden a caller's clipping region.
    const int old_height=bitmap.bmHeight-row_height;
    BOOL result=BitBlt(dc,0,bitmap.bmHeight-1,220,1,dc,0,old_height-1,SRCCOPY);
    if(result) {SetStretchBltMode(dc,COLORONCOLOR);result=StretchBlt(dc,0,old_height-1,220,row_height,dc,0,old_height-2,220,1,SRCCOPY);}
    if(result)result=original_text(dc,220-5-size.cx,y,text.data(),int(text.size()));
    RestoreDC(dc,saved);changed=result!=FALSE;return result;
}

bool wallet_text(const wchar_t* source,int length,int64_t wallet,std::wstring& output) {
    output.clear();
    if((wallet!=-1&&wallet<=INT32_MAX)||!candidate_text(source,length))return false;
    if(wallet==-1){output=L" | Zeny: --";return true;}
    auto number=std::to_wstring(wallet);
    for(int at=int(number.size())-3;at>0;at-=3)number.insert(at,1,L',');
    output=L" | Zeny: "+number;return true;
}

bool wallet_stack(const uintptr_t* frames,size_t count,bool measurement) {
    // These sequences were captured from the actual native Basic Info wallet
    // suffix. Generic font callers alone are shared with unrelated game text.
    static constexpr uintptr_t draw[]={0x15074d,0x1501c0,0x15035d,0x6864e6,0x597a02};
    static constexpr uintptr_t measure[]={0x150613,0x14fd31,0x6864c7,0x597a02};
    const auto* expected=measurement?measure:draw;
    const size_t needed=measurement?std::size(measure):std::size(draw);
    return count>=needed&&std::equal(expected,expected+needed-1,frames)&&
        (frames[needed-1]==0x597a02||frames[needed-1]==0x599231);
}

bool attested_wallet_stack(const uintptr_t* frames,size_t count,bool measurement,const RowContext& row) {
    if(!row.height)return wallet_stack(frames,count,measurement);
    if(!row_context_valid(row))return false;
    // GCC/native mixed frames can omit the original caller after detouring.
    // The scoped token supplies that exact caller; retain every shared native
    // GDI/text frame and the independent imported-call instruction check.
    static constexpr uintptr_t draw[]={0x15074d,0x1501c0,0x15035d,0x6864e6};
    static constexpr uintptr_t measure[]={0x150613,0x14fd31,0x6864c7};
    const auto* expected=measurement?measure:draw;
    const size_t needed=measurement?std::size(measure):std::size(draw);
    return count>=needed&&std::equal(expected,expected+needed,frames);
}

bool native_caller(bool measurement) {
    if(!InterlockedCompareExchange(&executable_verified,0,0)||!image_base)return false;
    void* captured[24]{};const auto count=CaptureStackBackTrace(0,24,captured,nullptr);
    uintptr_t frames[24]{};size_t used=0;
    for(unsigned i=0;i<count;++i) {
        const uintptr_t address=reinterpret_cast<uintptr_t>(captured[i]);
        if(address>=image_base&&address<image_base+0x460c000)frames[used++]=address-image_base;
    }
    const bool matched=attested_wallet_stack(frames,used,measurement,active_row);
    if(InterlockedCompareExchange(&diagnostics,0,0)) {
    char stack[512]{};int at=0;for(size_t i=0;i<used&&at<490;++i)at+=snprintf(stack+at,sizeof(stack)-at," %08lx",static_cast<unsigned long>(frames[i]));
    trace("candidate api=%s stack_match=%u stack=%s",measurement?"measure":"draw",unsigned(matched),stack);
    }
    if(!matched)return false;
    // Verify the executing unpacked call instruction as well as the PE hash.
    // Its absolute IAT operand is rebased, so compare against this image base.
    const uintptr_t caller=image_base+(measurement?0x150613:0x15074d);
    BYTE instruction[6]{};SIZE_T read=0;
    if(!ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(caller-6),instruction,sizeof(instruction),&read)||read!=sizeof(instruction))return false;
    uintptr_t target=0;memcpy(&target,instruction+2,sizeof(uint32_t));
    const bool valid=instruction[0]==0xff&&instruction[1]==0x15&&target==image_base+(measurement?0xcc70e0:0xcc712c);
    trace("instruction valid=%u target=%08lx",unsigned(valid),static_cast<unsigned long>(target));return valid;
}

bool native_surface(HDC dc,BITMAP& bitmap) {
    return GetObject(GetCurrentObject(dc,OBJ_BITMAP),sizeof(bitmap),&bitmap)==sizeof(bitmap)&&bitmap.bmWidth==220&&bitmap.bmHeight==149;
}

bool fitted_measure(HDC dc,const wchar_t* source,int length,int64_t wallet,SIZE& fitted,std::wstring& output) {
    BITMAP bitmap{};
    if(!native_surface(dc,bitmap)||!wallet_text(source,length,wallet,output))return false;
    // This native window has a fixed-width bottom row. Keep the original
    // suffix allocation and fit the complete replacement into those pixels.
    return original_measure(dc,source,length,&fitted)&&fitted.cx>0;
}

BOOL draw_fitted(HDC dc,int x,int y,const wchar_t* source,int length,int64_t wallet,bool& changed) {
    changed=false;SIZE fitted{},full{};std::wstring output;BITMAP bitmap{};
    if(!fitted_measure(dc,source,length,wallet,fitted,output)||!native_surface(dc,bitmap)||!original_measure(dc,output.data(),int(output.size()),&full))return FALSE;
    if(x<0||x>=bitmap.bmWidth-2||y<0||y+full.cy>bitmap.bmHeight||full.cx<=0)return FALSE;
    const int width=std::min<int>(fitted.cx,bitmap.bmWidth-x-2);
    const int saved=SaveDC(dc);if(!saved)return FALSE;
    if(GetGraphicsMode(dc)==GM_ADVANCED) {
        XFORM transform{};
        if(!GetWorldTransform(dc,&transform)||transform.eM11!=1||transform.eM22!=1||transform.eM12||transform.eM21||transform.eDx||transform.eDy){RestoreDC(dc,saved);return FALSE;}
    }
    // Render all glyphs before fitting. GDI's per-character font rounding can
    // overflow a floating-point world transform even when its extent fits.
    // A complete source raster avoids clipping the final digits at INT64_MAX.
    const int source_width=std::max<int>(full.cx+2,width);
    HDC scratch=CreateCompatibleDC(dc);HBITMAP raster=CreateCompatibleBitmap(dc,source_width,full.cy);
    if(!scratch||!raster){if(scratch)DeleteDC(scratch);if(raster)DeleteObject(raster);RestoreDC(dc,saved);return FALSE;}
    const auto old_bitmap=SelectObject(scratch,raster);
    const auto old_font=SelectObject(scratch,GetCurrentObject(dc,OBJ_FONT));
    SetTextColor(scratch,GetTextColor(dc));SetBkColor(scratch,GetBkColor(dc));SetBkMode(scratch,GetBkMode(dc));
    SetStretchBltMode(scratch,COLORONCOLOR);
    BOOL result=StretchBlt(scratch,0,0,source_width,full.cy,dc,x,y,width,full.cy,SRCCOPY);
    if(result)result=original_text(scratch,0,0,output.data(),int(output.size()));
    if(result) {
        SetStretchBltMode(dc,HALFTONE);SetBrushOrgEx(dc,0,0,nullptr);
        result=StretchBlt(dc,x,y,width,full.cy,scratch,0,0,source_width,full.cy,SRCCOPY);
    }
    SelectObject(scratch,old_font);SelectObject(scratch,old_bitmap);DeleteObject(raster);DeleteDC(scratch);
    RestoreDC(dc,saved);changed=result!=FALSE;return result;
}

bool read_snapshot(Snapshot& output) {
    AcquireSRWLockShared(&cache_lock);Snapshot copy=cache;ReleaseSRWLockShared(&cache_lock);
    // Never take the transport/session lock while holding the render cache
    // lock. Validate the generation after the copy to exclude another login.
    if(!copy.valid||copy.generation<=0||copy.wallet<0||!bank_current_generation(copy.generation)) {output={};return false;}
    output=copy;return true;
}

BOOL WINAPI hooked_measure(HDC dc,LPCWSTR text,int count,LPSIZE size) {
    Snapshot snapshot;SIZE fitted{};std::wstring output;
    if(size&&(active_row.height?row_source_matches(text,count):numeric_wallet_suffix(text,count))&&native_caller(true)) {
        const int64_t wallet=read_snapshot(snapshot)?snapshot.wallet:-1;
        BITMAP bitmap{};
        if(separate_surface(dc,bitmap)&&active_row.height==bitmap.bmHeight) {const auto full=full_row_text(wallet);return original_measure(dc,full.data(),int(full.size()),size);}
        if(fitted_measure(dc,text,count,wallet,fitted,output)){*size=fitted;return TRUE;}
    }
    return original_measure(dc,text,count,size);
}
BOOL WINAPI hooked_text(HDC dc,int x,int y,LPCWSTR text,int count) {
    Snapshot snapshot;
    if((active_row.height?row_source_matches(text,count):numeric_wallet_suffix(text,count))&&native_caller(false)) {
        if(!InterlockedCompareExchange(&layout_installed,0,0))install_layout_hooks();
        const int64_t wallet=read_snapshot(snapshot)?snapshot.wallet:-1;
        bool changed=false;BITMAP bitmap{};
        const BOOL result=(separate_surface(dc,bitmap)&&active_row.height==bitmap.bmHeight)?draw_separate_row(dc,y,wallet,changed):draw_fitted(dc,x,y,text,count,wallet,changed);
        trace("draw wallet=%lld xy=%d,%d changed=%u",static_cast<long long>(wallet),x,y,unsigned(changed));
        if(changed)return result;
    }
    return original_text(dc,x,y,text,count);
}

bool pinned_executable(const wchar_t* path) {
    static const BYTE expected[]={0x73,0xf7,0x2f,0xea,0x24,0x58,0xa4,0xc2,0xdf,0xd1,0xfc,0xc5,0xa2,0xa8,0xf7,0x68,0x44,0x85,0xae,0xcd,0x52,0xd6,0x12,0xa3,0xe6,0x8a,0x27,0xc8,0x28,0xb4,0x63,0x2d};
    HANDLE file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE)return false;
    HCRYPTPROV provider=0;HCRYPTHASH hash=0;bool valid=false;
    if(CryptAcquireContextW(&provider,nullptr,nullptr,PROV_RSA_AES,CRYPT_VERIFYCONTEXT)&&CryptCreateHash(provider,CALG_SHA_256,0,0,&hash)) {
        BYTE buffer[65536];DWORD count=0;bool complete=true;
        for(;;) {
            if(!ReadFile(file,buffer,sizeof(buffer),&count,nullptr)){complete=false;break;}
            if(!count)break;
            if(!CryptHashData(hash,buffer,count,0)){complete=false;break;}
        }
        BYTE digest[32]{};DWORD length=sizeof(digest);
        valid=complete&&CryptGetHashParam(hash,HP_HASHVAL,digest,&length,0)&&length==sizeof(expected)&&!memcmp(digest,expected,sizeof(expected));
    }
    if(hash)CryptDestroyHash(hash);
    if(provider)CryptReleaseContext(provider,0);
    CloseHandle(file);
    return valid;
}
}

void native_zeny_update(LONG generation,int64_t wallet,bool valid) {
    // A delayed response from an old character cannot replace or clear a
    // current character's cache. Explicit reset handles session transitions.
    if(generation>0&&!bank_current_generation(generation))return;
    native_zeny_detail::Snapshot next;
    if(valid&&generation>0&&wallet>=0)next={generation,wallet,true};
    AcquireSRWLockExclusive(&native_zeny_detail::cache_lock);native_zeny_detail::cache=next;ReleaseSRWLockExclusive(&native_zeny_detail::cache_lock);
}
void native_zeny_reset() {
    AcquireSRWLockExclusive(&native_zeny_detail::cache_lock);native_zeny_detail::cache={};ReleaseSRWLockExclusive(&native_zeny_detail::cache_lock);
}
void native_zeny_install() {
    using namespace native_zeny_detail;
    static volatile LONG attempted=0;if(InterlockedCompareExchange(&attempted,1,0))return;
    wchar_t path[MAX_PATH]{};HMODULE self=nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&native_zeny_install),&self);
    GetModuleFileNameW(self,path,MAX_PATH);auto slash=wcsrchr(path,L'\\');
    if(slash){wcscpy(slash+1,L"PNWallet64.ini");InterlockedExchange(&diagnostics,GetPrivateProfileIntW(L"Bank",L"Diagnostics",0,path)!=0);}
    if(!GetModuleFileNameW(nullptr,path,MAX_PATH)||!pinned_executable(path)){trace("install rejected executable");return;}
    image_base=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    const auto* dos=reinterpret_cast<IMAGE_DOS_HEADER*>(image_base);
    const auto* nt=reinterpret_cast<IMAGE_NT_HEADERS*>(image_base+dos->e_lfanew);
    if(nt->OptionalHeader.SizeOfImage!=0x460c000)return;
    const auto initialized=MH_Initialize();
    if(initialized!=MH_OK&&initialized!=MH_ERROR_ALREADY_INITIALIZED)return;
    auto module=GetModuleHandleW(L"gdi32.dll");
    void* text=reinterpret_cast<void*>(GetProcAddress(module,"TextOutW"));
    void* measure=reinterpret_cast<void*>(GetProcAddress(module,"GetTextExtentPoint32W"));
    if(MH_CreateHook(text,reinterpret_cast<void*>(hooked_text),reinterpret_cast<void**>(&original_text))!=MH_OK)return;
    if(MH_CreateHook(measure,reinterpret_cast<void*>(hooked_measure),reinterpret_cast<void**>(&original_measure))!=MH_OK){MH_RemoveHook(text);return;}
    // Both hooks become active together. Do not enable other modules' hooks.
    MH_QueueEnableHook(text);MH_QueueEnableHook(measure);
    if(MH_ApplyQueued()!=MH_OK){MH_RemoveHook(text);MH_RemoveHook(measure);return;}
    InterlockedExchange(&executable_verified,1);
    trace("installed pinned native wallet hooks pid=%lu",GetCurrentProcessId());
    install_layout_hooks();
}
