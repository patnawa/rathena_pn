"""Execute the real startup identity verifier against isolated temporary files."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import release_controller as controller

ROOT=Path(__file__).resolve().parents[2]

def main():
    with tempfile.TemporaryDirectory(prefix='pn-runtime-identity-') as temporary:
        work=Path(temporary);source=work/'test.cpp';binary=work/'test';root=work/'candidate'
        source.write_text('''#include <common/runtime_identity.hpp>
#include <iostream>
int main(int argc,char**argv){
 const bool valid=pn_runtime_identity::initialize(argv[1]);
 if(valid && pn_runtime_identity::current().size()!=64)return 2;
 pn_runtime_identity::invalidate();if(!pn_runtime_identity::current().empty())return 3;
 std::cout << (valid ? "valid" : "invalid");return 0;
}
''')
        subprocess.run(['g++','-std=c++17','-I'+str(ROOT/'src'),str(source),'-lcrypto','-o',str(binary)],check=True)
        for name in ('db','npc','conf/import'):(root/name).mkdir(parents=True)
        shutil.copyfile(binary,root/'map-server')
        path=root/controller.RUNTIME_MANIFEST
        def check(valid):
            result=subprocess.check_output([str(binary),str(root)],text=True)
            assert result==('valid' if valid else 'invalid'),result
        check(False)
        path.write_text(controller.runtime_identity_text(root));original=path.read_text();check(True)
        for name in ('conf/new.conf','db/new.yml','npc/new.txt'):
            (root/name).write_text('new');check(False);(root/name).unlink();check(True)
        for text in (original.replace('pn-runtime-v1','pn-runtime-v2'),original+'0'*64+' ../outside\n',original+original.splitlines()[-1]+'\n'):
            path.write_text(text);check(False)
        path.write_text(original);(root/'map-server').write_bytes(b'other executable');check(False)
        # A manifest matching disk still cannot certify a different running executable.
        path.write_text(controller.runtime_identity_text(root));check(False)
        shutil.copyfile(binary,root/'map-server');path.write_text(controller.runtime_identity_text(root));check(True)
        (root/'conf/x.conf').write_text('before');path.write_text(controller.runtime_identity_text(root));check(True)
        (root/'conf/x.conf').write_text('after');check(False)
        print('RUNTIME_IDENTITY_OK startup binary/content verification, extra files, malformed manifest, cached invalidation')

if __name__=='__main__':main()
