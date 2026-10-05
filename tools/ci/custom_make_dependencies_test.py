"""Check GNU make's actual input selection and custom implementation dependencies."""
from pathlib import Path
import re
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[2]
CASES={
 'map': {
  'obj/npc.o':('npc.cpp','../custom/shop_map.inc'),
  'obj-gen/npc.o':('npc.cpp','../custom/shop_map.inc'),
  'obj/script.o':('script.cpp','../custom/item_use_script.inc','../custom/lab_history.inc'),
  'obj-gen/script.o':('script.cpp','../custom/item_use_script.inc','../custom/lab_history.inc'),
  'obj/pc.o':('pc.cpp','../custom/item_use_map.inc','../custom/pet_floor.inc'),
  'obj-gen/pc.o':('pc.cpp','../custom/item_use_map.inc','../custom/pet_floor.inc'),
  'obj/intif.o':('intif.cpp','../custom/shop_inter.inc'),
  'obj-gen/intif.o':('intif.cpp','../custom/shop_inter.inc'),
  'obj/atcommand.o':('atcommand.cpp','../custom/atcommand.inc'),
  'obj-gen/atcommand.o':('atcommand.cpp','../custom/atcommand.inc'),
  'obj/battle.o':('battle.cpp','../custom/battle_config_init.inc'),
  'obj-gen/battle.o':('battle.cpp','../custom/battle_config_init.inc'),
 },
 'char': {
  'obj/int_storage.o':('int_storage.cpp','../custom/shop_sql.inc','../custom/mail_asset_sql.inc','../custom/point_asset_sql.inc','../custom/global_point.hpp','../custom/global_point_char.inc','../custom/pet_entitlement.hpp'),
  'obj/int_pet.o':('int_pet.cpp','../custom/pet_entitlement_sql.inc','../custom/pet_entitlement.hpp'),
  'obj/char_logif.o':('char_logif.cpp','../custom/global_point.hpp'),
  'obj/inter.o':('inter.cpp','../custom/global_point.hpp'),
 },
 'login':{target:(target[4:-2]+'.cpp','../custom/global_point.hpp','../custom/shop_commit.hpp','../custom/pet_entitlement.hpp') for target in ('obj/account.o','obj/login.o','obj/loginchrif.o')}
}
with tempfile.TemporaryDirectory(prefix='pn-make-deps-') as directory:
 checks=0
 for component,targets in CASES.items():
  source=(ROOT/'src'/component/'Makefile.in').read_text()
  source=re.sub(r'@([A-Za-z_][A-Za-z_0-9]*)@',lambda m:'echo PN_COMPILE' if m[1]=='CXX' else '',source)
  makefile=Path(directory)/(component+'.mk');makefile.write_text(source)
  command=['make','--no-print-directory','-Bn','-f',str(makefile),*targets]
  result=subprocess.run(command,cwd=ROOT/'src'/component,text=True,capture_output=True,check=True)
  compile_lines=[line for line in result.stdout.splitlines() if line.startswith('echo PN_COMPILE ')]
  assert len(compile_lines)==len(targets),(component,compile_lines)
  compiled={line.split()[line.split().index('-o')+1]:line.split()[-1] for line in compile_lines}
  database=subprocess.run(['make','--no-print-directory','-np','-f',str(makefile),*targets],cwd=ROOT/'src'/component,text=True,capture_output=True,check=True).stdout
  rules={line.split(':',1)[0]:line.split(':',1)[1].split() for line in database.splitlines() if ':' in line and line.split(':',1)[0] in targets}
  for target,required in targets.items():
   assert compiled[target]==required[0],(component,target,compiled[target])
   for dependency in required:assert dependency in rules[target],(component,target,dependency)
   checks+=1
 print('CUSTOM_MAKE_DEPENDENCIES_PASS targets='+str(checks)+'; GNU make dry run and resolved prerequisite database')
