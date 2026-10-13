"""Exercise bank item registration through the installed Lua 5.1 item loader."""
import argparse
import configparser
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys
import zlib

ROOT = Path(__file__).resolve().parents[2]
COMPAT = ROOT / 'client-patch/client_compat'
sys.path.insert(0, str(COMPAT))
from validate import ground_pair


def check_resources(client, resource):
    """Resolve exact legacy resource bytes in active GRF order, without UTF-8 conversion."""
    texture = bytes.fromhex('646174615c746578747572655cc0afc0fac0cec5cdc6e4c0ccbdba5c')
    sprite = bytes.fromhex('646174615c7370726974655cbec6c0ccc5db5c')
    wanted = {
        texture + b'item\\' + resource + b'.bmp': 'item.bmp',
        texture + b'collection\\' + resource + b'.bmp': 'collection.bmp',
        sprite + resource + b'.spr': 'ground.spr',
        sprite + resource + b'.act': 'ground.act',
    }
    wanted = {key.lower(): value for key, value in wanted.items()}
    ini = configparser.ConfigParser()
    ini.read(client / 'DATA.INI')
    assert 0 < len(ini['Data']) <= 10
    assert all(0 <= int(slot) <= 9 for slot in ini['Data'])
    assets, report = {}, []
    for _, archive in sorted(ini['Data'].items(), key=lambda row: int(row[0])):
        with (client / archive).open('rb') as stream:
            header = stream.read(46)
            if header.startswith(b'Master of Magic'):
                offset, seed, count, version = struct.unpack_from('<IIII', header, 30)
                count -= seed + 7
            else:
                assert header.startswith(b'Event Horizon')
                offset, count, version = struct.unpack_from('<QII', header, 30)
                offset += 4
            assert version in (0x200, 0x300)
            stream.seek(46 + offset)
            packed, size = struct.unpack('<II', stream.read(8))
            table = zlib.decompress(stream.read(packed))
            assert len(table) == size
            pos = 0
            for _ in range(count):
                end = table.index(0, pos)
                name = table[pos:end].replace(b'/', b'\\')
                pos = end + 1
                packed, aligned, size, flag = struct.unpack_from('<IIIB', table, pos)
                pos += 13
                offset = struct.unpack_from('<Q' if version == 0x300 else '<I', table, pos)[0]
                pos += 8 if version == 0x300 else 4
                key = name.lower()  # bytes.lower preserves Korean bytes.
                if key not in wanted or not flag & 1:
                    continue
                category = wanted.pop(key)
                assert flag == 1 and aligned >= packed, 'Unsupported encrypted item artwork'
                stream.seek(46 + offset)
                data = stream.read(packed)
                raw = data if packed == size else zlib.decompress(data)
                assert len(raw) == size
                assets[category] = raw
                report.append({'category': category, 'archive': archive,
                               'path_hex': name.hex(), 'size': size,
                               'sha256': hashlib.sha256(raw).hexdigest()})
            assert pos == len(table)
    assert not wanted, 'Missing bank item artwork: ' + repr(wanted)
    for category, expected in [('item.bmp', (24, 24)), ('collection.bmp', (75, 100))]:
        raw = assets[category]
        assert raw[:2] == b'BM' and struct.unpack_from('<I', raw, 2)[0] == len(raw)
        width, height, planes, bpp = struct.unpack_from('<iiHH', raw, 18)
        assert (width, height) == expected and planes == 1 and bpp in (8, 24, 32)
    ground_pair(assets['ground.spr'], assets['ground.act'])
    return sorted(report, key=lambda row: row['category'])


