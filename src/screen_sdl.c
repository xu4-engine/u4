/*
 * $Id$
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <SDL.h>

#include "u4.h"
#include "u4file.h"
#include "screen.h"
#include "map.h"
#include "context.h"
#include "savegame.h"
#include "ttype.h"

SDL_Surface *screen;
SDL_Surface *border, *tiles, *charset;
int scale, forceEga, forceVga;

int screenLoadBorders();
int screenLoadTiles();
int screenLoadCharSet();
int screenLoadPaletteEga(SDL_Surface *surface);
int screenLoadPaletteVga(SDL_Surface *surface, const char *filename);
int screenLoadTileSetEga(SDL_Surface **surface, int width, int height, int n, const char *filename);
int screenLoadTileSetVga(SDL_Surface **surface, int width, int height, int n, const char *filename, const char *palette_fname);
int screenLoadRleImageEga(SDL_Surface **surface, int width, int height, const char *filename);
int screenLoadRleImageVga(SDL_Surface **surface, int width, int height, const char *filename, const char *palette_fname);

#define RLE_RUNSTART 02

void screenInit(int screenScale) {
    scale = screenScale;
    forceEga = 0;
    forceVga = 0;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
	fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
	exit(1);
    }
    atexit(SDL_Quit);

    screen = SDL_SetVideoMode(320 * scale, 200 * scale, 16, SDL_SWSURFACE);
    if (!screen) {
	fprintf(stderr, "Unable to set video: %s\n", SDL_GetError());
	exit(1);
    }
    SDL_WM_SetCaption("Ultima IV", NULL);

    if (!screenLoadBorders() ||
        !screenLoadTiles() ||
        !screenLoadCharSet()) {
        fprintf(stderr, "Unable to load data files\n");
        exit(1);
    }

    SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL); 
}

/**
 * Load the background borders image from the "start" file.
 */
int screenLoadBorders() {
    int ret = 0;

    if (!forceEga)
        ret = screenLoadRleImageVga(&border, 320, 200, "start.ega", "u4vga.pal");

    if (!ret && !forceVga)
        ret = screenLoadRleImageEga(&border, 320, 200, "start.ega");

    return ret;
}

/**
 * Load the tiles graphics from the "shapes" file.
 */
int screenLoadTiles() {
    int ret = 0;

    if (!forceEga)
        ret = screenLoadTileSetVga(&tiles, TILE_WIDTH, TILE_HEIGHT, N_TILES, "shapes.vga", "u4vga.pal");

    if (!ret && !forceVga)
        ret = screenLoadTileSetEga(&tiles, TILE_WIDTH, TILE_HEIGHT, N_TILES, "shapes.ega");

    return ret;
}

/**
 * Load the character set graphics from the "charset" file.
 */
int screenLoadCharSet() {
    int ret = 0;

    if (!forceEga)
        ret = screenLoadTileSetVga(&charset, CHAR_WIDTH, CHAR_HEIGHT, N_CHARS, "charset.vga", "u4vga.pal");

    if (!ret && !forceVga)
        ret = screenLoadTileSetEga(&charset, CHAR_WIDTH, CHAR_HEIGHT, N_CHARS, "charset.ega");

    return ret;
}

/**
 * Set up an SDL surface with the basic EGA palette.
 */
int screenLoadPaletteEga(SDL_Surface *surface) {
    #define setpalentry(i, red, green, blue) \
        surface->format->palette->colors[i].r = red; \
        surface->format->palette->colors[i].g = green; \
        surface->format->palette->colors[i].b = blue;
    setpalentry(0, 0x00, 0x00, 0x00);
    setpalentry(1, 0x00, 0x00, 0x80);
    setpalentry(2, 0x00, 0x80, 0x00);
    setpalentry(3, 0x00, 0x80, 0x80);
    setpalentry(4, 0x80, 0x00, 0x00);
    setpalentry(5, 0x80, 0x00, 0x80);
    setpalentry(6, 0x80, 0x80, 0x00);
    setpalentry(7, 0xc3, 0xc3, 0xc3);
    setpalentry(8, 0xa0, 0xa0, 0xa0);
    setpalentry(9, 0x00, 0x00, 0xFF);
    setpalentry(10, 0x00, 0xFF, 0x00);
    setpalentry(11, 0x00, 0xFF, 0xFF);
    setpalentry(12, 0xFF, 0x00, 0x00);
    setpalentry(13, 0xFF, 0x00, 0xFF);
    setpalentry(14, 0xFF, 0xFF, 0x00);
    setpalentry(15, 0xFF, 0xFF, 0xFF);
    #undef setpalentry

    return 1;
}

/**
 * Set up an SDL surface with a 256 color palette loaded from the given file.
 */
int screenLoadPaletteVga(SDL_Surface *surface, const char *filename) {
    FILE *pal;
    int i;

    pal = u4fopen(filename);
    if (!pal)
        return 0;

    for (i = 0; i < 256; i++) {
        surface->format->palette->colors[i].r = getc(pal) * 255 / 63;
        surface->format->palette->colors[i].g = getc(pal) * 255 / 63;
        surface->format->palette->colors[i].b = getc(pal) * 255 / 63;
    }
    u4fclose(pal);

    return 1;
}

