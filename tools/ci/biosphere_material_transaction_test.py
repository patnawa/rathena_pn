#!/usr/bin/env python3
"""Isolated actual Omega/Ellie material VM proof; no server/network/SQL startup."""
import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import yaml

from audit_enchant_upgrades import renewal_records
from biosphere_crown_transaction_test import WRAPPERS as OLD_WRAPPERS
from biosphere_regression_scope import restore_reviewed_document

ROOT = Path(__file__).resolve().parents[2]
QUESTS = 'npc/custom/varmundt_biosphere_quests.txt'
DEPTH = 'npc/custom/varmundt_biosphere_depth.txt'
ACCESS = 'npc/custom/varmundt_biosphere.txt'
PREFIX = 'tools/ci/biosphere_crown_transaction_test.cpp'
DRIVER = 'tools/ci/biosphere_material_transaction_test.cpp'
BASE = {QUESTS: '787f6ff54a4c43a60510e4a606dc5bd33a80c502853f113ad6756e257ac5aa5a',
        DEPTH: '40730ba4620a448e03ad2d71ff6d28f9398934fa5c1af1e22cb083e6ac392f28'}
CANDIDATE = {QUESTS: 'edec07ec166f5f8ae4a7e90ed52e38a5f9d76cf5781d139754a4ebd9a1ba0a7a',
             DEPTH: '6582a4b1401398f505b695f67e213e1d14f9b9978c875ae24b663623ce3b6988'}


def require(ok, message):
    if not ok: raise AssertionError(message)


def sha(data): return hashlib.sha256(data).hexdigest()


def normal(data):
    data = data.replace(b'\r\n', b'\n')
    require(b'\r' not in data, 'Only LF/CRLF source newline forms are supported')
    return data


def replace_once(source, old, new):
    require(source.count(old) == 1, 'Exact source substitution drift: ' + old[:70])
    return source.replace(old, new, 1)


def reconstruct_original(path, data):
    """Inverse ONLY reviewed edits; full original hash guards genuine source.

    No sibling snapshot or Git invocation is required by a clean clone.
    Newline-only normalization is a test view; raw runtime files are untouched.
    """
    source = normal(restore_reviewed_document(data) if path == DEPTH else data).decode('utf-8')
    require(sha(source.encode()) == CANDIDATE[path], 'Reviewed candidate changed: ' + path)
    if path == QUESTS:
        start = source.index("// Only Omega's 17 material recipes")
        end = source.index('ba_in01,359,53,4\tscript\tEllie#biosphere_equipment', start)
        source = source[:start] + source[end:]
        # Omega has exactly two maxima/calls. Preserve every other NPC byte.
        source = replace_once(source, '\t\tif (.@max > .@zenymax)\n\t\t\t.@max = .@zenymax;\n\t\tif (.@max > 30000)\n\t\t\t.@max = 30000;',
            '\t\tif (.@max > .@zenymax)\n\t\t\t.@max = .@zenymax;')
        source = replace_once(source, '\t\tif (.@max > .@have)\n\t\t\t.@max = .@have;\n\t\tif (.@max > 30000)\n\t\t\t.@max = 30000;',
            '\t\tif (.@max > .@have)\n\t\t\t.@max = .@have;')
        source = replace_once(source,
            '\t\tif (input(.@amount,1,.@max) != 0)\n\t\t\tclose;\n\t\tif (!callfunc("F_BiosphereMaterialCommit",.@amount,.@output,20000,0,1,.@input,.@need))\n\t\t\tclose;',
            '\t\tinput .@amount,1,.@max;\n\t\tif (!checkweight(.@output,.@amount)) {\n\t\t\tmes "Make room for the converted material first.";\n\t\t\tclose;\n\t\t}\n\t\tdelitem .@input,.@need * .@amount;\n\t\tZeny -= 20000 * .@amount;\n\t\tgetitem .@output,.@amount;')
        source = replace_once(source,
            '\t\tif (input(.@amount,1,.@max) != 0)\n\t\t\tclose;\n\t\tif (!callfunc("F_BiosphereMaterialCommit",.@amount,.@water[.@water_type],20000,0,5,.@common_essence[0],5,.@common_essence[1],5,.@common_essence[2],5,.@common_essence[3],5,.@special_essence[.@water_type],5))\n\t\t\tclose;',
            '\t\tinput .@amount,1,.@max;\n\t\tif (!checkweight(.@water[.@water_type],.@amount)) {\n\t\t\tmes "Make room for the magical water first.";\n\t\t\tclose;\n\t\t}\n\t\tfor (.@i = 0; .@i < 4; ++.@i)\n\t\t\tdelitem .@common_essence[.@i],5 * .@amount;\n\t\tdelitem .@special_essence[.@water_type],5 * .@amount;\n\t\tZeny -= 20000 * .@amount;\n\t\tgetitem .@water[.@water_type],.@amount;')
    else:
        source = replace_once(source, '\t.@zenymax = Zeny / .@cost[.@stage];\n\tif (.@max > .@zenymax)\n\t\t.@max = .@zenymax;\n\tif (.@max > 30000)\n\t\t.@max = 30000;',
            '\t.@zenymax = Zeny / .@cost[.@stage];\n\tif (.@max > .@zenymax)\n\t\t.@max = .@zenymax;')
        source = replace_once(source,
            '\tif (input(.@amount,1,.@max) != 0)\n\t\tclose;\n\tif (!callfunc("F_BiosphereMaterialCommit",.@amount,.@output,.@cost[.@stage],1,1,.@input,.@need[.@stage]))\n\t\tclose;',
            '\tinput .@amount,1,.@max;\n\tif (!checkweight(.@output,.@amount)) {\n\t\tmes "Make room for the fused material first.";\n\t\tclose;\n\t}\n\tdelitem .@input,.@need[.@stage] * .@amount;\n\tZeny -= .@cost[.@stage] * .@amount;\n\tgetitem .@output,.@amount;')
    result = source.encode('utf-8')
    require(sha(result) == BASE[path], 'Entire pinned original differs; protected runtime bytes changed: ' + path)
    return result


