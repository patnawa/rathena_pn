// Native owned Windows panel, styled after the owner's supplied reference.
// GPL-3.0-or-later. No embedded browser, account password, or client-side balance authority.
#include "bank_client.hpp"
#include "../wide_market/market_client.hpp"
#include "../wide_mail/mail_client.hpp"
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
std::array<HWND,1> inputs{};
std::array<HWND,2> actions{};
HWND trade_input=nullptr;
std::array<HWND,4> trade_buttons{};
uint32_t pending_trade_action=0;
uint64_t pending_trade_request=0;
using TradeConfirmation = int (*)(HWND,LPCWSTR,LPCWSTR,UINT);
TradeConfirmation trade_confirmation=[](HWND owner,LPCWSTR message,LPCWSTR title,UINT flags){return MessageBoxW(owner,message,title,flags);};
constexpr int trade_input_id=600, trade_offer_id=601, trade_lock_id=602, trade_commit_id=603, trade_cancel_id=604;
constexpr int panel_width=412, panel_height=448;
constexpr int field_y=76, preset_y=106;
constexpr int refresh_id=200, close_id=201, title_close=202, help_id=203;
constexpr int market_id=204;
constexpr int mail_id=205;
constexpr int collect_id=206;
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
int64_t control_amount(HWND input) {
    wchar_t text[64]{}; GetWindowTextW(input,text,64);
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
int64_t amount(int row) {return control_amount(inputs[row]);}
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
        if(row<1) normalize_amount(static_cast<int>(row),false);
    }
    const auto result=CallWindowProcW(previous_input_proc,window,message,w,l);
    // Normalize after native replacement finishes, outside EN_CHANGE.
    if((message==WM_PASTE || message==WM_SETTEXT || message==EM_REPLACESEL) && !normalizing_input && GetFocus()==window) {
        const auto row=std::find(inputs.begin(),inputs.end(),window)-inputs.begin();
        if(row<1 && normalize_amount(static_cast<int>(row),false)) update("input_normalized");
    }
    return result;
}
void text(HDC dc,int x,int y,int width,const std::wstring& value,bool right=false) {
    RECT box{px(x),px(y),px(x+width),px(y+20)}; DrawTextW(dc,value.c_str(),-1,&box,DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX|(right?DT_RIGHT:DT_LEFT));
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
bool active_trade() {return verified && state.trade_id && state.partner_account_id;}
std::wstring partner_name() {
    const auto end=static_cast<const char*>(std::memchr(state.partner_name,0,sizeof(state.partner_name)));
    const int length=end?static_cast<int>(end-state.partner_name):static_cast<int>(sizeof(state.partner_name));
    wchar_t converted[48]{};
    int count=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,state.partner_name,length,converted,48);
    if(!count)count=MultiByteToWideChar(CP_ACP,0,state.partner_name,length,converted,48);
    return std::wstring(converted,count);
}
const wchar_t* trade_stage(uint32_t value) {
    return value==2?L"Confirmed":value==1?L"Locked":L"Editing";
}
void paint_trade(HDC dc) {
    line(dc,px(12),px(240),px(panel_width-12),px(240),RGB(218,223,233));
    text(dc,12,245,panel_width-24,active_trade()?L"Trading with "+partner_name():L"Trade: start a player trade in the game");
    text(dc,12,268,105,L"Your offer");
    text(dc,118,268,282,active_trade()?commas(state.own_offer,true):L"--",true);
    text(dc,12,290,105,L"Their offer");
    text(dc,118,290,282,active_trade()?commas(state.partner_offer,true):L"--",true);
    text(dc,12,312,388,active_trade()?std::wstring(L"You: ")+trade_stage(state.own_trade_state)+L"    Partner: "+trade_stage(state.partner_trade_state):L"No active trade");
    text(dc,12,337,38,L"Offer");
    RECT edit{px(52),px(337),px(282),px(361)};
    FillRect(dc,&edit,reinterpret_cast<HBRUSH>(GetStockObject(WHITE_BRUSH)));
    FrameRect(dc,&edit,reinterpret_cast<HBRUSH>(GetStockObject(LTGRAY_BRUSH)));
    wchar_t offer[64]{};GetWindowTextW(trade_input,offer,64);text(dc,56,339,222,offer);
    text(dc,12,416,388,L"Review the items in the game's trade window before confirming.");
}
void paint(HDC dc) {
    RECT all{0,0,px(panel_width),px(panel_height)}; FillRect(dc,&all,white); SelectObject(dc,font);
    SetBkMode(dc,TRANSPARENT); SetTextColor(dc,RGB(37,48,68));
    gradient(dc,RECT{0,0,px(panel_width),px(26)},RGB(194,205,249),RGB(234,239,255));
    text(dc,12,3,190,L"Wallet & Bank");
    text(dc,panel_width-84,3,42,L"v3.0",true);
    text(dc,12,30,86,L"In bank"); text(dc,12,49,86,L"Wallet");
    SelectObject(dc,balance_font);
    text(dc,100,30,panel_width-112,verified?commas(state.bank,true):L"--",true);
    text(dc,100,49,panel_width-112,verified?commas(state.wallet,true):L"--",true);
    SelectObject(dc,font);
    text(dc,12,field_y+1,34,L"Zeny");
    RECT backing{px(48),px(field_y),px(218),px(field_y+24)};
    FillRect(dc,&backing,reinterpret_cast<HBRUSH>(GetStockObject(WHITE_BRUSH)));
    FrameRect(dc,&backing,reinterpret_cast<HBRUSH>(GetStockObject(LTGRAY_BRUSH)));
    wchar_t value[64]{}; GetWindowTextW(inputs[0],value,64);
    text(dc,52,field_y+2,162,value);
    line(dc,px(12),px(138),px(panel_width-12),px(138),RGB(218,223,233));
    text(dc,12,143,panel_width-24,L"Wallet limit: 9,223,372,036,854,775,807z");
    text(dc,12,163,panel_width-24,L"Bank limit: 9,223,372,036,854,775,807z");
    auto guidance=status;
    if(!receipt.empty() && verified && !transaction_pending() && state.result==pn_bank::Ok && status==wide(pn_bank::message(pn_bank::Ok))) guidance=receipt;
    else if(actions_ready() && state.result==pn_bank::Ok && status==wide(pn_bank::message(pn_bank::Ok))) {
        guidance=L"Deposit or withdraw Zeny. Max fills the available amount.";
        if(GetFocus()==inputs[0] && amount(0)<=0) guidance=L"Enter the next Zeny amount.";
    }
    SetTextColor(dc,RGB(63,73,91));
    RECT footer{px(12),px(213),px(panel_width-12),px(238)};
    DrawTextW(dc,guidance.c_str(),-1,&footer,DT_WORDBREAK|DT_NOPREFIX);
    paint_trade(dc);
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
    std::fprintf(file,"version=3.0\nevent=%s\nuptime_ms=%llu\nauthenticated=%d\nverified=%d\nbusy=%d\nrefreshing=%d\nqueued_action=%u\nlast_reply_connected=%d\nserver_result=%s\n",
        event,static_cast<unsigned long long>(GetTickCount64()),bank_authenticated(),verified,busy,refreshing,queued_action,last_reply_connected,result_name(state.result));
    const char* names[]={"Deposit","Withdraw"};
    for(int i=0;i<2;++i) {
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
    if(GetPrivateProfileIntW(L"Bank",L"Diagnostics",0,(directory+L"PNWallet64.ini").c_str()))
        diagnostics_path=directory+L"PNWallet64-diagnostics.txt";
}
void enable(HWND control,bool enabled) {
    if(control && !!IsWindowEnabled(control)!=enabled) EnableWindow(control,enabled);
}
void update(const char* event="controls") {
    for(int i=0;i<2;++i) {
        int row=i/2; uint32_t action=i+1;
        auto plan=pn_bank::plan(state,action,amount(row));
        enable(actions[i],!active_trade() && actions_ready() && plan.result==pn_bank::Ok);
    }
    const bool ready=active_trade() && !busy && !pending_trade_action && state.result!=pn_bank::Saving;
    enable(trade_input,ready && state.own_trade_state==0);
    enable(trade_buttons[0],ready && state.own_trade_state==0 && control_amount(trade_input)>=0 && control_amount(trade_input)<=state.wallet);
    enable(trade_buttons[1],ready && state.own_trade_state==0);
    enable(trade_buttons[2],ready && state.own_trade_state==1 && state.partner_trade_state>=1);
    enable(trade_buttons[3],ready);
    enable(GetDlgItem(panel,refresh_id),!transaction_pending());
    enable(GetDlgItem(panel,market_id),verified&&!transaction_pending());
    enable(GetDlgItem(panel,mail_id),verified&&!transaction_pending());
    enable(GetDlgItem(panel,collect_id),!active_trade() && actions_ready());
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
        if(action==pn_bank::CollectOffline) {
            receipt=L"Saved: collected "+std::to_wstring(state.counts[0])+L" offline characters; skipped "+std::to_wstring(state.counts[1])+L".\nBank: "+commas(state.bank,true);
            pending_receipt={};return;
        }
        const wchar_t* past=action==pn_bank::Deposit?L"Deposited":L"Withdrew";
        receipt=std::wstring(L"Saved: ")+past+L" "+commas(pending_receipt.amount);
        receipt+=L"z";
        receipt+=L".\nBank: "+commas(state.bank,true);
        pending_receipt={};
    } else if(state.request_id>pending_receipt.id ||
        (state.result!=pn_bank::Ok && state.result!=pn_bank::Saving && state.result!=pn_bank::SaveFailed))
        pending_receipt={};
}
void submit(uint32_t action,int64_t value=0) {
    if(preview || (action>pn_bank::Withdraw && action!=pn_bank::CollectOffline)) return;
    if(action!=pn_bank::Refresh && (!actions_ready() || active_trade())) return;
    if(action==pn_bank::CollectOffline && value) return;
    if(action!=pn_bank::Refresh && action!=pn_bank::CollectOffline && pn_bank::plan(state,action,value).result!=pn_bank::Ok) return;
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
        if(refreshing && verified) {
            if(active_trade())update("trade_refresh_started");else write_diagnostics("refresh_started");
            return;
        }
        status=refreshing?L"Refreshing balances...":L"Saving transaction. Please wait...";
    }
    else { verified=false; status=L"Log in to a character to use the bank."; }
    update("request_started");
}
void submit_trade(uint32_t action,int64_t value=0) {
    if(preview || !active_trade() || busy || pending_trade_action || action<pn_bank::TradeSetOffer || action>pn_bank::TradeCancel) return;
    if(action==pn_bank::TradeSetOffer && (state.own_trade_state || value<0 || value>state.wallet))return;
    if(action==pn_bank::TradeLock && state.own_trade_state)return;
    if(action==pn_bank::TradeCommit && (state.own_trade_state!=1 || state.partner_trade_state<1))return;
    const auto id=++sequence;
    busy=bank_submit(panel,state,action,value,id);refreshing=false;
    if(busy){pending_trade_action=action;pending_trade_request=id;last_refresh=GetTickCount64();status=L"Waiting for the trade server...";}
    else {verified=false;status=L"Connection lost. Refresh before trading.";}
    update("trade_request");
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
void max_menu(HWND source) {
    HMENU menu=CreatePopupMenu();
    const auto first=std::wstring(L"Deposit ")+commas(state.max_deposit,true);
    const auto second=std::wstring(L"Withdraw ")+commas(state.max_withdraw,true);
    AppendMenuW(menu,MF_STRING,1,first.c_str());
    AppendMenuW(menu,MF_STRING,2,second.c_str());
    RECT box; GetWindowRect(source,&box);
    int selected=TrackPopupMenu(menu,TPM_RETURNCMD|TPM_NONOTIFY,box.left,box.bottom,0,panel,nullptr); DestroyMenu(menu);
    if(selected) set_amount(0,selected==1?state.max_deposit:state.max_withdraw);
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
    case WM_CREATE: {
        panel=window;
        font=CreateFontW(-px(12),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,L"Tahoma");
        balance_font=CreateFontW(-px(14),0,0,0,FW_SEMIBOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,L"Tahoma");
        white=CreateSolidBrush(RGB(255,255,255));
        button(title_close,L"x",panel_width-28,3,22,20);
        button(market_id,L"Market",154,3,80,20);
        button(mail_id,L"Mail",242,3,80,20);
        inputs[0]=CreateWindowExW(WS_EX_CLIENTEDGE,L"EDIT",L"0",WS_CHILD|WS_VISIBLE|WS_TABSTOP|ES_AUTOHSCROLL,
            px(48),px(field_y),px(170),px(24),window,reinterpret_cast<HMENU>(100),instance,nullptr);
        SendMessage(inputs[0],WM_SETFONT,reinterpret_cast<WPARAM>(font),TRUE);
        SendMessage(inputs[0],EM_SETLIMITTEXT,63,0);
        previous_input_proc=reinterpret_cast<WNDPROC>(SetWindowLongPtrW(inputs[0],GWLP_WNDPROC,reinterpret_cast<LONG_PTR>(input_proc)));
        button(300,L"x",223,field_y,22,24);
        actions[0]=button(400,L"Deposit",252,field_y,71,24);
        actions[1]=button(401,L"Withdraw",330,field_y,70,24);
        const wchar_t* cash[]={L"+100K",L"+1M",L"+10M",L"+100M",L"+1B",L"+10B",L"Max"};
        for(int i=0;i<7;++i) button(500+i,cash[i],12+i*56,preset_y,52,22);
        button(refresh_id,L"Refresh",12,184,94,25);
        button(help_id,L"Bank info",110,184,94,25);
        button(collect_id,L"Collect offline",208,184,94,25);
        button(close_id,L"Close",306,184,94,25);
        trade_input=CreateWindowExW(WS_EX_CLIENTEDGE,L"EDIT",L"0",WS_CHILD|WS_VISIBLE|WS_TABSTOP|ES_AUTOHSCROLL,
            px(52),px(337),px(230),px(24),window,reinterpret_cast<HMENU>(trade_input_id),instance,nullptr);
        SendMessage(trade_input,WM_SETFONT,reinterpret_cast<WPARAM>(font),TRUE);SendMessage(trade_input,EM_SETLIMITTEXT,63,0);
        trade_buttons[0]=button(trade_offer_id,L"Set offer",289,337,111,24);
        trade_buttons[1]=button(trade_lock_id,L"Lock offer",12,374,126,28);
        trade_buttons[2]=button(trade_commit_id,L"Confirm trade",143,374,126,28);
        trade_buttons[3]=button(trade_cancel_id,L"Cancel trade",274,374,126,28);
        for(auto control:trade_buttons)EnableWindow(control,FALSE);
        EnableWindow(trade_input,FALSE);
        SetTimer(window,1,250,nullptr); return 0;
    }
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
        if(id==trade_input_id && HIWORD(w)==EN_CHANGE) {update();return 0;}
        if(id==trade_offer_id){submit_trade(pn_bank::TradeSetOffer,control_amount(trade_input));return 0;}
        if(id==trade_lock_id){submit_trade(pn_bank::TradeLock);return 0;}
        if(id==trade_cancel_id){submit_trade(pn_bank::TradeCancel);return 0;}
        if(id==trade_commit_id) {
            if(!active_trade() || busy || state.own_trade_state!=1 || state.partner_trade_state<1)return 0;
            const auto trade=state.trade_id,revision=state.trade_revision;
            const auto details=L"Trade with "+partner_name()+L"\n\nYou give: "+commas(state.own_offer,true)+
                L"\nYou receive: "+commas(state.partner_offer,true)+L"\n\nAlso verify every item in the game's trade window. Confirm this trade?";
            if(trade_confirmation(window,details.c_str(),L"Confirm exact trade",MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2)==IDYES) {
                if(trade==state.trade_id && revision==state.trade_revision)submit_trade(pn_bank::TradeCommit);
                else {status=L"The trade changed. Review the current offers again.";update();}
            }
            return 0;
        }
        if(id==100) {
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
        if(id==collect_id) { submit(pn_bank::CollectOffline); return 0; }
        if(id==market_id) {if(verified)market_open(window,state);return 0;}
        if(id==mail_id) {if(verified)mail_open(window,state);return 0;}
        if(id==help_id) {
            const auto info=L"The bank is shared by all characters on this game login.\n\n"
                L"Deposit moves on-hand zeny into the bank; Withdraw returns it.\n"
                L"Max lets you choose the available deposit or withdrawal amount.\n\n"
                L"Amounts use plain digits while editing and commas when you leave the field. Ctrl+A selects the whole amount.\n"
                L"Saved receipts appear only after server confirmation.\n\n"
                L"Wallet limit: 9,223,372,036,854,775,807z\nBank limit: 9,223,372,036,854,775,807z";
            MessageBoxW(window,info,L"Bank info",MB_OK|MB_ICONINFORMATION); return 0;
        }
        if(id==300) { set_amount(0,0); return 0; }
        if(id>=400 && id<402) { submit(id-399,amount((id-400)/2)); return 0; }
        if(id>=500 && id<507) {
            int index=id-500;
            if(index==6) max_menu(reinterpret_cast<HWND>(l));
            else {
                int64_t cash[]={100000,1000000,10000000,100000000,1000000000,10000000000LL};
                int64_t value=std::min<int64_t>(pn_bank::wallet_limit,std::max<int64_t>(0,amount(0)));
                set_amount(0,value+std::min<int64_t>(pn_bank::wallet_limit-value,cash[index]));
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
        pending_trade_action=0;pending_trade_request=0;SetWindowTextW(trade_input,L"0");
        set_amount(0,0);
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
                const auto previous_trade=state.trade_id;
                state=result->state; sequence=std::max(sequence,state.request_id);
                verified=state.result!=pn_bank::Unauthorized;
                accept_receipt();
                if(!unchanged) status=wide(pn_bank::message(state.result));
                if(previous_trade!=state.trade_id) {
                    SetWindowTextW(trade_input,L"0");
                    if(state.trade_id){receipt.clear();pending_receipt={};}
                }
                if(pending_trade_action && (state.request_id>=pending_trade_request ||
                    (state.result!=pn_bank::Ok && state.result!=pn_bank::Saving))) {
                    pending_trade_action=0;pending_trade_request=0;
                    status=state.result==pn_bank::Ok?L"Trade updated. Review both offers before confirming.":wide(pn_bank::message(state.result));
                }
            } else {
                pending_trade_action=0;pending_trade_request=0;
                verified=false; status=L"Connection lost. Refresh to verify the transaction result.";
            }
            if(next_action!=pn_bank::Refresh && result->connected && state.result==pn_bank::Ok && actions_ready()) {
                const auto plan=pn_bank::plan(state,next_action,next_amount);
                if((next_action==pn_bank::CollectOffline && !active_trade()) || plan.result==pn_bank::Ok) submit(next_action,next_amount);
                else { status=wide(pn_bank::message(plan.result)); update("queued_action_rejected"); }
            } else if(!unchanged || active_trade()) update("reply_received");
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
            (state.result==pn_bank::Saving || active_trade()?500:(!verified || IsWindowVisible(window) || !bank_connection_ready()?3000:15000))) submit(pn_bank::Refresh);
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
                ini.resize(slash+1);ini+=L"PNWallet64.ini";
                HDC dc=GetDC(nullptr);const int dpi=dc?GetDeviceCaps(dc,LOGPIXELSX):96;if(dc)ReleaseDC(nullptr,dc);
                scale_percent=std::max(100,std::min(150,static_cast<int>(GetPrivateProfileIntW(L"Bank",L"UiScale",std::max(100,MulDiv(dpi,100,96)),ini.c_str()))));
            }
        }
    }
    WNDCLASSW type{}; type.lpfnWndProc=window_proc; type.hInstance=instance;
    type.hCursor=LoadCursor(nullptr,IDC_ARROW); type.lpszClassName=L"PNWallet64";
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
        set_amount(0,1000000); update(); save_preview();
        state.bank=INT64_MAX;state.wallet=INT64_MAX;state.max_deposit=state.max_withdraw=0;
        set_amount(0,1);assert(!IsWindowEnabled(actions[0]) && !IsWindowEnabled(actions[1]));
        save_preview("bank-preview-max.bmp");
        state.bank=9007199254740993LL;state.wallet=4294967296LL;state.max_deposit=std::min(state.wallet,INT64_MAX-state.bank);state.max_withdraw=std::min(state.bank,INT64_MAX-state.wallet);
        set_amount(0,1000000);update();save_preview("bank-preview-transfer.bmp");
        set_amount(0,0);update();save_preview("bank-preview-invalid.bmp");
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
