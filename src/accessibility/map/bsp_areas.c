#include "bsp_areas.h"

#include <math.h>
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
static int           *gLeafBox;     /* each leaf's mins and maxs, 6 to a leaf */
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
            for (k = 0; k < 6; k++)
            {
                gLeafBox[i * 6 + k] = ETA_Area_Int(record + 8 + 4 * k);
            }
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
    gLeafBox  = (int *)calloc((size_t)gLeafCount * 6, sizeof(*gLeafBox));
    if (!gPlanes || !gNodes || !gLeafArea || !gLeafBox)
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

static int *gRoot;

static int ETA_Area_Root(int i)
{
    while (gRoot[i] != i)
    {
        gRoot[i] = gRoot[gRoot[i]];
        i        = gRoot[i];
    }
    return i;
}

static int *gSortBox;

static int ETA_Area_ByMinX(const void *a, const void *b)
{
    int ia = *(const int *)a, ib = *(const int *)b;

    return gSortBox[ia * 6] - gSortBox[ib * 6];
}

int ETA_BspAreas_SplitAtDoors(const float (*boxes)[6], int count)
{
    int  *order, *size, *room, i, j, k, used = 0, rooms = 0;
    char *door;

    if (!gLeafArea || !gLeafBox || gLeafCount <= 0)
    {
        return gAreaCount;
    }
    order = (int *)calloc((size_t)gLeafCount, sizeof(*order));
    size  = (int *)calloc((size_t)gLeafCount, sizeof(*size));
    room  = (int *)calloc((size_t)gLeafCount, sizeof(*room));
    door  = (char *)calloc((size_t)gLeafCount, sizeof(*door));
    gRoot = (int *)calloc((size_t)gLeafCount, sizeof(*gRoot));
    if (!order || !size || !room || !door || !gRoot)
    {
        free(order);
        free(size);
        free(room);
        free(door);
        free(gRoot);
        gRoot = NULL;
        return gAreaCount;
    }

    /* The leaves to join: open, of some size, and clear of the doors. */
    for (i = 0; i < gLeafCount; i++)
    {
        const int *b = &gLeafBox[i * 6];

        gRoot[i] = i;
        if (gLeafArea[i] < 0 || ( b[0] == b[3] && b[1] == b[4] && b[2] == b[5] ))
        {
            continue;
        }
        for (k = 0; k < count; k++)
        {
            if (b[0] <= boxes[k][3] && boxes[k][0] <= b[3] && b[1] <= boxes[k][4] && boxes[k][1] <= b[4]
                && b[2] <= boxes[k][5] && boxes[k][2] <= b[5])
            {
                door[i] = 1;
                break;
            }
        }
        if (!door[i])
        {
            order[used++] = i;
        }
    }

    /* Touching leaves of the same area are one room: along x in order,
       so each is held only against those that could reach it. */
    gSortBox = gLeafBox;
    qsort(order, (size_t)used, sizeof(*order), ETA_Area_ByMinX);
    for (i = 0; i < used; i++)
    {
        const int *a = &gLeafBox[order[i] * 6];

        for (j = i + 1; j < used; j++)
        {
            const int *b = &gLeafBox[order[j] * 6];

            if (b[0] > a[3] + 1)
            {
                break;
            }
            if (gLeafArea[order[i]] != gLeafArea[order[j]])
            {
                continue;
            }
            if (b[1] <= a[4] + 1 && a[1] <= b[4] + 1 && b[2] <= a[5] + 1 && a[2] <= b[5] + 1)
            {
                gRoot[ETA_Area_Root(order[i])] = ETA_Area_Root(order[j]);
            }
        }
    }
    for (i = 0; i < used; i++)
    {
        size[ETA_Area_Root(order[i])]++;
    }

    /* Numbered in the order the leaves come, so the same map gives the
       same rooms each time. */
    for (i = 0; i < gLeafCount; i++)
    {
        room[i] = -1;
    }
    for (i = 0; i < gLeafCount; i++)
    {
        int r;

        if (door[i] || gLeafArea[i] < 0)
        {
            continue;
        }
        r = ETA_Area_Root(i);
        if (size[r] < 3)
        {
            continue;
        }
        if (room[r] < 0)
        {
            if (rooms >= ETA_MAX_AREAS)
            {
                continue;
            }
            room[r] = rooms++;
        }
        room[i] = room[r];
    }

    memset(gAreaHasLeaf, 0, sizeof(gAreaHasLeaf));
    for (i = 0; i < gLeafCount; i++)
    {
        int r = gLeafArea[i] >= 0 && !door[i] ? room[ETA_Area_Root(i)] : -1;

        gLeafArea[i] = (short)r;
        if (r < 0)
        {
            continue;
        }
        for (k = 0; k < 3; k++)
        {
            float lo = (float)gLeafBox[i * 6 + k], hi = (float)gLeafBox[i * 6 + 3 + k];

            if (!gAreaHasLeaf[r] || lo < gAreaMins[r][k])
            {
                gAreaMins[r][k] = lo;
            }
            if (!gAreaHasLeaf[r] || hi > gAreaMaxs[r][k])
            {
                gAreaMaxs[r][k] = hi;
            }
        }
        gAreaHasLeaf[r] = 1;
    }
    gAreaCount = rooms;

    free(order);
    free(size);
    free(room);
    free(door);
    free(gRoot);
    gRoot = NULL;
    return gAreaCount;
}

