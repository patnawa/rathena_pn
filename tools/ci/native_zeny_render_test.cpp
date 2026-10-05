#include "../../client-patch/wide_zeny/native_zeny.cpp"
#include <cassert>
#include <vector>
#include <iostream>
#include <fstream>
bool bank_current_generation(LONG generation,bool){return generation==7;}
namespace {
int resize_count=0,draw_count=0,last_resize=0;bool resize_success=true;
void __attribute__((thiscall)) fixture_resize(void* object,int height) {++resize_count;last_resize=height;if(resize_success)native_zeny_detail::set_field(object,0x18,height);}
void __attribute__((thiscall)) fixture_draw(void*) {++draw_count;}
void* __attribute__((thiscall)) fixture_ctor(void* object) {return object;}
bool nested_cleared=false;
void __attribute__((thiscall)) fixture_right_text(void*,int,int,const char*,int,int,int,int,int) {nested_cleared=native_zeny_detail::active_row.height==0;}

}
int main(int argc,char** argv) {
    using namespace native_zeny_detail;
    static constexpr uintptr_t draw[]={0x15074d,0x1501c0,0x15035d,0x6864e6,0x597a02};
    static constexpr uintptr_t measure[]={0x150613,0x14fd31,0x6864c7,0x597a02};
    assert(wallet_stack(draw,std::size(draw),false)&&wallet_stack(measure,std::size(measure),true));
    auto other=std::vector<uintptr_t>(std::begin(draw),std::end(draw));other[3]=0x6843df;
    assert(!wallet_stack(other.data(),other.size(),false)&&!wallet_stack(draw,3,false));
    HDC dc=CreateCompatibleDC(nullptr);BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=220;info.bmiHeader.biHeight=-149;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;
    BYTE* pixels=nullptr;auto bitmap=CreateDIBSection(dc,&info,DIB_RGB_COLORS,reinterpret_cast<void**>(&pixels),nullptr,0);auto old_bitmap=SelectObject(dc,bitmap);
    auto font=CreateFontW(-11,0,0,0,400,0,0,0,ANSI_CHARSET,0,0,NONANTIALIASED_QUALITY,0,L"Arial");auto old_font=SelectObject(dc,font);
    constexpr int bytes=220*149*4;const wchar_t* capped=L" | Zeny: 2,147,483,647";const int count=int(wcslen(capped));
    SetTextColor(dc,RGB(0,0,0));SetBkColor(dc,RGB(255,255,255));SetBkMode(dc,OPAQUE);
    auto reset=[&](){GdiFlush();memset(pixels,255,bytes);};
    auto snapshot=[&](){GdiFlush();return std::vector<BYTE>(pixels,pixels+bytes);};
    for(const auto wallet:{16709925001LL,9007199254740993LL,INT64_MAX}) {
        reset();assert(TextOutW(dc,107,131,capped,count));auto broken=snapshot();
        reset();bool changed=false;assert(draw_fitted(dc,107,131,capped,count,wallet,changed)&&changed);auto actual=snapshot();
        assert(actual!=broken); // Red-capable: the original capped raster fails.
        SIZE fitted{},full{};std::wstring replacement;
        assert(fitted_measure(dc,capped,count,wallet,fitted,replacement));
        assert(GetTextExtentPoint32W(dc,replacement.data(),int(replacement.size()),&full));
        assert(replacement==L" | Zeny: "+std::wstring(wallet==INT64_MAX?L"9,223,372,036,854,775,807":wallet==9007199254740993LL?L"9,007,199,254,740,993":L"16,709,925,001"));
        assert(full.cx>fitted.cx&&GetGraphicsMode(dc)==GM_COMPATIBLE);
        reset();HDC reference=CreateCompatibleDC(dc);auto reference_bitmap=CreateCompatibleBitmap(dc,full.cx+2,full.cy);
        auto reference_old=SelectObject(reference,reference_bitmap);auto reference_font=SelectObject(reference,font);
        RECT white{0,0,full.cx+2,full.cy};FillRect(reference,&white,static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH)));
        SetTextColor(reference,RGB(0,0,0));SetBkColor(reference,RGB(255,255,255));
        assert(TextOutW(reference,0,0,replacement.data(),int(replacement.size())));
        const int saved=SaveDC(dc);SetStretchBltMode(dc,HALFTONE);SetBrushOrgEx(dc,0,0,nullptr);
        assert(StretchBlt(dc,107,131,std::min<LONG>(fitted.cx,111),full.cy,reference,0,0,full.cx+2,full.cy,SRCCOPY));RestoreDC(dc,saved);
        SelectObject(reference,reference_font);SelectObject(reference,reference_old);DeleteObject(reference_bitmap);DeleteDC(reference);
        assert(actual==snapshot()); // Exact complete-text native GDI raster.
        for(int y=0;y<149;++y)for(int x=0;x<220;++x)if(y<131||y>=131+full.cy||x<107||x>=218)
            for(int channel=0;channel<3;++channel)if(actual[(y*220+x)*4+channel]!=255){std::cerr<<"outside x="<<x<<" y="<<y<<" value="<<int(actual[(y*220+x)*4+channel])<<"\n";assert(false);}
        std::wcout<<L"wallet="<<wallet<<L" native_full_width="<<full.cx<<L" fitted_width="<<std::min<LONG>(fitted.cx,111)<<L" height="<<full.cy<<L"\n";
        if(wallet==INT64_MAX) {
            BITMAPFILEHEADER header{};header.bfType=0x4d42;header.bfOffBits=sizeof(header)+sizeof(info.bmiHeader);header.bfSize=header.bfOffBits+bytes;
            std::ofstream artifact("native-zeny-int64-max.bmp",std::ios::binary);artifact.write(reinterpret_cast<const char*>(&header),sizeof(header));artifact.write(reinterpret_cast<const char*>(&info.bmiHeader),sizeof(info.bmiHeader));artifact.write(reinterpret_cast<const char*>(actual.data()),bytes);
        }
    }
    // Same exact capped text without attested game caller is untouched.
    native_zeny_update(7,INT64_MAX,true);reset();assert(hooked_text(dc,107,131,capped,count));auto unrelated=snapshot();reset();assert(TextOutW(dc,107,131,capped,count));assert(unrelated==snapshot());
    SIZE a{},b{};assert(hooked_measure(dc,capped,count,&a)&&GetTextExtentPoint32W(dc,capped,count,&b)&&a.cx==b.cx&&a.cy==b.cy);
    bool changed=false;assert(!draw_fitted(dc,107,131,capped,count,INT32_MAX,changed)&&!changed);
    assert(!draw_fitted(dc,107,145,capped,count,INT64_MAX,changed)&&!changed);
    std::wstring rejected;assert(!wallet_text(L"Zeny: 2,147,483,647",18,INT64_MAX,rejected));
    assert(wallet_text(capped,count,-1,rejected)&&rejected==L" | Zeny: --");
    reset();assert(draw_fitted(dc,107,131,capped,count,-1,changed)&&changed);
    SelectObject(dc,old_font);DeleteObject(font);SelectObject(dc,old_bitmap);DeleteObject(bitmap);DeleteDC(dc);
    // Constructor, old saved-height, collapse/re-expand and late-owner migration
    // exercise the exact native signatures without touching a running client.
    image_base=0x400000;BYTE object[0x118]{};
    auto object_reset=[&](bool ap) {
        memset(object,0,sizeof(object));set_field(object,0,0x1146f5c);set_field(object,0x14,220);
        set_field(object,0x18,ap?149:134);set_field(object,0xf8,ap?28:13);
        set_field(object,0xb4,ap?71:55);set_field(object,0xb8,ap?149:134);
        set_field(object,0x104,55);set_field(object,0x110,71);set_field(object,0x114,149);set_field(object,0x108,134);
    };
    original_resize=fixture_resize;original_draw=fixture_draw;original_ctor=fixture_ctor;
    object_reset(true);InterlockedExchange(&layout_installed,0);hooked_ctor(object,nullptr);hooked_resize(object,nullptr,149);hooked_draw(object,nullptr);assert(field(object,0xb8)==149&&field(object,0x18)==149);draw_count=0;InterlockedExchange(&layout_installed,1);
    object_reset(true);assert(hooked_ctor(object,nullptr)==object&&field(object,0xb8)==164&&field(object,0x114)==164&&field(object,0x108)==134);
    hooked_resize(object,nullptr,149);assert(last_resize==164&&field(object,0x18)==164);
    hooked_resize(object,nullptr,71);assert(last_resize==71&&field(object,0x18)==71);
    hooked_resize(object,nullptr,164);assert(last_resize==164&&field(object,0x18)==164);
    object_reset(true);resize_success=false;hooked_resize(object,nullptr,149);assert(field(object,0x18)==149&&field(object,0xb8)==149&&field(object,0x114)==149);
    object_reset(true);hooked_resize(object,nullptr,200);assert(field(object,0xb8)==149&&field(object,0x114)==149&&last_resize==200);resize_success=true;
    object_reset(true);hooked_draw(object,nullptr);assert(field(object,0x18)==164&&draw_count==1);
    resize_success=false;object_reset(true);hooked_draw(object,nullptr);assert(field(object,0x18)==149&&field(object,0xb8)==149&&field(object,0x114)==149);resize_success=true;
    object_reset(false);hooked_ctor(object,nullptr);assert(field(object,0xb8)==149&&field(object,0x108)==149&&field(object,0x114)==149);
    hooked_resize(object,nullptr,134);assert(last_resize==149&&field(object,0x18)==149);
    hooked_resize(object,nullptr,55);assert(last_resize==55&&field(object,0x18)==55);
    hooked_resize(object,nullptr,149);assert(last_resize==149&&field(object,0x18)==149);
    object_reset(false);hooked_draw(object,nullptr);assert(field(object,0x18)==149);
    object_reset(false);resize_success=false;hooked_resize(object,nullptr,134);assert(field(object,0x18)==134&&field(object,0xb8)==134&&field(object,0x108)==134);resize_success=true;
    // The same149px surface is legacy AP or expanded normal: require the
    // exact native owner and caller, never infer the row from bitmap size.
    int edge=100,row_y=131;object_reset(true);assert(route_row(object,0x597a02,edge,row_y)==0&&row_y==131);
    object_reset(false);expand_object(object);set_field(object,0x18,149);row_y=116;
    assert(route_row(object,0x599231,edge,row_y)==149&&row_y==131);
    row_y=116;assert(route_row(object,0x597a02,edge,row_y)==0&&row_y==116);
    edge=100;assert(route_row(object,0x5992c6,edge,row_y)==0&&edge==215);
    object_reset(true);expand_object(object);set_field(object,0x18,164);row_y=131;
    assert(route_row(object,0x597a02,edge,row_y)==164&&row_y==146);
    // RED reproduction: live post-detour unwind retains the common native
    // text prefix but skips597a02 and proceeds directly to598875.
    static constexpr uintptr_t observed_draw[]={0x15074d,0x1501c0,0x15035d,0x6864e6,0x598875,0x67b748};
    static constexpr uintptr_t observed_measure[]={0x150613,0x14fd31,0x6864c7,0x598875,0x67b748};
    const auto routed=row_context(object,0x597a02,164," | Zeny: 2,147,483,647",0);
    assert(row_context_valid(routed));active_row=routed;
    assert(!wallet_stack(observed_draw,std::size(observed_draw),false)); // old logic reproduces failure
    assert(attested_wallet_stack(observed_draw,std::size(observed_draw),false,active_row));
    assert(attested_wallet_stack(observed_measure,std::size(observed_measure),true,active_row));
    static constexpr uintptr_t secondary_measure[]={0x150613,0x14fd31,0x6805ff,0x597a1f,0x598875};
    assert(!attested_wallet_stack(secondary_measure,std::size(secondary_measure),true,active_row));
    assert(!attested_wallet_stack(secondary_measure,std::size(secondary_measure),true,RowContext{}));
    assert(row_source_matches(capped,count)&&!row_source_matches(L" | Zeny: 1",10));
    RowContext rejected_row=routed;rejected_row.caller=0x597b4f;assert(!attested_wallet_stack(observed_draw,std::size(observed_draw),false,rejected_row));
    rejected_row=routed;rejected_row.height=149;assert(!attested_wallet_stack(observed_draw,std::size(observed_draw),false,rejected_row));
    rejected_row=routed;rejected_row.owner=nullptr;assert(!attested_wallet_stack(observed_draw,std::size(observed_draw),false,rejected_row));
    auto wrong_prefix=std::vector<uintptr_t>(std::begin(observed_draw),std::end(observed_draw));wrong_prefix[3]=0x6843df;
    assert(!attested_wallet_stack(wrong_prefix.data(),wrong_prefix.size(),false,routed));
    assert(!attested_wallet_stack(observed_draw,std::size(observed_draw),false,RowContext{}));
    original_right_text=fixture_right_text;hooked_right_text(object,nullptr,215,131," | Zeny: 1",0,1,14,0,0);
    assert(nested_cleared&&active_row.caller==routed.caller&&active_row.height==164&&row_source_matches(capped,count));
    object_reset(false);expand_object(object);set_field(object,0x18,149);
    active_row=row_context(object,0x599231,149,"Zeny : 1",0);assert(row_context_valid(active_row));
    assert(attested_wallet_stack(observed_draw,std::size(observed_draw),false,active_row));
    set_field(object,0x18,134);assert(!attested_wallet_stack(observed_draw,std::size(observed_draw),false,active_row));
    active_row={};

    object_reset(true);set_field(object,0,0);assert(!expand_object(object));
    // All known instruction guards must match the actual captured decoded
    // binary, and changing any guarded byte must disable native mutation.
    if(argc==2) {
        auto memory=static_cast<BYTE*>(VirtualAlloc(nullptr,0x700000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));assert(memory);image_base=reinterpret_cast<uintptr_t>(memory);
        for(const auto rva:{0x597000,0x686000}) {
            char name[64];snprintf(name,sizeof(name),"/PNNativeLayout-%08x.bin",rva);
            std::ifstream source(std::string(argv[1])+name,std::ios::binary);assert(source);source.read(reinterpret_cast<char*>(memory+rva),0x2000);assert(source.gcount()==0x2000);
        }
        {std::ifstream source(std::string(argv[1])+"/PNNativeLayout-00598000.bin",std::ios::binary);assert(source);source.seekg(0x1000);source.read(reinterpret_cast<char*>(memory+0x599000),0x1000);assert(source.gcount()==0x1000);}
        assert(layout_code_verified());
        for(const auto& guard:layout_guards()){memory[guard.rva]^=1;assert(!layout_code_verified());memory[guard.rva]^=1;}
        assert(layout_code_verified());VirtualFree(memory,0,MEM_RELEASE);image_base=0;
    }
    // Normal/AP rows preserve native fonts and right-align every amount to
    // x215. Verify the entire raster against independent GDI output.
    for(const int surface_height:{149,164}) {
        dc=CreateCompatibleDC(nullptr);info.bmiHeader.biHeight=-surface_height;
        bitmap=CreateDIBSection(dc,&info,DIB_RGB_COLORS,reinterpret_cast<void**>(&pixels),nullptr,0);old_bitmap=SelectObject(dc,bitmap);
        font=CreateFontW(-11,0,0,0,400,0,0,0,ANSI_CHARSET,0,0,NONANTIALIASED_QUALITY,0,L"Arial");old_font=SelectObject(dc,font);
        SetTextColor(dc,RGB(0,0,0));SetBkColor(dc,RGB(255,255,255));SetBkMode(dc,OPAQUE);
        const int row_bytes=220*surface_height*4,zeny_y=surface_height-18,old_height=surface_height-15;
        for(const auto wallet:{0LL,1LL,2147483647LL,16709925001LL,INT64_MAX,-1LL}) {
            GdiFlush();memset(pixels,255,row_bytes);
            RECT bottom{0,old_height-1,220,old_height};FillRect(dc,&bottom,static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
            const wchar_t* weight=L"Weight: 134 / 3800";SIZE weight_size{};assert(GetTextExtentPoint32W(dc,weight,int(wcslen(weight)),&weight_size));
            assert(TextOutW(dc,215-weight_size.cx,zeny_y-15,weight,int(wcslen(weight))));GdiFlush();
            const std::vector<BYTE> before(pixels,pixels+220*zeny_y*4);
            changed=false;assert(draw_separate_row(dc,zeny_y,wallet,changed)&&changed);GdiFlush();
            assert(!memcmp(before.data(),pixels,220*zeny_y*4));
            for(int x=0;x<220;++x)for(int channel=0;channel<3;++channel)assert(pixels[((surface_height-1)*220+x)*4+channel]==0);
            const auto full_text=full_row_text(wallet);SIZE natural{};assert(GetTextExtentPoint32W(dc,full_text.data(),int(full_text.size()),&natural)&&natural.cx<=210);
            const int zeny_x=215-natural.cx;assert(zeny_x>=5);
            HDC reference=CreateCompatibleDC(dc);auto ref_bitmap=CreateCompatibleBitmap(dc,220,surface_height);auto ref_old=SelectObject(reference,ref_bitmap);auto ref_font=SelectObject(reference,font);
            RECT all{0,0,220,surface_height};FillRect(reference,&all,static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH)));SetTextColor(reference,RGB(0,0,0));SetBkColor(reference,RGB(255,255,255));SetBkMode(reference,OPAQUE);
            assert(TextOutW(reference,zeny_x,zeny_y,full_text.data(),int(full_text.size())));
            for(int y=zeny_y;y<zeny_y+natural.cy;++y)for(int x=0;x<220;++x)assert(GetPixel(dc,x,y)==GetPixel(reference,x,y));
            SelectObject(reference,ref_font);SelectObject(reference,ref_old);DeleteObject(ref_bitmap);DeleteDC(reference);
            if(wallet==INT64_MAX||(surface_height==149&&wallet==1)) {BITMAPFILEHEADER h{};h.bfType=0x4d42;h.bfOffBits=sizeof(h)+sizeof(info.bmiHeader);h.bfSize=h.bfOffBits+row_bytes;const char* filename=wallet==1?"native-zeny-normal-small.bmp":surface_height==149?"native-zeny-normal-max.bmp":"native-zeny-two-rows-max.bmp";std::ofstream out(filename,std::ios::binary);out.write(reinterpret_cast<const char*>(&h),sizeof(h));out.write(reinterpret_cast<const char*>(&info.bmiHeader),sizeof(info.bmiHeader));out.write(reinterpret_cast<const char*>(pixels),row_bytes);}
        }
        // Reproduce the actual skin on an expanded magenta/key-color canvas.
        // The missing renderer formerly exposed the town through this region.
        if(argc==2&&surface_height==164) {
            std::ifstream skin(std::string(argv[1])+"/../before-two-rows/PNNativeLayout-basic-info.bmp",std::ios::binary);assert(skin);
            BITMAPFILEHEADER header{};BITMAPINFOHEADER bitmap_header{};skin.read(reinterpret_cast<char*>(&header),sizeof(header));skin.read(reinterpret_cast<char*>(&bitmap_header),sizeof(bitmap_header));
            assert(header.bfType==0x4d42&&bitmap_header.biWidth==220&&abs(bitmap_header.biHeight)==149&&bitmap_header.biBitCount==32);
            std::vector<BYTE> skin_pixels(220*149*4);skin.seekg(header.bfOffBits);skin.read(reinterpret_cast<char*>(skin_pixels.data()),skin_pixels.size());assert(skin.gcount()==std::streamsize(skin_pixels.size()));
            GdiFlush();for(int i=0;i<220*164;++i)reinterpret_cast<DWORD*>(pixels)[i]=0x00ff00ff;
            for(int y=0;y<149;++y)memcpy(pixels+y*220*4,skin_pixels.data()+(bitmap_header.biHeight<0?y:148-y)*220*4,220*4);
            changed=false;assert(draw_separate_row(dc,146,INT64_MAX,changed)&&changed);GdiFlush();
            for(int y=149;y<163;++y)for(int x=1;x<219;++x)assert(GetPixel(dc,x,y)!=RGB(255,0,255));
            assert(GetPixel(dc,0,163)==RGB(255,0,255)&&GetPixel(dc,219,163)==RGB(255,0,255));
            BITMAPFILEHEADER h{};h.bfType=0x4d42;h.bfOffBits=sizeof(h)+sizeof(info.bmiHeader);h.bfSize=h.bfOffBits+row_bytes;
            std::ofstream artifact("native-zeny-captured-skin-max.bmp",std::ios::binary);artifact.write(reinterpret_cast<const char*>(&h),sizeof(h));artifact.write(reinterpret_cast<const char*>(&info.bmiHeader),sizeof(info.bmiHeader));artifact.write(reinterpret_cast<const char*>(pixels),row_bytes);
        }
        // GDI success can hide clipping: reject a short clip and a region with
        // a hole before changing any pixel. Retain the caller's exact clip.
        for(int shape=0;shape<2;++shape) {
            HRGN clip=CreateRectRgn(0,0,220,shape==0?old_height:surface_height);
            if(shape==1){HRGN hole=CreateRectRgn(50,zeny_y+2,60,zeny_y+4);CombineRgn(clip,clip,hole,RGN_DIFF);DeleteObject(hole);}
            SelectClipRgn(dc,clip);GdiFlush();std::vector<BYTE> before(pixels,pixels+row_bytes);
            changed=false;assert(!draw_separate_row(dc,zeny_y,INT64_MAX,changed)&&!changed);GdiFlush();assert(!memcmp(before.data(),pixels,row_bytes));
            HRGN after=CreateRectRgn(0,0,0,0);assert(GetClipRgn(dc,after)==1&&EqualRgn(clip,after));DeleteObject(after);DeleteObject(clip);SelectClipRgn(dc,nullptr);
        }
        changed=false;assert(!draw_separate_row(dc,zeny_y-15,INT64_MAX,changed)&&!changed);
        SelectObject(dc,old_font);DeleteObject(font);SelectObject(dc,old_bitmap);DeleteObject(bitmap);DeleteDC(dc);
    }
    assert(numeric_wallet_suffix(L" | Zeny: 0",10));assert(numeric_wallet_suffix(L"Zeny : 1,234",12));assert(!numeric_wallet_suffix(L" | Zeny: password",17));
    std::cout<<"PASS: legacy fallback and natural full-width two-row raster; maximum/all wallet sizes; Weight/border preservation; normal/AP saved-height and migration rollback; rightalignment and clipping; exact postdetour TLS attestation; captured skin transparency; captured instruction guards\n";
}
