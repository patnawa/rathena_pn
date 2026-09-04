# Fashion Points integration

The implementation is intentionally isolated and does not alter shared loader
or import files. rAthena does not discover arbitrarily named YAML databases.
Apply these integration changes when this package is accepted.

## NPC loader

```diff
--- a/npc/scripts_custom.conf
+++ b/npc/scripts_custom.conf
@@
+# BEGIN FASHION POINTS
+npc: npc/custom/fashion_points/FashionPoints.txt
+npc: npc/custom/fashion_points/FashionEnchant.txt
+# END FASHION POINTS
```

## Server database imports

This repository loads the two isolated item databases directly from the
`Footer.Imports` list in `db/item_db.yml`:

- `db/import/fashion_points_box_item_db.yml` (21 Delayconsume boxes)
- `db/import/fashion_points_missing_item_db.yml` (8 materials + 8 enchants)

It likewise loads `db/import/fashion_points_item_combos.yml` directly from the
`Footer.Imports` list in `db/item_combos.yml`.

The three uniquely named YAML files are explicit loader targets. This
repository ignores most `db/import/` additions, so force-add them when staging.

## Client metadata

Follow `client-patch/fashion_points/README.md`. The 37-entry Lua fragment must
be merged into the client's main itemInfo table during packaging; the client
does not auto-load arbitrary itemInfo fragments.
