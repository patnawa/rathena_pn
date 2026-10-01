"""Actual compiler metadata regression, live item/group/pet DB fixtures."""
from pathlib import Path
import subprocess,tempfile,yaml
from biosphere_crown_transaction_test import WRAPPERS
from audit_enchant_upgrades import renewal_records
ROOT=Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix='pn-item-use-metadata-') as temp:
 out=Path(temp);items={}
 for row in renewal_records(ROOT,'db/item_db.yml'):
  if row['Id'] in {501,502,503,4001,4002}:items.setdefault(row['Id'],{}).update(row)
 (out/'items.yml').write_text(yaml.safe_dump({'Body':list(items.values())},sort_keys=False))
 prefix=(ROOT/'tools/ci/biosphere_crown_transaction_test.cpp').read_text().split('extern "C" int __wrap_main(',1)[0]
 cpp=out/'test.cpp';cpp.write_text(prefix+(ROOT/'tools/ci/item_use_pet_native_test.cpp').read_text())
 flags=['g++','-std=c++17','-O0','-fsanitize=undefined','-fno-sanitize-recover=all','-DPACKETVER=20260219']
 flags+=['-I'+str(ROOT/p) for p in ('src','3rdparty/libconfig','3rdparty/rapidyaml/src','3rdparty/rapidyaml/ext/c4core/src','3rdparty/json/include')]+['-I/usr/include/mysql']
 objects=list((ROOT/'src/map/obj').rglob('*.o'))
 wrappers=list(WRAPPERS)+['_Z14pn_shop_submitR16map_session_dataSt10shared_ptrIN7pn_shop6CommitEESt6vectorINS2_5EventESaIS6_EEj','_Z9pc_setposP16map_session_datatii8clr_type','_Z9map_id2mdi','_Z16unit_remove_map_P10block_list8clr_typePKciS3_','_Z21status_percent_changeP10block_listS0_aaah','_Z17clif_pet_rouletteRK16map_session_datab','_Z35achievement_update_objective_valuesP16map_session_data19e_achievement_groupRKSt6vectorIiSaIiEE']
 libs=[ROOT/p for p in ('src/common/obj/common.a','3rdparty/libconfig/obj/libconfig.a','3rdparty/rapidyaml/obj/ryml.a')]
 exe=out/'test';subprocess.run(flags+[str(cpp)]+[str(p) for p in objects+libs]+['-Wl,--wrap='+x for x in wrappers]+['-lz','-ldl','-lmysqlclient','-l:libzstd.so.1','-lssl','-lcrypto','-lresolv','-lm','-o',str(exe)],cwd=ROOT,check=True)
 result=subprocess.run([str(exe),str(out)],cwd=ROOT,text=True,capture_output=True,timeout=60)
 print(result.stdout);print(result.stderr)
 if result.returncode or 'Memory leaks found' in result.stdout+result.stderr or 'ITEM_USE_PET cases=18 failures=0' not in result.stdout:raise SystemExit(1)


