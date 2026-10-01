"""Execute legacy socket functions in the real script VM, without networking.

Requires current Linux map objects. Registry/packet boundaries use the existing
Biosphere harness; inventory and script builtins are the linked production code.
"""
import argparse
import hashlib
import json
import re
from pathlib import Path
import subprocess
import sys
import tempfile
import yaml
from audit_enchant_upgrades import renewal_records
from biosphere_crown_transaction_test import WRAPPERS

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools'))
from audit_item_acquisition import strip_comments


def run(args,work):
    work.mkdir(parents=True,exist_ok=True)
    sources=[args.source] if args.source else [ROOT/'npc/merchants'/name for name in ('socket_enchant.txt','socket_enchant2.txt')]
    recipes={}
    wanted={2307,2308,999,512,1201,969,5022,5353}
    for source in sources:
        name='Func_Socket2' if source.name=='socket_enchant2.txt' else 'Func_Socket'
        rows=[]
        for match in re.finditer(r'callfunc "'+name+r'",([^;]+);',strip_comments(source.read_text())):
            values=[int(value.strip()) for value in match[1].split(',')]
            assert len(values) in (7,9),values
            wanted.update(values[:2]);wanted.add(values[5])
            if len(values)==9:wanted.add(values[7])
            rows.append(values)
        assert rows,source
        recipes[name]=rows
    (work/'recipes.yml').write_text(yaml.safe_dump(recipes))
    helpers=ROOT/'npc/other/Socket_Functions.txt'
    (work/'socket_helpers.txt').write_text(helpers.read_text())
    items={}
    for row in renewal_records(ROOT,'db/item_db.yml'):
        if row['Id'] in wanted:items.setdefault(row['Id'],{}).update(row)
    assert wanted<=items.keys(),wanted-items.keys()
    (work/'items.yml').write_text(yaml.safe_dump({'Header':{'Type':'ITEM_DB','Version':3},'Body':list(items.values())},sort_keys=False))
    prefix=(ROOT/'tools/ci/biosphere_crown_transaction_test.cpp').read_text().split('extern "C" int __wrap_main(',1)[0]
    prefix=prefix.replace('check(attached->status.zeny==7654321,"no Zeny charge");','')
    prefix=prefix.replace('++pause<30','++pause<64')
    original='nums[key]=value;return true;}'
    assert original in prefix
    prefix=prefix.replace(original,'nums[key]=value;if(script_getvaridx(key))script_array_update(&attached->regs,key,value==0);return true;}',1)
    driver=work/'driver.cpp';driver.write_text(prefix+Path(__file__).with_suffix('.cpp').read_text())
    objects=list((ROOT/'src/map/obj').rglob('*.o'))
    assert objects,'Build map objects first'
    libraries=[ROOT/p for p in ('src/common/obj/common.a','3rdparty/libconfig/obj/libconfig.a','3rdparty/rapidyaml/obj/ryml.a')]
    includes=('src','3rdparty/libconfig','3rdparty/rapidyaml/src','3rdparty/rapidyaml/ext/c4core/src','3rdparty/json/include','/usr/include/mysql')
    binary=work/'socket-test'
    command=['g++','-std=c++17','-O0','-DPACKETVER=20260219']+['-I'+p for p in includes]
    command += [str(driver)]+[str(p) for p in objects+libraries]+['-Wl,--wrap='+w for w in (*WRAPPERS,'_Z9map_id2bli')]
    command += ['-lz','-ldl','-lmysqlclient','-l:libzstd.so.1','-lssl','-lcrypto','-lresolv','-lm','-o',str(binary)]
    bound=objects+libraries+sources+[helpers,ROOT/'npc/scripts_athena.conf',Path(__file__),Path(__file__).with_suffix('.cpp'),ROOT/'tools/audit_item_acquisition.py',ROOT/'tools/ci/biosphere_crown_transaction_test.cpp',work/'items.yml',work/'recipes.yml',work/'socket_helpers.txt']
    def identity():return {str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in bound}
    hashes=identity()
    report={'passed':False,'inputs_sha256':hashes,'network':'kernel denied',
            'scope':'Real script VM and inventory builtins; existing map link objects; packet, registry and visual world boundaries are doubles.', 'cases':[]}
    (work/'receipt.json').write_text(json.dumps(report,indent=2))
    subprocess.run(command,cwd=ROOT,check=True)
    for source in sources:
        name='Func_Socket2' if source.name=='socket_enchant2.txt' else 'Func_Socket'
        result=subprocess.run([str(binary),str(work),str(source.resolve()),name],cwd=ROOT,capture_output=True,text=True)
        output=result.stdout+result.stderr;(work/(name+'.log')).write_text(output);print(output,end='')
        result.check_returncode()
        assert 'SOCKET_TRANSACTION_OK' in output and 'Memory manager: No memory leaks found.' in output
        report['cases'].append({'function':name,'log_sha256':hashlib.sha256(output.encode()).hexdigest(),'passed':True})
    assert identity()==hashes,'Inputs changed during socket validation'
    report['passed']=True
    (work/'receipt.json').write_text(json.dumps(report,indent=2))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source',type=Path)
    parser.add_argument('--build-dir',type=Path)
    args=parser.parse_args()
    if args.build_dir:run(args,args.build_dir.resolve())
    else:
        with tempfile.TemporaryDirectory(prefix='socket-transaction-') as directory:run(args,Path(directory))
