"""Exact package packet handler + actual catalog/planner, explicit transport/commit boundary.
No SQL or sockets: SQL asset atomicity is covered by the real database suite.
"""
from pathlib import Path
import subprocess,tempfile,yaml
from biosphere_crown_transaction_test import WRAPPERS
from audit_enchant_upgrades import renewal_records
ROOT=Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix='pn-package-pet-') as temp:
 out=Path(temp)
 items={}
 for row in renewal_records(ROOT,'db/item_db.yml'):
  if row['Id'] in {501,502,503}:items.setdefault(row['Id'],{}).update(row)
 (out/'items.yml').write_text(yaml.safe_dump({'Body':list(items.values())},sort_keys=False))
 prefix=(ROOT/'tools/ci/biosphere_crown_transaction_test.cpp').read_text().split('extern "C" int __wrap_main(',1)[0]
 source=(ROOT/'src/map/clif.cpp').read_text()
 source=source[source.index('void clif_parse_itempackage_select('):source.index('void clif_broadcast_refine_result(')]
 source=source.replace('clif_parse_itempackage_select(', 'audit_package(',1).replace('RFIFOP( fd, 0 )','&packet').replace('server_index( p->index )','(p->index - 2)').replace('pn_shop_begin(', 'fixture_begin(')
 planner=(ROOT/'src/custom/shop_map.inc').read_text()
 planner=planner[planner.index('bool pn_shop_plan_inventory('):planner.index('bool pn_shop_begin(')].replace('pn_shop_plan_inventory(','audit_plan(',1)
 mail=(ROOT/'src/map/mail.cpp').read_text()
 mail=mail[mail.index('bool pn_mail_getattachment_atomic('):mail.index('void mail_getattachment(')]
 mail=mail.replace('pn_mail_getattachment_atomic(', 'audit_mail(',1).replace('pn_mail_asset_result(', 'audit_mail_result(',1).replace('pn_shop_begin(', 'fixture_begin(').replace('clif_mail_getattachment(', 'fixture_mail_ack(')
 admin_source=(ROOT/'src/map/atcommand.cpp').read_text()
 admin=''
 for name in ('item','item2','makeegg'):
  start=admin_source.index('ACMD_FUNC('+name+')')
  end=admin_source.index('ACMD_FUNC(',start+1)
  fragment=admin_source[start:end]
  fragment=fragment[:fragment.rfind('/*')]
  fragment=fragment.replace('ACMD_FUNC('+name+')','int fixture_admin_'+name+'(const int32 fd,map_session_data* sd,const char* command,const char* message)',1)
  fragment=fragment.replace('atcommand_alias_db.checkAlias(command+1)','(command+1)').replace('parent_cmd','fixture_parent_cmd').replace('pn_shop_begin(', 'fixture_begin(').replace('pn_pet_grant(', 'fixture_pet_grant(')
  admin+=fragment+'\n'
 pet=(ROOT/'src/map/pet.cpp').read_text()
 pet=pet[pet.index('pn_pet_grant_result pn_pet_grant('):pet.index('/**\n * Make pet drop target.')]
 pet=pet.replace('pn_pet_grant(', 'fixture_pet_grant(',1).replace('pn_shop_begin(', 'fixture_begin(')
 driver='#include <custom/item_use.hpp>\n'+prefix+(ROOT/'tools/ci/package_pet_native_test.cpp').read_text().replace('// PRODUCTION',planner+'\n'+source+'\n'+mail+'\n'+pet+'\n'+admin)
 cpp=out/'test.cpp';cpp.write_text(driver)
 flags=['g++','-std=c++17','-O0','-fsanitize=undefined','-fno-sanitize-recover=all','-DPACKETVER=20260219']
 flags+=['-I'+str(ROOT/p) for p in ('src','3rdparty/libconfig','3rdparty/rapidyaml/src','3rdparty/rapidyaml/ext/c4core/src','3rdparty/json/include')]+['-I/usr/include/mysql']
 objects=list((ROOT/'src/map/obj').rglob('*.o'))
 libraries=[ROOT/p for p in ('src/common/obj/common.a','3rdparty/libconfig/obj/libconfig.a','3rdparty/rapidyaml/obj/ryml.a')]
 exe=out/'test'
 subprocess.run(flags+[str(cpp)]+[str(p) for p in objects+libraries]+['-Wl,--wrap='+x for x in WRAPPERS]+['-lz','-ldl','-lmysqlclient','-l:libzstd.so.1','-lssl','-lcrypto','-lresolv','-lm','-o',str(exe)],cwd=ROOT,check=True)
 result=subprocess.run([str(exe),str(out)],cwd=ROOT,text=True,capture_output=True,timeout=60)
 print(result.stdout);print(result.stderr)
 if result.returncode or 'Memory leaks found' in result.stdout+result.stderr or ('PACKAGE_PET cases=8 failures=0' not in result.stdout or 'MAIL_PET cases=8 failures=0' not in result.stdout or 'ADMIN_PET cases=7 failures=0' not in result.stdout):raise SystemExit(1)

