#!/usr/bin/env python3
# ============================================================================
#  PN  /  DEVELOPMENT TOOLS
#  scdata_reload_test.py
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/scdata_reload_test.py
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

"""Compile the production status reload handler against SQL/FIFO test doubles."""
import argparse
import pathlib
import subprocess
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser()
parser.add_argument('--emit', type=pathlib.Path)
parser.add_argument('--cxx', default='g++')
args = parser.parse_args()
source = (ROOT / 'src/char/char_mapif.cpp').read_text()
handler = source[source.index('int32 chmapif_parse_askscdata('):source.index('\n/**', source.index('int32 chmapif_parse_askscdata('))]
mmo = (ROOT / 'src/common/mmo.hpp').read_text()
record = mmo[mmo.index('struct status_change_data {'):mmo.index('\n};', mmo.index('struct status_change_data {')) + 3]
prefix = r'''
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
using int32 = int32_t; using uint16 = uint16_t; using int16 = int16_t; using t_tick = int64_t;
#define ENABLE_SC_SAVING
#define SQL_SUCCESS 0
#define SQL_ERROR -1
int sql_handle = 0;
struct { const char* scdata_db = "sc_data"; } schema_config;
size_t rows, row, skipped, warnings, reserved;
std::vector<unsigned char> input(10), output;
uint16& word(size_t offset) { return *reinterpret_cast<uint16*>(output.data()+offset); }
int32& number(size_t offset) { return *reinterpret_cast<int32*>(output.data()+offset); }
#define RFIFOREST(fd) input.size()
#define RFIFOL(fd,o) (*reinterpret_cast<int32*>(input.data()+(o)))
#define RFIFOSKIP(fd,n) (skipped += (n))
#define WFIFOHEAD(fd,n) (reserved=(n),output.resize(reserved))
#define WFIFOW(fd,o) word(o)
#define WFIFOL(fd,o) number(o)
#define WFIFOP(fd,o) (output.data()+(o))
#define WFIFOSET(fd,n) (assert((n)<=reserved),output.resize(n))
int Sql_Query(int, const char*, ...) { row=0; return SQL_SUCCESS; }
size_t Sql_NumRows(int) { return rows; }
int Sql_NextRow(int) { return row<rows ? (++row,SQL_SUCCESS) : SQL_ERROR; }
void Sql_GetData(int,int column,char** data,void*) {
 static std::string value;
 value = std::to_string(column == 0 ? row : column == 1 ? 123456789012LL+row : row*10+column);
 *data = value.data();
}
void Sql_ShowDebug(int) { assert(false); }
void Sql_FreeResult(int) {}
void ShowWarning(const char*,...) { ++warnings; }
'''
suffix = r'''
int main() {
 const size_t capacity=(UINT16_MAX-14)/sizeof(status_change_data);
 for(size_t n : {size_t(0),size_t(49),size_t(50),size_t(51),size_t(100),size_t(1000),capacity,capacity+1}) {
  rows=n; skipped=warnings=reserved=0; output.clear();
  RFIFOL(0,2)=123; RFIFOL(0,6)=456;
  assert(chmapif_parse_askscdata(0)==1);
  const size_t expected=std::min(n,capacity);
  assert(skipped==10 && warnings==(n>capacity));
  assert(word(0)==0x2b1d && number(4)==123 && number(8)==456);
  assert(word(12)==expected && word(2)==14+expected*sizeof(status_change_data));
  assert(output.size()==word(2) && output.size()<=UINT16_MAX);
  for(size_t i=0;i<expected;++i) {
   status_change_data value; memcpy(&value,output.data()+14+i*sizeof(value),sizeof(value));
   assert(value.type==i+1 && value.tick==123456789012LL+i+1);
   assert(value.val1==(i+1)*10+2 && value.val4==(i+1)*10+5);
  }
  std::cout << n << " SQL statuses -> " << expected << " packet statuses: PASS\n";
 }
}
'''
test = prefix + record + '\n' + handler + suffix
if args.emit:
    args.emit.write_text(test)
else:
    with tempfile.TemporaryDirectory() as directory:
        path = pathlib.Path(directory)
        (path / 'test.cpp').write_text(test)
        subprocess.run([args.cxx, '-std=c++17', '-O0', '-g', '-fsanitize=address', str(path / 'test.cpp'), '-o', str(path / 'test')], check=True)
        subprocess.run([str(path / 'test')], check=True)
