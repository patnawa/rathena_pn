"""Bounded, scope-local reward expression candidates for manual QA.
Candidate sets overapproximate branch/index choices. Unknown dependencies are
reported; a candidate is not proof of obtainability or a complete outcome set.
"""
import argparse,json,re,hashlib,sys
from collections import defaultdict
from pathlib import Path
import audit_item_acquisition as base
VAR=r"[.@$#']*[A-Za-z_]\w*\$?"

def split_args(text):
 out=[];start=0;stack=[];quoted=False;escape=False
 for i,c in enumerate(text):
  if quoted:
   if escape:escape=False
   elif c=='\\':escape=True
   elif c=='"':quoted=False
  elif c=='"':quoted=True
  elif c in '([':stack.append(c)
  elif c in ')]':
   if stack:stack.pop()
  elif c==',' and not stack:out.append(text[start:i].strip());start=i+1
 out.append(text[start:].strip());return out

def scopes(text):
 # NPC/functions begin at a tab-delimited header; brace matching honors strings.
 starts=list(re.finditer(r'(?m)^(?:function\s+script\s+[^\s{]+|[^\n]*\tscript(?:\(DISABLED\))?\t[^\n]*?)\s*\{',text))
 for m in starts:
  pos=m.end();depth=1;quote=False;esc=False
  for i in range(pos,len(text)):
   c=text[i]
   if quote:
    if esc:esc=False
    elif c=='\\':esc=True
    elif c=='"':quote=False
   elif c=='"':quote=True
   elif c=='{':depth+=1
   elif c=='}':
    depth-=1
    if not depth:yield m[0].split('{')[0].strip(),pos,text[pos:i];break

def candidates(body,names):
 assignments=defaultdict(list)
 for m in re.finditer(r'\bsetarray\s+('+VAR+r')(?:\s*\[([^;]*?)\])?\s*,([^;]+);',body):
  exprs=split_args(m[3]);assignments[m[1]].extend(exprs)
  start=(m[2] or '0').strip()
  if start.isdigit():
   for i,e in enumerate(exprs):assignments[m[1]+'['+str(int(start)+i)+']'].append(e)
 for m in re.finditer(r'(?<![\w])('+VAR+r')(?:\[[^;=]*?\])?\s*=(?!=)([^;]+);',body):assignments[m[1]].append(m[2].strip())
 for m in re.finditer(r'\bset\s+('+VAR+r')(?:\[[^;]*?\])?\s*,([^;]+);',body):assignments[m[1]].append(m[2].strip())
 def value(expr,seen=frozenset()):
  expr=expr.strip()
  if re.fullmatch(r'-?\d+',expr):return {int(expr)},set()
  if expr.startswith('"') and expr.endswith('"'):
   v=names.get(expr[1:-1]);return ({v},set()) if v else (set(),{expr})
  if expr in names:return {names[expr]},set()
  if re.fullmatch(VAR+r'(?:\[.*\])?',expr):
   key=expr if expr in assignments else expr.split('[')[0]
   if key in seen:return set(),{key+' cycle'}
   if key not in assignments:return set(),{key}
   vals=set();unknown=set()
   for e in assignments[key]:
    a,b=value(e,seen|{key});vals|=a;unknown|=b
   return vals,unknown
  tern=re.search(r'\?([^?:]+):([^?:]+)$',expr)
  if tern:
   a,b=value(tern[1].strip(' ()'),seen);c,d=value(tern[2].strip(' ()'),seen);return a|c,b|d
  # Known short constant ranges only; do not guess an unknown rand/select result.
  rnd=re.fullmatch(r'rand\(\s*(\d+)\s*,\s*(\d+)\s*\)',expr)
  if rnd and 0<=int(rnd[2])-int(rnd[1])<=200:return set(range(int(rnd[1]),int(rnd[2])+1)),set()
  arithmetic=re.fullmatch(r'('+VAR+r'|\d+)\s*([+\-])\s*('+VAR+r'|\d+)',expr)
  if arithmetic:
   a,b=value(arithmetic[1],seen);c,d=value(arithmetic[3],seen)
   if len(a)*len(c)<=5000:return {x+y if arithmetic[2]=='+' else x-y for x in a for y in c},b|d
  return set(),{expr}
 return value

def grant_sites(text,names):
 for scope,offset,body in scopes(base.strip_comments(text)):
  val=candidates(body,names)
  for call in base.grant_calls(body):
   expression=call['expression'];ids,unknown=val(expression)
   yield {'line':text.count('\n',0,offset)+call['line'],'scope':scope,
          'command':call['command'],'expression':expression,'candidate_ids':sorted(i for i in ids if i>0),
          'unknown_dependencies':sorted(unknown),'dynamic':not (expression.isdigit() or expression.strip('"') in names)}


def analyze(root):
 base.REPO=root;items={};sources={}
 for src,row in base.records('db/item_db.yml'):base.overlay(items.setdefault(row['Id'],{}),row);sources[row['Id']]=src
 names={v['AegisName']:k for k,v in items.items() if 'AegisName' in v};active=set();seen=set()
 def visit(path):
  if path in seen:return
  seen.add(path)
  for typ,target in re.findall(r'^(npc|import):\s*(\S+)',base.strip_comments((root/path).read_text(errors='replace')),re.M):
   if typ=='import':visit(target)
   elif (root/target).exists():active.add(target)
 visit('npc/re/scripts_main.conf');rows=[]
 for src in sorted(active):
  text=base.strip_comments((root/src).read_text(errors='replace'))
  rows.extend({'source':src,**row} for row in grant_sites(text,names))
 return items,active,rows

def main():
 p=argparse.ArgumentParser();p.add_argument('--root',type=Path,default=base.REPO);p.add_argument('--triage',type=Path,required=True);p.add_argument('--report',type=Path,required=True);a=p.parse_args()
 items,active,rows=analyze(a.root);prior=json.loads(a.triage.read_text());flagged={r['item_id']:r for r in prior['items']}
 for r in rows:
  r['flagged_candidates']=[i for i in r['candidate_ids'] if i in flagged]
  r['undefined_candidates']=[i for i in r['candidate_ids'] if i not in items]
 report={'summary':{'active_files':len(active),'grant_sites':len(rows),'dynamic_sites':sum(r['dynamic'] for r in rows),'sites_with_unknown_dependencies':sum(bool(r['unknown_dependencies']) for r in rows),'flagged_candidate_ids':sorted({i for r in rows for i in r['flagged_candidates']})},'grants':rows,'source_sha256':{p:hashlib.sha256((a.root/p).read_bytes()).hexdigest() for p in sorted(active)},'boundary':__doc__}
 a.report.write_text(json.dumps(report,indent=2));print(json.dumps(report['summary']))
if __name__=='__main__':main()
