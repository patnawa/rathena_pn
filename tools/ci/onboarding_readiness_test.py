#!/usr/bin/env python3
"""Run production onboarding/readiness functions in the native script VM.
Requires current Linux map objects. Networking is denied. Transport, registry and
party lookup are explicit boundaries; quest state/timers, instance metadata,
leader checks, inventory and scripts use production code. No rendered proof.
"""
import argparse
import json
from pathlib import Path
import subprocess
import tempfile
import sys
import yaml


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path)
    parser.add_argument('--source', type=Path)
    parser.add_argument('--driver', type=Path, default=Path(__file__).with_suffix('.cpp'))
    args = parser.parse_args()
    root = (args.root or Path(__file__).resolve().parents[2]).resolve()
    sys.path.insert(0, str(root / 'tools/ci'))
    from biosphere_crown_transaction_test import WRAPPERS
    from audit_enchant_upgrades import renewal_records
    source = (args.source or root / 'npc/custom/main_office/readiness.txt').resolve()
    # Require the live admission sites to consume the same read-only predicate.
    chapter = (root / 'npc/custom/chapter2/Chapter2.txt').read_text()
    assert chapter.count('callfunc("PN_InstanceMissing",.@name$)') == 2
    for entry in ('GhostPalace.txt', 'CharlestonCrisis.txt'):
        entrance = (root / 'npc/re/instances' / entry).read_text()
        assert 'callfunc("PN_InstanceMissing",.@md_name$)' in entrance
    assert 'npc: npc/custom/main_office/readiness.txt' in (root / 'npc/scripts_custom.conf').read_text()
    assert (root/'npc/custom/chapter1/CH1.c').read_text().count('callfunc("PN_InstanceMissing",.@md_name$)')==6
    subprocess.run([sys.executable,str(root/'tools/generate_chapter1_guide.py'),'--check'],cwd=root,check=True)

    items = {}
    for row in renewal_records(root, 'db/item_db.yml'):
        if row['Id'] == 512 or row.get('AegisName') in ('Dragon_Scale','Ch1_Broken_Petal'):
            items.setdefault(row['Id'], {}).update(row)
    prefix = (root / 'tools/ci/biosphere_crown_transaction_test.cpp').read_text().split('extern "C" int __wrap_main(', 1)[0]
    prefix = prefix.replace('std::map<int64,int64> nums;', '''std::map<int64,int64> nums;
std::map<int,map_session_data*> guide_players;
std::map<int,std::map<int64,int64>> guide_member_nums;
std::map<int64,int64>& guide_values(const map_session_data* sd){return sd==attached?nums:guide_member_nums[sd->status.account_id];}''')
    prefix = prefix.replace('return attached&&attached->id==id?attached:nullptr;', 'auto it=guide_players.find(id);return it!=guide_players.end()?it->second:(attached&&attached->id==id?attached:nullptr);')
    prefix = prefix.replace('check(name.rfind("$@__SW",0)==0&&name.substr(name.size()-4)=="_VAL",', 'check(name=="$@partymembercount"||(name.rfind("$@__SW",0)==0&&name.substr(name.size()-4)=="_VAL"),')
    # Reading an unset registry returns zero without creating a stored entry.
    prefix = prefix.replace('return nums[key];', 'auto it=nums.find(key);return it==nums.end()?0:it->second;')
    prefix = prefix.replace('return nums[add_str(name)];', 'auto it=nums.find(add_str(name));return it==nums.end()?0:it->second;')
    for function in ('readreg','registry','named_registry'):
        prefix=prefix.replace(function+'(const map_session_data*,',function+'(const map_session_data* sd,')
    prefix=prefix.replace('auto it=nums.find(key);return it==nums.end()?0:it->second;', 'auto& values=guide_values(sd);auto it=values.find(key);return it==values.end()?0:it->second;')
    prefix=prefix.replace('auto it=nums.find(add_str(name));return it==nums.end()?0:it->second;', 'auto& values=guide_values(sd);auto it=values.find(add_str(name));return it==values.end()?0:it->second;')
    prefix=prefix.replace('switch_read(int64 key){auto& values=guide_values(sd);','switch_read(int64 key){auto& values=nums;')
    prefix=prefix.replace('mes(const map_session_data&,uint32,const char* text){messages.emplace_back(text);}', 'mes(const map_session_data& sd,uint32,const char* text){check(&sd==attached,"readiness dialog only reaches requesting player");messages.emplace_back(text);}')
    prefix = prefix.replace('check(attached->status.zeny==7654321,"no Zeny charge");', '')
    original = 'nums[key]=value;return true;}'
    assert original in prefix
    prefix = prefix.replace(original, 'nums[key]=value;if(script_getvaridx(key))script_array_update(&attached->regs,key,value==0);return true;}', 1)
    with tempfile.TemporaryDirectory(prefix='onboarding-readiness-') as temp:
        work = Path(temp)
        quest = next(row for row in renewal_records(root, 'db/quest_db.yml') if row['Id'] == 8964)
        mobs = {row['Id']: row for row in renewal_records(root, 'db/mob_db.yml') if row.get('AegisName') == 'CH1_SHADOW_JAILER'}
        (work / 'guide-quest.yml').write_text(yaml.safe_dump({'Body': [quest]}))
        (work / 'guide-mobs.json').write_text(json.dumps(list(mobs.values())))
        (work / 'armor-items.yml').write_text(yaml.safe_dump({'Header': {'Type': 'ITEM_DB', 'Version': 3}, 'Body': list(items.values())}, sort_keys=False))
        driver = work / 'driver.cpp'
        driver.write_text(prefix + args.driver.read_text())
        objects = list((root / 'src/map/obj').rglob('*.o'))
        if not objects:
            raise SystemExit('Build map-server objects first')
        libraries = [root / p for p in ('src/common/obj/common.a', '3rdparty/libconfig/obj/libconfig.a', '3rdparty/rapidyaml/obj/ryml.a')]
        includes = ['src', '3rdparty/libconfig', '3rdparty/rapidyaml/src', '3rdparty/rapidyaml/ext/c4core/src', '3rdparty/json/include', '/usr/include/mysql']
        binary = work / 'guide-test'
        command = ['g++', '-std=c++17', '-O0', '-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-DPACKETVER=20260219']
        command += ['-I' + value for value in includes]
        command += [str(driver)] + [str(p) for p in objects + libraries]
        command += ['-Wl,--wrap=' + value for value in (*WRAPPERS, '_Z12party_searchi', '_Z15clif_navigateToPK16map_session_dataPKctthbt')]
        command += ['-lz', '-ldl', '-lmysqlclient', '-l:libzstd.so.1', '-lssl', '-lcrypto', '-lresolv', '-lm', '-o', str(binary)]
        subprocess.run(command, cwd=root, check=True)
        run = subprocess.run([str(binary), str(work), str(source)], cwd=root, capture_output=True, text=True)
        output = run.stdout + run.stderr
        print(output, end='')
        run.check_returncode()
        assert 'ONBOARDING_READINESS_OK' in output
        assert 'Memory manager: No memory leaks found.' in output
        assert 'TEST FAIL' not in output and '[Error]' not in output


if __name__ == '__main__':
    main()
