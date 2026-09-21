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
    // Check the actual captions using both native and supplied 1.10-size text.
    HDC dc=GetDC(panel);
    for(int height:{12,13}) {
        HFONT measure=CreateFontW(-px(height),0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,0,0,L"Tahoma");
        auto old=SelectObject(dc,measure);
        for(int row=1;row<3;++row) {
            set_amount(row,INT32_MAX);
            for(int action=0;action<2;++action) {
                const auto value=label(actions[row*2+action]);
                const auto split=value.find(L'\n');assert(split!=std::wstring::npos);
                for(const auto& part:{value.substr(0,split),value.substr(split+1)}) {
                    SIZE size{};GetTextExtentPoint32W(dc,part.c_str(),static_cast<int>(part.size()),&size);
                    assert(size.cx<=px(180) && size.cy<=px(16));
                }
            }
            set_amount(row,1);
        }
        SelectObject(dc,old);DeleteObject(measure);
    }
    ReleaseDC(panel,dc);
}
int main(int argc,char** argv) {
    if(argc==2) scale_percent=std::atoi(argv[1]);
    instance=GetModuleHandle(nullptr);
    WNDCLASSW type{};type.lpfnWndProc=window_proc;type.hInstance=instance;type.lpszClassName=L"PNBankUIFixture";
    assert(RegisterClassW(&type));
    panel=CreateWindowExW(WS_EX_TOOLWINDOW,type.lpszClassName,L"",WS_POPUP|WS_BORDER|WS_CLIPCHILDREN,0,0,px(panel_width)+2,px(panel_height)+2,nullptr,nullptr,instance,nullptr);
    assert(panel);
    ShowWindow(panel,SW_SHOWNOACTIVATE);
    // Emulate a game that hides the OS pointer for its software cursor.
    const int initial_cursor_count=ShowCursor(TRUE)-1;ShowCursor(FALSE);
    while(ShowCursor(FALSE)>-3) {}
    const int hidden_cursor_count=ShowCursor(TRUE)-1;ShowCursor(FALSE);
    SetCursor(nullptr);
    SendMessage(panel,WM_SETCURSOR,reinterpret_cast<WPARAM>(panel),MAKELPARAM(HTCLIENT,WM_MOUSEMOVE));
    assert(GetCursor()==LoadCursor(nullptr,IDC_ARROW));
    const int visible_cursor_count=ShowCursor(TRUE)-1;ShowCursor(FALSE);
    assert(visible_cursor_count>=0);
    for(int repeat=0;repeat<100;++repeat)
        SendMessage(panel,WM_SETCURSOR,reinterpret_cast<WPARAM>(panel),MAKELPARAM(HTCLIENT,WM_MOUSEMOVE));
    assert(ShowCursor(TRUE)-1==visible_cursor_count);ShowCursor(FALSE);
    SendMessage(inputs[0],WM_SETCURSOR,reinterpret_cast<WPARAM>(inputs[0]),MAKELPARAM(HTCLIENT,WM_MOUSEMOVE));
    assert(GetCursor()==LoadCursor(nullptr,IDC_IBEAM));
    ShowWindow(panel,SW_HIDE);
    assert(ShowCursor(TRUE)-1==hidden_cursor_count);ShowCursor(FALSE);
    for(int count=hidden_cursor_count;count<initial_cursor_count;++count) ShowCursor(TRUE);
    // These fail in the reported build: both item edits originally started at 0.
    assert(amount(0)==0 && amount(1)==1 && amount(2)==1);
    update();for(auto control:actions) assert(!IsWindowEnabled(control));
    auto value=funded();reply(value);
    check_layout();
    for(auto control:actions) assert(GetWindowLongPtr(control,GWL_STYLE)&WS_VISIBLE);
    set_amount(1,3);assert(label(actions[2])==L"Buy 3\n-1,503,000,000z" && label(actions[3])==L"Sell 3\n+1,497,000,000z");
    set_amount(2,2);assert(label(actions[4])==L"Buy 2\n-2,004,000z" && label(actions[5])==L"Sell 2\n+1,996,000z");
    set_amount(2,INT64_MAX);assert(label(actions[4])==L"Buy\nEnter quantity" && !IsWindowEnabled(actions[4]));
    set_amount(1,1);set_amount(2,1);
    for(int i=2;i<6;++i) assert(IsWindowEnabled(actions[i]));
    for(int row=1;row<3;++row) for(bool buy:{true,false}) assert(exchange_block_reason(row,buy).empty());
    for(auto control:actions)
        fixture::button_proc=reinterpret_cast<WNDPROC>(SetWindowLongPtr(control,GWLP_WNDPROC,reinterpret_cast<LONG_PTR>(counted_button)));
    diagnostics_path=L"bank-ui-test-diagnostics.txt";
    // The real timer used to disable every action and invalidate the entire
    // panel at both ends of each unchanged automatic balance refresh.
    set_amount(0,1);ValidateRect(panel,nullptr);const auto old_status=status;
    const auto before_refresh=fixture::requests.size();fixture::enable_messages=0;
    last_refresh=0;SendMessage(panel,WM_TIMER,1,0);
    assert(fixture::requests.size()==before_refresh+1 && fixture::requests.back().action==pn_bank::Refresh);
    assert(busy && refreshing && status==old_status && actions_ready());
    for(auto control:actions) assert(IsWindowEnabled(control));
    assert(!GetUpdateRect(panel,nullptr,FALSE) && fixture::enable_messages==0);
    for(int row=1;row<3;++row) for(bool buy:{true,false}) assert(exchange_block_reason(row,buy).empty());
    reply(value);
    assert(!busy && !refreshing && status==old_status);
    assert(!GetUpdateRect(panel,nullptr,FALSE) && fixture::enable_messages==0);
    std::ifstream diagnostic("bank-ui-test-diagnostics.txt");
    std::string report((std::istreambuf_iterator<char>(diagnostic)),{});diagnostic.close();
    assert(report.find("event=refresh_unchanged")!=std::string::npos);
    assert(report.find("BuyTicket.enabled=1\nBuyTicket.reason=Ok")!=std::string::npos);
    for(auto secret:{"nonce","account_id","char_id","password","bank=","wallet="}) assert(report.find(secret)==std::string::npos);
    diagnostics_path.clear();assert(DeleteFileW(L"bank-ui-test-diagnostics.txt"));
    // One click during a refresh is delivered exactly once, with the amount
    // that was clicked, even if the user edits the field before the reply.
    for(int id=400;id<406;++id) {
        reply(value);const int row=(id-400)/2;set_amount(row,1);
        const auto before=fixture::requests.size();submit(pn_bank::Refresh);
        click(id);assert(queued_action==static_cast<uint32_t>(id-399) && queued_amount==1);
        assert(fixture::requests.size()==before+1);
        for(auto control:actions) assert(!IsWindowEnabled(control));
        set_amount(row,2);click(id);SendMessage(panel,WM_COMMAND,id,0);
        assert(fixture::requests.size()==before+1 && queued_amount==1);
        reply(value);
        assert(fixture::requests.size()==before+2 && fixture::requests.back().action==static_cast<uint32_t>(id-399));
        assert(fixture::requests.back().amount==1 && busy && !refreshing && queued_action==pn_bank::Refresh);
        click(id);assert(fixture::requests.size()==before+2);
        reply(value);set_amount(row,1);
    }
    // Recheck every queued financial action against the fresh server snapshot.
    for(int id=400;id<406;++id) {
        reply(value);set_amount((id-400)/2,1);
        const auto before=fixture::requests.size();submit(pn_bank::Refresh);click(id);
        auto changed=value;
        if(id==400) changed.wallet=changed.max_deposit=0;
        else if(id==401 || id==402 || id==404) {
            changed.bank=changed.max_withdraw=0;changed.max_buy[0]=changed.max_buy[1]=0;
        } else { const int item=(id-402)/2;changed.counts[item]=changed.max_sell[item]=0; }
        reply(changed);
        assert(fixture::requests.size()==before+1 && !busy && queued_action==pn_bank::Refresh);
        assert(!IsWindowEnabled(GetDlgItem(panel,id)));
        assert(status==wide(pn_bank::message(id==403 || id==405?pn_bank::Items:pn_bank::Funds)));
    }
    reply(value);submit(pn_bank::Refresh);click(404);
    auto capacity=value;capacity.max_buy[1]=0;const auto before_capacity=fixture::requests.size();reply(capacity);
    assert(fixture::requests.size()==before_capacity && status==wide(pn_bank::message(pn_bank::Capacity)));
    for(auto result:{pn_bank::Saving,pn_bank::Unavailable,pn_bank::Unauthorized,pn_bank::Busy,pn_bank::Stale,
            pn_bank::SaveFailed,pn_bank::Invalid,pn_bank::Funds,pn_bank::Capacity,pn_bank::Limit,pn_bank::Items}) {
        reply(value);submit(pn_bank::Refresh);click(404);const auto before=fixture::requests.size();
        auto blocked=value;blocked.result=result;reply(blocked);
        assert(fixture::requests.size()==before && !busy && queued_action==pn_bank::Refresh);
        reply(value);assert(fixture::requests.size()==before); // no delayed retry
    }
    reply(value);submit(pn_bank::Refresh);click(404);
    const auto before_disconnect=fixture::requests.size();reply(value,false);
    assert(fixture::requests.size()==before_disconnect && !verified && queued_action==pn_bank::Refresh);
    reply(value);assert(fixture::requests.size()==before_disconnect);
    for(int id=402;id<=405;++id) {
        reply(value);const auto before=fixture::requests.size();click(id);
        assert(fixture::requests.size()==before+1);
        const auto request=fixture::requests.back();
        assert(request.action==static_cast<uint32_t>(id-399) && request.amount==1 && request.sequence>0);
        for(auto control:actions) assert(!IsWindowEnabled(control));
        click(id);SendMessage(panel,WM_COMMAND,id,0);
        assert(fixture::requests.size()==before+1); // no duplicate while saving
        auto pending=value;pending.result=pn_bank::Saving;reply(pending);
        click(id);SendMessage(panel,WM_COMMAND,id,0);assert(fixture::requests.size()==before+1);
        value.request_id=request.sequence;reply(value);
    }
    for(int row=1;row<3;++row) {
        for(auto invalid:{L"0",L"",L"-1",L"1.5",L"9223372036854775808"}) {
            SetWindowTextW(inputs[row],invalid);
            const auto before=fixture::requests.size();
            assert(!IsWindowEnabled(actions[row*2]) && !IsWindowEnabled(actions[row*2+1]));
            click(400+row*2);click(401+row*2);
            assert(fixture::requests.size()==before);
        }
        set_amount(row,0);click(500+row*10);assert(amount(row)==1);
        assert(IsWindowEnabled(actions[row*2]) && IsWindowEnabled(actions[row*2+1]));
    }
    // Each genuine restriction still disables the appropriate action.
    auto empty=value;empty.bank=0;empty.max_withdraw=0;
    for(int i=0;i<2;++i) empty.max_buy[i]=0;
    reply(empty);
    assert(!IsWindowEnabled(actions[2]) && !IsWindowEnabled(actions[4]));
    assert(IsWindowEnabled(actions[3]) && IsWindowEnabled(actions[5]));
    assert(exchange_block_reason(1,true).find(L"501,000,000 more Zeny")!=std::wstring::npos);
    assert(exchange_block_reason(2,true).find(L"1,002,000 more Zeny")!=std::wstring::npos);
    empty=value;
    for(int i=0;i<2;++i) empty.counts[i]=empty.max_sell[i]=0;
    reply(empty);
    assert(IsWindowEnabled(actions[2]) && IsWindowEnabled(actions[4]));
    assert(!IsWindowEnabled(actions[3]) && !IsWindowEnabled(actions[5]));
    assert(exchange_block_reason(1,false).find(L"No eligible items")!=std::wstring::npos);
    empty=value;for(int i=0;i<2;++i) empty.max_buy[i]=0;reply(empty);
    assert(!IsWindowEnabled(actions[2]) && !IsWindowEnabled(actions[4]));
    auto full=value;full.bank=INT64_MAX;full.max_deposit=0;
    for(int i=0;i<2;++i) full.max_sell[i]=0;
    reply(full);assert(!IsWindowEnabled(actions[3]) && !IsWindowEnabled(actions[5]));
    assert(exchange_block_reason(2,false).find(L"limit")!=std::wstring::npos);
    // Reported live state: 3M in bank with quantity 15 cannot buy 15 tickets.
    // Owning zero tickets does not block a purchase at an affordable quantity.
    auto quantity_case=value;quantity_case.bank=3000000;quantity_case.wallet=1000000000;
    quantity_case.max_deposit=quantity_case.wallet;quantity_case.max_withdraw=quantity_case.bank;
    for(int i=0;i<2;++i) quantity_case.counts[i]=quantity_case.max_buy[i]=quantity_case.max_sell[i]=0;
    quantity_case.max_buy[1]=2;set_amount(2,15);reply(quantity_case);
    assert(!IsWindowEnabled(actions[4]) && !IsWindowEnabled(actions[5]));
    assert(exchange_block_reason(2,true)==L"Buy: Deposit 12,030,000 more Zeny.");
    for(int quantity:{1,2}) {
        set_amount(2,quantity);assert(IsWindowEnabled(actions[4]) && !IsWindowEnabled(actions[5]));
        assert(exchange_block_reason(2,true).empty());
    }
    set_amount(2,1);
    // Reproduce the reported state: on-hand funds do not pay for bank purchases.
    auto unfunded=value;unfunded.bank=1000000;unfunded.wallet=1000000000;
    unfunded.max_deposit=unfunded.wallet;unfunded.max_withdraw=unfunded.bank;
    for(int i=0;i<2;++i) unfunded.counts[i]=unfunded.max_buy[i]=unfunded.max_sell[i]=0;
    reply(unfunded);
    for(int i=2;i<6;++i) assert(!IsWindowEnabled(actions[i]));
    assert(exchange_block_reason(1,true)==L"Buy: Deposit 500,000,000 more Zeny.");
    assert(exchange_block_reason(2,true)==L"Buy: Deposit 2,000 more Zeny.");
    set_amount(0,2000);click(400);
    assert(fixture::requests.back().action==pn_bank::Deposit && fixture::requests.back().amount==2000);
    unfunded.bank+=2000;unfunded.wallet-=2000;unfunded.max_deposit=unfunded.wallet;unfunded.max_withdraw=unfunded.bank;
    unfunded.max_buy[1]=1;reply(unfunded);
    assert(!IsWindowEnabled(actions[2]) && IsWindowEnabled(actions[4]));
    assert(exchange_block_reason(2,true).empty());
    click(404);assert(fixture::requests.back().action==pn_bank::BuyNote && fixture::requests.back().amount==1);
    unfunded.bank-=1002000;unfunded.max_withdraw=unfunded.bank;unfunded.max_buy[1]=0;
    unfunded.counts[1]=unfunded.max_sell[1]=1;reply(unfunded);
    assert(IsWindowEnabled(actions[5]) && exchange_block_reason(2,false).empty());
    click(405);assert(fixture::requests.back().action==pn_bank::SellNote && fixture::requests.back().amount==1);
    for(auto result:{pn_bank::Unavailable,pn_bank::Saving,pn_bank::Unauthorized}) {
        auto blocked=value;blocked.result=result;reply(blocked);
        const auto before=fixture::requests.size();
        for(int id=400;id<406;++id) { click(id);SendMessage(panel,WM_COMMAND,id,0); }
        assert(fixture::requests.size()==before);
    }
    reply(value);reply(value,false);
    for(auto control:actions) assert(!IsWindowEnabled(control));
    // A receipt requires the exact authenticated request, survives refresh,
    // and is never generated by a click, Saving, disconnect, or stale reply.
    reply(value);set_amount(2,1);click(404);
    const auto receipt_request=fixture::requests.back();assert(receipt.empty());
    auto confirmation=value;confirmation.request_id=receipt_request.sequence;confirmation.result=pn_bank::Saving;
    reply(confirmation);assert(receipt.empty());
    confirmation.result=pn_bank::Ok;confirmation.request_id--;reply(confirmation);assert(receipt.empty());
    reply(confirmation,false);assert(receipt.empty());
    confirmation.request_id=receipt_request.sequence;confirmation.bank-=confirmation.buy[1];
    confirmation.max_withdraw=confirmation.bank;
    for(int i=0;i<2;++i) confirmation.max_buy[i]=confirmation.bank/confirmation.buy[i];
    reply(confirmation);assert(receipt.find(L"Saved: Bought 1 Ticket.")==0);
    const auto saved_receipt=receipt;submit(pn_bank::Refresh);reply(confirmation);assert(receipt==saved_receipt);
    // Rejections can retain the server's previous request ID. They must clear
    // attribution so a later native transaction with that ID cannot claim our receipt.
    for(auto rejected:{pn_bank::Funds,pn_bank::Items,pn_bank::Capacity,pn_bank::Busy,pn_bank::Stale,pn_bank::Invalid,pn_bank::Limit,pn_bank::Unauthorized}) {
        reply(value);set_amount(2,1);click(404);const auto id=fixture::requests.back().sequence;
        auto failure=value;failure.request_id=id-1;failure.result=rejected;reply(failure);
        assert(!pending_receipt.action && receipt.empty());
        failure.result=pn_bank::Ok;failure.request_id=id;reply(failure);assert(receipt.empty());
    }
    reply(value);click(404);auto changed_nonce=value;changed_nonce.request_id=fixture::requests.back().sequence;
    changed_nonce.nonce_hi=1;reply(changed_nonce);assert(receipt.empty() && !pending_receipt.action);
    reply(value);set_amount(1,10);set_amount(2,100);
    submit(pn_bank::Refresh);click(404);assert(queued_action==pn_bank::BuyNote);
    const auto before_session=fixture::requests.size();
    fixture::authenticated=false;++fixture::generation;
    ShowWindow(panel,SW_SHOWNOACTIVATE);
    SendMessage(panel,BANK_SESSION,fixture::generation,0);
    assert(!IsWindowVisible(panel));
    SendMessage(panel,BANK_OPEN,0,0);
    assert(!IsWindowVisible(panel));
    assert(amount(0)==0 && amount(1)==1 && amount(2)==1);
    assert(receipt.empty() && !pending_receipt.action);
    reply(value,true,fixture::generation-1);
    assert(queued_action==pn_bank::Refresh && !busy && !refreshing && fixture::requests.size()==before_session);
    for(auto control:actions) assert(!IsWindowEnabled(control));
    assert(!IsWindowVisible(panel));
    DestroyWindow(panel);
    std::cout<<"PASS: compact controls stay inside the panel without overlaps; exact signed Buy/Sell totals track quantity; largest valid totals fit native and 1.10-size fonts; invalid quantity cannot display an overflowed total\n";
    std::cout<<"PASS: automatic refresh preserves enabled controls, status and paint region with zero WM_ENABLE messages; all six queued actions submit once with the clicked amount; changed funds/items/capacity, saving, failed refresh and session change cancel queued actions; local diagnostics omit identities, tokens and balances; 3M bank rejects 15 tickets and enables buying 1 or 2 with zero owned; default item quantities; all four native Buy/Sell clicks; pending/duplicate guards; zero/invalid input; presets; visible rejection reasons; deposit then buy/sell control recovery; funds, eligible items, inventory and bank capacity; unavailable, disconnected and stale sessions\n";
}
