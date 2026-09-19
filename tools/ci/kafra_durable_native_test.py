"""Production Kafra VM/inventory tests; explicitly mocked transport ACK, separate SQL proof."""
import argparse,json,hashlib
from pathlib import Path
import kafra_reserve_native_test as old

original_validate=old.validate
def validate(root):
    result=original_validate(root)
    result['durable_sources']={name:hashlib.sha256((root/name).read_bytes()).hexdigest() for name in ('src/custom/reserve_map.inc','src/custom/bank_state.hpp','src/custom/bank_commit.hpp','tools/ci/kafra_durable_native_test.py','tools/ci/weekly_practice_native_cases.inc','npc/custom/main_office/weekly_practice.txt')}
    return result
old.validate=validate

p=argparse.ArgumentParser();p.add_argument('--native-build-dir',type=Path,required=True);p.add_argument('--before',type=Path,required=True);a=p.parse_args()
inputs=old.prepare(a.native_build_dir,a.before)
weekly=(old.test.ROOT/'npc/custom/main_office/weekly_practice.txt').read_text()
for name,label in [('PN_WeeklyReset','reset'),('PN_WeeklyLabComplete','lab'),('PN_GuideMilestones','build'),('PN_WeeklyBoard','board')]:
    (a.native_build_dir/f'weekly-{label}.script').write_text(old.test.gate.body(weekly,name))
casefile=a.native_build_dir/'shop_cases.inc'
cases=casefile.read_text().replace('void shop_cases(const std::string& dir){','void shop_cases(const std::string& dir){\n weekly_cases(dir);',1)
casefile.write_text('void weekly_cases(const std::string& dir);\n'+cases+'\n'+(old.test.ROOT/'tools/ci/weekly_practice_native_cases.inc').read_text())
driver=Path(old.test.DRIVER);source=driver.read_text()
source=source.replace('sd->fd=0;sd->status.base_level=275;', 'sd->state={};sd->vars_ok=true;sd->battle_status.hp=100;sd->fd=0;sd->status.base_level=275;')
source=source.replace('delete sd;', 'if(sd->regs.arrays)sd->regs.arrays->destroy(sd->regs.arrays,script_free_array_db);delete sd;',1)
source=source.replace('name=="RESRVPTS"||','name.rfind("PNWeekly",0)==0||name=="RESRVPTS"||',1)
stub='''
extern "C" int reserve_connected() asm("__wrap__Z17chrif_isconnectedv");
extern "C" int reserve_connected(){return 1;}
extern "C" void weekly_effect(const block_list*,int32,send_target) asm("__wrap__Z18clif_specialeffectPK10block_listi11send_target");
extern "C" void weekly_effect(const block_list* bl,int32,send_target){check(bl==attached,"cosmetic effect owner");}
extern "C" void reserve_transport(map_session_data&) asm("__wrap__Z15intif_bank_saveR16map_session_data");
extern "C" void reserve_transport(map_session_data& sd){
    check(sd.bank_ui.pending && !sd.bank_ui.applying && sd.bank_ui.action==7,"reserve request stays locked until explicit transport ACK");
    check(sd.bank_ui.reserve_before-sd.bank_ui.reserve_after==sd.bank_ui.amount,"exact point debit in durable request");
    check(sd.bank_ui.request_id && (sd.bank_ui.nonce_hi || sd.bank_ui.nonce_lo),"durable purchase identity present");
    sd.bank_ui.pending=false;sd.bank_ui.result=pn_bank::Ok;
}
'''
source=source.replace('extern "C" int __wrap_main(',stub+'\nextern "C" int __wrap_main(',1);driver.write_text(source)
old.test.CROWN_WRAPPERS=tuple(old.test.CROWN_WRAPPERS)+('_Z17chrif_isconnectedv','_Z15intif_bank_saveR16map_session_data','_Z18clif_specialeffectPK10block_listi11send_target')
old.test.native(a.native_build_dir,inputs,verifier=old.validate,fixture_only=True)
print('KAFRA_DURABLE_VM_PASS: production preflight, grant, request identity and NPC flow; transport ACK is a double, durability is tested by reserve_sql_runtime.cpp')
