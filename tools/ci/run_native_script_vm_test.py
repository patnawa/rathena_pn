#!/usr/bin/env python3
# ============================================================================
#  PN  /  DEVELOPMENT TOOLS
#  run_native_script_vm_test.py
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/run_native_script_vm_test.py
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

"""Build/run an isolated Linux real-VM proof without launching map-server.

Requires a previously built local Linux map-server object set. The exercised
script.cpp, allocator and test driver are freshly compiled with current headers;
other objects only satisfy link dependencies. Explicit doubles isolate the few
world-boundary functions called by this narrow inventory test. This is not a
fresh full-server integration build and does not test arbitrary old objects.
"""
import argparse
import hashlib
from pathlib import Path
import subprocess
import tempfile

from audit_enchant_upgrades import renewal_records


ROOT = Path(__file__).resolve().parents[2]
WRAPPERS = (
    "main", "_Z9map_id2sdi", "_Z9map_id2ndi", "_Z11mapreg_initv", "_Z12mapreg_finalv",
    "_Z17npc_event_dequeueP16map_session_datab",
    "_Z11log_pick_pcPK16map_session_data15e_log_pick_typeiPK4item",
    "_Z12clif_additemPK16map_session_dataiih",
    "_Z12clif_delitemRK16map_session_dataiis", "_Z9ShowErrorPKcz",
)


def run(build, sanitizer):
    # Fail if the small native fixtures drift from the effective live-layout
    # item definitions. Item effects and unrelated DB fields are not simulated.
    expected = {490136: ("Armor", "Normal", 1), 310710: ("Card", "Enchant", 0),
                310711: ("Card", "Enchant", 0), 4001: ("Card", "Normal", 0)}
    records = {}
    for record in renewal_records(ROOT, "db/item_db.yml"):
        if record["Id"] in expected:
            records.setdefault(record["Id"], {}).update(record)
    for item_id, metadata in expected.items():
        item = records.get(item_id, {})
        actual = (item.get("Type"), item.get("SubType", "Normal"), item.get("Slots", 0))
        if actual != metadata:
            raise SystemExit(f"Native fixture {item_id} metadata drift: {actual} != {metadata}")
    print("Four native fixtures agree with effective Renewal item type/subtype/slots", flush=True)
    objects = sorted(path for path in (ROOT / "src/map/obj").rglob("*.o") if path.name != "script.o")
    libraries = [ROOT / path for path in (
        "src/common/obj/common.a", "3rdparty/libconfig/obj/libconfig.a",
        "3rdparty/rapidyaml/obj/ryml.a",
    )]
    if not objects or any(not path.is_file() for path in libraries):
        raise SystemExit("Build the local Linux map-server first; required support objects are missing")
    includes = ["src", "3rdparty/libconfig", "3rdparty/rapidyaml/src",
                "3rdparty/rapidyaml/ext/c4core/src", "3rdparty/json/include", "/usr/include/mysql"]
    sanitizer_flags = ["-fsanitize=" + sanitizer, "-fno-sanitize-recover=all"]
    flags = ["g++", "-std=c++17", "-O0", "-g", "-DPACKETVER=20260219", "-fno-strict-aliasing", "-fno-omit-frame-pointer"] + sanitizer_flags
    flags.extend("-I" + path for path in includes)
    compiled = []
    # The standalone allocator object resolves the common archive's allocator
    # symbols first, so a stale malloc.o cannot hide the current alignment fix.
    for source in ("src/map/script.cpp", "src/common/malloc.cpp", "tools/ci/native_script_vm_test.cpp"):
        target = build / (Path(source).stem + ".o")
        print("Compiling current " + source, flush=True)
        print("SHA256 " + hashlib.sha256((ROOT / source).read_bytes()).hexdigest(), flush=True)
        subprocess.run(flags + ["-c", source, "-o", str(target)], cwd=ROOT, check=True)
        compiled.append(target)
    executable = build / "native_script_vm_test"
    command = ["g++"] + sanitizer_flags + ["-o", str(executable)]
    command.extend(str(path) for path in compiled + objects + libraries)
    command.extend("-Wl,--wrap=" + symbol for symbol in WRAPPERS)
    command.extend(["-lz", "-ldl", "-lmysqlclient", "-lzstd", "-lssl", "-lcrypto", "-lresolv", "-lm"])
    print("Linking isolated test entry point (normal server startup is never invoked)", flush=True)
    subprocess.run(command, cwd=ROOT, check=True)
    print("Running with mandatory kernel-level socket/connect/bind/listen denial", flush=True)
    completed = subprocess.run([str(executable)], cwd=ROOT, capture_output=True, text=True, timeout=60)
    print(completed.stdout, end="", flush=True)
    print(completed.stderr, end="", flush=True)
    completed.check_returncode()
    if completed.stdout.count("NATIVE_VM_FIXTURE_COMPLETE: 13 builtin cases") != 1:
        raise SystemExit("Real VM fixture did not reach its required completion marker")
    if "PASS actual rAthena script VM:" not in completed.stdout:
        raise SystemExit("Native postconditions did not complete")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sanitizer", choices=("address", "undefined", "address,undefined"),
                        default="address,undefined")
    parser.add_argument("--build-dir", type=Path, help="Keep generated objects/binary in this Linux directory")
    arguments = parser.parse_args()
    if arguments.build_dir:
        build_dir = arguments.build_dir.resolve()
        build_dir.mkdir(parents=True, exist_ok=True)
        run(build_dir, arguments.sanitizer)
    else:
        with tempfile.TemporaryDirectory(prefix="rathena-script-vm-") as directory:
            run(Path(directory), arguments.sanitizer)
