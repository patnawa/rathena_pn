#!/usr/bin/env python3
"""Build a reviewed, offline two-entry Chapter 2 GRF; never install client files.

Requires PyYAML. Sources must match the checked-in manifest. The compiled Lua
lookup is extended, not decompiled/recompiled or replaced with plaintext.
"""
import argparse
import hashlib
import json
import re
import struct
import zlib
from pathlib import Path

from audit_enchant_upgrades import renewal_records
from audit_initial_enchants import server_configuration
from lua51_literal_table import literal_tables


ROOT = Path(__file__).resolve().parents[2]
MANIFEST = ROOT / 'client-patch/chapter2_native/manifest.json'
LUA_HEADER = b'\x1bLua\x51\0\1\4\4\4\x08\0'
LIST_ENTRY = r'data\luafiles514\lua files\Enchant\EnchantList.lub'
NAMES_ENTRY = r'data\luafiles514\lua files\ItemDBNameTbl.lub'
CAUTION = 'Choose carefully: enchant reset is unavailable for this equipment.'


def require(condition, message):
    if not condition:
        raise ValueError(message)


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def normalize(value):
    if isinstance(value, dict):
        return {str(k): normalize(v) for k, v in value.items()}
    if isinstance(value, set):
        return sorted(value)
    if isinstance(value, list):
        return [normalize(v) for v in value]
    return value


def canonical(value):
    return json.dumps(normalize(value), sort_keys=True, separators=(',', ':')).encode('ascii')


class ChunkReader:
    """Bounds-checked reader for the original 32-bit Lua 5.1 prototype layout.

    Raw constant/prototype ranges are retained for byte-exact preservation.
    This parses bytecode; it does not execute arbitrary Lua or decrypt assets.
    """
    def __init__(self, data):
        require(data[:12] == LUA_HEADER, 'Expected original 32-bit little-endian Lua 5.1 header')
        self.data, self.position = data, 12

    def take(self, size):
        require(0 <= size <= len(self.data) - self.position, 'Truncated Lua chunk')
        result = self.data[self.position:self.position + size]
        self.position += size
        return result

    def number(self):
        return struct.unpack('<I', self.take(4))[0]

    def string(self):
        size = self.number()
        if not size:
            return None
        raw = self.take(size)
        require(raw[-1] == 0, 'Unterminated Lua string')
        return raw[:-1].decode('cp949')

    def prototype(self, depth=0):
        require(depth <= 20, 'Unexpected Lua nesting')
        start = self.position
        source = self.string()
        self.take(8)  # Original line range.
        flags = self.take(4)  # Upvalues, parameters, vararg flags, stack size.
        code_position = self.position
        count = self.number()
        code = list(struct.unpack('<' + 'I' * count, self.take(count * 4)))
        constants_count_position = self.position
        constants_count = self.number()
        constants_start = self.position
        constants = []
        for _ in range(constants_count):
            tag = self.take(1)[0]
            if tag == 0:
                value = None
            elif tag == 1:
                value = bool(self.take(1)[0])
            elif tag == 3:
                value = struct.unpack('<d', self.take(8))[0]
            elif tag == 4:
                value = self.string()
            else:
                raise ValueError('Unsupported Lua constant type')
            constants.append(value)
        constants_end = self.position
        children_start = self.position
        children = [self.prototype(depth + 1) for _ in range(self.number())]
        children_end = self.position
        line_count = self.number()
        lines = list(struct.unpack('<' + 'I' * line_count, self.take(line_count * 4)))
        debug_tail_start = self.position
        locals_ = [(self.string(), self.number(), self.number()) for _ in range(self.number())]
        upnames = [self.string() for _ in range(self.number())]
        return {
            'source': source, 'flags': flags, 'code': code, 'constants': constants,
            'children': children, 'lines': lines, 'locals': locals_, 'upnames': upnames,
            'raw': self.data[start:self.position],
            'prefix': self.data[start:code_position],
            'constant_bytes': self.data[constants_start:constants_end],
            'child_bytes': self.data[children_start:children_end],
            'debug_tail': self.data[debug_tail_start:self.position],
            'constant_count_position': constants_count_position,
        }


def parse_chunk(data):
    reader = ChunkReader(data)
    result = reader.prototype()
    require(reader.position == len(data), 'Trailing Lua data')
    return result


