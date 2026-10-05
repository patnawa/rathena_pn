"""Rebuild and independently verify a persisted combined-economy certificate.

This requires only the ordinary YAML audit dependency, not SciPy. It refuses
changed inputs/model identities and checks every valuation inequality using
Fraction directly rather than trusting the solver verdict or report booleans.
The report's bounded-model exclusions and optimistic assumptions still apply.
"""
import argparse
from fractions import Fraction
import hashlib
import json
from pathlib import Path
import dynamic_reward_catalog as catalog
from audit_economy_cycles import action_digest, combined_actions
from economy_vendor_catalog import load_vendors, vendor_actions


def verify(report, root=catalog.ROOT):
    if report.get('verdict') != 'no_resource_growth_in_combined_model':
        raise ValueError('Report does not contain a no-growth certificate')
    if report.get('vendor_catalog', {}).get('unsupported'):
        raise ValueError('Unsupported vendor declarations prevent complete static-vendor verification')
    paths = {}
    for group in (report['source_sha256'], report['vendor_catalog']['source_sha256'], report['tool_sha256']):
        for path, digest in group.items():
            if path in paths and paths[path] != digest:
                raise ValueError('Conflicting source bindings: ' + path)
            paths[path] = digest
    for path, digest in paths.items():
        if hashlib.sha256((root / path).read_bytes()).hexdigest() != digest:
            raise ValueError('Source changed: ' + path)
    raw = (root / catalog.CATALOG).read_bytes()
    if hashlib.sha256(raw).hexdigest() != report['catalog_sha256']:
        raise ValueError('Catalog changed')
    data = json.loads(raw)
    catalog.validate(data, root)
    vendors = load_vendors(root)
    ids = {entry['item_id'] for row in data['recipes'] for entry in row['costs'] + row['outputs']}
    ids.update(offer['item_id'] for offer in vendors['offers'])
    added = vendor_actions(vendors['offers'], vendors['items'], ids, vendors['limits']['min_shop_sell'])
    actions, _ = combined_actions(data['recipes'], added)
    if action_digest(actions) != report['combined_model_sha256']:
        raise ValueError('Rebuilt action vectors differ from certified model')
    if len(actions) != report['combined_actions']:
        raise ValueError('Action count changed')
    potentials = {name: Fraction(value) for name, value in report['linear_screen']['certificate']['potentials'].items()}
    resources = {name for row in actions for name, amount in row['net'].items() if amount}
    if set(potentials) != resources or any(value <= 0 for value in potentials.values()):
        raise ValueError('Certificate omits a resource or gives it nonpositive value')
    tight = 0
    for row in actions:
        value = sum((amount * potentials[name] for name, amount in row['net'].items() if amount), Fraction())
        if value > 0:
            raise ValueError('Action increases certified resource value: ' + row['key'])
        tight += value == 0
    return dict(verified=True, actions=len(actions), resources=len(resources), tight_inequalities=tight,
                source_inputs=len(paths), combined_model_sha256=report['combined_model_sha256'], boundary=__doc__)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('certificate_report', type=Path)
    parser.add_argument('--root', type=Path, default=catalog.ROOT)
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    result = verify(json.loads(args.certificate_report.read_text()), args.root)
    print(json.dumps(result, indent=2))
    if args.report:
        args.report.write_text(json.dumps(result, indent=2) + '\n')


if __name__ == '__main__':
    main()
