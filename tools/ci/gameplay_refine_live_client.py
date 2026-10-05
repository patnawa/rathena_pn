"""Real Fashion NPC/refine packets and SQL persistence in labelled fixtures only.

Run after muhro_gameplay_live_client.py in the disposable game namespace. This
seeds only its synthetic character; it cannot target a general Ragnarok server.
"""
import argparse
import json
import os
import re
import socket
import struct
import subprocess
import time

from bank_live_client import Client, drain

AID, CID = 99000031, 99000032
UID = 9223372036854775701
FIELDS = ('id,nameid,amount,equip,identify,refine,attribute,card0,card1,card2,card3,'
          'option_id0,option_val0,option_parm0,option_id1,option_val1,option_parm1,'
          'option_id2,option_val2,option_parm2,option_id3,option_val3,option_parm3,'
          'option_id4,option_val4,option_parm4,expire_time,favorite,bound,unique_id,'
          'equip_switch,enchantgrade').split(',')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--game', required=True)
    parser.add_argument('--database', required=True)
    args = parser.parse_args()
    game = json.loads(subprocess.check_output(['docker', 'inspect', args.game]))[0]
    database = json.loads(subprocess.check_output(['docker', 'inspect', args.database]))[0]
    assert game['Config']['Labels'].get('pn.wallet64.fixture') == 'true'
    assert database['Config']['Labels'].get('pn.wallet64.fixture') == 'true'
    assert os.readlink('/proc/self/ns/net') == os.readlink('/proc/' + str(game['State']['Pid']) + '/ns/net')

    def sql(query):
        return subprocess.check_output(
            ['docker', 'exec', '-i', '-e', 'MYSQL_PWD=project-validation-only',
             args.database, 'mariadb', '-uroot', '-N', '--batch', 'ragnarok_ci'],
            input=query, text=True).strip()

    assert sql('SELECT DATABASE()') == 'ragnarok_ci'
    assert sql(f'SELECT account_id,name FROM `char` WHERE char_id={CID}') == f'{AID}\tgameplayfixture'
    assert sql('SELECT COUNT(*) FROM `char` WHERE char_id NOT IN (99000012,99000022,99000032)') == '0'

    def offline():
        for _ in range(40):
            if sql(f'SELECT online FROM `char` WHERE char_id={CID}') == '0':
                return
            time.sleep(.25)
        raise AssertionError('QA character did not save and go offline')

    def inventory_count(item):
        return int(sql(f'SELECT COALESCE(SUM(amount),0) FROM inventory WHERE char_id={CID} AND nameid={item}'))

    def costume():
        raw = sql(f'SELECT {",".join(FIELDS)} FROM inventory WHERE char_id={CID} AND nameid=19961')
        assert raw and '\n' not in raw, 'Exactly one costume must persist'
        return dict(zip(FIELDS, map(int, raw.split('\t'))))

    def points():
        return int(sql(f"SELECT value FROM acc_reg_num WHERE account_id={AID} AND `key`='#FP_Fashion' AND `index`=0"))

    offline()
    assert sql(f'SELECT refine FROM inventory WHERE char_id={CID} AND nameid=1501') == '15', 'Run the prior gameplay fixture first'
    prior_club = sql(f'SELECT {",".join(FIELDS)} FROM inventory WHERE char_id={CID} AND nameid=1501')
    prior_coins = inventory_count(50000)
    prior_zeny = int(sql(f'SELECT zeny FROM `char` WHERE char_id={CID}'))
    assert prior_zeny >= 50
    sql(f'''DELETE FROM inventory WHERE char_id={CID} AND nameid IN (19961,25058,1502,1010,501);
INSERT INTO inventory (char_id,nameid,amount,identify,refine,enchantgrade,bound,favorite,card0,option_id0,option_val0,unique_id)
VALUES ({CID},19961,1,1,7,3,1,1,4700,1,23,{UID});
INSERT INTO inventory (char_id,nameid,amount,identify) VALUES ({CID},25058,1,1),({CID},1502,1,1),({CID},1010,1,1);
INSERT INTO acc_reg_num (account_id,`key`,`index`,value) VALUES ({AID},'#FP_Fashion',0,100)
ON DUPLICATE KEY UPDATE value=100;
UPDATE `char` SET inventory_slots=200 WHERE char_id={CID};''')
    baseline = costume()
    cases = []
    sessions = []
    c = None

    def connect():
        nonlocal c
        c = Client(account_id=AID, character_id=CID, username=b'gameplayfixture',
                   password=b'wide-fixture-only', attach=False)

    def send(payload, delay=.35):
        c.world.sendall(payload)
        raw = drain(c.world, delay)
        assert b'GAMEPLAY_REFINE_FAIL' not in raw, raw.hex()
        return raw

    def command(action):
        time.sleep(.3)
        text = ('gameplayfixture : @gameplayrefinefixture ' + action + '\0').encode()
        return send(struct.pack('<HH', 0xf3, len(text) + 4) + text, .7)

    def number(raw, key):
        match = re.search(key.encode() + rb'=(\d+)', raw)
        assert match, (key, raw.hex())
        return int(match[1])

    def report(card, maxhp):
        raw = command('report ' + str(card))
        assert b'GAMEPLAY_REFINE_PASS retained costume metadata' in raw, raw.hex()
        assert number(raw, 'GAMEPLAY_REFINE_MAXHP') == maxhp, 'Visual changes must preserve calculated maximum HP'
        return raw

    def close():
        nonlocal c
        if c is None:
            return
        c.world.setsockopt(socket.SOL_SOCKET, socket.SO_LINGER, struct.pack('ii', 1, 0))
        c.world.close()
        c.char.close()
        c = None
        offline()

    def persisted(card, stone_amount, fashion_points):
        row = costume()
        # char_memitemdata_to_sql matches existing rows using card[3], among
        # other fields. Changing that card deletes/reinserts the SQL row, so
        # its auto-increment id is not the item's identity. Keep reporting id,
        # but assert the persistent unique_id and every gameplay field below.
        for key in FIELDS:
            if key not in ('id', 'card3', 'equip'):
                assert row[key] == baseline[key], (key, baseline[key], row[key])
        assert row['card3'] == card and row['equip'] != 0, row
        assert inventory_count(25058) == stone_amount
        assert points() == fashion_points
        assert inventory_count(50000) == prior_coins
        assert sql(f'SELECT {",".join(FIELDS)} FROM inventory WHERE char_id={CID} AND nameid=1501') == prior_club
        return row

    def prepare(action):
        raw = command(action)
        gid = number(raw, 'GAMEPLAY_REFINE_GID')
        maxhp = number(raw, 'GAMEPLAY_REFINE_MAXHP')
        # QA preparation performs an ordinary map warp; acknowledge map loading
        # before clicking the actual production NPC near the resulting position.
        send(struct.pack('<H', 0x7d), .7)
        return gid, maxhp

    def dialogue(gid, choices):
        raw = send(struct.pack('<HIB', 0x90, gid, 0), .6)
        used = 0
        trace = []
        for _ in range(32):
            actions = []
            for kind in (0xb5, 0xb6, 0xb7):
                start = 0
                while True:
                    at = raw.find(struct.pack('<H', kind), start)
                    if at < 0:
                        break
                    start = at + 2
                    offset = at + (4 if kind == 0xb7 else 2)
                    if len(raw) < offset + 4 or struct.unpack_from('<I', raw, offset)[0] != gid:
                        continue
                    if kind == 0xb7:
                        size = struct.unpack_from('<H', raw, at + 2)[0]
                        if size < 8 or at + size > len(raw):
                            continue
                    actions.append((at, kind))
            if not actions:
                raw += drain(c.world, .7)
                continue
            _, action = min(actions)
            if action == 0xb7:
                assert used < len(choices), ('Unexpected menu', raw.hex())
                choice = choices[used]
                used += 1
                trace.append(['select', choice])
                raw = send(struct.pack('<HIB', 0xb8, gid, choice))
            elif action == 0xb5:
                trace.append(['next'])
                raw = send(struct.pack('<HI', 0xb9, gid))
            else:
                send(struct.pack('<HI', 0x146, gid))
                assert used == len(choices), (used, choices, raw.hex())
                sessions.append(trace)
                return
        raise AssertionError('Production NPC dialogue did not terminate: ' + raw.hex())

    try:
        connect()
        gid, maxhp = prepare('prepare-apply')
        report(0, maxhp)
        dialogue(gid, [6, 1, 1])
        assert number(report(29041, maxhp), 'GAMEPLAY_REFINE_POINTS') == 100
        close()
        applied = persisted(29041, 0, 100)
        cases.append('actual Enchanter click/menus apply visual; logout persists original costume identity and all unrelated metadata')

        # Fill separate rows deliberately: the historical failure requires all
        # 200 slots plus an existing output stack. Insert that stack last.
        occupied = int(sql(f'SELECT COUNT(*) FROM inventory WHERE char_id={CID}'))
        assert occupied < 199
        filler = ','.join(f'({CID},501,1,1)' for _ in range(199 - occupied))
        if filler:
            sql('INSERT INTO inventory (char_id,nameid,amount,identify) VALUES ' + filler)
        sql(f'INSERT INTO inventory (char_id,nameid,amount,identify) VALUES ({CID},25058,7,1)')
        assert sql(f'SELECT COUNT(*) FROM inventory WHERE char_id={CID}') == '200'
        connect()
        gid, restored_maxhp = prepare('prepare-recover')
        assert restored_maxhp == maxhp
        assert number(report(29041, maxhp), 'GAMEPLAY_REFINE_INVENTORY') == 200
        dialogue(gid, [1, 2, 1, 1])
        assert number(report(0, maxhp), 'GAMEPLAY_REFINE_POINTS') == 70
        close()
        recovered = persisted(0, 8, 70)
        assert sql(f'SELECT COUNT(*) FROM inventory WHERE char_id={CID}') == '200'
        cases.append('real Gregio recovery at 200 occupied slots returns exactly one stone, charges 30 points, and persists the unchanged stat card/options/unique ID')

        connect()
        report(0, maxhp)
        cases.append('second relog retains equipped costume metadata and calculated maximum HP after recovery')
        raw = command('refine-open')
        assert struct.pack('<H', 0xaa0) in raw, raw.hex()
        index = number(raw, 'GAMEPLAY_REFINE_INDEX')
        raw = send(struct.pack('<HH', 0xaa1, index))
        at = raw.find(struct.pack('<H', 0xaa2))
        assert at >= 0, raw.hex()
        size, listed_index, blessing = struct.unpack_from('<HHB', raw, at + 2)
        assert listed_index == index and size >= 16 and (size - 7) % 9 == 0
        offers = [struct.unpack_from('<IBI', raw, pos) for pos in range(at + 7, at + size, 9)]
        assert (1010, 100, 50) in offers, offers
        send(struct.pack('<HHIB', 0xaa3, index, 1010, 0), .7)
        # An identical queued request after the single ore is consumed must
        # neither charge again nor advance the refine level a second time.
        send(struct.pack('<HHIB', 0xaa3, index, 1010, 0))
        send(struct.pack('<H', 0xaa4))
        assert b'GAMEPLAY_REFINE_PASS native refine plus one' in command('refine-report')
        close()
        persisted(0, 8, 70)
        assert sql(f'SELECT refine FROM inventory WHERE char_id={CID} AND nameid=1502') == '1'
        assert inventory_count(1010) == 0
        assert int(sql(f'SELECT zeny FROM `char` WHERE char_id={CID}')) == prior_zeny - 50
        cases.append('native refine UI packets and a repeated request produce only +1, consume one Phracon and 50 Zeny; prior +15 Club and coins remain unchanged')
    finally:
        close()
    print(json.dumps({'passed': True, 'cases': cases, 'dialogues': sessions,
                      'maximum_hp': maxhp, 'applied_costume': applied,
                      'recovered_costume': recovered, 'inventory_slots': 200}, indent=2))


if __name__ == '__main__':
    main()
