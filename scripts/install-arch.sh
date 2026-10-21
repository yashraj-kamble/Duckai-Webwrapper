#!/usr/bin/env bash
# =============================================================================
# scripts/install-arch.sh — Install built Arch package
# =============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

cd "${ROOT_DIR}"

shopt -s nullglob
packages=( "${ROOT_DIR}"/duckai-sidebar-*.pkg.tar.zst )
shopt -u nullglob

if [[ ${#packages[@]} -eq 0 ]]; then
    printf "No .pkg.tar.zst package found. Building now with scripts/build-arch.sh …\n"
    bash "${SCRIPT_DIR}/build-arch.sh"
    packages=( "${ROOT_DIR}"/duckai-sidebar-*.pkg.tar.zst )
fi

PKG_PATH="${packages[0]}"
printf "\nInstalling %s …\n\n" "${PKG_PATH}"
sudo pacman -U "${PKG_PATH}"

printf "\nDuck.ai sidebar installed successfully!\n"
printf "Add this line to your ~/.config/hypr/hyprland.conf:\n"
printf "  bind = \$mainMod, D, exec, duckai-sidebar --toggle\n\n"
