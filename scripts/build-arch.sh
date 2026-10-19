#!/usr/bin/env bash
# =============================================================================
# scripts/build-arch.sh — Build DuckAI sidebar package for Arch Linux
# =============================================================================
# Wraps https://duck.ai with Pake (Tauri/WebKitGTK) and produces a
# .pkg.tar.zst package in build/ ready for pacman -U installation.
#
# This script NEVER runs sudo or installs system packages automatically.
# Missing dependencies are reported with the exact command to install them.
# =============================================================================
set -euo pipefail

# ── Colour helpers ────────────────────────────────────────────────────────────
RED='\033[0;31m'
YEL='\033[1;33m'
GRN='\033[0;32m'
CYN='\033[0;36m'
BLD='\033[1m'
RST='\033[0m'

info()  { printf "${CYN}[info]${RST}  %s\n" "$*"; }
ok()    { printf "${GRN}[ok]${RST}    %s\n" "$*"; }
warn()  { printf "${YEL}[warn]${RST}  %s\n" "$*"; }
die()   { printf "${RED}[error]${RST} %s\n" "$*" >&2; exit 1; }

# ── 0. Sudo / root guard ─────────────────────────────────────────────────────
# The build script must NOT be run as root: rustup, npm, and pake keep
# per-user state in ~/.rustup, ~/.cargo, and ~/.npm-global.  Running as root
# uses a different HOME with no toolchain configured.
if [[ "${EUID}" -eq 0 ]]; then
    die "Do not run this script with sudo or as root.\n" \
        "       Run it as your normal user:  bash scripts/build-arch.sh"
fi

# ── Constants ─────────────────────────────────────────────────────────────────
DUCK_URL="https://duck.ai"
APP_NAME="DuckAI"
APP_VERSION="1.0.0"   # bump this when you rebuild after meaningful changes
WIN_WIDTH=420
WIN_HEIGHT=1000
# Minimum required versions (from Pake's official documentation)
MIN_NODE_MAJOR=20
MIN_NODE_MINOR=9
MIN_RUST_MAJOR=1
MIN_RUST_MINOR=85
# Output directory (relative to repo root, ignored by .gitignore)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="${REPO_ROOT}/build"

# ── 1. OS guard ───────────────────────────────────────────────────────────────
info "Checking operating system …"
if [[ ! -f /etc/arch-release ]]; then
    die "This script targets Arch Linux only (no /etc/arch-release found)."
fi
ok "Arch Linux detected."

# ── 2. Required CLI programs ──────────────────────────────────────────────────
info "Checking required CLI programs …"
MISSING_PKGS=()

check_cmd() {
    local cmd="$1" pkg="$2"
    if ! command -v "$cmd" &>/dev/null; then
        warn "'$cmd' not found (package: $pkg)"
        MISSING_PKGS+=("$pkg")
    fi
}

check_cmd curl   curl
check_cmd wget   wget
check_cmd file   file
check_cmd tar    tar
check_cmd node   nodejs
check_cmd npm    npm

