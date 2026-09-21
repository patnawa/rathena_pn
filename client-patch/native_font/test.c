#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void check(HFONT font,int h,const char *face,int width,int quality) {
    LOGFONTA f; assert(font);assert(GetObjectA(font,sizeof(f),&f));
    assert(f.lfHeight==h);assert(f.lfWidth==width);assert(!strcmp(f.lfFaceName,face));
    if(quality>=0)assert(f.lfQuality==quality);
    DeleteObject(font);
}
static BOOL WINAPI device(GUID *id,char *description,char *name,void *ctx,HMONITOR monitor) {
    (void)id;(void)description;(void)name;(void)ctx;(void)monitor;return FALSE;
}
int main(void) {
    HMODULE shim=LoadLibraryA("FontScale.dll");assert(shim);
    HRESULT (WINAPI *draw)(void*,void*,DWORD)=(void*)GetProcAddress(shim,"_DirectDrawEnumerateExA@12");
    assert(draw && SUCCEEDED(draw(device,NULL,0)));
    HMODULE font=GetModuleHandleA("FontScaleOriginal.dll");assert(font);
    DWORD (WINAPI *status)(void)=(void*)GetProcAddress(font,"PNFontStatus");assert(status && status()==1);
    for(int h=9;h<=16;++h) {
        check(CreateFontA(h,2,0,0,400,0,0,0,HANGUL_CHARSET,0,0,5,0,"Gulim"),h==14?-11:(h==13?-10:-h),"Arial",0,3);
        check(CreateFontW(-h,2,0,0,400,0,0,0,0,0,0,5,0,L"Tahoma"),-h,"Arial",0,3);
    }
    LOGFONTA a={0};a.lfHeight=12;a.lfCharSet=HANGUL_CHARSET;strcpy(a.lfFaceName,"Gulim");
    check(CreateFontIndirectA(&a),-12,"Arial",0,3);assert(a.lfHeight==12 && !strcmp(a.lfFaceName,"Gulim"));
    LOGFONTW w={0};w.lfHeight=12;wcscpy(w.lfFaceName,L"Tahoma");
    check(CreateFontIndirectW(&w),-12,"Arial",0,3);assert(w.lfHeight==12);
    /* Runtime probes distinguish bold 13-cell NPC names from regular 13-cell
     * resource values and both 11-cell inventory fonts. */
    check(CreateFontA(13,0,0,0,FW_BOLD,0,0,0,HANGUL_CHARSET,0,0,0,0,"Gulim"),-11,"Arial",0,3);
    check(CreateFontW(13,0,0,0,FW_BOLD,0,0,0,HANGUL_CHARSET,0,0,0,0,L"Gulim"),-11,"Arial",0,3);
    a.lfHeight=13;a.lfWeight=FW_BOLD;
    check(CreateFontIndirectA(&a),-11,"Arial",0,3);assert(a.lfHeight==13 && a.lfWeight==FW_BOLD);
    w.lfHeight=13;w.lfWeight=FW_BOLD;
    check(CreateFontIndirectW(&w),-11,"Arial",0,3);assert(w.lfHeight==13 && w.lfWeight==FW_BOLD);
    check(CreateFontA(13,0,0,0,FW_NORMAL,0,0,0,ANSI_CHARSET,0,0,0,0,"Arial"),-10,"Arial",0,3);
    for(int weight=FW_NORMAL;weight<=FW_BOLD;weight+=FW_BOLD-FW_NORMAL)
        check(CreateFontA(11,0,0,0,weight,0,0,0,HANGUL_CHARSET,0,0,0,0,"Gulim"),-11,"Arial",0,3);
    check(CreateFontA(24,0,0,0,400,0,0,0,0,0,0,0,0,"Tahoma"),24,"Tahoma",0,0);
    check(CreateFontA(12,0,0,0,400,0,0,0,SYMBOL_CHARSET,0,0,0,0,"Symbol"),12,"Symbol",0,0);
    check(CreateFontA(12,0,900,0,400,0,0,0,0,0,0,0,0,"Tahoma"),12,"Tahoma",0,0);
    HMODULE external=LoadLibraryA("ExternalFontTest.dll");assert(external);
    HFONT (WINAPI *external_font)(void)=(void*)GetProcAddress(external,"external_font@0");assert(external_font);
    check(external_font(),12,"Tahoma",0,0);
    DWORD deadline=GetTickCount()+5000;
    while(!GetModuleHandleA("BankUI.dll") && GetTickCount()<deadline)Sleep(10);
    assert(GetModuleHandleA("BankUI.dll"));

    HDC dc=CreateCompatibleDC(NULL);
    BITMAPINFO bi={0};bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=500;
    bi.bmiHeader.biHeight=-40;bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;
    void *bits;HBITMAP bm=CreateDIBSection(dc,&bi,DIB_RGB_COLORS,&bits,NULL,0);
    HGDIOBJ old_bm=SelectObject(dc,bm);
    HFONT body=CreateFontA(12,0,0,0,400,0,0,0,HANGUL_CHARSET,0,0,0,0,"Gulim");
    HGDIOBJ old_font=SelectObject(dc,body);char actual[80];GetTextFaceA(dc,sizeof(actual),actual);assert(!strcmp(actual,"Arial"));
    const char *text="First aid box containing simple medications.";
    SIZE extent;assert(GetTextExtentPoint32A(dc,text,(int)strlen(text),&extent));
    printf("Text extent: %ld x %ld\n",extent.cx,extent.cy);assert(extent.cx==241 && extent.cy==15);
    memset(bits,255,500*40*4);SetTextColor(dc,0);SetBkColor(dc,0xffffff);TextOutA(dc,0,0,text,(int)strlen(text));GdiFlush();
    BITMAPFILEHEADER header={0};header.bfType=0x4d42;header.bfOffBits=sizeof(header)+sizeof(BITMAPINFOHEADER);header.bfSize=header.bfOffBits+500*40*4;
    FILE *out=fopen("native-font-render.bmp","wb");assert(out);fwrite(&header,sizeof(header),1,out);fwrite(&bi.bmiHeader,sizeof(BITMAPINFOHEADER),1,out);fwrite(bits,500*40*4,1,out);fclose(out);
    SelectObject(dc,old_font);DeleteObject(body);
    HFONT panel=CreateFontA(14,0,0,0,400,0,0,0,HANGUL_CHARSET,0,0,0,0,"Gulim");
    old_font=SelectObject(dc,panel);
    const char *label="Base Lv. 68";
    memset(bits,255,500*40*4);TextOutA(dc,0,0,label,(int)strlen(label));GdiFlush();
    out=fopen("basic-font-render.bmp","wb");assert(out);fwrite(&header,sizeof(header),1,out);fwrite(&bi.bmiHeader,sizeof(BITMAPINFOHEADER),1,out);fwrite(bits,500*40*4,1,out);fclose(out);
    SelectObject(dc,old_font);DeleteObject(panel);
    HFONT values=CreateFontA(13,0,0,0,400,0,0,0,HANGUL_CHARSET,0,0,0,0,"Gulim");
    old_font=SelectObject(dc,values);label="5031  /  5031";
    memset(bits,255,500*40*4);TextOutA(dc,0,0,label,(int)strlen(label));GdiFlush();
    out=fopen("resource-font-render.bmp","wb");assert(out);fwrite(&header,sizeof(header),1,out);fwrite(&bi.bmiHeader,sizeof(BITMAPINFOHEADER),1,out);fwrite(bits,500*40*4,1,out);fclose(out);
    SelectObject(dc,old_font);DeleteObject(values);
    HFONT names=CreateFontA(13,0,0,0,FW_BOLD,0,0,0,HANGUL_CHARSET,0,0,0,0,"Gulim");
    old_font=SelectObject(dc,names);label="Kafra Employee";
    LOGFONTA name_font;assert(GetObjectA(names,sizeof(name_font),&name_font));assert(name_font.lfWeight==FW_BOLD);
    assert(GetTextExtentPoint32A(dc,label,(int)strlen(label),&extent));
    printf("NPC name extent: %ld x %ld (previous runtime font: 77 x 12)\n",extent.cx,extent.cy);
    assert(extent.cx>77 && extent.cy>12);
    memset(bits,255,500*40*4);TextOutA(dc,0,0,label,(int)strlen(label));GdiFlush();
    out=fopen("npc-name-render.bmp","wb");assert(out);fwrite(&header,sizeof(header),1,out);fwrite(&bi.bmiHeader,sizeof(BITMAPINFOHEADER),1,out);fwrite(bits,500*40*4,1,out);fclose(out);
    SelectObject(dc,old_font);DeleteObject(names);SelectObject(dc,old_bm);DeleteObject(bm);DeleteDC(dc);
    puts("PASS: native font creation, GDI text metrics, size hierarchy, system/bank exclusions, DirectDraw forwarding and bank loading");
    return 0;
}
