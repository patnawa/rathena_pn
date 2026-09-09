#!/usr/bin/env python3
# ============================================================================
#  PN  /  DEVELOPMENT TOOLS
#  druid_mentor_test.py
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/druid_mentor_test.py
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

"""Focused source-driven mentor simulations and effective Fashion DB checks.

Uses the restricted interpreter in episode_party_progression_test. Jobchange and
status lookups are test doubles, not compiled pc_jobchange or a real client.
Core save/registry/skill-reset checks below are explicitly source contracts.
Requires PyYAML for the existing Renewal import reader.
"""

from collections import defaultdict
import re
import unittest

from audit_enchant_upgrades import renewal_records
from episode_party_progression_test import (
    ROOT, Instance, Player, Script, ScriptEnd, npc_body, scan_to, statements,
)


MENTOR = ROOT / "npc/custom/druid_mentor.txt"
PC = ROOT / "src/map/pc.cpp"
FASHION = ROOT / "npc/custom/fashion_points/FashionPoints.txt"


def job_constants():
    source = (ROOT / "src/common/mmo.hpp").read_text(encoding="utf-8")
    start = source.index("{", source.index("enum e_job {"))
    enum = source[start + 1:scan_to(source, start, "{", "}")]
    enum = re.sub(r"/\*.*?\*/|//[^\n]*", "", enum, flags=re.S)
    result, value = {}, -1
    for entry in enum.split(","):
        if not entry.strip():
            continue
        match = re.fullmatch(r"\s*(JOB_\w+)(?:\s*=\s*(0x[0-9a-fA-F]+|[0-9]+))?\s*", entry)
        if not match:
            raise ValueError(f"Unsupported job enum entry {entry!r}")
        value = int(match[2], 0) if match[2] else value + 1
        result[match[1]] = value
    return result


JOBS = job_constants()


def function_body(path, name):
    source = path.read_text(encoding="utf-8")
    match = re.search(r"function\s+script\s+" + re.escape(name) + r"\s*\{", source)
    start = match.end() - 1
    return re.sub(r"//[^\n]*", "", source[start + 1:scan_to(source, start, "{", "}")])


def mentor_statements():
    source = npc_body(MENTOR, "Druid Mentor")
    # Inline the argument-free local guard, retaining its actual source logic.
    form = re.search(r"function\s+Check_Form\s*\{", source)
    if form:
        first = form.end() - 1
        last = scan_to(source, first, "{", "}")
        guard = source[first + 1:last].strip()
        if not guard.endswith("return;"):
            raise ValueError("Check_Form must end with a bare return")
        source = source[:form.start()] + source[last + 1:]
        source = source.replace("Check_Form();", guard[:-len("return;")])
    switch = re.search(r"switch\s*\(([^)]*)\)\s*\{", source)
    start = switch.end() - 1
    stop = scan_to(source, start, "{", "}")
    arms, body = {}, source[start + 1:stop]
    labels = list(re.finditer(r"(?m)^\s*(?:case\s+(\w+)|default):", body))
    for index, label in enumerate(labels):
        end = labels[index + 1].start() if index + 1 < len(labels) else len(body)
        code = statements(body[label.end():end])
        if code[-1] == ("command", "break"):
            code.pop()
        elif code[-1] != ("command", "close"):
            raise ValueError("Switch arm lacks a terminating break/close")
        arms[label[1] or "default"] = code
    return statements(source[:switch.start()]) + [("switch", switch[1], arms)] + statements(source[stop + 1:])


class MentorPlayer(Player):
    def __init__(self, job="NOVICE", base=1, level=10, points=0):
        super().__init__()
        self.class_id = JOBS["JOB_" + job]
        self.base, self.level, self.points = base, level, points
        self.statuses = set()
        self.companions = set()
        self.choice = 1
        self.jobchange_succeeds = True
        self.changes, self.effects = [], []