if [[ ${#MISSING_PKGS[@]} -gt 0 ]]; then
    printf "\n${RED}${BLD}Missing system packages. Install them with:${RST}\n"
    printf "  sudo pacman -S %s\n\n" "${MISSING_PKGS[*]}"
    die "Aborting — install the packages above and re-run this script."
fi
ok "All required CLI programs found."

# ── 3. Node.js version check ──────────────────────────────────────────────────
info "Checking Node.js version (need ≥ ${MIN_NODE_MAJOR}.${MIN_NODE_MINOR}.0) …"
NODE_RAW="$(node --version)"          # e.g. v22.1.0
NODE_VER="${NODE_RAW#v}"              # strip leading 'v'
NODE_MAJOR="${NODE_VER%%.*}"
NODE_REST="${NODE_VER#*.}"
NODE_MINOR="${NODE_REST%%.*}"

if (( NODE_MAJOR < MIN_NODE_MAJOR )) || \
   { (( NODE_MAJOR == MIN_NODE_MAJOR )) && (( NODE_MINOR < MIN_NODE_MINOR )); }; then
    printf "\n${RED}Node.js %s is too old. Pake requires ≥ %s.%s.0.${RST}\n" \
        "$NODE_VER" "$MIN_NODE_MAJOR" "$MIN_NODE_MINOR"
    printf "Install a newer version with:\n"
    printf "  sudo pacman -S nodejs npm\n"
    printf "Or use a version manager (nvm / fnm) to install v%s or later.\n\n" \
        "${MIN_NODE_MAJOR}"
    die "Node.js version too old."
fi
ok "Node.js ${NODE_VER} — OK."

# ── 4. Rust / rustc version check ────────────────────────────────────────────
info "Checking Rust (need ≥ ${MIN_RUST_MAJOR}.${MIN_RUST_MINOR}.0) …"

# Check 4a: is rustup present?
if ! command -v rustup &>/dev/null; then
    printf "\n${RED}rustup not found. Install it with:${RST}\n"
    printf "  sudo pacman -S rustup\n"
    printf "  rustup default stable\n\n"
    die "rustup not installed."
fi

# Check 4b: does rustup have a default toolchain configured for this user?
# 'rustup show active-toolchain' exits non-zero when no default is set.
if ! rustup show active-toolchain &>/dev/null; then
    printf "\n${RED}No default Rust toolchain is configured for user '%s'.${RST}\n" "${USER}"
    printf "Run the following to download and set the stable toolchain:\n"
    printf "  rustup default stable\n\n"
    printf "Note: run that command as yourself, NOT with sudo.\n\n"
    die "rustup default toolchain not set."
fi

# Check 4c: is rustc callable and recent enough?
if ! command -v rustc &>/dev/null; then
    printf "\n${RED}rustc not found even though rustup reports a toolchain.${RST}\n"
    printf "Ensure ~/.cargo/bin is in your PATH, then retry:\n"
    printf "  source ~/.cargo/env\n\n"
    die "rustc not in PATH."
fi

RUSTC_RAW="$(rustc --version)"       # e.g. rustc 1.85.0 (abc123 2025-01-01)
RUST_VER="$(echo "$RUSTC_RAW" | awk '{print $2}')"
RUST_MAJOR="${RUST_VER%%.*}"
RUST_REST="${RUST_VER#*.}"
RUST_MINOR="${RUST_REST%%.*}"

if (( RUST_MAJOR < MIN_RUST_MAJOR )) || \
   { (( RUST_MAJOR == MIN_RUST_MAJOR )) && (( RUST_MINOR < MIN_RUST_MINOR )); }; then
    printf "\n${RED}Rust %s is too old. Pake requires ≥ %s.%s.0.${RST}\n" \
        "$RUST_VER" "$MIN_RUST_MAJOR" "$MIN_RUST_MINOR"
    printf "Update Rust with:\n"
    printf "  rustup update stable\n\n"
    die "Rust version too old."
fi
ok "Rust ${RUST_VER} — OK."

# ── 5. pake-cli presence and write-access check ───────────────────────────────
info "Checking pake-cli …"
if ! command -v pake &>/dev/null; then
    # Pake might be installed but ~/.npm-global/bin not yet in PATH.
    # Auto-fix for this session rather than failing immediately.
    if [[ -x "${HOME}/.npm-global/bin/pake" ]]; then
        export PATH="${HOME}/.npm-global/bin:${PATH}"
        warn "~/.npm-global/bin was not in PATH — prepended for this build session."
        warn "Run 'bash scripts/setup.sh' once to make this permanent."
    else
        printf "\n${RED}pake-cli is not installed.${RST}\n\n"
        printf "Run the one-time setup script:\n"
        printf "  ${BLD}bash scripts/setup.sh${RST}\n\n"
        printf "It will configure your npm prefix, install pake-cli, and\n"
        printf "update your shell PATH automatically (fish / bash / zsh).\n\n"
        die "pake not found. Run scripts/setup.sh first."
    fi
fi

PAKE_BIN="$(command -v pake)"

# Tauri builds inside the pake-cli npm package directory (src-tauri/target/).
# Check whether the current user can write there; if not, pake was installed
# with sudo (or via AUR/pacman) into a root-owned prefix — it will always fail
# with EACCES regardless of where you call pake from.
NPM_GLOBAL_ROOT="$(npm root -g 2>/dev/null || true)"
PAKE_PKG_DIR="${NPM_GLOBAL_ROOT}/pake-cli"

if [[ -n "${NPM_GLOBAL_ROOT}" && -d "${PAKE_PKG_DIR}" && ! -w "${PAKE_PKG_DIR}" ]]; then
    printf "\n${RED}pake-cli is installed in a root-owned npm prefix.${RST}\n"
    printf "  pake binary  : %s\n" "${PAKE_BIN}"
    printf "  package dir  : %s  (not writable by %s)\n\n" "${PAKE_PKG_DIR}" "${USER}"
    printf "Tauri writes its Rust build output (src-tauri/target/) inside\n"
    printf "that directory, causing:  EACCES: permission denied, mkdir …/target\n\n"
    printf "Determine how pake was installed:\n"
    printf "  pacman -Qo \"%s\"\n\n" "${PAKE_BIN}"
    printf "Then remove it and reinstall as your normal user (no sudo):\n\n"
    printf "  # If installed via AUR / pacman:\n"
    printf "  sudo pacman -R pake-cli   # or: yay -R pake-cli\n\n"
    printf "  # If installed with sudo npm:\n"
    printf "  sudo npm uninstall -g pake-cli\n\n"
    printf "  # Then install into your home directory:\n"
    printf "  npm config set prefix ~/.npm-global\n"
    printf "  export PATH=\"\$HOME/.npm-global/bin:\$PATH\"\n"
    printf "  echo 'export PATH=\"\$HOME/.npm-global/bin:\$PATH\"' >> ~/.bashrc\n"
    printf "  npm install -g pake-cli\n\n"
    die "pake-cli package directory is not writable — reinstall as your normal user."
fi

PAKE_VER="$(pake --version 2>/dev/null || echo 'unknown')"
ok "pake-cli ${PAKE_VER} at ${PAKE_BIN} — OK."

# ── 6. Prepare build directory ────────────────────────────────────────────────
info "Preparing build directory: ${BUILD_DIR} …"
mkdir -p "${BUILD_DIR}"
# Remove any stale .pkg.tar.zst from previous runs so we can locate the new one
shopt -s nullglob
stale=( "${BUILD_DIR}"/*.pkg.tar.zst )
if [[ ${#stale[@]} -gt 0 ]]; then
    warn "Removing ${#stale[@]} stale package(s) from previous build …"
    rm -f "${stale[@]}"
fi
shopt -u nullglob
ok "Build directory ready."

# ── 7. Run Pake ───────────────────────────────────────────────────────────────
info "Building ${APP_NAME} → ${DUCK_URL} …"
info "  Window size : ${WIN_WIDTH} × ${WIN_HEIGHT}"
info "  Decorations : hidden (--hide-window-decorations)"
info "  Target      : zst (Arch .pkg.tar.zst)"
info "  Output dir  : ${BUILD_DIR}/"
printf "\n${BLD}This first build downloads and compiles Tauri's Rust dependencies.\n"
printf "It may take 10–20 minutes depending on your CPU and network speed.${RST}\n\n"

# Change to BUILD_DIR so pake places its output there.
cd "${BUILD_DIR}"

pake "${DUCK_URL}" \
    --name "${APP_NAME}" \
    --app-version "${APP_VERSION}" \
    --width "${WIN_WIDTH}" \
    --height "${WIN_HEIGHT}" \
    --hide-window-decorations \
    --targets zst

# ── 8. Locate the generated package ──────────────────────────────────────────
info "Locating generated .pkg.tar.zst …"
shopt -s nullglob
packages=( "${BUILD_DIR}"/*.pkg.tar.zst )
shopt -u nullglob

if [[ ${#packages[@]} -eq 0 ]]; then
    printf "\n${RED}${BLD}No .pkg.tar.zst file found in %s after build.${RST}\n" \
        "${BUILD_DIR}"
    printf "Possible causes:\n"
    printf "  • Pake produced a different format — check the output above.\n"
    printf "  • Tauri's pacman bundler was not invoked. Ensure webkit2gtk-4.1\n"
    printf "    and base-devel are installed (see README.md).\n\n"
    die "Package not found."
fi

if [[ ${#packages[@]} -gt 1 ]]; then
    warn "More than one .pkg.tar.zst found; using the newest one."
    # Sort by modification time, pick the most recent
    mapfile -t packages < <(ls -t "${BUILD_DIR}"/*.pkg.tar.zst)
fi

PKG_PATH="${packages[0]}"
PKG_FILE="$(basename "${PKG_PATH}")"

printf "\n${GRN}${BLD}Build successful!${RST}\n"
printf "  Package : %s\n" "${PKG_FILE}"
printf "  Path    : %s\n\n" "${PKG_PATH}"
printf "To install, run:\n"
printf "  ${BLD}bash scripts/install-arch.sh${RST}\n\n"
