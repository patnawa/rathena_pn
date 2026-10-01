"""Exercise source tracing with a small imported database and active NPC fixture."""
import importlib.util
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

spec=importlib.util.spec_from_file_location('audit',Path(__file__).parents[1]/'audit_item_acquisition.py')
audit=importlib.util.module_from_spec(spec);spec.loader.exec_module(audit)

class AcquisitionTest(unittest.TestCase):
    def test_grant_expressions_are_complete_and_not_dialogue(self):
        text = '''// getitem .@comment,1;
mes "Example: getitem 999,1;";
getitembound4 .@bound[.@i],1,1,0,0,0,0,0,0,0,0,0;
rentitem2(.@rental,60,1,0,0,0,0,0,0);
getitem (501 + .@offset),1;
getitem callfunc("reward",1,2),1;
getgroupitem(.@group);
getrandgroupitem IG_TEST;
getitem "Prize",1;
getitem 501,1;
'''
        calls=list(audit.grant_calls(text))
        self.assertEqual([(r['command'],r['expression']) for r in calls], [
            ('getitembound4','.@bound[.@i]'),('rentitem2','.@rental'),
            ('getitem','501 + .@offset'),('getitem','callfunc("reward",1,2)'),
            ('getgroupitem','.@group'),('getrandgroupitem','IG_TEST'),
            ('getitem','"Prize"'),('getitem','501')])
        self.assertEqual([r['line'] for r in calls],list(range(3,11)))

    def test_imported_rewards_called_barter_and_absent_catalog(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory)
            fixtures={
                'db/item_db.yml':'Body: [{Id: 1, AegisName: Box, Script: "getitem .@nested,1;"}, {Id: 2, AegisName: Prize}]\nFooter: {Imports: [{Path: db/extra.yml}]}',
                'db/extra.yml':'Body: [{Id: 3, AegisName: Barter}, {Id: 4, AegisName: Trophy}, {Id: 5, AegisName: Phantom}, {Id: 6, AegisName: Prefix}]',
                'db/item_group_db.yml':'Body: [{Group: TEST, SubGroups: [{SubGroup: 1, List: [{Index: 0, Item: Prize, Rate: 100}]}]}]',
                'db/achievement_db.yml':'Body: [{Id: 1, Rewards: {Item: Trophy, Script: "getitem4 .@prize,1,1,0,0,0,0,0,0,0,0,0;"}}]',
                'npc/re/scripts_main.conf':'npc: npc/test.txt',
                'npc/test.txt':'getitem 1,1; getgroupitem IG_TEST; callshop "Remote Shop",1; // getitem 999,1;\nmes "example: getitem 5,1;";\ngetitembound .@reward,1,1;\nrentitem2 .@rental,60,1,0,0,0,0,0,0;\ngetitem (6 + .@offset),1;\n',
                'npc/barters.yml':'Footer: {Imports: [{Path: npc/custom/barters.yml}]}',
                'npc/custom/barters.yml':'Body: [{Name: Remote Shop, Items: [{Item: Barter}]}]',
                'metadata.tsv':'1\tBox\n2\tPrize\n3\tBarter\n4\tTrophy\n',
                'triage.json':json.dumps({'catalog_missing_references':[{'item_id':i} for i in (1,2,3,4,5,6,999)]})}
            for name,text in fixtures.items():
                path=root/name;path.parent.mkdir(parents=True,exist_ok=True);path.write_text(text)
            report=root/'report.json'
            with patch.object(audit,'REPO',root),patch.object(sys,'argv',['audit','--metadata',str(root/'metadata.tsv'),
                 '--triage',str(root/'triage.json'),'--report',str(report)]):
                audit.main()
            rows={row['item_id']:row for row in json.loads(report.read_text())['items']}
            self.assertEqual({i for i,row in rows.items() if row['source_path_found']},{1,2,3,4})
            result=json.loads(report.read_text())
            self.assertEqual(result['summary']['dynamic_grant_lines_not_resolved'],5)
            self.assertTrue(any(r.get('owner')=='item:1' for r in result['dynamic_grants']))
            self.assertTrue(any(r['source']=='db/achievement_db.yml' for r in result['dynamic_grants']))
            self.assertFalse(rows[999]['in_server_database'])
            self.assertEqual(rows[2]['path'][-1]['kind'],'item group output')
            self.assertEqual(rows[3]['path'][0]['kind'],'configured barter output')

if __name__=='__main__':unittest.main()
