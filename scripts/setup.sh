#!/usr/bin/env bash
# =============================================================================
# scripts/setup.sh — Dependency setup for duckai-sidebar on Arch Linux
# =============================================================================
set -euo pipefail

printf "\n=== Installing duckai-sidebar build and runtime dependencies ===\n\n"
sudo pacman -S --needed \
    base-devel \
    gtk3 \
    gtk-layer-shell \
    webkit2gtk-4.1 \
    glib2 \
    meson \
    ninja \
    pkgconf \
    gcc

printf "\nAll dependencies installed. You can now build with:\n"
printf "  bash scripts/build.sh\n"
printf "or\n"
printf "  makepkg -si\n\n"
