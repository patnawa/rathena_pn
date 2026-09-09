#!/usr/bin/env python3
"""Native instance Warper routes: prerequisite boundaries and navigation output.

Route blocks and local Go/ValidateWarp/Restrict helpers execute unchanged.
Quest state and character variables are native. Final movement, build mode,
messages, and global access helpers are controlled boundaries.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import tempfile
import zlib

import episode21_encounter_flow_test as native
from episode_party_progression_test import scan_to

ROOT = native.ROOT
SOURCE = ROOT / 'npc/custom/warper.txt'
# Reviewed route inventory. A missing label, menu entry, arrival cell, or
# executable navigation registration fails independently of gate assertions.
ROUTES = {
    39: ('The Immortal','jor_twig',117,145),
    40: ('Episode 17.1 - OS Occupation','sp_cor',162,56),
    41: ('Episode 17.1 - Cor Memorial','sp_cor',113,130),
    42: ('Episode 17.2 - Farm Lost in Time','ba_maison',311,206),
    43: ('Episode 17.2 - Water Garden (Normal / Hard)','ba_maison',238,44),
    44: ('Episode 17.2 - Security Area 1 / 2','ba_maison',120,320),
    45: ('Episode 18 - Sanctuary Purification','rachel',169,245),
    46: ('Episode 18 - Villa of Deception (Normal / Advanced)','wolfvill',79,260),
    47: ('Episode 19 - Iwin Patrol','icecastle',23,115),
    48: ('Episode 19 - Airship Destruction','jor_nest',22,255),
    49: ('Episode 19 - Simulation Battle','jor_nest',66,260),
    50: ('Episode 20 - Sticky Sea','jor_back4',98,258),
    51: ('Chapter 1 - Simulated Dark Whisper','hem_dun01',202,248),
    52: ('Airship Raid','dali02',142,81),
    53: ('Nightmarish Jitterbug','moc_para01',26,95),
    54: ('Bios Island','moro_cav',50,66),
    55: ("Morse's Cave",'moro_cav',61,69),
    56: ('Temple of the Demon God','moro_cav',41,73),
    57: ('Central Laboratory','verus01',149,155),
    58: ('Last Room','un_myst',163,38),
    59: ('Charleston Crisis','verus04',75,114),
    60: ('Sky Fortress Invasion','dali02',122,63),
    61: ('Heart Hunter War Base','ein_fild04',281,337),
    62: ('Werner Laboratory (Daily)','slabw01',236,91),
    63: ('Infinite Space (Normal / Hard)','cmd_fild07',55,278),
    64: ('Weekend Memorial','pay_arche',44,124),
    65: ('Friday Memorial','gef_tower',57,170),
    66: ('Poring Village','prt_fild05',145,235),
    67: ('Wave Mode (Forest / Sky)','prontera',146,75),
    68: ('Chapter 2 - Phantom of Nyrholt','ch2safe4',86,146),
    69: ('Bioresearch Laboratory','yuno',216,343),
    70: ('Alice Twisted Madness','dali',66,100),
}


def scenarios():
    cases=[]
    def add(route, name, allow, level=300, var='', value=0, quests=(), complete=True, access=True, renewal=True):
        cases.append(dict(route=route,name=name,allow=allow,level=level,var=var,value=value,
                          quests=list(quests)+[(0,0)]*(2-len(quests)),complete=complete,access=access,renewal=renewal))
    add(39,'Immortal requires Episode20 completion',False,complete=False)
    add(39,'Immortal completed campaign',True)
    add(39,'Go retains Episode20 map access',False,access=False)
    add(39,'Restrict blocks Pre-Renewal',False,renewal=False)
    for qid in (11603,12452):
        add(40,'OS completed prerequisite '+str(qid),True,quests=[(qid,2)])
        add(40,'OS active prerequisite insufficient '+str(qid),False,quests=[(qid,1)])
    add(40,'OS missing both prerequisites',False)
    add(41,'Cor missing clear',False)
    add(41,'Cor active is not complete',False,quests=[(16360,1)])
    add(41,'Cor completed story',True,quests=[(16360,2)])
    for route,var,threshold,level in [(42,'ep17_2_main',9,150),(43,'ep172_watergarden',3,0),
        (45,'ep18_main',52,170),(46,'ep18_main',57,170),(47,'ep19_main',10,0),
        (48,'ep19_main',33,0),(49,'ep19_main',100,0)]:
        add(route,'milestone before unlock',False,level=level or 300,var=var,value=threshold-1)
        add(route,'milestone unlock',True,level=level or 300,var=var,value=threshold)
        if level: add(route,'level before unlock',False,level=level-1,var=var,value=threshold)
    add(44,'Security complete and level150',True,level=150,var='ep17_2_main',value=1,quests=[(12497,2)])
    add(44,'Security level149',False,level=149,var='ep17_2_main',value=1,quests=[(12497,2)])
    add(44,'Security missing introduction',False,level=150,quests=[(12497,2)])
    add(44,'Security active garden insufficient',False,level=150,var='ep17_2_main',value=1,quests=[(12497,1)])
    add(44,'Security missing garden',False,level=150,var='ep17_2_main',value=1)
    add(50,'Sticky Sea without request',False)
    for qid in (11944,11945,11946,11948,11949,11951):
        add(50,'Sticky Sea active '+str(qid),True,quests=[(qid,1)])
        add(50,'Sticky Sea completed '+str(qid),False,quests=[(qid,2)])
    add(50,'Sticky Sea map access still enforced',False,quests=[(11944,1)],access=False)
    add(51,'Dark Whisper missing invitation',False,quests=[(17899,2)])
    add(51,'Dark Whisper invitation cannot bypass chapter',False,quests=[(12660,1)])
    add(51,'Dark Whisper active chapter insufficient',False,quests=[(12660,1),(17899,1)])
    add(51,'Dark Whisper active invitation and completed chapter',True,quests=[(12660,1),(17899,2)])
    add(51,'Dark Whisper completed invitation retained',True,quests=[(12660,2),(17899,2)])
    for route,level in [(52,125),(53,120),(57,135),(58,150),(59,130),(60,145),(63,100),(64,60),(65,130)]:
        add(route,'classic level before unlock',False,level=level-1)
        add(route,'classic level unlock',True,level=level)
    add(54,'Bios missing request',False,level=160)
    add(54,'Bios level159',False,level=159,quests=[(15006,1)])
    for state in (1,2): add(54,'Bios eligible request',True,level=160,quests=[(15006,state)])
    add(55,'Morse missing introduction',False,level=160)
    add(55,'Morse level159',False,level=159,var='ep14_3_isle',value=1)
    add(55,'Morse eligible',True,level=160,var='ep14_3_isle',value=1)
    add(56,'Demon God missing request',False)
    for state in (1,2): add(56,'Demon God eligible request',True,quests=[(7593,state)])
    for route,step in [(61,19),(62,23)]:
        add(route,'Terra Gloria before unlock',False,var='terra_gloria_main',value=step-1)
        add(route,'Terra Gloria unlock',True,var='terra_gloria_main',value=step)
    for level,allowed in [(29,False),(30,True),(60,True),(61,False)]: add(66,'Poring Village level boundary',allowed,level=level)
    add(67,'Wave Mode public entrance',True,level=1)
    add(68,'Phantom requires Chapter2 completion',False,complete=False)
    add(68,'Phantom completed campaign',True)
    add(69,'Bioresearch level169',False,level=169)
    add(69,'Bioresearch level170',True,level=170)
    add(70,'Alice level174',False,level=174)
    add(70,'Alice level175',True,level=175)
    return cases


def label_body(source, label):
    match=re.search(r'(?m)^\s*'+re.escape(label)+r':',source)
    if not match: raise AssertionError('Missing route label '+label)
    next_label=re.search(r'(?m)^\s*[A-Za-z_][A-Za-z_0-9]*:',source[match.end():])
    return source[match.end():match.end()+next_label.start()] if next_label else source[match.end():]


def fixtures(build, pre_fix=False):
    source=SOURCE.read_text()
    helpers=[]
    for name in ('Go','ValidateWarp','Restrict'):
        match=re.search(r'function '+name+r'\s*\{',source); begin=match.end()-1
        helpers.append(source[match.start():scan_to(source,begin,'{','}')+1])
    # Native menu treats ':' as another option; labels must remain one option.
    menus=re.findall(r'\bmenu\s+(.*?);',source,re.S)
    for menu in menus:
        labels=re.findall(r'"([^"\n]*)"\s*,\s*I(?:\d+|_Episodes|_Classic)\b',menu)
        if labels:
            assert all(':' not in title for title in labels), ('split instance menu',labels)
            assert len(':'.join(labels).encode())<2048, 'instance menu exceeds native buffer'
    entries=re.findall(r'"([^"\n]*)"\s*,\s*(I\d+)\b',source)
    aliases=re.findall(r'naviregisterwarp\("Warper > ([^"]+)",\s*"([^"]+)",\s*(\d+),\s*(\d+)\)',label_body(source,'OnNaviGenerate'))
    alias_count=0
    for title,label in entries:
        arrival=re.search(r'Go\("([^"]+)",\s*(\d+),\s*(\d+)\)',label_body(source,label))
        assert arrival, ('missing literal instance destination',label)
        matches=[(m,x,y) for t,m,x,y in aliases if t==title]
        if matches:
            assert len(matches)==1 and matches[0]==arrival.groups(), ('stale or duplicate instance navigation',title,matches,arrival.groups())
            alias_count+=1
    cells={}
    for relative in ('db/import/map_cache.dat','db/re/map_cache.dat','db/map_cache.dat'):
        path=ROOT/relative
        if not path.exists(): continue
        data=path.read_bytes(); offset=8
        for _ in range(struct.unpack_from('<H',data,4)[0]):
            name,w,h,size=struct.unpack_from('<12shhi',data,offset); offset+=20
            name=name.split(b'\0')[0].decode(); cells.setdefault(name,(w,h,zlib.decompress(data[offset:offset+size]))); offset+=size
    header=['static Case source_cases[] = {']; digest=hashlib.sha256()
    for route,(title,mapname,x,y) in ROUTES.items():
        assert re.search(re.escape(json.dumps(title))+r'\s*,\s*I'+str(route)+r'\b',source), ('missing menu entry',route)
        w,h,data=cells[mapname]
        assert 0<=x<w and 0<=y<h and data[y*w+x] in (0,3,6), ('blocked arrival',route,mapname,x,y)
        block=label_body(source,'I'+str(route))
        assert re.search(r'Go\("'+re.escape(mapname)+r'",\s*'+str(x)+r',\s*'+str(y)+r'\)',block), ('wrong destination',route)
        body='{\nfunction Go; function ValidateWarp; function Restrict;\n'+block+'\nend;\n'+'\n'.join(helpers)+'\n}'
        filename=f'warper_{route}.script'; (build/filename).write_text(body); digest.update(body.encode())
        header.append('{"I'+str(route)+'","'+filename+'","",0,false},')
    for mapname in ('bl_ice','bl_depth1','bl_depth2','ch1fild1','ch1zero1','ch1zero3','jor_mbase','jor_base'):
        body='{\nfunction Go; function ValidateWarp; function Restrict;\nGo("'+mapname+'",1,1); end;\n'+'\n'.join(helpers)+'\n}'
        filename='region_'+mapname+'.script'; (build/filename).write_text(body); digest.update(body.encode())
        header.append('{"region_'+mapname+'","'+filename+'","",0,false},')
    body='{\n'+label_body(source,'OnNaviGenerate')+'\n}'
    (build/'navigation.script').write_text(body); digest.update(body.encode())
    header.append('{"OnNaviGenerate","navigation.script","",0,false},\n};')
    case_rows=[]; qids=set()
    for c in scenarios():
        q1,q2=c['quests']; qids.update(qid for qid,_ in (q1,q2) if qid)
        title,mapname,x,y=ROUTES[c['route']]
        values=[c['name'],'I'+str(c['route']),c['allow'],c['level'],c['var'],c['value'],*q1,*q2,c['complete'],c['access'],c['renewal'],mapname,x,y]
        case_rows.append('{'+','.join(json.dumps(v) for v in values)+'},')
    header.append('static int quest_ids[] = {'+','.join(str(q) for q in sorted(qids))+'};')
    header.append('struct RouteCase { const char* title; const char* route; bool allow; int level; const char* var; int value; int q1,s1,q2,s2; bool complete,access,renewal; const char* map; int x,y; };')
    header.append('static RouteCase route_cases[] = {\n'+'\n'.join(case_rows)+'\n};')
    text='\n'.join(header); (build/'episode_cases.inc').write_text(text); digest.update(text.encode())
    print(json.dumps({'reviewed_routes':len(ROUTES),'named_aliases_checked':alias_count,'gate_scenarios':len(case_rows),'walkable_destinations':len(ROUTES),'source_sha256':hashlib.sha256(source.encode()).hexdigest()}),flush=True)
    return digest.hexdigest()


EXTRA=r'''
bool campaign_complete=true,map_access=true,renewal=true;
std::vector<Move> navigation;
unsigned unrelated_helpers=0, regional_calls=0;
bool regional_mode=false;
std::string active_route;
std::map<int32,std::map<std::string,std::string>> string_regs;
extern "C" char* warp_read(const map_session_data*,int64) asm("__wrap__Z19pc_readregistry_strPK16map_session_datal");
extern "C" char* warp_read(const map_session_data* sd,int64 id) { return string_regs[sd->id][get_str((int32)id)].data(); }
extern "C" bool warp_set(map_session_data*,int64,const char*) asm("__wrap__Z18pc_setregistry_strP16map_session_datalPKc");
extern "C" bool warp_set(map_session_data* sd,int64 id,const char* value) { string_regs[sd->id][get_str((int32)id)]=value; return true; }
'''
MAIN=r'''
extern "C" int __wrap_main(int argc,char** argv) {
    boundary(argc==2,"fixture directory supplied"); fixture_dir=argv[1];
    deny_network(); static char name[]="instance-warper-test"; SERVER_NAME=name;
    malloc_init(); db_init(); do_init_database(); timer_init(); install_world_doubles(); do_init_script();
    save_settings=0; battle_config.atcommand_disable_npc=0;
    npc.id=NPC; npc.type=BL_NPC; npc.instance_id=1;
    instances[1]=std::make_shared<s_instance_data>(); instances.at(1)->state=INSTANCE_BUSY;
    instances.at(1)->regs.vars=i64db_alloc(DB_OPT_RELEASE_DATA);
    auto p=std::make_unique<map_session_data>(); p->id=99000010; p->type=BL_PC;
    p->status.account_id=p->id; p->status.char_id=p->id;
    p->fd=0; p->state.ignoretimeout=true; p->npc_idle_timer=INVALID_TIMER; players.emplace_back(std::move(p));
    for (int id:quest_ids) { auto entry=std::make_shared<s_quest_db>(); entry->id=id; quest_db.put(id,entry); }
    for (const auto& test:source_cases) {
        std::ifstream f(fixture_dir+"/"+test.path); const std::string source{std::istreambuf_iterator<char>(f),std::istreambuf_iterator<char>()};
        auto* code=parse_script(source.c_str(),test.name,1,0); boundary(code!=nullptr,"actual route and helpers parse"); codes[test.name]=code;
    }
    for (const auto& c:route_cases) {
        reset(); string_regs.clear(); unrelated_helpers=0; active_route=c.route; current=c.title; campaign_complete=c.complete; map_access=c.access; renewal=c.renewal;
        players[0]->status.base_level=c.level; if (*c.var) registries[players[0]->id][c.var]=c.value;
        if (c.q1) seed_quest(0,c.q1,c.s1==2); if (c.q2) seed_quest(0,c.q2,c.s2==2);
        string_regs[players[0]->id]["lastwarp$"]="old_map"; registries[players[0]->id]["lastwarpx"]=11; registries[players[0]->id]["lastwarpy"]=12;
        finish(c.route);
        check(unrelated_helpers==0,"instance route avoids unrelated access helper side effects");
        check(moves.size()==(c.allow ? 1u:0u),c.title);
        if (c.allow && moves.size()==1) {
            check(moves[0].map==c.map && moves[0].x==c.x && moves[0].y==c.y,"arrival matches reviewed entrance");
            check(string_regs[players[0]->id]["lastwarp$"]==c.map && registries[players[0]->id]["lastwarpx"]==c.x && registries[players[0]->id]["lastwarpy"]==c.y,"successful route records last warp");
        } else if (!c.allow) check(string_regs[players[0]->id]["lastwarp$"]=="old_map" && registries[players[0]->id]["lastwarpx"]==11 && registries[players[0]->id]["lastwarpy"]==12,"denied route preserves previous warp");
    }
    regional_mode=true;
    for (const char* region:{"bl_ice","bl_depth1","bl_depth2","ch1fild1","ch1zero1","ch1zero3","jor_mbase","jor_base"}) for (bool permit:{false,true}) {
        reset(); regional_calls=0; map_access=permit; players[0]->status.base_level=300;
        finish(std::string("region_")+region);
        check(regional_calls==1,"matching restricted region invokes exactly its access helper");
        check(moves.size()==(permit ? 1u:0u),"matching region honors helper denial and allowance");
    }
    regional_mode=false;
    reset(); renewal=true; navigation.clear(); finish("OnNaviGenerate");
    for (const auto& c:route_cases) if (c.allow) {
        bool found=false; for (const auto& n:navigation) if (n.map==c.map && n.x==c.x && n.y==c.y) found=true;
        check(found,"reviewed instance destination registered by executable navigation callback");
    }
    check(errors==0,"no native runtime or parser errors");
    for (auto& code:codes) script_free_code(code.second); codes.clear(); reset();
    script_free_vars(instances.at(1)->regs.vars); instances.clear(); quest_db.clear(); players.clear(); attached=nullptr;
    do_final_script(); timer_final(); db_final();
    std::printf("INSTANCE_WARPER_RESULT checks=%u failures=%u errors=%u\n",checks,failures,errors);
    malloc_final(); return failures||errors ? 1:0;
}
'''


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir',type=Path); parser.add_argument('--prepare-only',action='store_true'); args=parser.parse_args()
    prefix=native.CPP.split('extern "C" int __wrap_main',1)[0]
    prefix=prefix.replace('int32 world(script_state* st) {',EXTRA+'\nint32 world(script_state* st) {')
    prefix=prefix.replace('    if (command == "instance_mapname"','''    if (command == "checkre") script_pushint(st,renewal);
    else if (command == "message") { }
    else if (command == "strnpcinfo" && script_getnum(st,2)==4) script_pushstrcopy(st,"prontera");
    else if (command == "naviregisterwarp") navigation.push_back({st->rid,script_getstr(st,3),script_getnum(st,4),script_getnum(st,5)});
    else if (command == "instance_mapname"''',1)
    prefix=prefix.replace('if (function == "EP21_GhostShipUnlocked")','''if (function == "EP20_MainComplete") { if (active_route!="I39") ++unrelated_helpers; script_pushint(st,campaign_complete); }
        else if (function == "EP20_WarperAccess") { if (active_route!="I39" && active_route!="I50") ++unrelated_helpers; script_pushint(st,map_access); }
        else if (function == "CH2_Complete") { if (active_route!="I68") ++unrelated_helpers; script_pushint(st,campaign_complete); }
        else if (function == "F_BiosphereAccess" || function == "F_BiosphereDepth1Access" || function == "F_BiosphereDepth2Access" || function == "EP21_WarperAccess" || function == "EP21_GaebolgComplete" || function == "ZC_HasAccess" || function == "ZC_CanEnter") { if (regional_mode) ++regional_calls; else ++unrelated_helpers; script_pushint(st,regional_mode ? map_access:1); }
        else if (function == "F_BiosphereDepth2Purge") { if (!regional_mode) ++unrelated_helpers; }
        else if (function == "EP21_GhostShipUnlocked")''',1)
    prefix=prefix.replace('else boundary(false, "unrecognized callfunc boundary");', 'else { std::fprintf(stderr, "UNKNOWN FUNCTION %s case %s\\n",function.c_str(),current.c_str()); boundary(false, "unrecognized callfunc boundary"); }')
    prefix=prefix.replace('"getexp", "callfunc"};','"getexp", "callfunc", "message", "checkre", "naviregisterwarp"};',1)
    native.CPP=prefix+MAIN; native.fixtures=fixtures
    native.WRAPPERS+=('_Z19pc_readregistry_strPK16map_session_datal','_Z18pc_setregistry_strP16map_session_datalPKc')
    def run(directory):
        directory.mkdir(parents=True,exist_ok=True); native.run(directory.resolve(),False,False,args.prepare_only,completion_marker='INSTANCE_WARPER_RESULT ')
    if args.build_dir: run(args.build_dir)
    else:
        with tempfile.TemporaryDirectory(prefix='instance-warper-') as temp: run(Path(temp))


if __name__=='__main__': main()
