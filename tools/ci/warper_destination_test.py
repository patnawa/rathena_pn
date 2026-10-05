#!/usr/bin/env python3
"""Audit literal and generated Warper routes against map geometry and configuration."""
from pathlib import Path
import re, struct, zlib, json
from episode_party_progression_test import scan_to
root = Path(__file__).resolve().parents[2]
s = (root / 'npc/custom/warper.txt').read_text()
cells = {}
for rel in ('db/import/map_cache.dat', 'db/re/map_cache.dat', 'db/map_cache.dat'):
    p = root / rel
    if not p.exists():
        continue
    data = p.read_bytes()
    pos = 8
    for _ in range(struct.unpack_from('<H', data, 4)[0]):
        name, w, h, size = struct.unpack_from('<12shhi', data, pos)
        pos += 20
        name = name.split(b'\x00')[0].decode()
        cells.setdefault(name, (w, h, zlib.decompress(data[pos:pos + size])))
        pos += size
routes = []
route_source = re.sub(r'//[^\n]*', '', s)
# Main/Cancel labels precede local helpers. Helper bodies and documentation
# are not destination blocks, even when they mention Pick or Go.
while (match := re.search(r'function\s+\w+\s*\{', route_source)):
    end = scan_to(route_source, match.end()-1, '{', '}')+1
    route_source = route_source[:match.start()] + route_source[end:]
labels = list(re.finditer('(?m)^\\s*([A-Za-z_][\\w]*):', route_source))
for i, l in enumerate(labels):
    block = route_source[l.end():labels[i + 1].start() if i + 1 < len(labels) else len(route_source)]
    for m in re.finditer('Go\\("([^"]+)",\\s*(\\d+),\\s*(\\d+)\\)', block):
        routes.append((l[1], m[1], int(m[2]), int(m[3])))
    pick = re.search('Pick\\(([^;]+)\\)', block)
    if not pick:
        continue
    disp = re.search('Disp\\("([^"]*)"(?:,\\s*(\\d+),\\s*(\\d+))?\\)', block)
    arr = re.search('setarray @c\\[(\\d+)\\],([\\d, ]+);', block)
    assert disp and arr, ('unparsed route', l[1])
    co = [0] * int(arr[1]) + [int(x) for x in arr[2].split(',')]
    args = re.findall('"([^"]*)"|(?<=,)\\s*(\\d+)', pick[1])
    args = [a or b for a, b in args]
    n = int(disp[3]) - int(disp[2]) + 1 if disp[2] else len(disp[1].split(':'))
    assert args[0] or len(args) == n + 1, ('menu/map count mismatch', l[1])
    for selection in range(1, n + 1):
        ix = selection - (int(args[1]) if args[0] and len(args) > 1 else 0)
        name = args[0] + str(ix).zfill(2) if args[0] else args[ix]
        routes.append((l[1], name, co[ix * 2], co[ix * 2 + 1]))
issues = []
for label, name, x, y in routes:
    if name not in cells:
        issues.append((label, name, x, y, 'MISSING'))
        continue
    w, h, grid = cells[name]
    if not (0 <= x < w and 0 <= y < h) or grid[y * w + x] not in (0, 3, 6):
        issues.append((label, name, x, y, 'BLOCKED'))
print(json.dumps({'routes': len(routes), 'issues': issues}, indent=2))

def config_files(path, seen=None):
    seen = set() if seen is None else seen
    if path in seen:
        return []
    seen.add(path)
    result = []
    assert path.exists(), ('missing active configuration', str(path))
    for line in path.read_text(encoding='utf-8-sig').splitlines():
        line = line.split('//', 1)[0].strip()
        m = re.match('(import|npc):\\s*(.+)', line)
        if m:
            child = root / m[2].strip()
            if m[1] == 'import':
                result.extend(config_files(child, seen))
            else:
                assert child.exists(), ('missing active NPC', str(child))
                result.append(child)
    return result
scripts = config_files(root / 'npc/re/scripts_main.conf')
assert root / 'npc/custom/warper.txt' in scripts
loaded = set()

def maps(path):
    for line in path.read_text().splitlines():
        line = line.split('//', 1)[0].strip()
        m = re.match('(map|import):\\s*(.+)', line)
        if not m:
            continue
        if m[1] == 'map':
            loaded.add(m[2])
        elif (root / m[2]).exists():
            maps(root / m[2])
maps(root / 'conf/maps_athena.conf')
for label, name, x, y in routes:
    if name not in loaded:
        block = next((route_source[l.end():labels[i + 1].start() if i + 1 < len(labels) else len(route_source)] for i, l in enumerate(labels) if l[1] == label))
        assert 'Restrict("Pre-RE"' in block, ('unloaded destination', label, name)
nav = set(((m, int(x), int(y)) for m, x, y in re.findall('naviregisterwarp\\("[^"\\n]*",\\s*"([^"\\n]+)",\\s*(\\d+),\\s*(\\d+)\\)', s)))
missing_nav = [(label, name, x, y) for label, name, x, y in routes if (name, x, y) not in nav]
assert not missing_nav, missing_nav
assert not issues, issues
print(json.dumps({'active_scripts': len(scripts), 'destinations': len(routes), 'dungeon_destinations': sum((label.startswith(('D', 'SubD')) for label, *_ in routes)), 'missing_navigation': missing_nav}, indent=2))

functions = set()
for path in scripts:
    functions.update(re.findall(r'function\s+script\s+([^\s{]+)', path.read_text(encoding='utf-8-sig', errors='replace')))
helpers = set(re.findall(r'callfunc(?:\s*\(\s*|\s+)"([^"]+)"', s))
assert helpers <= functions, ('missing loaded access helpers', sorted(helpers - functions))
for menu in re.findall(r'\bmenu\s+(.*?);', s, re.S):
    for title, label in re.findall(r'"([^"\n]*)"\s*,\s*([A-Za-z_]\w*)', menu):
        assert ':' not in title, ('split menu title', title)
        assert label in {match[1] for match in labels}, ('missing menu label', label)
print('PASS: all named access helpers and menu targets are loaded')

# Coordinate-specific entrance checks must follow every instance route when
# an NPC moves, or saved travel could silently lose the route's prerequisites.
match = re.search(r'function ValidateEntrance\s*\{', s)
entrances = s[match.end():scan_to(s, match.end()-1, '{', '}')]
canonical = {}
for match in re.finditer(r'if \(\.@map\$ == "([^"]+)"\) \{', entrances):
    block = entrances[match.end():scan_to(entrances, match.end()-1, '{', '}')]
    for x, y, route in re.findall(r'if \(\.@x == (\d+) && \.@y == (\d+)\) \{ // (I\d+)\b', block):
        assert route not in canonical, ('duplicate canonical entrance', route)
        canonical[route] = (match[1], int(x), int(y))
instance_routes = {label:(name,x,y) for label,name,x,y in routes if re.fullmatch(r'I\d+',label)}
assert canonical == instance_routes, 'Instance menu and saved-route validators differ'
assert len(set(canonical.values())) == len(canonical), 'Ambiguous instance entrance'
print('PASS: '+str(len(canonical))+' canonical instance entrances keep saved travel gated')