class MentorScript(Script):
    def __init__(self, player):
        self.code = mentor_statements()
        self.name, self.instance, self.player = "Druid Mentor", Instance("", "", 0), player
        self.locals = defaultdict(int)
        self.returned = None
        self.item = {}
        self.argument = 0

    def value(self, expression):
        expression = re.sub(r'\bselect\("(?:\\.|[^"\\])*"\)', str(self.player.choice), expression)
        expression = re.sub(r"\bgetstatus\((SC_\w+)(?:\s*,\s*0)?\)", lambda m: str(int(m[1] in self.player.statuses)), expression)
        expression = re.sub(r"\b(checkfalcon|checkcart|checkriding|ismounting)\(\)", lambda m: str(int(m[1] in self.player.companions)), expression)
        expression = expression.replace("getarg(0)", str(self.argument))
        expression = re.sub(r"getiteminfo\(\.@enchant,ITEMINFO_TYPE\)", repr(self.item.get("Type", "Missing")), expression)
        expression = re.sub(r"getiteminfo\(\.@enchant,ITEMINFO_SUBTYPE\)", repr(self.item.get("SubType", "Missing")), expression)
        # JSON-style quoting avoids treating apostrophes as script variables.
        expression = expression.replace("'Card'", '"Card"').replace("'Enchant'", '"Enchant"').replace("'Etc'", '"Etc"').replace("'Missing'", '"Missing"')
        constants = {**{key: value for key, value in JOBS.items()}, "IT_CARD": '"Card"', "CARD_ENCHANT": '"Enchant"'}
        dynamic = {"Class": self.player.class_id, "BaseLevel": self.player.base,
                   "JobLevel": self.player.level, "SkillPoint": self.player.points}
        parts = re.split(r'("(?:\\.|[^"\\])*")', expression)
        for index in range(0, len(parts), 2):
            def replace(match):
                name = match[0]
                if name in dynamic:
                    return str(dynamic[name])
                return str(constants.get(name.upper(), name))
            parts[index] = re.sub(r"\b(?:Class|BaseLevel|JobLevel|SkillPoint|[Jj][Oo][Bb]_\w+|IT_CARD|CARD_ENCHANT)\b", replace, parts[index])
            parts[index] = re.sub(r"\s+", " ", parts[index])
        return super().value("".join(parts))

    def execute(self, code):
        for statement in code:
            if statement[0] == "switch":
                key = self.value(statement[1])
                arm = next((value for name, value in statement[2].items() if name != "default" and self.value(name) == key), statement[2]["default"])
                yield from self.execute(arm)
            elif statement[0] == "if" and "select(" in statement[1]:
                yield "confirmation"
                if self.value(statement[1]):
                    yield from self.execute(statement[2])
            elif statement[0] == "command" and statement[1].startswith("jobchange "):
                target = self.value(statement[1].split(" ", 1)[1])
                self.player.changes.append(target)
                if self.player.jobchange_succeeds:
                    self.player.class_id, self.player.level = target, 1
            elif statement[0] == "command" and statement[1].startswith("specialeffect2 "):
                self.player.effects.append(statement[1])
            elif statement[0] == "command" and statement[1].startswith("return "):
                self.returned = self.value(statement[1].split(" ", 1)[1])
                raise ScriptEnd
            else:
                yield from super().execute([statement])


