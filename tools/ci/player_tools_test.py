"""Native planner/history VM with explicit world/transport/registry boundaries.
Default denies networking; --sql uses only the disposable shop proof database.
"""
import argparse
import json
from pathlib import Path
import subprocess
import tempfile
import yaml
from audit_enchant_upgrades import renewal_records
from biosphere_crown_transaction_test import WRAPPERS

ROOT=Path(__file__).resolve().parents[2]

def transport_prefix(root):
    prefix=(root/'tools/ci/biosphere_crown_transaction_test.cpp').read_text().split('extern "C" int __wrap_main(',1)[0]
    prefix=prefix.replace('npc_lookup(int32){return nullptr;}','npc_lookup(int32 id){extern npc_data* planner_npc;return id==99000100?planner_npc:nullptr;}')
    prefix=prefix.replace('nums[key]=value;return true;}','nums[key]=value;if(script_getvaridx(key))script_array_update(&attached->regs,key,value==0);return true;}',1)
    prefix=prefix.replace('strings[key]=value;return true;}','strings[key]=value;if(script_getvaridx(key))script_array_update(&attached->regs,key,!value||!*value);return true;}',1)
    prefix=prefix.replace('++pause<30','++pause<100')
    changes={
        'std::vector<std::string> messages, menu_text;': 'std::vector<std::string> messages, menu_text, planner_visible; std::vector<std::vector<std::string>> planner_menu_pages, planner_next_pages;',
        'messages.emplace_back(text);}': 'messages.emplace_back(text);planner_visible.emplace_back(text);}',
        'void next(const map_session_data&,uint32){}': 'void next(const map_session_data&,uint32){planner_next_pages.push_back(planner_visible);planner_visible.clear();}',
        'menu_text.emplace_back(text);}': 'menu_text.emplace_back(text);planner_menu_pages.push_back(planner_visible);}',
    }
    for before,after in changes.items():
        assert before in prefix, 'Planner transport seam changed: '+before
        prefix=prefix.replace(before,after)
    return prefix

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--sql',action='store_true');parser.add_argument('--evidence',type=Path);args=parser.parse_args()
    subprocess.run(['python3','tools/generate_planner_routes.py','--check'],cwd=ROOT,check=True)
    shops=list(renewal_records(ROOT,'npc/custom/chapter1/barters.yml'))+list(renewal_records(ROOT,'npc/custom/episode21/barters.yml'))
    names={row['Item'] for shop in shops for row in shop['Items']}
    names.update(cost['Item'] for shop in shops for row in shop['Items'] for cost in row.get('RequiredItems',[]))
    items={}
    for row in renewal_records(ROOT,'db/item_db.yml'):
        if row.get('AegisName') in names:items.setdefault(row['Id'],{}).update(row)
    for row in items.values():
        for key in ('Script','EquipScript','UnEquipScript'):row.pop(key,None)
    prefix=transport_prefix(ROOT)
    with tempfile.TemporaryDirectory(prefix='player-tools-') as temp:
        work=Path(temp);driver=work/'driver.cpp';binary=work/'test'
        (work/'items.yml').write_text(yaml.safe_dump({'Body':list(items.values())}))
        (work/'shops.yml').write_text(yaml.safe_dump({'Body':shops}))
        driver.write_text(prefix+(ROOT/'tools/ci/player_tools_test.cpp').read_text())
        objects=list((ROOT/'src/map/obj').rglob('*.o'));assert objects,'Build current map objects first'
        libs=[ROOT/x for x in ('src/common/obj/common.a','3rdparty/libconfig/obj/libconfig.a','3rdparty/rapidyaml/obj/ryml.a')]
        includes=['src','3rdparty/libconfig','3rdparty/rapidyaml/src','3rdparty/rapidyaml/ext/c4core/src','3rdparty/json/include','/usr/include/mysql']
        wrappers=(*WRAPPERS,'_Z14pc_setregistryP16map_session_datall','_Z15clif_navigateToPK16map_session_dataPKctthbt','_Z18map_mapindex2mapidt','_Z16clif_scriptclearRK16map_session_datai')
        cmd=['g++','-std=c++17','-O0','-g','-fsanitize=undefined','-fno-sanitize-recover=all','-DPACKETVER=20260219']+['-I'+x for x in includes]+[str(driver)]+[str(x) for x in objects+libs]+['-Wl,--wrap='+x for x in wrappers]+['-lz','-ldl','-lmysqlclient','-l:libzstd.so.1','-lssl','-lcrypto','-lresolv','-lm','-o',str(binary)]
        subprocess.run(cmd,cwd=ROOT,check=True)
        result=subprocess.run([str(binary),str(work)]+(['sql'] if args.sql else []),cwd=ROOT,text=True,capture_output=True)
        output=result.stdout+result.stderr;print(output,end='')
        if args.evidence:args.evidence.mkdir(parents=True,exist_ok=True);(args.evidence/'player-tools.log').write_text(output)
        result.check_returncode();assert 'PLAYER_TOOLS_OK' in output and 'Memory manager: No memory leaks found.' in output
        assert 'TEST FAIL' not in output and '[Error]' not in output

if __name__=='__main__':main()
