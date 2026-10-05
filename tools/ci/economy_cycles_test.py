"""Independent counterexamples for conservative multi-resource cycle screening."""
from pathlib import Path
import importlib.util
import sys
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from audit_economy_cycles import cycle_core, combined_actions, linear_screen, verify_growth_witness, verify_no_growth_certificate
from economy_vendor_catalog import effective_items, parse_shop_line, vendor_actions


def recipe(key,costs,outputs,zeny=0,repeat='repeatable'):
    return dict(family='fixture',key=key,costs=[dict(item_id=i,amount=n) for i,n in costs],
        outputs=[dict(item_id=i,minimum=n,maximum=n,chance_per_100000=100000) for i,n in outputs],
        zeny_per_batch=zeny,repeat=repeat,source='fixture',line=1)


class EconomyCyclesTest(unittest.TestCase):
    @unittest.skipUnless(importlib.util.find_spec('scipy'),'Optional linear search needs scipy')
    def test_numerical_search_is_backed_by_exact_certificate_or_witness(self):
        items={1:dict(sell=4,nosell=False),2:dict(sell=5,nosell=False)}
        offers=[dict(item_id=1,minimum_cost=10,source='fixture',line=1,shop='A')]
        added=vendor_actions(offers,items,{1,2},0)
        profitable,_=combined_actions([recipe('craft',[(1,1)],[(2,2)])],added)
        result=linear_screen(profitable)
        self.assertEqual(result['verdict'],'combined_model_growth_requires_runtime_review')
        self.assertTrue(verify_growth_witness(profitable,result['witness']['coefficients'])['verified'])
        lossy,_=combined_actions([recipe('craft',[(1,2)],[(2,1)])],added)
        result=linear_screen(lossy)
        self.assertEqual(result['verdict'],'no_resource_growth_in_combined_model')
        self.assertTrue(verify_no_growth_certificate(lossy,result['certificate']['potentials'])['verified'])

    @unittest.skipUnless(importlib.util.find_spec('scipy'),'Optional linear search needs scipy')
    def test_item_only_growth_requires_positive_resource_prices(self):
        actions,_=combined_actions([recipe('free',[],[(1,1)])],[])
        self.assertEqual(linear_screen(actions)['verdict'],'combined_model_growth_requires_runtime_review')

    def test_recipe_and_vendor_combination_exposes_profit_without_direct_arbitrage(self):
        # Buy material 1 for 10, convert it to two item 2, sell each for 6.
        # Neither vendor item can be bought and immediately sold for profit.
        offers=[dict(item_id=1,minimum_cost=10,source='fixture',line=1,shop='A')]
        items={1:dict(sell=4,nosell=False),2:dict(sell=5,nosell=False)}
        actions,_=combined_actions([recipe('craft',[(1,1)],[(2,2)])],vendor_actions(offers,items,{1,2},0))
        self.assertEqual(verify_growth_witness(actions,{'buy:A:1:1':1,'fixture:craft':1,'sell:2':2})['net'],{'zeny':2})

    def test_lossy_combination_has_an_exact_positive_valuation_certificate(self):
        actions,_=combined_actions([recipe('craft',[(1,2)],[(2,1)])],vendor_actions(
            [dict(item_id=1,minimum_cost=10,source='fixture',line=1,shop='A')],
            {1:dict(sell=4,nosell=False),2:dict(sell=5,nosell=False)},{1,2},0))
        self.assertTrue(verify_no_growth_certificate(actions,{'zeny':'1','item:1':'10','item:2':'20'})['verified'])

    def test_invalid_certificate_and_witness_fail_closed(self):
        actions,_=combined_actions([recipe('gain',[(1,1)],[(1,2)])],[])
        for values in ({'item:1':1},{'item:1':0},{'item:1':'nan'},{}):
            self.assertFalse(verify_no_growth_certificate(actions,values)['verified'])
        for witness in ({'fixture:gain':-1},{'unknown':1},{}, {'fixture:gain':'nan'}):
            self.assertFalse(verify_growth_witness(actions,witness)['verified'])

    def test_fractional_witness_is_verified_exactly(self):
        actions,_=combined_actions([recipe('a',[(1,1)],[(2,3)]),recipe('b',[(2,2)],[(1,1)])],[])
        self.assertTrue(verify_growth_witness(actions,{'fixture:a':'2/3','fixture:b':'1'})['verified'])
        self.assertFalse(verify_growth_witness(actions,{'fixture:a':'1/2','fixture:b':'1'})['verified'])

    def test_roundoff_sized_resource_deficit_is_not_accepted_as_growth(self):
        actions=[dict(key='buy',net={'zeny':-1,'item:1':1}),dict(key='sell',net={'zeny':1,'item:1':-1})]
        self.assertFalse(verify_growth_witness(actions,{'buy':'1','sell':'1.000000000000000000000000000001'})['verified'])
        self.assertFalse(verify_no_growth_certificate(actions,
            {'zeny':'1','item:1':'1.000000000000000000000000000001'})['verified'])

    def test_nosell_material_buy_remains_available_for_crafting(self):
        actions=vendor_actions([dict(item_id=1,minimum_cost=10,source='fixture',line=1,shop='A')],
                              {1:dict(sell=100,nosell=True),2:dict(sell=6,nosell=False)},{1,2},0)
        self.assertTrue(any(row['key']=='buy:A:1:1' for row in actions))
        self.assertFalse(any(row['key']=='sell:1' for row in actions))

    def test_price_overlay_uses_final_row_presence_like_native_parser(self):
        items=effective_items([('base',dict(Id=1,Buy=100,Sell=40)),('import',dict(Id=1,Buy=80))])
        self.assertEqual((items[1]['buy'],items[1]['sell']),(80,40))
        items=effective_items([('base',dict(Id=1,Buy=100)),('import',dict(Id=1,Sell=30))])
        self.assertEqual((items[1]['buy'],items[1]['sell']),(60,30))
        items=effective_items([('base',dict(Id=1,Buy=100)),('import',dict(Id=1,Name='unchanged'))])
        self.assertEqual((items[1]['buy'],items[1]['sell']),(100,0))

    def test_null_trade_overlay_preserves_native_restriction(self):
        items=effective_items([('base',dict(Id=1,Trade={'NoSell':True})),('import',dict(Id=1,Trade=None))])
        self.assertTrue(items[1]['nosell'])

    def test_item_price_guard_and_integer_skill_rounding(self):
        items=effective_items([('fixture',dict(Id=1,Buy=10,Sell=10)),('fixture',dict(Id=2,Buy=101))])
        self.assertEqual(items[1]['sell'],1)
        offers=parse_shop_line('-\tshop\tA\tFAKE_NPC,2:-1',items,'fixture',1,1)
        self.assertEqual(offers[0]['minimum_cost'],75)
        actions=vendor_actions([],items,{2},0)
        self.assertEqual(actions[0]['net']['zeny'],62)
        with self.assertRaises(ValueError):
            effective_items([('fixture',dict(Id=1,Sell=2147483647))])

    def test_shop_discount_and_market_stock_are_explicit(self):
        items=effective_items([('fixture',dict(Id=1,Buy=101))])
        regular=parse_shop_line('-\tshop\tA\tFAKE_NPC,no,1:-1',items,'fixture',1,1)[0]
        market=parse_shop_line('-\tmarketshop\tB\tFAKE_NPC,1:-1:0',items,'fixture',2,1)[0]
        self.assertEqual(regular['minimum_cost'],101)
        self.assertEqual(market['minimum_cost'],101)
        self.assertEqual(market['stock'],0)
        self.assertTrue(market['stock_relaxed'])
        with self.assertRaises(ValueError):
            parse_shop_line('-\tmarketshop\tB\tFAKE_NPC,1:5:20:2:5:20',items,'fixture',2,1)

    def test_ordinary_shop_ignored_suffix_is_visible_like_native_prefix_scan(self):
        items=effective_items([('fixture',dict(Id=1,Buy=100))])
        offer=parse_shop_line('-\tshop\tA\tFAKE_NPC,1:-1:0',items,'fixture',1,1)[0]
        self.assertEqual(offer['minimum_cost'],75)
        self.assertEqual(offer['native_ignored_suffix'],':0')

    def test_profitable_closed_conversion_retained(self):
        result=cycle_core([recipe('a',[(1,1)],[(2,2)]),recipe('b',[(2,1)],[(1,1)])])
        self.assertEqual(len(result['candidate_core']),2)
        self.assertEqual(result['verdict'],'requires_runtime_review')

    def test_extra_material_and_currency_costs_break_apparent_cycle(self):
        for extra,zeny in [([(3,1)],0),([],1)]:
            result=cycle_core([recipe('a',[(1,1)]+extra,[(2,2)],zeny),recipe('b',[(2,1)],[(1,1)])])
            self.assertFalse(result['candidate_core'])
            self.assertEqual(len(result['elimination_rounds']),2)

    def test_one_time_reward_cannot_fund_repeatable_cycle(self):
        result=cycle_core([recipe('a',[(1,1)],[(2,2)],repeat='once_per_account'),recipe('b',[(2,1)],[(1,1)])])
        self.assertFalse(result['candidate_core']);self.assertEqual(len(result['excluded']),1)

    def test_rare_maximum_and_byproducts_are_not_discarded(self):
        row=recipe('a',[(1,1)],[(2,1),(3,2)])
        row['outputs'][1].update(minimum=0,chance_per_100000=1)
        result=cycle_core([row,recipe('b',[(3,1)],[(1,1)])])
        self.assertEqual(len(result['candidate_core']),2)

    def test_net_cost_and_lossy_cycles_remain_distinct(self):
        self.assertFalse(cycle_core([recipe('loss',[(1,2)],[(1,1)])])['candidate_core'])
        self.assertTrue(cycle_core([recipe('gain',[(1,1)],[(1,2)])])['candidate_core'])
        # Topological retention alone cannot establish profitable exchange rates.
        result=cycle_core([recipe('a',[(1,2)],[(2,1)]),recipe('b',[(2,2)],[(1,1)])])
        self.assertEqual(result['verdict'],'requires_runtime_review')

    def test_unpriced_repeatable_grant_is_visible(self):
        result=cycle_core([recipe('free',[],[(1,1)])])
        self.assertEqual(result['unpriced'],['fixture:free'])
        self.assertTrue(result['candidate_core'])


if __name__=='__main__':unittest.main()
