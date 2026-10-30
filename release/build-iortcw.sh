#!/usr/bin/env bash
#
# Builds the Return to Castle Wolfenstein downloads: the campaign's own
# engine, iortcw, with the accessibility mode built into its menus and its
# game, for 64-bit Windows and, given the app a Mac built, for the Mac.
# Each is a folder to unzip anywhere, with an empty main folder for the
# player's own RTCW files and the install guide.
#
#   release/build-iortcw.sh --out DIR --version NAME [--mac-app TAR] [--no-nvda] [--keep]
#
# --mac-app is the iowolfsp.app.tar.gz release/build-iortcw-mac.sh wrote.
#
# Clones iortcw at the commit in integration/iortcw/base.txt, applies
# integration/iortcw/*.patch and cross-compiles with mingw-w64. None of
# RTCW's own files are in either download: the player brings them.
#
# Requires: git, make, zip, sha256sum, python3 and the 64-bit mingw-w64 compilers.
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
base_file="$repo_root/integration/iortcw/base.txt"

out=""
version=""
nvda=1
keep=0
mac_app=""
while [ $# -gt 0 ]; do
    case "$1" in
        --out) out="$2"; shift 2 ;;
        --version) version="$2"; shift 2 ;;
        --no-nvda) nvda=0; shift ;;
        --mac-app) mac_app="$2"; shift 2 ;;
        --keep) keep=1; shift ;;
        *) echo "error: unknown argument $1" >&2; exit 2 ;;
    esac
done
[ -n "$out" ] && [ -n "$version" ] || {
    echo "usage: build-iortcw.sh --out DIR --version NAME [--mac-app TAR] [--no-nvda] [--keep]" >&2
    exit 2
}

base() { sed -n "s/^$1=//p" "$base_file" | head -1; }
rtcw_repo="$(base repo)"
rtcw_commit="$(base commit)"

for tool in git make zip sha256sum python3 x86_64-w64-mingw32-gcc x86_64-w64-mingw32-g++ x86_64-w64-mingw32-windres; do
    command -v "$tool" >/dev/null || { echo "error: $tool is not installed" >&2; exit 1; }
done

workdir="$(mktemp -d "${TMPDIR:-/tmp}/eta-rtcw-XXXXXX")"
cleanup() { [ "$keep" -eq 1 ] || rm -rf "$workdir"; }
trap cleanup EXIT

src="$workdir/iortcw"
echo "==> cloning iortcw at $rtcw_commit"
git clone --quiet "$rtcw_repo" "$src"
git -C "$src" checkout --quiet "$rtcw_commit"
[ "$(git -C "$src" rev-parse HEAD)" = "$rtcw_commit" ] || {
    echo "error: iortcw is not at $rtcw_commit" >&2
    exit 1
}

echo "==> applying the patch series"
for patch in "$repo_root"/integration/iortcw/0*.patch; do
    git -C "$src" apply "$patch" || {
        echo "error: $(basename "$patch") does not apply to iortcw $rtcw_commit" >&2
        exit 1
    }
done

echo "==> building for 64-bit Windows"
(cd "$src/SP" && ./cross-make-mingw64.sh -j"$(nproc)" release \
    BUILD_SERVER=0 FEATURE_ACCESSIBILITY=1 \
    ETA_SOURCE_DIR="$repo_root" ETA_RELEASE_VERSION="$version" >"$workdir/mingw.log" 2>&1) || {
    tail -40 "$workdir/mingw.log" >&2
    exit 1
}
built="$src/SP/build/release-mingw32-x86_64"

mkdir -p "$out"
out="$(cd "$out" && pwd)"

guide() {
    # $1 platform name, $2 destination
    sed -e "s/@VERSION@/$version/g" -e "s/@PLATFORM@/$1/g" "$repo_root/release/INSTALL-iortcw.md.in" >"$2"
}

main_readme() {
    cat >"$1/main/PUT-YOUR-RTCW-FILES-HERE.txt" <<'TXT'
Copy these five files here from the main folder of your own Return to
Castle Wolfenstein: pak0.pk3, sp_pak1.pk3, sp_pak2.pk3, sp_pak3.pk3 and
sp_pak4.pk3. INSTALL.md, one folder up, says where to find them.
sp_zaccessibility.pk3 is the accessibility mode's own sounds and menus: leave it.
TXT
}

# The mode's own sounds - the doors, ladders and things to pick up heard
# where they are - made from source, and its Accessibility menu pages
# (release/rtcw/ui), into a pk3 of its own. Named sp_ so the campaign's
# file system loads it, after the game's own.
echo "==> making the mode's sounds and menus"
python3 "$repo_root/release/make-cue-sounds.py" "$workdir/cues/sound/accessibility" >/dev/null
cp -R "$repo_root/release/rtcw/ui" "$workdir/cues/"
(cd "$workdir/cues" && zip -qrX9 "$workdir/sp_zaccessibility.pk3" sound ui)

win="rtcw-accessibility-$version-windows"
stage="$workdir/stage/$win"
mkdir -p "$stage/main"
cp "$built/ioWolfSP.x64.exe" "$built/SDL264.dll" "$built/OpenAL64.dll" \
   "$built/renderer_sp_opengl1_x64.dll" "$built/renderer_sp_rend2_x64.dll" "$stage/"
cp "$built/main/cgame_sp_x64.dll" "$built/main/qagame_sp_x64.dll" "$built/main/ui_sp_x64.dll" "$stage/main/"
cp "$workdir/sp_zaccessibility.pk3" "$stage/main/"
cp "$src/SP/COPYING.txt" "$stage/COPYING.txt"
main_readme "$stage"
guide Windows "$stage/INSTALL.md"
if [ "$nvda" -eq 1 ]; then
    # Beside the game, where Windows looks for it first.
    "$repo_root/release/fetch-nvda-client.sh" "$workdir/nvda"
    cp "$workdir/nvda/x64/nvdaControllerClient.dll" "$stage/"
    cp "$workdir/nvda/LICENSE.txt" "$stage/nvdaControllerClient-LICENSE.txt"
fi
(cd "$workdir/stage" && zip -qr9 "$out/$win.zip" "$win")
echo "==> wrote $win.zip"

if [ -n "$mac_app" ]; then
    mac="rtcw-accessibility-$version-mac"
    stage="$workdir/stage/$mac"
    mkdir -p "$stage/main"
    tar -xzf "$mac_app" -C "$stage"
    test -d "$stage/iowolfsp.app" || { echo "error: $mac_app holds no iowolfsp.app" >&2; exit 1; }
    cp "$src/SP/COPYING.txt" "$stage/COPYING.txt"
    # Beside the app, in the folder the Mac build takes as its base.
    cp "$workdir/sp_zaccessibility.pk3" "$stage/main/"
    main_readme "$stage"
    guide Mac "$stage/INSTALL.md"
    # --symlinks: the app's libraries are reached through links.
    (cd "$workdir/stage" && zip -qr9 --symlinks "$out/$mac.zip" "$mac")
    echo "==> wrote $mac.zip"
fi

(cd "$out" && sha256sum ./*.zip >SHA256SUMS)
cat "$out/SHA256SUMS"
