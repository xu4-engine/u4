/*
 * btree2.c
 * Written and dedicated to the public domain in 2022 by Karl Robillard.
 *
 * Generate a static binary space partition for 2D, axis aligned boxes.
 */

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifndef BTREE2_DIM
#define BTREE2_DIM  uint16_t
#endif
#ifndef BTREE2_DATA
#define BTREE2_DATA int
#endif
#ifndef BTREE2_LEAF_SIZE
#define BTREE2_LEAF_SIZE    4
#endif

typedef BTREE2_DIM  bt2dim_t;
typedef BTREE2_DATA bt2data_t;

// BTree2Split flags
#define BTREE2_AXIS_X   1
#define BTREE2_L_LEAF   2
#define BTREE2_H_LEAF   4

typedef struct {
    uint16_t flags;
    uint8_t  countL;
    uint8_t  countH;
    uint16_t indexL;    // Index into split or leaves array.
    uint16_t indexH;    // Index into split or leaves array.
    bt2dim_t center;
}
BTree2Split;

typedef struct {
    bt2dim_t  x, y;     // Minimum
    bt2dim_t  x2, y2;   // Maxiumum (x + width, y + height)
    bt2data_t data;
}
BTree2Box;

typedef struct {
    uint16_t splitCount;
    uint16_t leavesSize;
    BTree2Split split;      // split[splitCount]
    // uint16_t leaves[leavesSize];
    // BTree2Box boxes[boxCount];
}
BTree2;

#define BTREE2_SPLIT(bt)    &bt->split
#define BTREE2_LEAVES(bt)   ((uint16_t*) (&bt->split + bt->splitCount))
#define BTREE2_BYTES(bt) \
    (4 + sizeof(BTree2Split)*bt->splitCount + sizeof(uint16_t)*bt->leavesSize)

typedef struct {
    const BTree2Box* inbox;
    uint16_t* leaves;           // Box index array for each leaf.
    BTree2Split* split;
    int inCount;
    int leavesSize;
    int splitCount;
}
BTree2Gen;

BTree2* btree2_generate(BTree2Gen*, const BTree2Box* inbox, int boxCount);
const BTree2Box* btree2_pick(const BTree2*, const BTree2Box*,
                             bt2dim_t x, bt2dim_t y);


#ifndef BTREE2_PICK_ONLY
#define ALLOC(T,N)  (T*) malloc(sizeof(T) * N)

static int btree2_intersects(const BTree2Box* a, const BTree2Box* b)
{
    return (a->x < b->x2 && a->x2 > b->x &&
            a->y < b->y2 && a->y2 > b->y);
}

static int btree2_partition(BTree2Gen* gen, const BTree2Box* bound,
                            const uint16_t* list, int listSize)
{
    BTree2Box subL, subH;
    int dx = bound->x2 - bound->x;
    int dy = bound->y2 - bound->y;
    int si = gen->splitCount++;
    BTree2Split sp;

    subL = *bound;
    subH = *bound;

    // Using a simple bisect partitioning scheme.

    if (dx > dy) {
        sp.flags  = BTREE2_AXIS_X;
        sp.center = bound->x + dx/2;

        subL.x2 = sp.center;
        subH.x  = sp.center;
    } else {
        sp.flags  = 0;
        sp.center = bound->y + dy/2;

        subL.y2 = sp.center;
        subH.y  = sp.center;
    }

    sp.countL = sp.countH = 0;
    sp.indexL = sp.indexH = 0;

    {
    int part, i, inCount;
    BTree2Box* sub;
    uint16_t* inside = ALLOC(uint16_t, listSize);
    uint16_t* inp;

    for (part = 0; part < 2; ++part) {
        sub = part ? &subH : &subL;
        for (inp = inside, i = 0; i < listSize; ++i) {
            if (btree2_intersects(sub, gen->inbox + list[i]))
                *inp++ = list[i];
        }

        inCount = inp - inside;
        if (inCount <= BTREE2_LEAF_SIZE) {
            // Leaf node reached.
            int li = gen->leavesSize;

            if (part) {
                sp.flags |= BTREE2_H_LEAF;
                sp.countH = inCount;
                sp.indexH = li;
            } else {
                sp.flags |= BTREE2_L_LEAF;
                sp.countL = inCount;
                sp.indexL = li;
            }

            for (i = 0; i < inCount; ++i)
                gen->leaves[li++] = inside[i];

            gen->leavesSize = li;
        } else {
            // Sub-partition.
            int index = btree2_partition(gen, sub, inside, inCount);
            if (part)
                sp.indexH = index;
            else
                sp.indexL = index;
        }
    }

    free(inside);
    }

    gen->split[si] = sp;
    return si;
}