/* The bots' file: the header after its first eight bytes is scrambled
   (be_aas_file.c, AAS_DData). */
#define ETA_AAS_IDENT     (('S' << 24) + ('A' << 16) + ('A' << 8) + 'E')
#define ETA_AAS_VERSION   8
#define ETA_AAS_LUMPS     14
#define ETA_AAS_PLANES    2
#define ETA_AAS_AREAS     7
#define ETA_AAS_SETTINGS  8
#define ETA_AAS_NODES     10
#define ETA_AAS_PORTALS   11

typedef struct
{
    float mins[3], maxs[3], center[3];
    int   cluster;      /* from 1; negative, a portal */
    int   flags;        /* AREA_GROUNDED 1: a floor to stand on */
} ETA_AasArea;

static int            gAas;
static ETA_AreaPlane *gAasPlanes;
static int           *gAasNodes;    /* planenum and two children, 3 to a node */
static ETA_AasArea   *gAasAreas;
static int           *gAasPortals;  /* area, front and back cluster, 3 to a portal */
static int            gAasPlaneCount, gAasNodeCount, gAasAreaCount, gAasPortalCount;

static void ETA_Aas_Free(void)
{
    free(gAasPlanes);
    free(gAasNodes);
    free(gAasAreas);
    free(gAasPortals);
    gAasPlanes  = NULL;
    gAasNodes   = NULL;
    gAasAreas   = NULL;
    gAasPortals = NULL;
    gAasPlaneCount = gAasNodeCount = gAasAreaCount = gAasPortalCount = 0;
    gAas = 0;
}

static int ETA_Aas_Boxes(void);

