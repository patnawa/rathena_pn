"""Focused card-host and spawn-event binding regressions (Python 3 + PyYAML + g++).

The compiled probe executes the current getequipweaponlv builtin and the current
mobskill_use state filter extracted from source, with small local mocks. It is
not a player-attached combat test or a complete model of monster AI.
"""
import os
from pathlib import Path
import re
import subprocess
import tempfile
import unittest

from audit_enchant_upgrades import renewal_records


ROOT = Path(__file__).resolve().parents[2]
CARDS = (300768, 300769, 300770, 300771)


def braced_block(source, marker):
    start = source.index(marker)
    brace = source.index('{', start)
    depth = 0
    for end in range(brace, len(source)):
        depth += (source[end] == '{') - (source[end] == '}')
        if depth == 0:
            return source[start:end + 1]
    raise ValueError(f'Unclosed source block: {marker}')


def effective_skills():
    result = {}
    for path in ('db/re/mob_skill_db.txt', 'db/import/mob_skill_db.txt'):
        for line in (ROOT / path).read_text(encoding='utf-8').splitlines():
            if not line or line.startswith('//'):
                continue
            fields = line.split(',')
            if len(fields) != 19:
                raise ValueError(f'Invalid skill row in {path}: {line}')
            mob_id = int(fields[0])
            if fields[1] == 'clear':
                result.pop(mob_id, None)
            else:
                result.setdefault(mob_id, []).append(fields)
    return result


class CombatBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.items = {}
        for record in renewal_records(ROOT, 'db/item_db.yml'):
            cls.items.setdefault(record['Id'], {}).update(record)
        cls.skills = effective_skills()
        script = (ROOT / 'src/map/script.cpp').read_text(encoding='utf-8')
        mob = (ROOT / 'src/map/mob.cpp').read_text(encoding='utf-8')
        builtin = braced_block(script, 'BUILDIN_FUNC(getequipweaponlv)')
        use = braced_block(mob, 'bool mobskill_use(')
        state_filter = braced_block(use, 'if (ms[i]->state != md->state.skillstate)')
        source = r'''
#include <cassert>
#include <iostream>
#include <vector>
using int32 = int;
constexpr int EQI_COMPOUND_ON = -1, EQI_HAND_L = 0, EQI_HAND_R = 1;
constexpr int IT_WEAPON = 4, IT_ARMOR = 5;
constexpr int SCRIPT_CMD_SUCCESS = 0, SCRIPT_CMD_FAILURE = 1;
struct item_data { int type, weapon_level; };
struct map_session_data { item_data* inventory_data[2]; int equip_index[2]; };
struct ScriptState { std::vector<int> args; int value = -999; };
int current_equip_item_index = -1;
map_session_data player;
int equip_bitmask[2] = {0, 1};
bool script_hasdata(ScriptState* st, int pos) { return int(st->args.size()) > pos - 2; }
int script_getnum(ScriptState* st, int pos) { return st->args.at(pos - 2); }
void script_pushint(ScriptState* st, int value) { st->value = value; }
bool script_charid2sd(int, map_session_data*& sd) { sd = &player; return true; }
bool equip_index_check(int num) { return num >= 0 && num < 2; }
int pc_checkequip(map_session_data* sd, int mask) { return sd->equip_index[mask]; }
#define BUILDIN_FUNC(name) int name(ScriptState* st)
'''
        source += builtin
        source += r'''
enum MobState { MSS_IDLE, MSS_BERSERK, MSS_ANY, MSS_ANYTARGET, MSS_DEAD, MSS_LOOT };
struct SkillRow { MobState state; };
struct Mob { struct { MobState skillstate; } state; int target_id; };
bool state_allows(MobState row, MobState current, bool target) {
    SkillRow value{row};
    SkillRow* ms[1] = {&value};
    Mob actual{{current}, target ? 1 : 0};
    Mob* md = &actual;
    for (int i = 0; i < 1; ++i) {
'''
        source += state_filter
        source += r'''
        return true;
    }
    return false;
}
int main() {
    item_data left{IT_WEAPON, 5}, right{IT_WEAPON, 4}, shield{IT_ARMOR, 0};
    player = {{&left, &right}, {0, 1}};
    auto level = [](int host, std::vector<int> args = {}) {
        current_equip_item_index = host;
        ScriptState st{args};
        getequipweaponlv(&st);
        return st.value;
    };
    assert(level(0) == 5); // Left host Lv5 does not inherit right Lv4.
    assert(level(1) == 4);
    left.weapon_level = 4; right.weapon_level = 5;
    assert(level(0) == 4); // Left host Lv4 does not inherit right Lv5.
    assert(level(1) == 5);
    assert(level(0, {EQI_HAND_R}) == 5); // Explicit right-hand lookup differs.
    player.inventory_data[1] = nullptr;
    assert(level(0) == 4); // An absent other-hand weapon is irrelevant.
    player.inventory_data[0] = &shield;
    assert(level(0) == 0); // Shields/armor do not count as weapons.
    assert(level(-1) == 0); // Missing host fails closed.
    assert(!state_allows(MSS_BERSERK, MSS_IDLE, false));
    assert(state_allows(MSS_IDLE, MSS_IDLE, false));
    assert(state_allows(MSS_ANY, MSS_IDLE, false));
    assert(!state_allows(MSS_ANYTARGET, MSS_IDLE, false));
    assert(!state_allows(MSS_ANY, MSS_DEAD, false));
    std::cout << "13 source-extracted behavioral assertions passed\n";
}
'''
        with tempfile.TemporaryDirectory(prefix='rathena-combat-bindings-') as tmp:
            cpp, binary = Path(tmp) / 'probe.cpp', Path(tmp) / 'probe'
            cpp.write_text(source, encoding='utf-8')
            subprocess.run([os.environ.get('CXX', 'g++'), '-std=c++17', '-Wall', '-Wextra',
                            '-fsanitize=address,undefined', str(cpp), '-o', str(binary)], check=True)
            cls.probe = subprocess.check_output([str(binary)], text=True).strip()

    def test_source_extracted_behavior(self):
        self.assertEqual(self.probe, '13 source-extracted behavioral assertions passed')

    def test_card_level_reads_its_compounded_weapon(self):
        for item_id in CARDS:
            with self.subTest(item_id=item_id):
                script = self.items[item_id]['Script']
                self.assertRegex(script, r'\.@b\s*=\s*\(getequipweaponlv\(\)\s*==\s*5\s*\?\s*20\s*:\s*10\s*\)')
                self.assertNotIn('EQI_HAND_R', script)

    def test_cards_remain_weapon_cards_with_unchanged_effects(self):
        suffixes = {
            300768: 'bonus2 bAddEle,Ele_Fire,.@b; bonus2 bAddRace,RC_Formless,.@b;',
            300769: 'bonus2 bMagicAddEle,Ele_Fire,.@b; bonus2 bMagicAddRace,RC_Formless,.@b;',
            300770: 'bonus2 bAddSize,Size_Medium,.@b; bonus2 bAddSize,Size_Large,.@b;',
            300771: 'bonus2 bMagicAddSize,Size_Medium,.@b; bonus2 bMagicAddSize,Size_Large,.@b;',
        }
        for item_id in CARDS:
            with self.subTest(item_id=item_id):
                item = self.items[item_id]
                self.assertEqual(item['Type'], 'Card')
                self.assertEqual(item['Locations'], {'Right_Hand': True})
                self.assertEqual(item['Script'].strip().split(';', 1)[1].strip(), suffixes[item_id])

    def test_odium_spawn_buff_is_idle_reachable_and_unique(self):
        rows = self.skills[20796]
        spawn_rows = [row for row in rows if row[10] == 'onspawn']
        self.assertEqual(len(spawn_rows), 1)  # No earlier spawn skill consumes the only dispatch.
        row = spawn_rows[0]
        self.assertEqual(row[2], 'idle')
        self.assertEqual(row[3:10], ['349', '2', '2000', '0', '10000', 'yes', 'self'])

    def test_maintained_odium_row_matches_active_overlay(self):
        source_rows = [line for line in (ROOT / 'db/import/thanatos_mob_skill_db.txt').read_text().splitlines()
                       if line.startswith('20796,') and ',onspawn,' in line]
        active_rows = [','.join(row) for row in self.skills[20796] if row[10] == 'onspawn']
        self.assertEqual(source_rows, active_rows)

    def test_spawn_state_is_initialized_before_event_dispatch(self):
        source = (ROOT / 'src/map/mob.cpp').read_text(encoding='utf-8')
        for marker in ('int32 mob_spawn (', 'void mob_revive('):
            with self.subTest(marker=marker):
                block = braced_block(source, marker)
                self.assertLess(block.index('mob_setstate(*md, MSS_IDLE);'),
                                block.index('mobskill_use(md, tick, MSC_SPAWN);'))


if __name__ == '__main__':
    unittest.main()