PROGRAM = r'''
assert(_VERSION == 'Lua 5.1')
local function clone(v)
  if type(v) ~= 'table' then return v end
  local copy={}; for k,x in pairs(v) do copy[k]=clone(x) end; return copy
end
local function equal(a,b)
  if type(a) ~= type(b) then return false end
  if type(a) ~= 'table' then return a == b end
  for k,v in pairs(a) do if not equal(v,b[k]) then return false end end
  for k in pairs(b) do if a[k] == nil then return false end end
  return true
end
local original_dofile=dofile
local before, hooks, base_missing=nil,0,false
function dofile(path)
  if path == 'SystemEN/AccountBankInfo.lua' then
    hooks=hooks+1; before=clone(tbl); base_missing=(tbl[12781] == nil)
    -- Always reproduce the missing-base-record case, even after a base update.
    tbl[12781]=nil
    local result=original_dofile(PATCH)
    -- Check this patch at its boundary. The real loader intentionally applies
    -- unrelated branding, weights and new item tables after the bank overlay.
    for id,entry in pairs(tbl) do
      if id ~= 12781 then
        local expected=clone(before[id])
        if id == 6024 then
          expected.identifiedDescriptionName=clone(entry.identifiedDescriptionName)
        end
        assert(equal(entry,expected), 'Unrelated item metadata changed: '..id)
      end
    end
    for id in pairs(before) do assert(tbl[id], 'Removed item: '..id) end
    return result
  end
  return original_dofile(path)
end
dofile('SystemEN/itemInfo.lua')
dofile=original_dofile
assert(hooks == 1, 'Bank metadata must load exactly once through the actual item loader')
local ticket=tbl[12781]
assert(type(ticket) == 'table', 'Missing item 12781: the client would display Unknown Item')
assert(ticket.identifiedDisplayName == '1M Zeny Ticket')
assert(ticket.unidentifiedDisplayName == '1M Zeny Ticket')
assert(ticket.identifiedResourceName == '\196\237\198\249')
assert(ticket.unidentifiedResourceName == ticket.identifiedResourceName)
assert(ticket.slotCount == 0 and ticket.ClassNum == 0 and ticket.costume == false)
assert(ticket.Custom == true, 'Custom bank ticket must not link to an unrelated public item definition')
local count=0
for id,entry in pairs(tbl) do
  count=count+1
end
local once=clone(tbl); dofile(PATCH); assert(equal(tbl,once), 'Repeated metadata load changes items')
local registered, identified, unidentified={}, {}, {}
function AddItem(id,uname,uresource,name,resource,slots,classnum)
  assert(tbl[id] and not registered[id], 'Missing or duplicate item registration')
  registered[id]=true
  if id == 12781 then
    assert(uname == ticket.unidentifiedDisplayName and name == ticket.identifiedDisplayName)
    assert(uresource == ticket.unidentifiedResourceName and resource == ticket.identifiedResourceName)
    assert(slots == 0 and classnum == 0)
  end
  return true
end
function AddItemIdentifiedDesc(id,line)
  assert(type(line) == 'string'); if id == 12781 then identified[#identified+1]=line end; return true
end
function AddItemUnidentifiedDesc(id,line)
  assert(type(line) == 'string'); if id == 12781 then unidentified[#unidentified+1]=line end; return true
end
function AddItemIsCostume(id,value)
  if id == 12781 then assert(value == false) end; return true
end
function AddItemEffectInfo() return true end
function AddItemPackageID() return true end
local ok,msg=main(); assert(ok,msg)
local registrations=0; for _ in pairs(registered) do registrations=registrations+1 end
assert(registrations == count and registered[12781], 'Ticket was not registered by main()/AddItem')
for _,lines in ipairs({identified,unidentified}) do
  local text=table.concat(lines,'\n')
  assert(text:find('Retired currency. Use direct Zeny instead.',1,true), 'Missing currency retirement notice')
  assert(text:find('998,000',1,true) and text:find('@bank',1,true), 'Missing conversion value or bank guidance')
  assert(not text:find('1,002,000',1,true), 'Retired tickets must not advertise a purchase price')
  assert(text:find('Weight:^000000 0',1,true))
end
-- Preserve an existing installation's own resources and extra metadata.
local customized=clone(ticket)
customized.identifiedResourceName='custom-ticket'
customized.unidentifiedResourceName='custom-unidentified-ticket'
customized.Server='PN'; customized.extra={keep=true}
tbl[12781]=clone(customized); dofile(PATCH)
assert(equal(tbl[12781],customized), 'Existing ticket resources or extra metadata overwritten')
print(string.format('{"registered_items":%d,"base_ticket_missing":%s,"loader_hooks":%d,"ticket_registered":true,"identified_and_unidentified_descriptions":true,"repeat_load_safe":true,"custom_resources_preserved":true}', registrations,tostring(base_missing),hooks))
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--client', type=Path, required=True)
    parser.add_argument('--lua', type=Path, required=True)
    parser.add_argument('--patch', type=Path, default=COMPAT / 'SystemEN/AccountBankInfo.lua')
    parser.add_argument('--require-installed', action='store_true')
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    if args.require_installed:
        assert (args.client / 'SystemEN/AccountBankInfo.lua').read_bytes() == args.patch.read_bytes()
    program = 'PATCH=' + json.dumps(args.patch.resolve().as_posix()) + '\n' + PROGRAM
    result = subprocess.run([str(args.lua.resolve()), '-'], input=program, text=True,
                            cwd=args.client, capture_output=True, timeout=60)
    if result.returncode:
        raise SystemExit(result.stderr)
    report = json.loads(result.stdout)
    report['patch_sha256'] = hashlib.sha256(args.patch.read_bytes()).hexdigest()
    report['installed_bytes_verified'] = args.require_installed
    report['resources'] = check_resources(args.client, bytes.fromhex('c4edc6f9'))
    if args.report:
        args.report.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print('PASS: actual Lua item loader registered %d items, including the ticket; '
          'both descriptions and all four artwork resources verified; '
          'unrelated metadata and custom resources preserved.' % report['registered_items'])


if __name__ == '__main__':
    main()
