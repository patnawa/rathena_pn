#!/usr/bin/env python3
# ============================================================================
#  PN  /  DEVELOPMENT TOOLS
#  battlestats_test.py
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/battlestats_test.py
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

"""Compile the actual report function against explicit status/transport doubles.

The production helper is extracted unchanged. This checks output arithmetic,
pagination, argument validation and read-only behavior, not combat calculation.
"""
from pathlib import Path
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parents[2]
source=(ROOT/'src/map/atcommand.cpp').read_text()
helper=source[source.index('static int pn_battlestats('):source.index('ACMD_FUNC(bs)',source.index('static int pn_battlestats('))]
fixture=(ROOT/'tools/ci/battlestats_test.cpp').read_text()
assert fixture.count('// PRODUCTION_HELPER')==1
with tempfile.TemporaryDirectory(prefix='pn-battlestats-') as temp:
    cpp,exe=Path(temp)/'test.cpp',Path(temp)/'test'
    cpp.write_text(fixture.replace('// PRODUCTION_HELPER',helper))
    subprocess.run(['g++','-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-sanitize-recover=all',str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