def recipes():
    result = []
    def add(mats, needs, output, price, access, choices):
        result.append(dict(Materials=mats, Needs=needs, Output=output, Price=price, Access=access, Choices=choices))
    for element, (fragment, rune, essence) in enumerate(zip(
            [1000636,1000637,1000638,1000639,1001181,1001179,1001177],
            [1000640,1000641,1000642,1000643,1001182,1001180,1001178],
            [1001138,1001139,1001140,1001141,1001185,1001184,1001183])):
        add([fragment],[10],rune,20000,0,[1,element+1,1])
        add([rune],[5],essence,20000,0,[1,element+1,2])
    for choice,(special,output) in enumerate(zip([1001185,1001184,1001183],[1001186,1001189,1001188])):
        add([1001138,1001139,1001140,1001141,special],[5]*5,output,20000,0,[2,choice+1])
    for element in range(8):
        for stage,(need,price) in enumerate(zip([10,5,10],[30000,50000,100000])):
            material = 1001290+element+stage*8
            add([material],[need],material+8,price,1,[element+1,stage+1])
    require(len(result)==41 and len({r['Output'] for r in result})==41, 'Exact distinct recipe outputs')
    return result


def effective(path):
    rows = {}
    for row in renewal_records(ROOT,path): rows.setdefault(row['Id'],{}).update(row)
    return rows


