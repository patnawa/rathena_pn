#!/usr/bin/env python3
"""Audit every Renewal @go destination and compile its actual name resolver.

Run from WSL/Linux with Python 3 and g++. No game services are started.
Map-cache checks prove cell validity, not client rendering or quest access.
"""
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools'))
from inspect_mapcache import load_maps


def renewal(text):
    return re.sub(r'#ifdef RENEWAL\n(.*?)#else\n.*?#endif', r'\1', text, flags=re.S)


def main():
    source = (ROOT/'src/map/atcommand.cpp').read_text()
    body = source.split('ACMD_FUNC(go)\n{', 1)[1].split('\n/*====', 1)[0]
    macros = dict(re.findall(r'#define\s+(MAP_\w+)\s+"([^"]+)"', renewal((ROOT/'src/common/mapindex.hpp').read_text())))
    data = body.split('\n\tnullpo_retr', 1)[0]
    rows = re.findall(r'\{\s*(MAP_\w+|"[^"]+"),\s*(\d+),\s*(\d+)\s*\}', renewal(data))
    routes = [(macros.get(m, m.strip('"')), int(x), int(y)) for m,x,y in rows]
    assert len(routes) == 61, f'Review new destination count: {len(routes)}'
    help_text = (ROOT/'conf/atcommands.yml').read_text().split('  - Command: go\n',1)[1].split('  - Command:',1)[0]
    assert set(map(int,re.findall(r'(?<![\w-])(\d+):',help_text))) == set(range(61))
    assert not re.search(r'-\d+:',help_text), 'Unsupported negative destination advertised'
    maps = {}
    wanted = {m for m,_,_ in routes} | {'izlude','izlude_a','izlude_b','izlude_c','izlude_d'}
    for path in ('db/import/map_cache.dat','db/re/map_cache.dat','db/map_cache.dat'):
        file = ROOT/path
        if file.exists():
            for name, entry in load_maps(file, wanted).items():
                maps.setdefault(name, entry)
    failures = []
    for index, (name,x,y) in enumerate(routes):
        if name not in maps:
            failures.append(f'@go {index}: map missing: {name}')
            continue
        width,height,cells = maps[name]
        valid = any(c in (0,3) for c in cells) if (x,y)==(0,0) else (0<=x<width and 0<=y<height and cells[y*width+x] in (0,3))
        print(f'@go {index}: {name} {x},{y}: {"PASS" if valid else "BLOCKED"}')
        if not valid:
            candidates = [(abs(xx-x)+abs(yy-y),xx,yy) for yy in range(height) for xx in range(width) if cells[yy*width+xx] in (0,3)]
            failures.append(f'@go {index} blocked; nearest {min(candidates)[1:]}')
    for name in sorted(wanted & {'izlude','izlude_a','izlude_b','izlude_c','izlude_d'}):
        width,height,cells = maps[name]
        for file,kind,x,y in [('healer.txt','Healer',121,150),('warper.txt','Warper',134,150)]:
            text=(ROOT/'npc/custom'/file).read_text()
            assert re.search(rf'^{name},{x},{y},\d+\s+duplicate\({kind}\)',text,re.M), (name,kind)
            assert cells[y*width+x] in (0,3), (name,kind,'blocked')
    # Compile the actual declarations, input parsing and alias branches from go.
    resolver = body[body.index('\tmemset(map_name'):body.index('\n\tif (town >= 0 && town < ARRAYLENGTH(data))')]
    cpp = '#include <cstring>\n#include <cstdlib>\n#include <cstdio>\n#include <cctype>\n#include <cassert>\n#include <iostream>\nusing int32=int;\n#define RENEWAL\n#define MAP_NAME_LENGTH 12\n#define ARRAYLENGTH(a) (sizeof(a)/sizeof((a)[0]))\n#define TOLOWER(c) std::tolower(c)\n'
    cpp += '\n'.join(f'#define {k} "{v}"' for k,v in macros.items())
    cpp += '\nconst char* atcommand_help_string(const char*){return nullptr;}\nconst char* msg_txt(void*,int){return "";}\nvoid clif_displaymessage(int,const char*){}\nint resolve(const char* message){\nint fd=0; void* sd=nullptr; const char* command="go"; char atcmd_output[128];\n'
    cpp += data + '\n' + resolver + '\nreturn town;\n}\nint main(){\n'
    for index,(name,_,_) in enumerate(routes):
        cpp += f'assert(resolve("{index}")=={index}); assert(resolve("{name}")=={index});\n'
    for value in ('-1','61','999999999999999999999999','garbage','0garbage',''):
        cpp += f'assert(resolve("{value}")==-1);\n'
    for name,index in [('isgardairship',53),('isgard',37),('fishingalberta',44),('constellation',55),('mall',60),('jor_tail',53)]:
        cpp += f'assert(resolve("{name}")=={index});\n'
    cpp += 'std::cout << "PASS: 134 native resolver assertions"; }\n'
    with tempfile.TemporaryDirectory(prefix='go-routes-') as directory:
        path=Path(directory)
        (path/'test.cpp').write_text(cpp)
        subprocess.run(['g++','-std=c++17','-O2',str(path/'test.cpp'),'-o',str(path/'test')],check=True)
        subprocess.run([str(path/'test')],check=True)
    if failures:
        raise AssertionError('\n'.join(failures))
    print('\nPASS: all 61 destinations and 10 Izlude service placements')


if __name__ == '__main__':
    main()
