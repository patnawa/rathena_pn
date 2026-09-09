# Server navigation and episode quest guide repair

The installed client release is retained under `server-work/navigation-repair-20260909/release` in the owner's game directory. Its seven-file ZIP is for the matching client build. Close the game before installing it, then restart the game. Back up existing files first. For another client layout, merge the navigation GRF into DATA.INI at highest priority rather than replacing the archive list.

This patch supplies server-specific map, NPC, monster, travel and distance tables for both KRPRI and KRSAK navigation names. It also repairs quest-link syntax, map names and confirmed NPC coordinates; synchronizes the legacy quest loaders; and handles the navigation helper's end-of-table boundary safely.

`SystemEN/QuestNavigationRepair.lua` is loaded after the existing canonical `SystemEN/OngoingQuests.lub` table and compatibility patches:

```lua
dofile("SystemEN/QuestNavigationRepair.lua")
```

The three small OngoingQuestInfoList loader files route older client entry points to that canonical file. Five custom records from the old fallback table are preserved. Do not install these loaders without the canonical quest file and its repair overlay. The repair appends notes with direct table assignment because the game quest environment does not reliably expose `table.insert`; native regression loading disables that function.

DATA.INI supports only ten archives (slots 0–9). The current client combines this archive with the episode compatibility overlay as `client_repairs.grf`; do not add an eleventh archive and displace the base data archive.

The native generator now compares iterators from the correct warp vector when classifying segmented maps. Main Office travel and 52 missing literal Warper menu destinations have explicit navigation registrations. Additional existing literal script travel is registered only in an isolated generation workspace using `tools/navigation/register_script_travel.py`; those generated NPC copies are not live-server replacements. Conditions, rewards, player variables and quest access checks remain in the original scripts.

To stage those annotations after a first native generator pass:

```sh
python tools/navigation/register_script_travel.py --root /isolated/server --maps /first-pass/navi_map_krpri.lub --output /work/navigation
```

Review the manifest, copy its `registration-source/npc` overlay into the isolated generation tree, and rerun `map-server-generator --generate-navi` there. Use an isolated database and network; generator startup executes NPC initialization. The staging tool preserves original code and adds generation-only labels, with an explicit end before each label to prevent fall-through.

The deployed table set also preserves translated map labels, includes six script-spawned Luanda hunt monsters (50 per species in the relevant phase), and excludes 139 old special-shop records whose map/coordinate/sprite tuples have no counterpart in the current NPC catalog. Ordinary shop NPCs remain searchable through the main NPC table. Dynamic monsters are not guaranteed to be present during both Luanda phases.

Build a GRF from the validated table directory, without committing the generated archive:

```sh
python client-patch/navigation_repair/build_grf.py /validated/navigation --output navigation_repair.grf
```

This command packages tables; it does not regenerate or semantically validate them. The complete generation workspace and original/live snapshots are retained at `/app/rathena-builds/navigation-repair-20260909` and in the owner's local audit directory.

Validation covered 11,393 quest records, 3,972 text links, 318 structured NPC destinations and 280,348 native Lua navigation callbacks. All 966 links attributed to active episode scripts have valid destinations across 102 maps, and each map is connected to Prontera in the travel graph. Actual client GAT files passed coordinate/approach checks, using the low-byte collision cell type employed by mapcache and preserving additional flags. Existing NPC placements and travel endpoints were retained. All four quest loader entry points expose the same corrected table, and the client's original compiled quest helper ran against every record.

These are static data, geometry, graph and Lua checks, not a playthrough of every quest. Navigation cannot prove a particular character meets a route's quest, disguise, level, party or instance conditions. Eight NPC-name discrepancies remain for legacy client quest records outside the identified active episode references; they are recorded for review in the release validation JSON, rather than redirected to a guessed NPC.
