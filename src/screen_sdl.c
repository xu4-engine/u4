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

typedef enum {
    ANIM_HONCOM,
    ANIM_VALJUS,
    ANIM_SACHONOR,
    ANIM_SPIRHUM,
    ANIM_ANIMATE,
    ANIM_MAX
} AnimType;

SDL_Surface *screen;
SDL_Surface *bkgds[BKGD_MAX];
SDL_Surface *introAnimations[ANIM_MAX];
SDL_Surface *tiles, *charset;
int scale, forceEga, forceVga;

long decompress_u4_file(FILE *in, long filesize, void **out);

void screenFillRect(SDL_Surface *surface, int x, int y, int w, int h, int color);
void screenCopyRect(SDL_Surface *surface, int srcX, int srcY, int destX, int destY, int w, int h);
void screenWriteScaledPixel(SDL_Surface *surface, int x, int y, Uint8 color);
int screenLoadBackgrounds();
void screenFreeIntroBackground();
int screenLoadTiles();
int screenLoadCharSet();
int screenLoadPaletteEga(SDL_Surface *surface);
int screenLoadPaletteVga(SDL_Surface *surface, const char *filename);
int screenLoadTileSetEga(SDL_Surface **surface, int width, int height, int n, const char *filename);
int screenLoadTileSetVga(SDL_Surface **surface, int width, int height, int n, const char *filename, const char *palette_fname);
int screenLoadRleImageEga(SDL_Surface **surface, int width, int height, const char *filename);
int screenLoadRleImageVga(SDL_Surface **surface, int width, int height, const char *filename, const char *palette_fname);
int screenLoadLzwImageEga(SDL_Surface **surface, int width, int height, const char *filename);

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
#ifdef ICON_FILE
    SDL_WM_SetIcon(SDL_LoadBMP(ICON_FILE), NULL);
#endif

    if (!screenLoadBackgrounds() || 
        !screenLoadTiles() ||
        !screenLoadCharSet()) {
        fprintf(stderr, "Unable to load data files\n");
        exit(1);
    }

    SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL); 
}

/**
 *  Fills a rectangular screen area with the specified color.  The x,
 *  y, width and height parameters are unscaled, i.e. for 320x200.
 */
void screenFillRect(SDL_Surface *surface, int x, int y, int w, int h, int color) {
    SDL_Rect dest;

    dest.x = x * scale;
    dest.y = y * scale;
    dest.w = w * scale;
    dest.h = h * scale;

    SDL_FillRect(surface, &dest, color);
}

/**
 *  Copies a rectangular screen area.  I don't know what happens if
 *  the source and destination overlap ;-) The parameters are
 *  unscaled, i.e. for 320x200.
 */
void screenCopyRect(SDL_Surface *surface, int srcX, int srcY, int destX, int destY, int w, int h) {
    SDL_Rect src, dest;

    src.x = srcX * scale;
    src.y = srcY * scale;
    src.w = w * scale;
    src.h = h * scale;

    dest.x = destX * scale;
    dest.y = destY * scale;
    dest.w = w * scale;
    dest.h = h * scale;

    SDL_BlitSurface(surface, &src, surface, &dest);
}

void screenWriteScaledPixel(SDL_Surface *surface, int x, int y, Uint8 color) {
    int xs, ys;

    for (xs = 0; xs < scale; xs++) {
        for (ys = 0; ys < scale; ys++) {
            ((Uint8 *)surface->pixels)[(y*scale+ys) * scale * (surface->w / scale) + x*scale + xs] = color;
        }
    }
}

