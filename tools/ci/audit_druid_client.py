#!/usr/bin/env python3
# ============================================================================
#  PN  /  DEVELOPMENT TOOLS
#  audit_druid_client.py
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/audit_druid_client.py
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

"""Check explicitly extracted active client Druid tables without executing Lua.

Input directory contains english/, new/, data/ extraction roots. These priorities
match the owner's DATA.INI; this tool does not establish another client's order.
This checks identities/table coverage, not rendered sprites, packets or gameplay.
"""
import argparse
import hashlib
import json
import re
from pathlib import Path

import yaml
from lua51_literal_table import literal_tables


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('extracted', type=Path)
    args = parser.parse_args()
    hashes = {}

    def read(archive, path, binary=False):
        filename = args.extracted / archive / 'data/luafiles514/lua files' / path
        raw = filename.read_bytes()
        hashes[f'{archive}/{path}'] = hashlib.sha256(raw).hexdigest()
        return raw if binary else raw.decode('cp949')

    jobs = literal_tables(read('data', 'datainfo/jobidentity.lub', True))['JTtbl']
    expected_jobs = {'JT_DRUID': 4351, 'JT_KARNOS': 4353, 'JT_ALITEA': 4355}
    for name, identity in expected_jobs.items():
        assert jobs[name] == identity, (name, jobs.get(name), identity)
    npcs = literal_tables(read('data', 'datainfo/npcidentity.lub', True))['jobtbl']
    names = literal_tables(read('data', 'datainfo/jobname.lub', True), {'jobtbl': npcs})
    # The inheritance chunk installs function/metatable handlers. The literal
    # reader deliberately does not execute it; record provenance, not a PASS.
    read('data', 'skillinfoz/jobinheritlist.lub', True)

    ids_text = read('new', 'skillinfoz/skillid.lub')
    client_skills = {name: int(identity) for name, identity in
                     re.findall(r'\b((?:DR|KR|AT)_\w+)\s*=\s*(\d+)', ids_text)}
    records = yaml.load(Path('db/re/skill_db.yml').read_text(), Loader=yaml.CSafeLoader)['Body']
    server_skills = {x['Name']: x['Id'] for x in records if 6524 <= x['Id'] <= 6607}
    assert len(server_skills) == 84, len(server_skills)
    assert client_skills == server_skills, {'client_only': client_skills.keys() - server_skills.keys(),
                                            'server_only': server_skills.keys() - client_skills.keys()}
    info = read('english', 'skillinfoz/skillinfolist.lub')
    description = read('english', 'skillinfoz/skilldescript.lub')
    tree = read('new', 'skillinfoz/skilltreeview.lub')
    pc_names = read('new', 'datainfo/pcjobname.lub')
    for job in expected_jobs:
        assert f'[JOBID.{job}]' in tree and f'[JOBID.{job}]' in pc_names, job
    visible = set(re.findall(r'SKID\.((?:DR|KR|AT)_\w+)', tree))
    for skill in server_skills:
        # Internal damage variants can legitimately omit a visible tree cell.
        if skill in visible:
            assert f'[SKID.{skill}]' in info, ('missing visible info', skill)
            assert f'[SKID.{skill}]' in description, ('missing visible description', skill)
    print(json.dumps({'jobs': expected_jobs, 'matching_skill_ids': len(server_skills),
                      'visible_described_skills': len(visible),
                      'internal_info_entries': sum(f'[SKID.{s}]' in info for s in server_skills.keys() - visible),
                      'job_name_globals': {k: len(v) for k, v in names.items()},
                      'inheritance_runtime': 'not verified (nonliteral metatable code)',
                      'sha256': hashes}, indent=2))


if __name__ == '__main__':
    main()
