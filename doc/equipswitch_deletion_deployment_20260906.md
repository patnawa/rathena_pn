# Equipment-switch deletion deployment, 2026-09-06

Installed on the authorized Docker host `192.168.10.18` at
**2026-09-06T12:37:44Z (19:37:44 Asia/Bangkok)**. The scoped deployment exited
zero; all four game services subsequently reported running, not restarting,
and restart count zero. The map server completed 3,610 OnInit calls and became
online. The only observed startup warning was the existing root-privilege
warning. No graphical client playthrough is claimed.

## Runtime scope

Baseline: `50717df9e48a6a762303a05e5d92dcc65092b8bc` and the separately pinned
existing live differences. Exactly one source file and one binary changed:

| Target | Installed raw SHA256 |
| --- | --- |
| `src/map/pc.cpp` | `1c4dee142c4a34d7bb8065e37c5dc6558f0ff2ad52205756d44481c08e764403` |
| `map-server` | `63e554b5829bce76d50b0edb1dacd84ff6bc200393c3f43d821647d05ea7d98a` |

The source adds the inventory-index upper-bound rejection and removes an
exhausted item's equipment-switch registration before unequip/clearing. It
does not change NPC recipes, prices, item metadata, generic capacity commands,
GM reputation, client archives, account records, or Druid definitions. See
[the native audit](equipswitch_deletion_audit.md) for behavior and proof limits.

Candidate `/app/rathena-audit-candidate-20260906` was built using the retained
Renewal configuration and **PACKETVER20260219**, `make -j4 server`, in
`rathena:local`. `pc.cpp` was the only recompiled source; all four server link
steps ran, and login, character and web remained byte-identical. No configure-default
packet version or broad source synchronization was used.

The following existing live differences were preserved byte-for-byte before,
after, and in rollback checks:

| Existing file | SHA256 |
| --- | --- |
| `db/import/item_db.yml` | `20946df000e0fba0710843bfd676dc895818e96fac6a0f3e2296e04cb39a287b` |
| `db/import/pet_db.yml` | `8c70d5f5b70b905082f3431fc0990ea76c736fb3c4b18713e899764d5297c00a` |
| `src/map/skills/npc/suicidebombing.cpp` | `80e5ebd92e5eec5e26eec2f3bfb62cb61f894cd8c94bbed097545b7ee7ec4164` |

The Suicide Bombing difference remains an **unbuilt source change**, not part
of this candidate binary. Source-manifest equivalence does not imply otherwise.

## Verification before installation

- Strict episode integrity: 900 enabled NPC scripts, 110 database imports,
  71 wired runtime fragments, 99 reachable instances and 56 route assertions;
  no integrity errors or content-completeness warnings.
- Dedicated native regression: fixed 103 cases / 8,097 assertions; genuine
  separately compiled old engine 101 / 6,659 reproduces stale registrations,
  including all 15 Ellie exchanges. A separate old bounds child produces the
  exact expected index-200 UBSan diagnostic; fixed invalid indices reject
  cleanly. Both normal processes have clean sanitizer and allocator results.
- Crown regression with fresh patched `pc.cpp`: 4,152 cases / 200,773 assertions,
  including its original-NPC failure. Conversion regression with fresh patched
  `pc.cpp`: 225 / 49,497, plus 8 / 177 old-loss controls and 2 / 51 getter controls.
  Unchanged retained sanitizer objects were reused only with matching source
  hashes. These are bounded native fixtures, not full-world gameplay tests.
- Broad callback review manually updated only the two engine section pins;
  database/NPC/closure pins did not change. All 17 default and 21 explicit-live
  rejection controls passed. The separate conversion gate rejected all nine
  scoped changes. Seven zero-exit diagnostic controls passed in each applicable
  native output checker.
- Both candidate `--run-once` startup variants passed: project data and the
  actual preserved live item/pet overlays. Each used the three actual server
  configuration bindings, initialized 3,610 NPCs, and finalized without memory
  leaks. Only the existing root warning appeared.