void screenFixIntroScreen(const unsigned char *sigData) {
    const Uint8 sigColor = 0xB;   /* light cyan */
    const Uint8 lineColor = 4;   /* dark red */
    int i,x,y;

    /* -----------------------------------------------------------------------------
     * copy "present" to new location between "Origin Systems, Inc." and "Ultima IV"
     * ----------------------------------------------------------------------------- */
    screenCopyRect(bkgds[BKGD_INTRO],136,0,136,0x21,54,5);
    
    /* ----------------------------
     * erase the original "present"
     * ---------------------------- */
    screenFillRect(bkgds[BKGD_INTRO],136,0,54,5,0);

    /* -----------------------------
     * draw "Lord British" signature
     * ----------------------------- */
    i = 0;
    while (sigData[i] != 0) {
        /*  (x/y) are unscaled coordinates, i.e. in 320x200  */
        x = sigData[i] + 0x14;
        y = 0xBF - sigData[i+1];
        screenWriteScaledPixel(bkgds[BKGD_INTRO],x,y,sigColor);
        screenWriteScaledPixel(bkgds[BKGD_INTRO],x+1,y,sigColor);
        i += 2;
    }

    /* --------------------------------------------------------------
     * draw the red line between "Origin Systems, Inc." and "present"
     * -------------------------------------------------------------- */
    /* (x/y) are unscaled coordinates */
    screenFillRect(bkgds[BKGD_INTRO], 0x56, 0x1F, 0xEE - 0x56, 1, lineColor);
}

/**
 * Load in the background images from the "*.ega" files.
 */
int screenLoadBackgrounds() {
    unsigned int i;
    int ret;
    static const struct {
        BackgroundType bkgd;
        const char *filename;
    } lzwBkgdInfo[] = {
        { BKGD_INTRO, "title.ega" },
        { BKGD_TREE, "tree.ega" },
        { BKGD_PORTAL, "portal.ega" },
        { BKGD_OUTSIDE, "outside.ega" },
        { BKGD_INSIDE, "inside.ega" },
        { BKGD_WAGON, "wagon.ega" },
        { BKGD_GYPSY, "gypsy.ega" },
        { BKGD_ABACUS, "abacus.ega" }
    };

    for (i = 0; i < sizeof(lzwBkgdInfo) / sizeof(lzwBkgdInfo[0]); i++) {
        /* no vga version of lzw files... yet */
        ret = screenLoadLzwImageEga(&bkgds[lzwBkgdInfo[i].bkgd], 320, 200, lzwBkgdInfo[i].filename);
        if (!ret)
            return 0;
    }

    if (!forceEga)
        ret = screenLoadRleImageVga(&bkgds[BKGD_BORDERS], 320, 200, "start.ega", "u4vga.pal");
    if (!ret && !forceVga)
        ret = screenLoadRleImageEga(&bkgds[BKGD_BORDERS], 320, 200, "start.ega");
    if (!ret)
        return 0;


    return 1;
}

/**
 * Load in the intro animation images from the "*.ega" files.
 */
int screenLoadIntroAnimations() {
    unsigned int i;
    int ret;
    static const struct {
        AnimType anim;
        const char *filename;
    } lzwAnimInfo[] = {
        { ANIM_HONCOM, "honcom.ega" },
        { ANIM_VALJUS, "valjus.ega" },
        { ANIM_SACHONOR, "sachonor.ega" },
        { ANIM_SPIRHUM, "spirhum.ega" },
        { ANIM_ANIMATE, "animate.ega" }
    };

    for (i = 0; i < sizeof(lzwAnimInfo) / sizeof(lzwAnimInfo[0]); i++) {
        /* no vga version of lzw files... yet */
        ret = screenLoadLzwImageEga(&introAnimations[lzwAnimInfo[i].anim], 320, 200, lzwAnimInfo[i].filename);
        if (!ret)
            return 0;
    }

    return 1;
}

void screenFreeIntroAnimations() {
    unsigned int i;

    for (i = 0; i < sizeof(introAnimations) / sizeof(introAnimations[0]); i++)
        SDL_FreeSurface(introAnimations[i]);
}

void screenFreeIntroBackgrounds() {
    int i;

    for (i = 0; i < BKGD_MAX; i++) {
        if (i == BKGD_BORDERS)
            continue;
        SDL_FreeSurface(bkgds[i]);
        bkgds[i] = NULL;
    }
}

