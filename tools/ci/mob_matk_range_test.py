#!/usr/bin/env python3
"""Native source-extracted monster parser/MATK overflow regressions.

Uses the real RapidYAML library and production Attack2 parse block plus Renewal
base-MATK functions. Database diagnostics and unrelated status fields are doubles.
Requires Linux GCC and the configured map build's RapidYAML static library.
"""
import argparse
import hashlib
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def block(source, signature):
    start = source.index(signature)
    opening = source.index('{', start)
    depth, end = 1, opening+1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


def fixture(pre_fix):
    mob = (ROOT/'src/map/mob.cpp').read_text()
    status = (ROOT/'src/map/status.cpp').read_text()
    attack = block(mob, 'if (this->nodeExists(node, "Attack2"))')
    minimum = block(status, 'uint16 status_base_matk_min( const block_list*')
    maximum = block(status, 'uint16 status_base_matk_max( const block_list*')
    if pre_fix:
        attack = re.sub(r'\t\t// Decode before narrowing:.*?\n\t\tuint64 atk;', '\t\tuint16 atk;', attack, flags=re.S)
        attack = attack.replace('asUInt64', 'asUInt16')
        start = attack.index('\n\t\tif (atk > USHRT_MAX)')
        end = attack.index('\n#ifdef RENEWAL', start)
        attack = attack[:start]+attack[end:]
        attack = attack.replace('static_cast<uint16>(atk)', 'atk')
        minimum = minimum.replace('\t\t\t// Preserve the uint16 combat limit without modulo wrap at high MATK.\n', '')
        for rate in (70,130):
            old = f'cap_value(static_cast<int64>(status->int_) + level + status->rhw.matk * {rate} / 100, 0, USHRT_MAX)'
            new = f'status->int_ + level + status->rhw.matk * {rate} / 100'
            minimum = minimum.replace(old,new); maximum = maximum.replace(old,new)
        expected = (
            '852c5913a5ae52cd4d872453d3c831a1118a3143dd1bf1b8df8daabfbff31f62',
            '352d61c734a6709a51fcc920c8c05e73ee1644896e157dff0421dd22de3b1dbc',
            '2dd50e524936ebdc19ad8942992d62ba7b5163417298e2a7b9ad08eed7a647fd',
        )
        if tuple(hashlib.sha256(s.encode()).hexdigest() for s in (attack,minimum,maximum)) != expected:
            raise AssertionError('Negative fixture no longer matches baseline b8f6d158a95c729df5febc9063dfd745750b0617')
    weapon = block((ROOT/'src/map/status.hpp').read_text(), 'struct weapon_atk {')+';'
    prefix = r'''
#include <algorithm>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>
#include <ryml_std.hpp>
#include <ryml.hpp>
using uint16=uint16_t; using uint32=uint32_t; using uint64=uint64_t; using int16=int16_t; using int32=int32_t; using int64=int64_t;
template<class T,class A,class B> T cap_value(T v,A low,B high) { return std::max<T>(low,std::min<T>(high,v)); }
unsigned checks=0,failures=0;
void check(bool good,const char* text) { ++checks; if(!good) { if(failures<20) std::fprintf(stderr,"MATK FAIL: %s\n",text); ++failures; } }
'''
    middle = r'''
struct status_data { weapon_atk rhw{}; int16 int_=0,dex=0,luk=0,spl=0; };
enum {BL_PC,BL_PET,BL_MOB,BL_MER,BL_ELEM,BL_HOM};
struct block_list {int type;};
int status_get_homint(const block_list*) {return 100;}
int status_get_homdex(const block_list*) {return 120;}
int status_get_homluk(const block_list*) {return 80;}
struct Mob {status_data status;};
struct Database {
 unsigned warnings=0;
 bool nodeExists(const ryml::NodeRef& node,const char* name) {return node.has_child(c4::to_csubstr(name));}
 bool asUInt16(const ryml::NodeRef& node,const char* name,uint16& value) {node[c4::to_csubstr(name)] >> value;return true;}
 bool asUInt64(const ryml::NodeRef& node,const char* name,uint64& value) {node[c4::to_csubstr(name)] >> value;return true;}
 void invalidWarning(const ryml::NodeRef&,const char*,...) {++warnings;}
 int parse(const ryml::NodeRef& node,std::shared_ptr<Mob> mob) {
'''
    main = r'''
int main() {
 for(uint64 input : {uint64(0),uint64(1),uint64(50000),uint64(65534),uint64(65535),uint64(65536),uint64(67733),uint64(68299),uint64(131072),uint64(UINT32_MAX),uint64(UINT32_MAX)+1,UINT64_MAX}) {
  std::string yaml="Attack2: "+std::to_string(input);
  auto tree=ryml::parse_in_arena(c4::to_csubstr(yaml));
  Database db; auto mob=std::make_shared<Mob>();
  check(db.parse(tree.rootref(),mob)==1,"actual Attack2 block succeeds");
#ifdef RENEWAL
  auto value=mob->status.rhw.matk;
#else
  auto value=mob->status.rhw.atk2;
#endif
  check(value==std::min<uint64>(input,USHRT_MAX),"input is bounded before narrowing, never modulo-wrapped");
  check(db.warnings==(input>USHRT_MAX),"out-of-range input is explicitly diagnosed once");
  if(input==67733 || input==68299) std::printf("INPUT %llu RUNTIME %u WARNINGS %u\n",(unsigned long long)input,value,db.warnings);
 }
#ifdef RENEWAL
 for(int type : {BL_PET,BL_MOB,BL_MER,BL_ELEM}) {
  block_list bl{type}; status_data status;status.int_=311;
  for(unsigned raw=0;raw<=USHRT_MAX;++raw) {
   status.rhw.matk=raw;
   unsigned low=status_base_matk_min(&bl,&status,275), high=status_base_matk_max(&bl,&status,275);
   check(low==std::min(USHRT_MAX,586+int(raw)*70/100),"minimum variance saturates at combat limit");
   check(high==std::min(USHRT_MAX,586+int(raw)*130/100),"maximum variance saturates at combat limit");
   check(low<=high,"high MATK never inverts combat damage range");
  }
#ifndef EXPECT_ORIGINAL
  status.rhw.matk=USHRT_MAX; status.int_=SHRT_MAX;
  check(status_base_matk_min(&bl,&status,INT_MAX)==USHRT_MAX,"wide addition protects minimum at extreme level");
  check(status_base_matk_max(&bl,&status,INT_MAX)==USHRT_MAX,"wide addition protects maximum at extreme level");
#endif
 }
 for(int intelligence : {1,130,500}) {
  block_list bl{BL_PC};status_data status;status.int_=intelligence;status.dex=130;status.luk=100;status.spl=100;
  unsigned expected=intelligence+intelligence/2+130/5+100/3+275/4+500;
  check(status_base_matk_min(&bl,&status,275)==expected,"player minimum formula unchanged");
  check(status_base_matk_max(&bl,&status,275)==expected,"player maximum formula unchanged");
 }
 block_list hom{BL_HOM};status_data status;
 check(status_base_matk_min(&hom,&status,275)==419,"homunculus minimum unchanged");
 check(status_base_matk_max(&hom,&status,275)==475,"homunculus maximum unchanged");
#endif
 std::printf("MOB_MATK_RESULT checks=%u failures=%u\n",checks,failures);
 return failures ? 1 : 0;
}
'''
    return prefix+weapon+middle+attack+'\nreturn 1;\n}\n};\n#ifdef RENEWAL\n'+minimum+'\n'+maximum+'\n#endif\n'+main


