"""Real damage-lab script VM and identity builtin; registry/transport are doubles.
Requires built Linux map objects. Relog fixture recreates player retaining only
persistent character registry data; actual char SQL persistence is a release test.
"""
import argparse
import hashlib
import json
import re
from pathlib import Path
import subprocess
import tempfile
import yaml
from audit_enchant_upgrades import renewal_records
from biosphere_crown_transaction_test import WRAPPERS

def main():
 p=argparse.ArgumentParser();p.add_argument('--root',type=Path);p.add_argument('--source',type=Path);p.add_argument('--target-red',action='store_true');p.add_argument('--evidence',type=Path);a=p.parse_args()
 root=(a.root or Path(__file__).resolve().parents[2]).resolve()
 npc=(a.source or root/'npc/custom/quality_services.txt').resolve()
 if a.target_red and not a.source:p.error('--target-red requires an explicit pre-fix --source')
 source=npc.read_text()
 for name in ('PN_LabSave','PN_LabCompatible','PN_LabHistory'):
  assert re.search(r'^function\tscript\t'+name+r'\t\{',source,re.M), 'NPC loader requires tab before function body: '+name
 prefix=(root/'tools/ci/biosphere_crown_transaction_test.cpp').read_text().split('extern "C" int __wrap_main(',1)[0]
 prefix=prefix.replace('nums[key]=value;return true;}','nums[key]=value;if(script_getvaridx(key))script_array_update(&attached->regs,key,value==0);return true;}',1)
 with tempfile.TemporaryDirectory(prefix='lab-history-') as temp:
  work=Path(temp);driver=work/'driver.cpp';binary=work/'test'
  costs={c['Item'] for row in renewal_records(root,'db/skill_db.yml') for c in row.get('Requires',{}).get('ItemCost',[])}
  costs.update(n for row in renewal_records(root,'db/skill_db.yml') for n in row.get('Requires',{}).get('Equipment',{}))
  metadata={}
  for row in renewal_records(root,'db/item_db.yml'):
   if row.get('AegisName') in costs:metadata.setdefault(row['Id'],{}).update(row)
  (work/'items.yml').write_text(yaml.safe_dump({'Body':list(metadata.values())}))
  driver.write_text(prefix+(root/'tools/ci/lab_history_test.cpp').read_text())
  objects=list((root/'src/map/obj').rglob('*.o'))
  if not objects:raise SystemExit('Build map objects first')
  libraries=[root/x for x in ('src/common/obj/common.a','3rdparty/libconfig/obj/libconfig.a','3rdparty/rapidyaml/obj/ryml.a')]
  includes=['src','3rdparty/libconfig','3rdparty/rapidyaml/src','3rdparty/rapidyaml/ext/c4core/src','3rdparty/json/include','/usr/include/mysql']
  cmd=['g++','-std=c++17','-O0','-DPACKETVER=20260219']+['-I'+x for x in includes]+[str(driver)]+[str(x) for x in objects+libraries]+['-Wl,--wrap='+x for x in (*WRAPPERS,'_Z14pc_setregistryP16map_session_datall','_Z18pc_setregistry_strP16map_session_datalPKc','_Z19pc_readregistry_strPK16map_session_datal','_Z9map_id2bli','_Z12unit_refreshPK10block_listb')]+['-lz','-ldl','-lmysqlclient','-l:libzstd.so.1','-lssl','-lcrypto','-lresolv','-lm','-o',str(binary)]
  subprocess.run(cmd,cwd=root,check=True)
  run=subprocess.run([str(binary),str(npc),str(work/'items.yml')]+(['red'] if a.target_red else []),cwd=root,capture_output=True,text=True)
  output=run.stdout+run.stderr;print(output,end='')
  if a.evidence:
   a.evidence.mkdir(parents=True,exist_ok=True);(a.evidence/'native.log').write_text(output)
   inputs=[npc,root/'tools/ci/lab_history_test.py',root/'tools/ci/lab_history_test.cpp']+objects+libraries
   (a.evidence/'binding.json').write_text(json.dumps({str(x):hashlib.sha256(x.read_bytes()).hexdigest() for x in inputs},indent=2)+'\n')
  run.check_returncode()
  assert 'LAB_HISTORY_OK' in output and 'Memory manager: No memory leaks found.' in output
  assert 'TEST FAIL' not in output and '[Error]' not in output
if __name__=='__main__':main()
