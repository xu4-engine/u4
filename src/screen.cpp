/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include <cstdio>
#include <cstdarg>
#include <cfloat>
#include <cstring>
#include "u4.h"

#include "screen.h"

#include "context.h"
#include "debug.h"
#include "dngview.h"
#include "error.h"
#include "location.h"
#include "names.h"
#include "object.h"
#include "player.h"
#include "savegame.h"
#include "settings.h"
#include "textcolor.h"
#include "tileset.h"
#include "tileview.h"

using std::vector;

void screenFindLineOfSight(vector<MapTile> viewportTiles[VIEWPORT_W][VIEWPORT_H]);
void screenFindLineOfSightDOS(vector<MapTile> viewportTiles[VIEWPORT_W][VIEWPORT_H]);
void screenFindLineOfSightEnhanced(vector<MapTile> viewportTiles[VIEWPORT_W][VIEWPORT_H]);

int screenNeedPrompt = 1;
int screenCurrentCycle = 0;
int screenCursorX = 0;
int screenCursorY = 0;
int screenCursorStatus = 0;
int screenCursorEnabled = 1;
int screenLos[VIEWPORT_W][VIEWPORT_H];
int screen3dDungeonView = 1;

void screenTextAt(int x, int y, const char *fmt, ...) {
    char buffer[1024];
    unsigned int i;

    va_list args;
    va_start(args, fmt);
    vsprintf(buffer, fmt, args);
    va_end(args);

    for (i = 0; i < strlen(buffer); i++)
        screenShowChar(buffer[i], x + i, y);
}

void screenPrompt() {
    if (screenNeedPrompt && screenCursorEnabled && c->col == 0) {
        screenMessage("%c", CHARSET_PROMPT);
        screenNeedPrompt = 0;
    }
}

void screenMessage(const char *fmt, ...) {
    char buffer[1024];
    unsigned int i;
    int wordlen;

    va_list args;
    va_start(args, fmt);
    vsprintf(buffer, fmt, args);
    va_end(args);

    screenHideCursor();

    /* scroll the message area, if necessary */
    if (c->line == 12) {
        screenScrollMessageArea();
        c->line--;
    }

    for (i = 0; i < strlen(buffer); i++) {
        // include whitespace and color-change codes
        wordlen = strcspn(buffer + i, " \b\t\n\024\025\026\027\030\031");

        /* backspace */
        if (buffer[i] == '\b') {
            c->col--;
            if (c->col < 0) {
                c->col += 16;
                c->line--;
            }
            continue;
        }

		/* color-change codes */
		switch (buffer[i])
		{
			case FG_GREY:
			case FG_BLUE:
            case FG_PURPLE:
			case FG_GREEN:
			case FG_RED:
			case FG_YELLOW:
			case FG_WHITE:
				screenTextColor(buffer[i]);
				continue;
		}

        /* check for word wrap */
        if ((c->col + wordlen > 16) || buffer[i] == '\n' || c->col == 16) {
            if (buffer[i] == '\n' || buffer[i] == ' ')
                i++;
            c->line++;
            c->col = 0;
            screenMessage(buffer + i);
            return;
        }

        /* code for move cursor right */
        if (buffer[i] == 0x12) {
            c->col++;
            continue;
        }
        /* don't show a space in column 1.  Helps with Hawkwind. */
        if (buffer[i] == ' ' && c->col == 0)
          continue; 

        screenShowChar(buffer[i], TEXT_AREA_X + c->col, TEXT_AREA_Y + c->line);
        c->col++;
    }

    screenSetCursorPos(TEXT_AREA_X + c->col, TEXT_AREA_Y + c->line);
    screenShowCursor();

    screenNeedPrompt = 1;
}

vector<MapTile> screenViewportTile(unsigned int width, unsigned int height, int x, int y, bool &focus) {
    MapCoords center = c->location->coords;    
    static MapTile grass = c->location->map->tileset->getByName("grass")->id;
    
    if (c->location->map->width <= width &&
        c->location->map->height <= height) {
        center.x = c->location->map->width / 2;
        center.y = c->location->map->height / 2;
    }

    MapCoords tc = center;

    tc.x += x - (width / 2);
    tc.y += y - (height / 2);

    /* Wrap the location if we can */    
    tc.wrap(c->location->map);

    /* off the edge of the map: pad with grass tiles */
    if (MAP_IS_OOB(c->location->map, tc)) {        
        focus = false;
        vector<MapTile> result;
        result.push_back(grass);
        return result;
    }

    return c->location->tilesAt(tc, focus);
}

