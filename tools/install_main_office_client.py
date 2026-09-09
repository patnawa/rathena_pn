#!/usr/bin/env python3
# ============================================================================
#  PN  /  DEVELOPMENT TOOLS
#  install_main_office_client.py
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/install_main_office_client.py
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

"""Install a verified office GRF, preserving archive priority and a rollback copy."""
import argparse
import configparser
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import shutil


def install(client, built):
    client, built = client.resolve(), built.resolve()
    manifest = json.loads((built / 'manifest.json').read_text())
    archive = built / 'pn_office.grf'
    digest = hashlib.sha256(archive.read_bytes()).hexdigest()
    if digest != manifest['grf_sha256']:
        raise ValueError('Office archive differs from the verified build manifest')
    ini_path = client / 'DATA.INI'
    ini = configparser.ConfigParser()
    ini.read_string(ini_path.read_text(encoding='utf-8-sig'))
    if ini.sections() != ['Data'] or any(not key.isdigit() for key in ini['Data']):
        raise ValueError('Unexpected DATA.INI format; preserve and review it')
    order = [v for k, v in sorted(ini['Data'].items(), key=lambda p: int(p[0]))]
    if len(set(n.lower() for n in order)) != len(order):
        raise ValueError('Duplicate archive priorities')
    target = client / 'pn_office.grf'
    if target.exists() and hashlib.sha256(target.read_bytes()).hexdigest() != digest:
        raise ValueError('Existing office archive differs; review before replacing')
    if order[0].lower() == target.name and target.exists():
        print('Verified office client patch is already installed.')
        return
    backup = client / 'server-work' / ('client-before-main-office-' + datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%SZ'))
    backup.mkdir(parents=True, exist_ok=False)
    shutil.copy2(ini_path, backup / 'DATA.INI')
    if target.exists():
        shutil.copy2(target, backup / target.name)
    shutil.copy2(archive, target)
    new_order = [target.name] + [n for n in order if n.lower() != target.name]
    ini_path.write_text('[Data]\n' + ''.join(f'{i}={n}\n' for i, n in enumerate(new_order)), encoding='ascii')
    receipt = {'backup': str(backup), 'old_order': order, 'new_order': new_order, 'grf_sha256': digest}
    (backup / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(json.dumps(receipt, indent=2))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('client', type=Path)
    parser.add_argument('built', type=Path)
    args = parser.parse_args()
    install(args.client, args.built)