int ETA_BspAreas_LoadAas(ETA_BspRead read, ETA_BspRestart restart, void *context)
{
    unsigned char header[12 + 8 * ETA_AAS_LUMPS];
    int           offset[ETA_AAS_LUMPS], length[ETA_AAS_LUMPS], order[5], position = 0, i, j, k, settings = 0;
    unsigned char record[48];

    (void)restart;
    ETA_Aas_Free();
    if (!read || !ETA_Area_Read(read, context, header, (int)sizeof(header), &position))
    {
        return 0;
    }
    for (i = 8; i < (int)sizeof(header); i++)
    {
        header[i] ^= (unsigned char)((i - 8) * 119);
    }
    if (ETA_Area_Int(header) != ETA_AAS_IDENT || ETA_Area_Int(header + 4) != ETA_AAS_VERSION)
    {
        return 0;
    }
    for (i = 0; i < ETA_AAS_LUMPS; i++)
    {
        offset[i] = ETA_Area_Int(header + 12 + i * 8);
        length[i] = ETA_Area_Int(header + 16 + i * 8);
        if (offset[i] < 0 || length[i] < 0)
        {
            return 0;
        }
    }
    gAasPlaneCount  = length[ETA_AAS_PLANES] / 20;
    gAasAreaCount   = length[ETA_AAS_AREAS] / 48;
    gAasNodeCount   = length[ETA_AAS_NODES] / 12;
    gAasPortalCount = length[ETA_AAS_PORTALS] / 20;
    if (gAasPlaneCount <= 0 || gAasAreaCount <= 1 || gAasNodeCount <= 1 || length[ETA_AAS_SETTINGS] / 32 != gAasAreaCount
        || gAasPlaneCount > ETA_SANE_COUNT || gAasAreaCount > ETA_SANE_COUNT || gAasNodeCount > ETA_SANE_COUNT
        || gAasPortalCount > ETA_SANE_COUNT)
    {
        ETA_Aas_Free();
        return 0;
    }
    gAasPlanes  = (ETA_AreaPlane *)calloc((size_t)gAasPlaneCount, sizeof(*gAasPlanes));
    gAasNodes   = (int *)calloc((size_t)gAasNodeCount * 3, sizeof(*gAasNodes));
    gAasAreas   = (ETA_AasArea *)calloc((size_t)gAasAreaCount, sizeof(*gAasAreas));
    gAasPortals = (int *)calloc((size_t)(gAasPortalCount ? gAasPortalCount : 1) * 3, sizeof(*gAasPortals));
    if (!gAasPlanes || !gAasNodes || !gAasAreas || !gAasPortals)
    {
        ETA_Aas_Free();
        return 0;
    }

    /* In whatever order the file has them. */
    order[0] = ETA_AAS_PLANES;
    order[1] = ETA_AAS_AREAS;
    order[2] = ETA_AAS_SETTINGS;
    order[3] = ETA_AAS_NODES;
    order[4] = ETA_AAS_PORTALS;
    for (i = 0; i < 5; i++)
    {
        for (j = i + 1; j < 5; j++)
        {
            if (offset[order[j]] < offset[order[i]])
            {
                int t = order[i];

                order[i] = order[j];
                order[j] = t;
            }
        }
    }
    for (i = 0; i < 5; i++)
    {
        int lump = order[i];
        int size = lump == ETA_AAS_PLANES || lump == ETA_AAS_PORTALS ? 20 : lump == ETA_AAS_AREAS ? 48
                   : lump == ETA_AAS_SETTINGS ? 32 : 12;
        int count = length[lump] / size;

        if (!ETA_Area_SkipTo(read, context, offset[lump], &position))
        {
            ETA_Aas_Free();
            return 0;
        }
        for (j = 0; j < count; j++)
        {
            if (!ETA_Area_Read(read, context, record, size, &position))
            {
                ETA_Aas_Free();
                return 0;
            }
            switch (lump)
            {
            case ETA_AAS_PLANES:
                for (k = 0; k < 3; k++)
                {
                    gAasPlanes[j].normal[k] = ETA_Area_Float(record + 4 * k);
                }
                gAasPlanes[j].dist = ETA_Area_Float(record + 12);
                break;
            case ETA_AAS_AREAS:
                for (k = 0; k < 3; k++)
                {
                    gAasAreas[j].mins[k]   = ETA_Area_Float(record + 12 + 4 * k);
                    gAasAreas[j].maxs[k]   = ETA_Area_Float(record + 24 + 4 * k);
                    gAasAreas[j].center[k] = ETA_Area_Float(record + 36 + 4 * k);
                }
                break;
            case ETA_AAS_SETTINGS:
                gAasAreas[j].flags   = ETA_Area_Int(record + 4);
                gAasAreas[j].cluster = ETA_Area_Int(record + 12);
                break;
            case ETA_AAS_NODES:
                gAasNodes[j * 3]     = ETA_Area_Int(record);
                gAasNodes[j * 3 + 1] = ETA_Area_Int(record + 4);
                gAasNodes[j * 3 + 2] = ETA_Area_Int(record + 8);
                break;
            default:
                gAasPortals[j * 3]     = ETA_Area_Int(record);
                gAasPortals[j * 3 + 1] = ETA_Area_Int(record + 4);
                gAasPortals[j * 3 + 2] = ETA_Area_Int(record + 8);
                break;
            }
        }
    }

    settings = ETA_Aas_Boxes();
    if (!settings)
    {
        ETA_Aas_Free();
        return 0;
    }
    gAas = 1;
    return gAreaCount;
}