/**
 * Redraw the screen.  If showmap is set, the normal map is drawn in
 * the map area.  If blackout is set, the map area is blacked out. If
 * neither is set, the map area is left untouched.
 */
void screenUpdate(TileView *view, bool showmap, bool blackout) {
    vector<MapTile> tiles;
    int x, y;

    static MapTile black = c->location->map->tileset->getByName("black")->id;
    static MapTile avatar = c->location->map->tileset->getByName("avatar")->id;

    ASSERT(c != NULL, "context has not yet been initialized");


    if (c->location->map->flags & FIRST_PERSON) {
        
        /* 1st-person perspective */
        if (screen3dDungeonView) {
            screenEraseMapArea();
            if (c->party->getTorchDuration() > 0 && !blackout) {
                for (y = 3; y >= 0; y--) {
                    DungeonGraphicType type;

                    tiles = dungeonViewGetTiles(y, -1);
                    screenDungeonDrawWall(-1, y, (Direction)c->saveGame->orientation, dungeonViewTilesToGraphic(tiles));

                    tiles = dungeonViewGetTiles(y, 1);
                    screenDungeonDrawWall(1, y, (Direction)c->saveGame->orientation, dungeonViewTilesToGraphic(tiles));

                    tiles = dungeonViewGetTiles(y, 0);
                    type = dungeonViewTilesToGraphic(tiles);
                    if ((type == DNGGRAPHIC_DNGTILE) || (type == DNGGRAPHIC_BASETILE))
                        screenDungeonDrawTile(c->location->map->tileset->get(tiles.front().id), y, Direction(c->saveGame->orientation));
                    else
                        screenDungeonDrawWall(0, y, (Direction)c->saveGame->orientation, type);
                }
            }
        }

        /* 3rd-person perspective */
        else {
            for (y = 0; y < VIEWPORT_H; y++) {
                for (x = 0; x < VIEWPORT_W; x++) {
                    if (x < 2 || y < 2 || x >= 10 || y >= 10)
                        view->drawTile(black, false, x, y);
                    else {
                        tiles = dungeonViewGetTiles((VIEWPORT_H / 2) - y, x - (VIEWPORT_W / 2));

                        /* Only show blackness if there is no light */
                        if (c->party->getTorchDuration() <= 0 || blackout)
                            view->drawTile(black, false, x, y);
                        else if (x == VIEWPORT_W/2 && y == VIEWPORT_H/2)
                            view->drawTile(avatar, false, x, y);
                        else
                            view->drawTile(tiles, false, x, y);
                    }
                }
            }
        }
        screenRedrawMapArea();
    }

    else if (showmap) {
        vector<MapTile> viewportTiles[VIEWPORT_W][VIEWPORT_H];
        bool viewportFocus[VIEWPORT_W][VIEWPORT_H];

        for (y = 0; y < VIEWPORT_H; y++) {
            for (x = 0; x < VIEWPORT_W; x++) {
                viewportTiles[x][y] = screenViewportTile(VIEWPORT_W, VIEWPORT_H, x, y, viewportFocus[x][y]);
            }
        }

        if (!blackout)
            screenFindLineOfSight(viewportTiles);

        for (y = 0; y < VIEWPORT_H; y++) {
            for (x = 0; x < VIEWPORT_W; x++) {
                if (!blackout && screenLos[x][y])
                    view->drawTile(viewportTiles[x][y], viewportFocus[x][y], x, y);
                else
                    view->drawTile(black, false, x, y);
            }
        }
        screenRedrawMapArea();
    }

    screenUpdateCursor();
    screenUpdateMoons();
    screenUpdateWind();
}

void screenCycle() {
    if (++screenCurrentCycle >= SCR_CYCLE_MAX)
        screenCurrentCycle = 0;
}

void screenUpdateCursor() {
    int phase = screenCurrentCycle * SCR_CYCLE_PER_SECOND / SCR_CYCLE_MAX;

    ASSERT(phase >= 0 && phase < 4, "derived an invalid cursor phase: %d", phase);

    if (screenCursorStatus) {
        screenShowChar(31 - phase, screenCursorX, screenCursorY);
        screenRedrawTextArea(screenCursorX, screenCursorY, 1, 1);
    }
}

