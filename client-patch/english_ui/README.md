# Rune and English interface repair

This verified client snapshot contains native Tablets / Runes / Cards tabs,
the Episode 21 default, English descriptions for all 65 tablets and 133 runes,
shared English messages, translated function-window and settings text, and
English login warning artwork. Server Rune actions continue through the existing
transaction confirmation dialogs. Rune Stone is at grademk 46,178.

Build and validate from a clean checkout:

```text
python client-patch/english_ui/build.py
python client-patch/english_ui/build.py --merge CLIENT/client_repairs.grf --output repaired-client.grf
```

Close the client and back up the existing files. Copy the contents of `files/`
to the client directory, preserving paths, and replace its `client_repairs.grf`
with the merged output. Keep the existing ten-entry DATA.INI with
client_repairs.grf in slot 0. Restart through Start Game.cmd.

The manifest pins every payload. The builder verifies hashes, duplicate path
aliases and the GRF roundtrip, and preserves unrelated existing repair entries.
Native Lua 5.1 loading, all Rune IDs and bonus thresholds were verified before
deployment. The map binary built successfully and passed startup validation;
`tools/ci/pn_rune_ui_test.cpp` covers native packets and action-state checks.
Login images were inspected; live game-window rendering was not verified.

Translation source: bundled ROenglishRE commit
66cdfec631603fda6a90ba4bbe26ab07b5204c84, with its original notices retained in
`source/rune_desc.lua` and `source/runeset_desc.lua`. One residual Korean suffix
was translated. Native fallback tables were changed only at matching display
strings; numeric data and instructions were preserved. Unmatched legacy tip
content and Korean resource filenames remain; the client already selects the
English SystemEN tip loader. This is not a translation of every game dataset.

The login BMPs are built-in imagegen text localizations of the client's original
12+ and 18+ warning artwork, converted to their original dimensions. Prompts
requested preservation of composition, characters, pictograms and credits,
with English violence/age labels and equivalent age, supervision and health
notices. Gravity and other original artwork rights remain with their owners.
These assets are not a claim of a new age certification.

PN patch code and tab SVG contributions: (C) 2026 PN Development Team,
GPL-3.0-or-later; see ../../LICENSE. Imported data, translations and artwork
retain their original rights and attribution.
