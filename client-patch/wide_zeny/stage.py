"""Prepare an isolated economy patch payload and loader QA directory; never install."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess

HERE=Path(__file__).resolve().parent
REPO=HERE.parents[1]
def sha(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream,'sha256').hexdigest()

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--client-root',type=Path,required=True)
    parser.add_argument('--build',type=Path,required=True)
    parser.add_argument('--server-source',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    source=args.client_root.resolve();build=args.build.resolve();output=args.output.resolve()
    if output.exists():raise RuntimeError('Choose a fresh output directory; staging never overwrites an existing release.')
    if output==source or source in output.parents:raise RuntimeError('Staging output must be outside the active client directory.')
    protocols={'bank_protocol.hpp':HERE/'bank_protocol.hpp',
               'market_protocol.hpp':HERE.parent/'wide_market/market_protocol.hpp',
               'mail_protocol.hpp':HERE.parent/'wide_mail/mail_protocol.hpp'}
    for name,local in protocols.items():
        if local.read_bytes()!=(args.server_source/'src/custom'/name).read_bytes():
            raise RuntimeError(f'Client/server protocol snapshot differs: {name}')
    preserved_names=['Ragexe.exe','FontScaleOriginal.dll','FontScale.ini','PN-Turbo.ini','DATA.INI','SystemEN/itemInfo.lua']
    preserved={name:sha(source/name) for name in preserved_names}
    output.mkdir(parents=True)
    payload=output/'payload';payload.mkdir()
    for name in ['FontScale.dll','PNWallet64.dll','PNTurbo.dll']:
        shutil.copyfile(build/name,payload/name)
    shutil.copyfile(HERE/'PNWallet64.ini',payload/'PNWallet64.ini')
    # Retain the user's chosen bank UI scale and server ports in the new filename.
    old_ini=source/'BankUI.ini'
    if old_ini.exists():
        import configparser
        existing=configparser.ConfigParser();existing.read(old_ini,encoding='utf-8-sig')
        candidate=configparser.ConfigParser();candidate.optionxform=str;candidate.read(payload/'PNWallet64.ini',encoding='utf-8-sig')
        for key in ['UiScale','CharacterPort','MapPort']:
            if existing.has_option('Bank',key):candidate['Bank'][key]=existing.get('Bank',key)
        with (payload/'PNWallet64.ini').open('w',encoding='utf-8') as file:candidate.write(file)
    shutil.copyfile(HERE.parent/'account_bank/vendor/MinHook/LICENSE.txt',payload/'PNWallet64-LICENSE.txt')
    qa=output/'loader-qa';qa.mkdir()
    for path in payload.iterdir():shutil.copyfile(path,qa/path.name)
    for name in ['FontScaleOriginal.dll','FontScale.ini','PN-Turbo.ini']:
        shutil.copyfile(source/name,qa/name)
    shutil.copyfile(build/'wide_zeny_loader_test.exe',qa/'wide_zeny_loader_test.exe')
    checked=subprocess.run([str(qa/'wide_zeny_loader_test.exe')],cwd=qa,
                           capture_output=True,text=True,timeout=30)
    (output/'loader-verification.txt').write_text(checked.stdout+checked.stderr,encoding='utf-8')
    if checked.returncode:raise RuntimeError('Staged DLL-chain verification failed; see loader-verification.txt')
    manifest={'status':'staged; not installed or published','client_packet_baseline':20260219,
        'protocols':{'wallet_trade':{'version':3,'request_bytes':80,'reply_bytes':208},
                     'market':{'magic':'PMK1','version':1,'request_bytes':128,'reply_bytes':2760},
                     'mail':{'magic':'PZL1','version':1,'request_bytes':636,'reply_bytes':96}},
        'protocol_sha256':{name:sha(path) for name,path in protocols.items()},
        'payload':{path.name:{'bytes':path.stat().st_size,'sha256':sha(path)} for path in sorted(payload.iterdir())},
        'base_client_files_preserved':preserved,'loader_economy_module':'PNWallet64.dll',
        'legacy_bankui_module_loaded':False,'required_existing_modules':['FontScaleOriginal.dll'],
        'verification':'Windows UI/loopback tests passed; staged DLL-chain loader QA passed; server gameplay acceptance is tracked separately.',
        'loader_test':{'exit_code':checked.returncode,'output':checked.stdout.strip()}}
    (output/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
    (output/'PATCH-NOTES.md').write_text('''# Staged economy client

This payload is prepared for the matching isolated server build. It has not been
installed or published. Load the new FontScale.dll, which forwards the existing
font extension and selects PNWallet64.dll as the only economy module. BankUI.dll
may remain on disk for rollback but must not be loaded at the same time.

The wallet window includes Market and Mail buttons. Trade offers use exact values
and explicit confirmation. Market shops remain drafts until their prices are
reviewed and published. Mail quotes include the full fee and debit, and only a
matching durable server status is labeled Saved. Item-bearing mail and received
mail remain in the native RODEX interface.

Existing font settings, turbo configuration, executable, archives and Lua files
are untouched. The staged PNTurbo.dll supports the new module while retaining its
legacy BankUI fallback. PNWallet64.ini preserves the prior panel scale and ports.

Run loader-qa/wide_zeny_loader_test.exe from loader-qa to verify the actual DLL
chain, font behavior, single economy module, exported session gate and turbo.
Root integration owns the full client manifest, launcher verification, server
rollout and rollback. A safe rollback restores the preceding FontScale.dll and
PNTurbo.dll with its matching old server; do not mix protocol generations.
''',encoding='utf-8')
    if preserved!={name:sha(source/name) for name in preserved_names}:raise RuntimeError('Base client changed while staging; review before packaging.')
    print(f'Staged {payload}; loader QA at {qa}; base client preserved.')

if __name__=='__main__':main()
