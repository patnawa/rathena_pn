#!/bin/sh
# ============================================================================
#  PN  /  DEVELOPMENT TOOLS
#  builder.sh
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/docker/builder.sh
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

set -eu

cd /rathena
force_build=${BUILDER_FORCE_BUILD:-0}
build_jobs=${BUILD_JOBS:-2}
case "$force_build" in
  0|1) ;;
  *) echo "BUILDER_FORCE_BUILD must be 0 or 1" >&2; exit 2 ;;
esac
case "$build_jobs" in
  ''|*[!0-9]*|0) echo "BUILD_JOBS must be a positive integer" >&2; exit 2 ;;
esac

if [ "$force_build" = 0 ] && [ -x login-server ] && [ -x char-server ] \
    && [ -x map-server ] && [ -x web-server ]; then
  echo "Server binaries already exist. Set BUILDER_FORCE_BUILD=1 after source or packet-version changes."
  exit 0
fi

: "${BUILDER_CONFIGURE:?Set BUILDER_CONFIGURE, including --enable-packetver=YYYYMMDD}"
# Configure flags are space-separated arguments, never evaluated as shell code.
set -f
./configure $BUILDER_CONFIGURE
# Complete cleanup before parallel compilation to avoid clean/build races.
make clean
make -j"$build_jobs" server
for server_binary in login-server char-server map-server web-server; do
  test -x "$server_binary"
done
echo "Built login, character, map and web servers successfully."
