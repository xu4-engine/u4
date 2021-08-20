/*
 * Copyright 2016 faddenSoft. All Rights Reserved.
 * C version copyright 2021 Karl Robillard
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
Field-of-vision calculation for a simple tiled map.

Based on
http://www.roguebasin.com/index.php?title=FOV_using_recursive_shadowcasting,
but without the strange ideas about coordinate systems.

Computation is performed in the first octant, i.e. the triangle with vertices
{ (0,0), (N,0), (N,N) } on a Cartesian plane.  The cell grid identifies the
center of 1x1 cells, so a cell at (X,Y) extends 0.5 in each direction.  The
viewer is at (0,0), and we assume that the viewer's cell is visible to itself.

Cells are assumed to be in shadow unless light can reach them, so the caller
should reset all cells to "not visible" before calling gsc_computeVisibility.
*/

/*
This file is a C template which requires five macros to be defined for access
to the map data.  These GSC_ macros are equivalent to the following function
signatures:

    int  GSC_XDIM(GSC_TYPE*)
    int  GSC_YDIM(GSC_TYPE*)
    int  GSC_IS_WALL(GSC_TYPE*, int x, int y)
    void GSC_SET_LIGHT(GSC_TYPE*, int x, int y, float distanceSquared)

GSC_IS_WALL() returns non-zero if the cell is a wall.
Coordinates are range-checked, so the macros will only be used with valid
values.

Here is an example of using the template:

    #define GSC_TYPE                MyGrid
    #define GSC_XDIM(g)             g->width
    #define GSC_YDIM(g)             g->height
    #define GSC_IS_WALL(g,x,y)      g->solid[g->width * y + x];
    #define GSC_SET_LIGHT(g,x,y,ds) g->visible[g->width * y + x] = ds;
    #include "gridShadowCast.c"

    ...

    typedef struct {
        int width, height;
        uint8_t solid[32 * 32];
        float visible[32 * 32];
    } MyGrid;

    MyGrid grid;
    int viewPos[2];
    #define NOT_VISIBLE -1.0f;

    // Assuming grid & viewPos are initialized for the scene we only need to
    // clear the visible array used by GSC_SET_LIGHT.
    for (i = 0; i < 32*32; ++i)
        grid.visible[i] = NOT_VISIBLE;

    gsc_computeVisibility(&grid, viewPos, 11.0f);
*/

#include <assert.h>
#include <math.h>

// Struct for holding coordinate transform constants.
typedef struct {
    int xx, xy, yx, yy;
} OctantTransform;

static const OctantTransform s_octantTransform[8] = {
    { 1,  0,  0,  1 },   // 0 E-NE
    { 0,  1,  1,  0 },   // 1 NE-N
    { 0, -1,  1,  0 },   // 2 N-NW
    {-1,  0,  0,  1 },   // 3 NW-W
    {-1,  0,  0, -1 },   // 4 W-SW
    { 0, -1, -1,  0 },   // 5 SW-S
    { 0,  1, -1,  0 },   // 6 S-SE
    { 1,  0,  0, -1 }    // 7 SE-E
};

