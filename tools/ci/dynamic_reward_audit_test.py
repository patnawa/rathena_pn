import importlib.util,sys,unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from audit_dynamic_rewards import candidates,split_args,scopes,grant_sites
class DynamicAuditTests(unittest.TestCase):
 def test_actual_calls_only_with_parentheses_and_rental_variants(self):
  text='x,1,1,1\tscript\tOne\t1,{\nmes "getitem 999,1;";\n.@id=501;\nrentitem2(.@id,60,1,0,0,0,0,0,0);\ngetitembound4 .@unknown,1,1,0,0,0,0,0,0,0,0,0;\n}'
  rows=list(grant_sites(text,{}))
  self.assertEqual([(r['command'],r['line']) for r in rows],[('rentitem2',4),('getitembound4',5)])
  self.assertEqual(rows[0]['candidate_ids'],[501])
  self.assertEqual(rows[1]['unknown_dependencies'],['.@unknown'])
 def test_constant_array_index_does_not_treat_prices_as_items(self):
  f=candidates('setarray .@catalog[0],516,100,7; setarray .@catalog[0],501,1100,7;',{})
  self.assertEqual(f('.@catalog[0]'),({516,501},set()))
 def test_unknown_index_is_explicit_overapproximation(self):
  f=candidates('setarray .@id,501,502; .@item=.@id[.@choice];',{})
  self.assertEqual(f('.@item'),({501,502},set()))
 def test_unknown_argument_is_retained(self):
  f=candidates('.@id=getarg(0);',{})
  self.assertEqual(f('.@id'),(set(),{'getarg(0)'}))
 def test_nested_commas_do_not_split_argument(self):
  self.assertEqual(split_args('.@list[rand(0,3)],max(1,2)'),['.@list[rand(0,3)]','max(1,2)'])
 def test_same_named_variables_in_different_npcs_stay_separate(self):
  source='x,1,1,1\tscript\tOne\t1,{ .@id=501; }\nx,1,1,1\tscript\tTwo\t1,{ .@id=502; }'
  self.assertEqual([candidates(b,{})('.@id')[0] for _,_,b in scopes(source)],[{501},{502}])
 def test_cycles_terminate_as_unknown(self):
  self.assertTrue(candidates('.@id=.@id;',{})('.@id')[1])
if __name__=='__main__':unittest.main()
