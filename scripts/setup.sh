#!/usr/bin/env bash
# =============================================================================
# scripts/setup.sh — One-time environment setup for duckai-sidebar
# =============================================================================
# Run this ONCE before your first build.  It is safe to re-run at any time.
# It never uses sudo and never modifies system files.
#
# What it does:
#   1. Configures npm to use a user-owned global prefix (~/.npm-global)
#   2. Installs pake-cli into that prefix
#   3. Adds ~/.npm-global/bin to your shell's PATH config (fish / bash / zsh)
# =============================================================================
set -euo pipefail

# ── Colour helpers ────────────────────────────────────────────────────────────
RED='\033[0;31m'
YEL='\033[1;33m'
GRN='\033[0;32m'
CYN='\033[0;36m'
BLD='\033[1m'
RST='\033[0m'

info()  { printf "${CYN}[setup]${RST} %s\n" "$*"; }
ok()    { printf "${GRN}[ok]${RST}    %s\n" "$*"; }
warn()  { printf "${YEL}[warn]${RST}  %s\n" "$*"; }
die()   { printf "${RED}[error]${RST} %s\n" "$*" >&2; exit 1; }
step()  { printf "\n${BLD}── %s ──${RST}\n" "$*"; }

# ── Root guard ────────────────────────────────────────────────────────────────
if [[ "${EUID}" -eq 0 ]]; then
    die "Do not run setup.sh as root or with sudo."
fi

# ── Detect user's login shell ─────────────────────────────────────────────────
USER_SHELL="$(basename "${SHELL:-bash}")"

# ── Constants ─────────────────────────────────────────────────────────────────
NPM_PREFIX="${HOME}/.npm-global"
NPM_BIN="${NPM_PREFIX}/bin"
FISH_CONF_DIR="${HOME}/.config/fish"
FISH_CONF="${FISH_CONF_DIR}/conf.d/npm-global.fish"

# =============================================================================
step "Step 1: npm global prefix"
# =============================================================================
CURRENT_PREFIX="$(npm config get prefix 2>/dev/null || true)"

if [[ "${CURRENT_PREFIX}" == "${NPM_PREFIX}" ]]; then
    ok "npm prefix already set to ${NPM_PREFIX}"
else
    info "Setting npm global prefix to ${NPM_PREFIX} …"
    npm config set prefix "${NPM_PREFIX}"
    ok "npm prefix set to ${NPM_PREFIX}"
fi

# =============================================================================
step "Step 2: add ${NPM_BIN} to PATH"
# =============================================================================

# Export for THIS script session so npm installs to the right place
export PATH="${NPM_BIN}:${PATH}"

add_fish_path() {
    # Fish stores universal variables in a binary file — the cleanest way to
    # add a permanent path entry is a conf.d snippet that calls fish_add_path.
    mkdir -p "${FISH_CONF_DIR}/conf.d"
    if [[ -f "${FISH_CONF}" ]] && grep -q "npm-global" "${FISH_CONF}" 2>/dev/null; then
        ok "Fish PATH entry already exists: ${FISH_CONF}"
    else
        cat > "${FISH_CONF}" <<'FISH'
# Added by duckai-sidebar setup.sh
# Ensures npm user-global binaries (pake, etc.) are available.
fish_add_path --path "$HOME/.npm-global/bin"
FISH
        ok "Created Fish config: ${FISH_CONF}"
        info "Fish will pick this up automatically in all new sessions."
    fi
}

add_bash_path() {
    local rc_file="${HOME}/.bashrc"
    local marker="npm-global/bin"
    if grep -q "${marker}" "${rc_file}" 2>/dev/null; then
        ok "PATH entry already present in ${rc_file}"
    else
        printf '\n# Added by duckai-sidebar setup.sh\nexport PATH="$HOME/.npm-global/bin:$PATH"\n' \
            >> "${rc_file}"
        ok "Added PATH entry to ${rc_file}"
        info "Open a new terminal or run:  source ~/.bashrc"
    fi
}

add_zsh_path() {
    local rc_file="${HOME}/.zshrc"
    local marker="npm-global/bin"
    if grep -q "${marker}" "${rc_file}" 2>/dev/null; then
        ok "PATH entry already present in ${rc_file}"
    else
        printf '\n# Added by duckai-sidebar setup.sh\nexport PATH="$HOME/.npm-global/bin:$PATH"\n' \
            >> "${rc_file}"
        ok "Added PATH entry to ${rc_file}"
        info "Open a new terminal or run:  source ~/.zshrc"
    fi
}

case "${USER_SHELL}" in
    fish)
        info "Detected shell: fish"
        add_fish_path
        ;;
    zsh)
        info "Detected shell: zsh"
        add_zsh_path
        ;;
    *)
        info "Detected shell: ${USER_SHELL} (treating as bash)"
        add_bash_path
        ;;
esac

# =============================================================================
step "Step 3: install pake-cli"
# =============================================================================

if command -v pake &>/dev/null; then
    EXISTING_PAKE="$(command -v pake)"
    # Check if it's in a user-writable location
    NPM_ROOT="$(npm root -g 2>/dev/null || true)"
    if [[ -d "${NPM_ROOT}/pake-cli" && -w "${NPM_ROOT}/pake-cli" ]]; then
        PAKE_VER="$(pake --version 2>/dev/null || echo 'unknown')"
        ok "pake-cli ${PAKE_VER} already installed at ${EXISTING_PAKE}"
        info "To upgrade:  npm install -g pake-cli@latest"
    else
        warn "pake found at ${EXISTING_PAKE} but in a root-owned location."
        warn "Install it into ${NPM_PREFIX} instead:"
        warn "  sudo pacman -R pake-cli   (if from AUR)"
        warn "  sudo npm uninstall -g pake-cli   (if from sudo npm)"
        warn "Then re-run this setup script."
        exit 1
    fi
else
    info "Installing pake-cli into ${NPM_PREFIX} …"
    npm install -g pake-cli
    ok "pake-cli installed: $(command -v pake)"
fi

# =============================================================================
printf "\n${GRN}${BLD}Setup complete!${RST}\n\n"

if [[ "${USER_SHELL}" == "fish" ]]; then
    printf "Your Fish shell will automatically pick up the PATH change.\n"
    printf "No restart needed — you can build right now:\n\n"
else
    printf "Open a new terminal (or source your shell config), then build:\n\n"
fi

printf "  ${BLD}bash scripts/build-arch.sh${RST}\n\n"
