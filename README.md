# rAthena PN — Renewal Server Build

A customized Ragnarok Online Renewal server built on rAthena, bringing together expanded class support, endgame content, equipment progression, and coordinated client compatibility work.

This repository contains our server source, custom databases and NPC scripts, client patch tooling, and engineering documentation. It is a customization of rAthena, not an official upstream release or a complete game-client distribution.

[Features](#features) · [Player services](#player-services) · [Getting started](#getting-started) · [Docker build](#docker-build) · [Client compatibility](#client-compatibility) · [Validation](#validation) · [Operations](#operations) · [Documentation](#documentation)

## Project at a glance

| Area | Repository baseline |
| --- | --- |
| Game rules | Customized Renewal with fourth-job and Druid integration |
| Server processes | Login, character, map, and web |
| Docker build | Alpine Linux; four server binaries compiled together |
| PN packet baseline | `20260219`, explicitly selected by Docker configuration and CI |
| Content configuration | Enabled NPC scripts plus tracked custom database imports |
| Client delivery | Scoped patch sources and tools; executable and complete GRFs are separate |
| Verification | Compilation, integrity audits, focused native tests, documented deployment checks |

The client baseline is a build setting. Match the intended executable, packet
version, client resources, and server revision for each release.

## Features

### Classes and combat

- Renewal and fourth-job systems, with custom combat and equipment integration.
- Druid → Karnos → Alitea progression, including skills, transformations, job handling, equipment eligibility, and progression services.
- Druid-specific equipment, crowns, acquisition routes, and shadow enchant support.

See the [Druid integration notes](doc/druid_integration.md) and [client/progression coverage](doc/druid_client_progression.md) for provenance and implementation limits. Server implementation does not imply complete client rendering or gameplay certification.

### World and progression

- Custom episode progression and access services, with targeted quest, reward, re-entry, and party-progression fixes.
- Varmundt Biosphere services and Depth 2 access, plus Zero Cell monster and reward definitions.
- Grademk equipment services, healer-related fixes, and corrected travel entry points.
- Reputation initialization on login and map changes, with supporting deployment notes.

### Equipment and enchantments

- Native enchant-window integration for supported equipment groups.
- Grade Workshop services for equipment families including Constellation, seasonal gear, Frontier, Time Dimensions, and Biosphere.
- Ordinary and guaranteed upgrade handling, with recipe and client metadata audits.
- Inventory-preserving enchant changes and targeted checks for eligibility, material costs, capacity, and stale item state.

### Engineering and operations

- Docker build and runtime definitions for database and server services.
- Scoped client patch generators and installers with compatibility documentation.
- Source-level audits, native regression tests, and isolated startup validation tooling.
- Deployment records covering verification evidence, rollback preparation, and remaining checks.

## Player services

Locations below come from the enabled custom NPC scripts. GM accounts can use
`@warp <map> <x> <y>` when permitted.

| Service | Location | Function |
| --- | --- | --- |
| Main Office | `@office` / `pn_office,100,40` | Searchable directory across 52 lobby, training and fashion desks |
| Player settings | `@settings` / Office lobby | Save character or game-account login preferences with immediate application |
| Loot presets | `@alc save 1 Farming`, `@als 1` | Ten named game-account sets for autoloot rate, included items and types |
| Kill counter | `@kc 1002 1`, `@kc status` | Five persistent character slots; `@kc reset [slot]` clears them |
| Grade Enhancer | Office training floor / `grademk,34,184` | Native grading and Etel exchange with existing costs and risk options |
| Rune Tablet | Office training floor and `grademk,46,178` | Account collection/rewards; character tablets and enhancement |
| Battle statistics | `@battlestats` / `@bs`, `@battlestats2` / `@bs2` | Offensive and defensive snapshots with detailed, paginated modifiers |
| PN Services | `izlude,140,146` and `grademk,46,180` | Damage lab, access diagnostics, navigation |
| Skill Supplies | `izlude,137,150` and `grademk,42,180` | Consumables required to use skills |
| Reset Girl | `prontera,150,193` | Skills: 5,000 zeny; stats: 5,000 zeny; both: 9,000 zeny |
| Wise Old Woman — Card Remover | `prt_in,28,73` | Remove cards from equipped items |
| Druid Mentor | `prontera,153,193` | Custom Druid → Karnos → Alitea progression |

PN Services and Skill Supplies also have placements on the configured Izlude
variants. The reset and card removal NPCs are enabled through
[`npc/scripts_custom.conf`](npc/scripts_custom.conf).

The [Main Office guide](doc/main_office.md) covers its three maps, client patch,
service behavior and rollout. Rune Tablet uses NPC menus with account-shared
piece unlocks and one active character tablet; see
[transactions and persistence](doc/rune_tablet_transactions.md) and
[bonus implementation](doc/pn_rune_tablet_bonus_notes.md).
Use `@bs help`, `@bs race`, `@bs casting` or `@bs2 element` to inspect a build;
the [battle-stat guide](doc/pn_battlestats.md) explains the values and limits.

**Card removal terms:** 200,000 zeny plus 25,000 per card, one Star Crumb, and one
Yellow Gemstone. Current failure outcomes can destroy cards, equipment, or both.
Read the confirmation dialogue before proceeding.

**Damage lab:** the Poring is an intentional training dummy. The service creates
a private 30-minute instance and measures approximately 60 seconds of HP loss.
Configure target properties and repeat comparable runs; the normal-class dummy
does not reproduce boss AI or boss-only effects. See [PN Services](doc/quality_services.md)
for measurement limits and [the instance crash repair](doc/pn_lab_crash_fix_20260907.md)
for lifecycle validation.

The [player command guide](doc/player_commands_reference_audit_20260908.md) lists
available commands, exact aliases and compatibility limits. Use `@commands` and
`@help <command>` for native command discovery. Saved Settings cover autoloot,
EXP/zeny messages, skill-delay messages and invitation rejection. Character
overrides take priority over game-account preferences.

See the [element audit](doc/element_system_audit_20260908.md),
[refinement audit](doc/refine_system_audit_20260908.md) and
[grading audit](doc/grade_system_audit_20260908.md) for formulas, confirmed fixes
and test boundaries. Native refine/grade windows now require unequipped items
that are also removed from equipment-switch registration.

## Repository layout

| Path | Purpose |
| --- | --- |
| [src/](src/) | Server engine, networking, combat, and scripting |
| [db/import/](db/import/) | Custom database definitions and overrides |
| [npc/custom/](npc/custom/) | Custom NPCs, services, and progression scripts |
| [conf/](conf/) | Server configuration and import structure |
| [sql-files/](sql-files/) | Database schemas and upgrade scripts |
| [client-patch/](client-patch/) | Client compatibility patches and tooling |
| [tools/docker/](tools/docker/) | Container definitions and build helpers |
| [tools/ci/](tools/ci/) | Audits, regression tests, and validation utilities |
| [doc/](doc/) | Technical references and deployment records |

## Getting started

Treat a fresh checkout as a development environment first. The supplied Docker configuration is a starting point, **not a hardened production deployment**.

1. Review the [Docker setup](tools/docker/README.md), [configuration guide](conf/readme.md), and [database notes](sql-files/README.md).
2. Prepare an isolated database and local configuration. Replace example credentials, set correct advertised addresses, and restrict database access before exposing services.
3. Select the client executable and matching packet version before compiling. Do not assume the sample Docker packet version matches the deployed client.
4. Build compatible login, character, and map binaries together. Shared structure changes require coordinated rebuilds; mixing older character binaries with newer map binaries can prevent login.
5. Apply the required client patches and run relevant audits and isolated startup checks before admitting players.

Use the [login/character compatibility repair](doc/login_character_abi_repair_20260907.md) and [Druid deployment requirements](doc/druid_integration.md#deployment-requirement) as references when changing shared server structures.

## Docker build

Use a separate development checkout: compilation writes binaries into the mounted
repository. From the repository root in Bash or WSL:

```sh
docker build -t rathena-pn-build:local tools/docker
docker run --rm --network none \
  -v "$PWD:/rathena" \
  -e BUILDER_CONFIGURE=--enable-packetver=20260219 \
  -e BUILDER_FORCE_BUILD=1 -e BUILD_JOBS=2 \
  rathena-pn-build:local sh tools/docker/builder.sh
```

This compiles `login-server`, `char-server`, `map-server`, and `web-server` without
starting services or accessing a database. Building the image alone installs the
toolchain. Linux outputs require the matching Alpine runtime.

See the [Docker guide](tools/docker/README.md) for PowerShell commands, Compose
startup, packet overrides, database initialization, and troubleshooting.
The [PN Docker workflow](.github/workflows/build_servers_docker.yml) compiles
relevant changes pushed to `main` and retains binaries with checksums as CI
artifacts; it does not deploy them.

## Operations

### Release checklist

- Back up the database, configuration, and previous binaries before changes.
- Keep credentials, account data, and private deployment details out of commits.
- Review schema upgrades individually; do not re-import initialization SQL into an existing live database.
- Validate in an isolated candidate environment, then coordinate service restarts during maintenance.
- Inspect logs and test login, character loading, map travel, and changed gameplay after deployment. A running container alone does not establish a successful release.

Record the source commit, configure flags, binary checksums, script/DB changes,
and client archive order for each release. Restore-test database backups in
isolation and retain a matching previous set of binaries for rollback.

For script changes, preserve live overrides and apply a reviewed file delta.
Schedule reloads or restarts with online players and active instances in mind.
Shared engine changes require matching rebuilt server binaries. SQL migrations
need a separate backup and recovery procedure.

### Common operational checks

| Symptom | First checks |
| --- | --- |
| Character selection disconnects | Matching login/char/map builds, packet version, advertised addresses |
| Missing custom NPC | Enabled script, live file, map name, fresh startup errors |
| Poring in the damage lab | Expected appearance; start measurement through the service |
| Missing map or sprite | Active GRF order, map assets, sprite mappings, compatibility patch |
| Missing enchant options | Client metadata, target IDs, server import, supported packet path |
| Unexpected reward result | Quest state, capacity, item identity, relevant script logs |

## Client compatibility

Server data and client resources must be released as a matched set. Adding an item, job, map, or enchant recipe on the server does not automatically supply its client metadata, sprites, or interface support.

Start with the relevant package:

- [Druid item patch](client-patch/druid_items/README.md) and [compatibility report](doc/druid_item_compatibility.md)
- [Chapter 2 native enchant integration](client-patch/chapter2_native/README.md)
- [Biosphere patch](client-patch/biosphere/README.md)
- [Zero Cell patch](client-patch/zero_cell/README.md)
- [Enchant target metadata](client-patch/enchant_target_metadata/README.md)

Follow each package's prerequisites and installation instructions. Some tools generate review artifacts only; generation is not installation. Preserve the intended GRF load order and test the actual client executable. External reference archives are not automatically authorized for redistribution.

## Validation

Run checks from the repository root. Python audits may require PyYAML; native tests and client-resource tests have additional dependencies documented alongside their runners.

For example, run the strict episode integrity audit through PowerShell:

```powershell
powershell -NoProfile -File tools/audit_episode_integrity.ps1 -StrictContent
```

Choose regression tests for the area being changed rather than treating one audit as a full release gate. See [Druid reproducible checks](doc/druid_integration.md#reproducible-checks), [native script VM tests](tools/ci/native_script_vm_README.md), and [enchant protocol evidence](doc/enchant_upgrade_protocol.md).

**Verification scope:** source checks and clean startup logs are not substitutes for end-to-end playthroughs. Episode encounters, class behavior, reward flows, and client interactions have separate coverage limits. Consult the dated [episode audit status](doc/episode_audit_status.md) and feature-specific reports for evidence; historical deployment records are not a live health dashboard.

## Documentation

- [2026-09-08 service release, validation and rollback](doc/pn_services_release_20260908.md)
- [Docker build and development](tools/docker/README.md)
- [PN Services and damage lab](doc/quality_services.md)
- [Main Office and client installation](doc/main_office.md)
- [Battle-stat commands](doc/pn_battlestats.md)
- [Rune Tablet transactions](doc/rune_tablet_transactions.md) and [bonuses](doc/pn_rune_tablet_bonus_notes.md)
- [Chapter 2 reference audit](doc/chapter2_reference_audit_20260908.md) and [progression repairs](doc/chapter2_runtime_audit_20260908.md)
- [Shadow Gear coverage audit](doc/shadow_gear_reference_audit_20260908.md)
- [Reset and card removal activation](doc/reset_services_deployment_20260908.md)
- [Grademk equipment services](doc/grademk_equipment_service_audit.md)
- [Druid gear and enchants](doc/druid_gear_enchants_audit.md)
- [Chapter 2 client coverage](doc/chapter2_native_client_coverage.md)
- [Reputation, Constellation travel, and Depth 2 repairs](doc/reputation_login_and_go55_repair_20260907.md)
- [Script commands](doc/script_commands.txt), [item bonuses](doc/item_bonus.txt), and [GM commands](doc/atcommands.txt)

## Contributing

Keep changes scoped and preserve unrelated customizations. Include affected server definitions, client requirements, regression checks, and deployment or rollback notes where applicable. Report what was tested and what still requires in-game verification. Follow the [contribution guidelines](.github/CONTRIBUTING.md) for upstream conventions.

## License

Based on rAthena, with credit to the rAthena Development Team, the eAthena project, and their contributors. Original copyright and attribution notices are retained in the source.

The server source is distributed under the [GNU General Public License v3.0](LICENSE). Third-party components retain their respective licenses. This server-source license does not grant rights to redistribute Ragnarok Online client assets or third-party GRF archives.

## PN script signatures

Project scripts include PN contribution, license and source notices. See [the source signature guide](doc/script_licensing.md) and [client companion notices](client-patch/SOURCE-NOTICES.md).
