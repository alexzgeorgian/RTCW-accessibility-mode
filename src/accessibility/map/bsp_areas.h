#pragma once

#include "bsp_ladders.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The areas a map is made of, read from the map file itself.
 *
 * The map compiler cuts every map into areas: the stretches of floor and
 * room that run into each other with nothing between, split only where a
 * door stands that the compiler was told closes the way (an area
 * portal). So an area is a part of the level with a door, or several,
 * out of it - which is what the tester asked for, after the shapes of
 * the ways and the numbered places had them lost: a level as parts to
 * explore one at a time, as 1428 has them.
 *
 * Finding the area a point is in walks the map's own tree of planes down
 * to the leaf holding it, as the engine does; each leaf says its area.
 * Only the planes, the tree's nodes and its leaves are kept.
 *
 * Little-endian, as the map files are and as every platform this mod
 * builds for is.
 */

/* Reads the map's tree, replacing any read before. Returns how many areas
   the map has: 0 for a file that is not a map this engine would load. */
int ETA_BspAreas_Load(ETA_BspRead read, ETA_BspRestart restart, void *context);

/* The area a point is in, or -1 inside something solid or with no map. */
int ETA_BspAreas_At(const float point[3]);

/* How many areas the map read last has. */
int ETA_BspAreas_Count(void);

/* The box every leaf of an area fits in. 0 for an area with none. */
int ETA_BspAreas_Bounds(int area, float mins[3], float maxs[3]);

/*
 * Cuts the areas again at every door, not only those the compiler was
 * told close the way: count boxes, each mins then maxs, of the doors as
 * they stand closed. A map's area can be a whole floor of rooms, which
 * the tester found too much to go through as one. The map's leaves that
 * touch, in the same area and clear of every door, are a room; a room of
 * fewer than three is none, as the leaves in a doorway are. The rooms
 * are the areas from then on. Returns how many there are.
 */
int ETA_BspAreas_SplitAtDoors(const float (*boxes)[6], int count);

/* Lets go of the map read last. */
void ETA_BspAreas_Free(void);

#ifdef __cplusplus
}
#endif