def encode_words(values):
    return struct.pack('<I', len(values)) + b''.join(struct.pack('<I', v) for v in values)


def append_name_aliases(original, aliases):
    """Insert literal assignments while preserving every original instruction.

    The known original builds ItemDBNameTbl in R0, then binds it globally and
    declares ItemDB_To_ItemID. Three instructions/alias are inserted before the
    unchanged function declaration; R1/R2 are existing scratch registers.
    """
    old_table = literal_tables(original)
    require(set(old_table) == {'ItemDBNameTbl'}, 'Unexpected original global table')
    old_table = old_table['ItemDBNameTbl']
    require(all(isinstance(k, str) and type(v) is int and v > 0 for k, v in aliases.items()),
            'Aliases must be positive integer identities')
    require(not (old_table.keys() & aliases.keys()), 'Alias name already exists')
    require(not (set(old_table.values()) & set(aliases.values())), 'Alias item ID already exists')
    require(len(set(aliases.values())) == len(aliases), 'Duplicate alias IDs')
    chunk = parse_chunk(original)
    code, constants = chunk['code'], chunk['constants']
    require(chunk['flags'] == bytes([0, 0, 2, 3]), 'Unexpected original stack/upvalue schema')
    require(not chunk['locals'] and not chunk['upnames'], 'Unsupported top-level debug scopes')
    require(len(chunk['lines']) == len(code), 'Expected complete original debug lines')
    require(len(chunk['children']) == 1 and chunk['children'][0]['flags'][0] == 0,
            'Expected one zero-upvalue lookup function')
    insertion = len(code) - 3
    require(code[0] & 63 == 10 and (code[0] >> 6) & 255 == 0, 'Expected R0 literal table')
    for instruction in code[1:insertion]:
        op, a = instruction & 63, (instruction >> 6) & 255
        require((op == 9 and a == 0) or (op == 1 and a in (1, 2)) or
                (op == 7 and a == 0 and constants[instruction >> 14] == 'ItemDBNameTbl'),
                'Unexpected instruction before original function declaration')
    closure, binding, ret = code[insertion:]
    require(closure & 63 == 36 and closure >> 14 == 0 and
            binding & 63 == 7 and ((closure >> 6) & 255) == ((binding >> 6) & 255) and
            constants[binding >> 14] == 'ItemDB_To_ItemID' and ret & 63 == 30 and
            (ret >> 23) & 511 == 1, 'Unexpected lookup declaration/return tail')
    additions, constant_bytes = [], bytearray()
    for index, (name, item_id) in enumerate(sorted(aliases.items())):
        key = len(constants) + index * 2
        require(key + 1 < 262144, 'Lua LOADK constant index overflow')
        raw = name.encode('cp949') + b'\0'
        constant_bytes.extend(b'\x04' + struct.pack('<I', len(raw)) + raw)
        constant_bytes.extend(b'\x03' + struct.pack('<d', item_id))
        additions.extend([1 | (1 << 6) | (key << 14),
                          1 | (2 << 6) | ((key + 1) << 14),
                          9 | (1 << 23) | (2 << 14)])
    new_code = code[:insertion] + additions + code[insertion:]
    new_lines = chunk['lines'][:insertion] + [0] * len(additions) + chunk['lines'][insertion:]
    result = (LUA_HEADER + chunk['prefix'] + encode_words(new_code) +
              struct.pack('<I', len(constants) + 2 * len(aliases)) +
              chunk['constant_bytes'] + constant_bytes + chunk['child_bytes'] +
              encode_words(new_lines) + chunk['debug_tail'])
    rebuilt = parse_chunk(result)
    require(rebuilt['child_bytes'] == chunk['child_bytes'], 'Original function bytes changed')
    require(rebuilt['constant_bytes'].startswith(chunk['constant_bytes']), 'Original constants changed')
    require(rebuilt['code'][:insertion] + rebuilt['code'][insertion + len(additions):] == code,
            'Original instructions changed')
    require(literal_tables(result) == {'ItemDBNameTbl': {**old_table, **aliases}},
            'Independent literal-reader round trip failed')
    return result


