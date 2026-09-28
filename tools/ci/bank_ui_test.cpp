// Exercise real Windows edit/button controls and the production panel handlers.
// Only the authenticated transport is replaced; no player data is touched.
#define PN_BANK_UI_TEST
#include "../../client-patch/account_bank/bank_ui.cpp"
#include <iostream>
#include <fstream>

namespace fixture {
struct Request { uint32_t action; int64_t amount; uint64_t sequence; };
std::vector<Request> requests;
bool authenticated=true, connected=true;
LONG generation=7;
int enable_messages=0;
WNDPROC button_proc=nullptr;
}
static LRESULT CALLBACK counted_button(HWND window,UINT message,WPARAM w,LPARAM l) {
    if(message==WM_ENABLE) ++fixture::enable_messages;
    return CallWindowProc(fixture::button_proc,window,message,w,l);
}
void bank_install_transport(HWND) {}
bool bank_authenticated() { return fixture::authenticated; }
bool bank_current_generation(LONG value,bool active) {
    return value==fixture::generation && (!active || fixture::authenticated);
}
bool bank_connection_ready() { return fixture::connected; }
HWND bank_find_game_window() { return nullptr; }
bool bank_submit(HWND,const pn_bank::Reply&,uint32_t action,int64_t value,uint64_t id) {
    if(!fixture::authenticated || !fixture::connected) return false;
    fixture::requests.push_back({action,value,id});return true;
}
static void reply(pn_bank::Reply value,bool connected=true,LONG generation=fixture::generation) {
    assert(pn_bank::valid_reply(value));
    SendMessage(panel,BANK_RESULT,0,reinterpret_cast<LPARAM>(new BankResult{value,generation,connected}));
}
static pn_bank::Reply funded() {
    pn_bank::Reply value;value.result=pn_bank::Ok;value.bank=2000000000;value.wallet=1000000;
    value.max_deposit=value.wallet;value.max_withdraw=value.bank;
    for(int i=0;i<2;++i) {
        value.counts[i]=10;value.max_sell[i]=10;value.max_buy[i]=value.bank/value.buy[i];
    }
    return value;
}
static void click(int id) { SendMessage(GetDlgItem(panel,id),BM_CLICK,0,0); }
static std::wstring label(HWND control) {
    wchar_t value[128]{};GetWindowTextW(control,value,128);return value;
}
static void check_layout() {
    RECT bounds{};GetClientRect(panel,&bounds);
    assert(bounds.right==px(panel_width) && bounds.bottom==px(panel_height));
    std::vector<RECT> occupied;
    for(HWND child=GetWindow(panel,GW_CHILD);child;child=GetWindow(child,GW_HWNDNEXT)) {
        RECT box{};GetWindowRect(child,&box);MapWindowPoints(nullptr,panel,reinterpret_cast<POINT*>(&box),2);
        assert(box.left>=0 && box.top>=0 && box.right<=bounds.right && box.bottom<=bounds.bottom);
        for(const auto& other:occupied) { RECT overlap{};assert(!IntersectRect(&overlap,&box,&other)); }
        occupied.push_back(box);
    }
}
int main(int argc,char** argv) {
    if(argc==2) scale_percent=std::atoi(argv[1]);
    instance=GetModuleHandle(nullptr);
    WNDCLASSW type{};type.lpfnWndProc=window_proc;type.hInstance=instance;type.lpszClassName=L"PNBankUIFixture";
    assert(RegisterClassW(&type));
    panel=CreateWindowExW(WS_EX_TOOLWINDOW,type.lpszClassName,L"",WS_POPUP|WS_BORDER|WS_CLIPCHILDREN,0,0,px(panel_width)+2,px(panel_height)+2,nullptr,nullptr,instance,nullptr);
    assert(panel);ShowWindow(panel,SW_SHOWNOACTIVATE);
    assert(amount(0)==0);update();for(auto control:actions) assert(!IsWindowEnabled(control));
    for(int id:{101,102,301,302,402,403,404,405,510,520}) assert(!GetDlgItem(panel,id));
    auto value=funded();reply(value);check_layout();
    set_amount(0,1);for(auto control:actions) assert(IsWindowEnabled(control));
    const auto obsolete=fixture::requests.size();
    for(int id=402;id<=405;++id) SendMessage(panel,WM_COMMAND,id,0);
    for(uint32_t action=pn_bank::BuyDiamond;action<=pn_bank::SellNote;++action) submit(action,1);
    assert(fixture::requests.size()==obsolete);
    for(auto control:actions) fixture::button_proc=reinterpret_cast<WNDPROC>(SetWindowLongPtr(control,GWLP_WNDPROC,reinterpret_cast<LONG_PTR>(counted_button)));
    ValidateRect(panel,nullptr);fixture::enable_messages=0;const auto old_status=status;
    submit(pn_bank::Refresh);assert(busy && refreshing && actions_ready());
    for(auto control:actions) assert(IsWindowEnabled(control));
    assert(!GetUpdateRect(panel,nullptr,FALSE) && fixture::enable_messages==0);
    reply(value);assert(!busy && status==old_status && fixture::enable_messages==0 && !GetUpdateRect(panel,nullptr,FALSE));
    for(uint32_t action:{pn_bank::Deposit,pn_bank::Withdraw}) {
        reply(value);set_amount(0,100);submit(pn_bank::Refresh);
        click(399+action);assert(queued_action==action && queued_amount==100);
        const auto before=fixture::requests.size();click(399+action);set_amount(0,200);
        reply(value);assert(fixture::requests.size()==before+1);
        assert(fixture::requests.back().action==action && fixture::requests.back().amount==100);
        const auto id=fixture::requests.back().sequence;
        click(399+action);assert(fixture::requests.size()==before+1 && receipt.empty());
        auto confirmation=value;confirmation.request_id=id;confirmation.result=pn_bank::Saving;
        reply(confirmation);assert(receipt.empty());for(auto control:actions) assert(!IsWindowEnabled(control));
        auto plan=pn_bank::plan(value,action,100);confirmation.bank=plan.bank;confirmation.wallet=plan.wallet;confirmation.result=pn_bank::Ok;
        confirmation.max_deposit=confirmation.wallet;confirmation.max_withdraw=std::min(confirmation.bank,pn_bank::wallet_limit-confirmation.wallet);
        for(int i=0;i<2;++i) confirmation.max_buy[i]=confirmation.bank/confirmation.buy[i];
        reply(confirmation);assert(receipt.find(action==pn_bank::Deposit?L"Saved: Deposited 100z.":L"Saved: Withdrew 100z.")==0);
        const auto saved=receipt;submit(pn_bank::Refresh);reply(confirmation);assert(receipt==saved);
    }
    reply(value);
    for(const auto text:{L"0",L"-1",L"1.2",L"1,,000",L"9223372036854775808"}) {
        SetWindowTextW(inputs[0],text);for(auto control:actions) assert(!IsWindowEnabled(control));
    }
    set_amount(0,INT64_MAX);click(500);assert(amount(0)==pn_bank::wallet_limit);
    auto full=value;full.bank=INT64_MAX;full.wallet=INT32_MAX;full.max_deposit=full.max_withdraw=0;
    for(int i=0;i<2;++i) full.max_sell[i]=0;
    reply(full);set_amount(0,1);for(auto control:actions) assert(!IsWindowEnabled(control));
    full.bank=INT64_MAX-1;full.wallet=1;full.max_deposit=1;full.max_withdraw=INT32_MAX-1;
    reply(full);assert(IsWindowEnabled(actions[0]));set_amount(0,2);assert(!IsWindowEnabled(actions[0]));
    auto empty=value;empty.bank=0;empty.wallet=INT32_MAX;empty.max_withdraw=0;empty.max_deposit=INT32_MAX;
    for(int i=0;i<2;++i) empty.max_buy[i]=0;
    reply(empty);set_amount(0,INT32_MAX);assert(IsWindowEnabled(actions[0]) && !IsWindowEnabled(actions[1]));
    for(auto rejected:{pn_bank::Funds,pn_bank::Busy,pn_bank::Stale,pn_bank::Invalid,pn_bank::Limit,pn_bank::Unauthorized}) {
        reply(value);set_amount(0,1);click(400);const auto id=fixture::requests.back().sequence;
        auto failure=value;failure.request_id=id-1;failure.result=rejected;reply(failure);
        assert(!pending_receipt.action && receipt.empty());failure.request_id=id;failure.result=pn_bank::Ok;reply(failure);assert(receipt.empty());
    }
    for(auto result:{pn_bank::Unavailable,pn_bank::Saving,pn_bank::Unauthorized}) {
        auto blocked=value;blocked.result=result;reply(blocked);const auto before=fixture::requests.size();
        for(int id:{400,401}) { click(id);SendMessage(panel,WM_COMMAND,id,0); }
        assert(fixture::requests.size()==before);
    }
    reply(value);set_amount(0,1);submit(pn_bank::Refresh);click(400);
    auto depleted=value;depleted.wallet=0;depleted.max_deposit=0;
    const auto before=fixture::requests.size();reply(depleted);assert(fixture::requests.size()==before && queued_action==pn_bank::Refresh);
    reply(value);reply(value,false);for(auto control:actions) assert(!IsWindowEnabled(control));
    reply(value);set_amount(0,1);submit(pn_bank::Refresh);click(401);assert(queued_action==pn_bank::Withdraw);
    fixture::authenticated=false;++fixture::generation;SendMessage(panel,BANK_SESSION,fixture::generation,0);
    assert(!IsWindowVisible(panel) && amount(0)==0 && receipt.empty() && !pending_receipt.action);
    reply(value,true,fixture::generation-1);assert(!verified && queued_action==pn_bank::Refresh);
    for(auto control:actions) assert(!IsWindowEnabled(control));
    DestroyWindow(panel);
    std::cout<<"PASS: Zeny-only controls and layout, obsolete actions blocked, unchanged refresh, exact queued transfers, duplicate/save/receipt guards, maximum balances, invalid input, depleted funds, disconnect and stale session\n";
}