/**
 * Load a tile set from a ".ega" tile set file.
 */
int screenLoadTileSetEga(SDL_Surface **surface, int width, int height, int n, const char *filename) {
    FILE *in;
    int x, y, xs, ys;
    Uint8 *p;

    in = u4fopen(filename);
    if (!in)
        return 0;

    (*surface) = SDL_CreateRGBSurface(SDL_HWSURFACE, width * scale, height * n * scale, 8, 0, 0, 0, 0);
    if (!(*surface)) {
        u4fclose(in);
        return 0;
    }

    if (!screenLoadPaletteEga(*surface)) {
        SDL_FreeSurface(*surface);
        u4fclose(in);
        return 0;
    }

    p = (*surface)->pixels;
    for (y = 0; y < height * n; y++) {
        for (x = 0; x < width; x += 2) {
            int temp = getc(in);
            for (xs = 0; xs < scale; xs++) {
                *p = temp >> 4;
                p++;
            }
            for (xs = 0; xs < scale; xs++) {
                *p = temp & 0x0f;
                p++;
            }
        }
        p += ((*surface)->pitch) - (width * scale);
        for (ys = 1; ys < scale; ys++) {
            for (x = 0; x < width; x++) {
                for (xs = 0; xs < scale; xs++) {
                    *p = *(p - (*surface)->pitch);
                    p++;
                }
            }
            p += ((*surface)->pitch) - (width * scale);
        }
    }

    u4fclose(in);

    return 1;
}

/**
 * Load a tile set from a ".vga" tile set file (from the U4 upgrade).
 */
int screenLoadTileSetVga(SDL_Surface **surface, int width, int height, int n, const char *filename, const char *palette_fname) {
    FILE *in;
    int x, y, xs, ys;
    Uint8 *p;

    in = u4fopen(filename);
    if (!in)
        return 0;

    (*surface) = SDL_CreateRGBSurface(SDL_HWSURFACE, width * scale, height * n * scale, 8, 0, 0, 0, 0);
    if (!(*surface)) {
        u4fclose(in);
        return 0;
    }

    if (!screenLoadPaletteVga(*surface, palette_fname)) {
        SDL_FreeSurface(*surface);
        u4fclose(in);
        return 0;
    }

    p = (*surface)->pixels;
    for (y = 0; y < height * n; y++) {
        for (x = 0; x < width; x++) {
            int temp = getc(in);
            for (xs = 0; xs < scale; xs++) {
                *p++ = temp;
            }
        }
        p += ((*surface)->pitch) - (width * scale);
        for (ys = 1; ys < scale; ys++) {
            for (x = 0; x < width; x++) {
                for (xs = 0; xs < scale; xs++) {
                    *p = *(p - (*surface)->pitch);
                    p++;
                }
            }
            p += ((*surface)->pitch) - (width * scale);
        }
    }

    u4fclose(in);

    return 1;
}

/**
 * Load an image from an ".ega" RLE encoded image file.
 */
int screenLoadRleImageEga(SDL_Surface **surface, int width, int height, const char *filename) {
    FILE *in;
    int x, y, xs, ys;
    Uint8 *p;
    Uint8 *data;

    in = u4fopen(filename);
    if (!in)
        return 0;

    (*surface) = SDL_CreateRGBSurface(SDL_HWSURFACE, width * scale, height * scale, 8, 0, 0, 0, 0);
    if (!(*surface)) {
        u4fclose(in);
        return 0;
    }

    if (!screenLoadPaletteEga(*surface)) {
        SDL_FreeSurface(*surface);
        u4fclose(in);
        return 0;
    }

    data = malloc(sizeof(Uint8) * height * width);
    if (!data) {
        SDL_FreeSurface(*surface);
        u4fclose(in);
        return 0;
    }

    p = data;
    while (p < data + (height * width)) {
        int temp = getc(in);
        if (temp == RLE_RUNSTART) {
            int i, count;
            count = getc(in);
            temp = getc(in);
            for (i = 0; i < count; i++) {
                *p = temp >> 4;
                p++;
                *p = temp & 0x0f;
                p++;
            }
        } else {
            *p = temp >> 4;
            p++;
            *p = temp & 0x0f;
            p++;
        }
    }

    p = (*surface)->pixels;
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x += 2) {
            for (xs = 0; xs < scale; xs++) {
                *p = data[y * width + x];
                p++;
            }
            for (xs = 0; xs < scale; xs++) {
                *p = data[y * width + x + 1];
                p++;
            }
        }
        p += ((*surface)->pitch) - (width * scale);
        for (ys = 1; ys < scale; ys++) {
            for (x = 0; x < width; x++) {
                for (xs = 0; xs < scale; xs++) {
                    *p = *(p - (*surface)->pitch);
                    p++;
                }
            }
            p += ((*surface)->pitch) - (width * scale);
        }
    }
    free(data);
        
    u4fclose(in);

    return 1;
}

