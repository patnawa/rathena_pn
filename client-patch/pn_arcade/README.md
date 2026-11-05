# Minigame Arcade achievements

Adds the arcade's seventeen achievement rows to the client's table. The rows give
no score and no reward; the server definitions are in
`db/import/arcade_achievement_db.yml` and `tools/ci/minigame_arcade_test.py`
requires both sides to match.

```text
python client-patch/pn_arcade/build.py --client CLIENT --output OUTPUT
lua5.1 client-patch/pn_arcade/verify.lua OUTPUT/System/achievement_list.lub 17
```

The output holds `System/achievement_list.lub`, `SystemEN/achievements.lub` and a
merged `client_repairs.grf`, ready for a signed release. `System/achievement_list.lub`
changes from compiled Lua to the equivalent English text, which this client's Lua
loader accepts.

The arcade itself needs no new client map: its room is the existing `guild_vs5`.

PN contributions: (C) 2026 PN Development Team, GPL-3.0-or-later; see ../../LICENSE.