def validate(prepare=False):
    from biosphere_material_callback_audit import validate as gate, collect_evidence
    raw = {p:(ROOT/p).read_bytes() for p in (QUESTS,DEPTH,ACCESS)}
    before = {p:reconstruct_original(p,raw[p]) for p in (QUESTS,DEPTH)}
    # Explicit prepare-only never claims a callback validation or runtime PASS.
    callbacks = collect_evidence(ROOT) if prepare else gate(ROOT)
    all_items = effective('db/item_db.yml'); r = recipes()
    ids = sorted({i for row in r for i in row['Materials']+[row['Output']]})
    require(len(ids)==56, 'Exact 56 material identities')
    for i in ids:
        item = all_items[i]
        require(item['Type']=='Etc' and item['Weight'] in (1,10) and not item.get('Stack') and
                not any(item.get(k) for k in ('Script','EquipScript','UnEquipScript','Locations','Buy','Sell')) and
                not set(item.get('Flags',{}))-{'BuyingStore','DropEffect'}, 'Plain material invariant: '+str(i))
    require(sum(all_items[i]['Weight']==1 for i in ids)==23, 'Exact 23 light material identities')
    achievements = [row for row in effective('db/achievement_db.yml').values() if row.get('Group')=='Get_Item']
    require(len(achievements)==7, 'Exact seven native Get_Item conditions')
    print('MATERIAL_SOURCE_OK: original full-file reconstruction; 41 recipes; 56 exact items; '+
          ('UNVALIDATED build preparation only' if prepare else 'mandatory callback gate PASS'),flush=True)
    return raw,before,r,all_items,achievements,callbacks


def check_output(result, mode):
    result.check_returncode()
    text = re.sub(r'\x1b\[[0-9;]*m','',result.stdout+'\n'+result.stderr)
    require(text.count('Memory manager: No memory leaks found.')==1,'Explicit clean allocator teardown')
    require(not re.search(r'AddressSanitizer|UndefinedBehaviorSanitizer|runtime error:|(?:invalid|double) free|'
                          r'Memory manager:(?! No memory leaks found\.)',text,re.I),'Sanitizer/allocator diagnostic')
    marker = 'MATERIAL_OLD_FAILURES_OK' if mode=='original' else 'MATERIAL_NATIVE_OK'
    require(text.count(marker)==1,'Native coverage marker missing')
    residual = text
    if mode=='original':
        require(text.count("script_set_reg: failed to set param 'Zeny' to -1.")==41,'All 41 original stale-Zeny failures')
        residual = residual.replace("[Error]: script_set_reg: failed to set param 'Zeny' to -1.",'')
        require(text.count('buildin_delitem: failed to delete 5 items')==12,'Twelve later-water-ingredient failures')
        residual = re.sub(r'\[Error\]: buildin_delitem: failed to delete 5 items \(AID=99000001 item_id=\d+\)\.','',residual)
        require(text.count("[Warning]: Script command 'delitem' returned failure.")==12,'Twelve exact deletion failure warnings')
        residual = residual.replace("[Warning]: Script command 'delitem' returned failure.",'')
        require(text.count('buildin_getitem: Failed to add the item to player.')==3,'Three original metadata-capacity output failures')
        residual = residual.replace('[Error]: buildin_getitem: Failed to add the item to player.','')
        require(text.count("[Warning]: Script command 'getitem' returned failure.")==3,'Three exact output failure warnings')
        residual = residual.replace("[Warning]: Script command 'getitem' returned failure.",'')
    require(not re.search(r'\[(?:error|warning)\]|script_set_reg:|buildin_delitem: failed|fatal error',residual,re.I),
            'Unexpected native diagnostic, including negative child')
    return text


def output_controls():
    base='MATERIAL_NATIVE_OK cases=1 assertions=1\n[Info]: Memory manager: No memory leaks found.\n'
    check_output(subprocess.CompletedProcess([],0,base,''),'candidate')
    samples=[base.replace('Memory manager: No memory leaks found.','')]+[base+'\n'+x for x in
        ('[Error]: unexpected','[Warning]: unexpected','AddressSanitizer: bad','runtime error: overflow','Memory manager: invalid pointer','double free')]
    for text in samples:
        try: check_output(subprocess.CompletedProcess([],0,text,''),'candidate')
        except AssertionError: pass
        else: raise AssertionError('Zero-exit diagnostic accepted')
    print('MATERIAL_OUTPUT_GUARD_OK: 7 zero-exit negative controls',flush=True)


