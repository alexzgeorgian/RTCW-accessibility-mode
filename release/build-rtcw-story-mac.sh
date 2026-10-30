#!/usr/bin/env bash
# Build the native macOS download for Wolf's Lair: The End of Terror.
# The universal app carries x86_64 and arm64 versions of ioRTCW and its
# accessibility-enabled single-player modules.
#
# Usage:
#   release/build-rtcw-story-mac.sh --story-pk3 <zzz_story.pk3> \
#       --version <name> --out <folder>

set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
story_pk3="" version="" out=""
while [ $# -gt 0 ]; do
    case "$1" in
        --story-pk3) story_pk3="$2"; shift 2 ;;
        --version) version="$2"; shift 2 ;;
        --out) out="$2"; shift 2 ;;
        *) echo "unknown option $1" >&2; exit 2 ;;
    esac
done
for v in story_pk3 version out; do
    [ -n "${!v}" ] || { sed -n '7,9p' "$0" >&2; exit 2; }
done
[ -s "$story_pk3" ] || { echo "error: no story pk3 at $story_pk3" >&2; exit 1; }
command -v xcrun >/dev/null || { echo "error: Xcode command-line tools are required" >&2; exit 1; }
command -v lipo >/dev/null || { echo "error: lipo is required" >&2; exit 1; }
mkdir -p "$out"
out="$(cd "$out" && pwd)"

# shellcheck source=/dev/null
. "$repo_root/integration/iortcw-sp/base.txt"
work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT

echo "==> iortcw at $commit"
git init -q "$work/iortcw"
git -C "$work/iortcw" fetch -q --depth 1 "$repo" "$commit"
git -C "$work/iortcw" checkout -q FETCH_HEAD
for patch in "$repo_root"/integration/iortcw-sp/*.patch; do
    git -C "$work/iortcw" apply "$patch"
done

# The upstream make-macosx-ub.sh requires obsolete SDKs. Build each current
# architecture with the Xcode toolchain, then let iortcw's app builder lipo
# the engine, renderers and game modules into one app bundle.
for arch in x86_64 arm64; do
    if [ "$arch" = x86_64 ]; then
        min_os=10.15
    else
        min_os=11.0
    fi
    echo "==> building macOS $arch (minimum macOS $min_os)"
    make -C "$work/iortcw/SP" -j"$(sysctl -n hw.ncpu)" \
        ARCH="$arch" PLATFORM=darwin \
        MACOSX_VERSION_MIN="$min_os" \
        BUILD_CLIENT=1 BUILD_SERVER=0 BUILD_GAME_SO=1 BUILD_GAME_QVM=0 \
        FEATURE_ACCESSIBILITY=1 ETA_SOURCE_DIR="$repo_root" \
        USE_INTERNAL_ZLIB=0 USE_FREETYPE=0 \
        ETA_RELEASE_VERSION="$version" > "$work/build-$arch.log" 2>&1 \
        || { tail -60 "$work/build-$arch.log" >&2; exit 1; }
done

echo "==> assembling a universal ioWolfSP.app"
(
    cd "$work/iortcw/SP"
    MACOSX_DEPLOYMENT_TARGET=11.0 \
    MACOSX_DEPLOYMENT_TARGET_X86_64=10.15 \
    MACOSX_DEPLOYMENT_TARGET_ARM64=11.0 \
        ./make-macosx-app.sh release
)
app="$work/iortcw/SP/build/release-darwin-universal/iowolfsp.app"
[ -x "$app/Contents/MacOS/iowolfsp" ] || { echo "error: iortcw did not create its Mac app" >&2; exit 1; }

name="wolfs-lair-rtcw-$version-mac"
stage="$work/$name"
mkdir -p "$stage"
cp -R "$app" "$stage/iowolfsp.app"
main="$stage/iowolfsp.app/Contents/MacOS/main"
[ -d "$main" ] || { echo "error: iortcw app has no main directory" >&2; exit 1; }
cp "$story_pk3" "$main/zzz_story.pk3"

# Generate the sound cues as ordinary game data; the story and accessibility
# packages stay separate so either can be removed without replacing the engine.
mkdir -p "$work/cues/sound/accessibility"
python3 "$repo_root/release/make-cue-sounds.py" "$work/cues/sound/accessibility" >/dev/null
(cd "$work/cues" && zip -qr "$main/zzz_accessibility.pk3" sound)
cp "$work/iortcw/SP/COPYING.txt" "$stage/iortcw-COPYING.txt"

# The game keeps its settings and saves in a folder of its own. In the one
# every RTCW install shares, ~/Library/Application Support/RTCW, another
# install's files - RealRTCW's, say - would load over RTCW's own, and their
# characters name weapons this game does not have ("unknown token 'TT33'").
cat > "$stage/Play the prologue.command" <<'LAUNCHER'
#!/usr/bin/env bash
set -euo pipefail
here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
game="$here/iowolfsp.app/Contents/MacOS"
cd "$game"
exec ./iowolfsp +set fs_homepath "$HOME/Library/Application Support/WolfsLair" +set vm_game 0 +set vm_cgame 0 +set vm_ui 0 +spmap wl_prologue
LAUNCHER
cat > "$stage/Play Return to Castle Wolfenstein.command" <<'LAUNCHER'
#!/usr/bin/env bash
set -euo pipefail
here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
game="$here/iowolfsp.app/Contents/MacOS"
cd "$game"
exec ./iowolfsp +set fs_homepath "$HOME/Library/Application Support/WolfsLair" +set vm_game 0 +set vm_cgame 0 +set vm_ui 0
LAUNCHER
chmod +x "$stage/Play the prologue.command" "$stage/Play Return to Castle Wolfenstein.command"
sed "s/@VERSION@/$version/g" "$repo_root/release/RTCW-STORY-MAC-README.txt.in" > "$stage/README.txt"

archive="$out/$name.zip"
rm -f "$archive"
ditto -c -k --sequesterRsrc --keepParent "$stage" "$archive"
echo "wrote $archive"
