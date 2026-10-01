#!/usr/bin/env python3
"""Run production onboarding/readiness functions in the native script VM.
Requires current Linux map objects. Networking is denied. Transport, registry and
party lookup are explicit boundaries; quest state/timers, instance metadata,
leader checks, inventory and scripts use production code. No rendered proof.
"""
import argparse
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

    items = {}
    for row in renewal_records(root, 'db/item_db.yml'):
        if row['Id'] == 512:
            items.setdefault(row['Id'], {}).update(row)
    prefix = (root / 'tools/ci/biosphere_crown_transaction_test.cpp').read_text().split('extern "C" int __wrap_main(', 1)[0]
    # Reading an unset registry returns zero without creating a stored entry.
    prefix = prefix.replace('return nums[key];', 'auto it=nums.find(key);return it==nums.end()?0:it->second;')
    prefix = prefix.replace('return nums[add_str(name)];', 'auto it=nums.find(add_str(name));return it==nums.end()?0:it->second;')
    prefix = prefix.replace('check(attached->status.zeny==7654321,"no Zeny charge");', '')
    original = 'nums[key]=value;return true;}'
    assert original in prefix
    prefix = prefix.replace(original, 'nums[key]=value;if(script_getvaridx(key))script_array_update(&attached->regs,key,value==0);return true;}', 1)
    with tempfile.TemporaryDirectory(prefix='onboarding-readiness-') as temp:
        work = Path(temp)
        (work / 'armor-items.yml').write_text(yaml.safe_dump({'Header': {'Type': 'ITEM_DB', 'Version': 3}, 'Body': list(items.values())}, sort_keys=False))
        driver = work / 'driver.cpp'
        driver.write_text(prefix + args.driver.read_text())
        objects = list((root / 'src/map/obj').rglob('*.o'))
        if not objects:
            raise SystemExit('Build map-server objects first')
        libraries = [root / p for p in ('src/common/obj/common.a', '3rdparty/libconfig/obj/libconfig.a', '3rdparty/rapidyaml/obj/ryml.a')]
        includes = ['src', '3rdparty/libconfig', '3rdparty/rapidyaml/src', '3rdparty/rapidyaml/ext/c4core/src', '3rdparty/json/include', '/usr/include/mysql']
        binary = work / 'guide-test'
        command = ['g++', '-std=c++17', '-O0', '-DPACKETVER=20260219']
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
