#!/usr/bin/env python3
"""Execute legacy HD NPC rate expressions in the native VM and effective refine DB."""
import argparse, hashlib, json, re, subprocess
from pathlib import Path
from biosphere_crown_transaction_test import WRAPPERS
from audit_enchant_upgrades import renewal_records
import yaml

ROOT=Path(__file__).resolve().parents[2]

def run(build,hd_source):
    build.mkdir(parents=True,exist_ok=True)
    records={}
    for row in renewal_records(ROOT,'db/item_db.yml'):
        records.setdefault(row['Id'],{}).update(row)
    # Only material identity is required by the native refine database parser.
    materials={m for p in (ROOT/'db/re/refine.yml',ROOT/'db/import/refine.yml')
               for m in re.findall(r'(?m)^\s+Material: (\S+)',p.read_text())}
    items=[{'Id':r['Id'],'AegisName':r['AegisName'],'Name':r['Name'],'Type':'Etc'}
           for r in records.values() if r['AegisName'] in materials]
    assert {r['AegisName'] for r in items}==materials
    (build/'items.yml').write_text(yaml.safe_dump({'Body':items}))
    source=hd_source.read_text()
    rates=re.findall(r'if \(([^\n]+) > rand\((100(?:00)?)\)\)',source)
    assert len(rates)==2
    for i,(expression,scale) in enumerate(rates):
        (build/f'rate{i}.txt').write_text('{ .@part=EQI_HEAD_TOP; @answer='+expression+'; end; }')
    (build/'scales.txt').write_text(' '.join(x[1] for x in rates))
    (build/'shadow.txt').write_bytes((ROOT/'npc/re/merchants/shadow_refiner.txt').read_bytes())
    (build/'master.txt').write_bytes((ROOT/'npc/custom/grademk_services.txt').read_bytes())
    prefix=(ROOT/'tools/ci/biosphere_crown_transaction_test.cpp').read_text().split('extern "C" int __wrap_main(',1)[0]
    driver=build/'refine_recipe_driver.cpp'
    driver.write_text(prefix+(ROOT/'tools/ci/refine_recipe_native_test.cpp').read_text())
    flags=['g++','-std=c++17','-O0','-g','-DPACKETVER=20260219','-fno-strict-aliasing','-fsanitize=undefined','-fno-sanitize-recover=all']
    flags+=['-I'+p for p in ('src','3rdparty/libconfig','3rdparty/rapidyaml/src','3rdparty/rapidyaml/ext/c4core/src','3rdparty/json/include','/usr/include/mysql')]
    objects=[]
    for path in (ROOT/'src/map/script.cpp',ROOT/'src/common/malloc.cpp',driver):
        obj=build/(path.stem+'.o');stamp=obj.with_suffix('.sha256')
        fingerprint=hashlib.sha256(path.read_bytes()+repr(flags).encode()).hexdigest()
        if not obj.exists() or not stamp.exists() or stamp.read_text()!=fingerprint:
            subprocess.run(flags+['-c',str(path),'-o',str(obj)],cwd=ROOT,check=True)
            stamp.write_text(fingerprint)
        objects.append(obj)
    support=sorted(p for p in (ROOT/'src/map/obj').rglob('*.o') if p.name!='script.o')
    libraries=[ROOT/p for p in ('src/common/obj/common.a','3rdparty/libconfig/obj/libconfig.a','3rdparty/rapidyaml/obj/ryml.a')]
    binary=build/'refine_recipe_native_test'
    subprocess.run(['g++','-fsanitize=undefined','-o',str(binary)]+[str(p) for p in objects+support+libraries]+
        ['-Wl,--wrap='+n for n in (*WRAPPERS,'_Z18clif_refineui_openP16map_session_data')]+['-lz','-ldl','-lmysqlclient','-lzstd','-lssl','-lcrypto','-lresolv','-lm'],cwd=ROOT,check=True)
    result=subprocess.run([str(binary),str(build)],cwd=ROOT,capture_output=True,text=True)
    (build/'result.log').write_text(result.stdout+result.stderr)
    print(result.stdout+result.stderr);result.check_returncode()
    assert 'No memory leaks found' in result.stdout+result.stderr
    assert not re.search(r'\[(Error|Warning)\]|runtime error:',result.stdout+result.stderr)
    # Restore each old NPC rate query independently; the real VM must reject it.
    for index in range(2):
        path=build/f'rate{index}.txt';current=path.read_text();scale=(build/'scales.txt').read_text()
        try:
            path.write_text('{ .@part=EQI_HEAD_TOP; @answer=getequippercentrefinery(.@part,true); end; }')
            oldscale=scale.split();oldscale[index]='100';(build/'scales.txt').write_text(' '.join(oldscale))
            red=subprocess.run([str(binary),str(build)],cwd=ROOT,capture_output=True,text=True)
            assert red.returncode!=0 and 'RATE_MISMATCH' in red.stdout
            print(f'REFINE_RATE_REGRESSION_REJECTED npc={index}')
        finally:path.write_text(current);(build/'scales.txt').write_text(scale)

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--build-dir',type=Path,required=True)
    p.add_argument('--hd-source',type=Path,default=ROOT/'npc/re/merchants/hd_refiner.txt')
    args=p.parse_args();run(args.build_dir.resolve(),args.hd_source.resolve())
