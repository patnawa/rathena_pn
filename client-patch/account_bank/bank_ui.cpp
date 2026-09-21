// Native owned Windows panel, styled after the owner's supplied reference.
// GPL-3.0-or-later. No embedded browser, account password, or client-side balance authority.
#include "bank_client.hpp"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <string>
#include <vector>

namespace {
HINSTANCE instance;
HWND panel=nullptr, game=nullptr;
WNDPROC previous_game_proc=nullptr;
WNDPROC previous_input_proc=nullptr;
HFONT font=nullptr, balance_font=nullptr;
HBRUSH white=nullptr;
bool preview=false, busy=false, refreshing=false, verified=false;
uint32_t queued_action=pn_bank::Refresh; // One explicit click may wait for a read-only refresh.
int64_t queued_amount=0;
std::wstring diagnostics_path;
bool last_reply_connected=false;
pn_bank::Reply state;
uint64_t sequence=0;
ULONGLONG last_refresh=0;
std::wstring status=L"Log in to a character to use the bank.";
std::array<HWND,3> inputs{};
std::array<HWND,6> actions{};
constexpr int panel_width=412, panel_height=482;
constexpr int field_y[3]={76,172,302};
constexpr int preset_y[3]={106,172,302};
constexpr int action_y[3]={76,200,330};
constexpr int edit_ids[3]={100,101,102};
constexpr int refresh_id=200, close_id=201, title_close=202, help_id=203;
int scale_percent=100;
int px(int value) { return MulDiv(value,scale_percent,100); }
struct PendingReceipt { uint32_t action=0; int64_t amount=0; uint64_t id=0, nonce_hi=0, nonce_lo=0; } pending_receipt;
std::wstring receipt;
bool normalizing_input=false;
int cursor_show_adjustments=0;
constexpr UINT_PTR cursor_timer=2;

void release_panel_cursor() {
    KillTimer(panel,cursor_timer);
    // Balance only our own increments; do not change the game's cursor policy.
    while(cursor_show_adjustments>0) { ShowCursor(FALSE); --cursor_show_adjustments; }
}
void set_panel_cursor(HWND target) {
    const bool edit=std::find(inputs.begin(),inputs.end(),target)!=inputs.end();
    SetCursor(LoadCursor(nullptr,edit?IDC_IBEAM:IDC_ARROW));
    if(!cursor_show_adjustments) {
        int count;
        do { count=ShowCursor(TRUE); ++cursor_show_adjustments; }
        while(count<0 && cursor_show_adjustments<64);
        SetTimer(panel,cursor_timer,50,nullptr);
    }
}

bool transaction_pending() { return (busy && !refreshing) || queued_action!=pn_bank::Refresh; }
bool actions_ready() {
    return verified && !transaction_pending() && state.result!=pn_bank::Saving && state.result!=pn_bank::Unavailable;
}

std::wstring wide(const char* value) { return std::wstring(value,value+strlen(value)); }
std::wstring commas(int64_t value,bool zeny=false) {
    std::wstring text=std::to_wstring(value);
    for(int pos=static_cast<int>(text.size())-3;pos>0;pos-=3) text.insert(pos,L",");
    return text+(zeny?L"z":L"");
}
int64_t amount(int row) {
    wchar_t text[64]{}; GetWindowTextW(inputs[row],text,64);
    if(!text[0]) return 0;
    int64_t value=0; int digits=0; bool grouped=false;
    for(auto p=text;*p;++p) {
        if(*p==L',') {
            if(!digits || (grouped?digits!=3:digits>3)) return -1;
            grouped=true; digits=0; continue;
        }
        if(*p<L'0' || *p>L'9' || value>(INT64_MAX-(*p-L'0'))/10) return -1;
        value=value*10+(*p-L'0'); ++digits;
    }
    if(grouped && digits!=3) return -1;
    return value;
}
void set_amount(int row,int64_t value) { auto text=GetFocus()==inputs[row]?std::to_wstring(value):commas(value); SetWindowTextW(inputs[row],text.c_str()); }
bool normalize_amount(int row,bool grouped) {
    wchar_t buffer[64]{};GetWindowTextW(inputs[row],buffer,64);
    const std::wstring old(buffer);const auto value=amount(row);
    if(old.empty() || value<0) return false;
    auto next=grouped?commas(value):old;
    if(!grouped) next.erase(std::remove(next.begin(),next.end(),L','),next.end());
    if(old==next) return false;
    DWORD first=0,last=0;SendMessage(inputs[row],EM_GETSEL,reinterpret_cast<WPARAM>(&first),reinterpret_cast<LPARAM>(&last));
    const auto caret=[&](DWORD position) {
        size_t digits=0;for(size_t i=0;i<std::min<size_t>(position,old.size());++i)if(old[i]!=L',')++digits;
        size_t i=0;while(i<next.size() && digits) { if(next[i]!=L',')--digits;++i; }return i;
    };
    normalizing_input=true;SetWindowTextW(inputs[row],next.c_str());normalizing_input=false;
    SendMessage(inputs[row],EM_SETSEL,caret(first),caret(last));return true;
}
void update(const char* event);
LRESULT CALLBACK input_proc(HWND window,UINT message,WPARAM w,LPARAM l) {
    if(message==WM_CHAR && w==1) { SendMessage(window,EM_SETSEL,0,-1);return 0; } // Ctrl+A
    if(!normalizing_input && ((message==WM_CHAR && w!=L',') || (message==WM_KEYDOWN && w==VK_DELETE))) {
        const auto row=std::find(inputs.begin(),inputs.end(),window)-inputs.begin();
        if(row<3) normalize_amount(static_cast<int>(row),false);
    }
    const auto result=CallWindowProcW(previous_input_proc,window,message,w,l);
    // Normalize after native replacement finishes, outside EN_CHANGE.
    if((message==WM_PASTE || message==WM_SETTEXT || message==EM_REPLACESEL) && !normalizing_input && GetFocus()==window) {
        const auto row=std::find(inputs.begin(),inputs.end(),window)-inputs.begin();
        if(row<3 && normalize_amount(static_cast<int>(row),false)) update("input_normalized");
    }
    return result;
}
std::wstring exchange_block_reason(int row,bool buy) {
    const std::wstring label=buy?L"Buy: ":L"Sell: ";
    if(!verified) return label+L"Log in, then Refresh to connect to the bank.";
    if(transaction_pending() || state.result==pn_bank::Saving) return label+L"Waiting for the bank. Please wait...";
    if(state.result==pn_bank::Unavailable) return label+L"Banking is unavailable here.";
    const uint32_t action=(row==1?pn_bank::BuyDiamond:pn_bank::BuyNote)+(buy?0:1);
    const auto count=amount(row);
    const auto result=pn_bank::plan(state,action,count).result;
    switch(result) {
    case pn_bank::Ok: return L"";
    case pn_bank::Funds:
        return label+L"Deposit "+commas(count*state.buy[row-1]-state.bank)+L" more Zeny.";
    case pn_bank::Items:
        return label+(state.counts[row-1]?L"Only "+commas(state.counts[row-1])+L" eligible items on hand.":
            L"No eligible items on hand.");
    case pn_bank::Capacity: return label+L"Free inventory space/weight, or buy fewer.";
    case pn_bank::Limit: return label+L"Bank or transaction limit reached.";
    default: return label+L"Enter a whole quantity of 1 or more.";
    }
}
void text(HDC dc,int x,int y,int width,const std::wstring& value,bool right=false) {
    RECT box{px(x),px(y),px(x+width),px(y+20)}; DrawTextW(dc,value.c_str(),-1,&box,DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX|(right?DT_RIGHT:DT_LEFT));
}
std::wstring exchange_label(int row,bool buy) {
    const auto count=amount(row);
    const auto price=buy?state.buy[row-1]:state.sell[row-1];
    const std::wstring verb=buy?L"Buy":L"Sell";
    if(count<=0 || count>INT32_MAX || !price || count>INT64_MAX/price)
        return verb+L"\nEnter quantity";
    return verb+L" "+commas(count)+L"\n"+(buy?L"-":L"+")+commas(count*price,true);
}
void caption(HWND control,const std::wstring& value) {
    if(!control) return;
    wchar_t current[128]{}; GetWindowTextW(control,current,128);
    if(value!=current) SetWindowTextW(control,value.c_str());
}
void line(HDC dc,int x,int y,int x2,int y2,COLORREF color) {
    const auto old=SelectObject(dc,GetStockObject(DC_PEN)); const auto prior=SetDCPenColor(dc,color);
    MoveToEx(dc,x,y,nullptr); LineTo(dc,x2,y2); SetDCPenColor(dc,prior); SelectObject(dc,old);
}
void gradient(HDC dc,RECT box,COLORREF top,COLORREF bottom) {
    int height=std::max<LONG>(1,box.bottom-box.top);
    for(int i=0;i<height;++i) {
        COLORREF color=RGB(GetRValue(top)+(GetRValue(bottom)-GetRValue(top))*i/height,
            GetGValue(top)+(GetGValue(bottom)-GetGValue(top))*i/height,
            GetBValue(top)+(GetBValue(bottom)-GetBValue(top))*i/height);
        line(dc,box.left,box.top+i,box.right,box.top+i,color);
    }
}
void icon(HDC dc,int row,int x,int y) {
    if(row==1) {
        POINT shape[]={{x+16,y},{x+29,y+10},{x+21,y+29},{x+8,y+29},{x,y+13}};
        auto brush=CreateSolidBrush(RGB(192,191,249)); auto old=SelectObject(dc,brush);
        Polygon(dc,shape,5); SelectObject(dc,old); DeleteObject(brush);
        line(dc,x+16,y,x+9,y+29,RGB(245,247,255)); line(dc,x,y+13,x+29,y+10,RGB(245,247,255));
        line(dc,x+16,y,x+21,y+29,RGB(133,147,220)); line(dc,x+1,y+13,x+21,y+29,RGB(238,238,255));
    } else if(row==2) {
        RECT paper{x+6,y+3,x+25,y+30}; FillRect(dc,&paper,white);
        auto pen=CreatePen(PS_SOLID,2,RGB(193,81,76)); auto old=SelectObject(dc,pen);
        Rectangle(dc,paper.left,paper.top,paper.right,paper.bottom);
        SelectObject(dc,old); DeleteObject(pen);
        line(dc,x+10,y+11,x+20,y+11,RGB(179,75,67));
        line(dc,x+20,y+11,x+10,y+24,RGB(179,75,67));
        line(dc,x+10,y+24,x+20,y+24,RGB(179,75,67));
        line(dc,x+9,y,x+14,y+5,RGB(140,110,91)); line(dc,x+19,y,x+14,y+5,RGB(140,110,91));
    } else {
        auto brush=CreateSolidBrush(RGB(94,88,145)); auto old=SelectObject(dc,brush);
        Rectangle(dc,x,y+1,x+28,y+22); SelectObject(dc,old); DeleteObject(brush);
        RECT note{x+4,y+5,x+24,y+18}; FillRect(dc,&note,white);
        Ellipse(dc,x+10,y+10,x+33,y+33); text(dc,x+16,y+14,12,L"z");
    }
}
void paint(HDC dc) {
    RECT all{0,0,px(panel_width),px(panel_height)}; FillRect(dc,&all,white); SelectObject(dc,font);
    SetBkMode(dc,TRANSPARENT); SetTextColor(dc,RGB(37,48,68));
    gradient(dc,RECT{0,0,px(panel_width),px(26)},RGB(194,205,249),RGB(234,239,255));
    text(dc,12,3,190,L"Account Bank");
    text(dc,panel_width-84,3,42,L"v2.4.1",true);
    text(dc,12,30,86,L"In bank"); text(dc,12,49,86,L"On hand");
    SelectObject(dc,balance_font);
    text(dc,100,30,panel_width-112,verified?commas(state.bank,true):L"--",true);
    text(dc,100,49,panel_width-112,verified?commas(state.wallet,true):L"--",true);
    SelectObject(dc,font);
    text(dc,12,field_y[0]+1,34,L"Zeny");
    line(dc,px(12),px(138),px(panel_width-12),px(138),RGB(218,223,233));
    for(int row=0;row<3;++row) {
        const int x=row==0?48:44, width=row==0?170:140;
        RECT backing{px(x),px(field_y[row]),px(x+width),px(field_y[row]+24)};
        FillRect(dc,&backing,reinterpret_cast<HBRUSH>(GetStockObject(WHITE_BRUSH)));
        FrameRect(dc,&backing,reinterpret_cast<HBRUSH>(GetStockObject(LTGRAY_BRUSH)));
        wchar_t value[64]{}; GetWindowTextW(inputs[row],value,64);
        text(dc,x+4,field_y[row]+2,width-8,value);
    }
    for(int row=1;row<3;++row) {
        const int y=row==1?145:275;
        const auto saved=SaveDC(dc);
        SetMapMode(dc,MM_ANISOTROPIC); SetWindowExtEx(dc,33,33,nullptr);
        SetViewportExtEx(dc,px(20),px(20),nullptr); SetViewportOrgEx(dc,px(12),px(y),nullptr);
        icon(dc,row,0,0); RestoreDC(dc,saved);
        text(dc,40,y,226,row==1?L"17Carat Diamond":L"1M Zeny Ticket");
        text(dc,267,y,133,L"On hand: "+commas(state.counts[row-1]),true);
        text(dc,12,field_y[row]+1,30,L"Qty");
        for(int action=0;action<2;++action) {
            const auto reason=exchange_block_reason(row,action==0);
            SetTextColor(dc,reason.empty()?RGB(93,102,119):RGB(153,53,39));
            const auto hint=reason.empty()?
                std::wstring(action==0?L"Buy up to ":L"Sell up to ")+commas(action==0?state.max_buy[row-1]:state.max_sell[row-1]):reason;
            text(dc,12,action_y[row]+38+action*16,panel_width-24,hint);
        }
        SetTextColor(dc,RGB(37,48,68));
        if(row==1) line(dc,px(12),px(272),px(panel_width-12),px(272),RGB(218,223,233));
    }
    auto guidance=status;
    if(!receipt.empty() && verified && !transaction_pending() && state.result==pn_bank::Ok && status==wide(pn_bank::message(pn_bank::Ok))) guidance=receipt;
    else if(actions_ready() && state.result==pn_bank::Ok && status==wide(pn_bank::message(pn_bank::Ok))) {
        guidance=L"Buy / Sell totals use your bank zeny.";
        for(int row=0;row<3;++row)
            if(GetFocus()==inputs[row] && amount(row)<=0)
                guidance=row==0?L"Enter the next zeny amount.":L"Enter an item quantity to buy or sell.";
    }
    SetTextColor(dc,RGB(63,73,91));
    RECT footer{px(12),px(443),px(panel_width-12),px(panel_height-5)};
    DrawTextW(dc,guidance.c_str(),-1,&footer,DT_WORDBREAK|DT_NOPREFIX);
}

HWND button(int id,const wchar_t* label,int x,int y,int width,int height=21) {
    HWND child=CreateWindowW(L"BUTTON",label,WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_OWNERDRAW,
        px(x),px(y),px(width),px(height),panel,reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),instance,nullptr);
    SendMessage(child,WM_SETFONT,reinterpret_cast<WPARAM>(font),TRUE); return child;
}
const char* result_name(uint32_t result) {
    const char* names[]={"Ok","Saving","Unauthorized","Invalid","Busy","Unavailable","Funds","Capacity","Limit","Items","Stale","SaveFailed"};
    return result<=pn_bank::SaveFailed?names[result]:"Unknown";
}
void write_diagnostics(const char* event) {
    if(diagnostics_path.empty()) return;
    FILE* file=_wfopen(diagnostics_path.c_str(),L"w");
    if(!file) return;
    // Local, opt-in latest-state snapshot. Never log identities, balances,
    // inventory counts, login tokens, nonces, raw packets, or passwords.
    std::fprintf(file,"version=2.4.1\nevent=%s\nuptime_ms=%llu\nauthenticated=%d\nverified=%d\nbusy=%d\nrefreshing=%d\nqueued_action=%u\nlast_reply_connected=%d\nserver_result=%s\n",
        event,static_cast<unsigned long long>(GetTickCount64()),bank_authenticated(),verified,busy,refreshing,queued_action,last_reply_connected,result_name(state.result));
    const char* names[]={"Deposit","Withdraw","BuyDiamond","SellDiamond","BuyTicket","SellTicket"};
    for(int i=0;i<6;++i) {
        const char* reason=!verified?"Unverified":transaction_pending()?"Pending":state.result==pn_bank::Saving?"Saving":
            state.result==pn_bank::Unavailable?"Unavailable":result_name(pn_bank::plan(state,i+1,amount(i/2)).result);
        std::fprintf(file,"%s.enabled=%d\n%s.reason=%s\n",names[i],actions[i] && IsWindowEnabled(actions[i]),names[i],reason);
    }
    std::fclose(file);
}
void configure_diagnostics() {
    wchar_t module[MAX_PATH]{};
    const auto length=GetModuleFileNameW(instance,module,MAX_PATH);
    if(!length || length>=MAX_PATH) return;
    std::wstring directory(module);
    auto slash=directory.find_last_of(L"\\/");
    if(slash==std::wstring::npos) return;
    directory.resize(slash+1);
    if(GetPrivateProfileIntW(L"Bank",L"Diagnostics",0,(directory+L"BankUI.ini").c_str()))
        diagnostics_path=directory+L"BankUI-diagnostics.txt";
}
void enable(HWND control,bool enabled) {
    if(control && !!IsWindowEnabled(control)!=enabled) EnableWindow(control,enabled);
}
void update(const char* event="controls") {
    for(int i=0;i<6;++i) {
        int row=i/2; uint32_t action=i+1;
        if(row) caption(actions[i],exchange_label(row,i%2==0));
        auto plan=pn_bank::plan(state,action,amount(row));
        enable(actions[i],actions_ready() && plan.result==pn_bank::Ok);
    }
    enable(GetDlgItem(panel,refresh_id),!transaction_pending());
    InvalidateRect(panel,nullptr,FALSE);
    write_diagnostics(event);
}
void accept_receipt() {
    if(!pending_receipt.action) return;
    if(state.nonce_hi!=pending_receipt.nonce_hi || state.nonce_lo!=pending_receipt.nonce_lo) {
        pending_receipt={}; return;
    }
    if(state.request_id==pending_receipt.id && state.result==pn_bank::Ok) {
        const auto action=pending_receipt.action;
        const wchar_t* past=action==pn_bank::Deposit?L"Deposited":action==pn_bank::Withdraw?L"Withdrew":action%2?L"Bought":L"Sold";
        receipt=std::wstring(L"Saved: ")+past+L" "+commas(pending_receipt.amount);
        if(action<=pn_bank::Withdraw) receipt+=L"z";
        else { receipt+=action<=pn_bank::SellDiamond?L" Diamond":L" Ticket";if(pending_receipt.amount!=1)receipt+=L"s"; }
        receipt+=L".\nBank: "+commas(state.bank,true);
        pending_receipt={};
    } else if(state.request_id>pending_receipt.id ||
        (state.result!=pn_bank::Ok && state.result!=pn_bank::Saving && state.result!=pn_bank::SaveFailed))
        pending_receipt={};
}
void submit(uint32_t action,int64_t value=0) {
    if(preview) return;
    if(action!=pn_bank::Refresh && !actions_ready()) return;
    if(action!=pn_bank::Refresh && pn_bank::plan(state,action,value).result!=pn_bank::Ok) return;
    if(busy) {
        if(action!=pn_bank::Refresh && refreshing) {
            // Keep enabled controls responsive without racing a second worker.
            // Capture the clicked amount and revalidate it against the reply.
            queued_action=action; queued_amount=value;
            status=L"Waiting for the balance check before saving...";
            update("action_queued");
        }
        return;
    }
    uint64_t id=action==pn_bank::Refresh?0:++sequence;
    busy=bank_submit(panel,state,action,value,id);
    refreshing=busy && action==pn_bank::Refresh;
    if(busy) {
        last_refresh=GetTickCount64();
        if(!refreshing) {
            pending_receipt={action,value,id,state.nonce_hi,state.nonce_lo};receipt.clear();
        }
        if(refreshing && verified) { write_diagnostics("refresh_started"); return; }
        status=refreshing?L"Refreshing balances...":L"Saving transaction. Please wait...";
    }
    else { verified=false; status=L"Log in to a character to use the bank."; }
    update("request_started");
}
void show() {
    if(!preview && !bank_authenticated()) { ShowWindow(panel,SW_HIDE); return; }
    if(game) SetWindowLongPtr(panel,GWLP_HWNDPARENT,reinterpret_cast<LONG_PTR>(game));
    if(!IsWindowVisible(panel)) {
        RECT area{};
        if(game) GetWindowRect(game,&area); else SystemParametersInfo(SPI_GETWORKAREA,0,&area,0);
        const int width=px(panel_width)+2, height=px(panel_height)+2;
        int x=area.left+std::max<LONG>(0,(area.right-area.left-width)/2);
        int y=area.top+std::max<LONG>(0,(area.bottom-area.top-height)/2);
        MONITORINFO monitor{}; monitor.cbSize=sizeof(monitor);
        if(GetMonitorInfo(MonitorFromRect(&area,MONITOR_DEFAULTTONEAREST),&monitor)) {
            x=std::max<int>(monitor.rcWork.left,std::min<int>(x,monitor.rcWork.right-width));
            y=std::max<int>(monitor.rcWork.top,std::min<int>(y,monitor.rcWork.bottom-height));
        }
        SetWindowPos(panel,nullptr,x,y,width,height,SWP_NOZORDER);
        ShowWindow(panel,SW_SHOW); SetForegroundWindow(panel); SetFocus(inputs[0]);
    }
    submit(pn_bank::Refresh);
}
LRESULT CALLBACK game_proc(HWND window,UINT message,WPARAM w,LPARAM l) {
    if((message==WM_SYSKEYDOWN || message==WM_KEYDOWN) && w=='B' && ((GetKeyState(VK_MENU)|GetKeyState(VK_CONTROL))&0x8000)) {
        PostMessage(panel,BANK_OPEN,0,0); return 0;
    }
    return CallWindowProc(previous_game_proc,window,message,w,l);
}
void max_menu(int row,HWND source) {
    HMENU menu=CreatePopupMenu();
    const auto first=std::wstring(row==0?L"Deposit ":L"Buy ")+commas(row==0?state.max_deposit:state.max_buy[row-1],row==0);
    const auto second=std::wstring(row==0?L"Withdraw ":L"Sell ")+commas(row==0?state.max_withdraw:state.max_sell[row-1],row==0);
    AppendMenuW(menu,MF_STRING,1,first.c_str());
    AppendMenuW(menu,MF_STRING,2,second.c_str());
    RECT box; GetWindowRect(source,&box);
    int selected=TrackPopupMenu(menu,TPM_RETURNCMD|TPM_NONOTIFY,box.left,box.bottom,0,panel,nullptr); DestroyMenu(menu);
    if(selected) set_amount(row,row==0?(selected==1?state.max_deposit:state.max_withdraw):
        (selected==1?state.max_buy[row-1]:state.max_sell[row-1]));
}
LRESULT CALLBACK window_proc(HWND window,UINT message,WPARAM w,LPARAM l) {
    switch(message) {
    case WM_SETCURSOR:
        if(IsWindowVisible(window)) {
            set_panel_cursor(reinterpret_cast<HWND>(w));
            return TRUE;
        }
        break;
    case WM_SHOWWINDOW:
        if(!w) release_panel_cursor();
        break;
    case WM_ACTIVATE:
        if(LOWORD(w)==WA_INACTIVE) release_panel_cursor();
        break;
    case WM_CREATE:
        panel=window;
        font=CreateFontW(-px(12),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,L"Tahoma");
        balance_font=CreateFontW(-px(14),0,0,0,FW_SEMIBOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,L"Tahoma");
        white=CreateSolidBrush(RGB(255,255,255));
        button(title_close,L"x",panel_width-28,3,22,20);
        for(int row=0;row<3;++row) {
            inputs[row]=CreateWindowExW(WS_EX_CLIENTEDGE,L"EDIT",row==0?L"0":L"1",WS_CHILD|WS_VISIBLE|WS_TABSTOP|ES_AUTOHSCROLL,
                px(row==0?48:44),px(field_y[row]),px(row==0?170:140),px(24),window,reinterpret_cast<HMENU>(edit_ids[row]),instance,nullptr);
            SendMessage(inputs[row],WM_SETFONT,reinterpret_cast<WPARAM>(font),TRUE); SendMessage(inputs[row],EM_SETLIMITTEXT,63,0);
            const auto original=reinterpret_cast<WNDPROC>(SetWindowLongPtrW(inputs[row],GWLP_WNDPROC,reinterpret_cast<LONG_PTR>(input_proc)));
            if(!previous_input_proc) previous_input_proc=original;
            button(300+row,L"x",row==0?223:189,field_y[row],22,24);
            actions[row*2]=button(400+row*2,row==0?L"Deposit":L"Buy",row==0?252:12,action_y[row],row==0?71:190,row==0?24:36);
            actions[row*2+1]=button(401+row*2,row==0?L"Withdraw":L"Sell",row==0?330:210,action_y[row],row==0?70:190,row==0?24:36);
            int count=row==0?5:4;
            const wchar_t* cash[]={L"+1M",L"+10M",L"+100M",L"+1B",L"Max"};
            const wchar_t* items[]={L"+1",L"+10",L"+100",L"Max"};
            const int item_x[]={219,258,301,348}, item_width[]={35,39,43,52};
            for(int i=0;i<count;++i) button(500+row*10+i,row==0?cash[i]:items[i],row==0?12+i*79:item_x[i],preset_y[row],row==0?72:item_width[i],row==0?22:24);
        }
        button(refresh_id,L"Refresh",12,412,126,25);
        button(help_id,L"Bank info",143,412,126,25);
        button(close_id,L"Close",274,412,126,25);
        SetTimer(window,1,250,nullptr); return 0;
    case WM_PAINT: {
        PAINTSTRUCT ps; HDC dc=BeginPaint(window,&ps);
        RECT area{}; GetClientRect(window,&area);
        HDC memory=CreateCompatibleDC(dc);
        HBITMAP bitmap=CreateCompatibleBitmap(dc,area.right,area.bottom);
        if(memory && bitmap) {
            auto old=SelectObject(memory,bitmap); paint(memory);
            BitBlt(dc,0,0,area.right,area.bottom,memory,0,0,SRCCOPY);
            SelectObject(memory,old);
        } else paint(dc);
        if(bitmap) DeleteObject(bitmap);
        if(memory) DeleteDC(memory);
        EndPaint(window,&ps); return 0;
    }
    case WM_PRINTCLIENT: paint(reinterpret_cast<HDC>(w)); return 0;
    case WM_ERASEBKGND: return 1;
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT: SetBkColor(reinterpret_cast<HDC>(w),RGB(255,255,255)); return reinterpret_cast<LRESULT>(white);
    case WM_DRAWITEM: {
        auto draw=reinterpret_cast<DRAWITEMSTRUCT*>(l); auto box=draw->rcItem;
        bool disabled=draw->itemState&ODS_DISABLED;
        gradient(draw->hDC,box,disabled?RGB(247,248,250):RGB(232,237,255),disabled?RGB(232,235,241):RGB(186,201,249));
        auto border=CreateSolidBrush(disabled?RGB(191,198,211):RGB(127,146,196));
        FrameRect(draw->hDC,&box,border); DeleteObject(border);
        InflateRect(&box,-1,-1); FrameRect(draw->hDC,&box,reinterpret_cast<HBRUSH>(GetStockObject(WHITE_BRUSH)));
        wchar_t label[128]{}; GetWindowTextW(draw->hwndItem,label,128); SelectObject(draw->hDC,font);
        SetBkMode(draw->hDC,TRANSPARENT); SetTextColor(draw->hDC,disabled?RGB(96,106,123):RGB(30,48,91));
        if(draw->itemState&ODS_SELECTED) OffsetRect(&box,1,1);
        if(auto split=wcschr(label,L'\n')) {
            *split=0;
            RECT title=box, total=box;
            title.top+=px(1); title.bottom=title.top+px(16);
            total.top=title.bottom; total.bottom=total.top+px(16);
            DrawTextW(draw->hDC,label,-1,&title,DT_CENTER|DT_SINGLELINE|DT_NOPREFIX);
            DrawTextW(draw->hDC,split+1,-1,&total,DT_CENTER|DT_SINGLELINE|DT_NOPREFIX);
        } else DrawTextW(draw->hDC,label,-1,&box,DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX);
        if(draw->itemState&ODS_FOCUS) { InflateRect(&box,-2,-2); DrawFocusRect(draw->hDC,&box); }
        return TRUE;
    }
    case WM_NCHITTEST: {
        POINT point{static_cast<short>(LOWORD(l)),static_cast<short>(HIWORD(l))}; ScreenToClient(window,&point);
        if(point.y>=0 && point.y<px(26) && point.x<px(panel_width-32)) return HTCAPTION;
        break;
    }
    case WM_COMMAND: {
        int id=LOWORD(w);
        if(id>=100 && id<=102) {
            const int row=id-100;
            if(HIWORD(w)==EN_CHANGE) {
                if(normalizing_input) return 0;
                update();return 0;
            }
            if(HIWORD(w)==EN_SETFOCUS || HIWORD(w)==EN_KILLFOCUS) {
                if(normalize_amount(row,HIWORD(w)==EN_KILLFOCUS)) update();
                return 0;
            }
        }
        if(id==close_id || id==title_close || id==IDCANCEL) { ShowWindow(window,SW_HIDE); return 0; }
        if(id==refresh_id) { submit(pn_bank::Refresh); return 0; }
        if(id==help_id) {
            const auto info=L"The bank is shared by all characters on this game login.\n\n"
                L"Deposit moves on-hand zeny into the bank; Withdraw returns it.\n"
                L"Buy spends bank zeny; Sell adds zeny to the bank. Each button shows the total for the entered quantity, including the exchange fee.\n\n"
                L"Max lets you choose the available buy/sell or deposit/withdraw amount.\n\n"
                L"Amounts use plain digits while editing and commas when you leave the field. Ctrl+A selects the whole amount.\n"
                L"Saved receipts appear only after server confirmation.\n\n"
                L"Only eligible items can be sold. Equipped, favorite, bound, modified and rental items are excluded.\n\n"
                L"On-hand limit: 2,147,483,647z\nBank limit: 9,223,372,036,854,775,807z";
            MessageBoxW(window,info,L"Bank info",MB_OK|MB_ICONINFORMATION); return 0;
        }
        if(id>=300 && id<303) { set_amount(id-300,0); return 0; }
        if(id>=400 && id<406) { submit(id-399,amount((id-400)/2)); return 0; }
        if(id>=500 && id<530) {
            int row=(id-500)/10, index=(id-500)%10;
            if(index==(row==0?4:3)) max_menu(row,reinterpret_cast<HWND>(l));
            else {
                int64_t cash[]={1000000,10000000,100000000,1000000000}, item[]={1,10,100};
                int64_t value=std::min<int64_t>(pn_bank::wallet_limit,std::max<int64_t>(0,amount(row)));
                set_amount(row,std::min<int64_t>(pn_bank::wallet_limit,value+(row==0?cash[index]:item[index])));
            }
            return 0;
        }
        break;
    }
    case BANK_OPEN: show(); return 0;
    case BANK_REMOTE_OPEN:
        if(bank_current_generation(static_cast<LONG>(w))) show();
        return 0;
    case BANK_SESSION:
        if(!bank_current_generation(static_cast<LONG>(w),false)) return 0;
        ShowWindow(window,SW_HIDE);
        busy=refreshing=verified=last_reply_connected=false;
        queued_action=pn_bank::Refresh; queued_amount=0; state=pn_bank::Reply{}; sequence=0;
        pending_receipt={};receipt.clear();
        for(int row=0;row<3;++row) set_amount(row,row==0?0:1);
        status=L"Log in to a character to use the bank.";
        if(bank_authenticated()) submit(pn_bank::Refresh);
        update("session_changed"); return 0;
    case BANK_RESULT: {
        auto result=reinterpret_cast<BankResult*>(l);
        if(bank_current_generation(result->generation)) {
            const bool unchanged=refreshing && verified && result->connected &&
                queued_action==pn_bank::Refresh && !std::memcmp(&state,&result->state,sizeof(state));
            const auto next_action=queued_action; const auto next_amount=queued_amount;
            busy=refreshing=false; queued_action=pn_bank::Refresh; queued_amount=0;
            last_reply_connected=result->connected;
            if(result->connected) {
                state=result->state; sequence=std::max(sequence,state.request_id);
                verified=state.result!=pn_bank::Unauthorized;
                accept_receipt();
                if(!unchanged) status=wide(pn_bank::message(state.result));
            } else {
                verified=false; status=L"Connection lost. Refresh to verify the transaction result.";
            }
            if(next_action!=pn_bank::Refresh && result->connected && state.result==pn_bank::Ok && actions_ready()) {
                const auto plan=pn_bank::plan(state,next_action,next_amount);
                if(plan.result==pn_bank::Ok) submit(next_action,next_amount);
                else { status=wide(pn_bank::message(plan.result)); update("queued_action_rejected"); }
            } else if(!unchanged) update("reply_received");
            else write_diagnostics("refresh_unchanged");
        }
        delete result; return 0;
    }
    case WM_TIMER:
        if(w==cursor_timer) {
            POINT position{};
            HWND target=GetCursorPos(&position)?WindowFromPoint(position):nullptr;
            // Owned menus/help windows run on the panel thread too. Do not
            // leave our cursor adjustment active after returning to the game.
            if(IsWindowVisible(window) && target && GetWindowThreadProcessId(target,nullptr)==GetCurrentThreadId())
                set_panel_cursor(target);
            else release_panel_cursor();
            return 0;
        }
        if(!preview && (!game || !IsWindow(game))) {
            game=bank_find_game_window();
            if(game) previous_game_proc=reinterpret_cast<WNDPROC>(SetWindowLongPtr(game,GWLP_WNDPROC,reinterpret_cast<LONG_PTR>(game_proc)));
        }
        // Authenticate while hidden so the game's bank button/NPC/@bank can
        // open this panel through a server notification. Hidden keepalives are
        // infrequent; failed early logins and disconnected sockets retry.
        if(!preview && bank_authenticated() && !busy && GetTickCount64()-last_refresh>
            (state.result==pn_bank::Saving?500:(!verified || IsWindowVisible(window) || !bank_connection_ready()?3000:15000))) submit(pn_bank::Refresh);
        return 0;
    case WM_CLOSE: ShowWindow(window,SW_HIDE); return 0;
    case WM_DESTROY: release_panel_cursor(); KillTimer(window,1); DeleteObject(font);DeleteObject(balance_font);DeleteObject(white);font=balance_font=nullptr;white=nullptr;PostQuitMessage(0);return 0;
    }
    return DefWindowProcW(window,message,w,l);
}
void save_preview(const char* path="bank-preview.bmp") {
    // Render our own hidden window and its controls, without capturing the desktop.
    HDC screen=GetDC(nullptr), memory=CreateCompatibleDC(screen);
    BITMAPINFO info{}; info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
    const int width=px(panel_width),height=px(panel_height);
    info.bmiHeader.biWidth=width; info.bmiHeader.biHeight=-height; info.bmiHeader.biPlanes=1; info.bmiHeader.biBitCount=32;
    void* bits=nullptr; HBITMAP bitmap=CreateDIBSection(screen,&info,DIB_RGB_COLORS,&bits,nullptr,0);
    auto old=SelectObject(memory,bitmap);
    SendMessage(panel,WM_PRINT,reinterpret_cast<WPARAM>(memory),PRF_CLIENT|PRF_CHILDREN|PRF_ERASEBKGND);
    BITMAPFILEHEADER header{}; header.bfType=0x4d42; header.bfOffBits=sizeof(header)+sizeof(info.bmiHeader);
    header.bfSize=header.bfOffBits+width*height*4;
    FILE* output=fopen(path,"wb");
    if(output) { fwrite(&header,sizeof(header),1,output); fwrite(&info.bmiHeader,sizeof(info.bmiHeader),1,output); fwrite(bits,width*height*4,1,output); fclose(output); }
    SelectObject(memory,old); DeleteObject(bitmap); DeleteDC(memory); ReleaseDC(nullptr,screen);
}
}

