#!/usr/bin/env python3
# ============================================================================
#  PN  /  DEVELOPMENT TOOLS
#  run_native_allocator_test.py
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/run_native_allocator_test.py
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

"""Freshly compile actual common/malloc.cpp; no existing server objects needed.

Run on Linux/WSL. Only console and version strings are test doubles. The private
production allocator implementation is included directly, with normal USE_MEMMGR
and logging settings. Exit status is nonzero for misalignment or sanitizer errors.
"""
import argparse
import hashlib
from pathlib import Path
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[2]


def run(build, sanitizer, case, debug, source_ref):
    source = ROOT / "src/common/malloc.cpp"
    source_data = (subprocess.check_output(["git", "show", source_ref + ":src/common/malloc.cpp"], cwd=ROOT)
                   if source_ref else source.read_bytes())
    print("Fresh allocator SHA256 " + hashlib.sha256(source_data).hexdigest(), flush=True)
    fixture = ROOT / "tools/ci/native_allocator_test.cpp"
    if source_ref:
        source = build / "baseline_malloc.cpp"
        source.write_bytes(source_data)
        generated = build / "baseline_allocator_test.cpp"
        generated.write_text(fixture.read_text().replace('"../../src/common/malloc.cpp"', '"' + str(source) + '"'))
        fixture = generated
    binary = build / "native_allocator_test"
    command = ["g++", "-std=c++17", "-O0", "-g", "-fno-strict-aliasing",
               "-fno-omit-frame-pointer", "-Isrc", "-Isrc/common", "-I3rdparty/libconfig",
               str(fixture), "-o", str(binary)]
    if debug:
        command.append("-DDEBUG")
    if sanitizer != "none":
        command.extend(["-fsanitize=" + sanitizer, "-fno-sanitize-recover=all"])
    subprocess.run(command, cwd=ROOT, check=True)
    (build / "log").mkdir(exist_ok=True)
    result = subprocess.run([str(binary), case], cwd=build, capture_output=True,
                            text=True, timeout=30)
    print(result.stdout, end="", flush=True)
    print(result.stderr, end="", flush=True)
    return result.returncode


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sanitizer", choices=("none", "undefined", "address", "address,undefined"),
                        default="undefined")
    parser.add_argument("--case", choices=("matrix", "pool", "large-odd", "shutdown"), default="matrix")
    parser.add_argument("--build-dir", type=Path)
    parser.add_argument("--debug", action="store_true", help="Exercise DEBUG_MEMMGR poison checks")
    parser.add_argument("--source-ref", help="Compile allocator from a Git revision without modifying the worktree")
    arguments = parser.parse_args()
    if arguments.build_dir:
        directory = arguments.build_dir.resolve()
        directory.mkdir(parents=True, exist_ok=True)
        raise SystemExit(run(directory, arguments.sanitizer, arguments.case, arguments.debug, arguments.source_ref))
    with tempfile.TemporaryDirectory(prefix="rathena-allocator-") as temporary:
        raise SystemExit(run(Path(temporary), arguments.sanitizer, arguments.case, arguments.debug, arguments.source_ref))
