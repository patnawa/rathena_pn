#!/usr/bin/env python3
"""Verify Office asset aliases/layout and execute copy/travel rules in native VM."""
import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
import sys
import tempfile
import zlib
import yaml

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools'))
from build_main_office import MAPS,build,cache_records,write_cache,merge
from generate_main_office_layout import component
from biosphere_crown_transaction_test import WRAPPERS
from audit_enchant_upgrades import renewal_records

def unpack_grf(data):
    assert data[:16]==b'Master of Magic\0'
    tableoff,seed,count,version=struct.unpack_from('<IIII',data,30)
    assert seed==0 and version==0x200
    packed,rawlen=struct.unpack_from('<II',data,46+tableoff)
    table=zlib.decompress(data[54+tableoff:54+tableoff+packed]);assert len(table)==rawlen
    result={};pos=0
    for _ in range(count-7):
        end=table.index(0,pos);name=table[pos:end].decode();pos=end+1
        size,aligned,length,flags,offset=struct.unpack_from('<IIIBI',table,pos);pos+=17
        assert size==aligned and flags==1
        raw=zlib.decompress(data[46+offset:46+offset+size]);assert len(raw)==length
        result[name]=raw
    assert pos==len(table);return result

def static():
    with tempfile.TemporaryDirectory() as tmp:
        root=Path(tmp);(root/'db/re').mkdir(parents=True);assets=root/'assets';assets.mkdir();records={};originals={}
        for i,source in enumerate(MAPS.values()):
            cells=bytes([0,3,1,0]*4);packed=zlib.compress(cells)
            records[source]=struct.pack('<12shhi',source.encode(),4,4,len(packed))+packed
            gat=b'GRAT\x01\x02'+struct.pack('<II',4,4)+b''.join(struct.pack('<ffffI',i,i+1,i+2,i+3,c) for c in cells)
            rsw=bytearray(b'GRSW\x02\x01'+bytes(170));rsw[46:86]=(source+'.gnd').encode().ljust(40,b'\0');rsw[86:126]=(source+'.gat').encode().ljust(40,b'\0')
            for ext,data in [('gat',gat),('gnd',b'GRGN-test-original'),('rsw',bytes(rsw))]:
                (assets/(source+'.'+ext)).write_bytes(data);originals[source+'.'+ext]=data
        cache=root/'db/re/map_cache.dat';write_cache(cache,records);before=cache.read_bytes()
        output=root/'out';build(root,assets,output);grf=unpack_grf((output/'pn_office.grf').read_bytes())
        assert len(grf)==9 and cache.read_bytes()==before
        aliases=cache_records(output/'office-map-cache.dat')
        for alias,source in MAPS.items():
            assert aliases[alias][12:]==records[source][12:]
            for ext in ('gat','gnd'): assert grf['data\\'+alias+'.'+ext]==originals[source+'.'+ext]
            expected=bytearray(originals[source+'.rsw']);expected[46:86]=(alias+'.gnd').encode().ljust(40,b'\0');expected[86:126]=(alias+'.gat').encode().ljust(40,b'\0')
            assert grf['data\\'+alias+'.rsw']==expected
        merge(cache,output/'office-map-cache.dat');merged=cache.read_bytes();merge(cache,output/'office-map-cache.dat');assert cache.read_bytes()==merged
        assert all(cache_records(cache)[k]==v for k,v in records.items())
        drift=dict(aliases);key=next(iter(drift));bad=bytearray(drift[key]);bad[12:14]=struct.pack('<h',2)
        # Same valid cell count, different dimensions: collision must reject.
        bad[14:16]=struct.pack('<h',8);drift[key]=bytes(bad);write_cache(root/'drift.dat',drift)
        try: merge(cache,root/'drift.dat')
        except AssertionError: pass
        else: raise AssertionError('Conflicting existing alias accepted')
        assert cache.read_bytes()==merged
        gat=assets/(next(iter(MAPS.values()))+'.gat');damaged=bytearray(gat.read_bytes());damaged[30:34]=struct.pack('<I',1);gat.write_bytes(damaged)
        try: build(root,assets,root/'bad-output')
        except AssertionError: pass
        else: raise AssertionError('Client/server walkability mismatch accepted')
    layout=json.loads((ROOT/'npc/custom/main_office/layout.json').read_text());rows=layout['desks']
    assert len(rows)==52 and len({r['unique_name'] for r in rows})==52
    effective={}
    for path in ('db/import/map_cache.dat','db/re/map_cache.dat','db/map_cache.dat'):
        if (ROOT/path).exists():
            for name,record in cache_records(ROOT/path).items():effective.setdefault(name,record)
    for floor,source in MAPS.items():
        record=effective[source];w,h=struct.unpack_from('<hh',record,12);cells=zlib.decompress(record[20:]);desks=[r for r in rows if r['map']==floor]
        blocked={tuple(r['npc']) for r in desks};assert len(blocked)==len(desks)
        reached=component(w,h,cells,tuple(layout['entrances'][floor]),blocked)
        for row in desks:
            assert tuple(row['approach']) in reached
            assert sum(abs(a-b) for a,b in zip(row['npc'],row['approach']))==1
        if floor in effective: assert effective[floor][12:]==record[12:]
    active=set()
    def include(path):
        if path in active:return
        active.add(path)
        for kind,name in re.findall(r'^(npc|import):\s*([^\s]+)',(ROOT/path).read_text(errors='replace'),re.M):
            if kind=='import':include(name)
            else:active.add(name)
    include('npc/re/scripts_main.conf')
    definitions={}
    pattern=re.compile(r'^([^\t\r\n]+)\t(?:script|shop|cashshop|duplicate\([^\r\n]+?\))\t([^\t\r\n]+)\t',re.M)
    for file in active:
        for declaration in pattern.finditer((ROOT/file).read_text(errors='replace')):
            definitions.setdefault(declaration[2],[]).append(file)
    for row in rows: assert len(definitions.get(row['source'],[]))==1,(row['source'],definitions.get(row['source']))
    text=(ROOT/'npc/custom/main_office/services.txt').read_text()
    assert 'instance_id() ||' not in text
    assert '"%0*u#%0*u"' in (ROOT/'src/map/instance.cpp').read_text(),'Review instance-map delimiter guard after native naming changes'
    ids=list(map(int,re.search(r'setarray \.@ids,([\d,]+);',text)[1].split(',')))
    skills={}
    def overlay(old,new):
        for key,value in new.items():
            if isinstance(value,dict) and isinstance(old.get(key),dict):overlay(old[key],value)
            else:old[key]=value
    for row in renewal_records(ROOT,'db/skill_db.yml'):overlay(skills.setdefault(row['Id'],{}),row)
    for id in ids: assert skills[id]['CopyFlags']['Skill']=={'Plagiarism':True,'Reproduce':True},id
    print('OFFICE_STATIC_OK: GRF roundtrip, source preservation, collision/drift refusals, 52 reachable desks, active inherited templates, copy eligibility')
    return skills,ids

