#!/usr/bin/env python3
"""Actual pet reward builtin and Child Manager VM regression using Linux map objects.

Requires a built source tree and PyYAML. No server/world/account database starts;
the test denies networking with seccomp. Crown fixture transport and registry
boundaries remain explicit. Inventory deletion/addition use actual core code.
Use --source for a pre-fix NPC to reproduce the rejected binding-loss baseline.
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
    source = (args.source or root / 'npc/re/merchants/enchan_sage_legacy_17_2.txt').resolve()
    items = {}
    for row in renewal_records(root, 'db/item_db.yml'):
        if row['Id'] in (9123, 1000103, 1201):
            items.setdefault(row['Id'], {}).update(row)
    prefix = (root / 'tools/ci/biosphere_crown_transaction_test.cpp').read_text().split('extern "C" int __wrap_main(', 1)[0]
    prefix = prefix.replace('check(attached->status.zeny==7654321,"no Zeny charge");', '')
    original = 'nums[key]=value;return true;}'
    assert original in prefix
    prefix = prefix.replace(original, 'nums[key]=value;if(script_getvaridx(key))script_array_update(&attached->regs,key,value==0);return true;}', 1)
    with tempfile.TemporaryDirectory(prefix='pet-reward-') as temp:
        work = Path(temp)
        (work / 'pet-items.yml').write_text(yaml.safe_dump({'Header': {'Type': 'ITEM_DB', 'Version': 3}, 'Body': list(items.values())}, sort_keys=False))
        driver = work / 'driver.cpp'
        driver.write_text(prefix + args.driver.read_text())
        objects = list((root / 'src/map/obj').rglob('*.o'))
        if not objects:
            raise SystemExit('Build map-server objects first')
        libraries = [root / p for p in ('src/common/obj/common.a', '3rdparty/libconfig/obj/libconfig.a', '3rdparty/rapidyaml/obj/ryml.a')]
        includes = ['src', '3rdparty/libconfig', '3rdparty/rapidyaml/src', '3rdparty/rapidyaml/ext/c4core/src', '3rdparty/json/include', '/usr/include/mysql']
        binary = work / 'pet-test'
        command = ['g++', '-std=c++17', '-O0', '-DPACKETVER=20260219']
        command += ['-I' + value for value in includes]
        command += [str(driver)] + [str(p) for p in objects + libraries]
        command += ['-Wl,--wrap=' + value for value in (*WRAPPERS, '_Z14pn_shop_submitR16map_session_dataSt10shared_ptrIN7pn_shop6CommitEESt6vectorINS2_5EventESaIS6_EEj')]
        command += ['-lz', '-ldl', '-lmysqlclient', '-l:libzstd.so.1', '-lssl', '-lcrypto', '-lresolv', '-lm', '-o', str(binary)]
        subprocess.run(command, cwd=root, check=True)
        run = subprocess.run([str(binary), str(work), str(source)], cwd=root, capture_output=True, text=True)
        output = run.stdout + run.stderr
        print(output, end='')
        run.check_returncode()
        assert 'PET_REWARD_SCRIPT_OK cases=26' in output
        assert 'Memory manager: No memory leaks found.' in output
        assert 'TEST FAIL' not in output and '[Error]' not in output


if __name__ == '__main__':
    main()
