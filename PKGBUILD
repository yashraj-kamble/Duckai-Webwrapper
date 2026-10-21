# Maintainer: Yashraj Kamble <yashraj.kamble@proton.me>
pkgname=duckai-sidebar
pkgver=2.0.0
pkgrel=1
pkgdesc="Native Wayland sidebar panel for Duck.ai using GTK Layer Shell and WebKitGTK"
arch=('x86_64')
url="https://github.com/yashraj-kamble/duckai-sidebar"
license=('MIT')
depends=(
    'gtk3'
    'gtk-layer-shell'
    'webkit2gtk-4.1'
    'glib2'
)
makedepends=(
    'meson'
    'ninja'
    'pkgconf'
    'gcc'
)
source=()
sha256sums=()

build() {
    if [ ! -d "$startdir/build-pkg" ]; then
        meson setup "$startdir" "$startdir/build-pkg" --prefix=/usr --buildtype=release
    else
        meson setup --reconfigure "$startdir" "$startdir/build-pkg" --prefix=/usr --buildtype=release
    fi
    ninja -C "$startdir/build-pkg"
}

check() {
    "$startdir/build-pkg/duckai-sidebar" --version
}

package() {
    DESTDIR="$pkgdir" ninja -C "$startdir/build-pkg" install
    install -Dm644 "$startdir/LICENSE" "$pkgdir/usr/share/licenses/$pkgname/LICENSE"
    install -Dm644 "$startdir/README.md" "$pkgdir/usr/share/doc/$pkgname/README.md"
}
