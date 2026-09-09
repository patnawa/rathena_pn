#!/usr/bin/env python3
# ============================================================================
#  PN  /  DEVELOPMENT TOOLS
#  audit_druid_integration_test.py
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/audit_druid_integration_test.py
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

"""Regression test for the enhanced Roaring Piercer factory identity bug."""
import unittest

from audit_druid_integration import factory_errors


class DruidFactoryTest(unittest.TestCase):
    def test_enhanced_factory_rejects_ordinary_constructor(self):
        factory = 'case AT_ROARING_PIERCER_S: return std::make_unique<SkillRoaringPiercer>();'
        errors, _ = factory_errors(factory, {'SkillRoaringPiercer': 'AT_ROARING_PIERCER'},
                                   {'AT_ROARING_PIERCER_S': {}})
        self.assertEqual(len(errors), 1)
        self.assertIn('carries AT_ROARING_PIERCER', errors[0])

    def test_enhanced_factory_accepts_enhanced_constructor(self):
        factory = 'case AT_ROARING_PIERCER_S: return std::make_unique<SkillRoaringPiercerS>();'
        errors, _ = factory_errors(factory, {'SkillRoaringPiercerS': 'AT_ROARING_PIERCER_S'},
                                   {'AT_ROARING_PIERCER_S': {}})
        self.assertEqual(errors, [])

    def test_missing_factory_rejected(self):
        errors, _ = factory_errors('', {}, {'AT_ROARING_PIERCER_S': {}})
        self.assertEqual(len(errors), 1)

    def test_status_implementation_needs_status(self):
        factory = 'case DR_WEREWOLF: return std::make_unique<StatusSkillImpl>(skill_id, true);'
        self.assertTrue(factory_errors(factory, {}, {'DR_WEREWOLF': {}})[0])
        self.assertFalse(factory_errors(factory, {}, {'DR_WEREWOLF': {'Status': 'Werewolf'}})[0])


if __name__ == '__main__':
    unittest.main()
