"""Use GNU make to prove bank/reserve include edits invalidate actual objects."""
from pathlib import Path
import os
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parents[2]

def check(role, obj, includes, missing=None):
    template=(ROOT/f'src/{role}/Makefile.in').read_text()
    if missing:
        template=template.replace(' '+missing,'').replace('\t'+missing+' \\\n','')
    with tempfile.TemporaryDirectory(prefix='bank-build-dependencies-') as directory:
        root=Path(directory);cwd=root/'src'/role;cwd.mkdir(parents=True)
        names=['src/common/probe.hpp','src/char/probe.hpp','src/map/probe.hpp',
               'src/config/probe.hpp','3rdparty/libconfig/probe.h',
               '3rdparty/rapidyaml/probe.hpp','3rdparty/json/include/nlohmann/probe.hpp',
               f'src/{role}/{obj}.cpp',f'src/{role}/obj/{obj}.o',
               'src/custom/battle_config_struct.inc','src/custom/script_def.inc',
               'src/custom/script.inc','src/custom/bank_sql.inc',
               'src/custom/reserve_sql.inc','src/custom/bank_commit.hpp',
               'src/custom/bank_protocol.hpp','src/custom/multi_storage_sql.inc',
               'src/custom/multi_storage.hpp','src/custom/multi_storage_protocol.hpp',
               'src/custom/reserve_map.inc']
        for name in names:
            path=root/name;path.parent.mkdir(parents=True,exist_ok=True);path.touch()
            os.utime(path,(1000,1000))
        (cwd/'Makefile').write_text(template.replace('@SET_MAKE@',''))
        os.utime(cwd/f'obj/{obj}.o',(2000,2000))
        def query():
            result=subprocess.run(['make','-q',f'obj/{obj}.o'],cwd=cwd,capture_output=True,text=True)
            assert result.returncode in (0,1),result.stderr
            return result.returncode
        assert query()==0,'Unchanged object must be current'
        for include in includes:
            path=(cwd/include).resolve();os.utime(path,(3000,3000))
            expected=0 if include==missing else 1
            assert query()==expected,(role,include,'missing rebuild dependency')
            os.utime(path,(1000,1000))

if __name__=='__main__':
    check('char','int_storage',['../custom/bank_sql.inc','../custom/reserve_sql.inc'])
    check('map','script',['../custom/reserve_map.inc'])
    check('char','int_storage',['../custom/reserve_sql.inc'],missing='../custom/reserve_sql.inc')
    check('map','script',['../custom/reserve_map.inc'],missing='../custom/reserve_map.inc')
    print('PASS: bank/reserve include edits rebuild the affected char/map object; both old omissions reproduced')
