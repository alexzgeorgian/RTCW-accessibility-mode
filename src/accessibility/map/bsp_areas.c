#include "bsp_areas.h"

#include <stdlib.h>
#include <string.h>

/* The map format's own numbers (qfiles.h). */
#define ETA_BSP_IDENT        (('P' << 24) + ('S' << 16) + ('B' << 8) + 'I')
#define ETA_BSP_VERSION      47
#define ETA_BSP_LUMPS        17
#define ETA_BSP_PLANES       2
#define ETA_BSP_NODES        3
#define ETA_BSP_LEAFS        4
#define ETA_BSP_PLANE_SIZE   16     /* float normal[3], float dist */
#define ETA_BSP_NODE_SIZE    36     /* int planeNum, children[2], mins[3], maxs[3] */
#define ETA_BSP_LEAF_SIZE    48     /* int cluster, area, mins[3], maxs[3], 4 more */

/* Only turns away a header that is plainly not a map. */
#define ETA_SANE_COUNT       0x400000
#define ETA_MAX_AREAS        1024
#define ETA_TREE_DEPTH       4096   /* a walk this long is a broken tree */

typedef struct
{
    float normal[3];
    float dist;
} ETA_AreaPlane;

typedef struct
{
    int plane;
    int children[2];
} ETA_AreaNode;

static ETA_AreaPlane *gPlanes;
static ETA_AreaNode  *gNodes;
static short         *gLeafArea;
static int            gPlaneCount, gNodeCount, gLeafCount, gAreaCount;
static float          gAreaMins[ETA_MAX_AREAS][3], gAreaMaxs[ETA_MAX_AREAS][3];
static unsigned char  gAreaHasLeaf[ETA_MAX_AREAS];

typedef struct
{
    int offset;
    int length;
    int which;
} ETA_AreaLump;

static int ETA_Area_Int(const unsigned char *p)
{
    return (int)((unsigned int)p[0] | ((unsigned int)p[1] << 8) | ((unsigned int)p[2] << 16)
                 | ((unsigned int)p[3] << 24));
}

static float ETA_Area_Float(const unsigned char *p)
{
    unsigned int bits = (unsigned int)ETA_Area_Int(p);
    float        value;

    memcpy(&value, &bits, sizeof(value));
    return value;
}

static int ETA_Area_Read(ETA_BspRead read, void *context, void *buffer, int length, int *position)
{
    int done = 0;

    while (done < length)
    {
        int got = read(context, (unsigned char *)buffer + done, length - done);

        if (got <= 0)
        {
            return 0;
        }
        done += got;
    }
    *position += length;
    return 1;
}

/* Forward only: the file cannot be sought, so skipping is reading. */
static int ETA_Area_SkipTo(ETA_BspRead read, void *context, int offset, int *position)
{
    unsigned char scratch[4096];

    if (offset < *position)
    {
        return 0;
    }
    while (*position < offset)
    {
        int take = offset - *position;

        if (take > (int)sizeof(scratch))
        {
            take = (int)sizeof(scratch);
        }
        if (!ETA_Area_Read(read, context, scratch, take, position))
        {
            return 0;
        }
    }
    return 1;
}

static int ETA_Area_ByOffset(const void *a, const void *b)
{
    return ((const ETA_AreaLump *)a)->offset - ((const ETA_AreaLump *)b)->offset;
}

/* One table, read a record at a time into what is kept of it. */
static int ETA_Area_Table(ETA_BspRead read, void *context, const ETA_AreaLump *lump, int *position)
{
    unsigned char record[ETA_BSP_LEAF_SIZE];
    int           size = lump->which == ETA_BSP_PLANES ? ETA_BSP_PLANE_SIZE
                         : lump->which == ETA_BSP_NODES ? ETA_BSP_NODE_SIZE : ETA_BSP_LEAF_SIZE;
    int           count = lump->length / size, i, k;

    if (!ETA_Area_SkipTo(read, context, lump->offset, position))
    {
        return 0;
    }
    for (i = 0; i < count; i++)
    {
        if (!ETA_Area_Read(read, context, record, size, position))
        {
            return 0;
        }
        if (lump->which == ETA_BSP_PLANES)
        {
            for (k = 0; k < 3; k++)
            {
                gPlanes[i].normal[k] = ETA_Area_Float(record + 4 * k);
            }
            gPlanes[i].dist = ETA_Area_Float(record + 12);
        }
        else if (lump->which == ETA_BSP_NODES)
        {
            gNodes[i].plane       = ETA_Area_Int(record);
            gNodes[i].children[0] = ETA_Area_Int(record + 4);
            gNodes[i].children[1] = ETA_Area_Int(record + 8);
        }
        else
        {
            int area = ETA_Area_Int(record + 4);

            gLeafArea[i] = (short)(area >= 0 && area < ETA_MAX_AREAS ? area : -1);
            if (gLeafArea[i] < 0 || ETA_Area_Int(record) < 0)
            {
                gLeafArea[i] = -1;
                continue;
            }
            for (k = 0; k < 3; k++)
            {
                float lo = (float)ETA_Area_Int(record + 8 + 4 * k), hi = (float)ETA_Area_Int(record + 20 + 4 * k);

                if (!gAreaHasLeaf[area] || lo < gAreaMins[area][k])
                {
                    gAreaMins[area][k] = lo;
                }
                if (!gAreaHasLeaf[area] || hi > gAreaMaxs[area][k])
                {
                    gAreaMaxs[area][k] = hi;
                }
            }
            gAreaHasLeaf[area] = 1;
            if (area + 1 > gAreaCount)
            {
                gAreaCount = area + 1;
            }
        }
    }
    return 1;
}

