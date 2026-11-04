#!/bin/bash
#
# build_vmc_test_on_server.sh - compile vmc_test ON THE ATE SERVER and fetch
# the binary back (same idea as prepare_release.sh does for dpdk_vmc).
#
# Called automatically by CMake (target: vmc_test). Can also be run by hand:
#   scripts/build_vmc_test_on_server.sh <vmc_test_src_dir> <out_dir> <ServerConfig.h>
#
#   1. copy vmc_test sources -> <SERVER_REMOTE_DIR>/vmc_test_build
#   2. make clean && make     (on the server)
#   3. fetch the binary       -> <out_dir>/vmc_test  (+ VERSION.txt)
#
set -euo pipefail

if [ $# -ne 3 ]; then
    echo "Usage: $0 <vmc_test_src_dir> <out_dir> <ServerConfig.h>" >&2
    exit 2
fi

SRC_DIR="$1"
OUT_DIR="$2"
CONFIG_H="$3"

log()  { echo "[vmc_test] $*"; }
fail() { echo "[vmc_test][ERROR] $*" >&2; exit 1; }

read_define() {
    grep -E "^[[:space:]]*#define[[:space:]]+$1[[:space:]]" "$CONFIG_H" \
        | head -n1 | sed -E 's/.*"([^"]*)".*/\1/'
}

[ -d "$SRC_DIR" ]  || fail "source dir not found: $SRC_DIR"
[ -f "$CONFIG_H" ] || fail "config header not found: $CONFIG_H"
command -v sshpass >/dev/null || fail "sshpass not found (sudo apt install sshpass)"

SERVER_HOST="$(read_define SERVER_HOST)"
SERVER_USER="$(read_define SERVER_USER)"
SERVER_PASS="$(read_define SERVER_PASSWORD)"
SERVER_DIR="$(read_define SERVER_REMOTE_DIR)"
APP="$(read_define VMC_TEST_APP_NAME)"
[ -n "$SERVER_HOST" ] && [ -n "$SERVER_USER" ] && [ -n "$SERVER_DIR" ] && [ -n "$APP" ] \
    || fail "could not read server settings from $CONFIG_H"

REMOTE_BUILD_DIR="$SERVER_DIR/vmc_test_build"
TARGET="$SERVER_USER@$SERVER_HOST"
SSH_OPTS=(-o StrictHostKeyChecking=no -o ConnectTimeout=10)

ssh_exec() { sshpass -p "$SERVER_PASS" ssh -n "${SSH_OPTS[@]}" "$TARGET" "$1"; }

log "Server: $TARGET  build dir: $REMOTE_BUILD_DIR"

log "[1/4] Testing connection..."
ssh_exec "echo ok" >/dev/null || fail "cannot connect to server $SERVER_HOST"

log "[2/4] Copying sources to server..."
# tar stream: copies the whole tree in one go and leaves local build
# artifacts behind.
ssh_exec "rm -rf '$REMOTE_BUILD_DIR'"
tar -C "$SRC_DIR" --exclude="./$APP" --exclude="./$APP-debug" -cf - . \
    | sshpass -p "$SERVER_PASS" ssh "${SSH_OPTS[@]}" "$TARGET" \
        "mkdir -p '$REMOTE_BUILD_DIR' && tar -C '$REMOTE_BUILD_DIR' -xf -" \
    || fail "copying sources failed"

log "[3/4] Building on server..."
ssh_exec "cd '$REMOTE_BUILD_DIR' && (make clean >/dev/null 2>&1 || true) && make -j\$(nproc)" \
    || fail "build on server failed"

log "[4/4] Fetching binary..."
mkdir -p "$OUT_DIR"
sshpass -p "$SERVER_PASS" scp "${SSH_OPTS[@]}" \
    "$TARGET:$REMOTE_BUILD_DIR/$APP" "$OUT_DIR/$APP.tmp" \
    || fail "fetching binary failed"
chmod +x "$OUT_DIR/$APP.tmp"
mv -f "$OUT_DIR/$APP.tmp" "$OUT_DIR/$APP"

VERSION="$(grep -E '#define[[:space:]]+Version[[:space:]]' "$SRC_DIR/include/Version.h" \
    | head -n1 | sed -E 's/.*"([^"]*)".*/\1/')"
echo "${VERSION:-0.0.0}" > "$OUT_DIR/VERSION.txt"

log "Done: $OUT_DIR/$APP (v${VERSION:-0.0.0})"
