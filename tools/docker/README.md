# Docker build and development guide

The Dockerfile supplies an Alpine Linux toolchain and runtime libraries. Server
source and binaries live in a mounted checkout at `/rathena`; building the image
alone does not compile rAthena. Compose adds MariaDB and the login, character,
and map services for local development.

## Requirements

- Docker Engine or Docker Desktop with Linux containers and Docker Compose v2.
- A complete checkout, including its tracked custom database imports.
- Disk space for the image, compiler outputs, and database volume.
- Memory for C++ compilation. Start with two build jobs; use one if memory is limited.

Build in a separate development checkout. The builder writes binaries into its
mounted directory, so mounting a running server directory would replace its files.

## Compile the server

From the repository root in Bash, WSL, or a Linux terminal:

```sh
docker build -t rathena-pn-build:local tools/docker
docker run --rm --network none \
  -v "$PWD:/rathena" \
  -e BUILDER_CONFIGURE=--enable-packetver=20260219 \
  -e BUILDER_FORCE_BUILD=1 -e BUILD_JOBS=2 \
  rathena-pn-build:local sh tools/docker/builder.sh
```

PowerShell equivalent, from the repository root:

```powershell
docker build -t rathena-pn-build:local tools/docker
docker run --rm --network none `
  --mount "type=bind,source=$($PWD.Path),target=/rathena" `
  -e BUILDER_CONFIGURE=--enable-packetver=20260219 `
  -e BUILDER_FORCE_BUILD=1 -e BUILD_JOBS=2 `
  rathena-pn-build:local sh tools/docker/builder.sh
```

Image construction downloads dependencies; compilation needs no network or
database. A successful build produces `login-server`, `char-server`, `map-server`,
and `web-server` together. These Linux binaries link against the Alpine runtime;
execute them in the matching container environment.

| Variable | Default | Purpose |
| --- | --- | --- |
| `BUILDER_CONFIGURE` | Compose: `--enable-packetver=20260219`; required for direct builds | Space-separated configure arguments |
| `BUILDER_FORCE_BUILD` | `0` | Set to `1` to reconfigure, clean, and rebuild all servers |
| `BUILD_JOBS` | `2` | Concurrent compiler jobs |

The February 19, 2026 packet version matches this fork's documented PN baseline.
Choose the date required by your executable. The core source retains an upstream
default; explicitly select your intended packet version. When all binaries exist,
the builder skips compilation unless `BUILDER_FORCE_BUILD=1`. Use that flag after
source or packet-version changes.

Additional options can be passed as a single environment argument, for example
`-e 'BUILDER_CONFIGURE=--enable-packetver=20260219 --enable-debug'`.

## Start a local development stack

The supplied Compose configuration publishes database port 3306 and uses example
credentials. Review these settings before starting it on a networked machine.
Use an isolated development database; replace credentials for shared environments
and keep private overrides outside Git.

From `tools/docker`:

```sh
docker compose config --quiet
docker compose build builder
docker compose run --rm --no-deps -e BUILDER_FORCE_BUILD=1 builder
docker compose up -d db login char map
docker compose ps
docker compose logs --tail=100 login char map
```

Compose uses the image tag `rathena:local`, separate from the build-only tag above.
Do not start this sample stack where its fixed `rathena-*` container names are
already in use.

### Configuration and database initialization

| File or volume | Behavior |
| --- | --- |
| `asset/inter_conf.txt` | Overrides `conf/import/inter_conf.txt`; database connection settings |
| `asset/char_conf.txt` | Overrides character configuration; login connection and advertised address |
| `asset/map_conf.txt` | Overrides map configuration; character connection and advertised address |
| `rathenadb` volume | Persists MariaDB data across container recreation |
| `sql-files/` initialization mount | MariaDB processes SQL on first start with an empty data volume |

Review initialization SQL before creating the volume. See the
[SQL guide](../../sql-files/README.md). Existing databases require individual
migrations; restarting does not apply new schema files automatically. Changing
environment passwords does not change existing database users.

Set advertised character/map addresses to an address reachable by the game client.
Docker names such as `db` and `char` are internal service addresses. Loopback is
appropriate only when the game client connects on the same host.

The build produces `web-server`, but this Compose file does not define a web
service, FluxCP, or a reverse proxy. Configure those separately when needed.

### Stop and inspect

```sh
docker compose logs --tail=200 map
docker compose stop
docker compose down
```

`down` removes containers and the network while retaining the named database
volume. Adding `--volumes` deletes that volume; reserve it for intentionally
discarding a development database.

## Validation and CI

[Build PN servers with Docker](../../.github/workflows/build_servers_docker.yml)
runs on relevant pushes to `main`, pull requests, and manual dispatch. It builds
the toolchain image, compiles all four servers with `PACKETVER=20260219`, and
uploads binaries and SHA-256 checksums for seven days. It does not deploy a server
or publish a container image. Downloaded binaries may need executable permissions
restored before use.

Compilation does not prove startup or gameplay. Run the strict episode integrity
audit and validate startup against an isolated database before release. Player
acceptance should cover login, character loading, NPC interaction, map travel,
and changed instance/reward flows.

## Troubleshooting

| Symptom | Check |
| --- | --- |
| Builder skips compilation | Set `BUILDER_FORCE_BUILD=1` |
| Compiler is killed | Reduce `BUILD_JOBS`; inspect host and Docker memory limits |
| Character selection disconnects | Check packet version and matching login/char/map builds |
| Port or container name already used | Inspect existing services before starting Compose |
| Missing NPC | Check enabled imports, deployed scripts, map, and startup errors |
| Client cannot enter a map | Check advertised address, packet version, map cache, and client assets |
| Bind-mount permission denied | Check ownership, Docker file sharing, and applicable SELinux labels |

Docker references: [Compose quickstart](https://docs.docker.com/compose/gettingstarted/)
and [build documentation](https://docs.docker.com/build/).
