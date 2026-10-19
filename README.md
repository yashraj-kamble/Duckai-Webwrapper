# duckai-sidebar

An unofficial desktop sidebar for [Duck.ai](https://duck.ai), built with
[Pake](https://github.com/tw93/Pake) (Tauri + WebKitGTK) and tuned for
**Arch Linux + Hyprland**.

> **Disclaimer**  
> This is an *unofficial* community wrapper. It is not affiliated with,
> endorsed by, or sponsored by DuckDuckGo, Inc. Duck.ai is a service of
> DuckDuckGo; all Duck.ai content, trademarks, and intellectual property
> belong to DuckDuckGo, Inc. and are subject to their
> [Terms of Service](https://duckduckgo.com/terms) and
> [Privacy Policy](https://duckduckgo.com/privacy).

---

## Table of Contents

1. [What It Does](#what-it-does)
2. [Prerequisites](#prerequisites)
3. [Build](#build)
4. [Install](#install)
5. [Run](#run)
6. [Hyprland Sidebar Setup](#hyprland-sidebar-setup)
7. [Troubleshooting](#troubleshooting)
8. [Publishing to GitHub](#publishing-to-github)
9. [License](#license)

---

## What It Does

`duckai-sidebar` wraps `https://duck.ai` in a native desktop window
(420 × 1000 px, no title bar decorations) and pins it as a sidebar in
Hyprland using floating window rules.

| Property | Value |
|----------|-------|
| Wrapped URL | `https://duck.ai` |
| App name | `DuckAI` |
| Default size | 420 × 1000 px |
| Window decorations | Hidden |
| Package format | `.pkg.tar.zst` (Arch pacman) |
| Backend | Pake → Tauri → WebKitGTK |

---

## Prerequisites

### System packages

The build requires several system-level libraries.  
Run the following command once (pacman will show its normal update prompt — do
not bypass it with `--noconfirm`):

```bash
sudo pacman -Syu \
  base-devel \
  nodejs \
  npm \
  rustup \
  webkit2gtk-4.1 \
  gdk-pixbuf2 \
  librsvg \
  curl \
  wget \
  file \
  tar
```

> **Why these packages?**
> - `base-devel` — GCC, make, pkg-config, and other build essentials required
>   by Rust's C-FFI crates used inside Tauri.
> - `nodejs`, `npm` — runtime for Pake CLI (≥ 20.9.0 required).
> - `rustup` — manages the Rust toolchain (≥ 1.85.0 required; Pake's
>   dependency chain uses Rust edition 2024 features that were stabilised in
>   1.85.0).
> - `webkit2gtk-4.1` — the WebKit rendering engine that Tauri/Pake embeds on
>   Linux.
> - `gdk-pixbuf2`, `librsvg` — icon/loader libraries required by Tauri's
>   AppImage and pacman bundlers; missing these causes the `linuxdeploy`
>   plugin error.
> - `curl`, `wget`, `file`, `tar` — utilities Pake uses for dependency and
>   asset management.

### Rust toolchain

After installing `rustup`, initialise the stable channel:

```bash
rustup default stable
# Verify versions:
rustc --version   # should be ≥ 1.85.0
node --version    # should be ≥ v20.9.0
```

If `rustc` is already installed but old, update it:

```bash
rustup update stable
```

### Pake CLI

> **⚠️ Never install pake-cli with `sudo npm install -g`.**  
> Pake (via Tauri) writes its Rust build output inside the npm package directory.  
> A root-owned package directory causes immediate `EACCES` errors during the build.

The easiest way is to use the included setup script (see [Build](#build) below) — it handles everything automatically.

**If you already installed pake with sudo**, remove it first:

```bash
# Find out how it was installed:
pacman -Qo /usr/bin/pake
# → if owned by a pacman/AUR package:
sudo pacman -R pake-cli
# → if installed with sudo npm:
sudo npm uninstall -g pake-cli
```

Then run `bash scripts/setup.sh` as described below.

---

## Build

The project includes a **one-time setup script** that configures everything for you:

```bash
# Clone the project (if you haven't already)
git clone https://github.com/YOUR_USERNAME/duckai-sidebar.git
cd duckai-sidebar

# Make all scripts executable
chmod +x scripts/setup.sh scripts/build-arch.sh scripts/install-arch.sh

# FIRST TIME ONLY: set up npm prefix, install pake-cli, update PATH
bash scripts/setup.sh

# Set the Rust toolchain (first time only):
rustup default stable

# Build (no sudo)
bash scripts/build-arch.sh
```

**That's it.** `setup.sh` auto-detects your shell (fish / bash / zsh) and
updates the right config file, so you never have to manually edit PATH again.


The script will:

1. Verify you are on Arch Linux.
2. Check that `curl`, `wget`, `file`, `tar`, `node`, and `npm` are present.
3. Confirm Node.js ≥ 20.9.0 and Rust ≥ 1.85.0.
4. Confirm `pake` CLI is installed.
5. Run `pake https://duck.ai --name DuckAI --width 420 --height 1000 --hide-window-decorations --targets zst` with output going into `build/`.
6. Report the path of the generated `.pkg.tar.zst`.

> **First build time**  
> Pake compiles a full Tauri/Rust project on the first run. This downloads
> Rust crate dependencies and compiles them — expect **10–20 minutes** on a
> typical machine. Subsequent builds are much faster because the Cargo cache is
> warm.

The package will appear at a path similar to:

```
build/DuckAI_1.0.0_x86_64.pkg.tar.zst
```

---

## Install

```bash
bash scripts/install-arch.sh
```

The script will:

1. Locate the `.pkg.tar.zst` in `build/`.
2. Print the full filename and path.
3. Ask for **y/N confirmation** before proceeding.
4. Run `sudo pacman -U <package>` — pacman's own interactive prompt is
   preserved; no `--noconfirm` is used.

---

## Run

After installation, launch DuckAI from your terminal or application launcher:

```bash
duckai
```

Or search for **DuckAI** in Rofi, Wofi, or whatever launcher you use.

---

## Hyprland Sidebar Setup

### 1. Find the app class

Open DuckAI, then in another terminal:

```bash
hyprctl clients
```

Look for the DuckAI entry and note the `class:` field. It is commonly
`duckai` but may differ depending on your Pake/Tauri version.

### 2. Apply the window rules

Copy (or symlink) the example config:

```bash
cp hyprland/duckai-sidebar.conf.example ~/.config/hypr/duckai-sidebar.conf
```

Edit the copy and replace every `REPLACE_WITH_CLASS` with the actual class
you found above (e.g. `duckai`).

Then source it from your main Hyprland config:

```bash
# In ~/.config/hypr/hyprland.conf — add this line:
source = ~/.config/hypr/duckai-sidebar.conf
```

Reload Hyprland:

```bash
hyprctl reload
```

### 3. What the rules do

```ini
# Float DuckAI so it can overlay tiled windows
windowrule = float, match:class ^(duckai)$

# 420 px wide, 95% of monitor height (adjust to taste)
windowrule = size 420 95%, match:class ^(duckai)$

# Pin to the left edge, 2% from top (leaves room for a status bar)
windowrule = move 0 2%, match:class ^(duckai)$

# Keep visible on every workspace (remove this line to pin only on current WS)
windowrule = pin, match:class ^(duckai)$
```

> **Syntax note** — these rules use the modern unified `windowrule` syntax
> introduced in Hyprland 0.41. If you are on an older release, see the
> `hyprland/duckai-sidebar.conf.example` file for the `windowrulev2`
> equivalents.

### 4. Right-edge sidebar

To anchor DuckAI to the **right** side instead, change the `move` rule.
Replace `0` with your monitor width minus 420 pixels. For a 1920 px wide
monitor:

```ini
windowrule = move 1500 2%, match:class ^(duckai)$
```

### 5. Current-workspace-only mode

Remove or comment out the `pin` rule. The window will then only be visible
on the workspace where you opened DuckAI.

---

## Troubleshooting

### Build fails: "feature `edition2024` is required"

Your Rust version is below 1.85.0. Update it:

```bash
rustup update stable
rustc --version   # confirm ≥ 1.85.0
```

### Build fails: "Can't detect any appindicator library"

On Arch this is usually resolved by the `base-devel` package group. If the
error persists, check that `libappindicator-gtk3` is available from AUR or
install the equivalent:

```bash
# Try from AUR if not in official repos:
# yay -S libappindicator-gtk3
```

### Build fails: "failed to run linuxdeploy" / "cannot stat gdk-pixbuf"

Install the missing gdk-pixbuf loader packages and refresh the cache:

```bash
sudo pacman -S gdk-pixbuf2 librsvg
gdk-pixbuf-query-loaders --update-cache
```

Then rebuild.

### pake: command not found

```bash
npm install -g pake-cli
# If using a custom prefix:
export PATH="$HOME/.npm-global/bin:$PATH"
```

### rustc: command not found after installing rustup

```bash
rustup default stable
# Ensure ~/.cargo/bin is in your PATH:
source ~/.cargo/env
```

### No .pkg.tar.zst produced — only a .deb or .AppImage

Pake auto-detects the distro from `/etc/os-release`. If the auto-detection
fails, force the Arch format:

```bash
# Manually invoke pake from the build/ directory:
cd build
pake https://duck.ai --name DuckAI --width 420 --height 1000 \
    --hide-window-decorations --targets zst
```

### DuckAI window does not match the Hyprland rules

Run `hyprctl clients` while the DuckAI window is open and confirm the exact
`class:` value. Replace `REPLACE_WITH_CLASS` (or the value you set) in
`~/.config/hypr/duckai-sidebar.conf` with the correct class, then:

```bash
hyprctl reload
```

---

## Publishing to GitHub

These steps publish the local project to a new GitHub repository. They do
**not** upload any generated packages or credentials.

### 1. Initialise the local Git repository

```bash
cd duckai-sidebar

git init
git add .
git commit -m "chore: initial commit"
```

### 2. Create an empty repository on GitHub

Go to <https://github.com/new> and create a repository named `duckai-sidebar`.
Leave *"Initialize this repository with a README"* **unchecked** — you already
have one locally.

### 3. Push the local repository

```bash
# Replace YOUR_USERNAME with your GitHub username
git remote add origin https://github.com/YOUR_USERNAME/duckai-sidebar.git
git branch -M main
git push -u origin main
```

### 4. Verify `.gitignore` coverage

The `.gitignore` excludes `build/`, `*.pkg.tar.zst`, `node_modules/`, and
`target/`. Run the following to confirm no generated files are staged:

```bash
git status
```

If any binary or generated file appears, add it to `.gitignore` before
committing.

---

## License

The scripts, Hyprland configuration, and documentation in this repository are
licensed under the **MIT License** — see [LICENSE](LICENSE) for the full text.

The license does **not** cover Duck.ai, Pake, or Tauri. Refer to the
[LICENSE](LICENSE) disclaimer section for details.
