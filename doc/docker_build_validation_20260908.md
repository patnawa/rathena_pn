# PN Docker build validation — 2026-09-08

Built the repository source in a separate checkout using the documented Alpine
Dockerfile and updated `tools/docker/builder.sh`.

| Check | Result |
| --- | --- |
| Toolchain image | Built successfully as `rathena-pn-build:20260908` |
| Configure | `--enable-packetver=20260219` |
| Clean compilation | All four server binaries produced; exit code 0 |
| Isolation | No container network, no database mount, separate source directory |
| Resource limits | Two CPUs, two compiler jobs, 4 GiB memory limit |
| Compose configuration | Parsed successfully |
| Builder shell syntax | Passed `sh -n` |
| Documentation | Relative file links resolved |

Build artifacts and checksums are in `/app/pn-docker-build-20260908/`.
The complete image/build log is `/app/pn-docker-build-20260908.log`.

```text
4c6e67a12b1c9e89b96eeec8e1d62adfbcd5f7ffee6fb16fa65b4b6dd7ab868d  login-server
d1b37f45c18efc5ed776b9f9163c14a96106b0daac3a134297e0f90e0a09a284  char-server
fd49e3fca2c99c3e5d9e30a696ceda58e927120e4bfc70e57148730411a12943  map-server
2e554707bf580b09a4d39d4c47d3d38acc9273903b0ef074dbd72f1cd5487f40  web-server
```

These are compilation artifacts, not a deployed engine release or a successful
database-backed startup certification. A separate `--version` probe reached the
web server's database initialization and failed because the build container has
no database; it is not a usable four-process health check. No generated binary
replaced a running production binary.

The added GitHub workflow performs the same image and compilation steps on
relevant changes to `main`, with downloadable binaries and checksums. Its status
must be checked independently in GitHub Actions.
