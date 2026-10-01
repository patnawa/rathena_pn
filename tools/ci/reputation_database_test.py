#!/usr/bin/env python3
"""Reproduce current reputation parser/generator defects; never a repair PASS."""
from concurrent.futures import ThreadPoolExecutor
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import yaml

ROOT=Path(__file__).resolve().parents[2]
PINS={
    'src/map/pc.cpp':'660c0bcc0e9efeaafc65c862fb08d97de8d1dd2648548eeb1714919f247ec70f',
    'src/map/pc.hpp':'e7fef6dcf8cd03a2a934095f44296becc4e39026d8c140cbd22be71be7b9b768',
    'src/common/database.cpp':'03d394aa9ed809557fb4ce50419776e34214220504ef1229ec864a4b10be0011',
    'src/common/database.hpp':'1b54c493748f34907cd26e5be27959fc4bdbb2ea041ef32637dc01dd98fbad3e',
    'src/common/malloc.cpp':'7ab3e15c67b53e67c631feaf8cb5d5bd4bf75c0814abc6393becfdf055a6f171',
    '3rdparty/json/include/nlohmann/json.hpp':'a83dfaff19735532eb42510fd13c22a3154c0bc0f32b69d7c6c1b0da39d65174',
}
def sha(data):return hashlib.sha256(data).hexdigest()
def normalized(data):return data.replace(b'\r\n',b'\n')
def require(ok,message):
    if not ok:raise AssertionError(message)
def excerpt(text,start,end,source):
    require(text.count(start)==text.count(end)==1,'exact unique excerpt anchors: '+source)
    a,b=text.index(start),text.index(end)
    require(a<b,'excerpt order')
    return '#line '+str(text[:a].count('\n')+1)+' "'+source+'"\n'+text[a:b]
def graph(root,generator,cache):
    rows=[];active=set();edges=[]
    def visit(path):
        target=(ROOT/path).resolve();target.relative_to(ROOT)
        require(path not in active,'import cycle');active.add(path)
        cache.setdefault(path,(ROOT/path).read_bytes())
        data=yaml.safe_load(cache[path]);require(set(data)<= {'Header','Body','Footer'},'known YAML schema')
        rows.extend(data.get('Body') or [])
        for entry in (data.get('Footer') or {}).get('Imports') or []:
            require(set(entry)<= {'Path','Mode','Generator'},'known native import schema')
            mode=entry.get('Mode','Renewal');require(mode in ('Renewal','Prerenewal'),'known mode')
            if 'Generator' in entry:require(type(entry['Generator']) is bool,'actual generator bool')
            selected=mode=='Renewal' and ('Generator' not in entry or generator and entry['Generator'])
            edges.append([path,entry,selected])
            if selected:visit(entry['Path'])
        active.remove(path)
    visit(root)
    effective={}
    for row in rows:effective.setdefault(row['Id'],{}).update(row)
    return effective,edges