def native(build, inputs, reuse=False, prepare=False):
    build=build.resolve(); require(build!=ROOT and ROOT not in build.parents,'Artifacts must be outside repository')
    build.mkdir(parents=True,exist_ok=True)
    raw,before,r,all_items,achievements,callbacks=inputs
    for name,path in [('quests',QUESTS),('depth',DEPTH)]:
        (build/(name+'-before.txt')).write_bytes(before[path])
        (build/(name+'-after.txt')).write_bytes(normal(raw[path]))
    (build/'access.txt').write_bytes(normal(raw[ACCESS]))
    def emit(name,rows): (build/(name+'.yml')).write_text(yaml.safe_dump({'Body':rows},sort_keys=False),encoding='utf-8')
    emit('recipes',r);emit('achievements',achievements)
    # The callback fixture adapter below is intentionally schema checked, not
    # a silent fallback to an empty service-map condition list.
    prepare_callbacks(build,callbacks,all_items,emit)
    prefix=(ROOT/PREFIX).read_text().split('extern "C" int __wrap_main(',1)[0]
    prefix=replace_once(prefix,'extern "C" npc_data* npc_lookup(int32){return nullptr;}','')
    prefix=replace_once(prefix,'extern "C" void crown_log(const map_session_data*,e_log_pick_type,int32,const item*){}','')
    # Preserve severity in the inherited error recorder, including any error
    # emitted outside a case or during teardown. Never modify its shared source.
    prefix=replace_once(prefix,'extern "C" void error(const char* f,...){++errors;va_list a;va_start(a,f);std::vfprintf(stderr,f,a);va_end(a);}',
        'extern "C" void error(const char* f,...){++errors;std::fputs("[Error]: ",stderr);va_list a;va_start(a,f);std::vfprintf(stderr,f,a);va_end(a);}')
    combined=build/'combined_material_test.cpp'
    combined.write_text(prefix+'\n'+(ROOT/DRIVER).read_text())
    production=['src/map/pc.cpp','src/map/script.cpp','src/map/itemdb.cpp','src/map/clif.cpp',
                'src/map/achievement.cpp','src/map/quest.cpp','src/common/malloc.cpp']
    tracked=production+[PREFIX,DRIVER,'tools/ci/biosphere_material_transaction_test.py',
                        'tools/ci/biosphere_crown_transaction_test.py','tools/ci/biosphere_material_callback_audit.py',
                        'tools/ci/biosphere_regression_scope.py']
    headers=sorted(p.relative_to(ROOT).as_posix() for tree in ('src','3rdparty') for p in (ROOT/tree).rglob('*')
                   if p.is_file() and p.suffix in ('.h','.hpp','.inl','.tcc'))
    tracked+=headers
    hashes={p:sha((ROOT/p).read_bytes()) for p in tracked}
    header_sha=sha(json.dumps({p:hashes[p] for p in headers},sort_keys=True).encode())
    executable=build/'biosphere_material_transaction_test'
    if reuse:
        require(json.loads((build/'build.json').read_text())==hashes,'Retained binary source mismatch')
    else:
        san=['-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer']
        flags=['g++','-std=c++17','-O0','-g','-DPACKETVER=20260219','-fno-strict-aliasing']+san
        flags+=['-I'+p for p in ('src','3rdparty/libconfig','3rdparty/rapidyaml/src','3rdparty/rapidyaml/ext/c4core/src','3rdparty/json/include','/usr/include/mysql')]
        def compile_one(source):
            out=build/(Path(source).stem+'.o')
            sidecar=out.with_suffix('.source.json')
            object_source={'source':str(source),'sha256':sha(Path(source).read_bytes()),'flags':flags,'headers_sha256':header_sha}
            if out.is_file() and sidecar.is_file() and json.loads(sidecar.read_text())==object_source:print('Reuse exact-source sanitizer object '+str(source),flush=True)
            else:
                print('Fresh compile '+str(source),flush=True)
                subprocess.run(flags+['-c',str(source),'-o',str(out)],cwd=ROOT,check=True)
                require(object_source['sha256']==sha(Path(source).read_bytes()),'Source changed during object compile')
                sidecar.write_text(json.dumps(object_source,indent=2)+'\n')
            return out
        with ThreadPoolExecutor(max_workers=2) as pool: fresh=list(pool.map(compile_one,production+[str(combined)]))
        require(hashes=={p:sha((ROOT/p).read_bytes()) for p in tracked},'Source changed during compile')
        excluded={p.name for p in fresh};objects=sorted(p for p in (ROOT/'src/map/obj').rglob('*.o') if p.name not in excluded)
        libs=[ROOT/p for p in ('src/common/obj/common.a','3rdparty/libconfig/obj/libconfig.a','3rdparty/rapidyaml/obj/ryml.a')]
        require(objects and all(p.is_file() for p in libs),'Native support objects missing')
        wrappers=[w for w in OLD_WRAPPERS if w not in ('_Z17pc_show_questinfoP16map_session_data',
            '_Z28achievement_update_objectiveP16map_session_data19e_achievement_grouphz')]
        wrappers+=['_Z16clif_scriptinputR16map_session_dataj','_Z9map_id2bli',
            '_Z8log_zenyRK16map_session_data15e_log_pick_typeji','_Z14pc_setregistryP16map_session_datall',
            '_Z27achievement_check_conditionP11script_codeP16map_session_data',
            '_Z21clif_quest_show_eventPK16map_session_dataPK10block_list17e_questinfo_types21e_questinfo_markcolor']
        command=['g++']+san+['-o',str(executable)]+[str(p) for p in fresh+objects+libs]
        command+=['-Wl,--wrap='+w for w in wrappers]+['-lz','-ldl','-lmysqlclient','-lzstd','-lssl','-lcrypto','-lresolv','-lm']
        subprocess.run(command,cwd=ROOT,check=True)
        (build/'build.json').write_text(json.dumps(hashes,indent=2)+'\n')
    if prepare:
        print('MATERIAL_BUILD_ONLY: unvalidated callback preparation; no runtime PASS claimed',flush=True)
        return
    outputs={}
    for mode in ('candidate','original'):
        result=subprocess.run([str(executable),str(build),mode],cwd=ROOT,capture_output=True,text=True,timeout=180)
        (build/(mode+'.stdout.txt')).write_text(result.stdout);(build/(mode+'.stderr.txt')).write_text(result.stderr)
        print(result.stdout,end='');print(result.stderr,end='',file=sys.stderr);outputs[mode]=check_output(result,mode)
    from biosphere_material_callback_audit import validate as gate
    require(gate(ROOT)==callbacks,'Callback closure changed during proof')
    require(raw=={p:(ROOT/p).read_bytes() for p in raw},'Raw runtime source changed during proof')
    require(hashes=={p:sha((ROOT/p).read_bytes()) for p in tracked},'Compiled source changed during proof')
    counts=re.search(r'MATERIAL_NATIVE_OK cases=(\d+) assertions=(\d+) nested_checks=(\d+)',outputs['candidate']);require(counts,'Native coverage counts')
    old_counts=re.search(r'MATERIAL_OLD_FAILURES_OK cases=(\d+) assertions=(\d+) nested_checks=(\d+)',outputs['original'])
    require(old_counts and int(old_counts[1])==100,'All 100 genuine original failure controls executed')
    receipt={'result':'PASS','cases':int(counts[1]),'assertions':int(counts[2]),'source_hashes':hashes,
        'nested_condition_checks':int(counts[3]),
        'original_failure_controls':{'cases':int(old_counts[1]),'assertions':int(old_counts[2]),'nested_condition_checks':int(old_counts[3])},
        'mandatory_callback_gate':'PASS before and after; identical complete manifest',
        'runtime_raw_sha256':{p:sha(d) for p,d in raw.items()},'original_sha256':{p:sha(d) for p,d in before.items()},
        'executable_sha256':sha(executable.read_bytes()),'callback_manifest_sha256':sha(json.dumps(callbacks,sort_keys=True).encode()),
        'fixture_sha256':{p.name:sha(p.read_bytes()) for p in sorted(build.iterdir()) if p.suffix=='.yml' or p.name in ('quests-before.txt','depth-before.txt','quests-after.txt','depth-after.txt','access.txt','combined_material_test.cpp')},
        'native_output_sha256':{mode:{stream:sha((build/(mode+'.'+stream+'.txt')).read_bytes()) for stream in ('stdout','stderr')} for mode in outputs},
        'asan_ubsan':True,'network':'kernel denied','boundary':'Actual NPC/input/inventory/Zeny/QuestInfo/achievement; explicit world lookup, transport, registry and log persistence doubles'}
    (build/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')


def prepare_callbacks(build, callbacks, all_items, emit):
    regs=callbacks['questinfo']['registrations'];owners=callbacks['questinfo']['owners']
    require(len(regs)==33 and len(owners)==19,'Exact nonempty ba_in01 fixture inventory')
    groups={}
    for row in regs:groups.setdefault(row['npc'],[]).append(row)
    require(len(groups)==19,'Nineteen distinct native QuestInfo owners')
    native=[];probes=[]
    for owner,rows in groups.items():
        native.append({'Source':rows[0]['path'],'OwnerIndex':len(native),'Script':'{\n'+'\n'.join(r['statement'] for r in rows)+'\nend;\n}',
                       'Conditions':[r['condition'] for r in rows]})
        for row in rows:
            # Explicit source-derived TRUE states for the currently restricted
            # comparison expressions; all-false baseline is independently run.
            expression=row['condition'].split('||',1)[0]
            quests={};variables={};items={}
            for ident,value in re.findall(r'isbegin_quest\((\d+)\)\s*==\s*(-?\d+)',expression):
                if int(value):quests[int(ident)]={'Id':int(ident),'State':int(value),'Expired':False,'CompleteCounts':False}
            for ident,mode,value in re.findall(r'checkquest\((\d+),(HUNTING|PLAYTIME)\)\s*==\s*(-?\d+)',expression):
                value=int(value)
                if value!=-1:quests[int(ident)]={'Id':int(ident),'State':1,'Expired':value==(1 if mode=='HUNTING' else 2),'CompleteCounts':mode=='HUNTING' and value==2}
            for name,value in re.findall(r'\b(BaseLevel|ep17_2_main|ep17_2_bath)\s*(?:==|>=)\s*(\d+)',expression):variables[name]=int(value)
            for ident,value in re.findall(r'countitem\((\d+)\)\s*>=\s*(\d+)',expression):items[int(ident)]=int(value)
            probes.append({'Condition':row['condition'],'Quests':list(quests.values()),
                           'Variables':[{'Name':n,'Value':v} for n,v in variables.items()],
                           'Items':[{'Id':i,'Amount':v} for i,v in items.items()]})
    emit('questinfo',native);emit('qi-probes',probes)
    quest_rows=callbacks['quests']['referenced_ordered_records']
    require(len(callbacks['quests']['referenced_records'])==36 and len(quest_rows)==38,'Exact quest definition/override dependency closure')
    emit('quests',quest_rows)
    mob_names={t['Mob'] for q in quest_rows for key in ('Targets','Drops') for t in q.get(key,[]) if 'Mob' in t}
    mobs={r['AegisName']:r for r in effective('db/mob_db.yml').values() if r.get('AegisName') in mob_names}
    require(set(mobs)==mob_names,'Actual quest-target monster identity dependency missing')
    emit('quest-mob-identities',[{'Id':mobs[n]['Id'],'AegisName':n} for n in sorted(mobs)])
    ids={i for row in recipes() for i in row['Materials']+[row['Output']]}|{7110,7326,1000226}
    by_name={r['AegisName']:r['Id'] for r in all_items.values() if 'AegisName' in r}
    ids|={by_name[d['Item']] for q in quest_rows for d in q.get('Drops',[]) if 'Item' in d}
    emit('items',[all_items[i] for i in sorted(ids)])


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--native-build-dir',type=Path)
    parser.add_argument('--reuse-build',action='store_true')
    parser.add_argument('--prepare-only',action='store_true',help='Compile only; callback collection is explicitly unvalidated, never runtime PASS')
    args=parser.parse_args();output_controls();inputs=validate(args.prepare_only)
    if args.native_build_dir:native(args.native_build_dir,inputs,args.reuse_build,args.prepare_only)
