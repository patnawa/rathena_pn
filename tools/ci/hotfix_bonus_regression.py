# ============================================================================
#  PN  /  DEVELOPMENT TOOLS
#  hotfix_bonus_regression.py
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/hotfix_bonus_regression.py
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

"""Compile source-extracted bonus expressions, not the server VM or combat engine."""
import pathlib,re,subprocess,sys,argparse,tempfile
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--root',type=pathlib.Path,default=pathlib.Path(__file__).resolve().parents[2])
parser.add_argument('--build-dir',type=pathlib.Path)
args=parser.parse_args();repo=args.root;out=args.build_dir or pathlib.Path(tempfile.mkdtemp(prefix='hotfix-bonus-'));out.mkdir(parents=True,exist_ok=True)
equip=(repo/'db/re/item_db_equip.yml').read_text(encoding='utf-8')
script=equip.split('  - Id: 24063\n',1)[1].split('  - Id:',1)[0].split('    Script: |\n',1)[1]
script=script.replace('.@r','r').replace('r = getrefine();','int r = refine;')
script=re.sub(r'bonus2\s+([^;]+);',r'bonus2(\1);',script)
script=re.sub(r'bonus\s+([^;]+);',r'bonus(\1);',script)
status=(repo/'db/re/status.yml').read_text(encoding='utf-8')
expr=status.split('  - Status: Mtf_Aspd\n',1)[1].split('  - Status:',1)[0]
expr=re.search(r'bonus bAspd, (.*);',expr).group(1).replace('getstatus(SC_MTF_ASPD, 1)','10')
source='''#include <cassert>
#include <cstdio>
enum {bMaxHP,bAddClass,bMagicAddClass,Class_Boss};
int physical,magical;
void bonus(int,int){}
void bonus2(int type,int cls,int val){assert(cls==Class_Boss);if(type==bAddClass)physical+=val;else if(type==bMagicAddClass)magical+=val;else assert(false);}
void run(int refine){'''+script+'''}
int main(){for(int r=0;r<=20;++r){physical=magical=0;run(r);int expected=r>=9?5:(r>=7?3:2);assert(physical==expected&&magical==expected);}assert(('''+expr+''')==1);puts("PASS: Liberation 21 refine boundaries; Deviruchi +1 ASPD conversion");}
'''
(out/'bonus_regression.cpp').write_text(source)
if sys.platform!='win32':
    subprocess.run(['g++','-std=c++17','-fsanitize=undefined','-fno-sanitize-recover=all',str(out/'bonus_regression.cpp'),'-o',str(out/'bonus_regression')],check=True)
    subprocess.run([str(out/'bonus_regression')],check=True)
else:print('Wrote',out/'bonus_regression.cpp')
