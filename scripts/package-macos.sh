#!/usr/bin/env bash
# Creates the production disk image dist/ErgoPro2-<version>-macos.dmg from a
# Release build (documentation/BUILD.md 1.5).
#
#   scripts/package-macos.sh [build-dir] [output-dir]      (defaults: build-release, dist)
#
# Environment:
#   MACDEPLOYQT        path to macdeployqt (default: from PATH, else Homebrew Qt)
#   CODESIGN_IDENTITY  signing identity (default "-" = ad-hoc)
#
# The build directory is not modified: the app is copied to a staging folder,
# Qt is bundled into the copy (macdeployqt), the copy is signed again and put
# into a compressed, read-only disk image.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="${1:-build-release}"
OUT="${2:-dist}"
IDENTITY="${CODESIGN_IDENTITY:--}"

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

echo "== Creating the disk image"
mkdir -p "$OUT"
DMG="$OUT/ErgoPro2-$VERSION-macos.dmg"
rm -f "$DMG"
hdiutil create -volname "ErgoPro2 $VERSION" -srcfolder "$STAGE" -fs HFS+ -format UDZO -ov "$DMG" >/dev/null
hdiutil verify "$DMG" >/dev/null
echo "Created $DMG ($(du -h "$DMG" | cut -f1))"
