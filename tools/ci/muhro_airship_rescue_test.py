#!/usr/bin/env python3
"""Execute the five actual rescue bodies with explicit dialogue/reward doubles.

These small NPC bodies use C-compatible expressions. Translate only rAthena
register names and builtins, compile the unchanged branches/bit operations, and
exercise every rescue order. This is not a substitute for map-server script QA.
"""
from pathlib import Path
import itertools
import re
import struct
import subprocess
import tempfile
import zlib

from episode_party_progression_test import npc_body

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT/'npc/custom/instances/AirshipCrash.txt'
text = SOURCE.read_text()
cpp = '#include <cstdio>\n#include <algorithm>\nint started=0,mask=0,rewards=0,announcements=0;\n'
for i in range(1, 6):
    code = npc_body(SOURCE, f'Wounded#acr{i}')
    code = re.sub(r'mes\s+"[^";]*";', '', code)
    code = re.sub(r'instance_announce\s+[^;]+;', '++announcements;', code)
    code = code.replace('callfunc "F_AirshipCrashRescueCredit";', '++rewards;')
    code = code.replace("'started", 'started').replace("'rescue_mask", 'mask')
    code = code.replace('close;', 'return;')
    # Fail closed if another script command is added without reviewing this seam.
    assert not re.search(r'"|\b(mes|callfunc|instance_announce|close)\b', code)
    cpp += f'void rescue{i}() {{\n{code}\n}}\n'
selection = re.search(r'\.@mob = (\([^;]+\) \? \d+ : \d+);',text)
assert selection, 'Paid specimen must select its creature from the rescue state'
cpp += 'int boss() { return '+selection[1].replace("'rescue_mask",'mask')+'; }\n'
cpp += r'''
int failures=0,checks=0;
void check(bool ok,const char* label) { ++checks; if(!ok) { ++failures; std::printf("FAIL %s\n",label); } }
int main() {
    void (*rescue[])()={rescue1,rescue2,rescue3,rescue4,rescue5};
    for(auto fn:rescue) fn();
    check(mask==0 && rewards==0,"registration gate"); started=1;
    int order[]={0,1,2,3,4};
    do {
        mask=rewards=announcements=0;
        for(int i=0;i<5;++i) {
            int before=mask; rescue[order[i]]();
            check(mask!=before,"each passenger independently rescued");
            int after=mask; rescue[order[i]]();
            check(mask==after,"duplicate rescue has no effect");
            check(rewards==(i==4) && announcements==(i==4),"reward and message only once after all five");
            check(boss()==(i==4?20891:21062),"all five required for weaker paid boss");
        }
        check(mask==31,"five bits complete");
    } while(std::next_permutation(order,order+5));
    for(mask=0;mask<32;++mask) check(boss()==(mask==31?20891:21062),"every incomplete subset keeps stronger boss");
    std::printf("MUHRO_AIRSHIP_RESCUE checks=%d failures=%d\n",checks,failures);
    return failures?1:0;
}
'''
with tempfile.TemporaryDirectory(prefix='pn-airship-rescues-') as temp:
    path, executable = Path(temp)/'test.cpp', Path(temp)/'test'
    path.write_text(cpp)
    subprocess.run(['g++','-std=c++17','-fsanitize=address,undefined','-fno-sanitize-recover=all',str(path),'-o',str(executable)],check=True)
    subprocess.run([str(executable)],check=True)

cells = None
for relative in ('db/import/map_cache.dat','db/re/map_cache.dat','db/map_cache.dat'):
    path = ROOT/relative
    if not path.is_file():
        continue
    data = path.read_bytes(); position = 8
    for _ in range(struct.unpack_from('<H',data,4)[0]):
        name, width, height, size = struct.unpack_from('<12shhi',data,position)
        position += 20
        if name.split(b'\0')[0] == b'1@mjo1' and cells is None:
            cells = (width,height,zlib.decompress(data[position:position+size]))
        position += size
assert cells is not None, 'Airship crash map cache missing'
positions = re.findall(r'^1@mjo1,(\d+),(\d+),4\tscript\tWounded#acr\d',text,re.M)
assert len(positions)==5 and len(set(positions))==5
width,height,data = cells
for x,y in positions:
    x,y = int(x),int(y)
    assert 0<=x<width and 0<=y<height and data[y*width+x] in (0,3,6), (x,y)
print('MUHRO_AIRSHIP_GEOMETRY five unique walkable survivor locations')
