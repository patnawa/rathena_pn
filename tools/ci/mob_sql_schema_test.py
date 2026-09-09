#!/usr/bin/env python3
"""Ensure actual exported monster Attack2 values fit every matching SQL schema."""
from pathlib import Path
import re
import unittest

import yaml

ROOT = Path(__file__).resolve().parents[2]
TABLE_SOURCES = {
    'mob_db': 'db/pre-re/mob_db.yml',
    'mob_db2': 'db/import/mob_db.yml',
    'mob_db_re': 'db/re/mob_db.yml',
    'mob_db2_re': 'db/import/mob_db.yml',
}
WIDTHS = {'tinyint': 8, 'smallint': 16, 'mediumint': 24, 'int': 32, 'bigint': 64}


def attack2_width(source):
    match = re.search(r'`attack2`\s+(tinyint|smallint|mediumint|int|bigint)(?:\(\d+\))?\s+unsigned\s+DEFAULT NULL', source, re.I)
    if not match:
        raise AssertionError('Missing nullable unsigned Attack2 schema')
    return WIDTHS[match[1].lower()]


class MonsterSqlSchema(unittest.TestCase):
    def test_actual_exported_values_fit_templates_and_checked_in_schemas(self):
        checked = 0
        for table, source in TABLE_SOURCES.items():
            records = yaml.load((ROOT/source).read_text(encoding='utf-8-sig'), Loader=yaml.CSafeLoader)['Body']
            for directory in ('doc/yaml/sql', 'sql-files'):
                width = attack2_width((ROOT/directory/(table+'.sql')).read_text())
                for record in records or []:
                    if 'Attack2' in record:
                        with self.subTest(table=table, directory=directory, mob=record['Id']):
                            self.assertGreaterEqual(record['Attack2'], 0)
                            self.assertLess(record['Attack2'], 1 << width)
                            checked += 1
        self.assertGreater(checked, 1000)
        print(f'PASS: {checked} actual Attack2/schema range checks')

    def test_regeneration_preserves_checked_in_column_widths(self):
        for table in TABLE_SOURCES:
            with self.subTest(table=table):
                template = attack2_width((ROOT/'doc/yaml/sql'/(table+'.sql')).read_text())
                installed = attack2_width((ROOT/'sql-files'/(table+'.sql')).read_text())
                self.assertEqual(template, installed)

    def test_upgrade_preserves_unsigned_32_bit_domain_for_all_tables(self):
        migration = (ROOT/'sql-files/upgrades/upgrade_20260909_mob_attack2.sql').read_text()
        updates = re.findall(r'ALTER TABLE `([^`]+)` MODIFY (.*?);', migration)
        self.assertEqual({table for table,_ in updates}, set(TABLE_SOURCES))
        for table, column in updates:
            with self.subTest(table=table):
                self.assertEqual(attack2_width(column), 32)


if __name__ == '__main__':
    unittest.main()
