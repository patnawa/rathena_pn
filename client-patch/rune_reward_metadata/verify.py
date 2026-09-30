"""Stage all three client fixes and execute the actual Lua 5.1 client loader."""
import argparse
import configparser
from collections import defaultdict
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys
from build import PACKAGE, ROOT, IDS, generate

def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()
def run(lua, client):
    return subprocess.check_output([str(lua), str(PACKAGE/'export.lua')], cwd=client)
def records(raw):
    result = defaultdict(list)
    for line in raw.decode('ascii').splitlines():
        fields = line.split('\t')
        result[int(fields[1])].append(fields)
    return result

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--client', required=True, type=Path)
    parser.add_argument('--lua', required=True, type=Path)
    parser.add_argument('--stage', required=True, type=Path)
    args = parser.parse_args()
    client, stage = args.client.resolve(), args.stage.resolve()
    assert not stage.exists(), 'Stage must be a new directory'
    assert not stage.is_relative_to(client) and not client.is_relative_to(stage)
    loader = client/'SystemEN/itemInfo.lua'
    original_sha = sha(loader)
    assert original_sha == 'a4cdf2fa12c723febe35a1ad66a6621af6382bea47507fe09a0f7900088136ca', 'Unexpected installed loader'
    generated, specs = generate()
    assert (PACKAGE/'SystemEN/itemInfo_RuneRewards.lua').read_bytes() == generated.encode('ascii')
    baseline = run(args.lua, client)
    shutil.copytree(client/'SystemEN', stage/'SystemEN')
    weights = ROOT/'client-patch/pn_item_weights/SystemEN/itemInfo_PNWeights.lua'
    shutil.copy2(weights, stage/'SystemEN'/weights.name)
    shutil.copy2(PACKAGE/'SystemEN/itemInfo_RuneRewards.lua', stage/'SystemEN/itemInfo_RuneRewards.lua')
    data = loader.read_bytes()
    assert b'itemInfo_RuneRewards.lua' not in data and b'itemInfo_PNWeights.lua' not in data
    (stage/'SystemEN/itemInfo.lua').write_bytes(data + b'\n-- Reviewed effective server metadata.\ndofile("SystemEN/itemInfo_PNWeights.lua")\ndofile("SystemEN/itemInfo_RuneRewards.lua")\n')
    subprocess.run([sys.executable, str(ROOT/'client-patch/sealed_drake_tooltip/apply.py'), str(stage)], check=True)
    patched = run(args.lua, stage)
    before, after = records(baseline), records(patched)
    assert set(after)-set(before) == set(IDS)
    assert not set(before)-set(after)
    weight_specs = re.findall(r'\[(\d+)\] = \{"([^"]+)", "([^"]+)"\}', weights.read_text())
    assert len(weight_specs) == 184
    allowed = {int(row[0]) for row in weight_specs}
    card_after = (ROOT/'client-patch/sealed_drake_tooltip/4496.after.txt').read_bytes().replace(b'\r\n', b'\n')
    original_cards = (client/'SystemEN/LuaFiles514/itemInfo.lua').read_bytes().replace(b'\r\n', b'\n')
    if card_after not in original_cards:
        allowed.add(4496)
    assert card_after in (stage/'SystemEN/LuaFiles514/itemInfo.lua').read_bytes().replace(b'\r\n', b'\n')
    changed = {iid for iid in before if before[iid] != after[iid]}
    assert changed == allowed, ('Unexpected changed records', sorted(changed ^ allowed))
    for iid, old, new in weight_specs:
        iid = int(iid)
        desc = [bytes.fromhex(r[2]).decode('ascii', errors='replace') for r in after[iid] if r[0] in ('DESC', 'UNID')]
        assert new in desc and old not in desc, ('Unfixed weight', iid)
    for spec in specs:
        iid, source = spec['id'], spec['resource_source']
        item = next(r for r in after[iid] if r[0] == 'ITEM')
        art = next(r for r in after[source] if r[0] == 'ITEM')
        assert bytes.fromhex(item[2]).decode() == spec['name']
        assert bytes.fromhex(item[4]).decode() == spec['name']
        assert item[3] == art[3] and item[5] == art[5], 'Resource bytes changed'
        desc = [bytes.fromhex(r[2]).decode() for r in after[iid] if r[0]=='DESC']
        assert set(spec['description']).issubset(desc)
        assert all(line in spec['description'] or set(line)=={'_'} or line=='^0000CCItem ID:^000000 '+str(iid) for line in desc)
    assert sha(loader) == original_sha, 'Installed client was modified'
    (stage/'effective-export.tsv').write_bytes(patched)
    # The acquisition tools consume one numeric ID per metadata row.
    (stage/'metadata.tsv').write_text('\n'.join(str(iid) for iid in sorted(after))+'\n')
    sys.path.insert(0, str(ROOT/'tools'))
    from client_release_audit import index
    ini = configparser.ConfigParser()
    ini.read(client/'DATA.INI')
    effective = set()
    archives = []
    for _, name in sorted((int(k), v) for k,v in ini['Data'].items()):
        rows = index(client/name)
        archives.append({'name': name, 'size': (client/name).stat().st_size, 'entries':len(rows),
            'index_sha256':hashlib.sha256(b'\n'.join(repr(row).encode('ascii') for row in rows)).hexdigest()})
        effective.update(row[0].replace(b'/', b'\\').lower() for row in rows if row[3]&1)
    prefix = bytes.fromhex('646174615c746578747572655cc0afc0fac0cec5cdc6e4c0ccbdba5c')
    missing = []
    for iid, rows in after.items():
        if iid == 0: continue
        item = next((r for r in rows if r[0]=='ITEM'), None)
        if item is None: continue
        for field, offset in [('unidentified',3), ('identified',5)]:
            resource = bytes.fromhex(item[offset])
            for kind in ('item','collection'):
                path = (prefix+kind.encode()+b'\\'+resource+b'.bmp').lower()
                if path not in effective:
                    missing.append(dict(item_id=iid, field=field, kind=kind, path_hex=path.hex()))
    assert not [row for row in missing if row['item_id'] in IDS], 'Missing reward artwork'
    (stage/'triage.json').write_text(json.dumps(dict(archives=archives,catalog_missing_references=missing),indent=2)+'\n')
    report = dict(status='passed', added_ids=IDS, changed_existing_ids=sorted(changed),
        weight_records=len(weight_specs), installed_loader_sha256=original_sha,
        candidate_loader_sha256=sha(stage/'SystemEN/itemInfo.lua'),
        export_sha256=sha(stage/'effective-export.tsv'), lua_sha256=sha(args.lua),
        source_sha256={str(path.relative_to(ROOT)).replace('\\','/'):sha(path) for path in [
            PACKAGE/'build.py', PACKAGE/'verify.py', PACKAGE/'export.lua',
            PACKAGE/'SystemEN/itemInfo_RuneRewards.lua', weights, ROOT/'db/re/item_db_usable.yml']},
        triage_sha256=sha(stage/'triage.json'), metadata_sha256=sha(stage/'metadata.tsv'),
        boundary='Actual Lua 5.1 loader/callback metadata and DATA.INI archive index coverage; no rendered client acceptance or proof of loose-file lookup precedence.')
    (stage/'report.json').write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps(report, indent=2))

if __name__ == '__main__': main()
