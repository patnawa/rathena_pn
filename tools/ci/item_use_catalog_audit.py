"""Survey actual compiled effective item scripts and active global functions.
Outputs exact affected item IDs; does not assert that conservative refusals are acceptable.
"""
from pathlib import Path
import argparse,json,re,subprocess,tempfile,yaml
from biosphere_crown_transaction_test import WRAPPERS
from audit_enchant_upgrades import renewal_records
from biosphere_callback_closure_audit import Reader,npc_graph
from biosphere_material_callback_audit import lexical_views,body_end
ROOT=Path(__file__).resolve().parents[2]
def main():
 parser=argparse.ArgumentParser();parser.add_argument('--output',type=Path,required=True);args=parser.parse_args()
 effective={}
 for row in renewal_records(ROOT,'db/item_db.yml'):effective.setdefault(row['Id'],{}).update(row)
 reader=Reader(ROOT);graph=npc_graph(reader);functions={}
 for path in graph['scripts']:
  text=reader.text(path);_,masked=lexical_views(text)
  for match in re.finditer(r'(?m)^\s*function\s+script\s+(\w+)\s*\{',masked):
   start=masked.index('{',match.start());functions[match[1]]=(text[start:body_end(masked,start)],path)
 with tempfile.TemporaryDirectory(prefix='pn-item-catalog-') as directory:
  out=Path(directory);rows=[]
  for ident,row in sorted(effective.items()):
   rows.append({key:value for key,value in row.items() if key not in ('Script','EquipScript','UnEquipScript')})
  (out/'items.yml').write_text(yaml.safe_dump({'Body':rows},sort_keys=False))
  manifest=[]
  for index,(name,(source,path)) in enumerate(functions.items()):
   filename=f'f{index}.txt';(out/filename).write_bytes(source.encode('utf-8',errors='surrogateescape'));manifest.append(f'{name}\t{filename}\n')
  (out/'functions.tsv').write_text(''.join(manifest))
  manifest=[]
  for ident,row in sorted(effective.items()):
   if row.get('Script'):
    filename=f'i{ident}.txt';(out/filename).write_text('{\n'+row['Script']+'\n}');manifest.append(f'{ident}\t{filename}\n')
  (out/'scripts.tsv').write_text(''.join(manifest))
  names={row.get('AegisName'):ident for ident,row in effective.items()}
  eggs={names[row['EggItem']] for row in renewal_records(ROOT,'db/pet_db.yml') if row.get('EggItem') in names}
  (out/'eggs.txt').write_text('\n'.join(map(str,sorted(eggs))))
  groups=[]
  for row in renewal_records(ROOT,'db/item_group_db.yml'):
   clean={'Group':row['Group'],'SubGroups':[]}
   for group in row.get('SubGroups',[]):
    sub={key:group[key] for key in ('SubGroup','Algorithm','Clear') if key in group}
    sub['List']=[{key:item[key] for key in ('Index','Item','Rate','Amount','Clear') if key in item} for item in group.get('List',[])]
    clean['SubGroups'].append(sub)
   groups.append(clean)
  (out/'groups.yml').write_text(yaml.safe_dump({'Body':groups},sort_keys=False))
  prefix=(ROOT/'tools/ci/biosphere_crown_transaction_test.cpp').read_text().split('extern "C" int __wrap_main(',1)[0]
  cpp=out/'audit.cpp';cpp.write_text(prefix+(ROOT/'tools/ci/item_use_catalog_audit.cpp').read_text())
  flags=['g++','-std=c++17','-O0','-DPACKETVER=20260219']+['-I'+str(ROOT/p) for p in ('src','3rdparty/libconfig','3rdparty/rapidyaml/src','3rdparty/rapidyaml/ext/c4core/src','3rdparty/json/include')]+['-I/usr/include/mysql']
  objects=list((ROOT/'src/map/obj').rglob('*.o'));libs=[ROOT/p for p in ('src/common/obj/common.a','3rdparty/libconfig/obj/libconfig.a','3rdparty/rapidyaml/obj/ryml.a')]
  exe=out/'audit';subprocess.run(flags+[str(cpp)]+[str(p) for p in objects+libs]+['-Wl,--wrap='+x for x in WRAPPERS]+['-lz','-ldl','-lmysqlclient','-l:libzstd.so.1','-lssl','-lcrypto','-lresolv','-lm','-o',str(exe)],cwd=ROOT,check=True)
  result=subprocess.run([str(exe),str(out)],cwd=ROOT,text=True,capture_output=True,timeout=600)
  affected=[];compile_failures=[];counts={'ordinary':0,'pet_supported':0,'pet_world_refused':0,'unknown_pet':0}
  for line in result.stdout.splitlines():
   if line.startswith('COMPILE_FAILURE '):compile_failures.append(line)
   if not line.startswith('ITEM_EFFECT '):continue
   _,ident,effect=line.split();ident=int(ident);effect=int(effect)
   category='ordinary' if not effect&1 else 'pet_world_refused' if effect&2 else 'pet_supported'
   counts[category]+=1
   if effect&4:counts['unknown_pet']+=1
   if effect&1:affected.append({'id':ident,'name':effective[ident].get('AegisName'),'effects':effect,'category':category,'script':effective[ident]['Script']})
  report={'items':len(effective),'scripts':len(manifest),'active_npc_files':len(graph['scripts']),'global_functions':len(functions),'counts':counts,'affected':affected,'compile_failures':compile_failures,'native_returncode':result.returncode,'stdout':result.stdout,'stderr':result.stderr}
  args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(report,indent=2,ensure_ascii=True)+'\n')
  print(json.dumps({key:report[key] for key in ('items','scripts','active_npc_files','global_functions','counts','native_returncode')},indent=2))
  if result.returncode or compile_failures or 'Memory leaks found' in result.stdout+result.stderr:raise SystemExit(1)
if __name__=='__main__':main()