def run(command,cwd,env=None):
    p=subprocess.run(command,cwd=cwd,env=env,text=True,capture_output=True)
    return p
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir',type=Path,required=True)
    args=parser.parse_args();build=args.build_dir.resolve()
    require(build!=ROOT and ROOT not in build.parents and not build.exists(),'fresh artifact directory outside repository required')
    source_paths=set(PINS)|{'tools/ci/reputation_database_test.py','tools/ci/reputation_database_test.cpp'}
    support=list((ROOT/'3rdparty/rapidyaml/src').rglob('*.cpp'))+list((ROOT/'3rdparty/rapidyaml/ext/c4core/src').rglob('*.cpp'))
    source_paths.update(p.relative_to(ROOT).as_posix() for p in support)
    for tree in ('src/common','src/config','src/custom','3rdparty/rapidyaml/src','3rdparty/rapidyaml/ext/c4core/src','3rdparty/libconfig'):
        source_paths.update(p.relative_to(ROOT).as_posix() for p in (ROOT/tree).rglob('*') if p.is_file() and p.suffix in ('.h','.hpp','.inl'))
    raw={path:(ROOT/path).read_bytes() for path in sorted(source_paths)}
    for path,pin in PINS.items():require(sha(normalized(raw[path]))==pin,'reviewed original source changed: '+path)
    data={};expected={};graphs={}
    for mode in ('normal','generator'):
        expected[mode],graphs[mode]=graph('db/reputation.yml',mode=='generator',data)
        _,graphs[mode+'_groups']=graph('db/reputation_group.yml',mode=='generator',data)
        require(len(expected[mode])==13,'exact current13 effective entries')
    require(set(expected['normal'])=={1,2,3,4,6,9,13,14,15,16,17,18,19},'reviewed current identities')
    build.mkdir();(build/'obj').mkdir()
    pc=normalized(raw['src/map/pc.cpp']).decode();header=normalized(raw['src/map/pc.hpp']).decode()
    generated_header='#pragma once\n#include <common/database.hpp>\n'+excerpt(header,'struct s_reputation{','struct s_statpoint_entry{','reviewed_pc.hpp')
    generated_source='#include "reviewed_reputation.hpp"\n#include <common/showmsg.hpp>\n#include <nlohmann/json.hpp>\n#include <fstream>\n#include <cmath>\n'+excerpt(pc,'const std::string ReputationDatabase::getDefaultLocation(){','const std::string PenaltyDatabase::getDefaultLocation(){','reviewed_pc.cpp')
    (build/'reviewed_reputation.hpp').write_text(generated_header)
    (build/'reviewed_reputation.cpp').write_text(generated_source)
    for mode in ('normal','generator','minimum_ub','maximum_ub'):
        directory=build/mode;directory.mkdir()
        for path,content in data.items():
            target=directory/path;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(content)
        (directory/'expected.json').write_text(json.dumps(list(expected['generator' if mode=='generator' else 'normal'].values()),sort_keys=True))
    flags=['g++','-std=c++17','-O0','-g','-fno-strict-aliasing','-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer','-fno-pie']
    flags+=['-I'+str(ROOT/p) for p in ('src','3rdparty/libconfig','3rdparty/rapidyaml/src','3rdparty/rapidyaml/ext/c4core/src','3rdparty/json/include')]
    flags+=['-I'+str(build),'-include',str(ROOT/'src/config/renewal.hpp')]
    commands=[]
    def compile_one(pair):
        label,source,extra=pair;obj=build/'obj'/(label+'.o');cmd=flags+extra+['-c',str(source),'-o',str(obj)]
        p=run(cmd,build);(build/(label+'.compile.log')).write_text(p.stdout+p.stderr)
        require(p.returncode==0,'compile failed: '+label+'\n'+p.stderr[-6000:]);commands.append(cmd);return obj
    print('COMPILING_ISOLATED_CURRENT_SOURCE: no existing objects reused or changed',flush=True)
    jobs=[('ryml'+str(n),path,[]) for n,path in enumerate(support)]
    jobs += [('database',ROOT/'src/common/database.cpp',[]),('malloc',ROOT/'src/common/malloc.cpp',[])]
    with ThreadPoolExecutor(max_workers=3) as pool:objects=list(pool.map(compile_one,jobs))
    executables={}
    for mode in ('normal','generator'):
        extra=['-DMAP_GENERATOR'] if mode=='generator' else []
        parts=[compile_one((mode+'-reputation',build/'reviewed_reputation.cpp',extra)),compile_one((mode+'-driver',ROOT/'tools/ci/reputation_database_test.cpp',extra))]
        exe=build/('reputation-'+mode);cmd=flags+['-no-pie']+list(map(str,objects+parts))+['-o',str(exe)];p=run(cmd,build)
        (build/(mode+'.link.log')).write_text(p.stdout+p.stderr);require(p.returncode==0,'link failed '+mode+'\n'+p.stderr[-6000:]);commands.append(cmd);executables[mode]=exe
    env=os.environ.copy();env['ASAN_OPTIONS']='detect_leaks=1:halt_on_error=1';env['UBSAN_OPTIONS']='halt_on_error=1:print_stacktrace=1'
    evidence={}
    for mode in ('normal','generator','minimum_ub','maximum_ub'):
        p=run([str(executables['normal' if mode=='normal' else 'generator']),'current' if mode in ('normal','generator') else mode],build/mode,env)
        (build/(mode+'.stdout.txt')).write_text(p.stdout);(build/(mode+'.stderr.txt')).write_text(p.stderr)
        evidence[mode]={'returncode':p.returncode,'stdout_sha256':sha(p.stdout.encode()),'stderr_sha256':sha(p.stderr.encode())}
        if mode in ('normal','generator'):
            require(p.returncode==0,'current reproduction failed '+mode+'\n'+p.stdout+p.stderr)
            require('UNEXPECTED_NATIVE_DIAGNOSTIC' not in p.stdout+p.stderr and not re.search(r'AddressSanitizer|runtime error:|UndefinedBehaviorSanitizer|\[(?:Error|Warning)\]',p.stdout+p.stderr),'unexpected diagnostic '+mode)
            require(p.stdout.count('Memory manager: No memory leaks found.')==1,'actual clean allocator teardown')
            match=re.findall(r'^REPUTATION_CURRENT_REPRODUCTION_COMPLETE (.+)$',p.stdout,re.M);require(len(match)==1,'unique reproduction marker')
            evidence[mode]['observed']=json.loads(match[0])
        else:
            endpoint='minimum' if mode=='minimum_ub' else 'maximum'
            line=next(n for n,text in enumerate(pc.splitlines(),1) if 'std::abs(rep->'+endpoint+')' in text)
            require(p.returncode!=0 and p.stdout.count('EXPECT_GENERATOR_'+mode)==1,'exact expected isolated UB entry')
            require(re.search(r'reviewed_pc.cpp:'+str(line)+r':\d+: runtime error: negation of -9223372036854775808 cannot be represented',p.stderr),'exact endpoint UBSan diagnostic '+mode+'\n'+p.stderr)
            require(p.stderr.count('runtime error:')==1 and 'AddressSanitizer' not in p.stderr and 'UNEXPECTED_NATIVE_DIAGNOSTIC' not in p.stdout+p.stderr,'no unrelated expected-failure diagnostic')
            require(not (build/mode/'generated').exists(),'actual UB reached before opening/writing output directory')
            evidence[mode]['classification']='CONFIRMED_SIGNED_NEGATION_UNDEFINED_BEHAVIOR'
            evidence[mode]['original_source_line']=line
        print(mode,json.dumps(evidence[mode],sort_keys=True),flush=True)
    for path,content in {**raw,**data}.items():require((ROOT/path).read_bytes()==content,'source bytes changed during reproduction: '+path)
    receipt={'result':'REPRODUCED_CURRENT_DEFECTS_NOT_A_FIX_PASS','source_raw_sha256':{p:sha(v) for p,v in raw.items()},'reviewed_normalized_pins':PINS,
             'database_raw_sha256':{p:sha(v) for p,v in data.items()},'ordered_import_graphs':graphs,'executables':{k:sha(v.read_bytes()) for k,v in executables.items()},
             'generated_header_sha256':sha(generated_header.encode()),'generated_source_sha256':sha(generated_source.encode()),'compile_commands':commands,'evidence':evidence,
             'network':'kernel socket/connect/bind/listen denial','boundary':'actual extracted native classes/functions, fresh native YAML/allocator/RapidYAML; logger/config root are fixture boundaries; no runtime or existing objects changed'}
    (build/'receipt.json').write_text(json.dumps(receipt,sort_keys=True,indent=2)+'\n')
    print('CLASSIFIED_REPRODUCTION_RECEIPT '+sha((build/'receipt.json').read_bytes()),flush=True)
if __name__=='__main__':
    try:main()
    except (AssertionError,ValueError,OSError) as error:print('REPRODUCTION_FAILED: '+str(error),file=sys.stderr);raise SystemExit(1)
