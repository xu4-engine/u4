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
#include "settings.h"
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

typedef SDL_Surface *(*ScreenScaler)(SDL_Surface *src, int scale, int n);

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
SDL_Surface *screenScale(SDL_Surface *src, int scale, int n, int filter);
SDL_Surface *screenScaleDefault(SDL_Surface *src, int scale, int n);
SDL_Surface *screenScale2xBilinear(SDL_Surface *src, int scale, int n);
SDL_Surface *screenScale2xSaI(SDL_Surface *src, int scale, int N);
SDL_Surface *screenAdvanceMAMEScale2x(SDL_Surface *src, int scale, int N);
Uint32 getPixel(SDL_Surface *s, int x, int y);
void putPixel(SDL_Surface *s, int x, int y, Uint32 pixel);

SDL_Surface *screen;
SDL_Surface *bkgds[BKGD_MAX];
SDL_Surface *introAnimations[ANIM_MAX];
SDL_Surface *tiles, *charset;
int scale, forceEga, forceVga;
ScreenScaler filterScaler;

extern int verbose;

#define RLE_RUNSTART 02

void screenInit() {

    /* set up scaling parameters */
    scale = settings->scale;
    switch (settings->filter) {
    case SCL_2xBi:
        filterScaler = &screenScale2xBilinear;
        break;
    case SCL_2xSaI:
        filterScaler = &screenScale2xSaI;
        break;
    case SCL_AdvanceMAME:
        filterScaler = &screenAdvanceMAMEScale2x;
        break;
    default:
        filterScaler = NULL;
        break;
    }
    if (verbose)
        printf("using %s scaler\n", settingsFilterToString(settings->filter));

    if (scale < 1 || scale > 5)
        scale = 2;

    forceEga = 0;
    forceVga = 0;

    /* start SDL */
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
        fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
        exit(1);
    }

    screen = SDL_SetVideoMode(320 * scale, 200 * scale, 16, SDL_SWSURFACE | SDL_ANYFORMAT | (settings->fullscreen ? SDL_FULLSCREEN : 0));
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

void screenDelete() {
    SDL_FreeSurface(bkgds[BKGD_BORDERS]);
    SDL_FreeSurface(tiles);
    SDL_FreeSurface(charset);
    SDL_Quit();
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

    for (i = 0; i < sizeof(introAnimations) / sizeof(introAnimations[0]); i++) {
        SDL_FreeSurface(introAnimations[i]);
        introAnimations[i] = NULL;
    }
}