Generated native receipts are outside Git:

| Receipt | SHA256 |
| --- | --- |
| `../equipswitch-deletion-native-final-20260906/receipt.json` | `f10154c4eb97c111a071b920f9ed2c03668f269f74279b177d02aa05ab72055f` |
| `../equipswitch-crown-regression-20260906/receipt.json` | `c0d924af2c38451af72a10c396cf0e625d58e38762817db24a40561918ed8b42` |
| `../equipswitch-conversion-regression-20260906/receipt.json` | `b0832b91cb2e70036643b935f5f30f5193b1d49da72c8c00894db0fa6cff4840` |

## Quiescence, backups and actual post-state

The deployment first checked exact old binaries/source, the candidate manifest,
and the full expected live-after callback state. It stopped login, required zero
online characters, then stopped map/character/web and required zero persisted
`bonus_script` rows before copying. A database dump and an exact old `pc.cpp` +
map-binary archive were retained. No SQL records were deliberately changed.

Backup directory:
`/app/rathena-deploy-backups/equipswitch-artifacts-20260906/`.

| Backup | SHA256 |
| --- | --- |
| `pre-runtime-and-map.tar.gz` | `96bfc1b11e57b9fed5e8e658f8882d918c2d43d9a22d12ef041a3878d40be578` |
| `pre-runtime-and-map.sql` | `45ec199094426fdd6df985b9ed7a24f6a74c9611522d7ce32a1f07a8a5821431` |

Rollback requires all four services actually stopped before restoring only the
two backed-up targets; old hashes and the three preserved differences must pass
before restart. A stop or restore-verification failure does not restart into an
unverified state. No automatic SQL restore is performed. Rollback was not needed.

After installation, the actual live tree passed the explicit-live full 3,688-file
manifest and conversion callback gate before startup. Subsequent read-only SQL
checks again returned zero online characters and zero persisted bonus scripts.
Login, character and web binary hashes remained respectively
`4c6e67a12b1c9e89b96eeec8e1d62adfbcd5f7ffee6fb16fa65b4b6dd7ab868d`,
`6d07b1170ad144d1e3d7718e39f49868c22fa061df323c15d418366fe295aab4`, and
`2e554707bf580b09a4d39d4c47d3d38acc9273903b0ef074dbd72f1cd5487f40`.

## Reproducible artifact identities

The local sibling `../equipswitch-artifacts-20260906/` contains nonsecret
manifests and copied build/startup/deployment logs. Runtime archives, binaries,
database dumps, credentials and client archives are not staged in Git.

| Artifact | SHA256 |
| --- | --- |
| `runtime.tar.gz` | `159a95b490464e45284b5fef5b4d8322488e4f1576b1aa5d8909ae0aab9dd6e0` |
| `runtime-manifest.json` | `d19b525a3df33c93f7a565790b587d72e1e0d6de8d8d99f03821bdec83e06705` |
| `expected-live-callback-manifest.json` | `e38b4d1bf6f7c00ad49c5436468670b272f52af2481173745454e6e22eab0178` |
| `equipswitch-deploy-20260906.sh` | `d07be121f5db7a400f2a3eafe9cca8a39b1c41e2e89e6cdec498a4130b5d8713` |
| `deployment.log` | `bda9a50e895c066fbdda97f9b71c54c25bf2b6b6022b9b905baf7c4ab3c4e2fd` |
| `build.log` | `8ae2fcafcc0a76147f54261572119524d247185e5dad431b90855dca56bea465` |
| `startup-project.log` | `3149ea5f84e594b5cf1c59daf61f2f42309ba9e996b11aef749d93d6c28bffb8` |
| `startup-preserved-live.log` | `fe623bb24865fa3d2d3d9004e3556aaa80d5634a73fcee3aed9ba0ca36bd4d1d` |

Deferred material transaction and Final Battle findings remain explicitly
unimplemented in their separate audit documents. This batch does not establish
that every episode, quest, item or class interaction is correct.
