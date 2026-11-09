#!/usr/bin/env bash
# Regenerates the platform icon files from resources/icons/app/app.svg:
#   resources/icons/app/ErgoPro2.icns  (macOS, needs iconutil -> run on macOS)
#   resources/icons/app/ErgoPro2.ico   (Windows)
#   resources/icons/app/ergopro2.png   (Linux, 256x256)
# The generated files are committed, so this is only needed when app.svg changes.
#
# Usage: scripts/generate-icons.sh [path-to-Qt-prefix]
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="$ROOT/build-icongen"
OUT="$ROOT/resources/icons/app"
PREFIX_ARG=()
if [[ $# -ge 1 ]]; then PREFIX_ARG=(-DCMAKE_PREFIX_PATH="$1"); fi

cmake -S "$ROOT" -B "$BUILD" -G Ninja -DERGOPRO2_BUILD_TOOLS=ON -DERGOPRO2_BUILD_TESTS=OFF "${PREFIX_ARG[@]}"
cmake --build "$BUILD" --target ergopro2_icongen

TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
QT_QPA_PLATFORM=offscreen "$BUILD/bin/ergopro2_icongen" "$OUT/app.svg" "$TMP"

cp "$TMP/ErgoPro2.ico" "$OUT/ErgoPro2.ico"
cp "$TMP/ergopro2.png" "$OUT/ergopro2.png"
if command -v iconutil >/dev/null 2>&1; then
    iconutil -c icns "$TMP/ErgoPro2.iconset" -o "$OUT/ErgoPro2.icns"
else
    echo "iconutil not found (not macOS): ErgoPro2.icns was not regenerated" >&2
fi
echo "Icons written to $OUT"