/* The clusters are the areas, with their boxes. Returns how many pieces
   of the bots' file are in one. */
static int ETA_Aas_Boxes(void)
{
    int j, k, settings = 0;

    memset(gAreaHasLeaf, 0, sizeof(gAreaHasLeaf));
    gAreaCount = 0;
    for (j = 1; j < gAasAreaCount; j++)
    {
        int area = gAasAreas[j].cluster - 1;

        if (area < 0 || area >= ETA_MAX_AREAS)
        {
            continue;
        }
        for (k = 0; k < 3; k++)
        {
            if (!gAreaHasLeaf[area] || gAasAreas[j].mins[k] < gAreaMins[area][k])
            {
                gAreaMins[area][k] = gAasAreas[j].mins[k];
            }
            if (!gAreaHasLeaf[area] || gAasAreas[j].maxs[k] > gAreaMaxs[area][k])
            {
                gAreaMaxs[area][k] = gAasAreas[j].maxs[k];
            }
        }
        gAreaHasLeaf[area] = 1;
        if (area + 1 > gAreaCount)
        {
            gAreaCount = area + 1;
        }
        settings++;
    }
    return settings;
}

static int ETA_Aas_Root(int *root, int c)
{
    while (root[c] != c)
    {
        root[c] = root[root[c]];
        c       = root[c];
    }
    return c;
}

/* How far a point is from a box, 0 inside it. */
static float ETA_Aas_ToBox(const float at[3], const float box[6])
{
    float d2 = 0.0f;
    int   k;

    for (k = 0; k < 3; k++)
    {
        float out = at[k] < box[k] ? box[k] - at[k] : at[k] > box[k + 3] ? at[k] - box[k + 3] : 0.0f;

        d2 += out * out;
    }
    return d2 > 0.0f ? (float)sqrt(d2) : 0.0f;
}

