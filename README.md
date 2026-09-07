# rAthena PN — Renewal Server Build

A customized Ragnarok Online Renewal server built on rAthena, bringing together expanded class support, endgame content, equipment progression, and coordinated client compatibility work.

This repository contains our server source, custom databases and NPC scripts, client patch tooling, and engineering documentation. It is a customization of rAthena, not an official upstream release or a complete game-client distribution.

[Features](#features) · [Getting started](#getting-started) · [Client compatibility](#client-compatibility) · [Validation](#validation) · [Documentation](#documentation) · [License](#license)

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

### Deployment discipline

- Back up the database, configuration, and previous binaries before changes.
- Keep credentials, account data, and private deployment details out of commits.
- Review schema upgrades individually; do not re-import initialization SQL into an existing live database.
- Validate in an isolated candidate environment, then coordinate service restarts during maintenance.
- Inspect logs and test login, character loading, map travel, and changed gameplay after deployment. A running container alone does not establish a successful release.

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
