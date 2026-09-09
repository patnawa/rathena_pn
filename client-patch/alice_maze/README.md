# Original maze terrain

These maps contain original deterministic maze geometry. A fixed-seed depth-first maze provides four-cell-wide passages; clearings protect every scripted entrance and guide. Separate stages remain disconnected until their NPC route permits travel.

GAT collision and GND rendering share one grid: each GND cube covers exactly two by two GAT cells. Walls rise eighteen game units; floors remain level. GND1.7 and RSW2.0 reference existing base-client textures `gol\gol_bot01.bmp` and `gol\gol_wall01.bmp`. No third-party terrain or texture bytes are bundled. The two indexed 512x512 minimaps use the native CP949 `data\texture\???????\map` archive directory.

## Build and validate

Run from the repository root. Extract the client's effective `data/mapnametable.txt` into a temporary directory first: read the archives in `DATA.INI` priority order and use the first archive containing that file. Retain its original bytes and encoding.

```sh
python client-patch/alice_maze/build_maps.py --output /tmp/alice-maze --map-name-table /tmp/effective-mapnametable.txt
python tools/ci/alice_geometry_test.py
```

With `--map-name-table`, the output GRF contains nine entries: six terrain files, two minimaps, and the complete supplied map-name table with two labels appended. All prior table bytes are preserved, including legacy encoding. A missing final newline receives a separator; duplicate or conflicting Alice labels cause an error. Rebuilding from an already updated table is idempotent. The original supplied file is never modified. Do not commit the extracted or generated full table.

Without this option, the builder produces eight entries and leaves map labels for a separately managed overlay. Builds also emit PNG previews, `spatial-contract.json`, and `build-report.json` hashes. Committed `data` files must match the generator byte-for-byte. Tests independently parse binary formats, compare every rendered height against collision, validate actual NPC/travel coordinates, and check the server map cache.

## Install and restart

1. Close the game client completely. Back up `client_repairs.grf`, `SystemEN/QuestNavigationRepair.lua`, and `DATA.INI` outside the client release directory.
2. Merge the new nine-entry overlay first, preserving the existing repair archive's remaining resources:

   ```sh
   python client-patch/client_compat/merge_grfs.py /tmp/alice-maze/alice_maze.grf /path/to/client/client_repairs.grf --output /tmp/client_repairs.grf
   ```

3. Replace the client's `client_repairs.grf` with that merged file. Copy `client-patch/navigation_repair/SystemEN/QuestNavigationRepair.lua` to the client's `SystemEN/QuestNavigationRepair.lua`. The canonical `SystemEN/OngoingQuests.lub` must retain its existing final `dofile("SystemEN/QuestNavigationRepair.lua")` import; the three fallback quest loaders delegate to that canonical file.
4. Keep the existing ten-entry `DATA.INI` unchanged, with `client_repairs.grf` first. Do not add `alice_maze.grf` as an eleventh archive. Run `python tools/ci/client_preflight.py /path/to/client` and the native Lua quest check before distribution.
5. Deploy the matching server release, including both map registrations, generated map-cache entries, database imports, instance script, and required engine changes. The release's cache is checked with `python tools/build_alice_mapcache.py --check`. Restart the map server during the coordinated release; a script reload alone does not load new map-cache or engine changes.
6. Start the game client again to load the replacement archive and quest data. An already running client does not reliably refresh those resources.

The previews and geometry checks do not constitute a rendered in-game playthrough. Encounter balance, phase gates, timers, and reward rules remain in the server scripts.