class DruidMentorTest(unittest.TestCase):
    transitions = (("NOVICE", 1, 10, "DRUID"), ("DRUID", 99, 70, "KARNOS"), ("KARNOS", 200, 70, "ALITEA"))

    def test_only_intended_class_transitions(self):
        for source, base, level, target in self.transitions:
            with self.subTest(source=source):
                player = MentorPlayer(source, base, level)
                list(MentorScript(player).run())
                self.assertEqual(player.changes, [JOBS["JOB_" + target]])
                self.assertEqual(player.class_id, JOBS["JOB_" + target])
                self.assertEqual(player.level, 1)
                self.assertEqual(len(player.effects), 1)

    def test_below_each_level_boundary_is_rejected(self):
        for source, base, level, _ in self.transitions:
            for tested_base, tested_level in ((base - 1, level), (base, level - 1)):
                with self.subTest(source=source, base=tested_base, level=tested_level):
                    player = MentorPlayer(source, tested_base, tested_level)
                    list(MentorScript(player).run())
                    self.assertEqual(player.changes, [])

    def test_skill_points_must_be_spent(self):
        for source, base, level, _ in self.transitions:
            player = MentorPlayer(source, base, level, 1)
            list(MentorScript(player).run())
            self.assertEqual(player.changes, [])

    def test_cancel_has_no_mutation(self):
        for source, base, level, _ in self.transitions:
            player = MentorPlayer(source, base, level)
            player.choice = 2
            list(MentorScript(player).run())
            self.assertEqual(player.changes, [])
            self.assertEqual(player.effects, [])

    def test_other_jobs_and_already_alitea_are_rejected(self):
        for job in JOBS:
            if job in ("JOB_NOVICE", "JOB_DRUID", "JOB_KARNOS"):
                continue
            with self.subTest(job=job):
                player = MentorPlayer(job[4:], 275, 70)
                list(MentorScript(player).run())
                self.assertEqual(player.changes, [])

    def test_stale_confirmation_rechecks_every_mutable_gate(self):
        for source, base, level, _ in self.transitions:
            for attribute, value in (("class_id", JOBS["JOB_SWORDMAN"]), ("base", base - 1), ("level", level - 1), ("points", 1)):
                with self.subTest(source=source, changed=attribute):
                    player = MentorPlayer(source, base, level)
                    running = MentorScript(player).run()
                    self.assertEqual(next(running), "confirmation")
                    setattr(player, attribute, value)
                    list(running)
                    self.assertEqual(player.changes, [])

    def test_mounted_or_transformed_state_rejected(self):
        for status in ("SC_ALL_RIDING", "SC_WEREWOLF", "SC_WERERAPTOR"):
            for source, base, level, _ in self.transitions:
                with self.subTest(source=source, status=status):
                    player = MentorPlayer(source, base, level)
                    player.statuses.add(status)
                    list(MentorScript(player).run())
                    self.assertEqual(player.changes, [])

    def test_transformation_or_mount_during_confirmation_rejected(self):
        for status in ("SC_ALL_RIDING", "SC_WEREWOLF", "SC_WERERAPTOR"):
            with self.subTest(status=status):
                player = MentorPlayer("KARNOS", 200, 70)
                running = MentorScript(player).run()
                self.assertEqual(next(running), "confirmation")
                player.statuses.add(status)
                list(running)
                self.assertEqual(player.changes, [])

    def test_companions_and_other_mounts_rejected_before_or_after_confirmation(self):
        for companion in ("checkfalcon", "checkcart", "checkriding", "ismounting"):
            for during_dialog in (False, True):
                with self.subTest(companion=companion, during_dialog=during_dialog):
                    player = MentorPlayer("KARNOS", 200, 70)
                    running = MentorScript(player).run()
                    if during_dialog:
                        self.assertEqual(next(running), "confirmation")
                    player.companions.add(companion)
                    list(running)
                    self.assertEqual(player.changes, [])

    def test_engine_rejection_does_not_report_success_effect(self):
        player = MentorPlayer("KARNOS", 200, 70)
        player.jobchange_succeeds = False
        list(MentorScript(player).run())
        self.assertEqual(player.class_id, JOBS["JOB_KARNOS"])
        self.assertEqual(player.effects, [])

    def test_mentor_enabled_and_no_manual_quest_or_point_mutation(self):
        config = (ROOT / "npc/scripts_custom.conf").read_text(encoding="utf-8")
        self.assertRegex(config, r"(?m)^npc: npc/custom/druid_mentor\.txt\s*$")
        body = npc_body(MENTOR, "Druid Mentor")
        self.assertNotRegex(body, r"\b(?:getitem|delitem|setquest|changequest|completequest|resetlvl|resetskill|resetstatus)\b")
        self.assertNotRegex(body, r"\b(?:BaseLevel|JobLevel|SkillPoint|StatusPoint|Zeny|jobchange_level\w*)\s*=(?!=)")

    def test_core_records_job_levels_and_saves_normal_jobchange(self):
        source = PC.read_text(encoding="utf-8")
        start = source.index("{", source.index("bool pc_jobchange("))
        body = source[start + 1:scan_to(source, start, "{", "}")]
        for rank in ("2nd", "3rd"):
            self.assertIn(f"sd->change_level_{rank} = sd->status.job_level;", body)
            self.assertLess(body.index(f"sd->change_level_{rank} = sd->status.job_level;"), body.index("sd->status.job_level=1;"))
            self.assertRegex(source, rf"sd->change_level_{rank} = .*pc_readglobalreg\(sd, add_str\(JOBCHANGE{rank.upper()}_VAR\)\)")
        self.assertIn("pc_calc_skilltree(sd);", body)
        self.assertIn("chrif_save(sd, CSAVE_NORMAL);", body)


class DruidFashionDataTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.items = {}
        for record in renewal_records(ROOT, "db/item_db.yml"):
            cls.items.setdefault(record["Id"], {}).update(record)
        cls.skills = {record["Id"] for record in renewal_records(ROOT, "db/skill_db.yml")}
        cls.jobs = {}
        for record in renewal_records(ROOT, "db/job_stats.yml"):
            for job, enabled in record.get("Jobs", {}).items():
                if enabled:
                    cls.jobs.setdefault(job, {}).update({key: value for key, value in record.items() if key != "Jobs"})

    @staticmethod
    def supported(item, item_id):
        script = MentorScript(MentorPlayer())
        script.code = statements(function_body(FASHION, "FP_EnchantSupported"))
        script.item, script.argument = item, item_id
        list(script.run())
        return script.returned

    def test_all_366_published_pairs_pass_current_type_guard(self):
        body = function_body(FASHION, "FP_LoadBox")
        values = re.findall(r'\.@d\$="([0-9,]+)"', body)
        pairs = []
        for value in values:
            sequence = list(map(int, value.split(",")))
            self.assertEqual(len(sequence) % 2, 0)
            pairs.extend(zip(sequence[::2], sequence[1::2]))
        self.assertEqual(len(pairs), 366)
        for stone, enchant in pairs:
            with self.subTest(stone=stone, enchant=enchant):
                self.assertIn(stone, self.items)
                self.assertEqual(self.supported(self.items[enchant], enchant), 1)

    def test_type_guard_still_rejects_wrong_or_missing_records(self):
        for record in ({}, {"Type": "Etc"}, {"Type": "Card"}, {"Type": "Etc", "SubType": "Enchant"}):
            self.assertEqual(self.supported(record, 314848), 0)

    def test_new_enchants_and_stones_have_correct_effective_metadata(self):
        for stone, enchant in zip(range(1002625, 1002633), range(314848, 314856)):
            with self.subTest(stone=stone):
                self.assertEqual(self.items[stone]["Type"], "Etc")
                self.assertEqual(self.items[enchant]["Type"], "Card")
                self.assertEqual(self.items[enchant]["SubType"], "Enchant")
                self.assertTrue(self.items[enchant]["Script"].strip())

    def test_staged_client_item_info_covers_eight_new_pairs(self):
        info = (ROOT / "client-patch/fashion_points/SystemEN/LuaFiles514/itemInfo_fashion_points.lua").read_text(encoding="utf-8")
        for stone, enchant in zip(range(1002625, 1002633), range(314848, 314856)):
            with self.subTest(stone=stone):
                self.assertRegex(info, rf"\{{{stone},\s*\"")
                self.assertRegex(info, rf"add\({enchant},\s*\"")

    def test_every_new_enchant_and_combo_skill_exists(self):
        scripts = [self.items[item]["Script"] for item in range(314848, 314856)]
        scripts.extend(record["Script"] for record in renewal_records(ROOT, "db/import/fashion_points_item_combos.yml"))
        references = set()
        for script in scripts:
            references.update(map(int, re.findall(r"(?:getskilllv\(|bSkillAtk,)\s*(\d+)", script)))
        self.assertGreater(len(references), 20)
        self.assertEqual(references - self.skills, set())

    def test_mentor_levels_are_reachable_in_effective_job_database(self):
        for job, base, level in (("Novice", 1, 10), ("Druid", 99, 70), ("Karnos", 200, 70)):
            with self.subTest(job=job):
                self.assertGreaterEqual(self.jobs[job]["MaxBaseLevel"], base)
                self.assertGreaterEqual(self.jobs[job]["MaxJobLevel"], level)
        self.assertEqual(self.jobs["Alitea"]["MaxBaseLevel"], 275)


if __name__ == "__main__":
    unittest.main()