def native(builddir,skills,ids):
    builddir=builddir.resolve();builddir.mkdir(parents=True,exist_ok=True)
    prefix=(ROOT/'tools/ci/biosphere_crown_transaction_test.cpp').read_text().split('extern "C" int __wrap_main(',1)[0]
    for name in ('setreg','readreg','registry','named_registry','setstr','readstr'):
        prefix=re.sub(r'^extern "C"[^\n]*\b'+name+r'\([^\n]*\n','',prefix,flags=re.M)
    rune=(ROOT/'tools/ci/rune_tablet_transaction_test.cpp').read_text().split('extern "C" int __wrap_main(',1)[0]
    combined=builddir/'combined_office_test.cpp';combined.write_text(prefix+'\n'+rune+'\n'+(ROOT/'tools/ci/main_office_test.cpp').read_text())
    (builddir/'skills.yml').write_text(yaml.safe_dump({'Body':[skills[id] for id in ids+[225,2285]]},sort_keys=False))
    itemnames=set()
    def itemrefs(value):
        if isinstance(value,dict):
            for key,v in value.items():
                if key=='Item' and isinstance(v,str):itemnames.add(v)
                else:itemrefs(v)
        elif isinstance(value,list):
            for v in value:itemrefs(v)
    for id in ids+[225,2285]:itemrefs(skills[id])
    required={}
    for row in renewal_records(ROOT,'db/item_db.yml'):
        if row.get('AegisName') in itemnames:required.setdefault(row['Id'],{}).update(row)
    (builddir/'office-items.yml').write_text(yaml.safe_dump({'Body':list(required.values())},sort_keys=False))
    sources=[ROOT/'src/map'/f'{n}.cpp' for n in ('pc','script','itemdb','clif','skill')]+[ROOT/'src/common/malloc.cpp',combined]
    san=['-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer']
    flags=['g++','-std=c++17','-O0','-g','-DPACKETVER=20260219','-fno-strict-aliasing']+san+['-I'+str(ROOT/p) for p in ('src','3rdparty/libconfig','3rdparty/rapidyaml/src','3rdparty/rapidyaml/ext/c4core/src','3rdparty/json/include')]+['-I/usr/include/mysql']
    headers=hashlib.sha256(b''.join(p.read_bytes() for p in sorted((ROOT/'src').rglob('*.hpp')))).hexdigest()
    def compile_one(source):
        target=builddir/(source.stem+'.o');sha=hashlib.sha256(source.read_bytes()+repr(flags).encode()+headers.encode()).hexdigest();receipt=target.with_suffix('.sha')
        if not target.exists() or not receipt.exists() or receipt.read_text()!=sha:
            print('Compile '+str(source),flush=True);subprocess.run(flags+['-c',str(source),'-o',str(target)],cwd=ROOT,check=True);receipt.write_text(sha)
        return target
    with ThreadPoolExecutor(max_workers=2) as pool:fresh=list(pool.map(compile_one,sources))
    excluded={p.name for p in fresh};objects=sorted(p for p in (ROOT/'src/map/obj').rglob('*.o') if p.name not in excluded)
    libs=[ROOT/p for p in ('src/common/obj/common.a','3rdparty/libconfig/obj/libconfig.a','3rdparty/rapidyaml/obj/ryml.a')]
    wrappers=[w for w in WRAPPERS if not any(x in w for x in ('pc_setreg','pc_readreg'))]+['_Z9map_id2bli','_Z17map_mapname2mapidPKc','_Z17mapindex_name2idxPKcS0_','_Z9pc_setposP16map_session_datatii8clr_type','_Z13clif_addskillRK16map_session_datat','_Z16clif_deleteskillRK16map_session_datatb','_Z24clif_messagecolor_targetPK10block_listmPKcb11send_targetPK16map_session_data']
    exe=builddir/'main_office_test';subprocess.run(['g++']+san+['-o',str(exe)]+[str(p) for p in fresh+objects+libs]+['-Wl,--wrap='+w for w in wrappers]+['-lz','-ldl','-lmysqlclient','-lzstd','-lssl','-lcrypto','-lresolv','-lm'],cwd=ROOT,check=True)
    result=subprocess.run([str(exe),str(builddir)],cwd=ROOT,capture_output=True,text=True,timeout=120);print(result.stdout,end='');print(result.stderr,end='');result.check_returncode();assert 'OFFICE_NATIVE_OK' in result.stdout

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--native-build-dir',type=Path);args=parser.parse_args();skills,ids=static()
    if args.native_build_dir:native(args.native_build_dir,skills,ids)
