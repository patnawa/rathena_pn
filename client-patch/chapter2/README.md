# Chapter 2 client compatibility patch

Chapter 2's post-2025 map files are not fully extractable from current encrypted
Gravity archives. The server therefore ships a synchronized map-cache alias set.
Clients must append `resnametable_chapter2.txt` to the active
`data/resnametable.txt` and load `SystemEN/itemInfo_Chapter2.lua`, or run
`install_client_patch.ps1` to install both automatically. The installer also
adds readable Chapter 2 entries to `SystemEN/OngoingQuests.lub` through the
shared quest compatibility patch.

## Direct map fallback patch

If the client reports a world/NULL error when entering a Chapter 2 map,
resource-name redirects alone may not be sufficient. The offline builder
`tools/ci/build_chapter2_map_patch.py` materializes the 17 configured fallback
maps as 51 GAT/GND/RSW entries in `chapter2_maps.grf`. It preserves source bytes
and follows existing resource-table aliases, including `1@ch1b` to `1@ch1a`.

Run from the repository root with Python 3:

```sh
python3 tools/ci/build_chapter2_map_patch.py \
  --source-data /path/to/extracted/data \
  --resource-table /path/to/client/data/resnametable.txt \
  --output /path/to/new-review-directory
```

Supply the unencrypted fallback resources from the client's own archives:
`mu_fild01`, `icas_in`, `uknw_ruin2`, `ch1_geffen`, `1@ch1a`, and `ch1_sf02`.
The resource table determines any additional source alias resolution.
The builder does not install anything. Validate the output, back up DATA.INI,
then copy the GRF into the client root and add it as the highest-priority entry,
retaining the relative order of all existing archives. Fully close and restart
the client. Do not commit generated game assets to this repository.

Archive extraction/hash checks establish resource integrity, not successful
in-game rendering. Retest the affected warp with the actual executable.
To roll back, close the client and restore the previous DATA.INI.

Example:

```powershell
powershell -ExecutionPolicy Bypass -File install_client_patch.ps1 `
  -ClientDataDirectory "C:\path\to\Ragnarok\data" `
  -BaseResnametable "C:\path\to\extracted\data\resnametable.txt"
```

When native Chapter 2 GAT/GND/RSW resources become legitimately available,
remove these Chapter 2 alias lines and rebuild the server cache from the same
native GAT files.
