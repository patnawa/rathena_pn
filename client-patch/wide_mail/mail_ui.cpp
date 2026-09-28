// Zeny-only RODEX composer. Only a server-confirmed durable receipt is "Saved".
#include "mail_client.hpp"
#include "../wide_market/numbers.hpp"
#include <algorithm>
#include <cstring>
#include <string>

namespace mail_ui {
HWND window=nullptr,recipient=nullptr,title=nullptr,body=nullptr,amount=nullptr,summary=nullptr,status_label=nullptr;
HFONT font=nullptr;LONG generation=0;
bool busy=false,verified=false,quoted=false;
uint64_t sequence=0,pending_send=0;uint32_t active_action=pn_mail::Status;
pn_mail::Reply state;
pn_mail::Request quoted_form,pending_form;
ULONGLONG last_poll=0;
constexpr int quote_id=101,send_id=102,status_id=103;
using Confirmation=int(*)(HWND,LPCWSTR,LPCWSTR,UINT);
Confirmation confirm=[](HWND owner,LPCWSTR message,LPCWSTR caption,UINT flags){return MessageBoxW(owner,message,caption,flags);};
std::wstring text(HWND control){wchar_t content[1024]{};GetWindowTextW(control,content,1024);return content;}
bool encode(HWND control,char* target,int capacity) {
    const auto value=text(control);if(value.empty()){target[0]=0;return true;}
    int count=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,value.data(),value.size(),target,capacity-1,nullptr,nullptr);
    if(!count)return false;
    target[count]=0;return true;
}
bool form(pn_mail::Request& request) {
    return encode(recipient,request.recipient,sizeof(request.recipient))&&request.recipient[0]&&
        encode(title,request.title,sizeof(request.title))&&request.title[0]&&encode(body,request.body,sizeof(request.body))&&
        pn_market_ui::parse_amount(text(amount),request.amount)&&request.amount>0;
}
bool matches_quote() {
    pn_mail::Request current;
    return quoted&&form(current)&&current.amount==quoted_form.amount&&
        !std::memcmp(current.recipient,quoted_form.recipient,sizeof(current.recipient))&&
        !std::memcmp(current.title,quoted_form.title,sizeof(current.title))&&
        !std::memcmp(current.body,quoted_form.body,sizeof(current.body));
}
void update() {
    const bool editable=!busy&&!pending_send;
    for(HWND control:{recipient,title,body,amount})EnableWindow(control,editable);
    EnableWindow(GetDlgItem(window,quote_id),verified&&editable);
    EnableWindow(GetDlgItem(window,send_id),verified&&editable&&matches_quote()&&state.result==pn_mail::Ok);
    EnableWindow(GetDlgItem(window,status_id),!busy);
    auto value=L"Wallet: "+pn_market_ui::format_amount(state.wallet)+L"z";
    if(quoted)value+=L"\r\nFee: "+pn_market_ui::format_amount(state.fee)+L"z    Total: "+pn_market_ui::format_amount(state.total)+L"z";
    SetWindowTextW(summary,value.c_str());
}
bool dispatch(pn_mail::Request request) {
    if(busy)return false;
    request.nonce_hi=state.nonce_hi;request.nonce_lo=state.nonce_lo;request.request_id=++sequence;
    active_action=request.action;busy=mail_submit(window,request);last_poll=GetTickCount64();
    if(busy) {
        if(request.action==pn_mail::Send){pending_send=request.request_id;pending_form=request;quoted=false;}
        SetWindowTextW(status_label,request.action==pn_mail::Status?L"Checking the saved send result...":L"Waiting for the mail server...");
    }else {verified=false;SetWindowTextW(status_label,L"Connection unavailable. Check status before sending again.");}
    update();return busy;
}
void quote() {
    if(busy||pending_send)return;
    pn_mail::Request request;
    if(!form(request)){SetWindowTextW(status_label,L"Enter a recipient, title and valid Zeny amount. Shorten text that exceeds the mail limit.");return;}
    request.action=pn_mail::Quote;quoted_form=request;quoted=false;dispatch(request);
}
void send_mail() {
    if(busy||pending_send||!verified||!matches_quote()||state.result!=pn_mail::Ok)return;
    const auto recipient_text=text(recipient);
    const auto review=L"Recipient: "+recipient_text+L"\n\nZeny: "+pn_market_ui::format_amount(quoted_form.amount)+
        L"z\nFee: "+pn_market_ui::format_amount(state.fee)+L"z\nTotal debit: "+pn_market_ui::format_amount(state.total)+L"z\n\nSend this mail?";
    const auto total=state.total,fee=state.fee;const auto nonce_hi=state.nonce_hi,nonce_lo=state.nonce_lo;
    if(confirm(window,review.c_str(),L"Confirm Zeny mail",MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2)!=IDYES)return;
    if(!matches_quote()||state.total!=total||state.fee!=fee||nonce_hi!=state.nonce_hi||nonce_lo!=state.nonce_lo) {
        quoted=false;SetWindowTextW(status_label,L"The message or fee changed. Request a new quote.");update();return;
    }
    auto request=quoted_form;request.action=pn_mail::Send;request.expected_total=total;dispatch(request);
}
void check_status(){pn_mail::Request request;request.action=pn_mail::Status;dispatch(request);}
HWND control(LPCWSTR type,LPCWSTR caption,DWORD style,int id,int x,int y,int width,int height) {
    auto child=CreateWindowExW(std::wstring(type)==L"EDIT"?WS_EX_CLIENTEDGE:0,type,caption,WS_CHILD|WS_VISIBLE|style,x,y,width,height,window,reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),GetModuleHandle(nullptr),nullptr);
    SendMessage(child,WM_SETFONT,reinterpret_cast<WPARAM>(font),TRUE);return child;
}
const wchar_t* result_text(uint32_t result) {
    const wchar_t* labels[]={L"Quote ready. Review the recipient and exact total before sending.",L"The server is saving your mail. Checking status...",L"Log in to a character.",L"The mail request is invalid.",L"Finish your current action and try again.",L"Not enough Zeny for the amount plus fee.",L"The session or quoted fee changed. Check status and request a new quote.",L"The mail could not be saved. Check status before trying again."};
    return result<=pn_mail::Failed?labels[result]:L"Unexpected server response.";
}
LRESULT CALLBACK proc(HWND hwnd,UINT message,WPARAM w,LPARAM l) {
    switch(message) {
    case WM_CREATE:
        window=hwnd;font=CreateFontW(-14,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,0,0,L"Segoe UI");
        control(L"STATIC",L"Recipient",0,0,16,19,88,20);recipient=control(L"EDIT",L"",WS_TABSTOP|ES_AUTOHSCROLL,201,108,15,490,27);
        control(L"STATIC",L"Title",0,0,16,59,88,20);title=control(L"EDIT",L"",WS_TABSTOP|ES_AUTOHSCROLL,202,108,55,490,27);
        control(L"STATIC",L"Message",0,0,16,99,88,20);body=control(L"EDIT",L"",WS_TABSTOP|ES_MULTILINE|ES_AUTOVSCROLL|WS_VSCROLL,203,108,95,490,180);
        control(L"STATIC",L"Zeny",0,0,16,291,88,20);amount=control(L"EDIT",L"0",WS_TABSTOP|ES_AUTOHSCROLL,204,108,287,300,27);
        summary=control(L"STATIC",L"Wallet: --",0,0,16,331,584,48);
        control(L"BUTTON",L"Get exact quote",WS_TABSTOP|BS_PUSHBUTTON,quote_id,16,393,180,32);
        control(L"BUTTON",L"Confirm and send",WS_TABSTOP|BS_PUSHBUTTON,send_id,212,393,184,32);
        control(L"BUTTON",L"Check send status",WS_TABSTOP|BS_PUSHBUTTON,status_id,412,393,186,32);
        status_label=control(L"STATIC",L"Open received mail in the game's RODEX mailbox.",0,0,16,442,584,50);
        SetTimer(hwnd,1,500,nullptr);update();return 0;
    case WM_COMMAND:
        if(HIWORD(w)==EN_CHANGE){quoted=false;update();return 0;}
        if(LOWORD(w)==quote_id)quote();else if(LOWORD(w)==send_id)send_mail();else if(LOWORD(w)==status_id)check_status();return 0;
    case MAIL_RESULT: {
        auto result=reinterpret_cast<MailResult*>(l);
        if(result->generation==generation&&bank_current_generation(generation)) {
            busy=false;verified=result->connected&&result->state.result!=pn_mail::Denied;
            if(!result->connected){quoted=false;SetWindowTextW(status_label,L"Connection lost. Check send status; the mail will not be sent again automatically.");}
            else {
                state=result->state;sequence=std::max(sequence,state.send_sequence);
                if(active_action==pn_mail::Quote)quoted=state.result==pn_mail::Ok&&state.amount==quoted_form.amount;
                SetWindowTextW(status_label,result_text(state.result));
                if(pending_send&&active_action==pn_mail::Send&&state.send_sequence!=pending_send&&
                    state.result!=pn_mail::Ok&&state.result!=pn_mail::Pending) {pending_send=0;quoted=false;}
                if(pending_send&&active_action==pn_mail::Status&&state.send_sequence==pending_send&&state.amount==pending_form.amount&&state.total==pending_form.expected_total) {
                    if(state.result==pn_mail::Ok&&!state.pending) {
                        pending_send=0;quoted=false;SetWindowTextW(amount,L"0");SetWindowTextW(status_label,L"Saved: the mail and Zeny debit were confirmed by the server.");
                    }else if(state.result==pn_mail::Failed){pending_send=0;quoted=false;}
                }else if(!pending_send&&active_action==pn_mail::Status&&state.result==pn_mail::Ok)SetWindowTextW(status_label,L"Mail status checked. Enter a message and request an exact quote.");
                // Sending never directly produces a Saved receipt: poll the durable outcome.
                if(pending_send){last_poll=0;SetWindowTextW(status_label,L"Checking the durable send result. This mail will not be resent automatically.");}
            }
            update();
        }
        delete result;return 0;
    }
    case WM_TIMER:
        if(generation&&!bank_current_generation(generation)){generation=0;verified=busy=quoted=false;pending_send=0;state=pn_mail::Reply{};mail_forget_session();ShowWindow(hwnd,SW_HIDE);SetWindowTextW(amount,L"0");update();}
        else if(pending_send&&!busy&&GetTickCount64()-last_poll>=1000)check_status();
        return 0;
    case WM_CLOSE:ShowWindow(hwnd,SW_HIDE);return 0;
    case WM_DESTROY:KillTimer(hwnd,1);DeleteObject(font);font=nullptr;window=nullptr;return 0;
    }
    return DefWindowProcW(hwnd,message,w,l);
}
}
void mail_open(HWND owner,const pn_bank::Reply& bank) {
    using namespace mail_ui;BankSession session;if(!bank_session_snapshot(session))return;
    if(!window) {
        WNDCLASSW type{};type.lpfnWndProc=proc;type.hInstance=GetModuleHandle(nullptr);type.hCursor=LoadCursor(nullptr,IDC_ARROW);type.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_BTNFACE+1);type.lpszClassName=L"PNWideMail";RegisterClassW(&type);
        RECT area{0,0,616,500};AdjustWindowRectEx(&area,WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,FALSE,WS_EX_TOOLWINDOW);
        window=CreateWindowExW(WS_EX_TOOLWINDOW,type.lpszClassName,L"Send Zeny by mail",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,CW_USEDEFAULT,CW_USEDEFAULT,area.right-area.left,area.bottom-area.top,owner,nullptr,type.hInstance,nullptr);
    }
    if(generation!=session.generation){state=pn_mail::Reply{};sequence=0;pending_send=0;verified=busy=quoted=false;generation=session.generation;}
    state.nonce_hi=bank.nonce_hi;state.nonce_lo=bank.nonce_lo;ShowWindow(window,SW_SHOW);SetForegroundWindow(window);check_status();
}
