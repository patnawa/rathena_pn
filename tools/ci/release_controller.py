"""Release evidence controller: derive scope from a complete baseline inventory.

Baseline creation reads an immutable Git commit. Candidate comparison includes
untracked/import files and deletions; scope declarations can add requirements but
cannot remove inferred ones. No deploy, restart, or publication is performed.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys

import release_bundle as bundle

RUNTIME_MANIFEST = 'conf/import/pn_runtime_identity'


def canonical(value):
    return hashlib.sha256(json.dumps(value, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def inputs(root):
    result = bundle.inventory(root, (*bundle.SOURCE_DIRS, 'conf'))
    result.update(bundle.build_inventory(root))
    result.update(bundle.configuration_inventory(root))
    result.pop(RUNTIME_MANIFEST, None)
    return result


def baseline_from_git(root, ref):
    def git(*args):
        return subprocess.check_output(['git', '-C', str(root), *args])
    commit = git('rev-parse', '--verify', ref+'^{commit}').decode().strip()
    tree = git('ls-tree', '-rz', '--full-tree', commit)
    rows = []
    for entry in tree.split(b'\0'):
        if not entry: continue
        meta, name = entry.split(b'\t', 1)
        mode, kind, oid = meta.split()
        name = name.decode('utf-8')
        if name.split('/')[0] not in (*bundle.SOURCE_DIRS, 'conf') and name not in bundle.BUILD_FILES: continue
        if kind != b'blob' or mode not in (b'100644', b'100755'):
            raise ValueError('Unsupported baseline input: '+name)
        rows.append((name, oid))
    files = {}
    with subprocess.Popen(['git', '-C', str(root), 'cat-file', '--batch'], stdin=subprocess.PIPE, stdout=subprocess.PIPE) as process:
        for name, oid in rows:
            process.stdin.write(oid+b'\n'); process.stdin.flush()
            header = process.stdout.readline().split()
            if len(header)!=3 or header[1]!=b'blob': raise ValueError('Cannot read baseline object')
            data = process.stdout.read(int(header[2]))
            if process.stdout.read(1)!=b'\n': raise ValueError('Incomplete baseline object')
            files[name] = hashlib.sha256(data).hexdigest()
        process.stdin.close()
        if process.wait()!=0: raise ValueError('Git baseline read failed')
    return {'schema': 1, 'commit': commit, 'files': files, 'inventory_sha256': canonical(files)}


def changed_inputs(root, baseline):
    old = baseline['files']
    if baseline.get('schema')!=1 or not old or canonical(old)!=baseline.get('inventory_sha256'):
        raise ValueError('Invalid baseline inventory')
    current = inputs(root)
    return {name: {'before': old.get(name), 'after': current.get(name)}
            for name in sorted(old.keys() | current.keys()) if old.get(name)!=current.get(name)}


def mandatory_scopes(paths):
    scopes = set()
    for name in paths:
        name = name.replace('\\', '/').lower()
        # Shared engine code can affect all journeys. Unknown production inputs
        # deliberately require the full contract instead of guessing a narrow scope.
        if name.startswith(('src/', '3rdparty/', 'sql-files/')):
            scopes.update(bundle.SCOPES)
        elif name.startswith(('tools/', '.github/', 'conf/')) or name in bundle.BUILD_FILES:
            scopes.add('release')
            if 'metric' in name or 'health' in name: scopes.add('metrics')
        elif name.startswith('client-patch/'):
            scopes.add('client-fixes')
        elif name.startswith(('npc/', 'db/')):
            scopes.update(('content', 'npc-fixes'))
            if any(word in name for word in ('onboard', 'chapter', 'office', 'warper', 'instance')): scopes.add('guide')
            if any(word in name for word in ('pet', 'enchan_sage_legacy')): scopes.add('pets')
            if 'quality_services' in name or 'lab' in name: scopes.add('lab')
            if 'barter' in name: scopes.add('barter')
            if 'market' in name: scopes.add('market')
        else:
            scopes.update(bundle.SCOPES)
    return scopes


def runtime_identity_text(root):
    files = {}
    for directory in ('db', 'npc', 'conf'):
        if not (root/directory).is_dir(): raise ValueError('Missing runtime directory: '+directory)
        for path in (root/directory).rglob('*'):
            if path.is_symlink(): raise ValueError('Runtime symlink rejected: '+str(path))
            if path.is_file():
                name=path.relative_to(root).as_posix()
                if name!=RUNTIME_MANIFEST: files[name]=bundle.sha256(path)
    files['map-server']=bundle.sha256(root/'map-server')
    return 'pn-runtime-v1\nsource '+bundle.binding(root)['source_sha256']+'\n'+''.join(
        value+' '+name+'\n' for name,value in sorted(files.items()))


def verify_runtime_identity(root):
    if (root/RUNTIME_MANIFEST).read_text()!=runtime_identity_text(root):
        raise ValueError('Runtime identity differs from current binary/configuration/content')


def contract(root, baseline, scopes):
    changes = changed_inputs(root, baseline)
    required = mandatory_scopes(changes)
    declared = set(scopes) if scopes else required
    if required-declared:
        raise ValueError('Omitted mandatory scopes: '+', '.join(sorted(required-declared)))
    if not declared or declared-set(bundle.SCOPES): raise ValueError('Known nonempty scopes required')
    value = {'baseline_commit': baseline['commit'], 'baseline_inventory_sha256': baseline['inventory_sha256'],
             'changes': changes, 'scopes': sorted(declared), 'binding': bundle.binding(root),
             'configuration_sha256': bundle.configuration_inventory(root)}
    value['contract_sha256'] = canonical(value)
    return value


def assemble(root, baseline, scopes, receipts, stage='candidate', candidate_bundle=None):
    expected = contract(root, baseline, scopes)
    result = bundle.assemble(root, expected['scopes'], receipts, stage)
    result['release_contract'] = expected
    if 'lab' in expected['scopes']:
        try: verify_runtime_identity(root)
        except (ValueError, OSError) as exc: result['errors'].append('runtime identity: '+str(exc))
    if stage=='deployed':
        try:
            if candidate_bundle is None: raise ValueError('Candidate bundle required for deployed stage')
            candidate = json.loads(candidate_bundle.read_text())
            if not candidate.get('passed') or candidate.get('stage')!='candidate' or candidate.get('release_contract')!=expected:
                raise ValueError('Candidate bundle contract differs')
            receipt = json.loads(receipts['deployment'].read_text())
            if receipt.get('candidate_bundle_sha256')!=bundle.sha256(candidate_bundle):
                raise ValueError('Deployment receipt does not bind candidate bundle')
            attestations = receipt.get('scope_attestations', {})
            if set(attestations)!=set(expected['scopes']): raise ValueError('Deployment scope attestations incomplete')
            for scope, row in attestations.items():
                if row.get('status')!='passed': raise ValueError('Deployed scope not passed: '+scope)
                bundle.verify_files(receipts['deployment'].parent, row.get('artifacts'))
        except (ValueError, OSError, KeyError, TypeError) as exc:
            result['errors'].append('release controller: '+str(exc))
    if contract(root, baseline, scopes)!=expected: result['errors'].append('Candidate changed during controller validation')
    result['passed'] = not result['errors']
    return result


def verify_candidate_bundle(root, path, baseline_path):
    previous = json.loads(path.read_text())
    if previous.get('stage')!='candidate' or previous.get('passed') is not True:
        raise ValueError('A passed candidate controller bundle is required')
    baseline = json.loads(baseline_path.read_text())
    receipts = {kind: Path(row['path']) for kind, row in previous.get('evidence', {}).items()}
    for kind, receipt in receipts.items():
        if bundle.sha256(receipt)!=previous['evidence'][kind]['sha256']: raise ValueError('Evidence receipt changed: '+kind)
    result = assemble(root, baseline, previous.get('scopes'), receipts)
    if not result['passed'] or result['release_contract']!=previous.get('release_contract'):
        raise ValueError('Candidate controller verification failed: '+str(result['errors']))
    return result


def write_json(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    temp=path.with_name(path.name+'.tmp');temp.write_text(json.dumps(value, indent=2)+'\n');temp.replace(path)


def main():
    p=argparse.ArgumentParser(description=__doc__);sub=p.add_subparsers(dest='mode',required=True)
    base=sub.add_parser('baseline');base.add_argument('--repository',type=Path,required=True);base.add_argument('--ref',required=True);base.add_argument('--output',type=Path,required=True)
    attest=sub.add_parser('runtime-identity');attest.add_argument('--candidate',type=Path,required=True)
    check=sub.add_parser('check');check.add_argument('--candidate',type=Path,required=True);check.add_argument('--baseline',type=Path,required=True)
    check.add_argument('--scope',action='append');check.add_argument('--evidence',action='append',default=[]);check.add_argument('--stage',choices=('candidate','deployed'),default='candidate');check.add_argument('--candidate-bundle',type=Path);check.add_argument('--output',type=Path,required=True)
    a=p.parse_args()
    if a.mode=='baseline':write_json(a.output,baseline_from_git(a.repository,a.ref));return 0
    if a.mode=='runtime-identity':
        root=a.candidate.resolve();text=runtime_identity_text(root);path=root/RUNTIME_MANIFEST
        path.parent.mkdir(parents=True,exist_ok=True);path.write_text(text);verify_runtime_identity(root);return 0
    root=a.candidate.resolve();output=a.output.resolve()
    if root==output or root in output.parents:p.error('Evidence output must be outside candidate')
    receipts={}
    for value in a.evidence:
        kind,sep,path=value.partition('=')
        if not sep or kind in receipts:p.error('Unique KIND=REPORT evidence required')
        receipts[kind]=Path(path).resolve()
    result=assemble(root,json.loads(a.baseline.read_text()),a.scope,receipts,a.stage,a.candidate_bundle)
    write_json(output,result);print(json.dumps({'passed':result['passed'],'errors':result['errors']}));return 0 if result['passed'] else 1

if __name__=='__main__':sys.exit(main())
