#!/usr/bin/env bash
# Builds the Windows test download of the RTCW story game: iortcw's
# single player with the accessibility mode (integration/iortcw-sp/) and
# the story levels (story/).
#
#   release/build-rtcw-story.sh --rtcw <RTCW main folder> --q3map2 <q3map2> \
#       --bspc <bspc> --nvda <folder with x64/nvdaControllerClient.dll> \
#       --version <name> --out <folder>
#
# The RTCW folder is only read, to compile the levels against: nothing
# from it is packed. Players copy their own pak0.pk3 and sp_pak1-3.pk3.

set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
rtcw="" q3map2="" bspc="" nvda="" version="" out=""
while [ $# -gt 0 ]; do
    case "$1" in
        --rtcw) rtcw="$2"; shift 2 ;;
        --q3map2) q3map2="$2"; shift 2 ;;
        --bspc) bspc="$2"; shift 2 ;;
        --nvda) nvda="$2"; shift 2 ;;
        --version) version="$2"; shift 2 ;;
        --out) out="$2"; shift 2 ;;
        *) echo "unknown option $1" >&2; exit 2 ;;
    esac
done
for v in rtcw q3map2 bspc nvda version out; do
    [ -n "${!v}" ] || { sed -n '5,8p' "$0" >&2; exit 2; }
done
mkdir -p "$out"
out="$(cd "$out" && pwd)"

# shellcheck source=/dev/null
. "$repo_root/integration/iortcw-sp/base.txt"
# shellcheck source=/dev/null
. "$repo_root/release/nvda-client.pin"
echo "$NVDA_DLL_X64_SHA256  $nvda/x64/nvdaControllerClient.dll" | sha256sum -c --quiet \
    || { echo "error: the NVDA client is not the pinned one" >&2; exit 1; }

work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT

echo "==> iortcw at $commit"
git init -q "$work/iortcw"
git -C "$work/iortcw" fetch -q --depth 1 "$repo" "$commit"
git -C "$work/iortcw" checkout -q FETCH_HEAD
for p in "$repo_root"/integration/iortcw-sp/*.patch; do
    git -C "$work/iortcw" apply "$p"
done

echo "==> building for Windows"
(cd "$work/iortcw/SP" && ./cross-make-mingw64.sh -j"$(nproc)" BUILD_SERVER=0 BUILD_GAME_QVM=0 \
    FEATURE_ACCESSIBILITY=1 ETA_SOURCE_DIR="$repo_root" ETA_RELEASE_VERSION="$version" > "$work/build.log" 2>&1) \
    || { tail -40 "$work/build.log" >&2; exit 1; }
bin="$work/iortcw/SP/build/release-mingw32-x86_64"

echo "==> the story levels"
"$repo_root/story/build-story.sh" --rtcw "$rtcw" --q3map2 "$q3map2" --bspc "$bspc" --out "$work/story"

name="wolfs-lair-rtcw-$version-windows"
stage="$work/$name"
mkdir -p "$stage/main" "$stage/nvda"
cp "$bin/ioWolfSP.x64.exe" "$bin/SDL264.dll" "$bin/OpenAL64.dll" \
   "$bin/renderer_sp_opengl1_x64.dll" "$bin/renderer_sp_rend2_x64.dll" "$stage/"
cp "$bin/main/cgame_sp_x64.dll" "$bin/main/ui_sp_x64.dll" "$bin/main/qagame_sp_x64.dll" "$stage/main/"
cp "$work/story/zzz_story.pk3" "$stage/main/"
# NVDA's client beside the game, where LoadLibrary finds it, and its
# licence with it.
cp "$nvda/x64/nvdaControllerClient.dll" "$stage/"
cp "$nvda/LICENSE.txt" "$stage/nvda/"
cp "$work/iortcw/SP/COPYING.txt" "$stage/iortcw-COPYING.txt"

echo "==> the cue sounds"
mkdir -p "$work/cues/sound/accessibility"
python3 "$repo_root/release/make-cue-sounds.py" "$work/cues/sound/accessibility" > /dev/null
(cd "$work/cues" && zip -qr "$stage/main/zzz_accessibility.pk3" sound)

# Native libraries, not the QVMs a stock config might ask for: the mode
# is in the libraries.
# Settings and saves in a folder of the game's own: in the one RTCW installs
# share, another install's files (RealRTCW's, say) would load over RTCW's own.
args='+set fs_homepath "%APPDATA%\WolfsLair" +set vm_game 0 +set vm_cgame 0 +set vm_ui 0'
printf '@echo off\r\ncd /d "%%~dp0"\r\nstart "" ioWolfSP.x64.exe %s +spmap wl_prologue\r\n' "$args" \
    > "$stage/Play the prologue.bat"
printf '@echo off\r\ncd /d "%%~dp0"\r\nstart "" ioWolfSP.x64.exe %s\r\n' "$args" \
    > "$stage/Play Return to Castle Wolfenstein.bat"
sed "s/@VERSION@/$version/g" "$repo_root/release/RTCW-STORY-README.txt.in" | sed 's/$/\r/' > "$stage/README.txt"

rm -f "$out/$name.zip"
(cd "$work" && zip -qr "$out/$name.zip" "$name")
echo "wrote $out/$name.zip"
