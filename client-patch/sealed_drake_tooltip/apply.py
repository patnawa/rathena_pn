"""Correct card 4496's tooltip without changing other records or resource bytes."""
import argparse
from pathlib import Path

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('client', type=Path)
args = parser.parse_args()
target = args.client / 'SystemEN/LuaFiles514/itemInfo.lua'
patch = Path(__file__).resolve().parent
data = target.read_bytes()
newline = b'\r\n' if b'\r\n' in data else b'\n'
def record(name):
    return (patch / name).read_bytes().replace(b'\r\n', b'\n').replace(b'\n', newline)
before, after = record('4496.before.txt'), record('4496.after.txt')
if after in data:
    print('Card 4496 tooltip is already corrected.')
else:
    if data.count(before) != 1:
        raise SystemExit('Unexpected card definition; no files changed.')
    backup = target.with_name(target.name + '.before-sealed-drake')
    if backup.exists():
        raise SystemExit('Backup already exists; no files changed.')
    backup.write_bytes(data)
    target.write_bytes(data.replace(before, after, 1))
    print('Corrected card 4496. Restart the client to reload the tooltip.')
