/*
 * $Id$
 */

#ifndef U4_H
#define U4_H

#define SCALE 2

// info for building map.c from u4/world.map
#define MAP_WIDTH 256
#define MAP_HEIGHT 256
#define MAP_CHUNK_WIDTH 32
#define MAP_CHUNK_HEIGHT 32
#define MAP_HORIZ_CHUNKS (MAP_WIDTH/MAP_CHUNK_WIDTH)
#define MAP_VERT_CHUNKS (MAP_HEIGHT/MAP_CHUNK_HEIGHT)

// info for building city data from u4/*.ult and u4/*.tlk
#define CITY_HEIGHT 32
#define CITY_WIDTH 32
#define CITY_MAX_PERSONS 32

// info for building tiles.xpm from u4/shapes.ega
#define N_TILES 256
#define TILE_WIDTH (2 * CHAR_WIDTH)
#define TILE_HEIGHT (2 * CHAR_HEIGHT)

// info for building charset.xpm from u4/charset.ega
#define N_CHARS 128
#define CHAR_WIDTH 8
#define CHAR_HEIGHT 8

// map viewport size (in tiles)
#define VIEWPORT_W 11
#define VIEWPORT_H 11

// screen border size (in pixels)
#define BORDER_WIDTH 8
#define BORDER_HEIGHT 8

// text area (in character units)
#define TEXT_AREA_X 24
#define TEXT_AREA_Y 12
#define TEXT_AREA_W 16
#define TEXT_AREA_H 12

#endif
