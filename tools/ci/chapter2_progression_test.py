#!/usr/bin/env python3
# ============================================================================
#  PN  /  DEVELOPMENT TOOLS
#  chapter2_progression_test.py
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/chapter2_progression_test.py
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

"""Execute selected actual Chapter 2 source with explicit world/quest doubles.

This is a fail-closed source interpreter, not the native rAthena VM. It exercises
reward eligibility/commit ordering and the Phantom timer state machine. Full
engine/client combat, packet delivery, SQL/mail and map geometry remain separate.
"""
import ast
from collections import defaultdict
from pathlib import Path
import re
import unittest

from episode_party_progression_test import scan_to, npc_body

ROOT = Path(__file__).resolve().parents[2]
STORY = ROOT/'npc/custom/chapter2/Chapter2.txt'
INSTANCES = ROOT/'npc/custom/chapter2/Instances.txt'


def function_body(name):
    for path in (STORY, INSTANCES):
        source = path.read_text()
        match = re.search(r'function\s+script\s+'+name+r'\s*\{',source)
        if match:
            begin=match.end()-1
            return re.sub(r'//[^\n]*','',source[begin+1:scan_to(source,begin,'{','}')])
    raise AssertionError(name)


def parse(source):
    """Only known script control structures are accepted; no ignored commands."""
    pos=0
    def whitespace():
        nonlocal pos
        while pos<len(source) and source[pos].isspace(): pos+=1
    def one():
        nonlocal pos
        whitespace()
        if source[pos]=='{':
            end=scan_to(source,pos,'{','}'); body=parse(source[pos+1:end]);pos=end+1;return ('block',body)
        for keyword in ('if','switch','for'):
            match=re.match(keyword+r'\s*\(',source[pos:])
            if not match: continue
            start=pos+match.end()-1;end=scan_to(source,start,'(',')');expr=source[start+1:end];pos=end+1
            if keyword=='switch':
                whitespace();end=scan_to(source,pos,'{','}');body=source[pos+1:end];pos=end+1
                branches={}
                for case,branch in re.findall(r'case\s+(\d+)\s*:(.*?)(?=case\s+\d+\s*:|\Z)',body,re.S): branches[int(case)]=parse(branch)
                return ('switch',expr,branches)
            body=one()
            if keyword=='for':return ('for',expr,body)
            whitespace();other=None
            if source[pos:pos+4]=='else':pos+=4;other=one()
            return ('if',expr,body,other)
        match=re.match(r'(?:"(?:\\.|[^"\\])*"|[^;"{}])*;',source[pos:])
        if not match:raise AssertionError('Unsupported source: '+source[pos:pos+80])
        pos+=match.end();return ('command',match.group()[:-1].strip())
    result=[]
    while True:
        whitespace()
        if pos==len(source):return result
        result.append(one())


class Return(Exception):
    def __init__(self,value=0):self.value=value
class Closed(Exception):pass
class Break(Exception):pass
class Continue(Exception):pass
class S(str):
    def __add__(self,other):return S(str(self)+str(other))
    def __radd__(self,other):return S(str(other)+str(self))


