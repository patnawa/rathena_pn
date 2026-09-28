// Real Win32 controls, exact 64-bit input and wallet/bank transfer requests.
#define PN_BANK_UI_TEST
#include "../../client-patch/wide_zeny/bank_ui.cpp"
#include <iostream>

namespace fixture {
struct Request { uint32_t action; int64_t amount; uint64_t id,trade,revision; };
std::vector<Request> sent;
}
void bank_install_transport(HWND) {}
void market_open(HWND,const pn_bank::Reply&) {}
void mail_open(HWND,const pn_bank::Reply&) {}
bool bank_authenticated() {return true;}
bool bank_current_generation(LONG value,bool) {return value==7;}
bool bank_connection_ready() {return true;}
HWND bank_find_game_window() {return nullptr;}
bool bank_submit(HWND,const pn_bank::Reply& state,uint32_t action,int64_t value,uint64_t id) {
    fixture::sent.push_back({action,value,id,state.trade_id,state.trade_revision});return true;
}
void reply(pn_bank::Reply value) {
    value.max_deposit=std::min(value.wallet,INT64_MAX-value.bank);
    value.max_withdraw=std::min(value.bank,INT64_MAX-value.wallet);
    assert(pn_bank::valid_reply(value));
    SendMessage(panel,BANK_RESULT,0,reinterpret_cast<LPARAM>(new BankResult{value,7,true}));
}
int main(int argc,char** argv) {
    if(argc==2)scale_percent=std::atoi(argv[1]);
    assert(pn_bank::version==3 && pn_bank::wallet_limit==INT64_MAX);
    instance=GetModuleHandle(nullptr);
    WNDCLASSW type{};type.lpfnWndProc=window_proc;type.hInstance=instance;type.lpszClassName=L"PNWallet64Fixture";
    assert(RegisterClassW(&type));
    panel=CreateWindowExW(WS_EX_TOOLWINDOW,type.lpszClassName,L"",WS_POPUP|WS_BORDER|WS_CLIPCHILDREN,
        0,0,px(panel_width)+2,px(panel_height)+2,nullptr,nullptr,instance,nullptr);
    assert(panel);
    pn_bank::Reply value;value.result=pn_bank::Ok;value.bank=0;value.wallet=INT64_MAX;value.nonce_hi=123;value.nonce_lo=456;
    reply(value);
    for(int64_t n:{2147483648LL,4294967296LL,9007199254740993LL,INT64_MAX}) {
        set_amount(0,n);assert(amount(0)==n);
        assert(IsWindowEnabled(actions[0]) && !IsWindowEnabled(actions[1]));
    }
    assert(commas(INT64_MAX)==L"9,223,372,036,854,775,807");
    SetWindowTextW(inputs[0],L"9,007,199,254,740,993");assert(amount(0)==9007199254740993LL);
    for(const wchar_t* invalid:{L"9223372036854775808",L"-1",L"1e9",L"9,00",L"1.2"}) {
        SetWindowTextW(inputs[0],invalid);assert(amount(0)<0);assert(!IsWindowEnabled(actions[0]));
    }
    for(int id=500;id<=505;++id) {
        set_amount(0,INT64_MAX-1);SendMessage(panel,WM_COMMAND,id,0);assert(amount(0)==INT64_MAX);
        SendMessage(panel,WM_COMMAND,id,0);assert(amount(0)==INT64_MAX);
    }
    set_amount(0,0);SendMessage(panel,WM_COMMAND,505,0);assert(amount(0)==10000000000LL);
    set_amount(0,9007199254740993LL);submit(pn_bank::Deposit,amount(0));
    assert(fixture::sent.size()==1 && fixture::sent.back().amount==9007199254740993LL);
    submit(pn_bank::Deposit,1);assert(fixture::sent.size()==1);
    value.request_id=fixture::sent.back().id;value.result=pn_bank::Saving;reply(value);assert(receipt.empty());
    value.result=pn_bank::Ok;value.bank=9007199254740993LL;value.wallet=INT64_MAX-value.bank;reply(value);
    assert(receipt.find(L"9,007,199,254,740,993z")!=std::wstring::npos);
    const auto count=fixture::sent.size();for(uint32_t action=3;action<=6;++action)submit(action,1);
    assert(fixture::sent.size()==count);
    auto legacy=value;legacy.protocol=2;assert(!pn_bank::valid_reply(legacy));
    SendMessage(panel,WM_COMMAND,collect_id,0);
    assert(fixture::sent.back().action==pn_bank::CollectOffline && fixture::sent.back().amount==0);
    const auto collecting=fixture::sent.size();
    SendMessage(panel,WM_COMMAND,collect_id,0);assert(fixture::sent.size()==collecting);
    value.request_id=fixture::sent.back().id;value.result=pn_bank::Saving;reply(value);
    assert(!IsWindowEnabled(GetDlgItem(panel,collect_id)) && receipt.empty());
    value.result=pn_bank::Ok;value.counts[0]=2;value.counts[1]=1;reply(value);
    assert(receipt.find(L"collected 2 offline characters; skipped 1")!=std::wstring::npos);
    value.bank=INT64_MAX;value.wallet=INT64_MAX;reply(value);set_amount(0,1);
    assert(!IsWindowEnabled(actions[0]) && !IsWindowEnabled(actions[1]));
    value.trade_id=91;value.trade_revision=1;value.partner_account_id=1234;value.partner_char_id=5678;
    std::strcpy(value.partner_name,"Fixture Partner");value.partner_offer=9007199254740993LL;reply(value);
    assert(active_trade() && partner_name()==L"Fixture Partner" && receipt.empty());
    assert(!IsWindowEnabled(GetDlgItem(panel,collect_id)));
    SetWindowTextW(trade_input,L"9,223,372,036,854,775,807");
    assert(IsWindowEnabled(trade_buttons[0]) && !IsWindowEnabled(trade_buttons[2]));
    const auto trade_start=fixture::sent.size();
    SendMessage(panel,WM_COMMAND,trade_offer_id,0);
    assert(fixture::sent.size()==trade_start+1 && fixture::sent.back().amount==INT64_MAX);
    assert(fixture::sent.back().trade==91 && fixture::sent.back().revision==1);
    SendMessage(panel,WM_COMMAND,trade_offer_id,0);assert(fixture::sent.size()==trade_start+1);
    value.own_offer=INT64_MAX;value.request_id=fixture::sent.back().id;++value.trade_revision;reply(value);
    assert(!IsWindowEnabled(actions[0]) && !IsWindowEnabled(actions[1]));
    SendMessage(panel,WM_COMMAND,trade_lock_id,0);
    assert(fixture::sent.back().action==pn_bank::TradeLock);
    value.own_trade_state=1;value.request_id=fixture::sent.back().id;++value.trade_revision;reply(value);
    assert(!IsWindowEnabled(trade_buttons[2]) && !IsWindowEnabled(trade_input));
    const auto locked_count=fixture::sent.size();value.partner_trade_state=1;++value.trade_revision;reply(value);
    assert(IsWindowEnabled(trade_buttons[2]) && fixture::sent.size()==locked_count); // Never auto-confirm.
    trade_confirmation=[](HWND,LPCWSTR message,LPCWSTR,UINT flags)->int {
        assert(flags&MB_DEFBUTTON2);assert(std::wcsstr(message,L"9,223,372,036,854,775,807"));
        auto changed=state;++changed.trade_revision;reply(changed);return IDYES;
    };
    SendMessage(panel,WM_COMMAND,trade_commit_id,0);
    assert(fixture::sent.size()==locked_count && status.find(L"trade changed")!=std::wstring::npos);
    value=state;
    trade_confirmation=[](HWND,LPCWSTR,LPCWSTR,UINT)->int{return IDYES;};
    SendMessage(panel,WM_COMMAND,trade_commit_id,0);assert(fixture::sent.back().action==pn_bank::TradeCommit);
    value.own_trade_state=2;value.request_id=fixture::sent.back().id;++value.trade_revision;reply(value);
    assert(!IsWindowEnabled(trade_buttons[2]) && IsWindowEnabled(trade_buttons[3]));
    submit_trade(pn_bank::TradeCommit);assert(fixture::sent.size()==locked_count+1);
    value.result=pn_bank::Stale;reply(value);assert(!IsWindowEnabled(trade_buttons[2]));
    const auto cancel_count=fixture::sent.size();submit_trade(pn_bank::TradeCancel);
    assert(fixture::sent.size()==cancel_count+1 && fixture::sent.back().revision==value.trade_revision);
    // Errors carry the last accepted server request id, not our rejected id.
    reply(value);assert(!pending_trade_action && !busy && receipt.empty());
    // Check every real child rectangle at each supported scale.
    RECT bounds{};GetClientRect(panel,&bounds);std::vector<RECT> occupied;
    for(HWND child=GetWindow(panel,GW_CHILD);child;child=GetWindow(child,GW_HWNDNEXT)) {
        RECT box{};GetWindowRect(child,&box);MapWindowPoints(nullptr,panel,reinterpret_cast<POINT*>(&box),2);
        assert(box.left>=0 && box.top>=0 && box.right<=bounds.right && box.bottom<=bounds.bottom);
        for(auto other:occupied){RECT overlap{};assert(!IntersectRect(&overlap,&box,&other));}occupied.push_back(box);
    }
    save_preview("wide-zeny-ui-test.bmp");DestroyWindow(panel);
    std::cout<<"PASS: v3 exact wide wallet and trade UI, revision-bound offers, explicit two-stage confirmation, no auto-confirm, duplicate suppression, legacy rejection and layout\n";
}
