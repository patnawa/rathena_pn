#!/usr/bin/env python3
"""Validate the private DB patch and the separate instance warning sequence."""
import importlib.util
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("aquila_patch", ROOT / "tools/apply_aquila_cast_time.py")
patch = importlib.util.module_from_spec(spec)
spec.loader.exec_module(patch)


def row(mob, cast=0):
    return f"{mob},Aquila@NPC_MAXPAIN,attack,716,5,2500,{cast},30000,no,self,always,,,,,,,,\r\n".encode()


class AquilaCastTimeTests(unittest.TestCase):
    def test_only_cast_columns_change_and_repeat_is_safe(self):
        other = b"// retained comment\r\n21531,Aquila@KN_TWOHANDQUICKEN,attack,60,10,1000,0,300000,yes,self,always,,,,,,,,\r\n"
        original = row(21531) + other + row(21588)
        changed = patch.patched_database(original)
        self.assertEqual(changed, row(21531, 2000) + other + row(21588, 2000))
        self.assertEqual(patch.patched_database(changed), changed)

    def test_missing_duplicate_or_custom_rows_fail_closed(self):
        for data in [row(21531), row(21531) + row(21531) + row(21588), row(21531, 4000) + row(21588)]:
            with self.assertRaises(ValueError):
                patch.patched_database(data)

    def test_instance_keeps_two_second_announcement(self):
        text = (ROOT / "npc/custom/episode19/AirshipDestruction.txt").read_text()
        event = text.split("script\tairshipD_NPC_MAXPAIN\t", 1)[1].split("\n}", 1)[0]
        self.assertRegex(event, r'OnTimer5000:\s+unittalk .*?protection process')
        self.assertRegex(event, r'OnTimer7000:\s+unitskilluseid .*?"NPC_MAXPAIN",5;')


if __name__ == "__main__":
    unittest.main()