int ETA_BspAreas_MergeSmall(const float (*doors)[6], int doorCount, float minFloor)
{
    int   *root;
    float *floor;
    int   clusters = 0, i, j, pass, count;

    if (!gAas)
    {
        return gAreaCount;
    }
    for (i = 1; i < gAasAreaCount; i++)
    {
        clusters = gAasAreas[i].cluster > clusters ? gAasAreas[i].cluster : clusters;
    }
    for (i = 0; i < gAasPortalCount; i++)
    {
        clusters = gAasPortals[i * 3 + 1] > clusters ? gAasPortals[i * 3 + 1] : clusters;
        clusters = gAasPortals[i * 3 + 2] > clusters ? gAasPortals[i * 3 + 2] : clusters;
    }
    if (clusters <= 1)
    {
        return gAreaCount;
    }
    root  = (int *)calloc((size_t)clusters + 1, sizeof(*root));
    floor = (float *)calloc((size_t)clusters + 1, sizeof(*floor));
    if (!root || !floor)
    {
        free(root);
        free(floor);
        return gAreaCount;
    }
    for (i = 0; i <= clusters; i++)
    {
        root[i] = i;
    }
    for (i = 1; i < gAasAreaCount; i++)
    {
        const ETA_AasArea *a = &gAasAreas[i];

        if (a->cluster > 0 && ( a->flags & 1 ))
        {
            floor[a->cluster] += ( a->maxs[0] - a->mins[0] ) * ( a->maxs[1] - a->mins[1] );
        }
    }

    /* A piece too small to be a room - a stair, a ladder's shaft, a
       landing between two doorways - goes into the biggest room it opens
       onto with no door between. Again, for pieces that open only onto
       other small ones. */
    for (pass = 0; pass < 3; pass++)
    {
        for (j = 1; j <= clusters; j++)
        {
            int   c = ETA_Aas_Root(root, j), into = 0;
            float intoFloor = 0.0f;

            if (c != j || floor[c] <= 0.0f || floor[c] >= minFloor)
            {
                continue;
            }
            for (i = 0; i < gAasPortalCount; i++)
            {
                int   area = gAasPortals[i * 3], front, back, other, d;
                float nearest = 1e9f;

                if (area <= 0 || area >= gAasAreaCount)
                {
                    continue;
                }
                front = ETA_Aas_Root(root, gAasPortals[i * 3 + 1]);
                back  = ETA_Aas_Root(root, gAasPortals[i * 3 + 2]);
                if (( front != c && back != c ) || front == back || front <= 0 || back <= 0)
                {
                    continue;
                }
                for (d = 0; d < doorCount; d++)
                {
                    float far = ETA_Aas_ToBox(gAasAreas[area].center, doors[d]);

                    nearest = far < nearest ? far : nearest;
                }
                if (nearest < 128.0f)
                {
                    continue;   /* a door between: the rooms stay two */
                }
                other = front == c ? back : front;
                if (!into || floor[other] > intoFloor)
                {
                    into      = other;
                    intoFloor = floor[other];
                }
            }
            if (into)
            {
                root[c]      = into;
                floor[into] += floor[c];
            }
        }
    }

    /* Numbered again from 1, in the file's order, with no gaps. */
    {
        int *number = (int *)calloc((size_t)clusters + 1, sizeof(*number));
        int  next   = 0;

        if (number)
        {
            for (j = 1; j <= clusters; j++)
            {
                int r = ETA_Aas_Root(root, j);

                if (!number[r])
                {
                    number[r] = ++next;
                }
            }
            for (i = 1; i < gAasAreaCount; i++)
            {
                if (gAasAreas[i].cluster > 0 && gAasAreas[i].cluster <= clusters)
                {
                    gAasAreas[i].cluster = number[ETA_Aas_Root(root, gAasAreas[i].cluster)];
                }
            }
            for (i = 0; i < gAasPortalCount; i++)
            {
                for (j = 1; j <= 2; j++)
                {
                    int c = gAasPortals[i * 3 + j];

                    if (c > 0 && c <= clusters)
                    {
                        gAasPortals[i * 3 + j] = number[ETA_Aas_Root(root, c)];
                    }
                }
            }
            free(number);
        }
    }
    free(root);
    free(floor);
    ETA_Aas_Boxes();
    count = gAreaCount;
    return count;
}

/* The bots' area a point is in: their tree, as AAS_PointAreaNum walks it.
   0 for solid. */
static int ETA_Aas_AreaAt(const float point[3])
{
    int node = 1, depth = 0;

    while (node > 0)
    {
        const ETA_AreaPlane *plane;
        float                d;

        if (node >= gAasNodeCount || ++depth > ETA_TREE_DEPTH)
        {
            return 0;
        }
        if (gAasNodes[node * 3] < 0 || gAasNodes[node * 3] >= gAasPlaneCount)
        {
            return 0;
        }
        plane = &gAasPlanes[gAasNodes[node * 3]];
        d     = plane->normal[0] * point[0] + plane->normal[1] * point[1] + plane->normal[2] * point[2] - plane->dist;
        node  = gAasNodes[node * 3 + (d > 0.0f ? 1 : 2)];
    }
    return node < 0 && -node < gAasAreaCount ? -node : 0;
}

