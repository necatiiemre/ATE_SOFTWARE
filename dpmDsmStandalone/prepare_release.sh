#!/bin/bash
#
# prepare_release.sh - build everything for dpmDsmStandalone and collect the
# ready-to-run binaries into Execute/ (same idea as the repository's
# prepare_release.sh / MMUComputerTestSoftware).
#
#   1. cmake configure + build of the PC app (dpmDsmStandalone)
#   2. dpm_dsm_test is copied to the ATE server, built there, binary fetched back
#      (done by the CMake build - scripts/build_dpm_dsm_test_on_server.sh)
#   3. everything needed at run time is copied to Execute/
#
# Afterwards:
#   cd Execute && ./dpmDsmStandalone
#
# Usage:
#   ./prepare_release.sh                  # build + collect into Execute/
#   ./prepare_release.sh --clean          # wipe build/ first (full rebuild, dpm_dsm_test too)
#   ./prepare_release.sh --skip-dpm-dsm-test  # don't build dpm_dsm_test on the server
#                                         # (keeps the dpm_dsm_test already in Execute/)
#   ./prepare_release.sh -h               # help
#
# Execute/LOGS/ (run logs) is never touched.
#
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
EXEC_DIR="$SCRIPT_DIR/Execute"

RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; NC='\033[0m'
log_info()  { echo -e "${GREEN}[INFO]${NC} $1"; }
log_warn()  { echo -e "${YELLOW}[WARN]${NC} $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1" >&2; }
log_step()  { echo ""; echo "========================================"; echo -e "${GREEN}$1${NC}"; echo "========================================"; }

usage() { sed -n '2,25p' "$0" | sed 's/^# \{0,1\}//'; }

CLEAN=0
SKIP_TEST=0
for arg in "$@"; do
    case "$arg" in
        --clean)         CLEAN=1 ;;
        --skip-dpm-dsm-test) SKIP_TEST=1 ;;
        -h|--help)       usage; exit 0 ;;
        *) log_error "Unknown option: $arg"; usage; exit 2 ;;
    esac
done

read_version() {   # $1 = Version.h
    grep -E '#define[[:space:]]+Version[[:space:]]' "$1" | head -n1 | sed -E 's/.*"([^"]*)".*/\1/'
}

# ==================== 0. Tools ====================
log_step "Checking tools"
missing=0
tools=(cmake make g++)
[ "$SKIP_TEST" -eq 0 ] && tools+=(sshpass ssh scp tar)
for t in "${tools[@]}"; do
    if command -v "$t" >/dev/null 2>&1; then
        log_info "$t: OK"
    else
        log_error "$t not found"
        missing=1
    fi
done
if [ "$missing" -ne 0 ]; then
    log_error "Install the missing tools, e.g.: sudo apt install build-essential cmake sshpass openssh-client"
    exit 1
fi

# ==================== 1-2. Build ====================
if [ "$CLEAN" -eq 1 ]; then
    log_step "Cleaning build directory"
    rm -rf "$BUILD_DIR"
    log_info "Removed $BUILD_DIR"
fi

log_step "Configuring (cmake)"
if [ "$SKIP_TEST" -eq 1 ]; then SERVER_BUILD=OFF; else SERVER_BUILD=ON; fi
cmake -S "$SCRIPT_DIR" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release -DDPM_DSM_TEST_SERVER_BUILD="$SERVER_BUILD"

log_step "Building dpmDsmStandalone$( [ "$SKIP_TEST" -eq 0 ] && echo ' + dpm_dsm_test (on the server)')"
if ! cmake --build "$BUILD_DIR" -- -j"$(nproc)"; then
    log_error "Build failed - Execute/ was NOT changed."
    [ "$SKIP_TEST" -eq 0 ] && log_error "If the server is unreachable, build only the PC app with: ./prepare_release.sh --skip-dpm-dsm-test"
    exit 1
fi

BIN_DIR="$BUILD_DIR/bin"
[ -x "$BIN_DIR/dpmDsmStandalone" ] || { log_error "dpmDsmStandalone was not built"; exit 1; }
if [ "$SKIP_TEST" -eq 0 ] && [ ! -x "$BIN_DIR/dpm_dsm_test/dpm_dsm_test" ]; then
    log_error "dpm_dsm_test binary was not fetched from the server"
    exit 1
fi

# ==================== 3. Collect into Execute/ ====================
log_step "Collecting binaries into Execute/"
mkdir -p "$EXEC_DIR"

# dpmDsmStandalone + its interfaces file: always replaced.
rm -rf "$EXEC_DIR/dpmDsmStandalone" "$EXEC_DIR/CumulusInterfaces"
cp -a "$BIN_DIR/dpmDsmStandalone" "$EXEC_DIR/"
cp -a "$BIN_DIR/CumulusInterfaces" "$EXEC_DIR/"
log_info "dpmDsmStandalone + CumulusInterfaces/{DPM,DSM}/interfaces"

# dpm_dsm_test: replaced only when it was (re)built in this run.
if [ "$SKIP_TEST" -eq 0 ]; then
    rm -rf "$EXEC_DIR/dpm_dsm_test"
    cp -a "$BIN_DIR/dpm_dsm_test" "$EXEC_DIR/"
    log_info "dpm_dsm_test/dpm_dsm_test (built on the server)"
elif [ -x "$EXEC_DIR/dpm_dsm_test/dpm_dsm_test" ]; then
    log_warn "dpm_dsm_test not rebuilt - keeping the existing Execute/dpm_dsm_test (v$(cat "$EXEC_DIR/dpm_dsm_test/VERSION.txt" 2>/dev/null || echo '?'))"
else
    log_warn "dpm_dsm_test not built and none in Execute/ - step 2 will fail until you run without --skip-dpm-dsm-test"
fi

APP_VERSION="$(read_version "$SCRIPT_DIR/include/Version.h")"
echo "$APP_VERSION" > "$EXEC_DIR/VERSION.txt"

log_step "Release ready: $EXEC_DIR"
echo "  dpmDsmStandalone        v$APP_VERSION"
echo "  dpm_dsm_test             v$(cat "$EXEC_DIR/dpm_dsm_test/VERSION.txt" 2>/dev/null || echo 'N/A')"
echo ""
echo "Run:"
echo "  cd $EXEC_DIR"
echo "  ./dpmDsmStandalone"
