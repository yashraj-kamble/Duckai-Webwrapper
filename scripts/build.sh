#!/usr/bin/env bash
# =============================================================================
# scripts/build.sh — Build duckai-sidebar
# =============================================================================
set -euo pipefail

RED='\033[0;31m'
GRN='\033[0;32m'
CYN='\033[0;36m'
YEL='\033[1;33m'
BLD='\033[1m'
RST='\033[0m'

info()  { printf "${CYN}[info]${RST}  %s\n" "$*"; }
ok()    { printf "${GRN}[ok]${RST}    %s\n" "$*"; }
warn()  { printf "${YEL}[warn]${RST}  %s\n" "$*"; }
die()   { printf "${RED}[error]${RST} %s\n" "$*" >&2; exit 1; }

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

cd "${ROOT_DIR}"

printf "\n${BLD}=== Building duckai-sidebar (Native Wayland Layer Shell) ===${RST}\n\n"

# Verify dependencies
info "Checking system dependencies …"
MISSING=()
for pkg in gtk+-3.0 gtk-layer-shell-0 webkit2gtk-4.1 glib-2.0; do
    if ! pkg-config --exists "${pkg}"; then
        MISSING+=("${pkg}")
    fi
done

if [[ ${#MISSING[@]} -gt 0 ]]; then
    die "Missing required pkg-config libraries: ${MISSING[*]}\nInstall them with:\n  sudo pacman -S gtk3 gtk-layer-shell webkit2gtk-4.1 glib2"
fi

if ! command -v meson &>/dev/null || ! command -v ninja &>/dev/null; then
    die "meson or ninja not found. Install with:\n  sudo pacman -S meson ninja"
fi

ok "All build dependencies satisfied."

# Build using meson
info "Configuring build directory (build/) …"
if [[ ! -d "build" ]]; then
    meson setup build --prefix=/usr
else
    meson setup --reconfigure build --prefix=/usr
fi

info "Compiling with ninja …"
ninja -C build

ok "Compilation successful: build/duckai-sidebar"
printf "\nYou can run the binary directly:\n"
printf "  ${BLD}./build/duckai-sidebar --help${RST}\n"
printf "  ${BLD}./build/duckai-sidebar --toggle${RST}\n\n"
printf "To install system-wide, run:\n"
printf "  ${BLD}bash scripts/install.sh${RST}\n"
printf "Or build an Arch package with:\n"
printf "  ${BLD}makepkg -si${RST}\n\n"
