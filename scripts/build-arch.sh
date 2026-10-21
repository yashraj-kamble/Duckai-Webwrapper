#!/usr/bin/env bash
# =============================================================================
# scripts/build-arch.sh — Build Arch Linux package for duckai-sidebar
# =============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

cd "${ROOT_DIR}"

printf "\n=== Building Arch Linux package (makepkg) ===\n\n"
makepkg -f

printf "\nPackage built successfully!\n"
ls -la duckai-sidebar-*.pkg.tar.zst
printf "\nTo install, run:\n  bash scripts/install-arch.sh\n\n"
