#!/usr/bin/env bash
# =============================================================================
# scripts/install-arch.sh — Install the DuckAI .pkg.tar.zst with pacman
# =============================================================================
# Locates the package produced by build-arch.sh, shows its filename, and asks
# for confirmation before running  sudo pacman -U.
# No --noconfirm is passed — pacman's own interactive prompt is preserved.
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
die()   { printf "${RED}[error]${RST} %s\n" "$*" >&2; exit 1; }

# ── Locate the package ────────────────────────────────────────────────────────
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="${REPO_ROOT}/build"

if [[ ! -d "${BUILD_DIR}" ]]; then
    printf "\n${RED}Build directory not found: %s${RST}\n" "${BUILD_DIR}"
    printf "Run the build script first:\n"
    printf "  ${BLD}bash scripts/build-arch.sh${RST}\n\n"
    die "Nothing to install."
fi

shopt -s nullglob
packages=( "${BUILD_DIR}"/*.pkg.tar.zst )
shopt -u nullglob

if [[ ${#packages[@]} -eq 0 ]]; then
    printf "\n${RED}No .pkg.tar.zst found in %s${RST}\n" "${BUILD_DIR}"
    printf "Run the build script first:\n"
    printf "  ${BLD}bash scripts/build-arch.sh${RST}\n\n"
    die "Nothing to install."
fi

# If multiple packages exist, pick the most recently modified one
if [[ ${#packages[@]} -gt 1 ]]; then
    mapfile -t packages < <(ls -t "${BUILD_DIR}"/*.pkg.tar.zst)
    printf "${YEL}[warn]${RST}  Multiple packages found; selecting the newest.\n"
fi

PKG_PATH="${packages[0]}"
PKG_FILE="$(basename "${PKG_PATH}")"

# ── Confirm before installing ─────────────────────────────────────────────────
printf "\n${BLD}Package to install:${RST}\n"
printf "  File : %s\n" "${PKG_FILE}"
printf "  Path : %s\n\n" "${PKG_PATH}"
printf "${YEL}This will run:  sudo pacman -U \"%s\"${RST}\n" "${PKG_PATH}"
printf "Pacman will show its own dependency and confirmation prompt.\n\n"

read -r -p "Proceed with installation? [y/N] " CONFIRM
case "${CONFIRM}" in
    [yY] | [yY][eE][sS])
        : ;;  # continue
    *)
        printf "\nInstallation cancelled.\n"
        exit 0
        ;;
esac

# ── Install ───────────────────────────────────────────────────────────────────
info "Running: sudo pacman -U \"${PKG_PATH}\" …"
sudo pacman -U "${PKG_PATH}"

ok "DuckAI installed successfully."
printf "\nLaunch the app with:\n"
printf "  ${BLD}duckai${RST}   (or search for DuckAI in your application launcher)\n\n"
printf "Then apply your Hyprland sidebar rules — see:\n"
printf "  hyprland/duckai-sidebar.conf.example\n\n"