/**
 * Load an image from an ".ega" RLE encoded image file (from the U4 upgrade).
 */
int screenLoadRleImageVga(SDL_Surface **surface, int width, int height, const char *filename, const char *palette_fname) {
    FILE *in;
    int x, y, xs, ys;
    Uint8 *p, *data;

    in = u4fopen(filename);
    if (!in)
        return 0;

    (*surface) = SDL_CreateRGBSurface(SDL_HWSURFACE, width * scale, height * scale, 8, 0, 0, 0, 0);
    if (!(*surface)) {
        u4fclose(in);
        return 0;
    }

    if (!screenLoadPaletteVga(*surface, palette_fname)) {
        SDL_FreeSurface(*surface);
        u4fclose(in);
        return 0;
    }

    data = malloc(sizeof(Uint8) * height * width);
    if (!data) {
        SDL_FreeSurface(*surface);
        u4fclose(in);
        return 0;
    }

    p = data;
    while (p < data + (height * width)) {
        int temp = getc(in);
        if (temp == EOF) {
            u4fclose(in);
            free(data);
            SDL_FreeSurface(*surface);
            return 0;
        }
        if (temp == RLE_RUNSTART) {
            int i, count;
            count = getc(in);
            temp = getc(in);
            for (i = 0; i < count; i++) {
                *p = temp;
                p++;
            }
        } else {
            *p = temp;
            p++;
        }
    }

    p = (*surface)->pixels;
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            for (xs = 0; xs < scale; xs++) {
                *p = data[y * width + x];
                p++;
            }
        }
        p += ((*surface)->pitch) - (width * scale);
        for (ys = 1; ys < scale; ys++) {
            for (x = 0; x < width; x++) {
                for (xs = 0; xs < scale; xs++) {
                    *p = *(p - (*surface)->pitch);
                    p++;
                }
            }
            p += ((*surface)->pitch) - (width * scale);
        }
    }
    free(data);
        
    u4fclose(in);

    return 1;
}

/**
 * Draw the surrounding borders on the screen.
 */
void screenDrawBorders() {
    SDL_Rect r;

    r.x = 0;
    r.y = 0;
    r.w = 320 * scale;
    r.h = 200 * scale;
    SDL_BlitSurface(border, &r, screen, &r);
    screenForceRedraw();
}

/**
 * Draw a character from the charset onto the screen.
 */
void screenShowChar(int chr, int x, int y) {
    SDL_Rect src, dest;

    src.x = 0;
    src.y = chr * (charset->h / N_CHARS);
    src.w = charset->w;
    src.h = charset->h / N_CHARS;
    dest.x = x * charset->w;
    dest.y = y * (charset->h / N_CHARS);
    dest.w = charset->w;
    dest.h = charset->h / N_CHARS;
    SDL_BlitSurface(charset, &src, screen, &dest);
}

/**
 * Draw a tile graphic on the screen.
 */
void screenShowTile(int tile, int x, int y) {
    int offset;
    SDL_Rect src, dest;

    if (tileIsAnimated(tile))
        offset = screenCycle * scale;
    else
        offset = 0;

    src.x = 0;
    src.y = tile * (tiles->h / N_TILES);
    src.w = tiles->w;
    src.h = tiles->h / N_TILES - offset;
    dest.x = x * tiles->w + (BORDER_WIDTH * scale);
    dest.y = y * (tiles->h / N_TILES) + (BORDER_HEIGHT * scale) + offset;
    dest.w = tiles->w;
    dest.h = tiles->h / N_TILES;

    SDL_BlitSurface(tiles, &src, screen, &dest);

    if (offset == 0)
        return;

    src.x = 0;
    src.y = (tile + 1) * (tiles->h / N_TILES) - offset;
    src.w = tiles->w;
    src.h = offset;
    dest.x = x * tiles->w + (BORDER_WIDTH * scale);
    dest.y = y * (tiles->h / N_TILES) + (BORDER_HEIGHT * scale);
    dest.w = tiles->w;
    dest.h = tiles->h / N_TILES;

    SDL_BlitSurface(tiles, &src, screen, &dest);
}

/**
 * Scroll the text in the message area up one position.
 */
void screenScrollMessageArea() {
    SDL_Rect src, dest;
        
    src.x = TEXT_AREA_X * charset->w;
    src.y = (TEXT_AREA_Y + 1) * (charset->h / N_CHARS);
    src.w = TEXT_AREA_W * charset->w;
    src.h = (TEXT_AREA_H - 1) * charset->h / N_CHARS;

    dest.x = src.x;
    dest.y = src.y - (charset->h / N_CHARS);
    dest.w = src.w;
    dest.h = src.h;

    SDL_BlitSurface(screen, &src, screen, &dest);

    dest.y += dest.h;
    dest.h = (charset->h / N_CHARS);

    SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 0, 0, 0));
    SDL_UpdateRect(screen, 0, 0, 0, 0);
}

/**
 * Force a redraw.
 */
void screenForceRedraw() {
    SDL_UpdateRect(screen, 0, 0, 0, 0);
}
