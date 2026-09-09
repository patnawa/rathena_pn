#!/usr/bin/env python3
# ============================================================================
#  PN  /  DEVELOPMENT TOOLS
#  episode_party_progression_test.py
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/episode_party_progression_test.py
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

"""Source-driven simulations for three episode NPC party/re-entry regressions.

This is a deliberately small, fail-closed interpreter for the selected NPC bodies,
not the rAthena VM. Commands involving maps, inventory, quests, and dialogs use
explicit test doubles. Actual client movement, capacity, and script parsing still
need map-server/client tests. No alternate hard-coded copy of NPC logic is used.
"""

import argparse
import ast
from collections import defaultdict
from pathlib import Path
import re
import subprocess
import sys
import unittest


ROOT = Path(__file__).resolve().parents[2]
EP20 = ROOT / "npc/custom/episode20/Instances.txt"
EP21 = ROOT / "npc/custom/episode21/SecretAltar.txt"
SOURCE_REF = None


def scan_to(text, start, opening, closing):
    depth, quoted, escaped = 0, False, False
    for pos in range(start, len(text)):
        char = text[pos]
        if quoted:
            if escaped:
                escaped = False
            elif char == "\\":
                escaped = True
            elif char == '"':
                quoted = False
        elif char == '"':
            quoted = True
        elif char == opening:
            depth += 1
        elif char == closing:
            depth -= 1
            if depth == 0:
                return pos
    raise ValueError("Unbalanced source")


def npc_body(path, name):
    if SOURCE_REF is None:
        source = path.read_text(encoding="utf-8")
    else:
        source = subprocess.check_output(
            ["git", "show", SOURCE_REF + ":" + path.relative_to(ROOT).as_posix()],
            cwd=ROOT, text=True, encoding="utf-8")
    match = re.search(r"(?m)^.*\tscript(?:\(DISABLED\))?\t" + re.escape(name) + r"\t[^\n]*\{", source)
    if not match:
        raise ValueError(f"Missing NPC {name}")
    start = match.end() - 1
    body = source[start + 1:scan_to(source, start, "{", "}")]
    return re.sub(r"//[^\n]*", "", body)


def statements(source):
    """Parse only if/block and semicolon statements; fail on other control flow."""
    pos, result = 0, []
    while pos < len(source):
        if source[pos].isspace():
            pos += 1
            continue
        condition = re.match(r"if\s*\(", source[pos:])
        if condition:
            start = pos + condition.end() - 1
            stop = scan_to(source, start, "(", ")")
            expression = source[start + 1:stop]
            pos = stop + 1
            while source[pos].isspace():
                pos += 1
            if source[pos] == "{":
                stop = scan_to(source, pos, "{", "}")
                nested = statements(source[pos + 1:stop])
            else:
                stop = source.index(";", pos)
                nested = [("command", source[pos:stop].strip())]
            result.append(("if", expression, nested))
            pos = stop + 1
        else:
            # All selected commands use double-quoted strings. The semicolon
            # separator is ignored inside those strings.
            match = re.match(r'(?:"(?:\\.|[^"\\])*"|[^;"{}])*;', source[pos:])
            if not match:
                raise ValueError(f"Unsupported source near {source[pos:pos + 80]!r}")
            result.append(("command", match.group()[:-1].strip()))
            pos += match.end()
    return result


class ScriptEnd(Exception):
    pass


class Player:
    def __init__(self, active_quest=None, capacity=True):
        self.quests = defaultdict(int)
        if active_quest is not None:
            self.quests[active_quest] = 1
        self.capacity = capacity
        self.items = defaultdict(int)
        self.reputation = 0
        self.experience = (0, 0)
        self.warps = []


class Instance:
    def __init__(self, name, variable, stage):
        self.name = name
        self.variables = defaultdict(int, {variable: stage})
        self.spawned = []
        self.disabled = set()
        self.enabled = set()


