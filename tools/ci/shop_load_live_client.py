"""Real market purchase latency in the labelled disposable improvement realm.

Run in the game container network namespace on its Docker host. Four synthetic
accounts exercise native market packets, SQL receipts and the production health
parser. This is a bounded shopping workload, not a server capacity benchmark.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone
import hashlib
import json
import math
import os
from pathlib import Path
import re
import socket
import struct
import subprocess
import sys
import time

from bank_live_client import Client, drain

GAME = 'pn-improvement-rendered-20260929-game'
DB = 'pn-improvement-rendered-20260929-db'
DATABASE = 'ragnarok_ci'
ACCOUNTS = tuple(range(99000101, 99000105))
CHARACTERS = tuple(range(99000201, 99000205))
SUCCESS = struct.pack('<HHHIHI', 0x0b4e, 16, 0, 501, 1, 1)
REFUSAL = struct.pack('<HHH', 0x0b4e, 6, 65535)


def command(*args, **kwargs):
    return subprocess.check_output(args, text=True, timeout=120, **kwargs).strip()


def sql(query):
    return command('docker', 'exec', '-e', 'MYSQL_PWD=project-validation-only', '-i',
                   DB, 'mariadb', '-uroot', '--batch', '--raw', '-N', DATABASE,
                   input=query)


def stats(values):
    ordered = sorted(values)
    return {'count': len(values), **{name: ordered[math.ceil(len(ordered)*q)-1] if ordered else None
            for name, q in (('p50_ms', .50), ('p95_ms', .95), ('p99_ms', .99), ('max_ms', 1))}}


class Buyer(Client):
    def __init__(self, index, npc):
        super().__init__(attach=False, account_id=ACCOUNTS[index], character_id=CHARACTERS[index],
                         username=f'metrics{index+1}'.encode(), password=b'metrics-fixture-only',
                         login_port=58929, char_port=58129, map_port=57129)
        self.world.sendall(struct.pack('<HIB', 0x90, npc, 0))
        assert b'\x7a\x0b' in drain(self.world), 'Actual market did not open'

    def purchase(self):
        started = time.monotonic()
        self.world.sendall(struct.pack('<HHIi', 0x09d6, 12, 501, 1))
        wire = bytearray()
        self.world.settimeout(.2)
        next_tick = started+5
        while time.monotonic()-started < 150:
            if time.monotonic() >= next_tick:
                self.world.sendall(struct.pack('<HI', 0x0360, int(time.monotonic()*1000) & 0xffffffff))
                next_tick = time.monotonic()+5
            try:
                part = self.world.recv(65536)
                if not part:
                    raise RuntimeError('Map closed the buyer connection')
                wire.extend(part)
            except socket.timeout:
                continue
            # Match a whole known result frame, including exact item/qty/price.
            # SQL totals below independently establish all acknowledged effects.
            if SUCCESS in wire:
                return True, (time.monotonic()-started)*1000
            if REFUSAL in wire:
                return False, (time.monotonic()-started)*1000
            if len(wire) > 2**20:
                raise RuntimeError('No result in bounded receive buffer')
        raise RuntimeError('No durable market acknowledgement within 150 seconds')

    def close(self):
        self.world.close()
        self.char.close()


def bounded_seconds(value):
    try:
        seconds = int(value)
    except ValueError as error:
        raise argparse.ArgumentTypeError('duration must be an integer number of seconds') from error
    if not 1 <= seconds <= 1800:
        raise argparse.ArgumentTypeError('duration must be between 1 and 1800 seconds')
    return seconds


def parse_args(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--realm', type=Path, required=True)
    parser.add_argument('--evidence', type=Path, required=True)
    parser.add_argument('--single-seconds', type=bounded_seconds, default=125,
                        help='single-buyer phase duration, 1..1800 seconds (default: 125)')
    parser.add_argument('--four-seconds', type=bounded_seconds, default=245,
                        help='four-buyer phase duration, 1..1800 seconds (default: 245; final baseline: 1800)')
    args = parser.parse_args(argv)
    # At most one attempt/second plus the delayed transaction: keep a fresh
    # buyer below 2000 added potions even if it wins every contended attempt.
    if args.single_seconds + args.four_seconds > 1998:
        parser.error('combined phase durations must not exceed 1998 seconds')
    return args


def main():
    args = parse_args()
    info = json.loads(command('docker', 'inspect', GAME))[0]
    assert info['Config']['Labels'].get('pn.improvement.fixture') == 'true'
    assert os.readlink('/proc/self/ns/net') == os.readlink(f"/proc/{info['State']['Pid']}/ns/net")
    db_info = json.loads(command('docker', 'inspect', DB))[0]
    networks = set(info['NetworkSettings']['Networks']) & set(db_info['NetworkSettings']['Networks'])
    assert networks == {'pn-improvement-rendered-20260929'}
    assert sql('SELECT DATABASE()') == DATABASE
    assert sql('SELECT COUNT(*) FROM login WHERE account_id BETWEEN 99000101 AND 99000104') == '4'
    root = args.realm.parent / 'candidate'
    sys.path.insert(0, str(root / 'tools/admin'))
    from health_check import metrics_status
    from release_bundle import binding
    initial = binding(root)
    out = args.evidence.resolve()
    out.mkdir(parents=True, exist_ok=False)
    logpath = args.realm / 'runtime-map.log'
    # Realm harness may use the shorter role filename.
    if not logpath.exists():
        logpath = args.realm / 'map.log'
    npc = int(re.search(r'PN_METRICS_MARKET_ID=(\d+)', logpath.read_text(errors='replace'))[1])
    before = sql('SELECT COUNT(*),COALESCE(SUM(OCTET_LENGTH(payload)),0) FROM pn_shop_commits')
    receipts_before = int(sql('SELECT COUNT(*) FROM pn_shop_commits WHERE account_id BETWEEN 99000101 AND 99000104 AND outcome=1'))
    wallets_before = [int(sql(f'SELECT zeny FROM `char` WHERE char_id={cid}')) for cid in CHARACTERS]
    items_before = [int(sql(f'SELECT COALESCE(SUM(amount),0) FROM inventory WHERE char_id={cid} AND nameid=501')) for cid in CHARACTERS]
    stock_before = int(sql("SELECT amount FROM market WHERE name='PNMetricsMarket' AND nameid=501"))
    buyers = []
    results = [[] for _ in ACCOUNTS]
    report = {'passed': False, 'binding': initial, 'production_accessed': False,
              'driver_sha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
              'scope': 'One and four connected market buyers, one attempt/player/second; no combat capacity claim',
              'phase_seconds': {'single': args.single_seconds, 'four': args.four_seconds},
              'cases': [], 'receipt_before': before, 'stock_before': stock_before}

    def phase(index, seconds):
        rows = []
        deadline = time.monotonic()+seconds
        while time.monotonic() < deadline:
            started = time.monotonic()
            row = buyers[index].purchase()
            rows.append(row)
            time.sleep(max(0, 1-(time.monotonic()-started)))
        results[index].extend(rows)
        return rows

    try:
        for name, seconds, active in (('single-buyer', args.single_seconds, [0]), ('four-buyers', args.four_seconds, list(range(4)))):
            while len(buyers) <= max(active):
                buyers.append(Buyer(len(buyers), npc))
            started_utc = int(time.time())
            with ThreadPoolExecutor(max_workers=len(active)) as executor:
                rows = [row for group in executor.map(lambda i: phase(i, seconds), active) for row in group]
            assert any(ok for ok, _ in rows), 'No successful purchases'
            report['cases'].append({'name': name, 'status': 'passed', 'started_utc': started_utc,
                                    'duration_seconds': seconds, 'buyers': len(active),
                                    'success_latency': stats([ms for ok, ms in rows if ok]),
                                    'refusal_latency': stats([ms for ok, ms in rows if not ok])})
        # Hold the real receipt transaction on char SQL. The map stays alive,
        # retains its fence, reports the age and eventually acknowledges once.
        sql("CREATE TRIGGER pn_metrics_delay BEFORE INSERT ON pn_shop_commits FOR EACH ROW DO SLEEP(75)")
        stalled = None
        with ThreadPoolExecutor(max_workers=1) as executor:
            future = executor.submit(buyers[0].purchase)
            deadline = time.monotonic()+72
            while time.monotonic() < deadline:
                text = re.sub(r'\x1b\[[0-?]*[ -/]*[@-~]', '', logpath.read_text(errors='replace'))
                status = metrics_status(text, datetime.now(timezone.utc), 4000, max_pending_seconds=5)
                if not status['passed'] and any('pending' in reason.lower() for reason in status.get('reasons', [])):
                    stalled = status
                    break
                time.sleep(2)
            row = future.result(timeout=100)
            results[0].append(row)
        sql('DROP TRIGGER pn_metrics_delay')
        assert stalled is not None, 'Actual pending transaction never failed health threshold'
        assert row[0] and row[1] >= 74000, row
        report['cases'].append({'name': 'sql-delayed-purchase', 'status': 'passed',
                                'ack_ms': row[1], 'stalled_health': stalled})
        successes = [sum(ok for ok, _ in rows) for rows in results]
        for cid, count, wallet, items in zip(CHARACTERS, successes, wallets_before, items_before):
            assert int(sql(f'SELECT zeny FROM `char` WHERE char_id={cid}')) == wallet-count
            assert int(sql(f'SELECT COALESCE(SUM(amount),0) FROM inventory WHERE char_id={cid} AND nameid=501')) == items+count
        stock_after = int(sql("SELECT amount FROM market WHERE name='PNMetricsMarket' AND nameid=501"))
        assert stock_before-stock_after == sum(successes)
        report['receipt_after'] = sql('SELECT COUNT(*),COALESCE(SUM(OCTET_LENGTH(payload)),0) FROM pn_shop_commits')
        receipt_count = int(sql('SELECT COUNT(*) FROM pn_shop_commits WHERE account_id BETWEEN 99000101 AND 99000104 AND outcome=1'))
        assert receipt_count-receipts_before == sum(successes), (receipt_count, receipts_before, successes)
        report['successes_by_buyer'] = successes
        report['stock_after'] = stock_after
        deadline = time.monotonic()+75
        while time.monotonic() < deadline:
            text = re.sub(r'\x1b\[[0-?]*[ -/]*[@-~]', '', logpath.read_text(errors='replace'))
            status = metrics_status(text, datetime.now(timezone.utc), 4000)
            if status['passed']:
                break
            time.sleep(2)
        assert status['passed'], status
        report['recovered_health'] = status
        assert binding(root) == initial
        report['passed'] = True
    except Exception as error:
        report['error'] = str(error)
        raise
    finally:
        sql('DROP TRIGGER IF EXISTS pn_metrics_delay')
        for buyer in buyers:
            buyer.close()
        log = out / 'map.log'
        log.write_bytes(logpath.read_bytes())
        report['artifacts'] = {log.name: hashlib.sha256(log.read_bytes()).hexdigest()}
        for case in report['cases']:
            case['artifacts'] = dict(report['artifacts'])
        (out / 'report.json').write_text(json.dumps(report, indent=2)+'\n')


if __name__ == '__main__':
    main()