def chapter_groups(root, manifest):
    wanted = {int(g) for g in manifest['groups']}
    require(wanted == set(range(167, 172)), 'Manifest group scope changed')
    # The shared comparator intentionally models only supported fields. Refuse
    # additional raw fields here rather than silently omit a new server feature.
    allowed = {'Id', 'TargetItems', 'Order', 'Slots', 'MinimumRefine',
               'MinimumEnchantgrade', 'AllowRandomOptions'}
    raw_records = [r for r in renewal_records(root, 'db/item_enchant.yml') if r['Id'] in wanted]
    require({r['Id'] for r in raw_records} == wanted, 'Missing Chapter 2 server group')
    for record in raw_records:
        require(set(record) <= allowed, 'Unsupported Chapter 2 group field')
        for order in record.get('Order', []):
            require(set(order) == {'Slot'}, 'Unsupported order field')
        for slot in record.get('Slots', []):
            require(set(slot) <= {'Slot', 'PerfectEnchants'}, 'Unsupported Chapter 2 slot feature')
            for recipe in slot.get('PerfectEnchants', []):
                require(set(recipe) <= {'Item', 'Price', 'Materials'}, 'Unsupported perfect recipe field')
                require(0 <= recipe.get('Price', 0) <= 2147483647, 'Price would be clamped')
                for material in recipe.get('Materials', []):
                    require(set(material) <= {'Material', 'Amount'} and
                            1 <= material.get('Amount', 1) <= 30000, 'Unsupported material amount/schema')
    effective = server_configuration(root)
    groups = {g: effective[g] for g in sorted(wanted)}
    require(sha256(canonical(groups)) == manifest['server_groups_sha256'],
            'Effective Chapter 2 server configuration drift; review and repin')
    return groups


def recipe_names(groups):
    names = set()
    for group in groups.values():
        names.update(group['Targets'])
        for slot in group['Slots'].values():
            for name, recipe in slot['Perfect'].items():
                names.add(name)
                names.update(recipe['Materials'])
    return names


def append_enchants(original, groups, item_ids):
    require(not re.search(rb'Table\[(?:167|168|169|170|171)\]', original),
            'Chapter 2 group already present in source list')
    lines = ['', '-- Chapter 2 native recipes: generated from reviewed effective server data.']
    quote = json.dumps
    for group_id, group in sorted(groups.items()):
        target = f'Table[{group_id}]'
        lines += ['', f'{target} = CreateEnchantInfo()',
                  f'{target}:SetSlotOrder(' + ', '.join(map(str, group['Order'])) + ')',
                  f'{target}:SetCondition({group["MinimumRefine"]}, {group["MinimumEnchantgrade"]})',
                  f'{target}:ApproveRandomOption({str(group["AllowRandomOptions"]).lower()})',
                  f'{target}:SetReset(false, 0, 0)',
                  f'{target}:SetCaution({quote(CAUTION)})']
        for name in sorted(group['Targets'], key=item_ids.__getitem__):
            lines.append(f'{target}:AddTargetItem({quote(name)})')
        for slot in group['Order']:
            for name, recipe in sorted(group['Slots'][slot]['Perfect'].items(),
                                       key=lambda pair: item_ids[pair[0]]):
                materials = ', '.join('{' + quote(material) + ', ' + str(amount) + '}'
                                      for material, amount in sorted(recipe['Materials'].items(),
                                                                    key=lambda pair: item_ids[pair[0]]))
                lines.append(f'{target}.Slot[{slot}]:AddPerfectEnchant({quote(name)}, '
                             f'{recipe["Price"]}, {materials})')
    return original + ('\r\n'.join(lines) + '\r\n').encode('ascii')


def make_grf(entries):
    """Deterministic unencrypted GRF v2, compatible with the independent C# reader."""
    data, table = bytearray(), bytearray()
    require(set(entries) == {LIST_ENTRY, NAMES_ENTRY}, 'GRF must contain exactly two approved resources')
    for name, contents in sorted(entries.items()):
        require(not name.startswith(('\\', '/')) and '..' not in name.split('\\'), 'Unsafe GRF path')
        packed = zlib.compress(contents, 9)
        table.extend(name.encode('ascii') + b'\0')
        table.extend(struct.pack('<IIIBI', len(packed), len(packed), len(contents), 1, len(data)))
        data.extend(packed)
    packed_table = zlib.compress(bytes(table), 9)
    header = b'Master of Magic\0' + bytes(14) + struct.pack('<IIII', len(data), 0, len(entries) + 7, 0x200)
    require(len(header) == 46, 'Invalid GRF header size')
    return header + data + struct.pack('<II', len(packed_table), len(table)) + packed_table


