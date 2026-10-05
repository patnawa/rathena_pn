"""Read-only PN release audit: GRF integrity/collisions, Lua merges and hygiene.

Run with --client DIR --lua PATH --output REPORT.json. No game/server is started.
Archive winners assume ascending DATA.INI priority; loose-file/client executable
lookup behavior is not proven by this report. Encrypted payloads are unverified.
"""
import argparse
import configparser
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import zlib


def expand(data, expected):
    if expected > 256 * 1024 * 1024:
        raise ValueError('Expanded resource exceeds audit safety limit')
    decoder = zlib.decompressobj()
    raw = decoder.decompress(data, expected + 1)
    if len(raw) != expected or not decoder.eof or decoder.unused_data or decoder.unconsumed_tail:
        raise ValueError('Invalid compressed length or trailing data')
    return raw


def index(path):
    total = path.stat().st_size
    with path.open('rb') as stream:
        header = stream.read(46)
        if len(header) != 46:
            raise ValueError('Truncated GRF header')
        version = struct.unpack_from('<I', header, 42)[0]
        if version == 0x300 and header.startswith(b'Event Horizon\0'):
            offset = struct.unpack_from('<Q', header, 30)[0] + 4
            count = struct.unpack_from('<I', header, 38)[0]
            layout = '<IIIBQ'
        elif version == 0x200 and header.startswith(b'Master of Magic\0'):
            offset, seed, count = struct.unpack_from('<III', header, 30)
            count -= seed + 7
            layout = '<IIIBI'
        else:
            raise ValueError('Unsupported GRF signature/version')
        if count < 0 or not 46 <= 46 + offset <= total - 8:
            raise ValueError('Invalid GRF table bounds/count')
        stream.seek(46 + offset)
        packed, size = struct.unpack('<II', stream.read(8))
        if packed > 256 * 1024 * 1024 or 54 + offset + packed > total:
            raise ValueError('GRF table exceeds file bounds')
        table = expand(stream.read(packed), size)
    rows, pos = [], 0
    for _ in range(count):
        end = table.index(0, pos)
        name = table[pos:end]
        size, aligned, length, flags, start = struct.unpack_from(layout, table, end + 1)
        pos = end + 1 + struct.calcsize(layout)
        # Some writers omit final alignment padding before the index. Only
        # the compressed bytes are read; advertised padding need not be stored.
        if flags & 1 and (size > aligned or start + size > offset):
            raise ValueError('GRF payload overlaps table or has invalid bounds')
        rows.append((name, size, length, flags, start))
    if pos != len(table):
        raise ValueError('Unexpected GRF table bytes')
    return rows


def archive_audit(client):
    ini = configparser.ConfigParser(interpolation=None)
    ini.read(client / 'DATA.INI', encoding='utf-8-sig')
    order = sorted((int(k), v) for k, v in ini['Data'].items())
    if not order or [k for k, _ in order] != list(range(len(order))) or len(order) > 10:
        raise ValueError('Expected contiguous archive slots 0 through at most 9')
    seen_names, effective, collisions, errors, archives = set(), {}, [], [], []
    for _, archive in order:
        if any(c in archive for c in '/\\:') or not archive.lower().endswith('.grf'):
            raise ValueError('Expected a local GRF filename')
        if archive.lower() in seen_names:
            raise ValueError('Duplicate archive filename')
        seen_names.add(archive.lower())
        path = client / archive
        try:
            rows = index(path)
        except (OSError, ValueError, struct.error, zlib.error) as exc:
            errors.append({'archive': archive, 'error': str(exc)})
            continue
        verified = skipped = 0
        local = set()
        with path.open('rb') as stream:
            stream.seek(42)
            version = struct.unpack('<I', stream.read(4))[0]
            for name, size, length, flags, offset in rows:
                if not flags & 1:
                    continue
                key = name.replace(b'/', b'\\').lower()
                digest = None
                if flags & 6:
                    skipped += 1
                else:
                    try:
                        if size > 256 * 1024 * 1024 or length > 256 * 1024 * 1024:
                            raise ValueError('Resource exceeds audit safety limit')
                        stream.seek(46 + offset)
                        packed = stream.read(size)
                        if len(packed) != size:
                            raise ValueError('Truncated payload')
                        # v2 entries are zlib. v3 also permits stored entries.
                        try:
                            raw = expand(packed, length)
                        except (zlib.error, ValueError):
                            if version != 0x300 or size != length:
                                raise
                            raw = packed
                        digest = hashlib.sha256(raw).hexdigest()
                        verified += 1
                    except (zlib.error, ValueError) as exc:
                        errors.append({'archive': archive, 'path_hex': key.hex(), 'error': str(exc)})
                current = {'archive': archive, 'sha256': digest}
                if key in effective:
                    prior = effective[key]
                    collisions.append({'path_hex': key.hex(), 'winner': prior['archive'],
                                       'shadowed': archive,
                                       'identical': digest == prior['sha256'] if digest and prior['sha256'] else None,
                                       'within_archive': key in local})
                else:
                    effective[key] = current
                local.add(key)
        archives.append({'name': archive, 'entries': len(rows), 'verified_payloads': verified,
                         'encrypted_unverified': skipped})
        print(f'{archive}: {verified} payloads verified, {skipped} encrypted', flush=True)
    return {'archives': archives, 'errors': errors, 'collisions': collisions,
            'effective_resource_count': len(effective)}, effective


