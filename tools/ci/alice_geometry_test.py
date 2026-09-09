"""Validate generated original mazes and exact render/collision agreement."""
from pathlib import Path
import importlib.util
import json
import re
import subprocess
import sys
import struct
import unittest
import tempfile

ROOT = Path(__file__).resolve().parents[2]
PATCH = ROOT / 'client-patch/alice_maze'
spec = importlib.util.spec_from_file_location('alice_map_builder', PATCH/'build_maps.py')
builder = importlib.util.module_from_spec(spec)
spec.loader.exec_module(builder)


class AliceGeometry(unittest.TestCase):
    def test_deterministic_assets_and_protected_zones(self):
        self.assertEqual(json.loads((PATCH/'spatial-contract.json').read_text()), builder.CONTRACT)
        for name, contract in builder.CONTRACT.items():
            grid = builder.terrain(name, contract)
            builder.verify(grid, contract)
            for extension, raw in [('gat',builder.gat(grid)),('gnd',builder.gnd(grid)),('rsw',builder.rsw(name))]:
                self.assertEqual((PATCH/'data'/(name+'.'+extension)).read_bytes(), raw, (name,extension))

    def test_rendered_walls_exactly_match_collision(self):
        for name, contract in builder.CONTRACT.items():
            gat = (PATCH/'data'/(name+'.gat')).read_bytes()
            gnd = (PATCH/'data'/(name+'.gnd')).read_bytes()
            self.assertEqual(gat[:6],b'GRAT\x01\x02')
            self.assertEqual(gnd[:6],b'GRGN\x01\x07')
            gw,gh = struct.unpack_from('<II',gat,6)
            w,h,scale,textures,path_size = struct.unpack_from('<IIfII',gnd,6)
            self.assertEqual((gw,gh),(w*2,h*2))
            self.assertEqual((gw,gh),tuple(contract['size']))
            self.assertEqual(scale,10)
            paths=[gnd[26+i*path_size:26+(i+1)*path_size].split(b'\0')[0].decode().replace('\\','/') for i in range(textures)]
            self.assertEqual(paths,list(builder.TEXTURES))
            offset=26+textures*path_size
            count,lw,lh,cellsize=struct.unpack_from('<4I',gnd,offset)
            offset+=16+count*lw*lh*cellsize*4
            tiles=struct.unpack_from('<I',gnd,offset)[0];offset+=4+tiles*40
            self.assertEqual(len(gnd),offset+w*h*28)
            self.assertEqual(len(gat),14+gw*gh*20)
            for y in range(h):
                for x in range(w):
                    heights=struct.unpack_from('<4f',gnd,offset+(x+y*w)*28)
                    self.assertIn(heights,[(0.,)*4,(-18.,)*4])
                    top,front,side=struct.unpack_from('<3i',gnd,offset+(x+y*w)*28+16)
                    self.assertEqual(top,0 if heights[0]==0 else 1)
                    self.assertEqual((front,side),(1,1))
                    for dx,dy in [(0,0),(1,0),(0,1),(1,1)]:
                        cell=struct.unpack_from('<4fI',gat,14+((2*x+dx)+(2*y+dy)*gw)*20)
                        self.assertEqual(cell[:4],heights)
                        self.assertEqual(cell[4],0 if heights[0]==0 else 1)

    def test_actual_npc_and_travel_coordinates(self):
        source=(ROOT/'npc/custom/instances/AliceTwistedMadness.txt').read_text()
        maps={}
        for name in builder.CONTRACT:
            raw=(PATCH/'data'/(name+'.gat')).read_bytes()
            width,height=struct.unpack_from('<II',raw,6)
            maps[name]=(width,height,[struct.unpack_from('<I',raw,30+i*20)[0] for i in range(width*height)])
        sys.path.insert(0,str(ROOT/'tools'))
        from inspect_mapcache import load_maps
        maps.update(load_maps(ROOT/'db/map_cache.dat',{'dali'}))
        def walk(name,x,y):
            width,height,cells=maps[name]
            self.assertTrue(0<=x<width and 0<=y<height,(name,x,y))
            self.assertEqual(cells[x+y*width],0,(name,x,y))
        npcs=re.findall(r'^(1@alice_mad|2@alice_mad|dali),(\d+),(\d+),\d+\s+script\s+[^\t]+\t([^,]+)',source,re.M)
        self.assertGreater(len(npcs),30)
        for name,x,y,sprite in npcs:
            if sprite!='-1':walk(name,int(x),int(y))
        travels=re.findall(r'callfunc "F_AliceTravel",[^,;]+,([12]),(\d+),(\d+);',source)
        self.assertGreater(len(travels),10)
        for number,x,y in travels:walk(number+'@alice_mad',int(x),int(y))
        for name,x,y in re.findall(r'\b(?:warp|monster) instance_mapname\("([12]@alice_mad)"\),(\d+),(\d+)',source):walk(name,int(x),int(y))
        for name,x,y in re.findall(r'warp "(dali)",(\d+),(\d+)',source):walk(name,int(x),int(y))
        # Formula-driven corridor arrivals have six possible destinations.
        self.assertIn("12*'alice_corridor[.@cid],20",source)
        for x in range(12,73,12):walk('1@alice_mad',x,20)
        subprocess.run([sys.executable,str(ROOT/'tools/build_alice_mapcache.py'),'--check'],check=True)

    def test_minimap_bitmap_dimensions_and_floor_projection(self):
        for name,contract in builder.CONTRACT.items():
            grid=builder.terrain(name,contract)
            raw=(PATCH/'data'/(name+'.bmp')).read_bytes()
            self.assertEqual(raw[:2],b'BM')
            self.assertEqual(struct.unpack_from('<iiHH',raw,18),(512,512,1,8))
            offset=struct.unpack_from('<I',raw,10)[0]
            self.assertEqual(len(raw),offset+512*512)
            for y in range(512):
                expected=bytes(grid[y*len(grid)//512][x*len(grid[0])//512] for x in range(512))
                self.assertEqual(raw[offset+y*512:offset+(y+1)*512],expected)

    def test_map_label_overlay_preserves_existing_bytes(self):
        original=b"prontera.rsw#Prontera#\r\nlegacy.rsw#\xb0\xa1#"
        labels=builder.append_map_labels(original)
        self.assertEqual(labels,original+b'\r\n'+b'\r\n'.join(builder.MAP_LABELS)+b'\r\n')
        self.assertEqual(builder.append_map_labels(labels),labels)
        self.assertEqual(builder.append_map_labels(b''),b'\r\n'.join(builder.MAP_LABELS)+b'\r\n')
        with self.assertRaises(ValueError):
            builder.append_map_labels(b'1@alice_mad.rsw#Conflicting label#\n')
        with self.assertRaises(ValueError):
            builder.append_map_labels(builder.MAP_LABELS[0]+b'\n'+builder.MAP_LABELS[0]+b'\n')
        sys.path.insert(0,str(ROOT/'client-patch/client_compat'))
        from merge_grfs import read
        with tempfile.TemporaryDirectory() as tmp:
            directory=Path(tmp);source=directory/'existing.txt';source.write_bytes(original)
            output=directory/'built';builder.build(output,source)
            archive=read((output/'alice_maze.grf').read_bytes())
            self.assertEqual(len(archive),9)
            self.assertEqual(archive[b'data\\mapnametable.txt'],labels)
            self.assertEqual(source.read_bytes(),original)

    def test_world_references_own_geometry(self):
        for name in builder.CONTRACT:
            raw=(PATCH/'data'/(name+'.rsw')).read_bytes()
            self.assertEqual(raw[:6],b'GRSW\x02\x00')
            self.assertEqual(raw[46:86].split(b'\0')[0],(name+'.gnd').encode())
            self.assertEqual(raw[86:126].split(b'\0')[0],(name+'.gat').encode())
            self.assertEqual(len(raw),246)
            self.assertEqual(struct.unpack_from('<I',raw,242)[0],0)


if __name__=='__main__':
    unittest.main()
