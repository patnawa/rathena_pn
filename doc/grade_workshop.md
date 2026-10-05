# Grade Workshop arrangement

The existing `grademk` room uses two short service rows with a clear central
walking aisle. The northern row follows the upgrade workflow: Shadow books and
materials, equipment enchants, grade enhancement, refinement, then ore supplies.
Healing, episode progress, skill supplies, rune tablets and the Damage Lab occupy
the support row and sides. A Workshop Guide marks the walking route to the
adjacent standing cell for each service, including the Main Office entrance.

The Master Refiner is inside the walkable floor at **38,181**, beside the ore
dealer at **42,181**. Sratos moves from **34,184** to **32,181**, leaving the
existing `@go`/Warper arrival free. The original Prontera portal arrival at
**13,172** and exit portal are retained. All existing unique NPC identities and
service bodies, prices, success rates, eligibility and material recipes remain
unchanged by this arrangement.

`tools/generate_grade_workshop_layout.py` is the placement and directory source.
It generates `npc/custom/grade_workshop_layout.json` and
`npc/custom/grade_workshop_directory.txt`; `PN_GradeWorkshopDirectory` is the
shared script entry point. The Main Office generator owns `Main Office#gr` at
**20,179** and must retain that coordinate.

`tools/ci/grade_workshop_test.py` resolves the entire enabled Renewal script
configuration and checks all 12 placed services. It reads the effective native
map cache, excludes every NPC cell and exit warp trigger, then verifies both
arrivals and every adjacent standing cell are connected. Six small regressions
cover open floor, arrival overlap, walls, an NPC blocking a doorway, duplicate
NPC cells and warp collisions. The generator is repeatable without output drift.

The equipment service native regression now follows the actual consolidated
menu and its legacy submenu: 14 paths cover the five existing native enchant
groups through both routes, Cancel and Escape. Inventory and zeny must remain
unchanged on all these entry paths, and enchant windows must wait for close
acknowledgement. Paid Star of Spell upgrades remain outside this entry-route
regression and are not treated as free operations.

Geometry checks establish walking access against native collision cells. A
rendered client check is still needed to assess sprite overlap and visual
alignment with the original map artwork; geometry alone does not prove those.