def run(build,pre_fix):
    library = ROOT/'3rdparty/rapidyaml/obj/ryml.a'
    if not library.is_file():
        raise SystemExit('Build the Linux map server first; RapidYAML library is required')
    source = fixture(pre_fix); path=build/'matk.cpp';path.write_text(source)
    print('Extracted native fixture SHA256',hashlib.sha256(source.encode()).hexdigest(),flush=True)
    for mode in ('RE','PRE'):
        exe=build/('matk-'+mode)
        flags=['g++','-std=c++17','-O1','-fsanitize=undefined','-fno-sanitize-recover=all']
        if mode=='RE':flags+=['-DRENEWAL']
        if pre_fix:flags+=['-DEXPECT_ORIGINAL']
        flags+=['-I'+str(ROOT/'3rdparty/rapidyaml/src'),'-I'+str(ROOT/'3rdparty/rapidyaml/ext/c4core/src')]
        subprocess.run(flags+[str(path),str(library),'-o',str(exe)],check=True)
        result=subprocess.run([str(exe)],text=True,capture_output=True,timeout=30)
        print(mode, result.stdout, result.stderr, flush=True)
        if pre_fix:
            if result.returncode==0 or 'MOB_MATK_RESULT' not in result.stdout:
                raise AssertionError('Expected ordinary native baseline regression failure')
        else:result.check_returncode()


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--pre-fix',action='store_true')
    args=p.parse_args()
    with tempfile.TemporaryDirectory(prefix='mob-matk-') as directory:
        run(Path(directory),args.pre_fix)
