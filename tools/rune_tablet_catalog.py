#!/usr/bin/env python3
"""Non-executing readers for rune client tables and published reference tables."""
from html.parser import HTMLParser
import json
from pathlib import Path
import re

class LuaData:
    def __init__(self,text,env=None):
        self.env={} if env is None else env
        token=re.compile(r'\s+|--[^\n]*|"(?:\\.|[^"\\])*"|-?\d+(?:\.\d+)?|[A-Za-z_][A-Za-z_0-9]*|[{}\[\]=,.]')
        self.tokens=[];pos=0
        while pos<len(text):
            match=token.match(text,pos)
            if not match:raise ValueError(f'Unsupported Lua syntax at {pos}: {text[pos:pos+50]!r}')
            value=match[0];pos=match.end()
            if not value.isspace() and not value.startswith('--'):self.tokens.append(value)
        self.i=0
    def take(self,want=None):
        t=self.tokens[self.i];self.i+=1
        if want is not None and t!=want:raise ValueError((want,t))
        return t
    def value(self):
        t=self.take()
        if t=='{':
            table={};seq=1
            while self.tokens[self.i]!='}':
                if self.tokens[self.i]=='[':
                    self.take('[');key=self.value();self.take(']');self.take('=');value=self.value()
                elif self.i+1<len(self.tokens) and self.tokens[self.i+1]=='=':
                    key=self.take();self.take('=');value=self.value()
                else:key=seq;seq+=1;value=self.value()
                if key in table:raise ValueError('Duplicate Lua key')
                table[key]=value
                if self.tokens[self.i]==',':self.take(',')
            self.take('}');return table
        if t.startswith('"'):return json.loads(t)
        if re.fullmatch(r'-?\d+(?:\.\d+)?',t):return float(t) if '.' in t else int(t)
        if t in ('true','false','nil'):return {'true':True,'false':False,'nil':None}[t]
        v=self.env[t]
        while self.i<len(self.tokens) and self.tokens[self.i]=='.':self.take('.');v=v[self.take()]
        return v
    def read(self):
        while self.i<len(self.tokens):
            key=self.take();self.take('=');self.env[key]=self.value()
        return self.env

class Node:
    def __init__(self,tag='',attrs=()):self.tag=tag;self.attrs=dict(attrs);self.children=[]
    def find(self,tag):
        for child in self.children:
            if isinstance(child,Node):
                if child.tag==tag:yield child
                yield from child.find(tag)
    def text(self):
        if self.tag=='br':return '\n'
        return ''.join(c.text() if isinstance(c,Node) else c for c in self.children)

class Tree(HTMLParser):
    def __init__(self):super().__init__(convert_charrefs=True);self.root=Node();self.stack=[self.root]
    def handle_starttag(self,tag,attrs):
        n=Node(tag,attrs);self.stack[-1].children.append(n)
        if tag not in ('area','base','br','col','embed','hr','img','input','link','meta','param','source','track','wbr'):self.stack.append(n)
    def handle_endtag(self,tag):
        for i in range(len(self.stack)-1,0,-1):
            if self.stack[i].tag==tag:self.stack=self.stack[:i];return
    def handle_data(self,data):self.stack[-1].children.append(data)

def read_reference(path):
    tree=Tree();tree.feed(Path(path).read_text())
    result=[]
    for table in tree.root.find('table'):
        rows=[]
        for row in table.find('tr'):
            cells=[c for c in row.children if isinstance(c,Node) and c.tag in ('td','th')]
            if cells:rows.append([' '.join(c.text().split()) for c in cells])
        if rows:result.append(rows)
    return result

def read_client(folder):
    env={}
    for name in ['runesystemid','runeset_info','rune_info','runeSystem_table','runeset_reward','itemDecom']:
        text=(Path(folder)/(name+'.lua')).read_text(encoding='utf-8')
        # Reward loader functions are not data and are never evaluated.
        text=re.split(r'^main_runeset_reward\s*=\s*function\(',text,flags=re.M)[0]
        LuaData(text,env).read()
    return env

if __name__=='__main__':
    import argparse
    p=argparse.ArgumentParser();p.add_argument('client');p.add_argument('reference');p.add_argument('output')
    a=p.parse_args();out=Path(a.output);out.mkdir(exist_ok=False)
    client=read_client(a.client);tables=read_reference(a.reference)
    (out/'client.json').write_text(json.dumps(client,indent=2))
    (out/'reference-tables.json').write_text(json.dumps(tables,indent=2))
    for i,t in enumerate(tables):print(i,len(t),t[0][:4])
