#!/usr/bin/env python3
# ============================================================================
#  PN  /  DEVELOPMENT TOOLS
#  fly_wing_rental_regression.py
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/fly_wing_rental_regression.py
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

"""Check named rental boxes grant one wing with the advertised lifetime. Requires PyYAML."""
import argparse
from pathlib import Path
import yaml

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--root',type=Path,default=Path(__file__).resolve().parents[2])
args=parser.parse_args()
groups={row['Group']:row for row in yaml.safe_load((args.root/'db/re/item_group_db.yml').read_text(encoding='utf-8'))['Body']}
expected={'C_GIANT_FLY_1DAY_BOX':('C_Giant_Fly_Wing',1440),
          'C_GIANT_FLY_1DAY_BOX_':('C_Giant_Fly_Wing',1440),
          'C_GIANT_FLY_1DAY_BOX__':('C_Giant_Fly_Wing',1440),
          'E_WING_OF_FLY_3DAY_BOX':('C_Wing_Of_Fly',4320)}
for name,(item,minutes) in expected.items():
    subs=groups[name]['SubGroups']
    assert len(subs)==1,(name,'multiple reward groups')
    assert subs[0]['Algorithm']=='All',(name,'not deterministic')
    rows=subs[0]['List'];assert len(rows)==1,(name,'duplicate wing reward')
    row=rows[0]
    assert row['Item']==item,(name,'wrong item')
    assert row.get('Amount',1)==1,(name,'multiple wings')
    assert row['Duration']==minutes,(name,'incorrect rental lifetime')
    assert row.get('Rate',10000)==10000,(name,'unexpected conditional reward')
print('PASS: four rental groups, one deterministic wing each, correct 1/3-day lifetimes')
