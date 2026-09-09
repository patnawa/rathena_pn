"""Check Bioresearch NPC positions and reversible cages against native map cells."""
from pathlib import Path
import re
import sys
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
from inspect_mapcache import load_maps


class BioresearchGeometry(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.source = (ROOT / "npc/custom/instances/BioresearchLaboratory.txt").read_text()
        cls.maps = load_maps(ROOT / "db/map_cache.dat", {"1@gol1", "1@gol2", "yuno"})

    def walkable(self, name, x, y):
        width, height, cells = self.maps[name]
        return 0 <= x < width and 0 <= y < height and cells[x + y * width] in (0, 3)

    @staticmethod
    def array(source, name):
        found = re.search(r"setarray \.@" + name + r"\[0\],([\d,]+);", source)
        assert found, name
        return [int(x) for x in found[1].split(",")]

    def test_visible_npcs_and_literal_destinations(self):
        count = 0
        for match in re.finditer(r"^(1@gol[12]|yuno),(\d+),(\d+),\d+\s+script(?:\(DISABLED\))?\s+([^\t]+)\t([^,]+)", self.source, re.M):
            name, x, y, npc, sprite = match.groups()
            if sprite == "-1":
                continue
            self.assertTrue(self.walkable(name, int(x), int(y)), npc)
            count += 1
        self.assertEqual(count, 24)  # eight usable NPCs and sixteen flame visuals
        destinations = re.findall(r'(?:warp|monster|callfunc "F_BioMove",)\s*instance_mapname\("(1@gol[12])"(?:,[^)]*)?\),(\d+),(\d+)', self.source)
        destinations += re.findall(r'warp "(yuno)",(\d+),(\d+)', self.source)
        self.assertGreaterEqual(len(destinations), 4)
        for name, x, y in destinations:
            self.assertTrue(self.walkable(name, int(x), int(y)), (name, x, y))

    def test_stage_arrivals_and_encounter_paths(self):
        source = self.source.split("OnZone:", 1)[1]
        arrivals = list(zip(self.array(source, "x"), self.array(source, "y")))[1:]
        self.assertEqual(len(arrivals), 7)
        for zone, start in enumerate(arrivals, 1):
            block = re.search(r"if \('bio_zone == " + str(zone) + r"\) \{\s*setarray \.@sx.*?\}", source, re.S)[0]
            points = list(zip(self.array(block, "sx"), self.array(block, "sy")))
            self.assertTrue(self.walkable("1@gol1", *start))
            seen, todo = {start}, [start]
            for x, y in todo:
                for point in [(x-1, y), (x+1, y), (x, y-1), (x, y+1)]:
                    if point not in seen and self.walkable("1@gol1", *point):
                        seen.add(point)
                        todo.append(point)
            for point in points:
                self.assertIn(point, seen, (zone, point))

    def test_cage_perimeters_restore_only_native_floor(self):
        barrier = self.source.split("function\tscript\tF_BioBarrier", 1)[1].split("\nyuno,", 1)[0]
        centers = list(zip(self.array(barrier, "x"), self.array(barrier, "y")))
        self.assertEqual(len(centers), 4)
        zone = re.search(r"if \('bio_zone == 5\) \{\s*setarray \.@sx.*?\}", self.source, re.S)[0]
        self.assertEqual(centers, list(zip(self.array(zone, "sx"), self.array(zone, "sy"))))
        expected = [".@cx-3,.@cy-3,.@cx+3,.@cy-3", ".@cx-3,.@cy+3,.@cx+3,.@cy+3", ".@cx-3,.@cy-2,.@cx-3,.@cy+2", ".@cx+3,.@cy-2,.@cx+3,.@cy+2"]
        actual = re.findall(r"setcell \.@map\$,([^;]+);", barrier)
        self.assertEqual(actual, [p + ",CELL_WALKABLE,!.@closed" for p in expected])
        for wave, (x, y) in enumerate(centers):
            square = {(xx, yy) for xx in range(x-3, x+4) for yy in range(y-3, y+4)}
            perimeter = {p for p in square if abs(p[0]-x) == 3 or abs(p[1]-y) == 3}
            self.assertEqual(len(perimeter), 24)
            # All 49 native cells are floor, proving the 25-cell interior is
            # connected and reopening the boundary cannot create a wall hole.
            for point in square:
                self.assertTrue(self.walkable("1@gol1", *point), (wave, point))
            for match in re.finditer(r"^1@gol1,(\d+),(\d+),4\s+script\(DISABLED\)\s+Containment Flame#bio" + str(wave) + r"_\d", self.source, re.M):
                self.assertIn((int(match[1]), int(match[2])), perimeter)


if __name__ == "__main__":
    unittest.main()