LUA = r'''
local original_dofile, original_require = dofile, require
local origins, wrapped = {}, nil
local function hex(s)
  return (s:gsub('.', function(c) return string.format('%02x', string.byte(c)) end))
end
local function install()
  if F_itemInfoMerge and F_itemInfoMerge ~= wrapped then
    local merge = F_itemInfoMerge
    wrapped = function(src, overwrite)
      assert(type(src)=='table', 'Missing item merge source table')
      for id in pairs(src) do
        local action = not tbl[id] and 'added' or (overwrite and 'replaced' or 'ignored')
        io.write('MERGE\t',tostring(id),'\t',origins[src] or 'unknown','\t',action,'\n')
      end
      return merge(src, overwrite)
    end
    F_itemInfoMerge = wrapped
  end
end
dofile = function(path)
  local result = original_dofile(path)
  for key, value in pairs(_G) do
    if type(key)=='string' and key:match('^tbl_') and type(value)=='table' and not origins[value] then
      origins[value] = path
    end
  end
  install()
  return result
end
require = function(path)
  local result = original_require(path)
  install()
  return result
end
dofile('SystemEN/itemInfo.lua')
assert(type(tbl)=='table', 'Missing final item table')
for _, name in ipairs(ImportTables or {}) do
  assert(type(_G['tbl_'..name])=='table', 'Missing declared table: '..name)
end
for id, row in pairs(tbl) do
  assert(type(id)=='number' and type(row)=='table', 'Invalid item record')
  for _, field in ipairs({'identifiedResourceName','unidentifiedResourceName'}) do
    assert(type(row[field])=='string', 'Missing '..field..' for '..id)
    if id ~= 0 then -- base translation's empty sentinel is not an artwork item
      io.write('RESOURCE\t',tostring(id),'\t',field,'\t',hex(row[field]),'\n')
    end
  end
end
local count, registered = 0, {}
AddItem = function(id, ...)
  assert(tbl[id], 'Registered unknown item')
  assert(not registered[id], 'Registered duplicate item')
  registered[id] = true
  count = count + 1
  return true
end
AddItemUnidentifiedDesc = function(...) return true end
AddItemIdentifiedDesc = function(...) return true end
AddItemEffectInfo = function(...) return true end
AddItemIsCostume = function(...) return true end
AddItemPackageID = function(...) return true end
assert(type(main)=='function', 'Missing client registration callback')
assert(main(), 'Client item registration failed')
local expected=0;for _ in pairs(tbl) do expected=expected+1 end
assert(count==expected, 'Not all items registered')
io.write('REGISTERED\t',tostring(count),'\n')
'''


def lua_audit(client, lua, effective):
    run = subprocess.run([str(lua.resolve()), '-'], input=LUA.encode(), cwd=client,
                         capture_output=True, timeout=120)
    if run.returncode:
        raise ValueError(run.stderr.decode(errors='replace'))
    merges, missing, registered = [], [], None
    texture = bytes.fromhex('646174615c746578747572655cc0afc0fac0cec5cdc6e4c0ccbdba5c')
    for line in run.stdout.decode('ascii').splitlines():
        fields = line.split('\t')
        if fields[0] == 'MERGE':
            merges.append(dict(zip(('item_id', 'source', 'action'), fields[1:])))
        elif fields[0] == 'REGISTERED':
            registered = int(fields[1])
        elif fields[0] == 'RESOURCE':
            resource = bytes.fromhex(fields[3])
            key = (texture + b'item\\' + resource + b'.bmp').lower()
            if key not in effective:
                missing.append({'item_id': int(fields[1]), 'field': fields[2], 'path_hex': key.hex()})
    if registered is None:
        raise ValueError('Lua did not report registration count')
    return {'registered_items': registered, 'merge_decisions': merges,
            'missing_archive_icons': missing,
            'scope': 'All final item icons against archive index; loose files/fallbacks and obtainability require separate review. Merge provenance excludes direct field edits.'}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--client', type=Path, required=True)
    parser.add_argument('--lua', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    report = {'client': str(args.client.resolve()), 'passed': False}
    try:
        report['grf'], effective = archive_audit(args.client)
        report['lua'] = lua_audit(args.client, args.lua, effective)
        report['hygiene_review'] = [p.relative_to(args.client).as_posix()
                                    for p in sorted((args.client / 'AI/USER_AI/data').glob('H_*'))]
        report['passed'] = not report['grf']['errors'] and not any(
            row['encrypted_unverified'] for row in report['grf']['archives'])
        report['boundary'] = 'Integrity/runtime loader pass only; collisions, missing icon candidates and hygiene require review. No gameplay or server validation.'
    except (OSError, ValueError, KeyError, struct.error, configparser.Error, subprocess.SubprocessError) as exc:
        report['error'] = str(exc)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({'passed': report['passed'], 'error': report.get('error'), 'report': str(args.output)}))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
