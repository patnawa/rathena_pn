#!/usr/bin/env python3
"""Static content, map-cell and release safety regressions; not in-game tests."""
import io
import json
from pathlib import Path
import sys
import tarfile
import tempfile
from unittest.mock import patch

import yaml
from client_preflight import inspect
from deploy_scope_manifest import digest
import guarded_source_deploy as deploy

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools'))
from inspect_mapcache import load_maps


def main():
    service = (ROOT / 'npc/custom/quality_services.txt').read_text()
    gm = (ROOT / 'npc/custom/gm_character_setup.txt').read_text()
    config = (ROOT / 'npc/scripts_custom.conf').read_text()
    for file in ('quality_services', 'gm_character_setup'):
        assert config.count('npc: npc/custom/' + file + '.txt') == 1
    assert gm.count('getgmlevel() < 99') == 2
    assert gm.index('select("Cancel:Apply') < gm.index('BaseLevel = 275')
    assert 'setlook LOOK_BODY2,Class;' in gm and '@allskill' not in gm
    assert not any(token in gm for token in ('query_sql', 'getitem ', 'jobchange ', 'resetskill'))
    assert "gettimetick(2) >= 'Until" in service
    assert 'if (.@invalid)' in service and '.@damage*1000/max(1,.@elapsed)' in service
    assert 'killmonsterall .@map$;' in service and 'IM_CHAR' in service
    record = yaml.safe_load((ROOT / 'db/import/quality_lab_instance_db.yml').read_text())['Body'][0]
    assert record['Id'] == 1000 and record['Name'] == 'PN Damage Lab'
    assert 'quality_lab_instance_db.yml' in (ROOT / 'db/instance_db.yml').read_text()
    maps = {}
    for name in ('db/import/map_cache.dat', 'db/re/map_cache.dat', 'db/map_cache.dat'):
        path = ROOT / name
        if path.exists():
            for key, value in load_maps(path, {'guild_vs1', 'grademk', 'izlude', 'izlude_a', 'izlude_b', 'izlude_c', 'izlude_d'}).items():
                maps.setdefault(key, value)
    coordinates = [('guild_vs1', 50, y) for y in (43, 45, 50)]
    coordinates += [('grademk', 46, 180)]
    coordinates += [(name, 140, y) for name in maps if name.startswith('izlude') for y in (144, 146)]
    for name, x, y in coordinates:
        width, height, cells = maps[name]
        assert 0 <= x < width and 0 <= y < height and cells[y*width+x] in (0, 3), (name, x, y)
    # Formula fixtures, including totals exceeding signed 32-bit.
    for loss, elapsed, expected in ((60000, 60000, 1000), (60000000000, 60010, 999833361), (0, 60100, 0)):
        assert loss*1000//max(1, elapsed) == expected
    with tempfile.TemporaryDirectory() as directory:
        base = Path(directory)
        client = base / 'client'
        client.mkdir()
        (client / 'DATA.INI').write_text('[Data]\n0=patch.grf\n1=data.grf\n')
        (client / 'patch.grf').write_bytes(b'Master of Magic\0')
        (client / 'data.grf').write_bytes(b'Event Horizon\0RL')
        (client / 'Ragexe.exe').write_bytes(b'test')
        assert not inspect(client, '20260219')['issues']
        (client / 'data.grf').unlink()
        assert inspect(client, '20260219')['issues']
        live = base / 'live'
        live.mkdir()
        (live / 'old.txt').write_bytes(b'before')
        archive = base / 'release.tar.gz'
        payload = {'old.txt': b'after', 'added.txt': b'new'}
        with tarfile.open(archive, 'w:gz') as tar:
            for name, data in payload.items():
                info = tarfile.TarInfo(name)
                info.size = len(data)
                tar.addfile(info, io.BytesIO(data))
        manifest = base / 'release.json'
        manifest.write_text(json.dumps({'archive_sha256': digest(archive.read_bytes()), 'entries': [
            {'path': name, 'before': digest(b'before', True) if name == 'old.txt' else None,
             'after': digest(data)} for name, data in payload.items()]}))
        log = base / 'startup.log'
        log.write_text('Map Server is now online')
        backup = base / 'backup'
        args = ['--root', str(live), '--backup', str(backup), '--archive', str(archive),
                '--manifest', str(manifest), '--startup-log', str(log)]
        with patch.object(sys, 'argv', ['deploy', 'plan'] + args):
            deploy.main()
        assert not backup.exists() and (live / 'old.txt').read_bytes() == b'before'
        (live / 'old.txt').write_bytes(b'unrelated edit')
        with patch.object(sys, 'argv', ['deploy', 'plan'] + args):
            try:
                deploy.main()
                raise AssertionError('Live drift accepted')
            except ValueError:
                pass
        (live / 'old.txt').write_bytes(b'before')
        log.write_text("Server is 'ready' and listening\n[Error]: broken script")
        with patch.object(deploy, 'check_stopped'), patch.object(sys, 'argv', ['deploy', 'apply'] + args):
            try:
                deploy.main()
                raise AssertionError('Unclean candidate accepted')
            except ValueError:
                pass
        assert not backup.exists()
        log.write_text("Server is 'ready' and listening")
        with patch.object(deploy, 'check_stopped'), patch.object(sys, 'argv', ['deploy', 'apply'] + args):
            deploy.main()
        assert (live / 'old.txt').read_bytes() == b'after'
        with patch.object(deploy, 'check_stopped'), patch.object(sys, 'argv', ['deploy', 'rollback'] + args):
            deploy.main()
        assert (live / 'old.txt').read_bytes() == b'before'
        assert not (live / 'added.txt').exists()
        assert (backup / 'quarantine/added.txt').read_bytes() == b'new'
        with patch.object(deploy.subprocess, 'run') as run:
            run.return_value.stdout = 'true\n'
            try:
                deploy.check_stopped('rathena-map')
                raise AssertionError('Running container accepted')
            except ValueError:
                pass
        try:
            deploy.target(live, '../escape')
            raise AssertionError('Traversal accepted')
        except ValueError:
            pass
    print('PASS: quality service static contracts, walkable cells, client fixtures, scoped apply/rollback')


if __name__ == '__main__':
    main()