void screenUpdateMoons() {
    int trammelChar, feluccaChar;

    /* show "L?" for the dungeon level */
    if (c->location->context == CTX_DUNGEON) {
        screenShowChar('L', 11, 0);
        screenShowChar('1'+c->location->coords.z, 12, 0);        
    }
    /* show the current moons (non-combat) */
    else if ((c->location->context & CTX_NON_COMBAT) == c->location->context) {
        trammelChar = (c->saveGame->trammelphase == 0) ?
            MOON_CHAR + 7 :
            MOON_CHAR + c->saveGame->trammelphase - 1;
        feluccaChar = (c->saveGame->feluccaphase == 0) ?
            MOON_CHAR + 7 :
            MOON_CHAR + c->saveGame->feluccaphase - 1;

        screenShowChar(trammelChar, 11, 0);
        screenShowChar(feluccaChar, 12, 0);        
    }

    screenRedrawTextArea(11, 0, 2, 1);
}

void screenUpdateWind() {   
    
    /* show the direction we're facing in the dungeon */
    if (c->location->context == CTX_DUNGEON) {
        screenEraseTextArea(WIND_AREA_X, WIND_AREA_Y, WIND_AREA_W, WIND_AREA_H);
        screenTextAt(WIND_AREA_X, WIND_AREA_Y, "Dir: %5s", getDirectionName((Direction)c->saveGame->orientation));        
    }
    /* show the wind direction */
    else if ((c->location->context & CTX_NON_COMBAT) == c->location->context) {
        screenEraseTextArea(WIND_AREA_X, WIND_AREA_Y, WIND_AREA_W, WIND_AREA_H);
        screenTextAt(WIND_AREA_X, WIND_AREA_Y, "Wind %5s", getDirectionName((Direction) c->windDirection));
    }
    screenRedrawTextArea(WIND_AREA_X, WIND_AREA_Y, WIND_AREA_W, WIND_AREA_H);
}

void screenShowCursor() {
    if (!screenCursorStatus && screenCursorEnabled) {
        screenCursorStatus = 1;
        screenUpdateCursor();
    }
}

void screenHideCursor() {
    if (screenCursorStatus) {
        screenEraseTextArea(screenCursorX, screenCursorY, 1, 1);    
        screenRedrawTextArea(screenCursorX, screenCursorY, 1, 1);
    }
    screenCursorStatus = 0;
}

void screenEnableCursor(void) {
    screenCursorEnabled = 1;
}

void screenDisableCursor(void) {
    screenHideCursor();
    screenCursorEnabled = 0;
}

void screenSetCursorPos(int x, int y) {
    screenCursorX = x;
    screenCursorY = y;
}

/**
 * Finds which tiles in the viewport are visible from the avatars
 * location in the middle. (original DOS algorithm)
 */
void screenFindLineOfSight(vector <MapTile> viewportTiles[VIEWPORT_W][VIEWPORT_H]) {
    int x, y;

    if (!c)
        return;

    /*
     * if the map has the no line of sight flag, all is visible
     */
    if (c->location->map->flags & NO_LINE_OF_SIGHT) {
        for (y = 0; y < VIEWPORT_H; y++) {
            for (x = 0; x < VIEWPORT_W; x++) {
                screenLos[x][y] = 1;
            }
        }
        return;
    }

    /*
     * otherwise calculate it from the map data
     */
    for (y = 0; y < VIEWPORT_H; y++) {
        for (x = 0; x < VIEWPORT_W; x++) {
            screenLos[x][y] = 0;
        }
    }

    if (settings.lineOfSight == "DOS")
        screenFindLineOfSightDOS(viewportTiles);
    else if (settings.lineOfSight == "Enhanced")
        screenFindLineOfSightEnhanced(viewportTiles);
    else
        errorFatal("unknown line of sight style %s!\n", settings.lineOfSight.c_str());
}        


/**
 * Finds which tiles in the viewport are visible from the avatars
 * location in the middle. (original DOS algorithm)
 */
