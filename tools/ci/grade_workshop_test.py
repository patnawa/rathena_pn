"""Verify every enabled workshop service and both real entrances against native cells."""
import json
from pathlib import Path
import re
import sys
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools'))
from generate_grade_workshop_layout import ARRIVALS, connected, geometry


def enabled_scripts(root=ROOT):
    visited, result = set(), set()
    def visit(relative):
        if relative in visited:
            return
        visited.add(relative)
        for line in (root / relative).read_text(encoding='utf-8').splitlines():
            match = re.match(r'^\s*(import|npc):\s*([^\s]+)\s*(?://.*)?$', line)
            if match:
                kind, path = match.groups()
                if kind == 'import':
                    visit(path)
                else:
                    assert (root / path).is_file(), ('Missing enabled script', path)
                    result.add(path)
    visit('npc/re/scripts_main.conf')
    return sorted(result)


def verify_layout(rows, width, height, cells, arrivals=ARRIVALS, warp_cells=frozenset()):
    points = [tuple(row['npc']) for row in rows]
    if len(points) != len(set(points)):
        raise AssertionError('Workshop NPCs overlap')
    occupied = set(points)
    if occupied & set(warp_cells):
        raise AssertionError('Workshop NPC occupies warp trigger')
    components = [connected(width, height, cells, tuple(start), occupied | set(warp_cells)) for start in arrivals]
    for start in arrivals:
        if tuple(start) not in components[0]:
            raise AssertionError('Workshop arrivals are disconnected')
    for row in rows:
        x, y = row['npc']
        if not (0 <= x < width-1 and 0 <= y < height-1 and cells[x+y*width] in (0, 3)):
            raise AssertionError(f"Service is outside walkable interior: {row['id']}")
        approach = tuple(row['approach'])
        if max(abs(x-approach[0]), abs(y-approach[1])) != 1:
            raise AssertionError('Counter needs an adjacent standing cell')
        if any(approach not in seen for seen in components):
            raise AssertionError(f"Counter cannot be approached from every entrance: {row['id']}")
    return True


def verify_repository():
    layout = json.loads((ROOT / 'npc/custom/grade_workshop_layout.json').read_text())
    declarations, warps = {}, set()
    for relative in enabled_scripts():
        for number, line in enumerate((ROOT / relative).read_text(encoding='utf-8', errors='replace').splitlines(), 1):
            match = re.match(r'^grademk,(\d+),(\d+),[^\t]*\t([^\t]+)\t([^\t]+)\t(.*)$', line)
            if not match:
                continue
            x, y, kind, identity, tail = match.groups()
            point = (int(x), int(y))
            if kind == 'warp':
                rx, ry = map(int, tail.split(',')[:2])
                warps.update((point[0]+dx, point[1]+dy) for dx in range(-rx, rx+1) for dy in range(-ry, ry+1))
            elif kind in ('script', 'shop') or kind.startswith('duplicate('):
                if identity in declarations:
                    raise AssertionError(('Repeated workshop identity', identity))
                declarations[identity] = dict(id=identity, npc=list(point), file=relative, line=number)
            else:
                raise AssertionError(('Unreviewed workshop declaration type', kind, relative))
    assert set(declarations) == {row['id'] for row in layout['services']}, ('Unreviewed or missing service', set(declarations))
    for row in layout['services']:
        assert row['npc'] == declarations[row['id']]['npc'], ('Placement drift', row['id'])
    width, height, cells, cache, digest = geometry()
    assert cache == layout['cache']
    # Whole cache also includes the Office alias; validate this room's cells rather than unrelated map hashes.
    assert len(cells) == width * height
    verify_layout(layout['services'], width, height, cells, layout['arrivals'], warps)
    directory = (ROOT / 'npc/custom/grade_workshop_directory.txt').read_text()
    for row in layout['services']:
        if row['id'] != 'Workshop Guide#grademk':
            x, y = row['approach']
            assert f'navigateto "grademk",{x},{y}' in directory
    assert re.search(r'\{ "grademk",\s*34, 184 \}', (ROOT/'src/map/atcommand.cpp').read_text())
    assert 'Go("grademk",34,184)' in (ROOT/'npc/custom/warper.txt').read_text()
    return dict(map='grademk', services=len(declarations), arrivals=layout['arrivals'],
                cache=cache, cache_sha256=digest, npc_blocked_reachability=True)


class GeometryRegression(unittest.TestCase):
    def setUp(self):
        self.cells = bytes([0] * 49)
        self.rows = [dict(id='refiner', npc=[3, 4], approach=[3, 3])]

    def test_open_room(self):
        self.assertTrue(verify_layout(self.rows, 7, 7, self.cells, ((1, 1), (5, 5))))

    def test_npc_on_arrival(self):
        with self.assertRaisesRegex(AssertionError, 'arrival'):
            verify_layout(self.rows, 7, 7, self.cells, ((3, 4),))

    def test_walled_off_counter(self):
        cells = bytearray(self.cells)
        for y in range(7):
            cells[2 + y * 7] = 1
        with self.assertRaisesRegex(AssertionError, 'cannot be approached'):
            verify_layout(self.rows, 7, 7, cells, ((1, 1),))

    def test_npc_can_block_only_doorway(self):
        cells = bytearray(self.cells)
        for y in range(7):
            cells[3 + y * 7] = 1
        cells[3 + 4 * 7] = 0
        with self.assertRaisesRegex(AssertionError, 'disconnected'):
            verify_layout(self.rows, 7, 7, cells, ((1, 1), (5, 5)))

    def test_overlap(self):
        with self.assertRaisesRegex(AssertionError, 'overlap'):
            verify_layout(self.rows * 2, 7, 7, self.cells, ((1, 1),))

    def test_warp_collision(self):
        with self.assertRaisesRegex(AssertionError, 'warp trigger'):
            verify_layout(self.rows, 7, 7, self.cells, ((1, 1),), {(3, 4)})


if __name__ == '__main__':
    result = unittest.main(argv=[sys.argv[0]], exit=False).result
    if not result.wasSuccessful():
        raise SystemExit(1)
    print(json.dumps(verify_repository(), indent=2))
