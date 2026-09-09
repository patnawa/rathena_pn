#!/usr/bin/env python3
"""Validate Alice actor name limits, references and phase-safe reward sources."""
from pathlib import Path
import re
import unittest
import yaml
ROOT=Path(__file__).resolve().parents[2]
def read(path):
 return yaml.load((ROOT/path).read_text(encoding='utf-8'),Loader=yaml.CSafeLoader if hasattr(yaml,'CSafeLoader') else yaml.SafeLoader)
class AliceDatabase(unittest.TestCase):
 def test_native_names_and_unique_ids(self):
  actors=read('db/alice_mob_db.yml')['Body']
  self.assertEqual({r['Id'] for r in actors},{30239,30245,30246,30247,30248,30249,30250})
  self.assertEqual(len(actors),7)
  # mob.cpp rejects names >= NAME_LENGTH (24):23 encoded bytes maximum.
  for actor in actors:
   with self.subTest(actor=actor['Id']):
    self.assertLess(len(actor['AegisName'].encode('utf-8')),24)
    self.assertRegex(actor['AegisName'],r'^[A-Z][A-Z0-9_]*$')
 def test_alias_targets_resolve(self):
  actors=read('db/alice_mob_db.yml')['Body'];aliases=read('db/alice_mob_avail.yml')['Body']
  self.assertEqual({r['Mob'] for r in aliases},{r['AegisName'] for r in actors})
  native={r['AegisName'] for r in read('db/re/mob_db.yml')['Body']}
  for alias in aliases:self.assertIn(alias['Sprite'],native)
 def test_boss_currency_not_duplicated_as_phase_drops(self):
  actors={r['Id']:r for r in read('db/alice_mob_db.yml')['Body']}
  for mid in (30239,30250):
   self.assertFalse({'Heavy_Chain','Reptile_Stone'} & {r['Item'] for r in actors[mid].get('Drops',[])})
  self.assertFalse(actors[30249].get('Drops'))
  self.assertEqual(actors[30249]['BaseExp'],0)
 def test_reachable_imports_and_skill_rows(self):
  self.assertIn('db/alice_mob_db.yml',[r['Path'] for r in read('db/mob_db.yml')['Footer']['Imports']])
  self.assertIn('db/alice_mob_avail.yml',[r['Path'] for r in read('db/import-tmpl/mob_avail.yml')['Footer']['Imports']])
  skills={r['Id'] for r in read('db/re/skill_db.yml')['Body']}
  rows=[s.split(',') for s in (ROOT/'db/re/mob_skill_db.txt').read_text().splitlines() if re.match(r'302(39|4[5-9]|50),',s)]
  self.assertEqual(len(rows),9)
  for row in rows:self.assertEqual(len(row),19);self.assertIn(int(row[3]),skills)
if __name__=='__main__':unittest.main()
