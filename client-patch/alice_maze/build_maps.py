# ============================================================================
#  PN  /  CLIENT TOOLING
#  build_maps.py
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/client-patch/alice_maze/build_maps.py
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

"""Build original deterministic maze terrain; no installed client or downloads required.

The GND grid uses one cube for each 2x2 GAT cells. Walls rise 18 game units
(negative height in the file coordinate system), with matching collision.
"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import struct
import zlib

TEXTURES = ("gol/gol_bot01.bmp", "gol/gol_wall01.bmp")
HARD_GUIDES = [(154,543),(55,550),(61,438),(13,329),(11,201),(11,89),(100,57),(127,81),(39,115),(284,14),(287,198),(231,209),(277,579),(285,333)]
CONTRACT = {
    "1@alice_mad": {"size": [400,400], "zones": [
        {"name":"first_maze", "bounds":[100,100,300,280], "maze":True, "points":[[110,110],[113,151],[208,114],[220,114],[289,246],[289,256],[141,222],[141,218],[110,108]]},
        *[{"name":"corridor_"+str(i+1),"bounds":[x-2,18,x+3,82],"points":[[x,20],[x,80]]} for i,x in enumerate([12,24,36,48,60,72])],
        {"name":"detour", "bounds":[10,110,90,190], "maze":True,"points":[[14,114],[86,186]]},
        {"name":"clock", "bounds":[330,150,390,250],"points":[[360,160],[340,240],[380,240],[360,200]]},
        {"name":"second_maze", "bounds":[300,10,390,120],"maze":True,"points":[[310,20],[325,76]]},
        {"name":"chess", "bounds":[10,300,90,398],"points":[[50,310],[58,392]]},
        {"name":"boss", "bounds":[320,300,390,390],"points":[[350,310],[350,320],[350,360]]},
    ]},
    "2@alice_mad": {"size":[300,600], "zones":[{"name":"hard_maze","bounds":[6,6,294,594],"maze":True,"points":[[150,300],*[list(p) for p in HARD_GUIDES]]}]},
}


def carve(grid, x, y, radius=0):
    for yy in range(y-radius,y+radius+1):
        for xx in range(x-radius,x+radius+1):
            assert 0 < xx < len(grid[0])-1 and 0 < yy < len(grid)-1
            grid[yy][xx] = 1


def corridor(grid, a, b, width=2):
    x,y=a
    while (x,y)!=b:
        for dy in range(width):
            for dx in range(width): carve(grid,x+dx,y+dy)
        if x!=b[0]: x+=1 if b[0]>x else -1
        else: y+=1 if b[1]>y else -1
    for dy in range(width):
        for dx in range(width): carve(grid,x+dx,y+dy)


def terrain(name, spec):
    width,height=spec['size'];grid=[[0]*(width//2) for _ in range(height//2)]
    rng=random.Random(20260909 + (name.startswith('2@')))
    for zone in spec['zones']:
        x0,y0,x1,y1=(n//2 for n in zone['bounds'])
        x1=min(x1,width//2-2);y1=min(y1,height//2-2)
        if zone.get('maze'):
            nodes={(x,y) for y in range(y0+2,y1-2,4) for x in range(x0+2,x1-2,4)}
            start=min(nodes);visited={start};stack=[start]
            while stack:
                p=stack[-1];neighbors=[(p[0]+dx,p[1]+dy) for dx,dy in [(4,0),(-4,0),(0,4),(0,-4)] if (p[0]+dx,p[1]+dy) in nodes-visited]
                if neighbors:
                    q=rng.choice(neighbors);corridor(grid,p,q);visited.add(q);stack.append(q)
                else: stack.pop()
            assert visited==nodes
            # Small clearings protect all NPC/arrival coordinates. Connect each
            # clearing to a maze node without joining another scripted zone.
            for gx,gy in zone['points']:
                p=(gx//2,gy//2);q=min(nodes,key=lambda n:(abs(n[0]-p[0])+abs(n[1]-p[1]),n[0],n[1]))
                corridor(grid,p,q);carve(grid,*p,radius=1)
        else:
            for y in range(y0,y1+1):
                for x in range(x0,x1+1): carve(grid,x,y)
    return grid


def gat(grid):
    h,w=len(grid)*2,len(grid[0])*2
    raw=bytearray(b'GRAT\x01\x02'+struct.pack('<II',w,h))
    for y in range(h):
        for x in range(w):
            floor=grid[y//2][x//2];height=0.0 if floor else -18.0
            raw+=struct.pack('<ffffI',height,height,height,height,0 if floor else 1)
    return bytes(raw)


def gnd(grid):
    h,w=len(grid),len(grid[0]);raw=bytearray(b'GRGN\x01\x07'+struct.pack('<IIfII',w,h,10.0,2,80))
    for texture in TEXTURES:raw+=texture.replace('/', '\\').encode().ljust(80,b'\0')
    raw+=struct.pack('<IIII',1,8,8,1)+bytes([255])*256
    raw+=struct.pack('<I',2)
    for texture in range(2):
        raw+=struct.pack('<8fHH4B',0,1,0,1,0,0,1,1,texture,0,255,255,255,255)
    for y in range(h):
        for x in range(w):
            floor=grid[y][x];height=0.0 if floor else -18.0
            raw+=struct.pack('<4f3i',height,height,height,height,0 if floor else 1,1,1)
    return bytes(raw)


def rsw(name):
    # RSW2.0 predates the quadtree. No borrowed model geometry or alias needed.
    raw=bytearray(b'GRSW\x02\x00')
    for value in ['',name+'.gnd',name+'.gat','']:raw+=value.encode().ljust(40,b'\0')
    raw+=struct.pack('<fifffi',100.0,0,0.0,0.0,0.0,1) # water below raised terrain
    raw+=struct.pack('<ii7f',45,45,.8,.8,.8,.5,.5,.5,1.0)
    raw+=struct.pack('<4iI',0,0,0,0,0) # ground bounds; zero objects
    return bytes(raw)


def grf(entries):
    body=bytearray();table=bytearray()
    for name,raw in sorted(entries.items()):
        packed=zlib.compress(raw,9);table+=name.encode('cp949')+b'\0'+struct.pack('<IIIBI',len(packed),len(packed),len(raw),1,len(body));body+=packed
    packed=zlib.compress(table,9)
    return b'Master of Magic\0'+bytes(14)+struct.pack('<IIII',len(body),0,len(entries)+7,0x200)+body+struct.pack('<II',len(packed),len(table))+packed


def png(grid, spec):
    # Portable PNG preview, no imaging dependency. Marker crosses are the
    # protected coordinates; y is reversed to match the game's map convention.
    h,w=len(grid)*2,len(grid[0])*2;points={tuple(p) for z in spec['zones'] for p in z['points']}
    scan=bytearray()
    for y in range(h-1,-1,-1):
        scan.append(0)
        for x in range(w):
            marker=any((abs(x-px)<=2 and y==py) or (abs(y-py)<=2 and x==px) for px,py in points)
            scan+=bytes((240,70,80) if marker else ((216,209,189) if grid[y//2][x//2] else (44,39,53)))
    def chunk(kind,data):return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data))
    return b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',w,h,8,2,0,0,0))+chunk(b'IDAT',zlib.compress(scan,9))+chunk(b'IEND',b'')


def minimap(grid):
    # Base-client minimaps use 512x512, 8-bit indexed Windows BMP files.
    size=512;palette=bytearray(1024)
    palette[0:4]=bytes((53,39,44,0));palette[4:8]=bytes((189,209,216,0))
    pixels=bytearray()
    for y in range(size):
        for x in range(size):pixels.append(grid[y*len(grid)//size][x*len(grid[0])//size])
    offset=14+40+1024
    return (b'BM'+struct.pack('<IHHI',offset+len(pixels),0,0,offset)
            +struct.pack('<IiiHHIIiiII',40,size,size,1,8,0,len(pixels),0,0,256,0)
            +palette+pixels)


def verify(grid,spec):
    def component(start):
        seen={start};todo=[start]
        for x,y in todo:
            for q in [(x-1,y),(x+1,y),(x,y-1),(x,y+1)]:
                if q not in seen and 0<=q[1]<len(grid) and 0<=q[0]<len(grid[0]) and grid[q[1]][q[0]]:seen.add(q);todo.append(q)
        return seen
    receipts=[];components=[]
    for zone in spec['zones']:
        points=[(x//2,y//2) for x,y in zone['points']];assert all(grid[y][x] for x,y in points)
        seen=component(points[0]);assert set(points)<=seen,zone['name']
        assert not any(seen & old for old in components),'Scripted zones merged: '+zone['name']
        components.append(seen);receipts.append({'zone':zone['name'],'walkable_gat_cells':len(seen)*4,'protected_points':zone['points'],'connected':True})
    return receipts



MAP_LABELS = (
    b"1@alice_mad.rsw#Alice's Twisted Madness#",
    b"2@alice_mad.rsw#Alice's Twisted Madness - Hidden Maze#",
)


def append_map_labels(original):
    """Keep every existing byte; append missing labels, rejecting conflicts."""
    missing=[]
    for expected in MAP_LABELS:
        name=expected.split(b'#',1)[0]
        found=[line for line in original.splitlines() if line.split(b'#',1)[0]==name]
        if found and found!=[expected]:
            raise ValueError('Conflicting or duplicate map label: '+name.decode())
        if not found:missing.append(expected)
    if not missing:return original
    separator=b'' if not original or original.endswith(b'\n') else (b'\n' if original.endswith(b'\r') else b'\r\n')
    return original+separator+b'\r\n'.join(missing)+b'\r\n'


def build(output, map_name_table=None):
    output.mkdir(parents=True,exist_ok=True);entries={};report={'original_geometry':True,'wall_height_units':18,'textures':list(TEXTURES),'maps':{}}
    for name,spec in CONTRACT.items():
        grid=terrain(name,spec);receipt=verify(grid,spec)
        for ext,data in [('gat',gat(grid)),('gnd',gnd(grid)),('rsw',rsw(name))]:
            (output/(name+'.'+ext)).write_bytes(data);entries['data\\'+name+'.'+ext]=data
        (output/(name+'.png')).write_bytes(png(grid,spec))
        bitmap=minimap(grid);(output/(name+'.bmp')).write_bytes(bitmap)
        entries['data\\texture\\'+bytes.fromhex('c0afc0fac0cec5cdc6e4c0ccbdba').decode('cp949')+'\\map\\'+name+'.bmp']=bitmap
        report['maps'][name]={'size':spec['size'],'zones':receipt}
    if map_name_table is not None:
        original=map_name_table.read_bytes();labels=append_map_labels(original)
        entries['data\\mapnametable.txt']=labels
        (output/'mapnametable.txt').write_bytes(labels)
        report['map_name_table']={'source_sha256':hashlib.sha256(original).hexdigest(),
                                  'all_prior_bytes_preserved':labels.startswith(original)}
    archive=grf(entries);(output/'alice_maze.grf').write_bytes(archive)
    report['files']={name.replace('data\\',''):{'bytes':len(raw),'sha256':hashlib.sha256(raw).hexdigest()} for name,raw in entries.items()}
    report['grf_sha256']=hashlib.sha256(archive).hexdigest()
    (output/'spatial-contract.json').write_text(json.dumps(CONTRACT,indent=2)+'\n');(output/'build-report.json').write_text(json.dumps(report,indent=2)+'\n')
    return report


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--map-name-table',type=Path,help='Extracted effective data/mapnametable.txt; adds two labels without altering prior bytes');args=parser.parse_args();print(json.dumps(build(args.output,args.map_name_table),indent=2))
