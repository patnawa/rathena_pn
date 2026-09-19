// Tests the shipping Windows adapter without installing hooks or sending input.
#include "turbo.cpp"
#include <cassert>
#include <vector>
std::vector<INPUT> emitted;
UINT WINAPI capture_input(UINT count,LPINPUT inputs,int size){assert(count==1&&size==sizeof(INPUT));emitted.push_back(*inputs);return count;}
int main(){
    wchar_t temp[MAX_PATH];GetTempPathW(MAX_PATH,temp);swprintf(config_path,MAX_PATH,L"%lspn-turbo-native-%lu.ini",temp,GetCurrentProcessId());
    assert(WritePrivateProfileStringW(L"General",L"DefaultState",L"0",config_path));
    WritePrivateProfileStringW(L"SmartKeys",L"Key1",L"F1",config_path);
    WritePrivateProfileStringW(L"TurboKeys",L"Key1",L"F5",config_path);
    WritePrivateProfileStringW(L"TurboKeys",L"Delay1",L"75",config_path);
    reload();assert(configured&&!core.enabled&&core.bindings[VK_F1].mode==pn_turbo::Smart&&core.bindings[VK_F5].delay==75);
    core.enable(true);Sleep(20);WritePrivateProfileStringW(L"SmartKeys",L"Delay1",L"25",config_path);reload();
    assert(core.enabled&&core.bindings[VK_F1].delay==25); // Editing delays must not reset Alt+P.
    Sleep(20);WritePrivateProfileStringW(L"TurboKeys",L"Key1",L"F1",config_path);reload();assert(!core.enabled&&notice==4);
    assert(key_code(L"f12")==VK_F12&&key_code(L" Q ")=='Q'&&key_code(L"F99")==0&&key_code(L"CTRL")==0);
    tag=0x1234;auto down=key_input(VK_F1,false),up=key_input(VK_F1,true);
    assert(down.type==INPUT_KEYBOARD&&down.ki.wScan==MapVirtualKeyW(VK_F1,MAPVK_VK_TO_VSC));
    assert(down.ki.dwFlags==KEYEVENTF_SCANCODE&&up.ki.dwFlags==(KEYEVENTF_SCANCODE|KEYEVENTF_KEYUP)&&down.ki.dwExtraInfo==tag);
    assert(mouse_input(false,false).mi.dwFlags==MOUSEEVENTF_LEFTDOWN&&mouse_input(false,true).mi.dwFlags==MOUSEEVENTF_LEFTUP);
    assert(mouse_input(true,false).mi.dwFlags==MOUSEEVENTF_RIGHTDOWN&&mouse_input(true,true).mi.dwFlags==MOUSEEVENTF_RIGHTUP);
    // Drive the actual shipping input phases into a recorder, never the desktop.
    input_sink=capture_input;
    start_action({VK_F1,pn_turbo::Smart},100);assert(emitted.size()==1&&phase==1);
    advance_action(131);assert(emitted.size()==1);
    advance_action(132);assert(emitted.size()==2&&phase==3&&emitted.back().ki.dwFlags&KEYEVENTF_KEYUP);
    advance_action(181);assert(emitted.size()==2); // Target click cannot share the key-up tick.
    advance_action(182);assert(emitted.size()==3&&phase==2&&emitted.back().mi.dwFlags==MOUSEEVENTF_LEFTDOWN);
    advance_action(213);assert(emitted.size()==3);
    advance_action(214);assert(emitted.size()==4&&phase==0&&emitted.back().mi.dwFlags==MOUSEEVENTF_LEFTUP);
    // Releasing during the target wait must never emit a click; cancellation
    // during a click releases only that click. Key repeat retains its timing.
    emitted.clear();start_action({VK_F1,pn_turbo::Smart},300);advance_action(332);cancel();advance_action(1000);assert(emitted.size()==2&&phase==0);
    emitted.clear();start_action({VK_F1,pn_turbo::Smart},1100);advance_action(1132);advance_action(1182);cancel();assert(emitted.size()==4&&emitted.back().mi.dwFlags==MOUSEEVENTF_LEFTUP&&phase==0);
    emitted.clear();start_action({VK_F5,pn_turbo::Repeat},2000);advance_action(2015);assert(emitted.size()==1);advance_action(2016);assert(emitted.size()==2&&phase==0);
    assert(!base_allowed());DeleteFileW(config_path);
    puts("PASS: shipping INI parser, hot reload, duplicate rejection, tagged input, separated skill/target/click phases, release cancellation, unchanged key-repeat timing, inactive guard; no desktop input generated");
}
