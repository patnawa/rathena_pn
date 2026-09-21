// Repeat transfers through native focus/edit/click messages, without player data.
#define PN_BANK_UI_TEST
#include "../../client-patch/account_bank/bank_ui.cpp"
#include <iostream>
struct Submitted { uint32_t action;int64_t amount;uint64_t id; };
static std::vector<Submitted> submitted;
void bank_install_transport(HWND) {}
bool bank_authenticated() { return true; }
bool bank_current_generation(LONG generation,bool) { return generation==1; }
bool bank_connection_ready() { return true; }
HWND bank_find_game_window() { return nullptr; }
bool bank_submit(HWND,const pn_bank::Reply&,uint32_t action,int64_t amount,uint64_t id) {
    submitted.push_back({action,amount,id});return true;
}
static void receive(pn_bank::Reply reply) {
    reply.max_deposit=std::min(reply.wallet,pn_bank::bank_limit-reply.bank);
    reply.max_withdraw=std::min(reply.bank,pn_bank::wallet_limit-reply.wallet);
    for(int i=0;i<2;++i) { reply.max_buy[i]=reply.bank/reply.buy[i];reply.max_sell[i]=reply.counts[i]; }
    assert(pn_bank::valid_reply(reply));
    SendMessage(panel,BANK_RESULT,0,reinterpret_cast<LPARAM>(new BankResult{reply,1,true}));
}
static std::wstring input_text() { wchar_t text[64]{};GetWindowTextW(inputs[0],text,64);return text; }
static void type(const wchar_t* text,bool replace=true) {
    SetFocus(inputs[0]);assert(GetFocus()==inputs[0]);
    SendMessage(inputs[0],EM_SETSEL,replace?0:GetWindowTextLengthW(inputs[0]),-1);
    for(auto p=text;*p;++p) SendMessageW(inputs[0],WM_CHAR,*p,1);
}
static void transfer(uint32_t action,int64_t expected) {
    const auto planned=pn_bank::plan(state,action,expected);assert(planned.result==pn_bank::Ok);
    SetFocus(actions[action-1]);
    const auto count=submitted.size();SendMessage(actions[action-1],BM_CLICK,0,0);
    assert(submitted.size()==count+1 && submitted.back().action==action && submitted.back().amount==expected);
    const auto id=submitted.back().id;
    auto reply=state;reply.request_id=id;reply.result=pn_bank::Saving;receive(reply);
    assert(receipt.empty());
    reply.bank=planned.bank;reply.wallet=planned.wallet;reply.result=pn_bank::Ok;receive(reply);
    assert(!receipt.empty() && !busy && verified);
}
int main() {
    instance=GetModuleHandle(nullptr);
    WNDCLASSW cls{};cls.lpfnWndProc=window_proc;cls.hInstance=instance;cls.lpszClassName=L"PNBankReentryFixture";
    assert(RegisterClassW(&cls));
    panel=CreateWindowExW(WS_EX_TOOLWINDOW,cls.lpszClassName,L"",WS_POPUP|WS_BORDER,0,0,px(panel_width)+2,px(panel_height)+2,nullptr,nullptr,instance,nullptr);
    assert(panel);ShowWindow(panel,SW_SHOWNOACTIVATE);
    pn_bank::Reply initial;initial.result=pn_bank::Ok;initial.bank=2000000000;initial.wallet=50000000;
    initial.nonce_hi=1;initial.nonce_lo=2;receive(initial);
    type(L"1000000");transfer(pn_bank::Deposit,1000000);
    assert(input_text()==L"1,000,000");
    type(L"0",false);
    // v2.4 left grouping separators in the active edit: 1,000,0000 was invalid.
    assert(amount(0)==10000000 && IsWindowEnabled(actions[0]));
    transfer(pn_bank::Deposit,10000000);
    type(L"\b",false);assert(amount(0)==1000000 && IsWindowEnabled(actions[1]));
    transfer(pn_bank::Withdraw,1000000);
    type(L"2000000");transfer(pn_bank::Withdraw,2000000);
    type(L"0",false);assert(amount(0)==20000000);transfer(pn_bank::Deposit,20000000);
    assert(submitted.size()==5);
    SetFocus(inputs[0]);SetWindowTextW(inputs[0],L"2,000,000");
    type(L"0",false);assert(amount(0)==20000000);
    SetWindowTextW(inputs[0],L"1,,000");assert(amount(0)==-1 && !IsWindowEnabled(actions[0]));
    type(L"1000000");assert(amount(0)==1000000 && IsWindowEnabled(actions[0]));
    SendMessage(inputs[0],WM_CHAR,1,1);
    for(auto c:std::wstring(L"2000000")) SendMessageW(inputs[0],WM_CHAR,c,1);
    assert(amount(0)==2000000); // Native Ctrl+A replacement, not appending.
    type(L"1,000,000");assert(amount(0)==1000000);
    type(L"0",false);assert(amount(0)==10000000);
    DestroyWindow(panel);
    std::cout<<"PASS: repeated deposit/withdraw, refocus, append, backspace, replacement and grouped input; exact requests, no duplicate transfers, receipts only after confirmation\n";
}
