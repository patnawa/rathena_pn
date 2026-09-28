// Exact-price market UI. Native windows retain item selection and shop creation.
#include "market_client.hpp"
#include "numbers.hpp"
#include <commctrl.h>
#include <algorithm>
#include <cstring>
#include <string>

namespace market_ui {
HWND window=nullptr,listing=nullptr,kind=nullptr,item_filter=nullptr,min_price=nullptr,max_price=nullptr;
HWND quantity=nullptr,price=nullptr,budget=nullptr,status_label=nullptr,shop_label=nullptr,details=nullptr;
HFONT font=nullptr;
pn_market::Reply state;
pn_market::Request last_query;
pn_market::Request active_request;
bool busy=false,verified=false,awaiting_save=false;
LONG generation=0;
uint64_t sequence=0;
constexpr int my_id=101,selected_id=102,search_id=103,next_id=104,refresh_id=105;
constexpr int set_price_id=201,set_budget_id=202,publish_id=203,close_id=204,purchase_id=205,sell_id=206;
constexpr int list_id=300,kind_id=301;
using Confirmation=int(*)(HWND,LPCWSTR,LPCWSTR,UINT);
Confirmation confirm=[](HWND owner,LPCWSTR message,LPCWSTR title,UINT flags){return MessageBoxW(owner,message,title,flags);};
std::wstring wide(const char* bytes,size_t capacity) {
    auto end=static_cast<const char*>(std::memchr(bytes,0,capacity));int length=static_cast<int>(end?end-bytes:capacity);
    wchar_t output[256]{};int count=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,bytes,length,output,256);
    if(!count)count=MultiByteToWideChar(CP_ACP,0,bytes,length,output,256);
    return std::wstring(output,count);
}
std::wstring value(HWND control) {wchar_t text[128]{};GetWindowTextW(control,text,128);return text;}
bool read(HWND control,int64_t& number){return pn_market_ui::parse_amount(value(control),number);}
HWND control(LPCWSTR type,LPCWSTR text,DWORD style,int id,int x,int y,int width,int height) {
    HWND child=CreateWindowExW(type==std::wstring(L"EDIT")?WS_EX_CLIENTEDGE:0,type,text,WS_CHILD|WS_VISIBLE|style,
        x,y,width,height,window,reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),GetModuleHandle(nullptr),nullptr);
    SendMessage(child,WM_SETFONT,reinterpret_cast<WPARAM>(font),TRUE);return child;
}
void label(LPCWSTR text,int x,int y,int width){control(L"STATIC",text,SS_LEFT,0,x,y,width,21);}
void button(int id,LPCWSTR text,int x,int y,int width){control(L"BUTTON",text,WS_TABSTOP|BS_PUSHBUTTON,id,x,y,width,28);}
int selection(){return ListView_GetNextItem(listing,-1,LVNI_SELECTED);}
const pn_market::Entry* selected(){int index=selection();return index>=0&&static_cast<uint32_t>(index)<state.count?&state.entries[index]:nullptr;}
bool own(){BankSession session;return bank_session_snapshot(session)&&state.owner_account==session.account;}
std::wstring describe(const pn_market::Entry& row) {
    std::wstring text=wide(row.name,sizeof(row.name))+L" (item "+std::to_wstring(row.item_id)+L")";
    if(row.refine)text+=L"  Refine +"+std::to_wstring(row.refine);
    if(row.grade)text+=L"  Grade "+std::to_wstring(row.grade);
    for(auto card:row.cards)if(card)text+=L"  Card "+std::to_wstring(card);
    for(auto option:row.options)if(option.id)text+=L"  Option "+std::to_wstring(option.id)+L":"+std::to_wstring(option.value)+L"/"+std::to_wstring(option.param);
    return text;
}
void update() {
    const bool ready=verified&&!busy&&state.result==pn_market::Ok;
    const auto row=selected();const bool owner=ready&&own();
    for(int id:{my_id,selected_id,search_id,refresh_id})EnableWindow(GetDlgItem(window,id),!busy);
    EnableWindow(GetDlgItem(window,next_id),ready&&state.has_more);
    EnableWindow(GetDlgItem(window,set_price_id),owner&&row&&state.flags==pn_market::Draft);
    EnableWindow(GetDlgItem(window,set_budget_id),owner&&state.kind==pn_market::Buying&&state.flags==pn_market::Draft);
    EnableWindow(GetDlgItem(window,publish_id),owner&&state.flags==pn_market::Draft&&state.count);
    EnableWindow(GetDlgItem(window,close_id),owner&&state.shop_id);
    EnableWindow(GetDlgItem(window,purchase_id),ready&&!owner&&row&&state.kind==pn_market::Vending&&state.flags==pn_market::Published);
    EnableWindow(GetDlgItem(window,sell_id),ready&&!owner&&row&&state.kind==pn_market::Buying&&state.flags==pn_market::Published);
    SetWindowTextW(details,row?describe(*row).c_str():L"Select a listing to review its item details.");
}
void render() {
    ListView_DeleteAllItems(listing);
    for(uint32_t i=0;i<state.count;++i) {
        const auto& row=state.entries[i];auto name=wide(row.name,sizeof(row.name));
        LVITEMW item{};item.mask=LVIF_TEXT;item.iItem=i;item.pszText=const_cast<LPWSTR>(name.c_str());
        SendMessageW(listing,LVM_INSERTITEMW,0,reinterpret_cast<LPARAM>(&item));
        std::wstring texts[]={std::to_wstring(row.quantity),pn_market_ui::format_amount(row.price),std::to_wstring(row.refine),std::to_wstring(row.grade)};
        for(int column=1;column<5;++column){item.iSubItem=column;item.pszText=const_cast<LPWSTR>(texts[column-1].c_str());SendMessageW(listing,LVM_SETITEMTEXTW,i,reinterpret_cast<LPARAM>(&item));}
    }
    auto title=wide(state.title,sizeof(state.title));auto owner=wide(state.owner_name,sizeof(state.owner_name));
    auto summary=owner.empty()?L"No shop selected":owner+L" — "+title+(state.flags==pn_market::Draft?L" (draft — not for sale)":L"");
    summary+=L"    Wallet: "+pn_market_ui::format_amount(state.wallet)+L"z";
    SetWindowTextW(shop_label,summary.c_str());
    SetWindowTextW(budget,pn_market_ui::format_amount(state.budget).c_str());update();
}
void send(pn_market::Request request,bool query) {
    if(busy)return;
    request.nonce_hi=state.nonce_hi;request.nonce_lo=state.nonce_lo;request.request_id=++sequence;
    if(query)last_query=request;
    active_request=request;
    busy=market_submit(window,request);
    if(!busy){verified=false;SetWindowTextW(status_label,L"Log in to a character before using the market.");}
    else SetWindowTextW(status_label,L"Waiting for server confirmation...");
    update();
}
void query(uint32_t action,bool next=false) {
    pn_market::Request request;request.action=action;
    request.kind=static_cast<uint32_t>(SendMessage(kind,CB_GETCURSEL,0,0));
    if(next)request.cursor=state.next_cursor;
    if(action==pn_market::Search) {
        int64_t item=0;
        if(!read(item_filter,item)||item>UINT32_MAX||!read(min_price,request.min_price)||!read(max_price,request.max_price)||
            (request.max_price && request.min_price>request.max_price)) {
            SetWindowTextW(status_label,L"Use a whole item ID and valid minimum/maximum prices. Zero maximum means no upper filter.");return;
        }
        request.item_id=static_cast<uint32_t>(item);
    }
    if(next && action==pn_market::ListShop){request.target_account=state.owner_account;request.shop_id=state.shop_id;}
    send(request,true);
}
void mutate(uint32_t action) {
    if(!verified||busy||state.result!=pn_market::Ok)return;
    auto row=selected();pn_market::Request request;request.action=action;request.kind=state.kind;
    request.target_account=state.owner_account;request.shop_id=state.shop_id;request.revision=state.revision;
    auto review=L"Shop: "+wide(state.owner_name,sizeof(state.owner_name))+L"\n";
    pn_market::Entry quoted{};
    const bool requires_row=action==pn_market::SetPrice||action==pn_market::Purchase||action==pn_market::Sell;
    if(requires_row) {
        if(!row)return;
        quoted=*row;request.index=row->index;request.item_id=row->item_id;request.item_unique_id=row->unique_id;request.expected_price=row->price;
        review+=describe(*row)+L"\n";
        if(action==pn_market::SetPrice) {
            if(!read(price,request.budget)){SetWindowTextW(status_label,L"Enter a whole price within the Zeny limit.");return;}
            review+=L"Set unit price: "+pn_market_ui::format_amount(request.budget)+L"z";
        }else {
            int64_t count=0,total=0;
            if(!read(quantity,count)||count<=0||count>row->quantity||count>UINT32_MAX||!pn_market_ui::total(row->price,static_cast<uint32_t>(count),total)) {
                SetWindowTextW(status_label,L"Choose an available quantity whose total fits the Zeny limit.");return;
            }
            request.quantity=static_cast<uint32_t>(count);
            review+=L"Quantity: "+std::to_wstring(count)+L"\nUnit price: "+pn_market_ui::format_amount(row->price)+L"z\nTotal: "+pn_market_ui::format_amount(total)+L"z";
            if(action==pn_market::Sell)review+=L"\nOnly plain, eligible items from one inventory stack will be sold.";
        }
    }else if(action==pn_market::SetBudget) {
        if(!read(budget,request.budget)){SetWindowTextW(status_label,L"Enter a whole buying budget within the Zeny limit.");return;}
        review+=L"New buying budget: "+pn_market_ui::format_amount(request.budget)+L"z";
    }else if(action==pn_market::Publish) {
        review+=L"Publish these prices?\n";
        for(uint32_t i=0;i<state.count;++i)review+=wide(state.entries[i].name,sizeof(state.entries[i].name))+L" × "+std::to_wstring(state.entries[i].quantity)+L" @ "+pn_market_ui::format_amount(state.entries[i].price)+L"z\n";
        if(state.kind==pn_market::Buying)review+=L"Budget: "+pn_market_ui::format_amount(state.budget)+L"z";
    }else if(action==pn_market::Close)review+=L"Close this shop?";
    else return;
    if(confirm(window,review.c_str(),L"Confirm market action",MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2)!=IDYES)return;
    const auto current=selected();
    if(request.shop_id!=state.shop_id||request.revision!=state.revision||request.target_account!=state.owner_account||
        (requires_row && (!current||std::memcmp(&quoted,current,sizeof(quoted))))) {
        SetWindowTextW(status_label,L"The listing changed. Refresh and review the exact price again.");return;
    }
    send(request,false);
}
const wchar_t* result_text(uint32_t result) {
    const wchar_t* text[]={L"Server confirmed. Review the current listing before the next action.",L"Log in to a character.",L"The request is invalid.",L"Finish your current action and try again.",L"Shop not found. Select a shop in the game or create your own.",L"The quote changed. Refresh and review it again.",L"Not enough Zeny.",L"Inventory, stock or budget capacity is insufficient.",L"The market is unavailable here.",L"Saving transaction. Checking its status; please wait..."};
    return result<=pn_market::Saving?text[result]:L"Unexpected server response.";
}
LRESULT CALLBACK proc(HWND hwnd,UINT message,WPARAM w,LPARAM l) {
    switch(message) {
    case WM_CREATE: {
        window=hwnd;font=CreateFontW(-14,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,0,0,L"Segoe UI");
        kind=control(L"COMBOBOX",L"",WS_TABSTOP|CBS_DROPDOWNLIST,kind_id,14,12,135,150);
        SendMessageW(kind,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(L"Vending"));SendMessageW(kind,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(L"Buying stores"));SendMessage(kind,CB_SETCURSEL,0,0);
        button(my_id,L"My shop",160,12,108);button(selected_id,L"Selected shop",278,12,122);button(refresh_id,L"Refresh",410,12,95);button(next_id,L"Next",515,12,80);
        label(L"Item ID (0 = any)",14,53,115);item_filter=control(L"EDIT",L"0",WS_TABSTOP|ES_AUTOHSCROLL,0,132,50,92,25);
        label(L"Min price",234,53,68);min_price=control(L"EDIT",L"0",WS_TABSTOP|ES_AUTOHSCROLL,0,303,50,190,25);
        label(L"Max price",505,53,70);max_price=control(L"EDIT",L"0",WS_TABSTOP|ES_AUTOHSCROLL,0,575,50,190,25);button(search_id,L"Search",778,48,83);
        shop_label=control(L"STATIC",L"Select a shop in the game, then click Selected shop.",SS_LEFT,0,14,88,850,38);
        listing=control(WC_LISTVIEWW,L"",WS_TABSTOP|WS_BORDER|LVS_REPORT|LVS_SINGLESEL|LVS_SHOWSELALWAYS,list_id,14,130,848,250);
        ListView_SetExtendedListViewStyle(listing,LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES|LVS_EX_DOUBLEBUFFER);
        const wchar_t* names[]={L"Item",L"Available",L"Unit price (Zeny)",L"Refine",L"Grade"};int widths[]={390,90,245,58,58};
        for(int i=0;i<5;++i){LVCOLUMNW column{};column.mask=LVCF_TEXT|LVCF_WIDTH;column.cx=widths[i];column.pszText=const_cast<LPWSTR>(names[i]);SendMessageW(listing,LVM_INSERTCOLUMNW,i,reinterpret_cast<LPARAM>(&column));}
        details=control(L"STATIC",L"Select a listing to review its item details.",SS_LEFT,0,14,390,848,54);
        label(L"Quantity",14,459,60);quantity=control(L"EDIT",L"1",WS_TABSTOP|ES_AUTOHSCROLL,0,76,455,78,25);
        button(purchase_id,L"Buy selected",164,453,140);button(sell_id,L"Sell plain items",314,453,145);
        label(L"New price",14,501,65);price=control(L"EDIT",L"0",WS_TABSTOP|ES_AUTOHSCROLL,0,82,497,224,25);button(set_price_id,L"Set price",316,495,105);
        label(L"Buying budget",435,501,98);budget=control(L"EDIT",L"0",WS_TABSTOP|ES_AUTOHSCROLL,0,535,497,224,25);button(set_budget_id,L"Set budget",769,495,93);
        button(publish_id,L"Publish reviewed shop",14,539,205);button(close_id,L"Close shop",230,539,120);
        status_label=control(L"STATIC",L"",SS_LEFT,0,14,580,848,40);
        SetTimer(hwnd,1,500,nullptr);update();return 0;
    }
    case WM_NOTIFY:
        if(reinterpret_cast<NMHDR*>(l)->idFrom==list_id){update();return 0;}break;
    case WM_COMMAND: {
        int id=LOWORD(w);
        if(id==my_id)query(pn_market::InspectOwn);
        else if(id==selected_id)query(pn_market::ListShop);
        else if(id==search_id)query(pn_market::Search);
        else if(id==next_id)query(last_query.action,true);
        else if(id==refresh_id)send(last_query,true);
        else if(id==set_price_id)mutate(pn_market::SetPrice);
        else if(id==set_budget_id)mutate(pn_market::SetBudget);
        else if(id==publish_id)mutate(pn_market::Publish);
        else if(id==close_id)mutate(pn_market::Close);
        else if(id==purchase_id)mutate(pn_market::Purchase);
        else if(id==sell_id)mutate(pn_market::Sell);
        return 0;
    }
    case MARKET_RESULT: {
        auto result=reinterpret_cast<MarketResult*>(l);
        if(result->generation==generation&&bank_current_generation(generation)) {
            busy=false;verified=result->connected&&result->state.result!=pn_market::Unauthorized;
            awaiting_save=result->connected&&result->state.result==pn_market::Saving;
            if(result->connected){
                state=result->state;sequence=std::max(sequence,state.request_id);
                if(awaiting_save&&active_request.action>=pn_market::SetPrice) {
                    // Poll only a read-only query. Never resend the purchase.
                    last_query=pn_market::Request{};last_query.action=pn_market::ListShop;
                    last_query.kind=active_request.kind;last_query.target_account=active_request.target_account;
                    last_query.shop_id=active_request.shop_id;
                }
                render();SetWindowTextW(status_label,result_text(state.result));
            }
            else {SetWindowTextW(status_label,L"Connection lost. Refresh to check the outcome; no transaction is retried automatically.");update();}
        }
        delete result;return 0;
    }
    case WM_TIMER:
        if(generation&&!bank_current_generation(generation)){verified=busy=awaiting_save=false;generation=0;state=pn_market::Reply{};market_forget_session();ShowWindow(hwnd,SW_HIDE);render();}
        else if(awaiting_save&&!busy)send(last_query,true);
        return 0;
    case WM_CLOSE:ShowWindow(hwnd,SW_HIDE);return 0;
    case WM_DESTROY:KillTimer(hwnd,1);DeleteObject(font);font=nullptr;window=nullptr;return 0;
    }
    return DefWindowProcW(hwnd,message,w,l);
}
}
void market_open(HWND owner,const pn_bank::Reply& bank) {
    using namespace market_ui;
    BankSession session;if(!bank_session_snapshot(session))return;
    if(!window) {
        INITCOMMONCONTROLSEX controls{sizeof(controls),ICC_LISTVIEW_CLASSES};InitCommonControlsEx(&controls);
        WNDCLASSW type{};type.lpfnWndProc=proc;type.hInstance=GetModuleHandle(nullptr);type.hCursor=LoadCursor(nullptr,IDC_ARROW);
        type.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_BTNFACE+1);type.lpszClassName=L"PNWideMarket";RegisterClassW(&type);
        RECT area{0,0,878,632};AdjustWindowRectEx(&area,WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,FALSE,WS_EX_TOOLWINDOW);
        window=CreateWindowExW(WS_EX_TOOLWINDOW,type.lpszClassName,L"Zeny Market",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,
            CW_USEDEFAULT,CW_USEDEFAULT,area.right-area.left,area.bottom-area.top,owner,nullptr,type.hInstance,nullptr);
    }
    if(generation!=session.generation){state=pn_market::Reply{};sequence=0;verified=busy=awaiting_save=false;generation=session.generation;}
    state.nonce_hi=bank.nonce_hi;state.nonce_lo=bank.nonce_lo;
    ShowWindow(window,SW_SHOW);SetForegroundWindow(window);query(pn_market::InspectOwn);
}
