"""Offline SQL-failure reproduction; deliberately fails until durable correctness is repaired.

Uses exact purchase and persistence function bodies with real inventory/payment.
SQL Prepare, Execute and Query are deterministic in-memory boundaries. No DB.
Modes: success, prepare failure, execute/query failure, write applied but reply lost.
This is an unresolved diagnostic, not a release gate that accepts unsafe results.
"""
from pathlib import Path
import argparse,subprocess,tempfile,yaml
from biosphere_crown_transaction_test import WRAPPERS
from audit_enchant_upgrades import renewal_records
ROOT=Path(__file__).resolve().parents[2]
def run(out, source):
 items={}
 for row in renewal_records(ROOT,'db/item_db.yml'):
  if row['Id'] in {501,502,503}:items.setdefault(row['Id'],{}).update(row)
 (out/'items.yml').write_text(yaml.safe_dump({'Body':list(items.values())},sort_keys=False))
 prefix=(ROOT/'tools/ci/biosphere_crown_transaction_test.cpp').read_text().split('extern "C" int __wrap_main(',1)[0]
 src=source.read_text()
 market=src[src.index('void npc_market_tosql('):src.index('/**\n * Removes persistent NPC Market Data')]
 market=market.replace('npc_market_tosql(', 'audit_npc_market_tosql(',1).replace('SqlStmt stmt{ *mmysql_handle }','sql_fixture::Statement stmt{}').replace('SqlStmt_ShowDebug(stmt)','sql_fixture::debug(stmt)')
 buy=src[src.index('static int32 npc_buylist_sub(map_session_data* sd, std::vector'):src.index('/// npc_selllist for script-controlled shops')]
 buy=buy.replace('npc_buylist(', 'audit_npc_buylist(',1).replace('npc_checknear(sd,map_id2bl(sd->npc_shopid))','fixture_shop').replace('npc_market_tosql(', 'audit_npc_market_tosql(')
 barter=src[src.index('e_purchase_result npc_barter_purchase('):src.index('//Atempt to remove an npc')].replace('npc_barter_purchase(', 'audit_npc_barter_purchase(',1).replace('Sql_Query(', 'sql_fixture::query(').replace('Sql_ShowDebug(', 'sql_fixture::debug_sql(')
 driver=out/'native.cpp';driver.write_text(prefix+(ROOT/'tools/ci/shop_sql_failure_audit.cpp').read_text().replace('// FUNCTION','npc_data* fixture_shop=nullptr;\n'+market+buy+barter))
 flags=['g++','-std=c++17','-O0','-fsanitize=undefined','-fno-sanitize-recover=all','-DPACKETVER=20260219']
 flags+=['-I'+str(ROOT/p) for p in ('src','3rdparty/libconfig','3rdparty/rapidyaml/src','3rdparty/rapidyaml/ext/c4core/src','3rdparty/json/include')]+['-I/usr/include/mysql']
 objects=list((ROOT/'src/map/obj').rglob('*.o'))
 libraries=[ROOT/p for p in ('src/common/obj/common.a','3rdparty/libconfig/obj/libconfig.a','3rdparty/rapidyaml/obj/ryml.a')]
 binary=out/'sql-test'
 subprocess.run(flags+[str(driver)]+[str(p) for p in objects+libraries]+['-Wl,--wrap='+x for x in WRAPPERS]+['-lz','-ldl','-lmysqlclient','-l:libzstd.so.1','-lssl','-lcrypto','-lresolv','-lm','-o',str(binary)],cwd=ROOT,check=True)
 subprocess.run([str(binary),str(out)],cwd=ROOT,check=True,timeout=60)
if __name__=='__main__':
 parser=argparse.ArgumentParser(description=__doc__)
 parser.add_argument('--source',type=Path,default=ROOT/'src/map/npc.cpp')
 args=parser.parse_args()
 with tempfile.TemporaryDirectory(prefix='pn-sql-failure-') as d:run(Path(d),args.source)
