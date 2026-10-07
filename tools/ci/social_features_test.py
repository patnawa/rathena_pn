"""Run party ownership, public-build privacy and lab challenges in the real VM.

Linux map objects required. Registry, player/party lookup and transport are
explicit doubles. VM builtins, equipped-item reads and script logic are real.
Networking is denied by the inherited harness; no live database is touched.
"""
import argparse
from pathlib import Path
import subprocess
import tempfile

from biosphere_crown_transaction_test import WRAPPERS


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path)
    args = parser.parse_args()
    root = (args.root or Path(__file__).resolve().parents[2]).resolve()
    prefix = (root/'tools/ci/biosphere_crown_transaction_test.cpp').read_text().split('extern "C" int __wrap_main(', 1)[0]
    prefix = prefix.replace('check(name.rfind("$@__SW",0)==0&&name.substr(name.size()-4)=="_VAL",', 'check(name[0]==\'$\',')
    prefix = prefix.replace('nums[key]=value;return true;}', 'nums[key]=value;if(script_getvaridx(key))script_array_update(&attached->regs,key,value==0);return true;}', 1)
    prefix = prefix.replace('strings[key]=value;return true;}', 'strings[key]=value;if(script_getvaridx(key))script_array_update(&attached->regs,key,!value||!*value);return true;}', 1)
    prefix = prefix.replace('++pause<30', '++pause<100')
    prefix = prefix.replace('extern "C" map_session_data* lookup(int32 id){return attached&&attached->id==id?attached:nullptr;}',
        'map_session_data* social_other=nullptr;\nextern "C" map_session_data* lookup(int32 id){return attached&&attached->id==id?attached:social_other&&social_other->id==id?social_other:nullptr;}')
    with tempfile.TemporaryDirectory(prefix='social-features-') as temp:
        work = Path(temp)
        driver = work/'driver.cpp'
        driver.write_text(prefix+(root/'tools/ci/social_features_test.cpp').read_text())
        objects = list((root/'src/map/obj').rglob('*.o'))
        if not objects:
            raise SystemExit('Build current Linux map objects first')
        libs = [root/x for x in ('src/common/obj/common.a', '3rdparty/libconfig/obj/libconfig.a', '3rdparty/rapidyaml/obj/ryml.a')]
        includes = ['src','3rdparty/libconfig','3rdparty/rapidyaml/src','3rdparty/rapidyaml/ext/c4core/src','3rdparty/json/include','/usr/include/mysql']
        wrappers = (*WRAPPERS,'_Z14pc_setregistryP16map_session_datall','_Z18pc_setregistry_strP16map_session_datalPKc','_Z19pc_readregistry_strPK16map_session_datal','_Z12party_searchi','_Z16mapreg_setregstrlPKc','_Z17mapreg_readregstrl','_Z11map_msg_txtPK16map_session_datai','_Z16clif_scriptclearRK16map_session_datai','_Z15clif_navigateToPK16map_session_dataPKctthbt','_Z18map_mapindex2mapidt','_Z9pc_setposP16map_session_datatii8clr_type','_Z11map_nick2sdPKcb','_Z13map_charid2sdi','_Z12party_inviteR16map_session_dataPS_','_Z14party_isleaderPK16map_session_data')
        binary=work/'social-test'
        command=['g++','-std=c++17','-O0','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-DPACKETVER=20260219']+['-I'+x for x in includes]+[str(driver)]+[str(x) for x in objects+libs]+['-Wl,--wrap='+x for x in wrappers]+['-lz','-ldl','-lmysqlclient','-l:libzstd.so.1','-lssl','-lcrypto','-lresolv','-lm','-o',str(binary)]
        subprocess.run(command,cwd=root,check=True)
        result=subprocess.run([str(binary)],cwd=root,capture_output=True,text=True)
        output=result.stdout+result.stderr
        print(output,end='')
        result.check_returncode()
        assert 'SOCIAL_FEATURES_OK' in output and 'Memory manager: No memory leaks found.' in output
        assert 'TEST FAIL' not in output and '[Error]' not in output


if __name__=='__main__':
    main()
