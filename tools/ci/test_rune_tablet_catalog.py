#!/usr/bin/env python3
# ============================================================================
#  PN  /  DEVELOPMENT TOOLS
#  test_rune_tablet_catalog.py
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/test_rune_tablet_catalog.py
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

"""Regression checks for imported Rune Tablet facts and deterministic generation."""
import json
import hashlib
from collections import Counter
from pathlib import Path
import sys
import unittest

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools'))
from generate_rune_tablet_data import amounts, generate, validate
from rune_tablet_catalog import LuaData

class CatalogTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data=json.loads((ROOT/'npc/custom/rune_tablet/catalog.json').read_text())
        cls.sets={s['id']:s for s in cls.data['sets']}

    def test_cost_parser_preserves_parentheses_and_documented_typo(self):
        self.assertEqual(amounts('1T x Printed Glacier Gatling (1001703) 5 x Card (MVP) (4236)'),[[4236,5],[1001703,1]])
        with self.assertRaises(ValueError):amounts('Printed Item (123)')
        with self.assertRaises(ValueError):amounts('5 x Card (4236) unparsed')

    def test_lua_reader_rejects_executable_and_unrecognized_tokens(self):
        self.assertEqual(LuaData('a = { [1] = 2, name = "ok" } -- comment\n').read(),{'a':{1:2,'name':'ok'}})
        for source in ('a = 1 + 2','a = os.execute("bad")','a = { [1]=2,[1]=3 }'):
            with self.assertRaises((ValueError,KeyError)):LuaData(source).read()

    def test_published_scope_and_shared_piece_identity(self):
        self.assertEqual(len(self.sets),57)
        self.assertEqual(len(self.data['pieces']),120)
        self.assertEqual(len(self.data['prints']),136)
        self.assertEqual(self.sets[1260035]['pieces'],self.sets[1260036]['pieces'])
        self.assertEqual(self.sets[1260047]['description'],'')
        self.assertEqual(self.sets[1260047]['max_level'],0)
        self.assertNotIn(1260059,self.sets)

    def test_native_precision_and_reward_id_correction(self):
        self.assertEqual(self.sets[1260012]['base'][13],250)
        self.assertEqual(self.sets[1260002]['rewards']['1'],[[103349,1]])
        self.assertEqual(self.sets[1260002]['rewards']['7'],[[103350,1],[1001282,2]])
        self.assertEqual(self.data['decomposition']['1001595']['30'],[[1001283,30,30,100000]])
        self.assertNotIn('27266',self.data['decomposition'])
        self.assertNotIn('300483',self.data['decomposition'])

    def test_all_reward_totals_match_pinned_native_without_duplicate_claims(self):
        # Independent oracle derived from original RuneSettbl_Reward's nonzero
        # slots for all57sets, before wiki milestone aggregation. Native source
        # hash is recorded in catalog provenance; no client asset is required.
        totals={}
        for s in self.data['sets']:
            counts=Counter()
            for tier,rewards in s['rewards'].items():
                self.assertTrue(int(tier) in (1,7) or 2<=int(tier)<=len(s['pieces']))
                for iid,qty in rewards:counts[iid]+=qty
            totals[s['id']]=sorted(counts.items())
        self.assertEqual(hashlib.sha256(json.dumps(totals,sort_keys=True).encode()).hexdigest(), '59677c905d444a500053f230d598a8996026572fab7ee1b9df25d74791df5ce5')
        self.assertEqual(Counter(s['reward_source'] for s in self.data['sets']),{'published':52,'native_fallback':1,'none':4})
        self.assertEqual(set(self.sets[1260002]['rewards']),{'1','7'})
        self.assertEqual(set(self.sets[1260047]['rewards']),{'1','6','7'})
        self.assertEqual(self.sets[1260047]['rewards']['6'],[[104748,1],[104749,1],[104750,1],[1001283,2]])

    def test_imprint_catalog_includes_currency_and_exact_equipment(self):
        for recipe in self.data['prints']:
            self.assertEqual(recipe['cost'],[[recipe['id'],1],[1001282,10]])

    def test_effective_database_dependencies_and_generated_drift(self):
        self.assertGreater(validate(self.data),1800)
        self.assertEqual(generate(self.data),(ROOT/'npc/custom/rune_tablet/data.txt').read_text())

if __name__=='__main__':unittest.main()
