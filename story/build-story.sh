#!/usr/bin/env bash
# Builds the story campaign's levels into one pk3.
#
#   story/build-story.sh --rtcw <RTCW main folder> --q3map2 <q3map2> \
#                        --bspc <bspc> --out <folder>
#
# The RTCW main folder is the player's own game data (pak0.pk3,
# sp_pak1.pk3...): its textures and shaders are what the levels are
# compiled against. Nothing from it goes into the pk3, which holds only
# the levels themselves: each map's .bsp, its navigation file for the
# enemies (_b0.aas) and its AI script (.ai).
#
# q3map2 is GtkRadiant's, built for Wolfenstein ("-game wolf"). bspc is
# RTCW's own, from id Software's GPL release, built 32-bit: the 64-bit
# build crashes on reading a map. It reads a q3map2 map only with
# -forcesidesvisible, since q3map2's merged surfaces no longer match the
# brush sides one for one.

set -euo pipefail

here="$(cd "$(dirname "$0")" && pwd)"
rtcw="" q3map2="" bspc="" out=""
while [ $# -gt 0 ]; do
    case "$1" in
        --rtcw) rtcw="$2"; shift 2 ;;
        --q3map2) q3map2="$2"; shift 2 ;;
        --bspc) bspc="$2"; shift 2 ;;
        --out) out="$2"; shift 2 ;;
        *) echo "unknown option $1" >&2; exit 2 ;;
    esac
done
[ -n "$rtcw" ] && [ -n "$q3map2" ] && [ -n "$bspc" ] && [ -n "$out" ] || {
    sed -n '2,5p' "$0" >&2; exit 2; }
[ -f "$rtcw/pak0.pk3" ] || { echo "error: no pak0.pk3 in $rtcw" >&2; exit 1; }
mkdir -p "$out"
out="$(cd "$out" && pwd)"

work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT
mkdir -p "$work/stage/maps" "$work/home/.wolf/main/scripts" "$work/base/main"
for pak in "$rtcw"/*.pk3; do ln -s "$(cd "$(dirname "$pak")" && pwd)/$(basename "$pak")" "$work/base/main/"; done

# q3map2 only reads the shader files named in scripts/shaderlist.txt,
# and RTCW ships none: without it caulk is drawn and the sky is a wall.
# The menus' shader files are left out: no level uses them, and one of
# them, ui_wolf.shader, has a stray brace that stops q3map2 outright.
for pak in "$rtcw"/*.pk3; do unzip -Z1 "$pak"; done | grep '^scripts/.*\.shader$' \
    | sed 's|^scripts/||; s|\.shader$||' | grep -v '^ui' | sort -u > "$work/home/.wolf/main/scripts/shaderlist.txt"

q3map2() { HOME="$work/home" "$q3map2" -game wolf -fs_basepath "$work/base" -fs_game main "$@"; }

for level in "$here"/wl_*/; do
    name="$(basename "$level")"
    echo "==> $name"
    mkdir -p "$work/$name/maps"
    python3 "$level/make_map.py" "$work/$name/maps/$name.map"
    q3map2 -meta "$work/$name/maps/$name.map" > "$work/$name/bsp.log"
    q3map2 -vis "$work/$name/maps/$name.map" > "$work/$name/vis.log"
    q3map2 -light -fast -samples 2 "$work/$name/maps/$name.map" > "$work/$name/light.log"
    if grep -q "leaked" "$work/$name/bsp.log"; then
        echo "error: $name leaks" >&2; exit 1
    fi
    (cd "$work/$name/maps" && "$bspc" -forcesidesvisible -bsp2aas "$name.bsp" > /dev/null)
    [ -f "$work/$name/maps/$name.aas" ] || { echo "error: bspc made no navigation for $name" >&2; exit 1; }
    cp "$work/$name/maps/$name.bsp" "$work/stage/maps/"
    cp "$work/$name/maps/$name.aas" "$work/stage/maps/${name}_b0.aas"
    cp "$level/$name.ai" "$work/stage/maps/"
done

rm -f "$out/zzz_story.pk3"
(cd "$work/stage" && zip -qr "$out/zzz_story.pk3" maps)
echo "wrote $out/zzz_story.pk3"
