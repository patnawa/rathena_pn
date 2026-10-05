#!/usr/bin/env python3
"""Effective imported equipment properties for the published August fixes."""
from pathlib import Path
from audit_enchant_upgrades import renewal_records

root=Path(__file__).resolve().parents[2]
items={}
for row in renewal_records(root,'db/item_db.yml'):
    items.setdefault(row['Id'],{}).update(row)
for item_id in (2207,2253,2287):
    assert items[item_id]['Refineable'], items[item_id]['AegisName']
assert items[20462]['Locations']=={'Costume_Head_Low':True}
assert items[31682]['Locations']=={'Costume_Head_Mid':True}
print('MUHRO_ITEM_RULES three refinable headgears and two costume positions correct')
monsters={}
for row in renewal_records(root,'db/mob_db.yml'):
    monsters.setdefault(row['Id'],{}).update(row)
for mob_id,card in ((22414,'Yordos_Inve_Card'),(22415,'Yordos_Judge_Card')):
    drops=monsters[mob_id]['Drops']
    matching=[drop for drop in drops if drop['Item']==card]
    assert len(matching)==1 and matching[0]['Rate']==1 and matching[0]['StealProtected']
    assert not any(drop['Item']=='Yortus_Bailiff_Card' for drop in drops)
assert monsters[20891]['Hp']==868754928
assert monsters[21062]['Hp']==1578754928
print('MUHRO_MOB_RULES awakened Yordos cards and rescue-selected boss HP correct')
