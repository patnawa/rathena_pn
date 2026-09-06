#!/usr/bin/env python3
"""Verify the 31 sourced Druid aliases in the supplied active client and server.

Read-only: defining a missing server item can resolve an old audit finding
without changing the client. A generic unresolved result is not proof that
the client name table lacks that name.
"""
import argparse
import json
from pathlib import Path

from audit_enchant_upgrades import ClientItemNames


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('client_item_names', type=Path)
    parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[2])
    args = parser.parse_args()
    metadata = json.loads((args.root / 'client-patch/druid_items/item_name_aliases.json').read_text())
    resolve = ClientItemNames(args.client_item_names, args.root)
    failures = []
    client_failures = 0
    for name, identity in metadata['aliases'].items():
        if resolve.client.get(name) != identity:
            client_failures += 1
            failures.append({'reason': 'client_identity_mismatch', 'name': name,
                             'expected_id': identity, 'client_id': resolve.client.get(name)})
        if resolve.server.get(identity) != name:
            failures.append({'reason': 'server_identity_mismatch', 'name': name,
                             'id': identity, 'server_name': resolve.server.get(identity)})
    print(json.dumps({'client_sha256': resolve.sha256, 'client_names': len(resolve.client),
                      'checked_pairs': len(metadata['aliases']), 'failures': failures,
                      'client_identity_changes_needed': bool(client_failures)}, indent=2))
    if failures:
        raise SystemExit(1)


if __name__ == '__main__':
    main()
