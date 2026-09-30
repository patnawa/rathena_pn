"""Real damage-lab script VM and identity builtin; registry/transport are doubles.
Requires built Linux map objects. Relog fixture recreates player retaining only
persistent character registry data; actual char SQL persistence is a release test.
"""
import argparse
import re
from pathlib import Path
import subprocess
import tempfile
from biosphere_crown_transaction_test import WRAPPERS

def main():
 p=argparse.ArgumentParser();p.add_argument('--root',type=Path);a=p.parse_args()
 root=(a.root or Path(__file__).resolve().parents[2]).resolve()
 source=(root/'npc/custom/quality_services.txt').read_text()
 for name in ('PN_LabSave','PN_LabCompatible','PN_LabHistory'):
  assert re.search(r'^function\tscript\t'+name+r'\t\{',source,re.M), 'NPC loader requires tab before function body: '+name
 prefix=(root/'tools/ci/biosphere_crown_transaction_test.cpp').read_text().split('extern "C" int __wrap_main(',1)[0]
 prefix=prefix.replace('nums[key]=value;return true;}','nums[key]=value;if(script_getvaridx(key))script_array_update(&attached->regs,key,value==0);return true;}',1)
 with tempfile.TemporaryDirectory(prefix='lab-history-') as temp:
  work=Path(temp);driver=work/'driver.cpp';binary=work/'test'
  driver.write_text(prefix+(root/'tools/ci/lab_history_test.cpp').read_text())
  objects=list((root/'src/map/obj').rglob('*.o'))
  if not objects:raise SystemExit('Build map objects first')
  libraries=[root/x for x in ('src/common/obj/common.a','3rdparty/libconfig/obj/libconfig.a','3rdparty/rapidyaml/obj/ryml.a')]
  includes=['src','3rdparty/libconfig','3rdparty/rapidyaml/src','3rdparty/rapidyaml/ext/c4core/src','3rdparty/json/include','/usr/include/mysql']
  cmd=['g++','-std=c++17','-O0','-DPACKETVER=20260219']+['-I'+x for x in includes]+[str(driver)]+[str(x) for x in objects+libraries]+['-Wl,--wrap='+x for x in (*WRAPPERS,'_Z14pc_setregistryP16map_session_datall','_Z18pc_setregistry_strP16map_session_datalPKc','_Z19pc_readregistry_strPK16map_session_datal')]+['-lz','-ldl','-lmysqlclient','-l:libzstd.so.1','-lssl','-lcrypto','-lresolv','-lm','-o',str(binary)]
  subprocess.run(cmd,cwd=root,check=True)
  run=subprocess.run([str(binary),str(root/'npc/custom/quality_services.txt')],cwd=root,capture_output=True,text=True)
  output=run.stdout+run.stderr;print(output,end='');run.check_returncode()
  assert 'LAB_HISTORY_OK' in output and 'Memory manager: No memory leaks found.' in output
  assert 'TEST FAIL' not in output and '[Error]' not in output
if __name__=='__main__':main()