/*
    Recursively casts light into cells.  Operates on a single octant.

    \param grid             The cell grid definition.
    \param gridPos          The player's X,Y position within the grid.
    \param viewRadius       The view radius; can be a fractional value.
    \param startColumn      Current column; pass 1 as initial value.
    \param leftViewSlope    Slope of the left (upper) view edge; pass 1.0 as
                            the initial value.
    \param rightViewSlope   Slope of the right (lower) view edge; pass 0.0 as
                            the initial value.
    \param txfrm            Coordinate multipliers for the octant transform.

    Maximum recursion depth is ceiling(viewRadius).
*/
static void gsc_castLight(GSC_TYPE* grid, const int* gridPos, float viewRadius,
        int startColumn, float leftViewSlope, float rightViewSlope,
        const OctantTransform* txfrm)
{
    // Used for distance test.
    float viewRadiusSq = viewRadius * viewRadius;

    int viewCeiling = (int) ceilf(viewRadius);

    // Set true if the previous cell we encountered was blocked.
    int prevWasBlocked = 0;

    // As an optimization, when scanning past a block we keep track of the
    // rightmost corner (bottom-right) of the last one seen.  If the next cell
    // is empty, we can use this instead of having to compute the top-right
    // corner of the empty cell.
    float savedRightSlope = -1;

    int xDim = GSC_XDIM(grid);
    int yDim = GSC_YDIM(grid);
    int currentCol, xc, yc;

    assert(leftViewSlope >= rightViewSlope);

    // Outer loop: walk across each column, stopping when we reach the
    // visibility limit.
    for (currentCol = startColumn; currentCol <= viewCeiling; currentCol++) {
        xc = currentCol;

        // Inner loop: walk down the current column.  We start at the top,
        // where X==Y.
        //
        // TODO: we waste time walking across the entire column when the view
        //   area is narrow.  Experiment with computing the possible range of
        //   cells from the slopes, and iterate over that instead.

        for (yc = currentCol; yc >= 0; yc--) {
            // Translate local coordinates to grid coordinates.  For the
            // various octants we need to invert one or both values, or swap
            // X for Y.
            int gridX = gridPos[0] + xc * txfrm->xx + yc * txfrm->xy;
            int gridY = gridPos[1] + xc * txfrm->yx + yc * txfrm->yy;

            // Range-check the values.  This lets us avoid the slope division
            // for blocks that are outside the grid.
            //
            // Note that, while we will stop at a solid column of blocks, we
            // do always start at the top of the column, which may be outside
            // the grid if we're (say) checking the first octant while
            // positioned at the north edge of the map.

            if (gridX < 0 || gridX >= xDim || gridY < 0 || gridY >= yDim)
                continue;

            // Compute slopes to corners of current block.  We use the top-left
            // and bottom-right corners.  If we were iterating through a
            // quadrant, rather than an octant, we'd need to flip the corners
            // we used when we hit the midpoint.
            //
            // Note these values will be outside the view angles for the
            // blocks at the ends -- left value > 1, right value < 0.

            float leftBlockSlope  = (yc + 0.5f) / (xc - 0.5f);
            float rightBlockSlope = (yc - 0.5f) / (xc + 0.5f);

            // Check to see if the block is outside our view area.  Note that
            // we allow a "corner hit" to make the block visible.  Changing
            // the tests to >= / <= will reduce the number of cells visible
            // through a corner (from a 3-wide swath to a single diagonal
            // line), and affect how far you can see past a block as you
            // approach it.  This is mostly a matter of personal preference.

            if (rightBlockSlope > leftViewSlope) {
                // Block is above the left edge of our view area; skip.
                continue;
            } else if (leftBlockSlope < rightViewSlope) {
                // Block is below the right edge of our view area; we're done.
                break;
            }

            // This cell is visible, given infinite vision range.  If it's
            // also within our finite vision range, light it up.
            //
            // To avoid having a single lit cell poking out N/S/E/W, use a
            // fractional viewRadius, e.g. 8.5.
            //
            // TODO: we're testing the middle of the cell for visibility.  If
            //  we tested the bottom-left corner, we could say definitively
            //  that no part of the cell is visible, and reduce the view area as
            //  if it were a wall.  This could reduce iteration at the corners.

            float distanceSquared = xc * xc + yc * yc;
            if (distanceSquared <= viewRadiusSq) {
                GSC_SET_LIGHT(grid, gridX, gridY, distanceSquared);
            }

            int curBlocked = GSC_IS_WALL(grid, gridX, gridY);

            if (prevWasBlocked) {
                if (curBlocked) {
                    // Still traversing a column of walls.
                    savedRightSlope = rightBlockSlope;
                } else {
                    // Found the end of the column of walls.  Set the left
                    // edge of our view area to the right corner of the last
                    // wall we saw.
                    prevWasBlocked = 0;
                    leftViewSlope = savedRightSlope;
                }
            } else {
                if (curBlocked) {
                    // Found a wall.  Split the view area, recursively
                    // pursuing the part to the left.  The leftmost corner of
                    // the wall we just found becomes the right boundary of
                    // the view area.
                    //
                    // If this is the first block in the column, the slope of
                    // the top-left corner will be greater than the initial
                    // view slope (1.0).  Handle that here.

                    if (leftBlockSlope <= leftViewSlope) {
                        gsc_castLight(grid, gridPos, viewRadius, currentCol + 1,
                                      leftViewSlope, leftBlockSlope, txfrm);
                    }

                    // Once that's done, we keep searching to the right (down
                    // the column), looking for another opening.
                    prevWasBlocked = 1;
                    savedRightSlope = rightBlockSlope;
                }
            }
        }

        // Open areas are handled recursively, with the function continuing to
        // search to the right (down the column).  If we reach the bottom of
        // the column without finding an open cell, then the area defined by
        // our view area is completely obstructed, and we can stop working.
        if (prevWasBlocked)
            break;
    }
}

/*
    Lights up cells visible from the current position.  Clear all lighting
    before calling.

    \param grid         The cell grid definition.
    \param gridPos      The player's X,Y position within the grid.
    \param viewRadius   Maximum view distance; can be a fractional value.
*/
static void gsc_computeVisibility(GSC_TYPE* grid, const int* gridPos,
                                  float viewRadius)
{
    int txidx;

    assert(gridPos[0] >= 0 && gridPos[0] < GSC_XDIM(grid));
    assert(gridPos[1] >= 0 && gridPos[1] < GSC_YDIM(grid));

    // Viewer's cell is always visible.
    GSC_SET_LIGHT(grid, gridPos[0], gridPos[1], 0.0f);

    // Cast light into cells for each of 8 octants.

    // The left/right inverse slope values are initially 1 and 0, indicating a
    // diagonal and a horizontal line.  These aren't strictly correct, as the
    // view area is supposed to be based on corners, not center points.  We
    // only really care about one side of the wall at the edges of the octant
    // though.

    // NOTE: depending on the compiler, it's possible that passing the octant
    // transform values as four integers rather than an object reference would
    // speed things up. It's much tidier this way though.

    for (txidx = 0; txidx < 8; txidx++) {
        gsc_castLight(grid, gridPos, viewRadius, 1, 1.0f, 0.0f,
                      &s_octantTransform[txidx]);
    }
}

#undef GSC_TYPE
#undef GSC_XDIM
#undef GSC_YDIM
#undef GSC_IS_WALL
#undef GSC_SET_LIGHT
