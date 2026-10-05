"""Exact raw floor reservation and party distribution bodies with explicit world/SQL doubles."""
from pathlib import Path
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix='pn-pet-floor-') as directory:
 path=Path(directory)
 source=(ROOT/'src/custom/pet_floor.inc').read_text()
 (path/'pet_floor_body.inc').write_text(source)
 party=(ROOT/'src/map/party.cpp').read_text()
 (path/'party_loot_body.inc').write_text(party[party.index('int32 party_share_loot_with('):party.index('int32 party_share_loot(party_data*')])
 map_source=(ROOT/'src/map/map.cpp').read_text()
 start=map_source.index('TIMER_FUNC(map_clearflooritem_timer){')
 (path/'floor_timer_body.inc').write_text(map_source[start:map_source.index('/*\n * clears a single bl item',start)])
 binary=path/'pet-floor'
 subprocess.run(['g++','-std=c++17','-O1','-fsanitize=address,undefined','-fno-sanitize-recover=all','-I'+str(ROOT/'src'),'-I'+str(path),str(ROOT/'tools/ci/pet_floor_test.cpp'),'-o',str(binary)],check=True)
 subprocess.run([str(binary)],check=True)
