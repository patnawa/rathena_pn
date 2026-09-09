#!/usr/bin/env python3
# ============================================================================
#  PN  /  DEVELOPMENT TOOLS
#  char_shared_header_dependency_test.py
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/char_shared_header_dependency_test.py
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

"""Exercise the real char Makefile's shared-header dependencies with GNU make."""
import os
from pathlib import Path
import subprocess
import tempfile


def check(template: str, expected: int) -> None:
    with tempfile.TemporaryDirectory(prefix="char-header-dependency-") as tmp:
        root = Path(tmp)
        char = root / "src/char"
        files = ["src/char/probe.cpp", "src/char/probe.hpp",
                 "src/common/sql.hpp", "src/common/mmo.hpp",
                 "3rdparty/libconfig/probe.h", "3rdparty/rapidyaml/probe.hpp",
                 "src/char/obj/probe.o"]
        for name in files:
            path = root / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.touch()
            os.utime(path, (1000, 1000))
        (char / "Makefile").write_text(template.replace("@SET_MAKE@", ""))
        obj = char / "obj/probe.o"
        os.utime(obj, (2000, 2000))
        def query() -> int:
            result = subprocess.run(["make", "-q", "obj/probe.o"], cwd=char,
                                    capture_output=True, text=True)
            assert result.returncode in (0, 1), result.stderr
            return result.returncode
        assert query() == 0, "unchanged object should be current"
        os.utime(root / "src/common/mmo.hpp", (3000, 3000))
        assert query() == expected, "mmo.hpp rebuild dependency is incorrect"
        os.utime(root / "src/common/sql.hpp", (3000, 3000))
        assert query() == 1, "SQL header dependency must be preserved"


if __name__ == "__main__":
    root = Path(__file__).resolve().parents[2]
    template = (root / "src/char/Makefile.in").read_text()
    check(template, 1)
    old = template.replace("COMMON_OBJ = ../common/obj/sql.o",
                           "COMMON_OBJ = ../common/obj/sql.o\nCOMMON_H = ../common/sql.hpp")
    check(old, 0)
    print("PASS: shared-header edits rebuild char objects; old defect reproduced")
