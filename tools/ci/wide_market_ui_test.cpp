#include "../../client-patch/wide_market/market_ui.cpp"
#include <cassert>
#include <vector>
#include <iostream>
namespace fixture {std::vector<pn_market::Request> requests;}
bool bank_session_snapshot(BankSession& session){session.account=100;session.generation=7;return true;}
bool bank_current_generation(LONG generation,bool){return generation==7;}
void market_forget_session(){}
bool market_submit(HWND,pn_market::Request request){fixture::requests.push_back(request);return true;}
void receive(pn_market::Reply reply) {
    assert(pn_market::valid_reply(reply));
    SendMessage(market_ui::window,MARKET_RESULT,0,reinterpret_cast<LPARAM>(new MarketResult{reply,7,true}));
}
void choose(){ListView_SetItemState(market_ui::listing,0,LVIS_SELECTED,LVIS_SELECTED);}
void save_preview(HWND window) {
    RECT size{};GetClientRect(window,&size);HDC screen=GetDC(window),memory=CreateCompatibleDC(screen);
    BITMAPINFO info{};info.bmiHeader.biSize=sizeof(info.bmiHeader);info.bmiHeader.biWidth=size.right;info.bmiHeader.biHeight=-size.bottom;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;
    void* pixels=nullptr;HBITMAP bitmap=CreateDIBSection(screen,&info,DIB_RGB_COLORS,&pixels,nullptr,0);auto old=SelectObject(memory,bitmap);
    PrintWindow(window,memory,PW_CLIENTONLY);
    BITMAPFILEHEADER header{};header.bfType=0x4d42;header.bfOffBits=sizeof(header)+sizeof(info.bmiHeader);header.bfSize=header.bfOffBits+size.right*size.bottom*4;
    FILE* output=fopen("wide-market-preview.bmp","wb");assert(output);fwrite(&header,sizeof(header),1,output);fwrite(&info.bmiHeader,sizeof(info.bmiHeader),1,output);fwrite(pixels,size.right*size.bottom*4,1,output);fclose(output);
    SelectObject(memory,old);DeleteObject(bitmap);DeleteDC(memory);ReleaseDC(window,screen);
}
int main() {
    using namespace market_ui;
    pn_bank::Reply bank;bank.nonce_hi=11;bank.nonce_lo=22;market_open(nullptr,bank);
    assert(fixture::requests.size()==1&&fixture::requests.back().action==pn_market::InspectOwn);
    pn_market::Reply reply;reply.nonce_hi=11;reply.nonce_lo=22;reply.result=pn_market::Ok;reply.flags=pn_market::Published;
    reply.owner_account=200;reply.owner_char=300;reply.shop_id=91;reply.revision=17;reply.wallet=INT64_MAX;
    std::strcpy(reply.owner_name,"Test Merchant");std::strcpy(reply.title,"Exact Zeny Shop");reply.count=1;
    auto& row=reply.entries[0];row.index=5;row.item_id=501;row.quantity=3;row.price=9007199254740993LL;
    std::strcpy(row.name,"Red Potion");receive(reply);choose();
    assert(IsWindowEnabled(GetDlgItem(window,purchase_id))&&!IsWindowEnabled(GetDlgItem(window,set_price_id)));
    SetWindowTextW(quantity,L"3");
    confirm=[](HWND,LPCWSTR text,LPCWSTR,UINT flags)->int{
        assert(flags&MB_DEFBUTTON2);assert(std::wcsstr(text,L"27,021,597,764,222,979"));return IDYES;
    };
    SendMessage(window,WM_COMMAND,purchase_id,0);
    assert(fixture::requests.size()==2&&fixture::requests.back().expected_price==9007199254740993LL&&fixture::requests.back().quantity==3);
    assert(fixture::requests.back().revision==17&&fixture::requests.back().shop_id==91);
    SendMessage(window,WM_COMMAND,purchase_id,0);assert(fixture::requests.size()==2);
    receive(reply);choose();
    confirm=[](HWND,LPCWSTR,LPCWSTR,UINT)->int{auto changed=state;++changed.revision;receive(changed);choose();return IDYES;};
    SendMessage(window,WM_COMMAND,purchase_id,0);assert(fixture::requests.size()==2);
    assert(value(status_label).find(L"listing changed")!=std::wstring::npos);
    reply=state;reply.entries[0].price=INT64_MAX;receive(reply);choose();SetWindowTextW(quantity,L"2");
    SendMessage(window,WM_COMMAND,purchase_id,0);assert(fixture::requests.size()==2);
    reply.owner_account=100;reply.flags=pn_market::Draft;receive(reply);choose();
    assert(IsWindowEnabled(GetDlgItem(window,publish_id))&&!IsWindowEnabled(GetDlgItem(window,purchase_id)));
    SetWindowTextW(price,L"9,223,372,036,854,775,807");confirm=[](HWND,LPCWSTR,LPCWSTR,UINT)->int{return IDYES;};
    SendMessage(window,WM_COMMAND,set_price_id,0);assert(fixture::requests.back().budget==INT64_MAX&&fixture::requests.back().action==pn_market::SetPrice);
    receive(reply);choose();SendMessage(window,WM_COMMAND,publish_id,0);assert(fixture::requests.back().action==pn_market::Publish);
    receive(reply);SetWindowTextW(min_price,L"9,007,199,254,740,993");SetWindowTextW(max_price,L"9,223,372,036,854,775,807");
    SendMessage(window,WM_COMMAND,search_id,0);assert(fixture::requests.back().min_price==9007199254740993LL&&fixture::requests.back().max_price==INT64_MAX);
    receive(reply);RECT bounds{};GetClientRect(window,&bounds);
    for(HWND child=GetWindow(window,GW_CHILD);child;child=GetWindow(child,GW_HWNDNEXT)) {
        RECT box{};GetWindowRect(child,&box);MapWindowPoints(nullptr,window,reinterpret_cast<POINT*>(&box),2);
        assert(box.left>=0&&box.top>=0&&box.right<=bounds.right&&box.bottom<=bounds.bottom);
    }
    // A durable save response blocks further purchases and polls read-only
    // status until completion; it must never replay the mutation itself.
    reply.owner_account=200;reply.flags=pn_market::Published;reply.entries[0].price=9007199254740993LL;
    receive(reply);choose();SetWindowTextW(quantity,L"1");
    SendMessage(window,WM_COMMAND,purchase_id,0);assert(fixture::requests.back().action==pn_market::Purchase);
    reply.result=pn_market::Saving;receive(reply);choose();
    assert(awaiting_save&&!IsWindowEnabled(GetDlgItem(window,purchase_id)));
    assert(value(status_label).find(L"Saving transaction")!=std::wstring::npos);
    const auto sent=fixture::requests.size();SendMessage(window,WM_TIMER,1,0);
    assert(fixture::requests.size()==sent+1&&fixture::requests.back().action==pn_market::ListShop&&fixture::requests.back().target_account==200);
    SendMessage(window,WM_TIMER,1,0);assert(fixture::requests.size()==sent+1);
    receive(reply);SendMessage(window,WM_TIMER,1,0);assert(fixture::requests.size()==sent+2&&fixture::requests.back().action==pn_market::ListShop);
    reply.result=pn_market::Ok;receive(reply);choose();assert(!awaiting_save&&IsWindowEnabled(GetDlgItem(window,purchase_id)));
    SendMessage(window,WM_TIMER,1,0);assert(fixture::requests.size()==sent+2);
    save_preview(window);DestroyWindow(window);
    std::cout<<"PASS: real market controls, exact full-width quote/total, explicit draft publication, changed-quote rejection, duplicate suppression and search filters\n";
}
