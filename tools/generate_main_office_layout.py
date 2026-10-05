#!/usr/bin/env python3
# ============================================================================
#  PN  /  DEVELOPMENT TOOLS
#  generate_main_office_layout.py
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/generate_main_office_layout.py
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

"""Generate the compact Office desks and grouped directory from verified walkable cells."""
from collections import deque

def component(w,h,cells,start,blocked=frozenset()):
    seen={start};queue=deque([start])
    assert cells[start[0]+start[1]*w] in (0,3) and start not in blocked, ('bad entrance',start)
    while queue:
        x,y=queue.popleft()
        for p in [(x-1,y),(x+1,y),(x,y-1),(x,y+1)]:
            a,b=p
            if 0<=a<w and 0<=b<h and p not in seen and p not in blocked and cells[a+b*w] in (0,3):
                seen.add(p);queue.append(p)
    return seen

def generate():
    from compact_office_layout import generate as compact_generate
    compact_generate()

if __name__=='__main__':generate()
