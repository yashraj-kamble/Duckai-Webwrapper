#!/usr/bin/env bash
# =============================================================================
# scripts/install.sh — Install duckai-sidebar
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
die()   { printf "${RED}[error]${RST} %s\n" "$*" >&2; exit 1; }

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

cd "${ROOT_DIR}"

# Check for existing pacman package
shopt -s nullglob
packages=( "${ROOT_DIR}"/duckai-sidebar-*.pkg.tar.zst )
shopt -u nullglob

if [[ ${#packages[@]} -gt 0 ]]; then
    PKG_PATH="${packages[0]}"
    info "Found Arch package: ${PKG_PATH}"
    printf "\n${BLD}Install via pacman?${RST} (Recommended on Arch)\n"
    read -r -p "Proceed with sudo pacman -U \"${PKG_PATH}\"? [Y/n] " CONFIRM
    case "${CONFIRM:-y}" in
        [yY] | [yY][eE][sS])
            sudo pacman -U "${PKG_PATH}"
            ok "Installed successfully via pacman."
            exit 0
            ;;
    esac
fi

# Fallback: install from meson build directory
if [[ ! -f "build/duckai-sidebar" ]]; then
    info "No build found. Running scripts/build.sh first …"
    bash "${SCRIPT_DIR}/build.sh"
fi

info "Installing to /usr via ninja …"
sudo ninja -C build install

ok "Installed duckai-sidebar to /usr/bin/duckai-sidebar"
printf "\nNext steps:\n"
printf "1. Add to your ~/.config/hypr/hyprland.conf:\n"
printf "   ${BLD}bind = \$mainMod, D, exec, duckai-sidebar --toggle${RST}\n"
printf "2. Reload Hyprland:\n"
printf "   ${BLD}hyprctl reload${RST}\n"
printf "3. Press SUPER+D to launch the Duck.ai sidebar!\n\n"
