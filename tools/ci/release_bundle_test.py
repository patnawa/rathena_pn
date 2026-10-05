"""A green partial gate, stale binary or absent rendered proof cannot ship."""
import copy
import json
import shutil
from pathlib import Path
import tempfile
import unittest

import release_bundle as bundle
from release_checks import TESTS, FULL_TESTS


class BundleTest(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.base = Path(self.temporary.name)
        self.root = self.base / 'candidate'
        (self.root / 'src').mkdir(parents=True)
        (self.root / 'src/test.cpp').write_text('original')
        for name in ('map-server', 'char-server'):
            (self.root / name).write_text(name)
        self.identity = bundle.binding(self.root)
        names = TESTS + FULL_TESTS + ('database_yaml_syntax', 'client_compat_assets', 'isolated_startup')
        log=self.base/'native.log';log.write_text('synthetic native fixture')
        self.native = {'passed': True, 'phase': 'full', 'binding': self.identity,
                       'checks': [{'name': n, 'passed': True, 'status': 'passed', 'log': str(log), 'sha256': bundle.sha256(log)} for n in names]}
        self.receipts = {}
        self.write('native', self.native)

    def write(self, kind, value):
        path = self.base / (kind + '.json')
        path.write_text(json.dumps(value))
        self.receipts[kind] = path

    def assemble(self, scopes=('npc-fixes',)):
        return bundle.assemble(self.root, scopes, self.receipts)

    def test_complete_native_scope(self):
        self.assertTrue(self.assemble()['passed'])

    def test_relative_logs_survive_evidence_directory_move(self):
        for row in self.native['checks']:
            row['log'] = 'native.log'
        self.write('native', self.native)
        moved = self.base / 'relocated-evidence'
        moved.mkdir()
        for name in ('native.json', 'native.log'):
            shutil.move(str(self.base / name), moved / name)
        self.receipts['native'] = moved / 'native.json'
        self.assertTrue(self.assemble()['passed'])
        (moved / 'native.log').write_text('changed after relocation')
        self.assertFalse(self.assemble()['passed'])

    def test_relative_log_escape_is_rejected(self):
        outside = self.base.parent / (self.base.name + '-outside.log')
        outside.write_text('outside evidence root')
        self.addCleanup(outside.unlink)
        for row in self.native['checks']:
            row['log'] = '../' + outside.name
            row['sha256'] = bundle.sha256(outside)
        self.write('native', self.native)
        self.assertFalse(self.assemble()['passed'])

    def test_missing_required_capabilities(self):
        result = self.assemble(tuple(bundle.SCOPES))
        self.assertFalse(result['passed'])
        for kind in ('sql', 'rendered', 'responsiveness'):
            self.assertTrue(any('Missing ' + kind in e for e in result['errors']))

    def test_green_partial_or_duplicate_gate_rejected(self):
        for mode in ('partial', 'duplicate', 'failed', 'source'):
            receipt = copy.deepcopy(self.native)
            if mode == 'partial':
                receipt['checks'] = receipt['checks'][-1:]
            elif mode == 'duplicate':
                receipt['checks'].append(receipt['checks'][0])
            elif mode == 'failed':
                receipt['checks'][0]['status'] = 'running'
            else:
                receipt['phase'] = 'source'
            self.write('native', receipt)
            self.assertFalse(self.assemble()['passed'], mode)

    def test_configure_makefiles_do_not_change_source_identity(self):
        for directory in ('src/map', '3rdparty/libconfig'):
            target = self.root / directory
            target.mkdir(parents=True, exist_ok=True)
            (target / 'Makefile.in').write_text('tracked configure template')
        clean = bundle.binding(self.root)
        for directory in ('src/map', '3rdparty/libconfig'):
            (self.root / directory / 'Makefile').write_text('derived build paths')
        self.assertEqual(clean, bundle.binding(self.root))
        (self.root / 'src/map/Makefile').write_text('different configured paths')
        self.assertEqual(clean, bundle.binding(self.root))
        (self.root / 'src/map/Makefile.in').write_text('changed build recipe')
        self.assertNotEqual(clean, bundle.binding(self.root))

    def test_exact_generated_outputs_do_not_change_code_identity(self):
        (self.root/'Makefile.in').write_text('root build template')
        clean=bundle.binding(self.root)
        for name in (*bundle.DERIVED_OUTPUTS, 'makefile'):
            path=self.root/name;path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(b'generated output')
        self.assertEqual(clean,bundle.binding(self.root))
        # Similar extensions remain meaningful unless explicitly named.
        asset=self.root/'client-patch/retained.grf';asset.write_bytes(b'tracked archive')
        self.assertNotEqual(clean,bundle.binding(self.root))

    def test_operational_config_is_separate_and_complete(self):
        clean=bundle.binding(self.root);config=bundle.configuration_inventory(self.root)
        for name in bundle.OPERATIONAL_CONFIG:
            path=self.root/name;path.parent.mkdir(parents=True,exist_ok=True);path.write_text('fixture config')
        self.assertEqual(clean,bundle.binding(self.root))
        current=bundle.configuration_inventory(self.root)
        self.assertTrue(set(bundle.OPERATIONAL_CONFIG)<=current.keys());self.assertNotEqual(config,current)
        (self.root/'src/test.cpp').write_text('changed source')
        self.assertNotEqual(clean,bundle.binding(self.root))

    def test_runtime_import_overrides_remain_bound(self):
        target = self.root / 'db/import'
        target.mkdir(parents=True)
        clean = bundle.binding(self.root)
        (target / 'item_db.yml').write_text('meaningful override')
        self.assertNotEqual(clean, bundle.binding(self.root))

    def test_source_and_binary_drift_rejected(self):
        for name in ('src/test.cpp', 'map-server', 'char-server'):
            path = self.root / name
            original = path.read_bytes()
            path.write_bytes(b'changed')
            self.assertFalse(self.assemble()['passed'], name)
            path.write_bytes(original)

    def test_configuration_is_explicit_without_rebinding_fixture_source(self):
        (self.root / 'conf').mkdir()
        (self.root / 'conf/map.conf').write_text('fixture configuration')
        result = self.assemble()
        self.assertTrue(result['passed'])
        self.assertIn('conf/map.conf', result['configuration_sha256'])

    def test_rendered_requires_named_passed_case_and_intact_artifact(self):
        capture = self.base / 'capture.png'
        capture.write_bytes(b'test fixture, not actual rendered acceptance')
        receipt = {'passed': True, 'binding': self.identity, 'cases': [
            {'name': 'onboarding', 'status': 'passed', 'artifacts': {'capture.png': bundle.sha256(capture)}}]}
        self.write('rendered', receipt)
        self.assertTrue(self.assemble(('guide',))['passed'])
        self.assertFalse(self.assemble(('content',))['passed'])
        capture.write_bytes(b'altered')
        self.assertFalse(self.assemble(('guide',))['passed'])
        receipt['cases'][0]['artifacts'] = {'../capture.png': 'bad'}
        self.write('rendered', receipt)
        self.assertFalse(self.assemble(('guide',))['passed'])

    def test_short_baseline_cannot_pass(self):
        log = self.base / 'metrics.log'
        log.write_text('fixture')
        receipt = {'passed': True, 'binding': self.identity, 'cases': [
            {'name': n, 'status': 'passed', 'duration_seconds': 1800,
             'artifacts': {'metrics.log': bundle.sha256(log)}}
            for n in ('idle-baseline', 'load-baseline', 'process-stall',
                      'single-buyer', 'four-buyers', 'sql-delayed-purchase')]}
        self.write('responsiveness', receipt)
        self.assertTrue(self.assemble(('metrics',))['passed'])
        missing = dict(receipt, cases=receipt['cases'][:3])
        self.write('responsiveness', missing)
        self.assertFalse(self.assemble(('metrics',))['passed'], 'NPC-only workload cannot certify purchases')
        for duration in (1799, float('nan'), float('inf'), True, '1800'):
            receipt['cases'][0]['duration_seconds'] = duration
            self.write('responsiveness', receipt)
            self.assertFalse(self.assemble(('metrics',))['passed'])

    def test_candidate_pass_is_not_deployment_attestation(self):
        result = bundle.assemble(self.root, ('npc-fixes',), self.receipts, 'deployed')
        self.assertFalse(result['passed'])
        self.assertIn('deployment', result['requirements'])

    def test_synthetic_case_cannot_replace_sql_runner(self):
        artifact = self.base / 'note.txt'
        artifact.write_text('not a SQL recovery run')
        self.write('sql', {'passed': True, 'binding': self.identity, 'cases': [
            {'name': 'shop-recovery', 'status': 'passed', 'artifacts': {'note.txt': bundle.sha256(artifact)}}]})
        self.assertFalse(self.assemble(('release',))['passed'])

    def test_sql_binds_actual_linked_objects(self):
        obj = self.root / 'src/test.o'
        obj.write_bytes(b'object fixture')
        receipt = {'passed': True, 'binding': self.identity,
                   'input_sha256': {'src/test.o': bundle.sha256(obj)},
                   'artifacts': {'native.log': bundle.sha256(self.base/'native.log')}}
        for key in ('runtime', 'character_wire_handler', 'concurrent_final_unit',
                    'committed_restart', 'uncommitted_restart'):
            receipt[key] = 'fixture outcome'
        self.write('sql', receipt)
        self.assertTrue(self.assemble(('release',))['passed'])
        obj.write_bytes(b'changed object')
        self.assertFalse(self.assemble(('release',))['passed'])

    def test_unknown_or_empty_scope_rejected(self):
        for scopes in ((), ('typo',)):
            with self.assertRaises(ValueError):
                self.assemble(scopes)


if __name__ == '__main__':
    unittest.main()