void screenFreeIntroBackgrounds() {
    int i;

    for (i = 0; i < BKGD_MAX; i++) {
        if (i == BKGD_BORDERS || bkgds[i] == NULL)
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
    SDL_Surface *unscaled;
    int x, y;
    Uint8 *p;

    in = u4fopen(filename);
    if (!in)
        return 0;

    unscaled = SDL_CreateRGBSurface(SDL_HWSURFACE, width, height * n, 8, 0, 0, 0, 0);
    if (!unscaled) {
        u4fclose(in);
        return 0;
    }

    if (!screenLoadPaletteEga(unscaled)) {
        SDL_FreeSurface(unscaled);
        u4fclose(in);
        return 0;
    }

    p = unscaled->pixels;
    for (y = 0; y < height * n; y++) {
        for (x = 0; x < width; x += 2) {
            int temp = getc(in);
            *p = temp >> 4;
            p++;
            *p = temp & 0x0f;
            p++;
        }
        p += unscaled->pitch - width;
    }

    u4fclose(in);

    (*surface) = screenScale(unscaled, scale, n, 1);

    return 1;
}

/**
 * Load a tile set from a ".vga" tile set file (from the U4 upgrade).
 */
int screenLoadTileSetVga(SDL_Surface **surface, int width, int height, int n, const char *filename, const char *palette_fname) {
    FILE *in;
    SDL_Surface *unscaled;
    int x, y;
    Uint8 *p;

    in = u4fopen(filename);
    if (!in)
        return 0;

    unscaled = SDL_CreateRGBSurface(SDL_HWSURFACE, width, height * n, 8, 0, 0, 0, 0);
    if (!unscaled) {
        u4fclose(in);
        return 0;
    }

    if (!screenLoadPaletteVga(unscaled, palette_fname)) {
        SDL_FreeSurface(unscaled);
        u4fclose(in);
        return 0;
    }

    p = unscaled->pixels;
    for (y = 0; y < height * n; y++) {
        for (x = 0; x < width; x++) {
            int temp = getc(in);
            *p++ = temp;
        }
        p += unscaled->pitch - width;
    }

    u4fclose(in);

    (*surface) = screenScale(unscaled, scale, n, 1);

    return 1;
}

/**
 * Load an image from an ".ega" RLE encoded image file.
 */
int screenLoadRleImageEga(SDL_Surface **surface, int width, int height, const char *filename) {
    FILE *in;
    SDL_Surface *unscaled;
    Uint8 *p;

    in = u4fopen(filename);
    if (!in)
        return 0;

    unscaled = SDL_CreateRGBSurface(SDL_HWSURFACE, width, height, 8, 0, 0, 0, 0);
    if (!unscaled) {
        u4fclose(in);
        return 0;
    }

    if (!screenLoadPaletteEga(unscaled)) {
        SDL_FreeSurface(unscaled);
        u4fclose(in);
        return 0;
    }

    p = unscaled->pixels;
    while (p < ((Uint8 *)unscaled->pixels) + (unscaled->pitch * unscaled->h)) {
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

    u4fclose(in);

    (*surface) = screenScale(unscaled, scale, 1, 1);

    return 1;
}

/**
 * Load an image from an ".ega" RLE encoded image file (from the U4 upgrade).
 */
int screenLoadRleImageVga(SDL_Surface **surface, int width, int height, const char *filename, const char *palette_fname) {
    FILE *in;
    SDL_Surface *unscaled;
    Uint8 *p;

    in = u4fopen(filename);
    if (!in)
        return 0;

    unscaled = SDL_CreateRGBSurface(SDL_HWSURFACE, width, height, 8, 0, 0, 0, 0);
    if (!unscaled) {
        u4fclose(in);
        return 0;
    }

    if (!screenLoadPaletteVga(unscaled, palette_fname)) {
        SDL_FreeSurface(unscaled);
        u4fclose(in);
        return 0;
    }

    p = unscaled->pixels;
    while (p < ((Uint8 *)unscaled->pixels) + (unscaled->pitch * unscaled->h)) {
        int temp = getc(in);
        if (temp == EOF) {
            u4fclose(in);
            SDL_FreeSurface(unscaled);
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

    u4fclose(in);

    (*surface) = screenScale(unscaled, scale, 1, 1);

    return 1;
}

int screenLoadLzwImageEga(SDL_Surface **surface, int width, int height, const char *filename) {
    FILE *in;
    SDL_Surface *unscaled;
    int x, y;
    Uint8 *p, *data;

    in = u4fopen(filename);
    if (!in)
        return 0;

    unscaled = SDL_CreateRGBSurface(SDL_HWSURFACE, width, height, 8, 0, 0, 0, 0);
    if (!unscaled) {
        u4fclose(in);
        return 0;
    }

    if (!screenLoadPaletteEga(unscaled)) {
        SDL_FreeSurface(unscaled);
        u4fclose(in);
        return 0;
    }

    if (decompress_u4_file(in, u4flength(in), (void **) &data) == -1) {
        SDL_FreeSurface(unscaled);
        u4fclose(in);
        return 0;
    }

    p = unscaled->pixels;
    for (y = 0; y < height; y++) {
        for (x = 0; x < width / 2; x++) {
            *p = data[y * width / 2 + x] >> 4;
            p++;
            *p = data[y * width / 2 + x] & 0x0f;
            p++;
        }
        p += unscaled->pitch - width;
    }
    free(data);

    u4fclose(in);

    (*surface) = screenScale(unscaled, scale, 1, 0);

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
 * Draw a character from the charset onto the screen, but mask it with
 * horizontal lines.  This is used for the avatar symbol in the
 * statistics area, where a line is masked out for each virtue in
 * which the player is not an avatar.
 */
void screenShowCharMasked(int chr, int x, int y, unsigned char mask) {
    SDL_Rect dest;
    int i;

    screenShowChar(chr, x, y);
    dest.x = x * charset->w;
    dest.w = charset->w;
    dest.h = scale;
    for (i = 0; i < 8; i++) {
        if (mask & (1 << i)) {
            dest.y = y * (charset->h / N_CHARS) + (i * scale);
            SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 0, 0, 0));
        }
    }
}

/**
 * Draw a tile graphic on the screen.
 */
void screenShowTile(const ScreenTileInfo *tileInfo, int x, int y) {
    int offset;
    SDL_Rect src, dest;

    if (tileGetAnimationStyle(tileInfo->tile) == ANIM_SCROLL)
        offset = screenCurrentCycle * scale;
    else
        offset = 0;

    src.x = 0;
    src.y = tileInfo->tile * (tiles->h / N_TILES);
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
    src.y = (tileInfo->tile + 1) * (tiles->h / N_TILES) - offset;
    src.w = tiles->w;
    src.h = offset;
    dest.x = x * tiles->w + (BORDER_WIDTH * scale);
    dest.y = y * (tiles->h / N_TILES) + (BORDER_HEIGHT * scale);
    dest.w = tiles->w;
    dest.h = tiles->h / N_TILES;

    SDL_BlitSurface(tiles, &src, screen, &dest);
}

/**
 * Draw a tile graphic on the screen.
 */
void screenShowGemTile(const ScreenTileInfo *tileInfo, int x, int y) {
    /* FIXME: show gem tile rather than some random color rectangle */
    SDL_Rect src, dest;

    src.x = 0;
    src.y = tileInfo->tile * (tiles->h / N_TILES);
    src.w = GEMTILE_W * scale;
    src.h = GEMTILE_H * scale;
    dest.x = (GEMAREA_X + (x * GEMTILE_W)) * scale;
    dest.y = (GEMAREA_Y + (y * GEMTILE_H)) * scale;
    dest.w = GEMTILE_W * scale;
    dest.h = GEMTILE_H * scale;

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
void screenRedrawScreen() {
    SDL_UpdateRect(screen, 0, 0, 0, 0);
}

void screenRedrawMapArea() {
    SDL_UpdateRect(screen, BORDER_WIDTH * scale, BORDER_HEIGHT * scale, VIEWPORT_W * TILE_WIDTH * scale, VIEWPORT_H * TILE_HEIGHT * scale);
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

    dest.x = x * CHAR_WIDTH * scale;
    dest.y = y * CHAR_HEIGHT * scale;
    dest.w = width * CHAR_WIDTH * scale;
    dest.h = height * CHAR_HEIGHT * scale;

    SDL_FillRect(screen, &dest, 0);
}

void screenRedrawTextArea(int x, int y, int width, int height) {
    SDL_UpdateRect(screen, x * CHAR_WIDTH * scale, y * CHAR_HEIGHT * scale, width * CHAR_WIDTH * scale, height * CHAR_HEIGHT * scale);
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

void screenGemUpdate() {
    ScreenTileInfo tileInfo;
    int x, y;

    screenFillRect(screen, BORDER_WIDTH, BORDER_HEIGHT, VIEWPORT_W * TILE_WIDTH, VIEWPORT_H * TILE_HEIGHT, 0);

    for (x = 0; x < GEMAREA_W; x++) {
        for (y = 0; y < GEMAREA_H; y++) {
            tileInfo = screenViewportTile(GEMAREA_W, GEMAREA_H, x, y);
            screenShowGemTile(&tileInfo, x, y);
        }
    }
    screenRedrawMapArea();

    screenUpdateCursor();
    screenUpdateMoons();
    screenUpdateWind();
}

SDL_Surface *screenScale(SDL_Surface *src, int scale, int n, int filter) {
    SDL_Surface *dest;

    dest = src;

    if (filter && filterScaler && (scale % 2 == 0)) {
        dest = (*filterScaler)(src, 2, n);
        scale /= 2;
        SDL_FreeSurface(src);
        src = dest;
    }

    if (scale != 1) {
        dest = screenScaleDefault(src, scale, n);
        SDL_FreeSurface(src);
    }

    return dest;
}

SDL_Surface *screenScaleDefault(SDL_Surface *src, int scale, int n) {
    int x, y, i, j;
    SDL_Surface *dest;

    dest = SDL_CreateRGBSurface(SDL_HWSURFACE, src->w * scale, src->h * scale, src->format->BitsPerPixel, src->format->Rmask, src->format->Gmask, src->format->Bmask, src->format->Amask);
    if (!dest)
        return NULL;

    if (dest->format->palette)
        memcpy(dest->format->palette->colors, src->format->palette->colors, sizeof(SDL_Color) * src->format->palette->ncolors);

    for (y = 0; y < src->h; y++) {
        for (x = 0; x < src->w; x++) {
            for (i = 0; i < scale; i++) {
                for (j = 0; j < scale; j++)
                    memcpy(&((char *)dest->pixels)[dest->pitch * (y * scale + i) + (x * scale + j) * src->format->BytesPerPixel], &((char *)src->pixels)[src->pitch * y + (x * src->format->BytesPerPixel)], src->format->BytesPerPixel);
            }
        }
    }

    return dest;
}

SDL_Surface *screenScale2xBilinear(SDL_Surface *src, int scale, int n) {
    int i, x, y, xoff, yoff;
    SDL_Color a, b, c, d;
    SDL_Surface *dest;
    Uint32 rmask, gmask, bmask, amask;

    /* this bilinear scaler works only with 8-bit source images, scaled by 2x */
    assert(src->format->palette);
    assert(scale == 2);

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    rmask = 0xff000000;
    gmask = 0x00ff0000;
    bmask = 0x0000ff00;
    amask = 0x000000ff;
#else
    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = 0xff000000;
#endif

    dest = SDL_CreateRGBSurface(SDL_HWSURFACE, src->w * scale, src->h * scale, 32, rmask, gmask, bmask, amask);
    if (!dest)
        return NULL;

    /*
     * Each pixel in the source image is translated into four in the
     * destination.  The destination pixels are dependant on the pixel
     * itself, and the three surrounding pixels (A is the original
     * pixel):
     * A B
     * C D
     * The four destination pixels mapping to A are calculated as
     * follows:
     * [   A   ] [  (A+B)/2  ]
     * [(A+C)/2] [(A+B+C+D)/4]
     */

    for (i = 0; i < n; i++) {
        for (y = (src->h / n) * i; y < (src->h / n) * (i + 1); y++) {
            if (y == (src->h / n) * (i + 1) - 1)
                yoff = 0;
            else
                yoff = 1;

            for (x = 0; x < src->w; x++) {
                if (x == src->w - 1)
                    xoff = 0;
                else
                    xoff = 1;

                SDL_GetRGB(getPixel(src, x, y), src->format, &a.r, &a.g, &a.b);
                SDL_GetRGB(getPixel(src, x + xoff, y), src->format, &b.r, &b.g, &b.b);
                SDL_GetRGB(getPixel(src, x, y + yoff), src->format, &c.r, &c.g, &c.b);
                SDL_GetRGB(getPixel(src, x + xoff, y + yoff), src->format, &d.r, &d.g, &d.b);

                putPixel(dest, x * 2, y * 2, SDL_MapRGB(dest->format, a.r, a.g, a.b));
                putPixel(dest, x * 2 + 1, y * 2, SDL_MapRGB(dest->format, (a.r + b.r) >> 1, (a.g + b.g) >> 1, (a.b + b.b) >> 1));
                putPixel(dest, x * 2, y * 2 + 1, SDL_MapRGB(dest->format, (a.r + c.r) >> 1, (a.g + c.g) >> 1, (a.b + c.b) >> 1));
                putPixel(dest, x * 2 + 1, y * 2 + 1, SDL_MapRGB(dest->format, (a.r + b.r + c.r + d.r) >> 2, (a.g + b.g + c.g + d.g) >> 2, (a.b + b.b + c.b + d.b) >> 2));
            }
        }
    }

    return dest;
}

int colorEqual(SDL_Color a, SDL_Color b) {
    return 
        a.r == b.r &&
        a.g == b.g &&
        a.b == b.b;
}

int _2xSaI_GetResult1(SDL_Color a, SDL_Color b, SDL_Color c, SDL_Color d) {
    int x = 0;
    int y = 0;
    int r = 0;
    if (colorEqual(a, c)) x++; else if (colorEqual(b, c)) y++;
    if (colorEqual(a, d)) x++; else if (colorEqual(b, d)) y++;
    if (x <= 1) r++; 
    if (y <= 1) r--;
    return r;
}

int _2xSaI_GetResult2(SDL_Color a, SDL_Color b, SDL_Color c, SDL_Color d) {
    int x = 0; 
    int y = 0;
    int r = 0;
    if (colorEqual(a, c)) x++; else if (colorEqual(b, c)) y++;
    if (colorEqual(a, d)) x++; else if (colorEqual(b, d)) y++;
    if (x <= 1) r--;
    if (y <= 1) r++;
    return r;
}

SDL_Surface *screenScale2xSaI(SDL_Surface *src, int scale, int N) {
    int ii, x, y, xoff0, xoff1, xoff2, yoff0, yoff1, yoff2;
    SDL_Color a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p;
    Uint32 prod0, prod1, prod2;
    SDL_Surface *dest;
    Uint32 rmask, gmask, bmask, amask;

    /* this scaler works only with 8-bit source images, scaled by 2x */
    assert(src->format->palette);
    assert(scale == 2);

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    rmask = 0xff000000;
    gmask = 0x00ff0000;
    bmask = 0x0000ff00;
    amask = 0x000000ff;
#else
    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = 0xff000000;
#endif

    dest = SDL_CreateRGBSurface(SDL_HWSURFACE, src->w * scale, src->h * scale, 32, rmask, gmask, bmask, amask);
    if (!dest)
        return NULL;

    /*
     * Each pixel in the source image is translated into four in the
     * destination.  The destination pixels are dependant on the pixel
     * itself, and the surrounding pixels as shown below (A is the
     * original pixel):
     * I E F J
     * G A B K
     * H C D L
     * M N O P
     */

    for (ii = 0; ii < N; ii++) {
        for (y = (src->h / N) * ii; y < (src->h / N) * (ii + 1); y++) {
            if (y == 0)
                yoff0 = 0;
            else
                yoff0 = -1;
            if (y == (src->h / N) * (ii + 1) - 1) {
                yoff1 = 0;
                yoff2 = 0;
            }
            else if (y == (src->h / N) * (ii + 1) - 2) {
                yoff1 = 1;
                yoff2 = 1;
            }
            else {
                yoff1 = 1;
                yoff2 = 2;
            }
                

            for (x = 0; x < src->w; x++) {
                if (x == 0)
                    xoff0 = 0;
                else
                    xoff0 = -1;
                if (x == src->w - 1) {
                    xoff1 = 0;
                    xoff2 = 0;
                }
                else if (x == src->w - 2) {
                    xoff1 = 1;
                    xoff2 = 1;
                }
                else {
                    xoff1 = 1;
                    xoff2 = 2;
                }
                

                SDL_GetRGB(getPixel(src, x, y), src->format, &a.r, &a.g, &a.b);
                SDL_GetRGB(getPixel(src, x + xoff1, y), src->format, &b.r, &b.g, &b.b);
                SDL_GetRGB(getPixel(src, x, y + yoff1), src->format, &c.r, &c.g, &c.b);
                SDL_GetRGB(getPixel(src, x + xoff1, y + yoff1), src->format, &d.r, &d.g, &d.b);

                SDL_GetRGB(getPixel(src, x, y + yoff0), src->format, &e.r, &e.g, &e.b);
                SDL_GetRGB(getPixel(src, x + xoff1, y + yoff0), src->format, &f.r, &f.g, &f.b);
                SDL_GetRGB(getPixel(src, x + xoff0, y), src->format, &g.r, &g.g, &g.b);
                SDL_GetRGB(getPixel(src, x + xoff0, y + yoff1), src->format, &h.r, &h.g, &h.b);
                
                SDL_GetRGB(getPixel(src, x + xoff0, y + yoff0), src->format, &i.r, &i.g, &i.b);
                SDL_GetRGB(getPixel(src, x + xoff2, y + yoff0), src->format, &j.r, &j.g, &j.b);
                SDL_GetRGB(getPixel(src, x + xoff0, y), src->format, &k.r, &k.g, &k.b);
                SDL_GetRGB(getPixel(src, x + xoff0, y + yoff1), src->format, &l.r, &l.g, &l.b);

                SDL_GetRGB(getPixel(src, x + xoff0, y + yoff2), src->format, &m.r, &m.g, &m.b);
                SDL_GetRGB(getPixel(src, x, y + yoff2), src->format, &n.r, &n.g, &n.b);
                SDL_GetRGB(getPixel(src, x + xoff1, y + yoff2), src->format, &o.r, &o.g, &o.b);
                SDL_GetRGB(getPixel(src, x + xoff2, y + yoff2), src->format, &p.r, &p.g, &p.b);

                if (colorEqual(a, d) && !colorEqual(b, c)) {
                    if ((colorEqual(a, e) && colorEqual(b, l)) ||
                        (colorEqual(a, c) && colorEqual(a, f) && !colorEqual(b, e) && colorEqual(b, j)))
                        prod0 = SDL_MapRGB(dest->format, a.r, a.g, a.b);
                    else
                        prod0 = SDL_MapRGB(dest->format, (a.r + b.r) >> 1, (a.g + b.g) >> 1, (a.b + b.b) >> 1);
                    if ((colorEqual(a, g) && colorEqual(c, o)) ||
                        (colorEqual(a, b) && colorEqual(a, h) && !colorEqual(g, c) && colorEqual(c, m)))
                        prod1 = SDL_MapRGB(dest->format, a.r, a.g, a.b);
                    else
                        prod1 = SDL_MapRGB(dest->format, (a.r + c.r) >> 1, (a.g + c.g) >> 1, (a.b + c.b) >> 1);
                    
                    prod2 = SDL_MapRGB(dest->format, a.r, a.g, a.b);
                }
                else if (colorEqual(b, c) && !colorEqual(a, d)) {
                    if ((colorEqual(b, f) && colorEqual(a, h)) ||
                        (colorEqual(b, e) && colorEqual(b, d) && !colorEqual(a, f) && colorEqual(a, i)))
                        prod0 = SDL_MapRGB(dest->format, b.r, b.g, b.b);
                    else
                        prod0 = SDL_MapRGB(dest->format, (a.r + b.r) >> 1, (a.g + b.g) >> 1, (a.b + b.b) >> 1);
                    if ((colorEqual(c, h) && colorEqual(a, f)) ||
                        (colorEqual(c, g) && colorEqual(c, d) && !colorEqual(a, h) && colorEqual(a, i)))
                        prod1 = SDL_MapRGB(dest->format, c.r, c.g, c.b);
                    else
                        prod1 = SDL_MapRGB(dest->format, (a.r + c.r) >> 1, (a.g + c.g) >> 1, (a.b + c.b) >> 1);
                    prod2 = SDL_MapRGB(dest->format, b.r, b.g, b.b);
                }
                else if (colorEqual(a, d) && colorEqual(b, c)) {
                    if (colorEqual(a, b)) {
                        prod0 = SDL_MapRGB(dest->format, a.r, a.g, a.b);
                        prod1 = SDL_MapRGB(dest->format, a.r, a.g, a.b);
                        prod2 = SDL_MapRGB(dest->format, a.r, a.g, a.b);
                    }
                    else {
                        int r = 0;
                        prod0 = SDL_MapRGB(dest->format, (a.r + b.r) >> 1, (a.g + b.g) >> 1, (a.b + b.b) >> 1);
                        prod1 = SDL_MapRGB(dest->format, (a.r + c.r) >> 1, (a.g + c.g) >> 1, (a.b + c.b) >> 1);

                        r += _2xSaI_GetResult1(a, b, g, e);
                        r += _2xSaI_GetResult2(b, a, k, f);
                        r += _2xSaI_GetResult2(b, a, h, n);
                        r += _2xSaI_GetResult1(a, b, l, o);

                        if (r > 0)
                            prod2 = SDL_MapRGB(dest->format, a.r, a.g, a.b);
                        else if (r < 0)
                            prod2 = SDL_MapRGB(dest->format, b.r, b.g, b.b);
                        else
                            prod2 = SDL_MapRGB(dest->format, (a.r + b.r + c.r + d.r) >> 2, (a.g + b.g + c.g + d.g) >> 2, (a.b + b.b + c.b + d.b) >> 2);
                    }
                }
                else {
                    if (colorEqual(a, c) && colorEqual(a, f) && !colorEqual(b, e) && colorEqual(b, j))
                        prod0 = SDL_MapRGB(dest->format, a.r, a.g, a.b);
                    else if (colorEqual(b, e) && colorEqual(b, d) && !colorEqual(a, f) && colorEqual(a, i))
                        prod0 = SDL_MapRGB(dest->format, b.r, b.g, b.b);
                    else
                        prod0 = SDL_MapRGB(dest->format, (a.r + b.r) >> 1, (a.g + b.g) >> 1, (a.b + b.b) >> 1);

                    if (colorEqual(a, b) && colorEqual(a, h) && !colorEqual(g, c) && colorEqual(c, m))
                        prod1 = SDL_MapRGB(dest->format, a.r, a.g, a.b);
                    else if (colorEqual(c, g) && colorEqual(c, d) && !colorEqual(a, h) && colorEqual(a, i))
                        prod1 = SDL_MapRGB(dest->format, c.r, c.g, c.b);
                    else
                        prod1 = SDL_MapRGB(dest->format, (a.r + c.r) >> 1, (a.g + c.g) >> 1, (a.b + c.b) >> 1);

                    prod2 = SDL_MapRGB(dest->format, (a.r + b.r + c.r + d.r) >> 2, (a.g + b.g + c.g + d.g) >> 2, (a.b + b.b + c.b + d.b) >> 2);
                }


                putPixel(dest, x * 2, y * 2, SDL_MapRGB(dest->format, a.r, a.g, a.b));
                putPixel(dest, x * 2 + 1, y * 2, prod0);
                putPixel(dest, x * 2, y * 2 + 1, prod1);
                putPixel(dest, x * 2 + 1, y * 2 + 1, prod2);
            }
        }
    }

    return dest;
}

SDL_Surface *screenAdvanceMAMEScale2x(SDL_Surface *src, int scale, int n) {
    int ii, x, y, xoff0, xoff1, yoff0, yoff1;
    SDL_Color a, b, c, d, e, f, g, h, i;
    SDL_Color e0, e1, e2, e3;
    SDL_Surface *dest;
    Uint32 rmask, gmask, bmask, amask;

    /* this scaler works only with 8-bit source images, scaled by 2x */
    assert(src->format->palette);
    assert(scale == 2);

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    rmask = 0xff000000;
    gmask = 0x00ff0000;
    bmask = 0x0000ff00;
    amask = 0x000000ff;
#else
    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = 0xff000000;
#endif

    dest = SDL_CreateRGBSurface(SDL_HWSURFACE, src->w * scale, src->h * scale, src->format->BitsPerPixel, src->format->Rmask, src->format->Gmask, src->format->Bmask, src->format->Amask);
    if (!dest)
        return NULL;

    if (dest->format->palette)
        memcpy(dest->format->palette->colors, src->format->palette->colors, sizeof(SDL_Color) * src->format->palette->ncolors);

    /*
     * Each pixel in the source image is translated into four in the
     * destination.  The destination pixels are dependant on the pixel
     * itself, and the eight surrounding pixels (E is the original
     * pixel):
     * 
     * A B C
     * D E F
     * G H I
     */

    for (ii = 0; ii < n; ii++) {
        for (y = (src->h / n) * ii; y < (src->h / n) * (ii + 1); y++) {
            if (y == 0)
                yoff0 = 0;
            else
                yoff0 = -1;
            if (y == (src->h / n) * (ii + 1) - 1)
                yoff1 = 0;
            else
                yoff1 = 1;

            for (x = 0; x < src->w; x++) {
                if (x == 0)
                    xoff0 = 0;
                else
                    xoff0 = -1;
                if (x == src->w - 1)
                    xoff1 = 0;
                else
                    xoff1 = 1;

                SDL_GetRGB(getPixel(src, x + xoff0, y + yoff0), src->format, &a.r, &a.g, &a.b);
                SDL_GetRGB(getPixel(src, x, y + yoff0), src->format, &b.r, &b.g, &b.b);
                SDL_GetRGB(getPixel(src, x + xoff1, y + yoff0), src->format, &c.r, &c.g, &c.b);

                SDL_GetRGB(getPixel(src, x + xoff0, y), src->format, &d.r, &d.g, &d.b);
                SDL_GetRGB(getPixel(src, x, y), src->format, &e.r, &e.g, &e.b);
                SDL_GetRGB(getPixel(src, x + xoff1, y), src->format, &f.r, &f.g, &f.b);

                SDL_GetRGB(getPixel(src, x + xoff0, y + yoff1), src->format, &g.r, &g.g, &g.b);
                SDL_GetRGB(getPixel(src, x, y + yoff1), src->format, &h.r, &h.g, &h.b);
                SDL_GetRGB(getPixel(src, x + xoff1, y + yoff1), src->format, &i.r, &i.g, &i.b);

                e0 = colorEqual(d, b) && (!colorEqual(b, f)) && (!colorEqual(d, h)) ? d : e;
                e1 = colorEqual(b, f) && (!colorEqual(b, d)) && (!colorEqual(f, h)) ? f : e;
                e2 = colorEqual(d, h) && (!colorEqual(d, b)) && (!colorEqual(h, f)) ? d : e;
                e3 = colorEqual(h, f) && (!colorEqual(d, h)) && (!colorEqual(b, f)) ? f : e;
                
                putPixel(dest, x * 2, y * 2, SDL_MapRGB(dest->format, e0.r, e0.g, e0.b));
                putPixel(dest, x * 2 + 1, y * 2, SDL_MapRGB(dest->format, e1.r, e1.g, e1.b));
                putPixel(dest, x * 2, y * 2 + 1, SDL_MapRGB(dest->format, e2.r, e2.g, e2.b));
                putPixel(dest, x * 2 + 1, y * 2 + 1, SDL_MapRGB(dest->format, e3.r, e3.g, e3.b));
            }
        }
    }

    return dest;
}

Uint32 getPixel(SDL_Surface *s, int x, int y) {
    int bpp = s->format->BytesPerPixel;

    Uint8 *p = (Uint8 *) s->pixels + y * s->pitch + x * bpp;

    switch(bpp) {
    case 1:
        return *p;

    case 2:
        return *(Uint16 *)p;

    case 3:
        if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
            return p[0] << 16 | p[1] << 8 | p[2];
        else
            return p[0] | p[1] << 8 | p[2] << 16;

    case 4:
        return *(Uint32 *)p;

    default:
        return 0;
    }
}

void putPixel(SDL_Surface *s, int x, int y, Uint32 pixel) {
    int bpp = s->format->BytesPerPixel;

    Uint8 *p = (Uint8 *)s->pixels + y * s->pitch + x * bpp;

    switch(bpp) {
    case 1:
        *p = pixel;
        break;

    case 2:
        *(Uint16 *)p = pixel;
        break;

    case 3:
        if(SDL_BYTEORDER == SDL_BIG_ENDIAN) {
            p[0] = (pixel >> 16) & 0xff;
            p[1] = (pixel >> 8) & 0xff;
            p[2] = pixel & 0xff;
        } else {
            p[0] = pixel & 0xff;
            p[1] = (pixel >> 8) & 0xff;
            p[2] = (pixel >> 16) & 0xff;
        }
        break;

    case 4:
        *(Uint32 *)p = pixel;
        break;
    }
}