void screenFindLineOfSightDOS(vector <MapTile> viewportTiles[VIEWPORT_W][VIEWPORT_H]) {
    int x, y;

    screenLos[VIEWPORT_W / 2][VIEWPORT_H / 2] = 1;

    for (x = VIEWPORT_W / 2 - 1; x >= 0; x--)
        if (screenLos[x + 1][VIEWPORT_H / 2] &&
            !viewportTiles[x + 1][VIEWPORT_H / 2].front().getTileType()->isOpaque())
            screenLos[x][VIEWPORT_H / 2] = 1;

    for (x = VIEWPORT_W / 2 + 1; x < VIEWPORT_W; x++)
        if (screenLos[x - 1][VIEWPORT_H / 2] &&
            !viewportTiles[x - 1][VIEWPORT_H / 2].front().getTileType()->isOpaque())
            screenLos[x][VIEWPORT_H / 2] = 1;

    for (y = VIEWPORT_H / 2 - 1; y >= 0; y--)
        if (screenLos[VIEWPORT_W / 2][y + 1] &&
            !viewportTiles[VIEWPORT_W / 2][y + 1].front().getTileType()->isOpaque())
            screenLos[VIEWPORT_W / 2][y] = 1;

    for (y = VIEWPORT_H / 2 + 1; y < VIEWPORT_H; y++)
        if (screenLos[VIEWPORT_W / 2][y - 1] &&
            !viewportTiles[VIEWPORT_W / 2][y - 1].front().getTileType()->isOpaque())
            screenLos[VIEWPORT_W / 2][y] = 1;

    for (y = VIEWPORT_H / 2 - 1; y >= 0; y--) {
        
        for (x = VIEWPORT_W / 2 - 1; x >= 0; x--) {
            if (screenLos[x][y + 1] &&
                !viewportTiles[x][y + 1].front().getTileType()->isOpaque())
                screenLos[x][y] = 1;
            else if (screenLos[x + 1][y] &&
                     !viewportTiles[x + 1][y].front().getTileType()->isOpaque())
                screenLos[x][y] = 1;
            else if (screenLos[x + 1][y + 1] &&
                     !viewportTiles[x + 1][y + 1].front().getTileType()->isOpaque())
                screenLos[x][y] = 1;
        }
                
        for (x = VIEWPORT_W / 2 + 1; x < VIEWPORT_W; x++) {
            if (screenLos[x][y + 1] &&
                !viewportTiles[x][y + 1].front().getTileType()->isOpaque())
                screenLos[x][y] = 1;
            else if (screenLos[x - 1][y] &&
                     !viewportTiles[x - 1][y].front().getTileType()->isOpaque())
                screenLos[x][y] = 1;
            else if (screenLos[x - 1][y + 1] &&
                     !viewportTiles[x - 1][y + 1].front().getTileType()->isOpaque())
                screenLos[x][y] = 1;
        }
    }

    for (y = VIEWPORT_H / 2 + 1; y < VIEWPORT_H; y++) {
        
        for (x = VIEWPORT_W / 2 - 1; x >= 0; x--) {
            if (screenLos[x][y - 1] &&
                !viewportTiles[x][y - 1].front().getTileType()->isOpaque())
                screenLos[x][y] = 1;
            else if (screenLos[x + 1][y] &&
                     !viewportTiles[x + 1][y].front().getTileType()->isOpaque())
                screenLos[x][y] = 1;
            else if (screenLos[x + 1][y - 1] &&
                     !viewportTiles[x + 1][y - 1].front().getTileType()->isOpaque())
                screenLos[x][y] = 1;
        }
                
        for (x = VIEWPORT_W / 2 + 1; x < VIEWPORT_W; x++) {
            if (screenLos[x][y - 1] &&
                !viewportTiles[x][y - 1].front().getTileType()->isOpaque())
                screenLos[x][y] = 1;
            else if (screenLos[x - 1][y] &&
                     !viewportTiles[x - 1][y].front().getTileType()->isOpaque())
                screenLos[x][y] = 1;
            else if (screenLos[x - 1][y - 1] &&
                     !viewportTiles[x - 1][y - 1].front().getTileType()->isOpaque())
                screenLos[x][y] = 1;
        }
    }
}

/**
 * Finds which tiles in the viewport are visible from the avatars
 * location in the middle.
 *
 * A new, more accurate LOS function
 *
 * Based somewhat off Andy McFadden's 1994 article,
 *   "Improvements to a Fast Algorithm for Calculating Shading
 *   and Visibility in a Two-Dimensional Field"
 *   -----
 *   http://www.fadden.com/techmisc/fast-los.html
 *
 * This function uses a lookup table to get the correct shadowmap,
 * therefore, the table will need to be updated if the viewport
 * dimensions increase. Also, the function assumes that the
 * viewport width and height are odd values and that the player
 * is always at the center of the screen.
 */