/* A point's area by the bots' file: where a player standing there would
   be - an item on the floor is lower than a player's middle - else the
   nearest piece within two meters. A portal is between areas: -1. */
static int ETA_Aas_At(const float point[3])
{
    static const float lift[] = { 0.0f, 24.0f, 40.0f, 8.0f, 56.0f, -16.0f };
    float              best = 64.0f;
    int                i, k, found = -1;

    for (i = 0; i < (int)(sizeof(lift) / sizeof(lift[0])); i++)
    {
        float p[3];
        int   a;

        p[0] = point[0];
        p[1] = point[1];
        p[2] = point[2] + lift[i];
        a    = ETA_Aas_AreaAt(p);
        if (a > 0)
        {
            return gAasAreas[a].cluster > 0 ? gAasAreas[a].cluster - 1 : -1;
        }
    }
    for (i = 1; i < gAasAreaCount; i++)
    {
        float d2 = 0.0f, d;

        if (gAasAreas[i].cluster <= 0)
        {
            continue;
        }
        for (k = 0; k < 3; k++)
        {
            float out = point[k] < gAasAreas[i].mins[k] ? gAasAreas[i].mins[k] - point[k]
                        : point[k] > gAasAreas[i].maxs[k] ? point[k] - gAasAreas[i].maxs[k] : 0.0f;

            d2 += out * out;
        }
        d = d2 > 0.0f ? (float)sqrt(d2) : 0.0f;
        if (d < best)
        {
            best  = d;
            found = gAasAreas[i].cluster - 1;
        }
    }
    return found;
}

int ETA_BspAreas_PortalCount(void)
{
    return gAas ? gAasPortalCount : 0;
}

int ETA_BspAreas_Portal(int index, float at[3], int *front, int *back)
{
    int area;

    if (!gAas || index < 0 || index >= gAasPortalCount)
    {
        return 0;
    }
    area = gAasPortals[index * 3];
    if (area <= 0 || area >= gAasAreaCount)
    {
        return 0;
    }
    memcpy(at, gAasAreas[area].center, sizeof(gAasAreas[area].center));
    *front = gAasPortals[index * 3 + 1] - 1;
    *back  = gAasPortals[index * 3 + 2] - 1;
    return *front >= 0 && *back >= 0 && *front != *back;
}

int ETA_BspAreas_PointIn(int area, const float near[3], float out[3])
{
    float best = 0.0f;
    int   i, k, found = 0;

    if (!gAas)
    {
        return 0;
    }
    for (i = 1; i < gAasAreaCount; i++)
    {
        float d2 = 0.0f;

        if (gAasAreas[i].cluster - 1 != area)
        {
            continue;
        }
        for (k = 0; k < 3; k++)
        {
            d2 += (gAasAreas[i].center[k] - near[k]) * (gAasAreas[i].center[k] - near[k]);
        }
        if (!found || d2 < best)
        {
            best  = d2;
            found = 1;
            memcpy(out, gAasAreas[i].center, sizeof(gAasAreas[i].center));
        }
    }
    return found;
}

int ETA_BspAreas_At(const float point[3])
{
    int index = 0, depth = 0;

    if (gAas)
    {
        return ETA_Aas_At(point);
    }
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
    ETA_Aas_Free();
    free(gPlanes);
    free(gNodes);
    free(gLeafArea);
    free(gLeafBox);
    gPlanes     = NULL;
    gNodes      = NULL;
    gLeafArea   = NULL;
    gLeafBox    = NULL;
    gPlaneCount = gNodeCount = gLeafCount = gAreaCount = 0;
    memset(gAreaHasLeaf, 0, sizeof(gAreaHasLeaf));
}