int ETA_BspAreas_Load(ETA_BspRead read, ETA_BspRestart restart, void *context)
{
    unsigned char header[8 + 8 * ETA_BSP_LUMPS];
    ETA_AreaLump  lumps[3];
    int           position = 0, i;

    ETA_BspAreas_Free();
    if (!read || !restart)
    {
        return 0;
    }
    if (!ETA_Area_Read(read, context, header, (int)sizeof(header), &position)
        || ETA_Area_Int(header) != ETA_BSP_IDENT || ETA_Area_Int(header + 4) != ETA_BSP_VERSION)
    {
        return 0;
    }

    lumps[0].which = ETA_BSP_PLANES;
    lumps[1].which = ETA_BSP_NODES;
    lumps[2].which = ETA_BSP_LEAFS;
    for (i = 0; i < 3; i++)
    {
        lumps[i].offset = ETA_Area_Int(header + 8 + lumps[i].which * 8);
        lumps[i].length = ETA_Area_Int(header + 12 + lumps[i].which * 8);
        if (lumps[i].offset < (int)sizeof(header) || lumps[i].length <= 0)
        {
            return 0;
        }
    }
    gPlaneCount = lumps[0].length / ETA_BSP_PLANE_SIZE;
    gNodeCount  = lumps[1].length / ETA_BSP_NODE_SIZE;
    gLeafCount  = lumps[2].length / ETA_BSP_LEAF_SIZE;
    if (gPlaneCount <= 0 || gPlaneCount > ETA_SANE_COUNT || gNodeCount <= 0 || gNodeCount > ETA_SANE_COUNT
        || gLeafCount <= 0 || gLeafCount > ETA_SANE_COUNT)
    {
        gPlaneCount = gNodeCount = gLeafCount = 0;
        return 0;
    }

    gPlanes   = (ETA_AreaPlane *)calloc((size_t)gPlaneCount, sizeof(*gPlanes));
    gNodes    = (ETA_AreaNode *)calloc((size_t)gNodeCount, sizeof(*gNodes));
    gLeafArea = (short *)calloc((size_t)gLeafCount, sizeof(*gLeafArea));
    if (!gPlanes || !gNodes || !gLeafArea)
    {
        ETA_BspAreas_Free();
        return 0;
    }

    /* In whatever order the file has them. */
    qsort(lumps, 3, sizeof(lumps[0]), ETA_Area_ByOffset);
    for (i = 0; i < 3; i++)
    {
        if (!ETA_Area_Table(read, context, &lumps[i], &position))
        {
            ETA_BspAreas_Free();
            return 0;
        }
    }
    return gAreaCount;
}

int ETA_BspAreas_At(const float point[3])
{
    int index = 0, depth = 0;

    if (!gNodes)
    {
        return -1;
    }
    while (index >= 0)
    {
        const ETA_AreaPlane *plane;
        float                d;

        if (index >= gNodeCount || ++depth > ETA_TREE_DEPTH)
        {
            return -1;
        }
        if (gNodes[index].plane < 0 || gNodes[index].plane >= gPlaneCount)
        {
            return -1;
        }
        plane = &gPlanes[gNodes[index].plane];
        d     = plane->normal[0] * point[0] + plane->normal[1] * point[1] + plane->normal[2] * point[2] - plane->dist;
        index = gNodes[index].children[d >= 0.0f ? 0 : 1];
    }
    index = -1 - index;
    return index < gLeafCount ? gLeafArea[index] : -1;
}

int ETA_BspAreas_Count(void)
{
    return gAreaCount;
}

int ETA_BspAreas_Bounds(int area, float mins[3], float maxs[3])
{
    if (area < 0 || area >= gAreaCount || !gAreaHasLeaf[area])
    {
        return 0;
    }
    memcpy(mins, gAreaMins[area], sizeof(gAreaMins[area]));
    memcpy(maxs, gAreaMaxs[area], sizeof(gAreaMaxs[area]));
    return 1;
}

void ETA_BspAreas_Free(void)
{
    free(gPlanes);
    free(gNodes);
    free(gLeafArea);
    gPlanes     = NULL;
    gNodes      = NULL;
    gLeafArea   = NULL;
    gPlaneCount = gNodeCount = gLeafCount = gAreaCount = 0;
    memset(gAreaHasLeaf, 0, sizeof(gAreaHasLeaf));
}
