#!/usr/bin/env python3
"""Generate the NPC's bounded stone tables and exact reverse indexes.

The reviewed catalogue records physical IDs separately from enchant IDs and
costume slot families. Run --check in validation, --write after reviewing data.
"""
import argparse
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
CATALOGUE = ROOT / 'npc/custom/fashion_points/stone_catalogue.json'
NPC = ROOT / 'npc/custom/fashion_points/FashionPoints.txt'


def load_catalogue():
    data = json.loads(CATALOGUE.read_text(encoding='utf-8'))
    seen = set()
    for index, box in enumerate(data['boxes']):
        if box['index'] != index or not 0 <= box['category'] <= 9:
            raise ValueError('Invalid box index/category')
        if not box['pairs'] or len(box['pairs']) > 128:
            raise ValueError('Empty or oversized box: ' + str(index))
        for entry in box['pairs']:
            if entry['stone'] in seen:
                raise ValueError('Duplicate physical stone: ' + str(entry['stone']))
            if entry['source'] not in data['sources']:
                raise ValueError('Missing provenance: ' + str(entry['stone']))
            seen.add(entry['stone'])
    return data


def generated_source(data, source):
    boxes = data['boxes']
    rows = []
    for box in boxes:
        values = ','.join(str(entry[key]) for entry in box['pairs'] for key in ('stone', 'enchant'))
        rows.append(f'\tcase {box["index"]}: .@d$="{values}"; break;')
    source, count = re.subn(r'\tcase 0:\s*\.@d\$=.*?(?=\tdefault: return 0;)',
                            '\n'.join(rows) + '\n', source, count=1, flags=re.S)
    if count != 1:
        raise ValueError('FP_LoadBox table anchor missing')
    stones = sorted(entry['stone'] for box in boxes for entry in box['pairs'])
    known = ('function\tscript\tFP_StoneKnown\t{\n'
             '\treturn compare(",' + ','.join(map(str, stones)) + ',","," + getarg(0) + ",");\n}')
    source, count = re.subn(r'function\tscript\tFP_StoneKnown\t\{.*?\n\}',
                            lambda _: known, source, count=1, flags=re.S)
    if count != 1:
        raise ValueError('FP_StoneKnown anchor missing')
    rows = []
    for category in list(range(10)) + [None]:
        mapping = {}
        for box in boxes:
            if category is None or box['category'] == category:
                for entry in box['pairs']:
                    mapping.setdefault(entry['enchant'], entry['stone'])
        for entry in data.get('recovery_aliases', []):
            if category is None or entry['category'] == category:
                mapping.setdefault(entry['enchant'], entry['stone'])
        values = ','.join(f'{enchant}={stone}' for enchant, stone in mapping.items())
        label = 'default' if category is None else f'case {category}'
        rows.append(f'\t{label}: .@lookup$=",{values},"; break;')
    reverse = '''function\tscript\tFP_StoneFromEnchant\t{
\t.@needle=getarg(0);
\tif (.@needle<1) return 0;
\t// Generated per-family reverse indexes preserve physical stone identity.
\tswitch (getarg(1,-1)) {
ROWS
\t}
\t.@key$="," + .@needle + "=";
\t.@pos=strpos(.@lookup$,.@key$);
\tif (.@pos<0) return 0;
\treturn atoi(substr(.@lookup$,.@pos+getstrlen(.@key$),getstrlen(.@lookup$)-1));
}'''.replace('ROWS', '\n'.join(rows))
    source, count = re.subn(r'function\tscript\tFP_StoneFromEnchant\t\{.*?\n\}',
                            lambda _: reverse, source, count=1, flags=re.S)
    if count != 1:
        raise ValueError('FP_StoneFromEnchant anchor missing')
    return source


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    group = parser.add_mutually_exclusive_group()
    group.add_argument('--write', action='store_true')
    group.add_argument('--check', action='store_true')
    args = parser.parse_args()
    data = load_catalogue()
    original = NPC.read_text(encoding='utf-8')
    generated = generated_source(data, original)
    if args.write:
        NPC.write_text(generated, encoding='utf-8')
    elif original != generated:
        raise SystemExit('NPC stone tables differ from the reviewed catalogue; run --write')
    print(f'FASHION_STONE_CATALOGUE_OK stones={sum(len(b["pairs"]) for b in data["boxes"])} boxes={len(data["boxes"])}')


if __name__ == '__main__':
    main()
