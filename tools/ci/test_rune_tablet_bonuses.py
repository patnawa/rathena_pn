#!/usr/bin/env python3
# ============================================================================
#  PN  /  DEVELOPMENT TOOLS
#  test_rune_tablet_bonuses.py
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/test_rune_tablet_bonuses.py
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

"""Focused regression checks for tablet threshold math and native proc lifecycle.

Run: python3 tools/ci/test_rune_tablet_bonuses.py
The timer behavior test compiles the actual pc.cpp implementation with a minimal
timer/session harness; g++ is required. Full map-server loading remains separate.
"""
import json
from pathlib import Path
import re
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
SCRIPT = (ROOT / 'npc/custom/rune_tablet/bonuses.txt').read_text()
CATALOG = json.loads((ROOT / 'npc/custom/rune_tablet/catalog.json').read_text())
CASES = dict((int(k), v) for k, v in re.findall(r'case (\d+):(.*?)(?=\n\s*case |\n\t\})', SCRIPT, re.S))


def bonuses(set_id, count, level):
    result = {}
    for threshold, body in re.findall(r'if \(\.@n >= (\d+)\) \{ (.*?) \}', CASES[set_id]):
        if count < int(threshold):
            continue
        if 'if (.@lv == 15)' in body and level != 15:
            continue
        for cmd, args in re.findall(r'\b(bonus2?) ([^;]+);', body):
            parts = args.split(',')
            expression = parts[-1].replace('.@q3', str(level // 3)).replace('.@q5', str(level // 5)).replace('.@lv', str(level))
            if not re.fullmatch(r'[0-9+*\- ]+', expression):
                raise AssertionError(expression)
            value = eval(expression, {'__builtins__': {}}, {})
            key = tuple(parts[:-1])
            result[key] = result.get(key, 0) + value
    return result


class TabletBonuses(unittest.TestCase):
    def test_episode_groups_follow_existing_mob_names(self):
        expected = {}
        for file in ('db/re/mob_db.yml', 'db/import/mob_db.yml', 'db/import/episode20_mob_db.yml'):
            source = (ROOT/file).read_text()
            for mid, ep in re.findall(r'^\s*- Id: (\d+)\n\s+AegisName: EP(18|19|20|21)_', source, re.M):
                expected[int(mid)] = int(ep)
        overlay = (ROOT/'db/import/rune_tablet_mob_groups.yml').read_text()
        actual = {int(mid): int(ep) for mid, ep in re.findall(r'- Id: (\d+)\n    RaceGroups:\n      PN_EP(\d+): true', overlay)}
        self.assertEqual(actual, expected)
        self.assertEqual(len(actual), 164)

    def test_catalog_coverage_and_membership(self):
        self.assertEqual(set(CASES), {s['id'] for s in CATALOG['sets']})
        self.assertEqual(len(CASES), 57)
        for tablet in CATALOG['sets']:
            case = CASES[tablet['id']]
            members = [1263000 + int(x) for x in re.findall(r'#PNRTPiece\[(\d+)\]', case)]
            self.assertCountEqual(members, tablet['pieces'])
            for level in range(16):
                self.assertFalse(bonuses(tablet['id'], 1, level))
                for count in range(len(members) + 1):
                    bonuses(tablet['id'], count, level)

    def test_cumulative_thresholds_and_floor_scaling(self):
        self.assertEqual(bonuses(1260010, 5, 15), {('bBaseAtk',):75, ('bMatk',):75, ('bMaxHPrate',):7, ('bMaxSPrate',):7})
        self.assertEqual(bonuses(1260020, 6, 14)[('bBaseAtk',)], 22)
        self.assertEqual(bonuses(1260020, 6, 15)[('bBaseAtk',)], 25)
        self.assertEqual(bonuses(1260022, 3, 15)[('bAddClass','Class_Boss')], 40)
        self.assertEqual(bonuses(1260055, 6, 15)[('bShortAtkRate',)], 15)
        self.assertEqual(bonuses(1260055, 6, 15)[('bCritAtkRate',)], 60)
        self.assertEqual(bonuses(1260046, 5, 15)[('bMaxHPrate',)], 20)

    def test_penalties_and_conditional_drains(self):
        self.assertEqual(bonuses(1260026, 2, 0)[('bSubEle','Ele_All')], -15)
        self.assertEqual(bonuses(1260026, 2, 15)[('bSubEle','Ele_All')], 0)
        self.assertEqual(bonuses(1260039, 2, 15)[('bSubEle','Ele_All')], -5)
        for sid in (1260027,1260030,1260031):
            self.assertEqual(bonuses(sid,2,0)[('bUseSPrate',)],5)
            self.assertEqual(bonuses(sid,3,0)[('bUseSPrate',)],0)
        self.assertEqual(bonuses(1260029,2,15)[('bCritical',)],-3)
        self.assertEqual(bonuses(1260029,3,15)[('bCritical',)],7)
        self.assertNotIn(('bHPDrainRate','20'),bonuses(1260011,5,14))
        self.assertEqual(bonuses(1260011,5,15)[('bHPDrainRate','20')],2)
        self.assertEqual(bonuses(1260012,6,15)[('bSPDrainRate','10')],1)
        self.assertFalse(bonuses(1260047,6,15))

    def test_native_regen_timer(self):
        source = (ROOT/'src/map/pc.cpp').read_text()
        code = source[source.index('static TIMER_FUNC(pc_rune_sp_regen_timer)'):source.index('int32 pc_dead(')]
        harness = r'''
#include <cassert>
#include <cstdint>
#include <map>
using t_tick = long long;
constexpr int INVALID_TIMER = -1;
struct map_session_data {
    struct { bool pn_rune_sp_regen_proc = true; } bonus;
    int id = 1, pn_rune_sp_regen_timer = -1, sp = 0;
    bool dead = false;
};
map_session_data session;
map_session_data* online = &session;
long long now = 0;
int next_id = 1;
#define TIMER_FUNC(n) int n(int tid, t_tick tick, int id, intptr_t data)
using callback = int(*)(int,t_tick,int,intptr_t);
struct event { t_tick tick; callback fn; int id; intptr_t data; };
std::map<int,event> events;
map_session_data* map_id2sd(int id) { return online && online->id==id ? online : nullptr; }
bool pc_isdead(map_session_data* sd) { return sd->dead; }
void status_heal(map_session_data* sd,int,int sp,int) { sd->sp += sp; }
t_tick gettick() { return now; }
int add_timer(t_tick tick, callback fn, int id, intptr_t data) {
    int tid = next_id++; events.emplace(tid,event{tick,fn,id,data}); return tid;
}
void delete_timer(int tid, callback) { events.erase(tid); }
void step() {
    assert(!events.empty()); auto it=events.begin(); int tid=it->first;
    event e=it->second; events.erase(it); now=e.tick; e.fn(tid,now,e.id,e.data);
}
'''+code+r'''
int main() {
    pc_rune_sp_regen_start(session);
    for(int i=1;i<=4;i++) { step(); assert(session.sp==200*i); }
    assert(events.empty() && session.pn_rune_sp_regen_timer==-1);
    pc_rune_sp_regen_start(session); step();
    pc_rune_sp_regen_start(session); // refreshing replaces remaining ticks
    assert(events.size()==1);
    for(int i=0;i<4;i++) step();
    assert(session.sp==1800);
    pc_rune_sp_regen_start(session); session.bonus.pn_rune_sp_regen_proc=false;
    step(); assert(session.sp==1800 && events.empty());
    session.bonus.pn_rune_sp_regen_proc=true;
    pc_rune_sp_regen_start(session); pc_rune_sp_regen_clear(session);
    assert(events.empty());
    pc_rune_sp_regen_start(session); session.dead=true;
    step(); assert(session.sp==1800 && events.empty()); session.dead=false;
    pc_rune_sp_regen_start(session); session=map_session_data{}; // relog same ID
    step(); assert(session.sp==0 && events.empty());
    pc_rune_sp_regen_start(session); online=nullptr; // logout
    step(); assert(events.empty());
}
'''
        with tempfile.TemporaryDirectory() as temp:
            cpp, binary = Path(temp)/'test.cpp',Path(temp)/'test'
            cpp.write_text(harness)
            subprocess.run(['g++','-std=c++17','-Wall','-Wextra',str(cpp),'-o',str(binary)],check=True)
            subprocess.run([str(binary)],check=True)


if __name__ == '__main__':
    unittest.main()
