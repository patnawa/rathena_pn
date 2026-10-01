#!/usr/bin/env python3
"""Keep fast Fashion indexes exactly equivalent to the authoritative box lists."""
from pathlib import Path
import json
import re
import unittest
from audit_enchant_upgrades import renewal_records

ROOT=Path(__file__).resolve().parents[2]
SOURCE=(ROOT/'npc/custom/fashion_points/FashionPoints.txt').read_text(encoding='utf8')
CATALOGUE=json.loads((ROOT/'npc/custom/fashion_points/stone_catalogue.json').read_text(encoding='utf8'))

class FashionDataTest(unittest.TestCase):
    def test_lookup_parity(self):
        boxes=[list(map(int,s.split(','))) for s in re.findall(r'\.@d\$="([0-9,]+)"',SOURCE)]
        self.assertEqual(len(boxes),len(CATALOGUE['boxes']))
        stones={stone for box in boxes for stone in box[::2]}
        indexed=set(map(int,re.search(r'return compare\(",([0-9,]+),"',SOURCE)[1].split(',')))
        self.assertEqual(stones,indexed)
        reviewed={entry['stone'] for box in CATALOGUE['boxes'] for entry in box['pairs']}
        self.assertEqual(stones,reviewed)
        self.assertEqual(sum(entry['source']=='legacy' for box in CATALOGUE['boxes'] for entry in box['pairs']),366)
        lookups=re.findall(r'\.@lookup\$=",([0-9=,]+),"',SOURCE)
        categories=[[b['index'] for b in CATALOGUE['boxes'] if b['category']==category] for category in range(10)]
        for category,(indices,lookup) in enumerate(zip(categories+[range(len(boxes))],lookups)):
            expected={}
            for i in indices:
                for stone,enchant in zip(boxes[i][::2],boxes[i][1::2]):expected.setdefault(enchant,stone)
            for entry in CATALOGUE.get('recovery_aliases',[]):
                if category==10 or entry['category']==category:expected.setdefault(entry['enchant'],entry['stone'])
            actual=dict(tuple(map(int,p.split('='))) for p in lookup.split(','))
            self.assertEqual(expected,actual)
        self.assertEqual(len(lookups),11)

    def test_every_stone_has_a_functional_enchant(self):
        items={}
        for record in renewal_records(ROOT,'db/item_db.yml'):items.setdefault(record['Id'],{}).update(record)
        skills={r['Name'] for r in renewal_records(ROOT,'db/skill_db.yml')}
        for box in CATALOGUE['boxes']:
            for entry in box['pairs']:
                with self.subTest(stone=entry['stone']):
                    stone=items[entry['stone']]; enchant=items[entry['enchant']]
                    self.assertIn(stone['Type'],('Etc','Card'))
                    self.assertEqual(enchant['Type'],'Card')
                    self.assertEqual(enchant['SubType'],'Enchant')
                    self.assertTrue(enchant.get('Script','').strip())
                    for skill in re.findall(r'"([A-Z][A-Z0-9_]+)"',enchant['Script']):
                        self.assertIn(skill,skills)
                    if 5<=box['category']<=8:
                        effect=re.search(r'hateffect\s+(\w+),true;',enchant['Script'])
                        self.assertIsNotNone(effect)
                        self.assertIn('hateffect '+effect[1]+',false;',enchant.get('UnEquipScript',''))

    def test_new_combo_references_and_skills_exist(self):
        items={}
        for record in renewal_records(ROOT,'db/item_db.yml'):items.setdefault(record['Id'],{}).update(record)
        names={r['AegisName'] for r in items.values()}
        skills={r['Name'] for r in renewal_records(ROOT,'db/skill_db.yml')}
        for record in renewal_records(ROOT,'db/import/fashion_stone_expansion_combos.yml'):
            for combo in record['Combos']:
                self.assertTrue(set(combo['Combo'])<=names)
            for skill in re.findall(r'"([A-Z][A-Z0-9_]+)"',record['Script']):
                self.assertIn(skill,skills)

    def test_range_middle_corrects_output_without_breaking_existing_costumes(self):
        middle={entry['stone']:entry['enchant'] for box in CATALOGUE['boxes']
                if box['category']==1 for entry in box['pairs']}
        self.assertEqual(middle[25061],310330,
                         'New Range Middle stones must use the enchant referenced by official combos')
        self.assertTrue(any(entry['category']==1 and entry['enchant']==29048
                            and entry['stone']==25061
                            for entry in CATALOGUE.get('recovery_aliases',[])),
                        'Existing Expert Archer costume enchants must remain recoverable')
        items={}
        for record in renewal_records(ROOT,'db/item_db.yml'):items.setdefault(record['Id'],{}).update(record)
        compact=lambda script:re.sub(r'\s+','',script)
        self.assertEqual(compact(items[29048]['Script']),'bonusbLongAtkRate,3;',
                         'Generic Expert Archer must retain its existing base effect')
        self.assertEqual(compact(items[310330]['Script']),'bonusbLongAtkRate,3;')
        aliases={}
        for record in renewal_records(ROOT,'db/import/fashion_stone_expansion_combos.yml'):
            for combo in record['Combos']:
                aliases.setdefault(frozenset(combo['Combo']),[]).append(record['Script'])
        expected={frozenset(('Range_Top','Range_Bottom','Expert_Archer0')):'bonus bLongAtkRate,6;',
                  frozenset(('Range_Robe_D','Expert_Archer0')):'bonus bLongAtkRate,2;',
                  frozenset(('R_PATK_Robe','Expert_Archer0')):'bonus bPAtk,1; bonus bCon,1;'}
        for combo,bonus in expected.items():
            with self.subTest(combo=sorted(combo)):
                self.assertEqual(len(aliases.get(combo,[])),1,
                                 'Each old costume combo needs exactly one compatibility alias')
                self.assertEqual(compact(aliases[combo][0]),
                                 'if(getequipcardid(EQI_COSTUME_HEAD_MID,1)==29048){'+compact(bonus)+'}',
                                 'Legacy bonuses must apply only to the old costume middle slot, not generic Expert Archer elsewhere')

    def test_available_catalogue_and_boxes(self):
        items={}
        for record in renewal_records(ROOT,'db/item_db.yml'):items.setdefault(record['Id'],{}).update(record)
        catalogue=SOURCE[SOURCE.index('script\tFashion Catalogue#FP'):]
        ids=[int(n) for row in re.findall(r'"([0-9][0-9,]+)"',catalogue) for n in row.split(',')]
        installed=[i for i in ids if i in items]
        self.assertGreater(len(installed),40)
        for i in installed:
            self.assertEqual(items[i]['Type'],'Armor')
            self.assertTrue(any(v for k,v in items[i]['Locations'].items() if k.startswith('Costume_')))
        for record in renewal_records(ROOT,'db/import/fashion_points_box_item_db.yml'):
            self.assertEqual(items[record['Id']]['Type'],'Delayconsume')
            self.assertIn('FP_OpenBox',items[record['Id']]['Script'])
        print(f'FASHION_CATALOGUE installed={len(installed)} unavailable={len(ids)-len(installed)} price=150')

if __name__=='__main__':unittest.main()
