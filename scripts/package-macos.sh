#!/usr/bin/env bash
# Creates the production disk image dist/ErgoPro2-<version>-macos.dmg from a
# Release build (documentation/BUILD.md 1.5).
#
#   scripts/package-macos.sh [build-dir] [output-dir]      (defaults: build-release, dist)
#
# Environment:
#   MACDEPLOYQT        path to macdeployqt (default: from PATH, else Homebrew Qt)
#   CODESIGN_IDENTITY  signing identity (default "-" = ad-hoc)
#   DMGBUILD           path to dmgbuild (default: from PATH, else installed
#                      with pip into a temporary Python environment)
#
# The build directory is not modified: the app is copied to a staging folder,
# Qt is bundled into the copy (macdeployqt), the copy is signed again and put
# into a compressed, read-only disk image whose window shows ErgoPro2.app, a
# link to /Applications and an arrow (drag-and-drop install, 2.0.0).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="${1:-build-release}"
OUT="${2:-dist}"
IDENTITY="${CODESIGN_IDENTITY:--}"
DMGBUILD_VERSION="1.6.5" # pinned: same layout on every build

fail() { echo "package-macos: $*" >&2; exit 1; }

VERSION="$(sed -nE 's/^[[:space:]]+VERSION ([0-9]+\.[0-9]+\.[0-9]+).*/\1/p' "$ROOT/CMakeLists.txt" | head -1)"
[[ -n "$VERSION" ]] || fail "cannot read the version from CMakeLists.txt"
APP="$BUILD/bin/ErgoPro2.app"
[[ -d "$APP" ]] || fail "$APP not found - build first (cmake --build $BUILD)"

MACDEPLOYQT="${MACDEPLOYQT:-$(command -v macdeployqt || true)}"
if [[ -z "$MACDEPLOYQT" ]] && command -v brew >/dev/null; then
    MACDEPLOYQT="$(brew --prefix qt)/bin/macdeployqt"
fi
[[ -x "$MACDEPLOYQT" ]] || fail "macdeployqt not found (set MACDEPLOYQT)"

STAGE="$(mktemp -d)"
trap 'rm -rf "$STAGE"' EXIT
ditto "$APP" "$STAGE/ErgoPro2.app" # keeps symlinks and permissions

echo "== Bundling Qt ($MACDEPLOYQT)"
"$MACDEPLOYQT" "$STAGE/ErgoPro2.app" -always-overwrite
# Keep only the Qt plugins ErgoPro2 uses: the window system (cocoa) and SVG icons.
# Everything else (styles - ErgoPro2 uses Fusion -, other image formats, TLS,
# network information, virtual keyboard, PDF) is removed; Homebrew Qt even
# copies plugins whose libraries it cannot resolve.
PLUGINS="$STAGE/ErgoPro2.app/Contents/PlugIns"
KEEP="platforms/libqcocoa.dylib imageformats/libqsvg.dylib iconengines/libqsvgicon.dylib"
for p in $KEEP; do
    [[ -f "$PLUGINS/$p" ]] || fail "required Qt plugin missing after macdeployqt: $p"
done
while IFS= read -r f; do
    rel="${f#"$PLUGINS"/}"
    [[ " $KEEP " == *" $rel "* ]] || rm -f "$f"
done < <(find "$PLUGINS" -type f)
find "$PLUGINS" -type d -empty -delete
echo "Plugins: $(cd "$PLUGINS" && find . -type f | sed 's|^\./||' | tr '\n' ' ')"

# Keep only the frameworks/libraries that the program and the kept plugins
# really load (followed through their load commands with otool).
FRAMEWORKS="$STAGE/ErgoPro2.app/Contents/Frameworks"
binary_of() { # framework or dylib name -> path of its binary
    if [[ "$1" == *.framework ]]; then echo "$FRAMEWORKS/$1/Versions/A/${1%.framework}"; else echo "$FRAMEWORKS/$1"; fi
}
needed=" "
queue=("$STAGE/ErgoPro2.app/Contents/MacOS/ErgoPro2")
for p in $KEEP; do queue+=("$PLUGINS/$p"); done
while ((${#queue[@]})); do
    bin="${queue[0]}"; queue=("${queue[@]:1}")
    while IFS= read -r dep; do
        name=""
        if [[ "$dep" =~ ([A-Za-z0-9_]+\.framework) ]]; then name="${BASH_REMATCH[1]}"; else name="$(basename "$dep")"; fi
        [[ -e "$FRAMEWORKS/$name" && "$needed" != *" $name "* ]] || continue
        needed+="$name "
        queue+=("$(binary_of "$name")")
    done < <(otool -L "$bin" | tail -n +2 | awk '{print $1}' | grep -E '^@(rpath|executable_path|loader_path)/')
done
for entry in "$FRAMEWORKS"/*; do
    name="$(basename "$entry")"
    [[ "$needed" == *" $name "* ]] || { echo "  not needed: $name"; rm -rf "$entry"; }
done

echo "== Signing ($IDENTITY)"
# macdeployqt changed the bundle: sign it again, otherwise macOS rejects it and
# does not recognize the Accessibility permission (BUILD.md 1.6).
codesign --force --deep --sign "$IDENTITY" "$STAGE/ErgoPro2.app"
codesign --verify --deep --strict "$STAGE/ErgoPro2.app"
echo "Architectures: $(lipo -archs "$STAGE/ErgoPro2.app/Contents/MacOS/ErgoPro2")"

echo "== Creating the disk image (drag-and-drop window)"
# dmgbuild writes the window layout (icons, Applications link, background)
# without scripting the Finder. It is installed into a temporary Python
# environment unless DMGBUILD points to an existing dmgbuild.
DMGBUILD="${DMGBUILD:-$(command -v dmgbuild || true)}"
if [[ -z "$DMGBUILD" ]]; then
    command -v python3 >/dev/null || fail "python3 not found (needed for dmgbuild)"
    python3 -m venv "$STAGE/venv" || fail "cannot create a Python environment for dmgbuild"
    "$STAGE/venv/bin/python" -m pip install --quiet "dmgbuild==$DMGBUILD_VERSION" \
        || fail "cannot install dmgbuild $DMGBUILD_VERSION"
    DMGBUILD="$STAGE/venv/bin/dmgbuild"
fi
# One TIFF with the 1x and 2x background, so Retina screens get the sharp one.
tiffutil -cathidpicheck "$ROOT/packaging/macos/dmg-background.png" "$ROOT/packaging/macos/dmg-background@2x.png" \
    -out "$STAGE/background.tiff" >/dev/null || fail "cannot create the background TIFF"

mkdir -p "$OUT"
DMG="$OUT/ErgoPro2-$VERSION-macos.dmg"
rm -f "$DMG"
"$DMGBUILD" -s "$ROOT/packaging/macos/dmg-settings.py" \
    -D app="$STAGE/ErgoPro2.app" -D background="$STAGE/background.tiff" \
    -D volume_icon="$ROOT/resources/icons/app/ErgoPro2.icns" \
    "ErgoPro2 $VERSION" "$DMG" || fail "dmgbuild failed"
hdiutil verify "$DMG" >/dev/null
echo "Created $DMG ($(du -h "$DMG" | cut -f1))"