def build_payloads(root, enchant_list, item_names, helper, manifest):
    require(manifest['schema'] == 1, 'Unsupported manifest schema')
    require(manifest['client_only_metadata'] == {'caution': CAUTION}, 'Required client caution metadata changed')
    for key, data in [('enchant_list', enchant_list), ('item_names', item_names), ('helper', helper)]:
        require(sha256(data) == manifest['sources'][key + '_sha256'], f'{key} source hash mismatch')
    require(len(re.findall(rb'^Table\[\d+\] = CreateEnchantInfo\(\)', enchant_list, re.M)) ==
            manifest['original_group_count'], 'Unexpected source group count')
    original_names = literal_tables(item_names)['ItemDBNameTbl']
    require(len(original_names) == manifest['original_mapping_count'] == 5208, 'Unexpected original name count')
    aliases, existing = manifest['aliases'], manifest['existing_required_names']
    require(len(aliases) == 84 and existing == {'Ch1_Mana_Ring': 1001996}, 'Reviewed identity scope changed')
    require(all(original_names.get(k) == v for k, v in existing.items()), 'Existing dependency mapping changed')
    groups = chapter_groups(root, manifest)
    required = {**existing, **aliases}
    require(recipe_names(groups) == set(required), 'Recipe identities differ from reviewed manifest')
    server_ids = {}
    for record in renewal_records(root, 'db/item_db.yml'):
        if 'AegisName' in record:
            server_ids[record['Id']] = record['AegisName']
    require(all(server_ids.get(v) == k for k, v in required.items()), 'Server identity drift/collision')
    for group_id, group in groups.items():
        expected = manifest['groups'][str(group_id)]
        require(len(group['Targets']) == expected['targets'] and
                sum(len(s['Perfect']) for s in group['Slots'].values()) == expected['recipes'],
                'Unexpected group target/recipe count')
    entries = {LIST_ENTRY: append_enchants(enchant_list, groups, required),
               NAMES_ENTRY: append_name_aliases(item_names, aliases)}
    archive = make_grf(entries)
    report = {'schema': 1, 'installed': False, 'original_mappings': len(original_names),
              'added_mappings': len(aliases), 'added_groups': sorted(groups),
              'added_targets': 22, 'added_perfect_initial_recipes': 58,
              'server_groups_sha256': sha256(canonical(groups)),
              'sources': manifest['sources'], 'grf_sha256': sha256(archive),
              'client_only_metadata': manifest['client_only_metadata'],
              'entries': {name: {'bytes': len(value), 'sha256': sha256(value)} for name, value in entries.items()},
              'original_lookup_prototype_sha256': sha256(parse_chunk(item_names)['children'][0]['raw']),
              'display_metadata_not_included': [1002700, 1002751, 1002752, 1002753]}
    return entries, archive, report


def output_directory(path, root):
    # The server --root is a read-only input and cannot move the write boundary.
    path, root = path.resolve(), ROOT.resolve()
    require(path.is_relative_to(root.parent) and path != root.parent and not path.is_relative_to(root),
            'Output must be a new sibling artifact directory inside server-work, outside the repository/client')
    require(not path.exists(), 'Output directory exists; refusing to overwrite')
    return path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--enchant-list', type=Path, required=True)
    parser.add_argument('--item-names', type=Path, required=True)
    parser.add_argument('--helper', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--root', type=Path, default=ROOT)
    args = parser.parse_args()
    output = output_directory(args.output, args.root)
    manifest = json.loads(MANIFEST.read_text(encoding='utf-8'))
    entries, archive, report = build_payloads(args.root, args.enchant_list.read_bytes(),
                                             args.item_names.read_bytes(), args.helper.read_bytes(), manifest)
    # Validation completes before any output is created; exclusive writes only.
    output.mkdir(parents=True, exist_ok=False)
    for name, value in entries.items():
        path = output / Path(name.replace('\\', '/'))
        path.parent.mkdir(parents=True, exist_ok=True)
        with path.open('xb') as handle:
            handle.write(value)
    with (output / 'chapter2_native.grf').open('xb') as handle:
        handle.write(archive)
    with (output / 'build-report.json').open('x', encoding='utf-8', newline='\n') as handle:
        json.dump(report, handle, sort_keys=True, indent=2)
        handle.write('\n')
    print(json.dumps(report, sort_keys=True, indent=2))


if __name__ == '__main__':
    main()
