"""Validate actual Lua loading, resource preservation, archive bytes and ground sprites."""
import argparse
import configparser
from collections import defaultdict
import hashlib
import io
import json
import re
from pathlib import Path
import struct
import subprocess
import tempfile
import zlib

from build_grf import build


def archive_index(path):
    with (io.BytesIO(path) if isinstance(path, bytes) else path.open('rb')) as f:
        header = f.read(46)
        if header.startswith(b'Master of Magic'):
            offset, seed, count, version = struct.unpack_from('<IIII', header, 30)
            count -= seed + 7
        else:
            assert header.startswith(b'Event Horizon')
            offset, count, version = struct.unpack_from('<QII', header, 30)
            offset += 4
        assert version in (0x200, 0x300)
        f.seek(46 + offset)
        packed, size = struct.unpack('<II', f.read(8))
        table = zlib.decompress(f.read(packed))
        assert len(table) == size
        pos, result = 0, {}
        for _ in range(count):
            end = table.index(0, pos)
            name = table[pos:end].decode('latin1').replace('\\', '/').lower()
            pos = end + 1
            packed, aligned, size, flag = struct.unpack_from('<IIIB', table, pos)
            pos += 13
            offset = struct.unpack_from('<Q' if version == 0x300 else '<I', table, pos)[0]
            pos += 8 if version == 0x300 else 4
            if flag & 1:
                result[name] = (packed, size, flag, offset)
        assert pos == len(table)
        return result


