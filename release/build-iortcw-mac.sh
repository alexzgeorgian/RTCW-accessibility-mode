#!/usr/bin/env bash
#
# Build the campaign of Return to Castle Wolfenstein with the accessibility
# mode for the Mac: iortcw, as integration/iortcw/base.txt pins it, with the
# patches in integration/iortcw applied, built for Apple Silicon and Intel
# and joined into one iowolfsp.app by iortcw's own script.
#
#   release/build-iortcw-mac.sh <out-dir> [version]
#
# Writes <out-dir>/iowolfsp.app.tar.gz: a tar, because the app holds
# symbolic links an artifact upload would flatten. Runs on a Mac.

set -euo pipefail

out="${1:?usage: build-iortcw-mac.sh <out-dir> [version]}"
version="${2:-dev}"
repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# Read plainly: the Mac's own bash, 3.2, cannot source a process
# substitution.
repo="$(sed -n 's/^repo=//p' "$repo_root/integration/iortcw/base.txt" | head -1)"
commit="$(sed -n 's/^commit=//p' "$repo_root/integration/iortcw/base.txt" | head -1)"
test -n "$repo" && test -n "$commit" || { echo "integration/iortcw/base.txt names no repo or commit" >&2; exit 1; }

work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT

git clone --quiet "$repo" "$work/iortcw"
git -C "$work/iortcw" checkout --quiet "$commit"
test "$(git -C "$work/iortcw" rev-parse HEAD)" = "$commit" \
    || { echo "iortcw is not at $commit" >&2; exit 1; }
for patch in "$repo_root"/integration/iortcw/*.patch; do
    git -C "$work/iortcw" apply "$patch"
done

cd "$work/iortcw/SP"
ncpu="$(sysctl -n hw.ncpu)"
for arch in arm64 x86_64; do
    # Apple Silicon Macs start at macOS 11; Intel ones as far back as the
    # speech's own calls go.
    if [ "$arch" = arm64 ]; then min=11.0; else min=10.13; fi
    ARCH="$arch" MACOSX_VERSION_MIN="$min" make -j"$ncpu" release \
        BUILD_SERVER=0 FEATURE_ACCESSIBILITY=1 \
        ETA_SOURCE_DIR="$repo_root" ETA_RELEASE_VERSION="$version"
done

# Joins whichever architectures were built into one universal app.
./make-macosx-app.sh release

app="build/release-darwin-universal/iowolfsp.app"
test -d "$app" || { echo "no iowolfsp.app was built" >&2; exit 1; }

for lib in "$app/Contents/MacOS/iowolfsp" "$app/Contents/MacOS/main/cgame.sp.dylib" "$app/Contents/MacOS/main/ui.sp.dylib"; do
    lipo -info "$lib"
    for arch in arm64 x86_64; do
        lipo "$lib" -verify_arch "$arch" || { echo "$lib has no $arch" >&2; exit 1; }
    done
done
# Listed to a file first: grep -q stops reading early, and under pipefail
# the listing it cut off fails the check.
nm "$app/Contents/MacOS/main/cgame.sp.dylib" >"$work/cgame-symbols.txt"
grep -q ETA_Speech_Say "$work/cgame-symbols.txt" \
    || { echo "the cgame has no speech in it" >&2; exit 1; }

# Signed for this machine only, as the linker does by itself for Apple
# Silicon; nothing here is notarized.
codesign --force --deep --sign - "$app"

mkdir -p "$out"
tar -czf "$out/iowolfsp.app.tar.gz" -C "$(dirname "$app")" iowolfsp.app
echo "built $out/iowolfsp.app.tar.gz"
