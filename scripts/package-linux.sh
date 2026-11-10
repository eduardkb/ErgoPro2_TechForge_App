#!/usr/bin/env bash
# Creates the Debian package dist/ergopro2_<version>_<arch>.deb from a Release
# build (documentation/BUILD.md 3.5). Needs cpack (cmake), dpkg-dev and file.
#
#   scripts/package-linux.sh [build-dir] [output-dir]      (defaults: build-release, dist)
#
# The package installs /usr/bin/ErgoPro2, the menu entry and the icon. Its
# dependencies (Qt 6, X11/XTest, libevdev) are computed with dpkg-shlibdeps,
# so "sudo apt install ./ergopro2_<version>_<arch>.deb" installs everything.
set -euo pipefail

BUILD="${1:-build-release}"
OUT="${2:-dist}"
fail() { echo "package-linux: $*" >&2; exit 1; }

[[ -x "$BUILD/bin/ErgoPro2" ]] || fail "$BUILD/bin/ErgoPro2 not found - build first (cmake --build $BUILD)"
command -v dpkg-shlibdeps >/dev/null || fail "dpkg-shlibdeps not found - sudo apt install dpkg-dev file"

(cd "$BUILD" && cpack -G DEB)
mkdir -p "$OUT"
DEB="$(ls -t "$BUILD"/ergopro2_*.deb | head -1)"
mv "$DEB" "$OUT/"
DEB="$OUT/$(basename "$DEB")"
echo "== Package information"
dpkg-deb --info "$DEB"
dpkg-deb --contents "$DEB"
echo "Created $DEB"
