#!/usr/bin/env python3
# ============================================================================
#  PN  /  DEVELOPMENT TOOLS
#  npc_audit_20261008_test.py
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/npc_audit_20261008_test.py
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

"""Static regressions for the October 8 NPC audit's critical and high repairs.

Each check pins the ordering that closes one exploit (see
doc/npc_script_audit_20261008.md). These are source-order assertions only; they
do not execute the script VM or replace the native startup gate.
"""
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[2]


def read(path):
    return (ROOT / path).read_text(encoding='utf-8')


def npc(path, name):
    """Body of the script NPC or function `name`, up to its closing column-0 brace."""
    text = read(path)
    match = re.search(r'^[^\n]*\t(?:script(?:\(DISABLED\))?|function\tscript)\t' + re.escape(name) + r'\t[^\n]*\{\n', text, re.M)
    assert match, f'{name} not found in {path}'
    end = text.find('\n}\n', match.end())
    assert end > 0, f'{name} has no closing brace in {path}'
    return text[match.end():end]


def before(body, first, second, label):
    a, b = body.find(first), body.find(second)
    assert a >= 0, f'{label}: missing {first!r}'
    assert b >= 0, f'{label}: missing {second!r}'
    assert a < b, f'{label}: {first!r} must precede {second!r}'


def hall_of_life():
    body = npc('npc/custom/instances/HallOfLife.txt', 'Hall of Life#pn_hol')
    reserve = body[body.find('Use one Hall Key and reserve.'):]
    before(reserve, 'countitem(1001415) < 1', 'delitem 1001415, 1;', 'Hall of Life recheck')
    before(reserve, 'delitem 1001415, 1;', 'instance_create(.@instance$)', 'Hall of Life key consumption')
    failed = reserve[reserve.find('instance_create(.@instance$)'):reserve.find("setinstancevar 'party_id")]
    assert 'getitem 1001415, 1;' in failed, 'Hall of Life failed reservation must refund the key'


def fall_of_glast_heim():
    path = 'npc/custom/instances/FallOfGlastHeim.txt'
    for room, stage in ((1, 2), (2, 4), (3, 6), (4, 8)):
        body = npc(path, f'Oscar#fogh_room{room}')
        guard = f"if ('fogh_stage != {stage})"
        assert body.count(guard) == 2, f'Oscar#fogh_room{room} must recheck its stage after the last pause'
        last_pause = max(body.rfind('next;'), body.rfind('select('))
        assert body.rfind(guard) > last_pause, f'Oscar#fogh_room{room} recheck must follow the last pause'
    text = read(path)
    for label, bit in (('OnSpawnRoom0', 1), ('OnSpawnRoom1', 2), ('OnSpawnRoom2', 4), ('OnSpawnRoom3', 8), ('OnSpawnBoss', 16)):
        handler = text[text.find(label + ':\n'):]
        assert handler.startswith(label), f'{label} missing'
        head = handler[:handler.find('monster ')]
        assert f"'fogh_spawned & {bit}" in head and f"'fogh_spawned |= {bit}" in head, f'{label} must spawn once'


def chapter1_eg03():
    body = npc('npc/custom/chapter1/CH1.c', 'eventtrigger#EG03')
    assert '.active = true' not in body, 'EG03 must not use a boolean cutscene lock'
    before(body, 'setquest 19240;', '.active = gettimetick(2);', 'EG03 progression before shared cutscene')
    assert 'gettimetick(2) - .active < 90' in body, 'EG03 cutscene lock must expire'


def episode19_vellgunde():
    text = read('npc/custom/episode19/quests_19.txt')
    assert 'IG_ICE_F_STONE_BOX2' not in text, 'Undefined item group IG_ICE_F_STONE_BOX2'
    block = text[text.find('Extract the Ice Fire Gem'):]
    before(block, '.@used >= getinventoryslots()', 'delitem 1000608,35;', 'Vellgunde capacity check')
    before(block, 'delitem 1000608,35;', 'getgroupitem(IG_ICE_F_STONE_BOX);', 'Vellgunde grant')


def episode21_cult():
    ivan = npc('npc/custom/episode21/Progression.txt', 'Ivan#ep21_cult')
    assert 'if (!isbegin_quest(17752))' not in ivan, 'Ivan must not restart 17752 after it advanced'
    assert '!callfunc("EP21_QuestInRange",17752,17763)' in ivan
    assert '!EP21_QuestInRange(17752,17763)' in ivan, 'Ivan quest marker must match his gate'
    path = 'npc/custom/episode21/GimliInfiltration.txt'
    maristella = npc(path, 'Maristella Walter#ep21gimli_report')
    before(maristella, 'if (isbegin_quest(17757))', 'changequest 17756,17757;', 'Maristella duplicate report')
    before(maristella, 'isbegin_quest(17757) != 1', 'getitem 1001618,10;', 'Maristella change verification')
    reinhardt = npc(path, 'Reinhardt#ep21gimli_report')
    before(reinhardt, 'isbegin_quest(17769) != 1', 'getitem 1001618,30;', 'Reinhardt change verification')


