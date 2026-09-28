#include "../../client-patch/wide_mail/mail_ui.cpp"
#include <cassert>
#include <vector>
#include <iostream>
namespace fixture {std::vector<pn_mail::Request> requests;}
bool bank_session_snapshot(BankSession& session){session.account=100;session.generation=7;return true;}
bool bank_current_generation(LONG generation,bool){return generation==7;}
void mail_forget_session(){}
bool mail_submit(HWND,pn_mail::Request request){fixture::requests.push_back(request);return true;}
void receive(pn_mail::Reply state,bool connected=true) {
    assert(valid_mail_reply(state));SendMessage(mail_ui::window,MAIL_RESULT,0,reinterpret_cast<LPARAM>(new MailResult{state,7,connected}));
}
int sends(){return std::count_if(fixture::requests.begin(),fixture::requests.end(),[](const auto& r){return r.action==pn_mail::Send;});}
void preview(HWND window) {
    RECT size{};GetClientRect(window,&size);HDC screen=GetDC(window),memory=CreateCompatibleDC(screen);
    BITMAPINFO info{};info.bmiHeader.biSize=sizeof(info.bmiHeader);info.bmiHeader.biWidth=size.right;info.bmiHeader.biHeight=-size.bottom;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;
    void* pixels=nullptr;HBITMAP bitmap=CreateDIBSection(screen,&info,DIB_RGB_COLORS,&pixels,nullptr,0);auto old=SelectObject(memory,bitmap);PrintWindow(window,memory,PW_CLIENTONLY);
    BITMAPFILEHEADER header{};header.bfType=0x4d42;header.bfOffBits=sizeof(header)+sizeof(info.bmiHeader);header.bfSize=header.bfOffBits+size.right*size.bottom*4;
    FILE* output=fopen("wide-mail-preview.bmp","wb");assert(output);fwrite(&header,sizeof(header),1,output);fwrite(&info.bmiHeader,sizeof(info.bmiHeader),1,output);fwrite(pixels,size.right*size.bottom*4,1,output);fclose(output);
    SelectObject(memory,old);DeleteObject(bitmap);DeleteDC(memory);ReleaseDC(window,screen);
}
int main() {
    using namespace mail_ui;
    pn_bank::Reply bank;bank.nonce_hi=11;bank.nonce_lo=22;mail_open(nullptr,bank);
    assert(fixture::requests.back().action==pn_mail::Status);
    pn_mail::Reply reply;reply.result=pn_mail::Ok;reply.nonce_hi=11;reply.nonce_lo=22;reply.wallet=INT64_MAX;receive(reply);
    SetWindowTextW(recipient,L"Fixture Recipient");SetWindowTextW(title,L"Zeny transfer");SetWindowTextW(body,L"Full-width mail fixture.");SetWindowTextW(amount,L"9,007,199,254,740,993");
    SendMessage(window,WM_COMMAND,quote_id,0);assert(fixture::requests.back().amount==9007199254740993LL);
    reply.amount=9007199254740993LL;reply.fee=90071992547409LL;reply.total=reply.amount+reply.fee;receive(reply);
    assert(IsWindowEnabled(GetDlgItem(window,send_id)));
    confirm=[](HWND,LPCWSTR message,LPCWSTR,UINT flags)->int{
        assert(flags&MB_DEFBUTTON2);assert(std::wcsstr(message,L"9,097,271,247,288,402"));SetWindowTextW(body,L"Changed during confirmation");return IDYES;
    };
    SendMessage(window,WM_COMMAND,send_id,0);assert(sends()==0&&!quoted);
    quote();receive(reply);confirm=[](HWND,LPCWSTR,LPCWSTR,UINT)->int{return IDYES;};
    SendMessage(window,WM_COMMAND,send_id,0);assert(sends()==1&&pending_send);
    const auto send_id_value=pending_send;const auto request=fixture::requests.back();
    assert(request.expected_total==9097271247288402LL&&request.amount==9007199254740993LL);
    send_mail();assert(sends()==1);
    reply.result=pn_mail::Pending;reply.pending=1;reply.send_sequence=send_id_value;receive(reply);check_status();
    receive(reply,false);assert(pending_send==send_id_value&&sends()==1&&!verified);
    check_status();assert(fixture::requests.back().action==pn_mail::Status);
    reply.result=pn_mail::Ok;reply.pending=0;receive(reply);
    assert(!pending_send&&text(status_label).find(L"Saved:")==0&&text(amount)==L"0"&&sends()==1);
    // A refused SEND is not left stuck waiting for a receipt that will never exist.
    SetWindowTextW(amount,L"1");quote();reply.amount=reply.total=1;reply.fee=0;receive(reply);send_mail();
    reply.result=pn_mail::Funds;receive(reply);assert(!pending_send&&sends()==2);
    auto malformed=reply;malformed.result=pn_mail::Ok;malformed.amount=INT64_MAX;malformed.fee=1;assert(!valid_mail_reply(malformed));
    malformed=reply;malformed.protocol=2;assert(!valid_mail_reply(malformed));
    reply.result=pn_mail::Ok;reply.amount=9007199254740993LL;reply.fee=90071992547409LL;reply.total=reply.amount+reply.fee;
    SetWindowTextW(amount,L"9,007,199,254,740,993");quote();receive(reply);preview(window);
    DestroyWindow(window);std::cout<<"PASS: exact mail quote/fee/total, form-change rejection, no automatic resend, pending/disconnect status recovery, matching durable receipt and malformed reply rejection\n";
}
