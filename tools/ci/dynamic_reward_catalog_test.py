"""Fail-closed output, source, metadata and resource catalog regressions."""
import copy
import json
from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
import dynamic_reward_catalog as catalog


class CatalogTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data=catalog.generate()

    def test_generated_catalog_current(self):
        self.assertEqual(self.data,json.loads((catalog.ROOT/catalog.CATALOG).read_text()))
        result=catalog.validate(self.data)
        self.assertEqual(result['output_ids'],342)
        self.assertEqual(sum(r['family']=='biosphere.material' for r in self.data['recipes']),41)
        self.assertEqual(sum(r['family']=='abyss.conversion' for r in self.data['recipes']),5)

    def test_wrong_output_and_stale_binding_fail(self):
        for mutation in ('output','binding'):
            data=copy.deepcopy(self.data)
            if mutation=='output':data['recipes'][0]['outputs'][0]['item_id']=501
            else:data['source_sha256'][catalog.RUNE]='0'*64
            with self.assertRaisesRegex(ValueError,'Declared outputs'):catalog.validate(data)

    def test_windows_checkout_line_endings_preserve_recipe_identity(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory)
            for name in self.data['source_sha256']:
                target=root/name;target.parent.mkdir(parents=True,exist_ok=True)
                raw=(catalog.ROOT/name).read_bytes().replace(b'\r\n',b'\n')
                target.write_bytes(raw.replace(b'\n',b'\r\n'))
            self.assertEqual(catalog.generate(root),self.data)

    def test_seal_costs_are_explicit_for_economy_analysis(self):
        source=json.loads((catalog.ROOT/catalog.RUNE).read_text())
        rows=[r for r in self.data['recipes'] if r['family']=='rune.seal']
        self.assertEqual({r['key'] for r in rows},{str(i) for i in source['seals']})
        for row in rows:
            self.assertEqual(row['costs'],[{'item_id':int(row['key']),'amount':1}])

    def test_missing_metadata_and_resources_fail(self):
        ids=sorted({o['item_id'] for r in self.data['recipes'] for o in r['outputs']})
        with tempfile.TemporaryDirectory() as directory:
            meta=Path(directory)/'metadata.tsv';triage=Path(directory)/'triage.json'
            meta.write_text('\n'.join(str(i)+'\titem' for i in ids))
            triage.write_text(json.dumps({'catalog_missing_references':[]}))
            self.assertEqual(catalog.validate(self.data,metadata=meta,triage=triage)['client'],'passed')
            meta.write_text('\n'.join(str(i)+'\titem' for i in ids[1:]))
            with self.assertRaisesRegex(ValueError,'Missing client metadata'):catalog.validate(self.data,metadata=meta,triage=triage)
            meta.write_text('\n'.join(str(i)+'\titem' for i in ids))
            triage.write_text(json.dumps({'catalog_missing_references':[{'item_id':ids[0],'path':'missing.bmp'}]}))
            with self.assertRaisesRegex(ValueError,'Missing client resources'):catalog.validate(self.data,metadata=meta,triage=triage)
            triage.write_text('{}')
            with self.assertRaisesRegex(ValueError,'Unknown client'):catalog.validate(self.data,metadata=meta,triage=triage)


if __name__=='__main__':unittest.main()