/**
 * Load the tiles graphics from the "shapes.ega" or "shapes.vga" file.
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
 * Load the character set graphics from the "charset.ega" or "charset.vga" file.
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

int screenLoadLzwImageEga(SDL_Surface **surface, int width, int height, const char *filename) {
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

    if (!screenLoadPaletteEga(*surface)) {
        SDL_FreeSurface(*surface);
        u4fclose(in);
        return 0;
    }

    if (decompress_u4_file(in, u4flength(in), (void **) &data) == -1) {
        SDL_FreeSurface(*surface);
        u4fclose(in);
        return 0;
    }


    p = (*surface)->pixels;
    for (y = 0; y < height; y++) {
        for (x = 0; x < width / 2; x++) {
            for (xs = 0; xs < scale; xs++) {
                *p = data[y * width / 2 + x] >> 4;
                p++;
            }
            for (xs = 0; xs < scale; xs++) {
                *p = data[y * width / 2 + x] & 0x0f;
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
void screenDrawBackground(BackgroundType bkgd) {
    SDL_Rect r;

    assert(bkgd < BKGD_MAX);

    r.x = 0;
    r.y = 0;
    r.w = 320 * scale;
    r.h = 200 * scale;
    SDL_BlitSurface(bkgds[bkgd], &r, screen, &r);
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

    if (tileGetAnimationStyle(tile) == ANIM_SCROLL)
        offset = screenCurrentCycle * scale;
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

void screenAnimateIntro(int frame) {
    SDL_Rect src, dest;

    if (frame == 0) {
        src.x = 0 * scale;
        src.y = 152 * scale;
    } else if (frame == 1) {
        src.x = 24 * scale;
        src.y = 152 * scale;
    } else
        assert(0);
        
    src.w = 24 * scale;
    src.h = 24 * scale;

    dest.x = 72 * scale;
    dest.y = 68 * scale;
    dest.w = 24 * scale;
    dest.h = 24 * scale;

    SDL_BlitSurface(bkgds[BKGD_TREE], &src, screen, &dest);
}

void screenEraseTextArea(int x, int y, int width, int height) {
    SDL_Rect dest;

    dest.x = x * 8 * scale;
    dest.y = y * 8 * scale;
    dest.w = width * 8 * scale;
    dest.h = height * 8 * scale;

    SDL_FillRect(screen, &dest, 0);
}

/**
 * Draws a card on the screen for the character creation sequence with
 * the gypsy.
 */
void screenShowCard(int pos, int card) {
    SDL_Rect src, dest;

    assert(pos == 0 || pos == 1);
    assert(card < 8);

    src.x = ((card % 2) ? 218 : 12) * scale;
    src.y = 12 * scale;
    src.w = 90 * scale;
    src.h = 124 * scale;

    dest.x = (pos ? 218 : 12) * scale;
    dest.y = 12 * scale;
    dest.w = 90 * scale;
    dest.h = 124 * scale;

    SDL_BlitSurface(introAnimations[card / 2], &src, screen, &dest);
}

void screenShowBeastie(int beast, int frame) {
    SDL_Rect src, dest;
    int col, row, destx;
    
    assert(beast == 0 || beast == 1);

    row = frame % 6;
    col = frame / 6;

    if (beast == 0) {
        src.x = col * 56 * scale;
        src.w = 56 * scale;
    }
    else {
        src.x = (176 + col * 48) * scale;
        src.w = 48 * scale;
    }

    src.y = row * 32 * scale;
    src.h = 32 * scale;

    destx = beast ? (320 - 48) : 0;

    dest.x = destx * scale;
    dest.y = 0 * scale;
    dest.w = src.w;
    dest.h = src.h;

    SDL_BlitSurface(introAnimations[ANIM_ANIMATE], &src, screen, &dest);
}
