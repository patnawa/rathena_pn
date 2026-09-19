#define WIN32_LEAN_AND_MEAN
#include <windows.h>
__declspec(dllexport) HFONT WINAPI external_font(void) {
    return CreateFontA(12,0,0,0,400,0,0,0,0,0,0,0,0,"Tahoma");
}