class Run:
    def __init__(self,shared=None,cid=11,party=1):
        self.local=defaultdict(int);self.vars=defaultdict(int)
        self.shared=shared if shared is not None else defaultdict(int)
        self.quests=defaultdict(int);self.items=defaultdict(int);self.inventory=[]
        self.cid,self.party,self.args=cid,party,()
        self.capacity=True;self.now=100000;self.exp=[];self.mail=[];self.messages=[]
        self.unit={};self.actions=[];self.selections=[];self.on_next=None
    def get(self,name,index=None):
        store=self.local if name.startswith('.@') else self.shared if name.startswith("'") else self.vars
        if index is not None:return store.get((name,index),0)
        return store.get(name,S('') if name.endswith('$') else 0)
    def put(self,name,value,index=None):
        store=self.local if name.startswith('.@') else self.shared if name.startswith("'") else self.vars
        store[(name,index) if index is not None else name]=value
    def value(self,expr):
        expr=expr.replace('CH2_Complete()', 'callfunc("CH2_Complete")')
        expr=re.sub(r"getinstancevar\(('\w+\$?),",lambda m:'getinstancevar("'+m[1]+'",',expr)
        parts=re.split(r'("(?:\\.|[^"\\])*")',expr)
        for i in range(0,len(parts),2):
            parts[i]=re.sub(r"(?:\.@|'|CH2_)[A-Za-z_][A-Za-z_0-9]*\$?(?:\[[^\]]+\])?",lambda m:self.translate_var(m[0]),parts[i])
            parts[i]=parts[i].replace('||',' or ').replace('&&',' and ')
            parts[i]=re.sub(r'!(?!=)',' not ',parts[i])
        transformed=''.join(parts).strip()
        funcs={'v':self.get,'S':S,'getarg':lambda i,default=0:self.args[i] if i<len(self.args) else default,
            'getcharid':lambda kind:{0:self.cid,1:self.party,3:self.cid+100}[kind],
            'getinstancevar':lambda var,iid:self.get(var),'compare':lambda hay,needle:str(needle) in str(hay),
            'callfunc':self.call,'isbegin_quest':lambda q:self.quests[q],'checkweight':lambda *a:self.capacity,
            'gettimetick':lambda _:self.now,'getd':lambda k:self.get(k),'countitem':lambda i:self.items[i],
            'select':self.select,'unitexists':lambda gid:gid in self.unit,
            'distance':lambda x,y,a,b:max(abs(x-a),abs(y-b)),
            'instance_id':lambda:1,'instance_npcname':lambda s:s,
            'min':min,'max':max,
        }
        constants=['UMOB_X','UMOB_Y','UMOB_MODE','UMOB_DAMAGETAKEN','SC_GROGGY','SCSTART_NOAVOID','SCSTART_NOTICKDEF','SCSTART_NORATEDEF','MD_NOCAST','MD_CANMOVE','MD_CANATTACK','MD_AGGRESSIVE','bc_map']
        funcs.update({name:1<<n for n,name in enumerate(constants)})
        tree=ast.parse(transformed,mode='eval')
        class Strings(ast.NodeTransformer):
            def visit_Constant(self,node):
                return ast.copy_location(ast.Call(ast.Name('S',ast.Load()),[node],[]),node) if isinstance(node.value,str) else node
        tree=ast.fix_missing_locations(Strings().visit(tree))
        allowed=(ast.Expression,ast.Constant,ast.Name,ast.Load,ast.Call,ast.Tuple,ast.BoolOp,ast.Or,ast.And,ast.UnaryOp,ast.Not,ast.Invert,ast.USub,ast.Compare,ast.Eq,ast.NotEq,ast.Lt,ast.Gt,ast.LtE,ast.GtE,ast.BinOp,ast.Add,ast.Sub,ast.BitAnd,ast.BitOr)
        for node in ast.walk(tree):
            if not isinstance(node,allowed):raise AssertionError(type(node).__name__)
            if isinstance(node,ast.Name) and node.id not in funcs:raise AssertionError(node.id)
        return eval(compile(tree,'<Chapter2 source>','eval'),{'__builtins__':{}},funcs)
    def translate_var(self,name):
        if '[' in name:
            base,index=name.split('[',1)
            return 'v('+repr(base)+','+re.sub(r"(?:\.@|')\w+",lambda m:'v('+repr(m[0])+')',index[:-1])+')'
        return 'v('+repr(name)+')'
    def select(self,menu):
        self.messages.append(menu)
        if not self.selections:raise AssertionError('Missing menu answer')
        return self.selections.pop(0)
    def call(self,name,*args):
        oldlocal,oldargs=self.local,self.args;self.local=defaultdict(int);self.args=args
        try:
            self.execute(parse(function_body(name)))
        except Return as result:return result.value
        finally:self.local,self.args=oldlocal,oldargs
        return 0
    def execute(self,code):
        for node in code:
            kind=node[0]
            if kind=='block':self.execute(node[1])
            elif kind=='if':
                branch=node[2] if self.value(node[1]) else node[3]
                if branch:self.execute([branch])
            elif kind=='switch':
                try:self.execute(node[2].get(self.value(node[1]),[]))
                except Break:pass
            elif kind=='for':
                init,condition,step=node[1].split(';');self.command(init.strip());count=0
                while self.value(condition):
                    count+=1
                    if count>100:raise AssertionError('Unbounded loop')
                    try:self.execute([node[2]])
                    except Continue:pass
                    except Break:break
                    self.command(step.strip())
            else:self.command(node[1])
    def command(self,cmd):
        if cmd.startswith('setd('):cmd='setd '+cmd[5:-1]
        if cmd in ('close','end'):raise Closed
        if cmd=='break':raise Break
        if cmd=='continue':raise Continue
        if cmd=='next':
            if self.on_next:self.on_next(self)
            return
        assign=re.fullmatch(r"((?:\.@|'|CH2_)\w+\$?)(?:\[([^]]+)\])?\s*(=|\+=)\s*(.*)",cmd)
        if assign:
            name,index,op,value=assign.groups();index=self.value(index) if index else None
            self.put(name,self.value(value)+(self.get(name,index) if op=='+=' else 0),index);return
        change=re.fullmatch(r"(\+\+|--)((?:\.@|')\w+)",cmd)
        if change:self.put(change[2],self.get(change[2])+(1 if change[1]=='++' else -1));return
        name,_,args=cmd.partition(' ')
        if name=='return':raise Return(self.value(args) if args else 0)
        if name in ('mes','dispbottom'):
            self.messages.append(self.value(args));return
        if name=='setinstancevar':
            var,rest=args.split(',',1);values=self.value('('+rest+',)');self.put(var,values[0]);return
        if name=='setarray':
            target,rest=args.split(',',1);base,start=target.split('[');values=self.value('('+rest+',)')
            for i,value in enumerate(values):self.put(base,value,int(start[:-1])+i)
            return
        if name=='getunitdata':
            gid,target=args.split(',');data=self.unit[self.value(gid)]
            for field,value in data.items():self.put(target.strip(),value,self.value(field))
            return
        if name in ('initnpctimer','stopnpctimer'):
            self.actions.append((name,));return
        values=self.value('('+args+',)')
        if name=='callfunc':self.call(*values)
        elif name=='setd':self.put(*values)
        elif name=='completequest':self.quests[values[0]]=2
        elif name=='setquest':self.quests[values[0]]=1
        elif name=='erasequest':self.quests.pop(values[0],None)
        elif name=='getitem':self.items[values[0]]+=values[1]
        elif name=='getexp':self.exp.append(values)
        elif name=='setunitdata':
            gid,field,value=values
            self.unit[gid][{self.value(n):n for n in ('UMOB_MODE','UMOB_DAMAGETAKEN')}[field]]=value
        elif name in ('sc_start','sc_end','unitwarp','setnpctimer','instance_announce'):self.actions.append((name,)+values)
        elif name=='close2':pass
        else:raise AssertionError('Unsupported command '+cmd)
    def run(self,source):
        try:self.execute(parse(source))
        except Closed:pass


