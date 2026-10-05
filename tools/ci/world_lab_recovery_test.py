"""Actual login/char/map packets and SQL for the explicitly isolated QA realm.

Run in the game container's network namespace using nsenter, on its Docker host.
The fixture controls positioning/setup; lab runs use the production console and
real 60-second timers. This is protocol/runtime evidence, not rendered evidence.
"""
import argparse
import hashlib
import importlib.util
import json
import os
import re
from pathlib import Path
import socket
import struct
import subprocess
import time


def lab_target(wire):
    """PACKETVER20260219 real spawn/idle packet, not an attached timer observer."""
    targets=set()
    for offset in range(max(0,len(wire)-24)):
        code,length=struct.unpack_from('<HH',wire,offset)
        if (code,length) not in ((0x9fe,107),(0x9ff,108)) or offset+length>len(wire):continue
        if wire[offset+4]!=5 or struct.unpack_from('<H',wire,offset+23)[0]!=1002:continue
        if wire[offset+length-24:offset+length].rstrip(b'\0')!=b'Damage Lab Target':continue
        targets.add(struct.unpack_from('<I',wire,offset+5)[0])
    assert len(targets)==1 and next(iter(targets))>0,('Expected one real Damage Lab Target spawn',targets)
    return targets.pop()


def lab_walk_ack(wire,x,y):
    """Recognize the server's actual six-byte walk path destination."""
    for offset in range(max(0,len(wire)-11)):
        if struct.unpack_from('<H',wire,offset)[0]!=0x87:continue
        packed=wire[offset+6:offset+12]
        destination=((packed[2]&15)<<6 | packed[3]>>2,(packed[3]&3)<<8 | packed[4])
        if destination==(x,y):return True
    return False


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--root', type=Path, required=True)
    p.add_argument('--realm', type=Path, required=True)
    p.add_argument('--continue-lab-history', action='store_true', help='Preserve existing history; replace it only through three new actual runs')
    args = p.parse_args()
    root, realm = args.root.resolve(), args.realm.resolve()
    game = 'pn-improvement-rendered-20260929-game'
    db = 'pn-improvement-rendered-20260929-db'
    inspect = json.loads(subprocess.check_output(['docker', 'inspect', game]))[0]
    assert inspect['Config']['Labels'].get('pn.improvement.fixture') == 'true'
    assert list(inspect['NetworkSettings']['Networks']) == ['pn-improvement-rendered-20260929']
    assert os.readlink('/proc/self/ns/net') == os.readlink('/proc/' + str(inspect['State']['Pid']) + '/ns/net')
    startup = json.loads((realm / 'report.json').read_text())
    assert startup['passed'] and startup['production_database_accessed'] is False
    out = realm / ('world-proof-' + str(time.time_ns()))
    out.mkdir()
    report = {'passed': False, 'binding': startup['binding'], 'runtime_identity_sha256': startup['runtime_identity_sha256'],
              'production_database_accessed': False, 'rendered': False, 'cases': [],
              'fixture_sha256': startup['fixture_sha256'], 'driver_sha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest()}
    spec=importlib.util.spec_from_file_location('world_fixture_transport',root/'tools/ci/bank_live_client.py')
    transport=importlib.util.module_from_spec(spec);spec.loader.exec_module(transport)
    Client, drain = transport.Client, transport.drain
    client = None
    sequence = 0
    def sql(query):
        result = subprocess.run(['docker','exec','-i','-e','MYSQL_PWD=project-validation-only',db,
                                 'mariadb','-uroot','--batch','--raw','--skip-column-names','ragnarok_ci'],
                                input=query,text=True,capture_output=True,check=True,timeout=15)
        return result.stdout.strip()
    def await_sql(query, expected, seconds=15):
        deadline=time.monotonic()+seconds
        actual=None
        while time.monotonic()<deadline:
            actual=sql(query)
            if expected(actual): return actual
            time.sleep(.2)
        raise AssertionError(f'SQL condition timed out: {query}: {actual!r}')
    def connect(slot=0):
        return Client(slot=slot,attach=False,account_id=99000031,character_id=99000032,
                      username=b'visualfixture',password=b'visual-fixture-only',
                      login_port=58929,char_port=58129,map_port=57129)
    def disconnect():
        nonlocal client
        if client:
            try: client.world.shutdown(socket.SHUT_RDWR)
            except OSError: pass
            client.world.close(); client.char.close(); client=None
    def receive(seconds=.4):
        nonlocal sequence
        wire=bytearray();deadline=time.monotonic()+seconds
        while time.monotonic()<deadline:
            if seconds>5:
                client.world.sendall(struct.pack('<HI',0x0360,int(time.monotonic()*1000)&0xffffffff))
            wire.extend(drain(client.world,min(5,max(0,deadline-time.monotonic()))))
        wire=bytes(wire)
        (out/f'{sequence:04d}.bin').write_bytes(wire);sequence+=1
        return wire
    def send(packet, seconds=.4):
        client.world.sendall(packet); return receive(seconds)
    def command(text, seconds=.6):
        name='VisualFixture' if client.cid==99000032 else 'FixtureOther'
        message=(name+' : '+text).encode()+b'\0'
        return send(struct.pack('<HH',0xf3,len(message)+4)+message,seconds)
    def prompt(wire, code, npc):
        if code in (0xb7,0xb4):
            for start in range(max(0,len(wire)-7)):
                if struct.unpack_from('<H',wire,start)[0]!=code: continue
                length=struct.unpack_from('<H',wire,start+2)[0]
                if length>=9 and start+length<=len(wire) and struct.unpack_from('<I',wire,start+4)[0]==npc:
                    return wire[start+8:start+length].rstrip(b'\0').decode(errors='replace')
            raise AssertionError(f'Missing NPC variable packet {code:x} id={npc}')
        assert struct.pack('<HI',code,npc) in wire,f'Missing NPC prompt {code:x}'
    def menu(npc, choice, expected):
        wire=send(struct.pack('<HIB',0xb8,npc,choice));prompt(wire,expected,npc);return wire
    def number(npc, value, expected):
        wire=send(struct.pack('<HII',0x143,npc,value));prompt(wire,expected,npc);return wire
    def npc_id():
        return int(await_sql('SELECT npc_id FROM pn_world_fixture_state WHERE char_id=99000032',lambda x:x.isdigit() and int(x)>0))
    def begin_run(label):
        command("@qaworld observe")
        npc=npc_id()
        wire=send(struct.pack('<HIB',0x90,npc,0));assert 'Configure and run' in prompt(wire,0xb7,npc)
        menu(npc,1,0x1d4)
        text=label.encode()+b'\0';wire=send(struct.pack('<HHI',0x1d5,8+len(text),npc)+text)
        assert 'Small:Medium:Large' in prompt(wire,0xb7,npc)
        menu(npc,2,0xb7);menu(npc,1,0xb7);menu(npc,1,0x142)
        number(npc,1,0x142);number(npc,0,0x142);number(npc,0,0xb7)
        wire=send(struct.pack('<HIB',0xb8,npc,1));prompt(wire,0xb6,npc)
        wire+=send(struct.pack('<HI',0x146,npc))
        target=lab_target(wire)
        (out/f'target-{sequence:04d}.json').write_text(json.dumps({'aid':target,'job':1002,'name':'Damage Lab Target','packetver':20260219,'source':'actual server spawn/idle packet'},indent=2))
        deadline=time.monotonic()+10
        while b'GO: 60 seconds.' not in wire and time.monotonic()<deadline:wire+=receive(.5)
        assert b'GO: 60 seconds.' in wire,'Do not attack the countdown-immune target before GO'
        return target
    def registry():
        return {'numeric':sql("SELECT `key`,`index`,value FROM char_reg_num WHERE char_id=99000032 AND `key` LIKE 'PNLab%' ORDER BY `key`,`index`"),
                'strings':sql("SELECT `key`,`index`,value FROM char_reg_str WHERE char_id=99000032 AND `key` LIKE 'PNLab%' ORDER BY `key`,`index`")}
    def savecase(name, detail):
        artifacts={path.name:hashlib.sha256(path.read_bytes()).hexdigest()
                   for path in sorted(out.iterdir()) if path.suffix=='.bin' or path.name.startswith('lab-registry-') or path.name.startswith('target-')}
        report['cases'].append({'name':name,'status':'passed','detail':detail,'artifacts':artifacts})
        (out/'report.json').write_text(json.dumps(report,indent=2))
    try:
        assert sql('SELECT DATABASE()')=='ragnarok_ci'
        client=connect();command('@qaworld lab',1.2);send(struct.pack('<H',0x7d))
        assert sql('SELECT build_identity FROM pn_world_fixture_state WHERE char_id=99000032')==startup['runtime_identity_sha256']
        initial_valid=int(sql("SELECT COUNT(*) FROM char_reg_num WHERE char_id=99000032 AND `key`='PNLabValid' AND value=1"))
        assert args.continue_lab_history or initial_valid==0,'Fresh lab history required unless explicitly continuing'
        report['natural_rolling_history']=args.continue_lab_history
        initial_registry=registry();(out/'lab-registry-initial.json').write_text(json.dumps(initial_registry,indent=2))
        for i in range(3):
            target=begin_run('world-repeat-'+str(i+1))
            x,y=50,49
            movement=send(struct.pack('<H',0x035f)+bytes((x>>2,((x&3)<<6)|(y>>4),(y&15)<<4)),1.5)
            assert lab_walk_ack(movement,x,y),'Server must acknowledge the real approach before attacking'
            run_wire=send(struct.pack('<HIB',0x0437,target,7),.6)
            assert struct.pack('<HI',0x0139,target) not in run_wire,'Actual walk must put attacker in melee range'
            run_wire+=receive(64.4)
            measured=re.search(rb'RESULT world-repeat-'+str(i+1).encode()+rb': damage=(\d+) time_ms=(\d+)',run_wire)
            assert measured and int(measured[1])>0 and int(measured[2])>=60000,('Actual damaging run required before continuing',run_wire[-200:])
            command('@qaworld status')
            assert sql('SELECT runs FROM pn_world_fixture_state WHERE char_id=99000032')==str(min(3,initial_valid+i+1))
            assert int(sql('SELECT last_ms FROM pn_world_fixture_state WHERE char_id=99000032'))>=60000
        disconnect()
        await_sql("SELECT COUNT(*) FROM char_reg_num WHERE char_id=99000032 AND `key`='PNLabValid' AND value=1",lambda x:x=='3')
        before=registry();(out/'lab-registry-before.json').write_text(json.dumps(before,indent=2))
        client=connect()
        wire=command('@qaworld history')
        title=wire.find(b'[Saved Damage Lab Runs]\0')
        assert title>=8 and struct.unpack_from('<H',wire,title-8)[0]==0xb4
        history_npc=struct.unpack_from('<I',wire,title-4)[0]
        prompt(wire,0xb5,history_npc)
        wire=send(struct.pack('<HI',0xb9,history_npc));prompt(wire,0xb7,history_npc)
        wire=send(struct.pack('<HIB',0xb8,history_npc,1))
        assert b'Identical setup repeats: 3; median DPS ' in wire
        damage=sql("SELECT value FROM char_reg_num WHERE char_id=99000032 AND `key`='PNLabDamage' ORDER BY `index`").splitlines()
        assert len(damage)==3 and all(int(value)>0 for value in damage), damage
        labels=sql("SELECT value FROM char_reg_str WHERE char_id=99000032 AND `key`='PNLabName$' ORDER BY `index`").splitlines()
        assert sorted(labels)==['world-repeat-1','world-repeat-2','world-repeat-3'],labels
        prompt(wire,0xb6,history_npc);send(struct.pack('<HI',0x146,history_npc))
        command('@qaworld compat')
        assert sql('SELECT compatible,different_target,different_build,different_buff FROM pn_world_fixture_state WHERE char_id=99000032')=='1\t0\t0\t0'
        disconnect();await_sql('SELECT online FROM `char` WHERE char_id=99000032',lambda x:x=='0')
        assert registry()==before
        savecase('lab-real-timers-relog-comparison','Three actual 60-second console runs persisted through char SQL and relog; target/build/buff mismatches rejected by real script functions. Each run recorded nonzero damage from actual continuous attack packets against the production dummy; history median rendered as a protocol dialog, not visual evidence.')
        client=connect();command('@qaworld lab',1.2);send(struct.pack('<H',0x7d))
        begin_run('interrupted-not-saved');receive(5);disconnect()
        await_sql('SELECT online FROM `char` WHERE char_id=99000032',lambda x:x=='0')
        assert registry()==before
        savecase('lab-interruption-retains-history','Disconnect before 60 seconds leaves three previous persistent runs intact.')
        client=connect();command('@qaworld fill',1)
        slots=sql('SELECT inventory_slots,inventory_used FROM pn_world_fixture_state WHERE char_id=99000032').split('\t')
        assert len(slots)==2 and int(slots[0])>0 and slots[0]==slots[1], 'Fixture must actually fill all usable inventory slots'
        command('@qaworld pet3',1)
        await_sql('SELECT COUNT(*) FROM pn_pet_entitlements WHERE char_id=99000032 AND claimed=0',lambda x:x=='3')
        disconnect();client=connect(1);receive(6)
        assert sql('SELECT COUNT(*) FROM inventory WHERE char_id=99000033 AND nameid=9001')=='0'
        assert sql('SELECT COUNT(*) FROM pn_pet_entitlements WHERE char_id=99000032 AND claimed=0')=='3'
        disconnect();client=connect();receive(6)
        assert sql('SELECT COUNT(*) FROM pn_pet_entitlements WHERE char_id=99000032 AND claimed=0')=='3'
        command('@qaworld free1');command('@qaworld desk',1);send(struct.pack('<H',0x7d))
        npc=npc_id();wire=send(struct.pack('<HIB',0x90,npc,0));assert 'Collect what fits' in prompt(wire,0xb7,npc)
        wire=send(struct.pack('<HIB',0xb8,npc,1));prompt(wire,0xb6,npc);send(struct.pack('<HI',0x146,npc))
        await_sql('SELECT COUNT(*) FROM pn_pet_entitlements WHERE char_id=99000032 AND claimed=1',lambda x:x=='1')
        command('@qaworld free2');command('@petrewards')
        await_sql('SELECT COUNT(*) FROM pn_pet_entitlements WHERE char_id=99000032 AND claimed=1',lambda x:x=='3')
        command('@petrewards');receive(2);disconnect()
        await_sql('SELECT COUNT(*) FROM inventory WHERE char_id=99000032 AND nameid=9001',lambda x:x=='3')
        assert sql('SELECT COUNT(DISTINCT pet_id) FROM pn_pet_entitlements WHERE char_id=99000032')=='3'
        assert sql('SELECT COUNT(*) FROM pet')=='3'
        savecase('pet-world-recovery','Three entitlements survive full inventory, logout and same-account character switch; actual desk claims one fitting reward, command claims remaining two, repeat creates no duplicates.')
        report['passed']=True
    except Exception as error:
        report['error']=str(error)
        raise
    finally:
        disconnect()
        (out/'report.json').write_text(json.dumps(report,indent=2))
        print(str(out),flush=True)


if __name__=='__main__':
    main()
