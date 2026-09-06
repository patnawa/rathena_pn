"""Structural service wiring checks; client interaction is not simulated."""
from pathlib import Path
import re
import unittest
import yaml
from audit_enchant_upgrades import renewal_records

ROOT = Path(__file__).resolve().parents[2]


class ShadowServiceTests(unittest.TestCase):
    def test_single_renewal_import_and_effective_group(self):
        root = yaml.safe_load((ROOT / 'db/item_enchant.yml').read_text())
        imports = [x for x in root['Footer']['Imports'] if x['Path'] == 'db/import/shadow_group128_enchant.yml']
        self.assertEqual(imports, [{'Path': 'db/import/shadow_group128_enchant.yml', 'Mode': 'Renewal'}])
        self.assertEqual(sum(x['Id'] == 128 for x in renewal_records(ROOT, 'db/item_enchant.yml')), 1)

    def test_service_closes_cancel_and_dialog_before_native_window(self):
        text = (ROOT / 'npc/custom/grademk_services.txt').read_text()
        match = re.search(r'(?ms)^grademk,40,184,4\tscript\tShadow Gear Enchanter#grademk\t4_M_REPAIR,\{\n(.*?)^\}', text)
        self.assertIsNotNone(match)
        body = match[1]
        self.assertEqual(body.count('item_enchant(128);'), 1)
        self.assertEqual(body.count('item_enchant(166);'), 1)
        self.assertIn('select("Open Shadow Enchant:Cancel:M. Alitea Shadow Enchant")', body)
        self.assertRegex(body, r'if \(\.@service == 2\)\s+close;\s+close2;\s+if \(\.@service == 3\) \{\s+item_enchant\(166\);\s+end;\s+\}\s+item_enchant\(128\);\s+end;')
        self.assertIn('retain or lower', body)
        self.assertIn('no reset', body)
        self.assertNotRegex(body, r'\b(?:getitem|delitem|Zeny)\b')

    def test_enabled_file_and_collision_free_coordinate(self):
        config = (ROOT / 'npc/scripts_custom.conf').read_text()
        self.assertEqual(len(re.findall(r'^npc:\s*npc/custom/grademk_services\.txt\s*$', config, re.M)), 1)
        declarations = []
        for path in (ROOT / 'npc').rglob('*.txt'):
            for line in path.read_text(errors='replace').splitlines():
                if line.startswith('grademk,40,184,'):
                    declarations.append(line)
        self.assertEqual(len(declarations), 1)


if __name__ == '__main__':
    unittest.main()