class Script:
    def __init__(self, path, name, instance, player):
        self.code = statements(npc_body(path, name))
        self.name, self.instance, self.player = name, instance, player
        self.locals = defaultdict(int)

    def value(self, expression):
        parts = re.split(r'("(?:\\.|[^"\\])*")', expression)
        for index in range(0, len(parts), 2):
            part = re.sub(r"(?:'|\.@)[A-Za-z_][A-Za-z_0-9]*\$?", lambda m: "v(" + repr(m.group()) + ")", parts[index])
            part = part.replace("||", " or ").replace("&&", " and ")
            parts[index] = re.sub(r"!(?!=)", " not ", part)
        transformed = "".join(parts).strip()
        functions = {
            "v": lambda key: (self.instance.variables if key.startswith("'") else self.locals)[key],
            "instance_live_info": lambda _: self.instance.name,
            "instance_mapname": lambda name: name,
            "instance_npcname": lambda name: name,
            "isbegin_quest": lambda qid: self.player.quests[qid],
            "checkweight": lambda *args: self.player.capacity,
            "ILI_NAME": 0,
        }
        tree = ast.parse(transformed, mode="eval")
        allowed = (ast.Expression, ast.Constant, ast.Name, ast.Load, ast.Call,
                   ast.Tuple, ast.BoolOp, ast.Or, ast.And, ast.UnaryOp, ast.Not,
                   ast.Compare, ast.Eq, ast.NotEq, ast.Lt, ast.Gt, ast.LtE,
                   ast.GtE, ast.BinOp, ast.Add)
        for node in ast.walk(tree):
            if not isinstance(node, allowed):
                raise ValueError(f"Unsupported expression node {type(node).__name__}")
            if isinstance(node, ast.Name) and node.id not in functions:
                raise ValueError(f"Unknown expression name {node.id}")
        return eval(compile(tree, "<npc-expression>", "eval"), {"__builtins__": {}}, functions)

    def execute(self, code):
        for statement in code:
            if statement[0] == "if":
                if self.value(statement[1]):
                    yield from self.execute(statement[2])
                continue
            command = statement[1]
            assignment = re.fullmatch(r"((?:'|\.@)[A-Za-z_][A-Za-z_0-9]*\$?)\s*=\s*(.*)", command)
            if assignment:
                name, expression = assignment.groups()
                target = self.instance.variables if name.startswith("'") else self.locals
                target[name] = self.value(expression)
                continue
            name, _, args = command.partition(" ")
            if name == "mes":
                continue
            if name == "close2":
                yield "dialog_closed"
                continue
            if name in ("end", "close"):
                raise ScriptEnd
            if name in ("disablenpc", "disablenpc()"):
                self.instance.disabled.add(self.name)
                continue
            values = self.value("(" + args + ",)")
            if name == "monster":
                self.instance.spawned.append(values)
            elif name == "warp":
                self.player.warps.append(values)
            elif name == "changequest":
                self.player.quests.pop(values[0], None)
                self.player.quests[values[1]] = 1
            elif name == "getitem":
                self.player.items[values[0]] += values[1]
            elif name == "callfunc" and values[0] == "EP21_AddReputation":
                self.player.reputation += values[1]
            elif name == "getexp":
                self.player.experience = tuple(a + b for a, b in zip(self.player.experience, values))
            elif name == "enablenpc":
                self.instance.enabled.add(values[0])
            else:
                raise ValueError(f"Unsupported command {command}")

    def run(self):
        if self.name in self.instance.disabled:
            return
        try:
            yield from self.execute(self.code)
        except ScriptEnd:
            return


