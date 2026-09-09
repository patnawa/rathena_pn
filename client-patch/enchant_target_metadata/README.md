# Optional existing-server enchant-target metadata

Supplies **only twelve existing-server crown records** that were missing from
the original active client's merged itemInfo table. It adds no server items,
aliases, recipes, gameplay effects, or acquisition routes. The parent agent
installed this reviewed package separately after the candidate checks.

All twelve card-slot counts are explicitly 1 in both effective server data
and supplied reference server metadata. ClassNum follows their matching server View.
Identified resource names are reference-backed and present in the active
GRFs; the unidentified icon is the verified generic EpisodClear20 fallback.
Names preserve current server labels, including unfinished translations.
Descriptions are clean-room renderings of all twelve effective item Scripts
and fifteen matching combo Scripts: numeric bonuses, cumulative refine/grade
gates, named skills, equipped partners, learned-skill gates, cooldowns and
autocast levels/chances. They do not copy third-party effect prose. The strict
renderer rejects unknown operations, conditions, expressions and source drift.

See `doc/enchant_target_metadata_audit.md` for every ID, resource and evidence
hash. `manifest.json` is the exact reviewed allowlist.

## Verify without installing

From the repository root, using Python 3, PyYAML and the already supplied
matching Win32 Lua 5.1 runtime:

```sh
python3 -B client-patch/enchant_target_metadata/verify.py --reference-system /path/to/reference/System --reference-items /path/to/reference/items.lua
python3 -B client-patch/enchant_target_metadata/test_effects.py
```

Under Windows prefix the command with `wsl -d Ubuntu --exec`.
Verification executes the real active SystemEN loader, original compiled
item-name lookup, actual F_itemInfoMerge, and scoped original enchant-helper
target checks. It compares every generated description/scalar against fresh
effective server definitions, deep-copies all pre-existing record fields and
requires exactly twelve new records. It also indexes active GRFs read-only.
Use Python `-B` to suppress imported-module bytecode caches.

Verified 2026-09-06: PASS; missing target metadata 43 → 31; exactly twelve
one-slot records; every previous metadata field deeply preserved; twenty
existing graphic entries verified; ten focused regressions passed. Fragment
SHA-256: `8ab7c0eafa30bf95918f24c2a799eb2d63ef18fe0a6884d2c61d21cd2bcf2772`.

Candidate verification accepts `--before-loader PATH`; its default is the
currently active `SystemEN/itemInfo.lua`. The selected path is always reported.
After a separately authorized installation, verify the real current loader
against a pre-install backup, not an ephemeral candidate merge:

```sh
python3 -B client-patch/enchant_target_metadata/verify.py --reference-system /path/to/reference/System --reference-items /path/to/reference/items.lua --installed \
  --before-loader ../client-before-enchant-crowns-20260906/SystemEN/itemInfo.lua
```

Installed mode requires exactly one added file/table pair, no other loader
source changes, preserved previous import order and all previous metadata
fields deeply unchanged. It must still find exactly twelve added records.
Later additions can be tested separately; `--loader PATH` selects an explicit
retained after-crown checkpoint. Output always reports `checked_loader` and
`loader_is_active`, so archived checks never masquerade as active-client proof.

The package's `run_native_bonus_vm_test.py` builds an isolated Linux actual
rAthena VM proof of the Cardinal alias and Hyper Novice combo conditions.
It freshly compiles `pc.cpp`, `skill.cpp`, `script.cpp`, `clif.cpp` and
`malloc.cpp` with ASan/UBSan, linking unrelated existing local objects only
for dependencies. Explicit world-boundary doubles and mandatory kernel
socket denial avoid normal server startup/network activity. Generated test
files stay in a temporary directory or an explicit `--build-dir` outside
the repository.

Native result: PASS, 44,205 actual VM executions / 221,352 assertions, no
ASan/UBSan findings or allocator leaks. Cardinal's parent and dummy queries
stay single-counted for all refines 0–20 and grades 0–4. Hyper Novice's old
unreachable bonus and corrected bonus are compared across both equipment
refines 0–20, both grades 0–4 and learned levels 4/5; unchanged Napalm and
autocast registration are also checked. This does not simulate live damage
delivery or execute the resulting autocasts against game targets.
The runner requires the allocator's explicit clean-teardown message and
rejects allocator warnings, errors, sanitizer or runtime-error diagnostics
even when the process exits zero and prints its completion marker.

## Later integration boundary

A separately reviewed installation can copy
`SystemEN/itemInfo_EnchantTargets.lua` to the existing SystemEN directory
and add its file/table pair to the existing multi-itemInfo loader:

- ImportFiles: `itemInfo_EnchantTargets.lua`
- ImportTables postfix: `enchanttargets` (global table `tbl_enchanttargets`)

Preserve every existing import and its relative order, keep override merging
last, and use the normal non-overwriting merge. If an ID already has metadata,
stop and review it; do not overwrite its description or card-slot count.

At the isolated crown checkpoint, 31 original target gaps remained; these
are outside this package. A later separately verified Shadow166 installation
changes the active metadata baseline. Reproduce this package's exact installed
checkpoint with:

```sh
python3 -B client-patch/enchant_target_metadata/verify.py --reference-system /path/to/reference/System --reference-items /path/to/reference/items.lua --installed \
  --before-loader ../client-before-enchant-crowns-20260906/SystemEN/itemInfo.lua \
  --loader ../client-before-druid-shadow166-20260906/SystemEN/itemInfo.lua
```

This passed with `loader_is_active: false`, exactly twelve new records and
all prior fields deeply preserved. The checked after-crown loader SHA-256 is
`6f26c390128be7b6620bf7727f85b47d0151af56db6eb8275578ad93511b7333`.
The strict crown-only verifier correctly refuses extra later imports when
pointed at the expanded active loader; it does not hide them as preserved
crown-only state. Ten other group-165 crowns were outside this patch, so
this is not complete group-165 or whole-client enchant support. No live game UI,
rendering, charging or live combat verification is claimed.

Reference input paths are explicit; the verifier accepts the original filename and checks the pinned content hashes.
