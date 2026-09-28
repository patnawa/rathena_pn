#!/usr/bin/env python3
"""Keep fast Fashion indexes exactly equivalent to the authoritative box lists."""
from pathlib import Path
import re
import unittest
from audit_enchant_upgrades import renewal_records

ROOT=Path(__file__).resolve().parents[2]
SOURCE=(ROOT/'npc/custom/fashion_points/FashionPoints.txt').read_text(encoding='utf8')

class FashionDataTest(unittest.TestCase):
    def test_lookup_parity(self):
        boxes=[list(map(int,s.split(','))) for s in re.findall(r'\.@d\$="([0-9,]+)"',SOURCE)]
        self.assertEqual(len(boxes),21)
        stones={stone for box in boxes for stone in box[::2]}
        indexed=set(map(int,re.search(r'return compare\(",([0-9,]+),"',SOURCE)[1].split(',')))
        self.assertEqual(stones,indexed)
        self.assertEqual(len(stones),366)
        lookups=re.findall(r'\.@lookup\$=",([0-9=,]+),"',SOURCE)
        for indices,lookup in zip([range(5),range(5,10),range(10,15),range(15,20),range(20,21),range(21)],lookups):
            expected={}
            for i in indices:
                for stone,enchant in zip(boxes[i][::2],boxes[i][1::2]):expected.setdefault(enchant,stone)
            actual=dict(tuple(map(int,p.split('='))) for p in lookup.split(','))
            self.assertEqual(expected,actual)
        self.assertEqual(len(lookups),6)

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
