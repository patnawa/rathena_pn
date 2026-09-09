#!/usr/bin/env python3
# ============================================================================
#  PN  /  DEVELOPMENT TOOLS
#  aloot_sets_test.py
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/aloot_sets_test.py
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

"""Compile the unchanged profile command against registry and item-DB doubles."""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / 'src/map/atcommand.cpp').read_text()
start = source.index('static int pn_aloot_set_command(')
helper = source[start:source.index('ACMD_FUNC(alootconfig)', start)]
fixture = (ROOT / 'tools/ci/aloot_sets_test.cpp').read_text()
with tempfile.TemporaryDirectory(prefix='pn-aloot-sets-') as temp:
    cpp, exe = Path(temp) / 'test.cpp', Path(temp) / 'test'
    cpp.write_text(fixture.replace('// PRODUCTION_HELPER', helper))
    subprocess.run(['g++', '-std=c++17', '-Wall', '-Wextra', '-Werror', '-fsanitize=address,undefined', '-fno-sanitize-recover=all', str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