def ground_pair(spr, act):
    assert spr[:4] == b'SP\x01\x02'
    indexed, rgba = struct.unpack_from('<HH', spr, 4)
    assert indexed == 1 and rgba == 0
    width, height, size = struct.unpack_from('<HHH', spr, 8)
    payload = spr[14:14 + size]
    assert len(spr) == 14 + size + 1024
    pos = pixels = 0
    while pos < len(payload):
        value = payload[pos]
        pos += 1
        if value:
            pixels += 1
        else:
            pixels += payload[pos]
            pos += 1
    assert pixels == width * height
    assert act[:4] == b'AC\x05\x02'
    actions = struct.unpack_from('<H', act, 4)[0]
    pos = 16
    for _ in range(actions):
        frames = struct.unpack_from('<I', act, pos)[0]
        pos += 4
        assert 0 < frames < 1000
        for _ in range(frames):
            pos += 32
            layers = struct.unpack_from('<I', act, pos)[0]
            pos += 4
            for _ in range(layers):
                sprite = struct.unpack_from('<i', act, pos + 8)[0]
                sprite_type = struct.unpack_from('<i', act, pos + 32)[0]
                assert -1 <= sprite < indexed and sprite_type == 0
                pos += 44
            sound, anchors = struct.unpack_from('<iI', act, pos)
            assert sound == -1
            pos += 8 + 16 * anchors
    sounds = struct.unpack_from('<I', act, pos)[0]
    pos += 4 + sounds * 40 + actions * 4
    assert pos == len(act)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--assets-only', action='store_true',
                        help='Validate tracked asset sources and an in-memory GRF without a client or Lua')
    parser.add_argument('--client', type=Path)
    parser.add_argument('--lua', type=Path)
    args = parser.parse_args()
    if not args.assets_only and (args.client is None or args.lua is None):
        parser.error('--client and --lua are required unless --assets-only is specified')
    root = Path(__file__).resolve().parent
    archive = build(root)
    index = archive_index(archive)
    rows = json.loads((root / 'assets.json').read_text())
    assert len(index) == len(rows) == 117
    card_manifest = json.loads((root / 'card_illustrations.json').read_text())
    card_path = root / 'assets/tables/num2cardillustnametable.txt'
    card_data = card_path.read_bytes()
    baseline = card_data[:card_manifest['baseline_bytes']]
    assert hashlib.sha256(baseline).hexdigest() == card_manifest['baseline_sha256']
    additions = card_manifest['additions']
    assert additions == {str(i): 'sorry' for i in range(300761, 300784)}
    expected_tail = b''.join(f'{i}#{name}#\r\n'.encode() for i, name in additions.items())
    assert card_data == baseline + b'\r\n' + expected_tail
    # The original table has no terminal newline. Preserve its bytes, then
    # explicitly separate the first new record for the client's line reader.
    new_lines = card_data[len(baseline) + 2:].splitlines()
    assert len(new_lines) == len(additions)
    assert all(re.fullmatch(rb'\d+#sorry#', line) for line in new_lines)
    before_cards = dict(re.findall(rb'(\d+)#([^#\r\n]+)#', baseline))
    after_cards = dict(re.findall(rb'(\d+)#([^#\r\n]+)#', card_data))
    assert all(key not in before_cards for key in (i.encode() for i in additions))
    assert all(after_cards[key] == value for key, value in before_cards.items())
    assert len(after_cards) == len(before_cards) + len(additions)
    dimensions = {}
    for row in rows:
        raw = (root / row['source']).read_bytes()
        assert hashlib.sha256(raw).hexdigest() == row['sha256']
        key = bytes.fromhex(row['archive_path_hex']).decode('latin1').replace('\\', '/').lower()
        packed, size, flag, offset = index[key]
        with io.BytesIO(archive) as f:
            f.seek(46 + offset)
            assert zlib.decompress(f.read(packed)) == raw
        if row['source'].endswith('.bmp'):
            assert raw[:2] == b'BM' and struct.unpack_from('<I', raw, 2)[0] == len(raw)
            w, h, planes, bpp = struct.unpack_from('<iiHH', raw, 18)
            expected = (24, 24, 8) if '/item/' in row['source'] else (75, 100, 24)
            assert (w, h, bpp) == expected and planes == 1
            dimensions[str(expected)] = dimensions.get(str(expected), 0) + 1
        elif row['source'].endswith('.spr'):
            ground_pair(raw, (root / row['source']).with_suffix('.act').read_bytes())
    asset_report = {'archive_entries': len(rows), 'bitmap_dimensions': dimensions,
                    'ground_pairs': 29, 'native_item_assets': 116, 'fallback_assets': 0,
                    'preserved_card_mappings': len(before_cards),
                    'official_card_illustration_placeholders': len(additions)}
    if args.assets_only:
        print(json.dumps({'mode': 'assets-only', **asset_report}))
        return
    deployed_archive = root / 'client_compat.grf'
    if deployed_archive.exists():
        assert deployed_archive.read_bytes() == archive, 'Built archive does not match source assets'
    ini = configparser.ConfigParser()
    ini.read(args.client / 'DATA.INI')
    effective = dict(index)
    for _, archive_name in sorted(ini['Data'].items(), key=lambda r: int(r[0])):
        for key, value in archive_index(args.client / archive_name).items():
            effective.setdefault(key, value)
    resources = json.loads((root / 'resources.json').read_text())
    by_name = defaultdict(list)
    for key in effective:
        by_name[key.rsplit('/', 1)[-1]].append(key)
    for illustration in additions.values():
        assert any('/cardbmp/' in key for key in by_name[illustration + '.bmp']), illustration
    for row in resources:
        for field in ('identifiedResourceName', 'unidentifiedResourceName'):
            resource = row[field].lower()
            for category, suffix in [('item', '.bmp'), ('collection', '.bmp'), ('sprite', '.spr'), ('sprite', '.act')]:
                assert any(key.endswith('/' + resource + suffix) and
                           (key.startswith('data/sprite/') if category == 'sprite' else '/' + category + '/' in key)
                           for key in by_name[resource + suffix]), (row['id'], category, resource)
    lua = r'''
dofile('SystemEN/itemInfo.lua')
local function clone(v)
  if type(v) ~= 'table' then return v end
  local t = {}; for k,x in pairs(v) do t[k]=clone(x) end; return t
end
local function equal(a,b)
  if type(a) ~= type(b) then return false end
  if type(a) ~= 'table' then return a == b end
  for k,v in pairs(a) do if not equal(v,b[k]) then return false end end
  for k in pairs(b) do if a[k] == nil then return false end end
  return true
end
local before=clone(tbl)
local cardfile=assert(io.open(CARD_TABLE,'rb'))
local carddata=cardfile:read('*a'); cardfile:close()
local function cardnames(text)
  local result={}
  for id,name in text:gmatch('(%d+)#([^#\r\n]+)#') do result[tonumber(id)]=name end
  return result
end
local oldcards=cardnames(carddata:sub(1,CARD_BASELINE_BYTES))
local cards=cardnames(carddata)
local delta=carddata:sub(CARD_BASELINE_BYTES+1)
assert(delta:sub(1,2)=='\r\n', 'Missing card row separator')
local newcount=0
for line in delta:sub(3):gmatch('[^\r\n]+') do
  local id=line:match('^(%d+)#sorry#$')
  assert(tonumber(id)==300761+newcount, 'Invalid card illustration row')
  newcount=newcount+1
end
assert(newcount==23)
local oldcount=0
for id,name in pairs(oldcards) do assert(cards[id]==name); oldcount=oldcount+1 end
for id=300761,300783 do assert(oldcards[id]==nil and cards[id]=='sorry') end
print('preserved_card_mappings='..oldcount..' official_illustration_placeholders=23')
local original_dofile=dofile
function dofile(path)
  if path == 'SystemEN/itemInfo_ClientCompat.lua' then return original_dofile(PATCH) end
  return original_dofile(path)
end
dofile(LOADER)
local count, changed=0,0
for id,item in pairs(tbl) do
  count=count+1; assert(before[id], 'Added item')
  if not equal(item,before[id]) then changed=changed+1 end
  local copy=clone(item)
  copy.identifiedResourceName=before[id].identifiedResourceName
  copy.unidentifiedResourceName=before[id].unidentifiedResourceName
  assert(equal(copy,before[id]), 'Changed item metadata: '..id)
end
for id in pairs(before) do assert(tbl[id], 'Removed item') end
local registered=0
function AddItem(id,...) assert(tbl[id]); registered=registered+1; return true end
function AddItemIdentifiedDesc(id,text) assert(type(text)=='string'); return true end
AddItemUnidentifiedDesc=AddItemIdentifiedDesc
function AddItemEffectInfo() return true end
AddItemIsCostume=AddItemEffectInfo; AddItemPackageID=AddItemEffectInfo
local ok,msg=main(); assert(ok,msg); assert(registered==count)
print('native_items='..count..' changed_resource_records='..changed..' registered='..registered)
'''
    patch_path = (root / 'SystemEN/itemInfo_ClientCompat.lua').as_posix()
    with tempfile.TemporaryDirectory(prefix='client-compat-') as tmp:
        script = Path(tmp) / 'probe.lua'
        loader_path = (root / 'SystemEN/itemInfo.lua').as_posix()
        script.write_text('PATCH=' + json.dumps(patch_path) + '\nLOADER=' + json.dumps(loader_path)
                          + '\nCARD_TABLE=' + json.dumps(card_path.as_posix())
                          + '\nCARD_BASELINE_BYTES=' + str(card_manifest['baseline_bytes']) + '\n' + lua)
        result = subprocess.run([str(args.lua.resolve()), str(script)], cwd=args.client, capture_output=True, check=True)
        print(result.stdout.decode().strip())
    print(json.dumps({'covered_episode_items': len(resources), **asset_report}))


if __name__ == '__main__':
    main()
