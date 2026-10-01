#!/usr/bin/env python3
"""Execute actual mission/rescue commit branches for both mission orders."""
from pathlib import Path
import re
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parents[2]
source=(ROOT/'npc/custom/extended_memo.txt').read_text(encoding='utf-8')
mission=source[source.index('\tif (checkquest(.@hunt,HUNTING)'):source.index('\n}\n\numbala,')]
rescue=source[source.index('\tif (!callfunc("F_PNMemoEligible") || PN_MissionStage != 2 || PN_MissionRegion != .@region || checkquest(.@hunt) < 0)'):source.index('\n}\n\nnif_dun01,')]
def translate(text):
    text=re.sub(r'mes\s+[^;]+;', '',text)
    text=re.sub(r'completequest ([^;]+);',r'completequest(\1);',text)
    text=re.sub(r'specialeffect [^;]+;|cloakonnpcself;', '',text)
    return text.replace('.@','').replace('close;', 'return;')
cpp=r'''
#include <algorithm>
#include <cstdio>
#include <map>
int PN_MissionStage,PN_MissionRegion,PN_MissionDone,PN_MissionRescue,completions;
constexpr int HUNTING=1;bool eligible=true;std::map<int,int> quests;
bool callfunc(const char*) {return eligible;}
int checkquest(int id,int mode=0) {auto it=quests.find(id);return it==quests.end()?-1:it->second;}
void completequest(int id) {quests[id]=3;++completions;}
'''
cpp+='void mission(int region) { int hunt=region==1?16592:16596;\n'+translate(mission)+'\n}\n'
cpp+='void rescue(int region,int bit,int quest) { int hunt=region==1?16592:16596;\n'+translate(rescue)+'\n}\n'
cpp+=r'''
int main() {
 int checks=0,failures=0;auto check=[&](bool ok){++checks;if(!ok)++failures;};
 for(int first=1;first<=2;++first) {
  int order[]={0,1,2};
  do {
   PN_MissionDone=0;quests.clear();
   for(int step=0;step<2;++step) {
    int region=step==0?first:3-first;int hunt=region==1?16592:16596;
    PN_MissionStage=2;PN_MissionRegion=region;PN_MissionRescue=0;quests[hunt]=0;completions=0;
    mission(region);check(PN_MissionStage==2 && completions==0);
    for(int n=0;n<3;++n) {
     int bit=1<<order[n],quest=hunt-3+order[n];
     rescue(region,bit,quest);int before=completions;rescue(region,bit,quest);
     check(completions==before); // no repeated rescue credit
     mission(region);check(PN_MissionStage==2); // hunt still incomplete
    }
    check(PN_MissionRescue==7);quests[hunt]=2;mission(region);
    check(PN_MissionRegion==0 && PN_MissionRescue==0);
    check(PN_MissionStage==(step==0?1:3));
   }
   check(PN_MissionDone==3);
  } while(std::next_permutation(order,order+3));
 }
 std::printf("MEMO_QUEST_PROGRESS checks=%d failures=%d;both mission orders and all rescue orders\n",checks,failures);return failures?1:0;
}
'''
with tempfile.TemporaryDirectory(prefix='pn-memo-quest-') as temp:
    path,exe=Path(temp)/'test.cpp',Path(temp)/'test';path.write_text(cpp)
    subprocess.run(['g++','-std=c++17','-fsanitize=address,undefined','-fno-sanitize-recover=all',str(path),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
