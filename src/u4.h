/*
 * $Id$
 */

#ifndef U4_H
#define U4_H

#ifdef __cplusplus
extern "C" {
#endif

/* Microsoft VC port */
#if defined(_MSC_VER)
    #define VERSION "1.0beta1"
#endif

/* info for loading city data from *.ult and *.tlk */
#define CITY_HEIGHT 32
#define CITY_WIDTH 32
#define CITY_MAX_PERSONS 32

/* info for loading area data from *.con */
#define CON_HEIGHT 11
#define CON_WIDTH 11

/* info for loading dungeon map data from *.dng */
#define DNG_HEIGHT 8
#define DNG_WIDTH 8

/* info for loading image data from shapes.ega */
#define N_TILES 256
#define TILE_WIDTH (2 * CHAR_WIDTH)
#define TILE_HEIGHT (2 * CHAR_HEIGHT)

/* info for loading image data from charset.ega */
#define CHAR_WIDTH 8
#define CHAR_HEIGHT 8

/* some character defines */
#define CHARSET_BULLET '\010'
#define CHARSET_COPYRIGHT '\011'
#define CHARSET_REGISTERED '\012'
#define CHARSET_MALE '\013'
#define CHARSET_FEMALE '\014'
#define CHARSET_PROMPT '\020'

/* map viewport size (in tiles) */
#define VIEWPORT_W 11
#define VIEWPORT_H 11

/* screen border size (in pixels) */
#define BORDER_WIDTH 8
#define BORDER_HEIGHT 8

/* text area (in character units) */
#define TEXT_AREA_X 24
#define TEXT_AREA_Y 12
#define TEXT_AREA_W 16
#define TEXT_AREA_H 12

/* moons/moongates */
#define MOON_PHASES 24
#define MOON_SECONDS_PER_PHASE 4
#define MOON_CHAR 20

/* wind */
#define WIND_AREA_X 7
#define WIND_AREA_Y 23
#define WIND_AREA_W 10
#define WIND_AREA_H 1
#define WIND_SECONDS_PER_PHASE 1

#ifdef __cplusplus
}
#endif

#endif
