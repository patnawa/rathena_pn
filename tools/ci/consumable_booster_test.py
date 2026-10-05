"""Execute the loaded booster scripts in the real offline script VM under UBSan."""
import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
from pathlib import Path
import re
import subprocess
import shutil
import tempfile
from biosphere_crown_transaction_test import WRAPPERS

ROOT=Path(__file__).resolve().parents[2]

def run(build, scripts):
    build.mkdir(parents=True,exist_ok=True)
    prefix=(ROOT/'tools/ci/biosphere_crown_transaction_test.cpp').read_text().split('extern "C" int __wrap_main(',1)[0]
    status=(ROOT/'src/map/status.cpp').read_text()
    start=status.index('static uint16 status_calc_speed(block_list *bl,')
    end=status.index('\n}',start)+2
    speed=status[start:end].replace('status_calc_speed(', 'audited_status_calc_speed(',1)
    symbols=subprocess.check_output(['nm',str(ROOT/'src/map/obj/status.o')],text=True)
    status_symbol=re.search(r' T (_Z15status_calc_bl_\S+)',symbols)[1]
    driver=build/'consumable_booster_driver.cpp'
    driver.write_text(prefix+'\n#include "common/utils.hpp"\n#include "map/status.hpp"\n#include "map/skill.hpp"\n#include "map/unit.hpp"\n'+speed+'\n'+(ROOT/'tools/ci/consumable_booster_test.cpp').read_text().replace('@STATUS_CALC_SYMBOL@',status_symbol))
    sanitize=['-fsanitize=undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer']
    flags=['g++','-std=c++17','-O0','-g','-DPACKETVER=20260219','-fno-strict-aliasing']+sanitize
    flags+=['-I'+p for p in ('src','3rdparty/libconfig','3rdparty/rapidyaml/src','3rdparty/rapidyaml/ext/c4core/src','3rdparty/json/include','/usr/include/mysql')]
    def compile_one(path):
        target=build/(path.stem+'.o');stamp=target.with_suffix('.sha256')
        fingerprint=hashlib.sha256(path.read_bytes()+repr(flags).encode()).hexdigest()
        if not target.exists() or not stamp.exists() or stamp.read_text()!=fingerprint:
            print('Compiling '+str(path),flush=True)
            subprocess.run(flags+['-c',str(path),'-o',str(target)],cwd=ROOT,check=True)
            stamp.write_text(fingerprint)
        return target
    with ThreadPoolExecutor(max_workers=2) as pool:
        objects=list(pool.map(compile_one,[ROOT/'src/map/script.cpp',ROOT/'src/map/pc.cpp',ROOT/'src/common/malloc.cpp',driver]))
    support=sorted(p for p in (ROOT/'src/map/obj').rglob('*.o') if p.name not in ('script.o','pc.o'))
    libraries=[ROOT/p for p in ('src/common/obj/common.a','3rdparty/libconfig/obj/libconfig.a','3rdparty/rapidyaml/obj/ryml.a')]
    wrappers=(*WRAPPERS,status_symbol)
    binary=build/'consumable_booster_test'
    subprocess.run(['g++']+sanitize+['-o',str(binary)]+[str(p) for p in objects+support+libraries]+['-Wl,--wrap='+n for n in wrappers]+['-lz','-ldl','-lmysqlclient','-lzstd','-lssl','-lcrypto','-lresolv','-lm'],cwd=ROOT,check=True)
    result=subprocess.run([str(binary),str(scripts)],cwd=ROOT,capture_output=True,text=True,timeout=30)
    print(result.stdout,end='');print(result.stderr,end='');result.check_returncode()
    assert 'CONSUMABLE_BOOSTER_OK' in result.stdout
    assert not re.search(r'\[(?:Error|Warning)\]|runtime error:',result.stdout+result.stderr)
    for name,filename,change,signal in (
        ('missing-force','102803.txt',lambda text:'','consumable applies one effect'),
        ('missing-speed','102985.txt',lambda text:'','consumable applies one effect'),
        ('sp-cost-sign','power.txt',lambda text:text.replace('bonus bUseSPrate, -.@val3;','bonus bUseSPrate, .@val3;'),'Power Booster reduces SP cost'),
        ('speed-stacking','102985.txt',lambda text:text.replace('bSpeedRate','bSpeedAddRate'),'movement uses non-stacking haste'),
    ):
        with tempfile.TemporaryDirectory(prefix='consumable-mutant-') as directory:
            altered=Path(directory);shutil.copytree(scripts,altered,dirs_exist_ok=True)
            path=altered/filename;original=path.read_text();mutated=change(original)
            assert mutated!=original
            path.write_text(mutated)
            failed=subprocess.run([str(binary),str(altered)],cwd=ROOT,capture_output=True,text=True,timeout=30)
            assert failed.returncode!=0 and signal in failed.stderr,(name,failed.stdout,failed.stderr)
            print('REGRESSION_REJECTED: '+name,flush=True)

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--build-dir',type=Path,required=True);p.add_argument('--scripts',type=Path,required=True)
    a=p.parse_args();run(a.build_dir.resolve(),a.scripts.resolve())
