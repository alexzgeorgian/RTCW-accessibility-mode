#!/usr/bin/env bash
# Builds the two map tools the story levels need, from pinned sources:
#
#   q3map2 - GtkRadiant's map compiler, which knows Wolfenstein's BSP;
#   bspc   - RTCW's own navigation builder for enemies, from id Software's
#            GPL release of the single-player code, built 32-bit (the
#            64-bit build crashes reading a map: it was written for
#            32-bit pointers).
#
#   story/build-tools.sh <output folder>
#
# Needs gcc with 32-bit support (gcc-multilib), and the development
# packages of glib, libxml2, libpng and libjpeg.

set -euo pipefail

out="${1:?usage: build-tools.sh <output folder>}"
mkdir -p "$out"
out="$(cd "$out" && pwd)"

radiant_repo=https://github.com/TTimo/GtkRadiant.git
radiant_commit=270af88f3c2471f6773bded0b5760a3115b52965
rtcw_repo=https://github.com/id-Software/RTCW-SP.git
rtcw_commit=70951bc71b730efe6bcb07db7ae76ef0c4ae7c14

work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT

fetch() {
    git init -q "$2"
    git -C "$2" fetch -q --depth 1 "$1" "$3"
    git -C "$2" checkout -q FETCH_HEAD
}

echo "==> q3map2"
fetch "$radiant_repo" "$work/radiant" "$radiant_commit"
cd "$work/radiant"
# C23 made bool a keyword; the header still typedefs it for C.
sed -i 's|^typedef qboolean bool; // some code uses bool directly ..|#if !defined( __bool_true_false_are_defined ) \&\& !defined( bool )\ntypedef qboolean bool; // some code uses bool directly ..\n#endif|' libs/bytebool.h
q3map2_src=""
for f in bspfile_abstract bspfile_ibsp bspfile_rbsp image main mesh minimap model path_init \
         shaders surface_extra brush brush_primit bsp bsp_analyze bsp_scale bsp_info decals \
         facebsp fixaas fog leakfile map patch portals prtfile surface surface_foliage \
         surface_fur surface_meta tjunction tree writebsp light light_bounce light_trace \
         light_ydnar lightmaps_ydnar exportents vis visflow convert_ase convert_bsp convert_map; do
    q3map2_src="$q3map2_src tools/quake3/q3map2/$f.c"
done
for f in jpeg cmdlib imagelib inout mutex polylib scriplib threads unzip vfs; do
    q3map2_src="$q3map2_src tools/quake3/common/$f.c"
done
# shellcheck disable=SC2086
gcc -std=gnu99 -O2 -w -fcommon -DPOSIX -DXWINDOWS -D_GNU_SOURCE \
    -Itools/quake3/common -Ilibs -Iinclude \
    $(pkg-config --cflags glib-2.0 libxml-2.0 libpng) \
    -o "$out/q3map2" $q3map2_src \
    libs/picomodel/*.c libs/picomodel/lwo/*.c libs/ddslib/*.c libs/mathlib/*.c \
    libs/l_net/l_net.c libs/l_net/l_net_berkeley.c libs/md5lib/*.c \
    $(pkg-config --libs glib-2.0 libxml-2.0 libpng) -ljpeg -lm -lpthread

echo "==> bspc"
fetch "$rtcw_repo" "$work/rtcw" "$rtcw_commit"
cd "$work/rtcw/src"
# An increment of a cast, which compilers since have refused.
sed -i 's|checksum = ( checksum << 4 ) ^ \*( (char \*)buffer )++;|{ checksum = ( checksum << 4 ) ^ *(char *)buffer; buffer = (char *)buffer + 1; }|' bspc/l_bsp_hl.c
# shellcheck disable=SC2046
gcc -std=gnu89 -O1 -w -fcommon -m32 -DBSPC -Dstricmp=strcasecmp -DLINUX -o "$out/bspc" \
    $(ls bspc/*.c | grep -v _files.c) \
    botlib/be_aas_bspq3.c botlib/be_aas_cluster.c botlib/be_aas_move.c botlib/be_aas_optimize.c \
    botlib/be_aas_reach.c botlib/be_aas_sample.c botlib/l_libvar.c botlib/l_precomp.c \
    botlib/l_script.c botlib/l_struct.c qcommon/cm_load.c qcommon/cm_patch.c qcommon/cm_test.c \
    qcommon/cm_trace.c qcommon/md4.c qcommon/unzip.c -lm -lpthread

echo "wrote $out/q3map2 and $out/bspc"
