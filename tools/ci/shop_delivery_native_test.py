"""Run exact shop/barter transaction bodies with real inventory and payment code.

Linux build objects required. This suite covers synchronous ordinary/unlimited shops.
The durable planner boundary is explicit and any unexpected invocation fails the suite;
SQL receipt/recovery is covered separately. Dedicated multi-output delivery fixture. NPC proximity/world lookup is an explicit buylist
substitution; actual native quest conditions fill capacity between output grants. Common native fixture doubles client transport, logs and registry.
No server starts; sockets are denied. Synthetic catalog prices exercise numeric
boundaries, not a claim that these exact prices exist in the enabled catalog.
"""
from pathlib import Path
import argparse
import subprocess
import tempfile
import yaml
from biosphere_crown_transaction_test import WRAPPERS
from audit_enchant_upgrades import renewal_records
ROOT=Path(__file__).resolve().parents[2]

def run(out, source, cash_source):
    items={}
    for row in renewal_records(ROOT,'db/item_db.yml'):
        if row['Id'] in {501,502,503,504,2301}: items.setdefault(row['Id'],{}).update(row)
    (out/'items.yml').write_text(yaml.safe_dump({'Body':list(items.values())},sort_keys=False))
    prefix=(ROOT/'tools/ci/biosphere_crown_transaction_test.cpp').read_text().split('extern "C" int __wrap_main(',1)[0]
    prefix=prefix.replace('extern "C" void quest(map_session_data*){}',
        'extern "C" void real_quest(map_session_data*) asm("__real__Z17pc_show_questinfoP16map_session_data"); '
        'extern "C" void quest(map_session_data* sd){real_quest(sd);}')
    prefix=prefix.replace('extern "C" npc_data* npc_lookup(int32){return nullptr;}',
        'npc_data* quest_npc=nullptr; extern "C" npc_data* npc_lookup(int32){return quest_npc;}')
    src=source.read_text()
    barter=src[src.index('e_purchase_result npc_barter_purchase('):src.index('//Atempt to remove an npc')].replace('npc_barter_purchase(', 'audit_npc_barter_purchase(',1)
    buy=src[src.index('static int32 npc_buylist_sub(map_session_data* sd, std::vector'):src.index('/// npc_selllist for script-controlled shops')]
    buy=buy.replace('npc_buylist(', 'audit_npc_buylist(',1).replace('npc_checknear(sd,map_id2bl(sd->npc_shopid))','fixture_shop')
    cash=src[src.index('static enum e_CASHSHOP_ACK npc_cashshop_process_payment'):src.index(' * Returns the shop currency type')]
    cash=cash[:cash.rfind('/**')].replace('npc_cashshop_buylist(', 'audit_npc_cashshop_buylist(',1).replace('(npc_data *)map_id2bl(sd->npc_shopid)','fixture_shop')
    pc=(ROOT/'src/map/pc.cpp').read_text()
    deletion=pc[pc.index('char pc_delitem('):pc.index(' * Attempt to drop an item.')]
    deletion=deletion[:deletion.rfind('/*')].replace('pc_delitem(', 'audit_pc_delitem(',1).replace('pc_show_questinfo(sd)','quest(sd)')
    barter=barter.replace('pc_delitem(', 'audit_pc_delitem(').replace('pc_show_questinfo( &sd )','quest( &sd )')
    button=cash_source.read_text()
    button=button[button.index('bool cashshop_buylist('):button.index('/*\n * Reloads cashshop database')]
    button=button.replace('cashshop_buylist(', 'audit_cashshop_buylist(',1).replace('clif_cashshop_result(', 'fixture_cash_result(')
    # Legacy regression carts contain no finite stock/sale listings. Do not
    # turn async acceptance/refusal into a claimed synchronous delivery result.
    boundary = """
unsigned durable_boundary_calls=0;
bool fixture_durable_begin(map_session_data&,std::shared_ptr<pn_shop::Commit>,
 const std::vector<pn_shop::Grant>&,const uint32_t* = nullptr){++durable_boundary_calls;return false;}
"""
    buy=buy.replace('pn_shop_begin(', 'fixture_durable_begin(')
    barter=barter.replace('pn_shop_begin(', 'fixture_durable_begin(')
    button=button.replace('pn_shop_begin(', 'fixture_durable_begin(')
    driver=out/'native.cpp'
    driver.write_text(prefix+(ROOT/'tools/ci/shop_delivery_native_test.cpp').read_text().replace('// FUNCTION','npc_data* fixture_shop=nullptr;\nvoid fixture_cash_result(const map_session_data*,t_itemid,uint16){}\n'+boundary+deletion+buy+cash+barter+button))
    flags=['g++','-std=c++17','-O0','-fsanitize=address,undefined','-fno-sanitize-recover=all','-DPACKETVER=20260219']
    flags+=['-I'+str(ROOT/p) for p in ('src','3rdparty/libconfig','3rdparty/rapidyaml/src','3rdparty/rapidyaml/ext/c4core/src','3rdparty/json/include')]+['-I/usr/include/mysql']
    objects=list((ROOT/'src/map/obj').rglob('*.o'))
    libraries=[ROOT/p for p in ('src/common/obj/common.a','3rdparty/libconfig/obj/libconfig.a','3rdparty/rapidyaml/obj/ryml.a')]
    binary=out/'shop-test'
    subprocess.run(flags+[str(driver)]+[str(p) for p in objects+libraries]+['-Wl,--wrap='+x for x in WRAPPERS if x not in {'_Z28achievement_update_objectiveP16map_session_data19e_achievement_grouphz','_Z12pc_equipitemP16map_session_datasib','_Z14pc_unequipitemP16map_session_dataii'}]+['-Wl,--wrap=_Z15status_calc_bl_P10block_listSt6bitsetILm45EEh','-lz','-ldl','-lmysqlclient','-l:libzstd.so.1','-lssl','-lcrypto','-lresolv','-lm','-o',str(binary)],cwd=ROOT,check=True)
    subprocess.run([str(binary),str(out)],cwd=ROOT,check=True,timeout=60)

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--source',type=Path,default=ROOT/'src/map/npc.cpp')
    p.add_argument('--cash-source',type=Path,default=ROOT/'src/map/cashshop.cpp')
    p.add_argument('--build-dir',type=Path)
    args=p.parse_args()
    if args.build_dir:
        args.build_dir.mkdir(parents=True,exist_ok=True);run(args.build_dir.resolve(),args.source.resolve(),args.cash_source.resolve())
    else:
        with tempfile.TemporaryDirectory(prefix='pn-shop-native-') as d:run(Path(d),args.source.resolve(),args.cash_source.resolve())