int bank_window_main(HINSTANCE module,bool render) {
    instance=module; preview=render;
    if(!preview) {
        wchar_t path[MAX_PATH]{};
        if(GetModuleFileNameW(module,path,MAX_PATH)<MAX_PATH) {
            std::wstring ini(path);const auto slash=ini.find_last_of(L"\\/");
            if(slash!=std::wstring::npos) {
                ini.resize(slash+1);ini+=L"BankUI.ini";
                HDC dc=GetDC(nullptr);const int dpi=dc?GetDeviceCaps(dc,LOGPIXELSX):96;if(dc)ReleaseDC(nullptr,dc);
                scale_percent=std::max(100,std::min(150,static_cast<int>(GetPrivateProfileIntW(L"Bank",L"UiScale",std::max(100,MulDiv(dpi,100,96)),ini.c_str()))));
            }
        }
    }
    WNDCLASSW type{}; type.lpfnWndProc=window_proc; type.hInstance=instance;
    type.hCursor=LoadCursor(nullptr,IDC_ARROW); type.lpszClassName=L"PNAccountBank";
    RegisterClassW(&type);
    panel=CreateWindowExW(WS_EX_TOOLWINDOW|WS_EX_CONTROLPARENT,type.lpszClassName,L"Bank",WS_POPUP|WS_BORDER|WS_CLIPCHILDREN,
        100,100,px(panel_width)+2,px(panel_height)+2,nullptr,nullptr,instance,nullptr);
    if(!panel) return 1;
    bank_install_transport(panel);
    if(!preview) configure_diagnostics();
    if(preview) {
        state.result=pn_bank::Ok; state.bank=1834023229; state.wallet=0;
        state.max_deposit=0; state.max_withdraw=state.bank;
        state.counts[0]=1;state.counts[1]=10;
        for(int i=0;i<2;++i) { state.max_buy[i]=state.bank/state.buy[i]; state.max_sell[i]=state.counts[i]; }
        verified=true; status=L"Choose a banking action.";
        set_amount(0,INT64_MAX); assert(!IsWindowEnabled(actions[0]) && !IsWindowEnabled(actions[1]));
        SendMessage(panel,WM_COMMAND,500,0); assert(amount(0)==pn_bank::wallet_limit);
        set_amount(1,3); assert(IsWindowEnabled(actions[2]));
        set_amount(1,4); assert(!IsWindowEnabled(actions[2]));
        set_amount(0,0); set_amount(1,1); set_amount(2,1); update(); save_preview();
        state.bank=INT64_MAX;state.wallet=INT32_MAX;state.max_deposit=state.max_withdraw=0;
        for(int i=0;i<2;++i) { state.max_buy[i]=30000; state.max_sell[i]=0; }
        set_amount(0,1);assert(!IsWindowEnabled(actions[0]) && !IsWindowEnabled(actions[1]));
        save_preview("bank-preview-max.bmp");
        state.bank=1000000;state.wallet=1000000000;state.max_deposit=state.wallet;
        state.max_withdraw=state.bank;
        for(int i=0;i<2;++i) state.counts[i]=state.max_buy[i]=state.max_sell[i]=0;
        update(); save_preview("bank-preview-needs-deposit.bmp");
        state.bank=INT64_MAX;state.wallet=0;state.max_deposit=0;state.max_withdraw=INT32_MAX;
        for(int i=0;i<2;++i) { state.counts[i]=30000;state.max_buy[i]=30000;state.max_sell[i]=0; }
        set_amount(1,INT32_MAX);set_amount(2,INT32_MAX);update();save_preview("bank-preview-large-total.bmp");
        set_amount(1,0);set_amount(2,0);update();save_preview("bank-preview-invalid.bmp");
        verified=false;status=L"Connection lost. Refresh to verify the transaction result.";
        update();save_preview("bank-preview-disconnected.bmp");
        DestroyWindow(panel); return 0;
    }
    update();
    MSG message;
    while(GetMessage(&message,nullptr,0,0)>0) {
        if(!IsWindowVisible(panel) || !IsDialogMessage(panel,&message)) { TranslateMessage(&message); DispatchMessage(&message); }
    }
    return 0;
}
#ifdef PN_BANK_PREVIEW
int main(int argc,char** argv) {
    if(argc==2 && !std::strncmp(argv[1],"--scale=",8)) scale_percent=std::max(100,std::min(150,std::atoi(argv[1]+8)));
    if(argc==2 && !std::strcmp(argv[1],"--scaled")) {
        assert(LoadLibraryW(L"FontScaleOriginal.dll"));
        const auto deadline=GetTickCount64()+5000;
        bool ready=false;
        do {
            auto sample=CreateFontW(-10,0,0,0,400,0,0,0,0,0,0,0,0,L"Tahoma");
            LOGFONTW details{};GetObjectW(sample,sizeof(details),&details);DeleteObject(sample);
            ready=details.lfHeight==-11;
            if(!ready) Sleep(10);
        } while(!ready && GetTickCount64()<deadline);
        assert(ready && "The preview requires the supplied 1.10 font configuration");
    }
    return bank_window_main(GetModuleHandle(nullptr),true);
}
#elif !defined(PN_BANK_UI_TEST)
static DWORD WINAPI bank_start(void* module) { return bank_window_main(static_cast<HINSTANCE>(module),false); }
BOOL WINAPI DllMain(HINSTANCE module,DWORD reason,void*) {
    if(reason==DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(module);
        HANDLE thread=CreateThread(nullptr,0,bank_start,module,0,nullptr); if(thread) CloseHandle(thread);
    }
    return TRUE;
}
#endif
