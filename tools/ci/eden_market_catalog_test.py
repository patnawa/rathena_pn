"""Guard literal Eden market declarations and their replenishment targets."""
from pathlib import Path
import re,unittest
ROOT=Path(__file__).resolve().parents[2]

def markets(text):
    result={}
    for line in text.splitlines():
        fields=line.split('//',1)[0].split('\t')
        if len(fields)<4 or fields[1]!='marketshop':continue
        stock={}
        for token in fields[3].split(',')[1:]:
            token=token.strip()
            if not re.fullmatch(r'\d+:-?\d+:-?\d+',token):
                raise ValueError('Malformed market entry: '+token)
            item,price,quantity=map(int,token.split(':'))
            if item in stock:raise ValueError('Duplicate market item: '+str(item))
            stock[item]=(price,quantity)
        result[fields[2]]=stock
    return result

class EdenMarketCatalogTests(unittest.TestCase):
    def test_restocked_items_exist_at_declared_prices(self):
        text=(ROOT/'npc/re/merchants/eden_market.txt').read_text()
        shops=markets(text)
        self.assertEqual(shops['para_alc10'][7136],(7000,20))
        targets=re.findall(r'npcshopupdate\s+"([^"]+)",\s*(\d+),\s*0,\s*(\d+)\s*;',text)
        self.assertGreater(len(targets),50)
        for name,item,quantity in targets:
            with self.subTest(shop=name,item=item):
                self.assertIn(int(item),shops[name])
                # Opening stock and a later replenishment need not be equal.
                self.assertGreater(int(quantity),0)
    def test_extra_colon_cannot_silently_hide_an_item(self):
        with self.assertRaises(ValueError):
            markets('-\tmarketshop\tprobe\tFAKE_NPC,970:12000:20:7136:7000:20')

if __name__=='__main__':unittest.main()
