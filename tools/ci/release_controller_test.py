"""Portable integrity tests; synthetic receipts are never deployment evidence."""
import copy
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch
import release_bundle as bundle
import release_controller as controller
from release_checks import TESTS,FULL_TESTS

class ControllerTest(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory();self.addCleanup(self.temp.cleanup)
        self.base=Path(self.temp.name);self.root=self.base/'candidate'
        for d in ('src','tools','db','npc','conf'): (self.root/d).mkdir(parents=True)
        (self.root/'src/a.cpp').write_text('original')
        for name in ('map-server','char-server'):(self.root/name).write_text(name)
        files=controller.inputs(self.root)
        self.baseline={'schema':1,'commit':'a'*40,'files':files,'inventory_sha256':controller.canonical(files)}
        self.receipts={};identity=bundle.binding(self.root)
        log=self.base/'native.log';log.write_text('synthetic native fixture')
        self.write('native',{'passed':True,'phase':'full','binding':identity,'checks':[
            {'name':name,'passed':True,'status':'passed','log':str(log),'sha256':bundle.sha256(log)} for name in (*TESTS,*FULL_TESTS,'database_yaml_syntax','client_compat_assets','isolated_startup')]})
    def write(self,kind,value):
        path=self.base/(kind+'.json');path.write_text(json.dumps(value));self.receipts[kind]=path;return path
    def result(self,scopes=('npc-fixes',),stage='candidate',candidate=None):
        return controller.assemble(self.root,self.baseline,scopes,self.receipts,stage,candidate)
    def test_no_changes_still_requires_explicit_nonempty_scope(self):
        self.assertTrue(self.result()['passed'])
        with self.assertRaises(ValueError):self.result(None)
    def test_untracked_and_deleted_inputs_cannot_be_omitted(self):
        (self.root/'tools/new.py').write_text('new')
        with self.assertRaisesRegex(ValueError,'release'):self.result()
        (self.root/'tools/new.py').unlink();(self.root/'src/a.cpp').unlink()
        with self.assertRaisesRegex(ValueError,'Omitted'):self.result()
    def test_unknown_shared_source_requires_all_scopes(self):
        self.assertEqual(controller.mandatory_scopes(['src/new/unknown.cpp']),set(bundle.SCOPES))
    def test_scope_mappings_cover_runtime_and_client_paths(self):
        self.assertEqual(controller.mandatory_scopes(['client-patch/weight/item.lua']),{'client-fixes'})
        self.assertTrue({'content','guide','npc-fixes'}<=controller.mandatory_scopes(['npc/custom/main_office.txt']))
        self.assertEqual(controller.mandatory_scopes(['conf/import/private.conf']),{'release'})
    def test_baseline_tampering_rejected(self):
        self.baseline['files']['src/a.cpp']='0'*64
        with self.assertRaisesRegex(ValueError,'baseline'):self.result()
    def test_renamed_file_records_both_sides(self):
        (self.root/'src/a.cpp').rename(self.root/'src/b.cpp')
        changes=controller.changed_inputs(self.root,self.baseline)
        self.assertIsNone(changes['src/a.cpp']['after']);self.assertIsNone(changes['src/b.cpp']['before'])
    def test_verify_rechecks_original_receipts_and_artifacts(self):
        result=self.result();path=self.write('candidate-bundle',result)
        baseline=self.write('baseline',self.baseline)
        self.assertTrue(controller.verify_candidate_bundle(self.root,path,baseline)['passed'])
        report=json.loads(self.receipts['native'].read_text());report['checks'].pop();self.write('native',report)
        with self.assertRaisesRegex(ValueError,'changed'):controller.verify_candidate_bundle(self.root,path,baseline)
    def deployment(self,candidate):
        artifact=self.base/'deployment.log';artifact.write_text('synthetic fixture')
        row={'status':'passed','artifacts':{'deployment.log':bundle.sha256(artifact)}}
        receipt={'passed':True,'binding':bundle.binding(self.root),'configuration_sha256':bundle.configuration_inventory(self.root),
                 'cases':[dict(row,name=n) for n in ('backup-restore','deployed-hashes','health','data-invariants','git-remote')],
                 'scope_attestations':{'npc-fixes':row},'candidate_bundle_sha256':bundle.sha256(candidate)}
        self.write('deployment',receipt);return receipt
    def test_deployed_requires_exact_candidate_and_each_scope(self):
        candidate=self.write('candidate-bundle',self.result());receipt=self.deployment(candidate)
        self.assertTrue(self.result(stage='deployed',candidate=candidate)['passed'])
        receipt['scope_attestations']={};self.write('deployment',receipt)
        self.assertFalse(self.result(stage='deployed',candidate=candidate)['passed'])
    def test_deployed_cannot_bind_other_candidate(self):
        candidate=self.write('candidate-bundle',self.result());receipt=self.deployment(candidate)
        receipt['candidate_bundle_sha256']='0'*64;self.write('deployment',receipt)
        self.assertFalse(self.result(stage='deployed',candidate=candidate)['passed'])
    def test_runtime_identity_binds_binary_and_all_configuration_contents(self):
        path=self.root/controller.RUNTIME_MANIFEST;path.parent.mkdir(parents=True)
        path.write_text(controller.runtime_identity_text(self.root));controller.verify_runtime_identity(self.root)
        for name in ('conf/changed.conf','db/new.yml','npc/new.txt','map-server'):
            target=self.root/name;old=target.read_bytes() if target.exists() else None
            target.write_bytes(b'changed')
            with self.assertRaisesRegex(ValueError,'Runtime identity'):controller.verify_runtime_identity(self.root)
            if old is None:target.unlink()
            else:target.write_bytes(old)
    def test_operational_configuration_drift_is_not_source_drift(self):
        target=self.root/'tools/docker/asset/inter_conf.txt';target.parent.mkdir(parents=True)
        target.write_text('fixture configuration before')
        source=bundle.binding(self.root);before=bundle.configuration_inventory(self.root)
        files=controller.inputs(self.root);self.baseline.update(files=files,inventory_sha256=controller.canonical(files))
        candidate=self.write('candidate-bundle',self.result());receipt=self.deployment(candidate)
        self.assertTrue(self.result(stage='deployed',candidate=candidate)['passed'])
        target.write_text('fixture configuration after')
        self.assertEqual(source,bundle.binding(self.root))
        self.assertNotEqual(before,bundle.configuration_inventory(self.root))
        result=bundle.assemble(self.root,('npc-fixes',),self.receipts,stage='deployed')
        self.assertFalse(result['passed'])
        self.assertTrue(any('configuration differs' in error for error in result['errors']))
        self.assertIn('tools/docker/asset/inter_conf.txt',controller.changed_inputs(self.root,self.baseline))
        self.assertIn('release',controller.mandatory_scopes(['tools/docker/asset/inter_conf.txt']))

    def test_configuration_drift_after_bundle_rejected(self):
        candidate=self.write('candidate-bundle',self.result());baseline=self.write('baseline',self.baseline)
        (self.root/'conf/private.conf').write_text('changed')
        with self.assertRaises(ValueError):controller.verify_candidate_bundle(self.root,candidate,baseline)

if __name__=='__main__':unittest.main()
