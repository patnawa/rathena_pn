"""Require every native cash catalogue refresh to return the full fixture list.

Run inside a pn.cashshop.fixture Docker network namespace. The expected JSON is
an array of id/price/tab/view/location rows, matching the client preview patch.
This tests the actual packet handler; it does not render the game client.
"""
import argparse
import json
import os
from pathlib import Path
import struct
import subprocess

from bank_live_client import Client, drain


def catalogue(wire):
    actual = {}
    pos = 0
    while True:
        pos = wire.find(b'\xca\x08', pos)
        if pos < 0:
            break
        if pos + 8 <= len(wire):
            _, length, count, tab = struct.unpack_from('<HHHH', wire, pos)
            if (tab < 10 and count and length == 8 + count * 14
                    and pos + length <= len(wire)):
                for i in range(count):
                    ident, price, view, location = struct.unpack_from(
                        '<IIHI', wire, pos + 8 + 14 * i)
                    assert ident not in actual, ('duplicate catalogue row', ident)
                    actual[ident] = dict(price=price, tab=tab, view=view, location=location)
        pos += 2
    return actual


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--expected', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--game-container', default='pn-cashshop-20261006-game')
    args = parser.parse_args()
    info = json.loads(subprocess.check_output(['docker', 'inspect', args.game_container]))[0]
    assert info['Config']['Labels'].get('pn.cashshop.fixture') == 'true', 'Private fixture required'
    assert os.readlink('/proc/self/ns/net') == os.readlink(
        '/proc/' + str(info['State']['Pid']) + '/ns/net'), 'Private network namespace required'
    rows = json.loads(args.expected.read_text())
    expected = {r['id']: {k: r[k] for k in ('price', 'tab', 'view', 'location')} for r in rows}
    assert expected and len(expected) == len(rows), 'Unique expected catalogue required'
    client = Client(attach=False, account_id=99000011, character_id=99000012,
                    username=b'widefixturea', password=b'wide-fixture-only')
    cases = []
    try:
        for phase in ('first request', 'second request in same session',
                      'request after close and reopen'):
            if phase == 'request after close and reopen':
                client.world.sendall(struct.pack('<H', 0x84a))
                drain(client.world, .1)
                client.world.sendall(struct.pack('<HI', 0xb6d, 0))
                drain(client.world, .1)
            client.world.sendall(struct.pack('<H', 0x8c9))
            actual = catalogue(drain(client.world, .4))
            cases.append(dict(phase=phase, items=len(actual), passed=actual == expected))
            print(json.dumps(cases[-1]), flush=True)
    finally:
        client.world.close()
        client.char.close()
    report = dict(passed=all(r['passed'] for r in cases), expected_items=len(expected),
                  cases=cases, production_database_accessed=False)
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    assert report['passed'], 'A repeat catalogue request did not return the full catalogue'


if __name__ == '__main__':
    main()
