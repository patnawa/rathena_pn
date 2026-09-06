#!/usr/bin/env python3
"""Read-only real Lua 5.1 candidate/installed metadata and active GRF asset proof."""
import argparse
import base64
import configparser
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
from build import ROOT, PACKAGE, ITEMS, outputs, require

CLIENT = ROOT.parent.parent


def digest(path): return hashlib.sha256(path.read_bytes()).hexdigest()


def windows(path):
    value = str(path.resolve())
    if os.name != 'nt': value = subprocess.check_output(['wslpath', '-w', value], text=True).strip()
    return value.replace('\\', '/')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--lua', type=Path, default=ROOT.parent / 'chapter2-lua51-runtime-20260906/runtime/lua5.1.exe')
    parser.add_argument('--loader', type=Path, default=CLIENT / 'SystemEN/itemInfo.lua')
    parser.add_argument('--before-loader', type=Path)
    parser.add_argument('--installed', action='store_true')
    parser.add_argument('--grf-reader', type=Path, default=ROOT / 'tools/grf_v3_extract/grf_v3_extract.exe')
    args = parser.parse_args()
    loader = args.loader.resolve(); before = (args.before_loader or loader).resolve()
    require(not args.installed or args.before_loader and before != loader, 'Installed mode needs a distinct pre-install loader')
    if args.installed:
        source = loader.read_text()
        for value in ('itemInfo_DruidMissingCrowns.lua', 'druidmissingcrowns'):
            source, count = re.subn(r'^\s*["\']' + re.escape(value) + r'["\'],[^\n]*\n', '', source, flags=re.M)
            require(count == 1, 'Expected exactly one new import pair')
        require(source == before.read_text(), 'Loader changed beyond the exact new import pair')
    for path, expected in outputs().items(): require((ROOT / path).read_text() == expected, 'Renderer/source drift: ' + path)
    ini = configparser.ConfigParser(); ini.read(CLIENT / 'DATA.INI', encoding='utf-8-sig')
    resources = {f['resource'].lower() for f in ITEMS} | {'episodclear20'}
    crowns = {f['resource'].lower() for f in ITEMS if f['item']['Type'] == 'Armor'}
    pattern = '(?i)(?:' + '|'.join(sorted(resources)) + r')\.(?:bmp|spr|act)$'
    assets = {}
    for _, archive in sorted(ini['Data'].items(), key=lambda p: int(p[0])):
        # Windows PowerShell relays the native console's Unicode strings into
        # an ASCII base64 envelope. Direct WSL capture uses a lossy OEM codepage
        # for Korean-path bytes, which cannot prove male/female sprite names.
        quote = lambda value: "'" + value.replace("'", "''") + "'"
        command = '[Console]::OutputEncoding = New-Object System.Text.UTF8Encoding; $result = & ' + ' '.join(quote(v) for v in
            (windows(args.grf_reader), windows(CLIENT / archive), pattern))
        command += '; if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }; '
        command += '[Convert]::ToBase64String([Text.Encoding]::UTF8.GetBytes(($result -join "`n")))'
        result = subprocess.run(['powershell.exe', '-NoProfile', '-NonInteractive', '-EncodedCommand',
            base64.b64encode(command.encode('utf-16-le')).decode('ascii')], capture_output=True, check=True)
        for line in base64.b64decode(result.stdout.strip(), validate=True).decode('utf-8').splitlines():
            if '\t' not in line: continue
            path = line.split('\t')[0].lower(); basename = path.rsplit('\\', 1)[-1]
            stem, extension = basename.rsplit('.', 1)
            if stem in resources:
                role = extension if extension in ('spr', 'act') else ('item' if '\\item\\' in path else 'collection' if '\\collection\\' in path else None)
                if role: assets.setdefault((stem, role), archive)
            else:
                for crown in crowns:
                    if stem.endswith('_' + crown) and extension in ('spr', 'act'):
                        # GRF reader preserves path bytes through Latin-1. These
                        # are exact CP949 male/female accessory directory names.
                        gender = 'male' if '\\³²\\' in path else 'female' if '\\¿©\\' in path else None
                        if gender: assets.setdefault((crown, gender + '_' + extension), archive)
    wanted = {(r, role) for r in resources for role in ('item', 'collection', 'spr', 'act')}
    wanted |= {(r, g + '_' + e) for r in crowns for g in ('male', 'female') for e in ('spr', 'act')}
    require(wanted <= set(assets), 'Missing active client art: ' + repr(sorted(wanted - set(assets))))
    names = ROOT.parent / 'audit-item-aliases-20260906/data/data/luafiles514/lua files/itemdbnametbl.lub'
    accessory = ROOT.parent / 'missing-crowns-client-source-20260906/data/data/luafiles514/lua files/datainfo'
    list_path = ROOT.parent / 'audit-client-enchants-20260906/nebula/data/luafiles514/lua files/Enchant/EnchantList.lub'
    targets = sorted(set(re.findall(r':AddTargetItem(?:_Duplicate)?\("([^"]+)"', list_path.read_text(encoding='cp949'))))
    values = {'LOADER': loader, 'BEFORE': before, 'FRAGMENT': PACKAGE / 'SystemEN/itemInfo_DruidMissingCrowns.lua',
              'NAMES': names, 'ACCESSORY_ID': accessory / 'accessoryid.lub', 'ACCESSORY_NAME': accessory / 'accname.lub'}
    source = '\n'.join(k + '=' + json.dumps(windows(v)) for k, v in values.items())
    source += '\nINSTALLED=' + str(args.installed).lower()
    source += '\nEXPECTED={' + ','.join('[' + str(f['item']['Id']) + ']={' +
        'name=' + json.dumps(f['item']['Name']) + ',aegis=' + json.dumps(f['item']['AegisName']) +
        ',slots=' + str(f['item']['Slots']) + ',view=' + str(f['item'].get('View', f.get('client_view'))) +
        ',resource=' + json.dumps(f['resource']) + ',crown=' + str(f['item']['Type'] == 'Armor').lower() + '}' for f in ITEMS) + '}'
    source += '\nTARGETS={' + ','.join(json.dumps(n, ensure_ascii=False) for n in targets) + '}\n'
    source += r'''
assert(_VERSION=="Lua 5.1")
os.execute,io.popen=nil,nil
MessageBox=function(message) error(message) end
local function clone(x,seen)
 if type(x)~="table" then return x end
 seen=seen or {};if seen[x] then return seen[x] end
 local y={};seen[x]=y;for k,v in pairs(x) do y[clone(k,seen)]=clone(v,seen) end
 return setmetatable(y,getmetatable(x))
end
local function equal(a,b,seen)
 if type(a)~=type(b) then return false end
 if type(a)~="table" then return a==b end
 if getmetatable(a)~=getmetatable(b) then return false end
 seen=seen or {};if seen[a] then return seen[a]==b end;seen[a]=b
 for k,v in pairs(a) do if not equal(v,b[k],seen) then return false end end
 for k in pairs(b) do if a[k]==nil then return false end end
 return true
end
local probe={nested={1,2}};local copy=clone(probe);assert(equal(probe,copy));probe.nested[2]=3;assert(not equal(probe,copy))
dofile(BEFORE);dofile(NAMES);dofile(ACCESSORY_ID);dofile(ACCESSORY_NAME)
local old=clone(tbl);local oldfiles,oldtables=clone(ImportFiles),clone(ImportTables)
for _,v in ipairs(oldfiles) do assert(v~="itemInfo_DruidMissingCrowns.lua") end
for _,v in ipairs(oldtables) do assert(v~="druidmissingcrowns") end
local function missing()
 local result={};for _,n in ipairs(TARGETS) do local id=ItemDB_To_ItemID(n);assert(id>0)
  if not tbl[id] or type(tbl[id].slotCount)~="number" then result[id]=n end end;return result
end
local missing_before=missing()
local env={};local f=assert(loadfile(FRAGMENT));setfenv(f,env);f()
local addition=assert(env.tbl_druidmissingcrowns);local count=0
for id,e in pairs(addition) do
 count=count+1;local expected=assert(EXPECTED[id]);assert(old[id]==nil,"Existing item must never be overwritten: "..id)
 assert(ItemDB_To_ItemID(expected.aegis)==id)
 assert(e.slotCount==expected.slots and e.ClassNum==expected.view and e.costume==false)
 assert(e.identifiedDisplayName==expected.name and e.identifiedResourceName==expected.resource)
 assert(e.unidentifiedResourceName=="EpisodClear20" and #e.identifiedDescriptionName>20)
 if expected.crown then
  assert(ACCESSORY_IDs['ACCESSORY_'..expected.resource]==expected.view)
  assert(AccNameTable[expected.view]=='_'..expected.resource)
  assert(missing_before[id]==expected.aegis)
 end
end
assert(count==23)
if INSTALLED then
 dofile(LOADER);local oi,n=1,0
 assert(#ImportFiles==#oldfiles+1 and #ImportTables==#oldtables+1)
 for i,v in ipairs(ImportFiles) do if v=="itemInfo_DruidMissingCrowns.lua" then
  assert(ImportTables[i]=="druidmissingcrowns");n=n+1
 else assert(v==oldfiles[oi] and ImportTables[i]==oldtables[oi]);oi=oi+1 end end
 assert(n==1 and oi==#oldfiles+1)
else F_itemInfoMerge(addition);assert(equal(ImportFiles,oldfiles) and equal(ImportTables,oldtables)) end
local oldcount,added=0,0
for id,e in pairs(old) do oldcount=oldcount+1;assert(equal(tbl[id],e),"Old nested metadata changed: "..id) end
for id,e in pairs(tbl) do if old[id]==nil then added=added+1;assert(addition[id] and equal(e,addition[id])) end end
for id,e in pairs(addition) do assert(equal(tbl[id],e)) end
assert(added==23)
local after=missing();local mb,ma,removed=0,0,0
for id,n in pairs(missing_before) do mb=mb+1;if not after[id] then assert(EXPECTED[id] and EXPECTED[id].crown);removed=removed+1 end end
for id,n in pairs(after) do ma=ma+1;assert(missing_before[id]==n) end
assert(removed==12 and mb-ma==12)
print("MISSING_CROWNS_CLIENT_OK added=23 old_deep_preserved="..oldcount.." target_missing_before="..mb.." target_missing_after="..ma)
'''
    tracked = [CLIENT / 'DATA.INI', CLIENT / 'SystemEN/itemInfo.lua', *values.values()]
    hashes = {str(p): digest(p) for p in tracked}
    env = {k: v for k, v in os.environ.items() if not k.startswith('LUA')}
    result = subprocess.run([str(args.lua.resolve()), '-'], input=source.encode('cp949'), capture_output=True,
                            cwd=CLIENT, env=env, timeout=40)
    print(result.stdout.decode('cp949', errors='replace'), end=''); print(result.stderr.decode('cp949', errors='replace'), end='')
    result.check_returncode()
    require(result.stdout.count(b'MISSING_CROWNS_CLIENT_OK added=23') == 1 and not result.stderr.strip(), 'Missing/unclean real Lua completion')
    require(hashes == {str(p): digest(p) for p in tracked}, 'Client/source changed during read-only proof')
    print(json.dumps({'result': 'PASS', 'installed': args.installed, 'checked_loader': str(loader),
                      'loader_is_active': loader == (CLIENT / 'SystemEN/itemInfo.lua').resolve(),
                      'before_loader': str(before), 'verified_assets': len(wanted), 'source_hashes': hashes}, indent=2))


if __name__ == '__main__': main()