void screenFindLineOfSightEnhanced(vector <MapTile> viewportTiles[VIEWPORT_W][VIEWPORT_H]) {
    int x, y;

    /*
     * the shadow rasters for each viewport octant
     *
     * shadowRaster[0][0]    // number of raster segments in this shadow
     * shadowRaster[0][1]    // #1 shadow bitmask value (low three bits) + "newline" flag (high bit)
     * shadowRaster[0][2]    // #1 length
     * shadowRaster[0][3]    // #2 shadow bitmask value
     * shadowRaster[0][4]    // #2 length
     * shadowRaster[0][5]    // #3 shadow bitmask value
     * shadowRaster[0][6]    // #3 length
     * ...etc...
     */
    const int shadowRaster[14][13] = {
        { 6, __VCH, 4, _N_CH, 1, __VCH, 3, _N___, 1, ___CH, 1, __VCH, 1 },    // raster_1_0
        { 6, __VC_, 1, _NVCH, 2, __VC_, 1, _NVCH, 3, _NVCH, 2, _NVCH, 1 },    // raster_1_1
        //
        { 4, __VCH, 3, _N__H, 1, ___CH, 1, __VCH, 1,     0, 0,     0, 0 },    // raster_2_0
        { 6, __VC_, 2, _N_CH, 1, __VCH, 2, _N_CH, 1, __VCH, 1, _N__H, 1 },    // raster_2_1
        { 6, __V__, 1, _NVCH, 1, __VC_, 1, _NVCH, 1, __VC_, 1, _NVCH, 1 },    // raster_2_2
        //
        { 2, __VCH, 2, _N__H, 2,     0, 0,     0, 0,     0, 0,     0, 0 },    // raster_3_0
        { 3, __VC_, 2, _N_CH, 1, __VCH, 1,     0, 0,     0, 0,     0, 0 },    // raster_3_1
        { 3, __VC_, 1, _NVCH, 2, _N_CH, 1,     0, 0,     0, 0,     0, 0 },    // raster_3_2
        { 3, _NVCH, 1, __V__, 1, _NVCH, 1,     0, 0,     0, 0,     0, 0 },    // raster_3_3
        //
        { 2, __VCH, 1, _N__H, 1,     0, 0,     0, 0,     0, 0,     0, 0 },    // raster_4_0
        { 2, __VC_, 1, _N__H, 1,     0, 0,     0, 0,     0, 0,     0, 0 },    // raster_4_1
        { 2, __VC_, 1, _N_CH, 1,     0, 0,     0, 0,     0, 0,     0, 0 },    // raster_4_2
        { 2, __V__, 1, _NVCH, 1,     0, 0,     0, 0,     0, 0,     0, 0 },    // raster_4_3
        { 2, __V__, 1, _NVCH, 1,     0, 0,     0, 0,     0, 0,     0, 0 }     // raster_4_4
    };

    /*
     * As each viewport tile is processed, it will store the bitmask for the shadow it casts.
     * Later, after processing all octants, the entire viewport will be marked visible except
     * for those tiles that have the __VCH bitmask.
     */
    const int _OCTANTS = 8;
    const int _NUM_RASTERS_COLS = 4;
    
    int octant;
    int xOrigin, yOrigin, xSign, ySign, reflect, xTile, yTile, xTileOffset, yTileOffset;

    for (octant = 0; octant < _OCTANTS; octant++) {
        switch (octant) {
            case 0:  xSign=  1;  ySign=  1;  reflect=false;  break;        // lower-right
            case 1:  xSign=  1;  ySign=  1;  reflect=true;   break;
            case 2:  xSign=  1;  ySign= -1;  reflect=true;   break;        // lower-left
            case 3:  xSign= -1;  ySign=  1;  reflect=false;  break;
            case 4:  xSign= -1;  ySign= -1;  reflect=false;  break;        // upper-left
            case 5:  xSign= -1;  ySign= -1;  reflect=true;   break;
            case 6:  xSign= -1;  ySign=  1;  reflect=true;   break;        // upper-right
            case 7:  xSign=  1;  ySign= -1;  reflect=false;  break;
        }

        // determine the origin point for the current LOS octant
        xOrigin = VIEWPORT_W / 2;
        yOrigin = VIEWPORT_H / 2;

        // make sure the segment doesn't reach out of bounds
        int maxWidth      = xOrigin;
        int maxHeight     = yOrigin;
        int currentRaster = 0;

        // just in case the viewport isn't square, swap the width and height
        if (reflect) {
            // swap height and width for later use
            maxWidth ^= maxHeight;
            maxHeight ^= maxWidth;
            maxWidth ^= maxHeight;
        }

        // check the visibility of each tile
        for (int currentCol = 1; currentCol <= _NUM_RASTERS_COLS; currentCol++) {
            for (int currentRow = 0; currentRow <= currentCol; currentRow++) {
                // swap X and Y to reflect the octant rasters
                if (reflect) {
                    xTile = xOrigin+(currentRow*ySign);
                    yTile = yOrigin+(currentCol*xSign);
                }
                else {
                    xTile = xOrigin+(currentCol*xSign);
                    yTile = yOrigin+(currentRow*ySign);
                }

                if (viewportTiles[xTile][yTile].front().getTileType()->isOpaque()) {
                    // a wall was detected, so go through the raster for this wall
                    // segment and mark everything behind it with the appropriate
                    // shadow bitmask.
                    //
                    // first, get the correct raster
                    //
                    if ((currentCol==1) && (currentRow==0)) { currentRaster=0; }
                    else if ((currentCol==1) && (currentRow==1)) { currentRaster=1; }
                    else if ((currentCol==2) && (currentRow==0)) { currentRaster=2; }
                    else if ((currentCol==2) && (currentRow==1)) { currentRaster=3; }
                    else if ((currentCol==2) && (currentRow==2)) { currentRaster=4; }
                    else if ((currentCol==3) && (currentRow==0)) { currentRaster=5; }
                    else if ((currentCol==3) && (currentRow==1)) { currentRaster=6; }
                    else if ((currentCol==3) && (currentRow==2)) { currentRaster=7; }
                    else if ((currentCol==3) && (currentRow==3)) { currentRaster=8; }
                    else if ((currentCol==4) && (currentRow==0)) { currentRaster=9; }
                    else if ((currentCol==4) && (currentRow==1)) { currentRaster=10; }
                    else if ((currentCol==4) && (currentRow==2)) { currentRaster=11; }
                    else if ((currentCol==4) && (currentRow==3)) { currentRaster=12; }
                    else { currentRaster=13; }  // currentCol and currentRow must equal 4

                    xTileOffset = 0;
                    yTileOffset = 0;

                    //========================================
                    for (int currentSegment = 0; currentSegment < shadowRaster[currentRaster][0]; currentSegment++) {
                        // each shadow segment is 2 bytes
                        int shadowType   = shadowRaster[currentRaster][currentSegment*2+1];
                        int shadowLength = shadowRaster[currentRaster][currentSegment*2+2];

                        // update the raster length to make sure it fits in the viewport
                        shadowLength = (shadowLength+1+yTileOffset > maxWidth ? maxWidth : shadowLength);

                        // check to see if we should move up a row
                        if (shadowType & 0x80) {
                            // remove the flag from the shadowType
                            shadowType ^= _N___;
//                            if (currentRow + yTileOffset >= maxHeight) {
                            if (currentRow + yTileOffset > maxHeight) {
                                break;
                            }
                            xTileOffset = yTileOffset;
                            yTileOffset++;
                        }

                        /* it is seemingly unnecessary to swap the edges for
                         * shadow tiles, because we only care about shadow
                         * tiles that have all three parts (V, C, and H)
                         * flagged.  if a tile has fewer than three, it is
                         * ignored during the draw phase, so vertical and
                         * horizontal shadow edge accuracy isn't important
                         */
                        // if reflecting the octant, swap the edges
//                        if (reflect) {
//                            int shadowTemp = 0;
//                            // swap the vertical and horizontal shadow edges
//                            if (shadowType & __V__) { shadowTemp |= ____H; }
//                            if (shadowType & ___C_) { shadowTemp |= ___C_; }
//                            if (shadowType & ____H) { shadowTemp |= __V__; }
//                            shadowType = shadowTemp;
//                        }

                        for (int currentShadow = 1; currentShadow <= shadowLength; currentShadow++) {
                            // apply the shadow to the shadowMap
                            if (reflect) {
                                screenLos[xTile + ((yTileOffset) * ySign)][yTile + ((currentShadow+xTileOffset) * xSign)] |= shadowType;
                            }
                            else {
                                screenLos[xTile + ((currentShadow+xTileOffset) * xSign)][yTile + ((yTileOffset) * ySign)] |= shadowType;
                            }
                        }
                        xTileOffset += shadowLength;
                    }  // for (int currentSegment = 0; currentSegment < shadowRaster[currentRaster][0]; currentSegment++)
                    //========================================

                }  // if (viewportTiles[xTile][yTile].front().getTileType()->isOpaque())
            }  // for (int currentRow = 0; currentRow <= currentCol; currentRow++)
        }  // for (int currentCol = 1; currentCol <= _NUM_RASTERS_COLS; currentCol++)
    }  // for (octant = 0; octant < _OCTANTS; octant++)

    // go through all tiles on the viewable area and set the appropriate visibility
    for (y = 0; y < VIEWPORT_H; y++) {
        for (x = 0; x < VIEWPORT_W; x++) {
            // if the shadow flags equal __VCH, hide it, otherwise it's fully visible
            //
            if ((screenLos[x][y] & __VCH) == __VCH) {
                screenLos[x][y] = 0;
            }
            else {
                screenLos[x][y] = 1;
            }
        }
    }
}

