#!/usr/bin/env python3
"""Execute actual certificate commit guard with mutation/cancellation doubles."""
from pathlib import Path
import os
import re
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parents[2]
source=(ROOT/'npc/custom/refine15_certificates.txt').read_text()
commit=source[source.index('\tif (select("Use certificate:Cancel")'):source.rindex('\n}')]
commit=re.sub(r'mes\s+[^;]+;','',commit)
commit=re.sub(r'delitem ([^,;]+),([^;]+);',r'delitem(\1,\2);',commit)
commit=re.sub(r'successrefitem ([^,;]+),([^;]+);',r'successrefitem(\1,\2);',commit)
for key,name in [('unique$','expected_unique'),('slot','slot'),('id','expected_id'),('refine','expected_refine'),('ticket','ticket')]:
    commit=commit.replace('.@'+key,name)
cpp=r'''
#include <string>
#include <cstdio>
int actual_id=100,actual_refine=9,certificates=1,choice=1,consumed=0,refined=0;
bool equipped=true,refinable=true;
std::string actual_unique="123456";
int select(const char*) { return choice; }
bool getequipisequiped(int) { return equipped; }
int getequipid(int) { return actual_id; }
std::string getequipuniqueid(int) { return actual_unique; }
int getequiprefinerycnt(int) { return actual_refine; }
bool getequipisenableref(int) { return refinable; }
int countitem(int) { return certificates; }
void delitem(int,int amount) { certificates-=amount;consumed+=amount; }
void successrefitem(int,int amount) { actual_refine+=amount;refined+=amount; }
'''
cpp+='void run() { int slot=1,expected_id=100,expected_refine=9,ticket=6872; std::string expected_unique="123456";\n'+commit+'\n}\n'
cpp+=r'''
int main() {
    int checks=0,failures=0;
    for(int scenario=0;scenario<8;++scenario) {
        actual_id=100;actual_refine=9;certificates=1;choice=1;consumed=refined=0;
        equipped=refinable=true;actual_unique="123456";
        if(scenario==1) actual_id=101;
        if(scenario==2) actual_unique="123457";
        if(scenario==3) actual_refine=10;
        if(scenario==4) certificates=0;
        if(scenario==5) refinable=false;
        if(scenario==6) equipped=false;
        if(scenario==7) choice=2;
        run();++checks;
        bool ok=scenario==0 ? (consumed==1 && refined==6 && actual_refine==15 && certificates==0) : (consumed==0 && refined==0);
        if(!ok) {++failures;std::printf("FAIL certificate scenario %d\n",scenario);}
    }
    std::printf("REFINE15_CERTIFICATE checks=%d failures=%d\n",checks,failures);return failures?1:0;
}
'''
with tempfile.TemporaryDirectory(prefix='pn-refine15-') as temp:
    path,exe=Path(temp)/'test.cpp',Path(temp)/'test';path.write_text(cpp)
    subprocess.run(['g++','-std=c++17','-fsanitize='+os.environ.get('REFINE_SANITIZERS','address,undefined'),'-fno-sanitize-recover=all',str(path),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
assert '(.@type == IT_WEAPON) ? 6872 : 6878' in source
assert 'callfunc "F_PNRefine15Ticket"' in (ROOT/'npc/custom/grademk_services.txt').read_text()
assert 'npc: npc/custom/refine15_certificates.txt' in (ROOT/'npc/scripts_custom.conf').read_text()
assert not re.search(r'next;|sleep|select\(',source[source.index('\tdelitem .@ticket,1;'):source.index('\tsuccessrefitem')])
print('REFINE15_CERTIFICATE weapon/armor IDs and Master Refiner service linked')
