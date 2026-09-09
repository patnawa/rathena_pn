# ============================================================================
#  PN  /  DEVELOPMENT TOOLS
#  register_script_travel.py
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/navigation/register_script_travel.py
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

"""Stage generator-only annotations for literal NPC travel; never edits the input server tree."""
import pathlib,re,json,argparse
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--root',type=pathlib.Path,required=True,help='Isolated server source snapshot')
parser.add_argument('--maps',type=pathlib.Path,required=True,help='Raw navi_map_krpri.lub from first generator pass')
parser.add_argument('--output',type=pathlib.Path,required=True)
args=parser.parse_args();live=args.root.resolve();base=args.output.resolve()
assert live!=base,'Use a separate output directory'
base.mkdir(parents=True,exist_ok=True)
active=set()
def visit(rel):
 p=(live/rel).resolve();p.relative_to(live)
 if rel in active or not p.exists():return
 active.add(rel)
 if p.suffix=='.conf':
  for child in re.findall(r'^\s*(?:npc|import)\s*:\s*([^/\r\n][^\r\n]*?)(?:\s*//.*)?$',p.read_text(errors='replace'),re.M):visit(child.strip())
visit('npc/re/scripts_main.conf');visit('npc/scripts_athena.conf')
maps={mp:[mp,mp,0,int(w),int(h)] for mp,w,h in re.findall(r'\{\s*"([^"]+)",\s*"[^"]*",\s*\d+,\s*(\d+),\s*(\d+)\}',args.maps.read_text(errors='replace'))}
assert maps,'No maps in generator output'
header=re.compile(r'(?m)^(?:[\w@#]+,-?\d+,-?\d+,\d+|-)\s*\tscript(?:\([^\n]*?\))?\t([^\t]+)\t[^\n]*?\{')
def close(s,start):
 depth=0;string=False;line=False;block=False;i=start
 while i<len(s):
  c=s[i];n=s[i:i+2]
  if line:
   if c=='\n':line=False
  elif block:
   if n=='*/':block=False;i+=1
  elif string:
   if c=='\\':i+=1
   elif c=='"':string=False
  elif n=='//':line=True;i+=1
  elif n=='/*':block=True;i+=1
  elif c=='"':string=True
  elif c=='{':depth+=1
  elif c=='}':
   depth-=1
   if depth==0:return i
  i+=1
 raise ValueError('Unclosed NPC')
changes=[];out=base/'registration-source';out.mkdir(exist_ok=True)
for rel in sorted(active):
 if not rel.endswith('.txt'):continue
 # Register static NPC travel throughout the active world, so episode approaches connect through towns too.
 p=live/rel;s=p.read_text(encoding='utf-8',errors='surrogateescape');edits=[]
 masked=re.sub(r'"(?:\\.|[^"\\])*"|//[^\n]*|/\*.*?\*/',lambda m:m[0] if m[0].startswith('"') else re.sub(r'[^\n]',' ',m[0]),s,flags=re.S)
 for m in header.finditer(masked):
  try:end=close(s,m.end()-1)
  except ValueError as e:raise ValueError((rel,m[1],s.count("\n",0,m.start())+1)) from e
  body=s[m.end():end]
  if 'OnNaviGenerate:' in body:continue
  clean=re.sub(r'/\*.*?\*/','',body,flags=re.S);clean=re.sub(r'(?m)^\s*//[^\n]*','',clean)
  destinations=[]
  for w in re.finditer(r'\bwarp\s*\(?\s*"([\w@]+)"\s*,\s*(\d+)\s*,\s*(\d+)',clean):
   mp,x,y=w.groups();x=int(x);y=int(y)
   if mp not in maps or x==0 or y==0 or x>=maps[mp][3] or y>=maps[mp][4]:continue
   destination=(mp,x,y)
   if destination not in destinations:destinations.append(destination)
  if not destinations:continue
  label='\n\tend;\n// Navigation describes existing script travel; normal quest checks still apply.\nOnNaviGenerate:\n'
  for mp,x,y in destinations:label+=f'\tnaviregisterwarp("Quest travel > {mp}", "{mp}", {x}, {y});\n'
  label+='\tend;\n'
  edits.append((end,label));changes.append(dict(file=rel,npc=m[1],line=s.count('\n',0,m.start())+1,destinations=destinations))
 if edits:
  for pos,label in reversed(edits):s=s[:pos]+label+s[pos:]
  dest=out/rel;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_text(s,encoding='utf-8',errors='surrogateescape',newline='\n')
(base/'registration-manifest.json').write_text(json.dumps(changes,indent=2))
print('Navigation metadata candidates:',len(changes),'NPC scripts in',len({r['file'] for r in changes}),'files;',sum(len(r['destinations']) for r in changes),'destinations')
print('Episode registrations:',sum('episode' in r['file'] or re.search('quests_1[3-9]',r['file']) is not None for r in changes))