/*
 * Return BTree2 pointer which caller must free().
 */
BTree2* btree2_generate(BTree2Gen* gen, const BTree2Box* inbox, int boxCount)
{
    BTree2Box bound;
    BTree2* hdr;
    const BTree2Box* box = inbox;
    uint16_t* inside;
    int i;

    // Calculate bounding box.
    bound = *box++;
    for (i = 1; i < boxCount; ++box, ++i) {
        if (bound.x > box->x)
            bound.x = box->x;
        if (bound.y > box->y)
            bound.y = box->y;
        if (bound.x2 < box->x2)
            bound.x2 = box->x2;
        if (bound.y2 < box->y2)
            bound.y2 = box->y2;
    }

    gen->inbox = inbox;
    gen->leaves = ALLOC(uint16_t, boxCount * 2);
    gen->split = ALLOC(BTree2Split, boxCount / 2);
    gen->inCount = boxCount;
    gen->leavesSize = 0;
    gen->splitCount = 0;

    inside = ALLOC(uint16_t, boxCount);
    for (i = 0; i < boxCount; ++i)
        inside[i] = i;
    btree2_partition(gen, &bound, inside, boxCount);
    free(inside);

    assert(gen->leavesSize <= boxCount * 2);
    assert(gen->splitCount <= boxCount / 2);

    // Transfer generator buffers to a minimally sized, single chunk of memory.
    {
    size_t sizeSpl = sizeof(BTree2Split) * gen->splitCount;
    size_t sizeLvs = sizeof(uint16_t)    * gen->leavesSize;
    hdr = (BTree2*) malloc(4 + sizeSpl + sizeLvs);
    hdr->splitCount = gen->splitCount;
    hdr->leavesSize = gen->leavesSize;
    memcpy(BTREE2_SPLIT(hdr), gen->split, sizeSpl);
    memcpy(BTREE2_LEAVES(hdr), gen->leaves, sizeLvs);
    }

    free(gen->leaves);
    free(gen->split);
    return hdr;
}
#endif

const BTree2Box* btree2_pick(const BTree2* tree, const BTree2Box* boxes,
                             bt2dim_t x, bt2dim_t y)
{
    const BTree2Split* split = BTREE2_SPLIT(tree);
    const BTree2Split* sp = split;
    int part, leafIndex, bc;

    while (1) {
        part = 0;
        if (sp->flags & BTREE2_AXIS_X) {
            if (x > sp->center)
                part = 1;
        } else {
            if (y > sp->center)
                part = 1;
        }

        if (part) {
            if (sp->flags & BTREE2_H_LEAF) {
                bc = sp->countH;
                leafIndex = sp->indexH;
                break;
            }
            sp = split + sp->indexH;
        } else {
            if (sp->flags & BTREE2_L_LEAF) {
                bc = sp->countL;
                leafIndex = sp->indexL;
                break;
            }
            sp = split + sp->indexL;
        }
    }

    {
    const uint16_t* li  = BTREE2_LEAVES(tree) + leafIndex;
    const uint16_t* end = li + bc;
    for (; li != end; ++li) {
        const BTree2Box* box = boxes + *li;
        if (x >= box->x && x < box->x2 &&
            y >= box->y && y < box->y2)
            return box;
    }
    }
    return NULL;
}
