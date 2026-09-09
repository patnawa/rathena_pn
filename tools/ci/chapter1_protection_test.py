#!/usr/bin/env python3
# ============================================================================
#  PN  /  DEVELOPMENT TOOLS
#  chapter1_protection_test.py
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/chapter1_protection_test.py
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

"""Run actual Chapter 1 protection script in an isolated native VM (Linux build required)."""
from pathlib import Path
import argparse,hashlib,re,subprocess,tempfile
WRAPPERS = (
    "main", "_Z9map_id2sdi", "_Z9map_id2ndi", "_Z11mapreg_initv", "_Z12mapreg_finalv",
    "_Z17npc_event_dequeueP16map_session_datab",
    "_Z11log_pick_pcPK16map_session_data15e_log_pick_typeiPK4item",
    "_Z12clif_additemPK16map_session_dataiih",
    "_Z12clif_delitemRK16map_session_dataiis", "_Z9ShowErrorPKcz",
)
ROOT=Path(__file__).resolve().parents[2]
EXTRA=("_Z14map_getmapdatas","_Z16pc_addeventtimerP16map_session_dataiPKc","_Z16pc_deleventtimerP16map_session_dataPKc","_Z17mapindex_name2idxPKcS0_","_Z9pc_setposP16map_session_datatii8clr_type","_Z11map_nick2sdPKcb","_Z19clif_displaymessageiPKc")
def run(build):
 build.mkdir(parents=True,exist_ok=True)
 raw=(ROOT/"npc/custom/chapter1/CH1.c").read_text(encoding="utf-8")
 body=re.search(r"-\tscript\tCH1_DimensionalGuard\t-1,(\{.*?\n\})",raw,re.S).group(1)
 for name in ("hem_dun02","ch1_gfn01","ch1_gfn03"):assert f"{name}\tmapflag\tloadevent" in raw
 assert "amicitia2\tmapflag\tnomemo" in (ROOT/"npc/re/mapflag/nomemo.txt").read_text()
 fixture=build/"guard.script";fixture.write_text(body)
 prefix=(ROOT/"tools/ci/native_script_vm_test.cpp").read_text().split('extern "C" int __wrap_main')[0]
 driver=build/"guard.cpp";driver.write_text(prefix+(ROOT/"tools/ci/chapter1_protection_test.cpp").read_text())
 objects=sorted(p for p in (ROOT/"src/map/obj").rglob("*.o") if p.name!="script.o")
 libs=[ROOT/p for p in ("src/common/obj/common.a","3rdparty/libconfig/obj/libconfig.a","3rdparty/rapidyaml/obj/ryml.a")]
 assert objects and all(p.exists() for p in libs)
 flags=["g++","-std=c++17","-O0","-g","-DPACKETVER=20260219","-fsanitize=undefined","-fno-sanitize-recover=all","-fno-strict-aliasing"]
 flags += ["-I"+str(ROOT/p) for p in ("src","3rdparty/libconfig","3rdparty/rapidyaml/src","3rdparty/rapidyaml/ext/c4core/src","3rdparty/json/include")]+["-I/usr/include/mysql"]
 fresh=[]
 for p in (ROOT/"src/map/script.cpp",ROOT/"src/common/malloc.cpp",driver):
  target=build/(p.stem+".o");print("SHA256",p,hashlib.sha256(p.read_bytes()).hexdigest(),flush=True)
  subprocess.run(flags+["-c",str(p),"-o",str(target)],cwd=ROOT,check=True);fresh.append(target)
 exe=build/"guard-test"
 linklibs=["-lz","-ldl","-lmysqlclient","-lssl","-lcrypto","-lresolv","-lm"]
 # Optional configured dependencies vary: MySQL may require zstd and
 # npc_chat.o requires PCRE when configure detected it. Preserve builds
 # without either library (including Alpine MariaDB configurations).
 for library in ("zstd", "pcre"):
  for suffix in ("so", "a"):
   filename=f"lib{library}.{suffix}"
   resolved=subprocess.check_output(["g++",f"-print-file-name={filename}"],text=True).strip()
   if resolved!=filename and Path(resolved).is_file():
    linklibs.append(f"-l{library}");break
 cmd=["g++","-fsanitize=undefined","-o",str(exe)]+list(map(str,fresh+objects+libs))+["-Wl,--wrap="+x for x in WRAPPERS+EXTRA]+linklibs
 subprocess.run(cmd,cwd=ROOT,check=True)
 result=subprocess.run([str(exe),str(fixture)],cwd=ROOT,text=True,capture_output=True,timeout=60)
 print(result.stdout);print(result.stderr);result.check_returncode()
 assert "CH1_PROTECTION_OK cases=20" in result.stdout and not result.stderr
 assert "Memory manager: No memory leaks found." in result.stdout
if __name__=="__main__":
 p=argparse.ArgumentParser();p.add_argument("--build-dir",type=Path);a=p.parse_args()
 if a.build_dir:run(a.build_dir.resolve())
 else:
  with tempfile.TemporaryDirectory(prefix="ch1-protection-") as d:run(Path(d))