def episode21_mvps():
    rows = [line for line in read('npc/custom/episode21/FieldMonsters.txt').splitlines()
            if '\tboss_monster\t' in line]
    assert len(rows) == 5, 'Expected five Episode 21 floor MVP spawns'
    for line in rows:
        fields = line.split('\t')[3].split(',')
        assert len(fields) >= 3 and int(fields[2]) >= 3600000, f'MVP needs an explicit respawn: {line}'


def episode20_gates():
    path = 'npc/custom/episode20/Progression.txt'
    text = read(path)
    returns = re.findall(r'^\S+\tscript\t(#ep20_\w+_return)\t', text, re.M)
    assert len(returns) == 9, 'Expected nine Episode 20 return portals'
    for name in returns:
        body = npc(path, name)
        dest = re.search(r'warp "(\w+)"', body).group(1)
        before(body, f'callfunc("EP20_WarperAccess","{dest}")', 'warp "', name)
    travel = text[text.find('Travel to Icy Zone.:Stay.'):]
    before(travel, 'callfunc("EP20_WarperAccess","jor_twice")', 'warp "jor_twice"', 'Diving Iwin travel')


def old_glast_heim_challenge():
    body = npc('npc/custom/instances/OldGlastHeimChallenge.txt', 'Oscar#oghcm_reward')
    assert "'ogh_oscar_claimed[getcharid(3)]" in body.split('setarray')[0], 'Oscar claim must be per instance'
    assert "'ogh_entered[getcharid(0)]" in body.split('setarray')[0], 'Oscar claim requires admission'
    before(body, "'ogh_oscar_claimed[getcharid(3)] = 1;", 'getitem 25864', 'Oscar claim record')


def job_master():
    text = read('npc/custom/jobmaster.txt')
    main = text[text.find('// Begin of the NPC'):]
    before(main, '.@line == EAJ_DRUID || .@line == EAJ_KARNOS', 'Can_Change_Third()', 'Druid line redirect')


def kafra_lottery():
    text = read('npc/cities/aldebaran.txt')
    rolls = [m.start() for m in re.finditer(r'\.@choose_prize = rand\(1,20\);', text)]
    assert len(rolls) == 2, 'Expected two Kafra lottery rolls'
    for roll in rolls:
        preflight = text.rfind(',1)) {', 0, roll)
        assert preflight > 0 and 'F_ReserveLotteryGrant' in text[text.rfind('\n', 0, preflight):preflight], \
            'Every lottery prize must be preflighted before the roll'
        grant = text.find('callfunc("F_ReserveLotteryGrant"', roll)
        between = text[roll:grant]
        assert 'next;' not in between and 'select(' not in between and 'input(' not in between, \
            'Nothing may pause between the lottery roll and its grant'
    grant_fn = npc('npc/cities/aldebaran.txt', 'F_ReserveLotteryGrant')
    before(grant_fn, 'if (.@check_only)', 'reservepurchase(', 'Lottery preflight purchases nothing')


def dylan_reset():
    text = read('npc/re/quests/quests_16_1.txt')
    roll = text.find('.@success = (rand(1,100) < 80);')
    assert roll > 0
    window = text[roll - 600:roll]
    assert 'if (!checkweight(6920,15))' in window, 'Dylan must check room before the roll'
    assert '!.@success && !checkweight' not in text, 'Dylan room check must not depend on the roll'


def engine_guards():
    clif = read('src/map/clif.cpp')
    for handler, call in (('void clif_parse_PurchaseReq(', 'vending_purchasereq('),
                          ('void clif_parse_PurchaseReq2(', 'vending_purchasereq('),
                          ('static void clif_parse_ReqClickBuyingStore(', 'buyingstore_open('),
                          ('static void clif_parse_ReqTradeBuyingStore(', 'buyingstore_trade(')):
        body = clif[clif.find(handler):]
        body = body[:body.find(call)]
        assert 'if( sd->npc_id )' in body, f'{handler} must refuse while an NPC dialog is open'
    script = read('src/map/script.cpp')
    search = script[script.find('static bool buildin_delitem_search('):]
    search = search[:search.find('\n}\n')]
    before(search, 'pc_transaction_locked(sd)', 'delete_items = true;', 'delitem fails closed while locked')


CHECKS = (hall_of_life, fall_of_glast_heim, chapter1_eg03, episode19_vellgunde, episode21_cult,
          episode21_mvps, episode20_gates, old_glast_heim_challenge, job_master, kafra_lottery,
          dylan_reset, engine_guards)

if __name__ == '__main__':
    for check in CHECKS:
        check()
    print(f'NPC_AUDIT_20261008_OK checks={len(CHECKS)}')
