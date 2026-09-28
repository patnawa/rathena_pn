#!/usr/bin/env python3
"""Verify fast pacing preserves the existing encounter and timer control flow."""
from pathlib import Path
import re
import subprocess
import struct
import zlib

ROOT=Path(__file__).resolve().parents[2]
relative='npc/re/instances/SarahAndFenrir.txt'
old=subprocess.check_output(['git','show','1f4fd4cab:'+relative],cwd=ROOT).decode('utf-8')
new=(ROOT/relative).read_text(encoding='utf-8')
pattern=r"(sleep2?) \('sarah_fast \? 100 : (\d+)\);"
pauses=re.findall(pattern,new)
assert len(pauses)==61
assert all(int(delay)>=1000 for _,delay in pauses)
# Reverse only the documented additions. Every timer, spawn, reward, quest,
# movement, cooldown and original dialogue must then equal the pre-change file.
restored=re.sub(pattern,lambda m:f'{m[1]} {m[2]};',new)
start=restored.index('// Optional per-instance story pacing.')
end=restored.index('1@glast,360,295,0\tscript\t#glast_event_1',start)
restored=restored[:start]+restored[end:]
restored=restored.replace("\t'sarah_scene_started = 1;\n",'',1)
assert restored==old,'An encounter operation changed outside the pacing feature'
assert "if ('sarah_scene_started || .@mode == 3 || !is_party_leader())" in new
assert new.index("'sarah_scene_started = 1;")<new.index('disablenpc instance_npcname("#glast_event_1")')
data=(ROOT/'db/map_cache.dat').read_bytes(); pos=8
found=False
for _ in range(struct.unpack_from('<H',data,4)[0]):
    name,w,h,size=struct.unpack_from('<12shhi',data,pos); pos+=20
    if name.split(b'\0')[0]==b'1@glast':
        cells=zlib.decompress(data[pos:pos+size]); assert cells[304*w+370] in (0,3,6); found=True
    pos+=size
assert found
normal=sum(int(delay) for _,delay in pauses)
print(f'MUHRO_SARAH_FAST_MODE 61 story pauses: {normal}ms -> {len(pauses)*100}ms; encounter operations preserved')
