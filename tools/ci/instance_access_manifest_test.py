# ============================================================================
#  PN  /  DEVELOPMENT TOOLS
#  instance_access_manifest_test.py
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/instance_access_manifest_test.py
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

"""Keep reviewed instance database, enabled scripts and Warper routes connected.

Client asset receipts are installation evidence, not a claim that CI has a client.
This test intentionally checks only source facts available in a clean checkout.
"""
import json
from pathlib import Path
import re

from audit_enchant_upgrades import renewal_records


ROOT = Path(__file__).resolve().parents[2]


def enabled_scripts(root):
    scripts, visited = set(), set()

    def read(relative):
        if relative in visited:
            return
        visited.add(relative)
        path = root / relative
        assert path.is_file(), ('Missing NPC import', relative)
        text = re.sub(r'/\*.*?\*/', '', path.read_text(errors='replace'), flags=re.S)
        for kind, target in re.findall(r'^\s*(npc|import):\s*(\S+)', text, re.M):
            if kind == 'import':
                read(target)
            else:
                assert (root / target).is_file(), ('Missing NPC script', target)
                scripts.add(target)

    read('npc/re/scripts_main.conf')
    return scripts


def main():
    manifest = json.loads((ROOT / 'tools/ci/instance_access_manifest.json').read_text())
    enabled = enabled_scripts(ROOT)
    instances = {}
    for record in renewal_records(ROOT, 'db/instance_db.yml'):
        instances.setdefault(record['Id'], {}).update(record)
    assert set(instances) == {row['id'] for row in manifest['local_instances']}, 'Review the access catalog when instance definitions are added or removed'
    for reviewed in manifest['local_instances']:
        current = instances[reviewed['id']]
        assert current['Name'] == reviewed['name'], ('Instance renamed', reviewed['id'])
        assert [current['Enter']['Map'], *current.get('AdditionalMaps', {})] == reviewed['maps']
        scripts = [path for path in reviewed['name_references'] if 'warper' not in path]
        assert scripts and any(path in enabled for path in scripts), ('Instance script disabled', reviewed['name'])
        assert any('"' + reviewed['name'] + '"' in (ROOT / path).read_text(errors='replace')
                   for path in scripts if path in enabled), ('Instance name no longer referenced', reviewed['name'])
    warper = (ROOT / 'npc/custom/warper.txt').read_text()
    section = warper.split('\n\tInstances:', 1)[1].split('\n\tSpecial:', 1)[0]
    labels = set(re.findall(r'"([^"\r\n]+)",\s*I\d+', section))
    assert all(':' not in label for label in labels), 'A colon splits a visible menu label into extra choices'
    for label in manifest['required_warper_labels']:
        assert label in labels, ('Reviewed Warper route removed', label)
    for row in manifest['reference_catalog']:
        for instance_id in row['local_instance_ids']:
            assert instance_id in instances
        assert all(path in enabled for path in row['enabled_scripts'])
    print(json.dumps({'instance_definitions': len(manifest['local_instances']),
                      'required_warper_routes': len(manifest['required_warper_labels']),
                      'reference_categories': len(manifest['reference_catalog']),
                      'source_access_integrity': 'PASS'}))


if __name__ == '__main__':
    main()
