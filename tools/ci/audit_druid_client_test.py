"""Reference-table reader regression tests; no client assets or Lua execution."""
import unittest

from audit_enchant_upgrades_test import HEADER, RETURN, abc, abx, prototype
from lua51_literal_table import literal_tables


class ReferenceTableTests(unittest.TestCase):
    constants = ['JTtbl', 'JT_DRUID', 'Druid', 'Names']

    def chunk(self, code):
        return HEADER + prototype(code + [RETURN], self.constants)

    def test_explicit_literal_lookup(self):
        code = [abx(5), abc(6, a=1, b=0, c=257), abc(10, a=2),
                abc(9, a=2, b=1, c=258), abx(7, a=2, bx=3)]
        refs = {'JTtbl': {'JT_DRUID': 4351}}
        self.assertEqual(literal_tables(self.chunk(code), refs), {'Names': {4351: 'Druid'}})
        self.assertEqual(refs, {'JTtbl': {'JT_DRUID': 4351}})

    def test_default_mode_still_rejects_global_reads(self):
        with self.assertRaises(ValueError):
            literal_tables(self.chunk([abx(5)]))

    def test_reference_mutation_rejected(self):
        refs = {'JTtbl': {'JT_DRUID': 4351}}
        with self.assertRaises(ValueError):
            literal_tables(self.chunk([abx(5), abc(9, b=257, c=258)]), refs)
        self.assertEqual(refs['JTtbl']['JT_DRUID'], 4351)

    def test_reference_global_overwrite_rejected(self):
        with self.assertRaises(ValueError):
            literal_tables(self.chunk([abc(10), abx(7)]), {'JTtbl': {}})

    def test_unknown_global_rejected(self):
        with self.assertRaises(KeyError):
            literal_tables(self.chunk([abx(5)]), {})

    def test_call_rejected_even_with_explicit_references(self):
        with self.assertRaises(ValueError):
            literal_tables(self.chunk([abx(5), abc(28, b=1, c=1)]), {'JTtbl': {}})


if __name__ == '__main__':
    unittest.main()
