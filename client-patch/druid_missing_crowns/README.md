# Missing crowns / Sky weapon partners

Partial, additive package: 12 supported crowns and 11 complete partner weapons; Frontier 401195 remains unresolved. See [the audit](../../doc/druid_missing_crowns_audit.md) for primary sources, exact assumptions and verification limits.

Integration status: parent wired the server imports and installed this fragment together with the coordinated 12-weapon fragment. The combined actual-active 35-record preservation test passed. The package itself never installs or deploys anything.

Nothing here installs automatically. Parent integration owns the three Renewal import lines, native enchant target overlay, and one `itemInfo_DruidMissingCrowns.lua` / `druidmissingcrowns` pair in the active SystemEN loader. Existing imports must retain their order. Copying the fragment without the complete server items and corresponding set files is not a supported installation.

The original compiled client name table already knows all 23 identities. No replacement bytecode, GRF overlay, guessed sprites or new item aliases are needed. Art is reference-backed and present; only unidentified equipment uses the already installed generic `EpisodClear20` icon.

From the repository under WSL:

```sh
python3 -B client-patch/druid_missing_crowns/build.py
python3 -B tools/ci/druid_missing_crowns_test.py
python3 -B tools/ci/druid_missing_crowns_test.py --native-build-dir ../missing-crowns-native-proof-20260906
python3 -B client-patch/druid_missing_crowns/verify_client.py --loader ../client-before-druid-missing-equipment-20260906/SystemEN/itemInfo.lua
```

After server import wiring, add `--require-import` to the server test. After a dedicated client installation, use `verify_client.py --installed --before-loader PATH`; `--loader PATH` allows an explicitly labeled retained checkpoint if another package is installed later. Parent may instead run a combined 35-record installation proof for this package plus the coordinated 12-weapon batch.

`facts.py` is the clean-room reviewed source. `build.py` checks exact deterministic YAML, complete descriptions, and the manifest. `--patch` only emits an apply_patch patch for entirely missing files and refuses overwrites; it never edits active client data. Tests and generated native fixtures belong in isolated server-work directories outside the repository. No network/socket/SQL/live-player server startup is part of the native proof.