class EpisodePartyProgressionTest(unittest.TestCase):
    passages = (
        ("Canyon Passage#ep20", "Canyon Exploration", "'ep20_canyon_stage", ("1@20cn2", 135, 208), 12),
        ("Dimensional Tear#ep20_sanct", "Separated Sanctuary", "'ep20_story_sanct_stage", ("1@twbs", 80, 80), 1),
    )

    def test_scripts_are_enabled(self):
        config = (ROOT / "npc/scripts_custom.conf").read_text(encoding="utf-8")
        for path in (EP20, EP21):
            self.assertRegex(config, r"(?m)^npc: " + re.escape(path.relative_to(ROOT).as_posix()) + r"\s*$")

    def test_passage_blocks_early_or_wrong_instance(self):
        for npc, name, variable, _, _ in self.passages:
            for instance_name, stage in ((name, 0), (name, 1), (name, 5), ("Wrong instance", 2)):
                with self.subTest(npc=npc, name=instance_name, stage=stage):
                    instance, player = Instance(instance_name, variable, stage), Player()
                    list(Script(EP20, npc, instance, player).run())
                    self.assertEqual(player.warps, [])
                    self.assertEqual(instance.spawned, [])

    def test_passage_remains_usable_during_and_after_encounter(self):
        for npc, name, variable, destination, count in self.passages:
            with self.subTest(npc=npc):
                instance = Instance(name, variable, 2)
                for stage in (2, 3, 4, 4):
                    instance.variables[variable] = stage
                    player = Player()
                    list(Script(EP20, npc, instance, player).run())
                    self.assertEqual(player.warps, [destination])
                    self.assertNotIn(npc, instance.disabled)
                    self.assertEqual(sum(mob[5] for mob in instance.spawned), count)

    def test_two_open_dialogs_only_spawn_once(self):
        for npc, name, variable, destination, count in self.passages:
            with self.subTest(npc=npc):
                instance = Instance(name, variable, 2)
                players = (Player(), Player())
                pending = [Script(EP20, npc, instance, player).run() for player in players]
                self.assertEqual([next(run) for run in pending], ["dialog_closed"] * 2)
                for run in pending:
                    list(run)
                self.assertEqual(sum(mob[5] for mob in instance.spawned), count)
                self.assertEqual(instance.variables[variable], 3)
                self.assertEqual([player.warps for player in players], [[destination], [destination]])

    def test_altar_each_party_member_claims_once(self):
        instance = Instance("Secret Altar", "'sa_stage", 8)
        npc = "Giant Egg#ep21_sa"
        for _ in range(3):
            player = Player(18352)
            for _ in range(2):
                list(Script(EP21, npc, instance, player).run())
            self.assertEqual(player.quests[18353], 1)
            self.assertEqual(player.quests[18352], 0)
            self.assertEqual(dict(player.items), {1001618: 60})
            self.assertEqual(player.reputation, 50)
            self.assertEqual(player.experience, (224735905, 55709690))
            self.assertEqual(instance.variables["'sa_stage"], 8)
            self.assertNotIn(npc, instance.disabled)
        self.assertIn("#EP21_SA_Exit", instance.enabled)

    def test_altar_full_inventory_can_retry_after_another_member(self):
        instance = Instance("Secret Altar", "'sa_stage", 8)
        npc, full, other = "Giant Egg#ep21_sa", Player(18352, False), Player(18352)
        list(Script(EP21, npc, instance, full).run())
        self.assertEqual(full.quests[18352], 1)
        self.assertEqual(dict(full.items), {})
        list(Script(EP21, npc, instance, other).run())
        full.capacity = True
        list(Script(EP21, npc, instance, full).run())
        self.assertEqual(full.items[1001618], 60)
        self.assertEqual(other.items[1001618], 60)

    def test_altar_ineligible_visitor_does_not_lock_party_reward(self):
        instance = Instance("Secret Altar", "'sa_stage", 8)
        npc, visitor, eligible = "Giant Egg#ep21_sa", Player(), Player(18352)
        list(Script(EP21, npc, instance, visitor).run())
        list(Script(EP21, npc, instance, eligible).run())
        self.assertEqual(dict(visitor.items), {})
        self.assertEqual(eligible.items[1001618], 60)

    def test_altar_cannot_claim_before_clear(self):
        instance, player = Instance("Secret Altar", "'sa_stage", 7), Player(18352)
        list(Script(EP21, "Giant Egg#ep21_sa", instance, player).run())
        self.assertEqual(player.quests[18352], 1)
        self.assertEqual(dict(player.items), {})


if __name__ == "__main__":
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument("--source-ref", help="Read NPC bodies from a Git revision for a regression baseline")
    arguments, remaining = parser.parse_known_args()
    SOURCE_REF = arguments.source_ref
    unittest.main(argv=[sys.argv[0], *remaining])