/**
 * Generates terms a and b for equation "ax + b = y" that defines the
 * line containing the two given points.  Vertical lines are special
 * cased to return DBL_MAX for a and the x coordinate as b since they
 * cannot be represented with the above formula.
 */
static void screenGetLineTerms(int x1, int y1, int x2, int y2, double *a, double *b) {
    if (x2 - x1 == 0) {
        *a = DBL_MAX;
        *b = x1;
    }
    else {
        *a = ((double)(y2 - y1)) / ((double)(x2 - x1));
        *b = y1 - ((*a) * x1);
    }
}

/**
 * Determine if two points are on the same side of a line (or both on
 * the line).  The line is defined by the terms a and b of the
 * equation "ax + b = y".
 */
static int screenPointsOnSameSideOfLine(int x1, int y1, int x2, int y2, double a, double b) {
    double p1, p2;

    if (a == DBL_MAX) {
        p1 = x1 - b;
        p2 = x2 - b;
    }
    else {
        p1 = x1 * a + b - y1;
        p2 = x2 * a + b - y2;
    }

    if ((p1 > 0.0 && p2 > 0.0) ||
        (p1 < 0.0 && p2 < 0.0) ||
        (p1 == 0.0 && p2 == 0.0))
        return 1;

    return 0;
}

static int screenPointInTriangle(int x, int y, int tx1, int ty1, int tx2, int ty2, int tx3, int ty3) {
    double a[3], b[3];

    screenGetLineTerms(tx1, ty1, tx2, ty2, &(a[0]), &(b[0]));
    screenGetLineTerms(tx2, ty2, tx3, ty3, &(a[1]), &(b[1]));
    screenGetLineTerms(tx3, ty3, tx1, ty1, &(a[2]), &(b[2]));

    if (!screenPointsOnSameSideOfLine(x, y, tx3, ty3, a[0], b[0]))
        return 0;
    if (!screenPointsOnSameSideOfLine(x, y, tx1, ty1, a[1], b[1]))
        return 0;
    if (!screenPointsOnSameSideOfLine(x, y, tx2, ty2, a[2], b[2]))
        return 0;

    return 1;
}

/**
 * Determine if the given point is within a mouse area.
 */
int screenPointInMouseArea(int x, int y, MouseArea *area) {
    ASSERT(area->npoints == 2 || area->npoints == 3, "unsupported number of points in area: %d", area->npoints);

    /* two points define a rectangle */
    if (area->npoints == 2) {
        if (x >= (int)(area->point[0].x * settings.scale) &&
            y >= (int)(area->point[0].y * settings.scale) &&
            x < (int)(area->point[1].x * settings.scale) &&
            y < (int)(area->point[1].y * settings.scale)) {
            return 1;
        }
    }

    /* three points define a triangle */
    else if (area->npoints == 3) {
        return screenPointInTriangle(x, y, 
                                     area->point[0].x * settings.scale, area->point[0].y * settings.scale,
                                     area->point[1].x * settings.scale, area->point[1].y * settings.scale,
                                     area->point[2].x * settings.scale, area->point[2].y * settings.scale);
    }

    return 0;
}