class Chapter2Regression(unittest.TestCase):
    def member(self,shared=None,cid=11):
        r=Run(shared,cid);r.shared.update({"'ch2_party_id":1,"'ch2_roster$":S(',11,12,112,')})
        return r
    def test_reward_capacity_preserves_daily_and_retry_commits_once(self):
        r=self.member();r.quests[27119]=1;r.vars['CH2_DailyPhantom']=1;r.capacity=False
        for _ in range(2):
            try:r.call('CH2_GiveReward',27119,3)
            except Closed:pass
        self.assertEqual(r.quests[27119],1);self.assertEqual(r.vars['CH2_DailyPhantom'],1)
        self.assertFalse(r.items);self.assertFalse(r.exp)
        r.capacity=True
        for _ in range(2):
            try:r.call('CH2_GiveReward',27119,3)
            except Closed:pass
        self.assertEqual(r.items[1002700],3);self.assertEqual(r.quests[27119],2)
        self.assertEqual(r.vars['CH2_DailyPhantom'],0);self.assertEqual(r.vars['CH2_Reputation'],2)
        self.assertEqual(r.vars['CH2_Daily_CD_27119'],r.now+14400)
        self.assertEqual(r.exp,[(12000000,8400000)])
    def test_actual_gate_full_inventory_does_not_erase_credit(self):
        r=self.member();r.quests.update({27101:2,27119:1});r.vars['CH2_DailyPhantom']=1
        r.capacity=False;r.selections=[1]
        r.run(npc_body(STORY,'Phantom Gate#ch2main'))
        self.assertEqual(r.vars['CH2_DailyPhantom'],1);self.assertEqual(r.quests[27119],1)
        self.assertFalse(r.items)
    def test_final_story_capacity_revalidated_after_dialog(self):
        r=self.member();r.vars.update({'CH2_Step':12,'CH2_Phantom':1});r.quests[27101]=1
        r.on_next=lambda state:setattr(state,'capacity',False)
        r.run(npc_body(STORY,'Guardian El#ch2main'))
        self.assertEqual(r.quests[27101],1);self.assertFalse(r.items);self.assertFalse(r.exp)
        r.on_next=None;r.capacity=True;r.run(npc_body(STORY,'Guardian El#ch2main'))
        self.assertEqual(r.quests[27101],2);self.assertEqual(r.items[1002700],10)
        r.run(npc_body(STORY,'Guardian El#ch2main'));self.assertEqual(r.items[1002700],10)
    def test_story_clear_cannot_be_reused_for_daily_same_run(self):
        r=self.member();r.vars['CH2_Step']=11
        self.assertEqual(r.call('CH2_ClaimInstanceClear',1,3),1)
        self.assertEqual(r.items[1002700],3)
        r.quests.update({27101:2,27119:1});r.vars['CH2_Step']=12
        self.assertEqual(r.call('CH2_ClaimInstanceClear',1,3),0)
        self.assertEqual(r.vars['CH2_DailyPhantom'],0)
        # A different original member returning during grace gets their own credit.
        other=self.member(r.shared,12);other.vars['CH2_Step']=11
        self.assertEqual(other.call('CH2_ClaimInstanceClear',1,3),1)
        self.assertEqual(other.items[1002700],3)
    def test_roster_delimiters_party_and_stage_rejection(self):
        for cid,party,step in ((1,1,11),(11,2,11),(11,1,10)):
            r=self.member(cid=cid);r.party=party;r.vars['CH2_Step']=step
            self.assertEqual(r.call('CH2_ClaimInstanceClear',1,3),0)
            self.assertFalse(r.get("'ch2_claimed$"));self.assertFalse(r.items)
    def test_story_modes_and_original_member_reentry_once(self):
        for mode,step,flag,coins in ((1,2,'CH2_Crossroads',3),(2,8,'CH2_Nyrholt',4),(2,9,'CH2_Nyrholt',4)):
            r=self.member();r.vars['CH2_Step']=step
            for attempt in range(2):self.assertEqual(r.call('CH2_ClaimInstanceClear',1,mode),1 if attempt==0 else 0)
            self.assertEqual(r.vars[flag],1);self.assertEqual(r.items[1002700],coins)
    def test_phantom_ten_ticks_and_circle_rearm(self):
        body=npc_body(INSTANCES,'#ch2_nyrholt')
        timer=body.split('OnTimer1000:',1)[1].split('OnTimer84000:',1)[0]
        r=self.member();r.shared.update({"'ch2_stage":2,"'ch2_mode":2,"'ch2_boss_gid":99,"'ch2_boss_mode":65535,"'ch2_map$":S('arena')})
        r.unit[99]={'UMOB_X':78,'UMOB_Y':141,'UMOB_MODE':65535,'UMOB_DAMAGETAKEN':1}
        r.run(timer)
        self.assertEqual(r.get("'ch2_groggy_ticks"),10)
        self.assertEqual(r.unit[99]['UMOB_DAMAGETAKEN'],100)
        status=[a for a in r.actions if a[0]=='sc_start'];self.assertEqual(len(status),1)
        self.assertEqual(status[0][2],10000);self.assertEqual(status[0][-1],99)
        for _ in range(9):r.run(timer)
        self.assertEqual(r.get("'ch2_groggy_ticks"),1);self.assertEqual(r.unit[99]['UMOB_DAMAGETAKEN'],100)
        r.run(timer)
        self.assertEqual(r.get("'ch2_groggy_ticks"),0);self.assertEqual(r.unit[99]['UMOB_DAMAGETAKEN'],1)
        self.assertEqual(r.unit[99]['UMOB_MODE'],65535)
        self.assertIn(('unitwarp',99,'arena',86,157),r.actions)
        for stage in (1,3):
            r.shared["'ch2_stage"]=stage;old=list(r.actions);r.run(timer);self.assertEqual(r.actions,old)
        r.shared["'ch2_stage"]=2;r.unit.clear();old=list(r.actions);r.run(timer);self.assertEqual(r.actions,old)
    def test_menu_maps_ten_missions_and_cancel(self):
        body=npc_body(STORY,'Flame Branch Mission Board')
        menu=re.search(r'\.@pick = select\("([^"]+)"\)',body)[1].split(':')
        self.assertEqual(len(menu),11);self.assertEqual(menu[9:],["Phantom of Nyrholt","Cancel"])
        prefix=body.split('if (isbegin_quest(.@qid)',1)[0]
        for pick in range(1,12):
            r=self.member();r.quests[27101]=2;r.selections=[pick];r.run(prefix)
            self.assertEqual(r.local['.@qid'],27109+pick if pick<11 else 0)


if __name__=='__main__':unittest.main()
