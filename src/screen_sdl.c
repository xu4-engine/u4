/*
 * $Id$
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <SDL.h>

#include "debug.h"
#include "error.h"
#include "event.h"
#include "rle.h"
#include "savegame.h"
#include "settings.h"
#include "screen.h"
#include "ttype.h"
#include "u4.h"
#include "u4file.h"

typedef enum {
    COMP_NONE,
    COMP_RLE,
    COMP_LZW
} CompressionType;

typedef SDL_Surface *(*ScreenScaler)(SDL_Surface *src, int scale, int n);

long decompress_u4_file(FILE *in, long filesize, void **out);
long decompress_u4_memory(void *in, long inlen, void **out);

void screenFillRect(SDL_Surface *surface, int x, int y, int w, int h, int r, int g, int b);
void screenCopyRect(SDL_Surface *surface, int srcX, int srcY, int destX, int destY, int w, int h);
void screenWriteScaledPixel(SDL_Surface *surface, int x, int y, int r, int g, int b);
int screenLoadBackground(BackgroundType bkgd);
void screenFreeIntroBackground();
int screenLoadTiles();
int screenLoadCharSet();
int screenLoadPaletteEga();
int screenLoadPaletteVga(const char *filename);
int screenLoadImageEga(SDL_Surface **surface, int width, int height, const char *filename, CompressionType comp);
int screenLoadImageVga(SDL_Surface **surface, int width, int height, const char *filename, CompressionType comp);
SDL_Surface *screenScale(SDL_Surface *src, int scale, int n, int filter);
SDL_Surface *screenScalePoint(SDL_Surface *src, int scale, int n);
SDL_Surface *screenScale2xBilinear(SDL_Surface *src, int scale, int n);
SDL_Surface *screenScale2xSaI(SDL_Surface *src, int scale, int N);
SDL_Surface *screenScaleScale2x(SDL_Surface *src, int scale, int N);
Uint32 getPixel(SDL_Surface *s, int x, int y);
void putPixel(SDL_Surface *s, int x, int y, Uint32 pixel);

SDL_Surface *screen;
SDL_Surface *bkgds[BKGD_MAX];
SDL_Surface *tiles, *charset;
SDL_Color egaPalette[16];
SDL_Color vgaPalette[256];
int scale, forceEga, forceVga;
ScreenScaler filterScaler;

const struct {
    const char *filename;
    int width, height;
    int hasVga;
    CompressionType comp;
    int filter;
    int introAnim;
    int scaleOnLoad;
} backgroundInfo[] = {
    /* main game borders */
    { "start.ega",    320, 200, 1, COMP_RLE, 1, 0, 1 },

    /* introduction screen images */
    { "title.ega",    320, 200, 0, COMP_LZW, 1, 1, 0 },
    { "title.ega",    320, 200, 0, COMP_LZW, 1, 1, 0 },
    { "tree.ega",     320, 200, 0, COMP_LZW, 1, 1, 1 },
    { "portal.ega",   320, 200, 0, COMP_LZW, 1, 1, 1 },
    { "outside.ega",  320, 200, 0, COMP_LZW, 1, 1, 1 },
    { "inside.ega",   320, 200, 0, COMP_LZW, 1, 1, 1 },
    { "wagon.ega",    320, 200, 0, COMP_LZW, 1, 1, 1 },
    { "gypsy.ega",    320, 200, 0, COMP_LZW, 1, 1, 1 },
    { "abacus.ega",   320, 200, 0, COMP_LZW, 1, 1, 1 },
    { "honcom.ega",   320, 200, 0, COMP_LZW, 1, 1, 1 },
    { "valjus.ega",   320, 200, 0, COMP_LZW, 1, 1, 1 },
    { "sachonor.ega", 320, 200, 0, COMP_LZW, 1, 1, 1 },
    { "spirhum.ega",  320, 200, 0, COMP_LZW, 1, 1, 1 },
    { "animate.ega",  320, 200, 0, COMP_LZW, 1, 1, 1 },

    /* abyss vision images */
    { "compassn.ega", 320, 200, 1, COMP_RLE, 1, 0, 1 },
    { "courage.ega",  320, 200, 1, COMP_RLE, 1, 0, 1 },
    { "honesty.ega",  320, 200, 1, COMP_RLE, 1, 0, 1 },
    { "honor.ega",    320, 200, 1, COMP_RLE, 1, 0, 1 },
    { "humility.ega", 320, 200, 1, COMP_RLE, 1, 0, 1 },
    { "justice.ega",  320, 200, 1, COMP_RLE, 1, 0, 1 },
    { "love.ega",     320, 200, 1, COMP_RLE, 1, 0, 1 },
    { "sacrific.ega", 320, 200, 1, COMP_RLE, 1, 0, 1 },
    { "spirit.ega",   320, 200, 1, COMP_RLE, 1, 0, 1 },
    { "truth.ega",    320, 200, 1, COMP_RLE, 1, 0, 1 },
    { "valor.ega",    320, 200, 1, COMP_RLE, 1, 0, 1 },

    /* shrine vision images */
    { "rune_0.ega",   320, 200, 1, COMP_RLE, 1, 0, 1 },
    { "rune_1.ega",   320, 200, 1, COMP_RLE, 1, 0, 1 },
    { "rune_2.ega",   320, 200, 1, COMP_RLE, 1, 0, 1 },
    { "rune_3.ega",   320, 200, 1, COMP_RLE, 1, 0, 1 },
    { "rune_4.ega",   320, 200, 1, COMP_RLE, 1, 0, 1 },
    { "rune_5.ega",   320, 200, 1, COMP_RLE, 1, 0, 1 },
    { "rune_6.ega",   320, 200, 1, COMP_RLE, 1, 0, 1 },
    { "rune_7.ega",   320, 200, 1, COMP_RLE, 1, 0, 1 },
    { "rune_8.ega",   320, 200, 1, COMP_RLE, 1, 0, 1 }
};

extern int verbose;

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
    case SCL_Scale2x:
        filterScaler = &screenScaleScale2x;
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
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0)
        errorFatal("unable to init SDL: %s", SDL_GetError());

    screen = SDL_SetVideoMode(320 * scale, 200 * scale, 16, SDL_SWSURFACE | SDL_ANYFORMAT | (settings->fullscreen ? SDL_FULLSCREEN : 0));
    if (!screen)
        errorFatal("unable to set video: %s", SDL_GetError());

    SDL_WM_SetCaption("Ultima IV", NULL);
#ifdef ICON_FILE
    SDL_WM_SetIcon(SDL_LoadBMP(ICON_FILE), NULL);
#endif

    screenLoadPaletteEga();
    if (!screenLoadPaletteVga("u4vga.pal"))
        forceEga = 1;

    if (!screenLoadBackground(BKGD_INTRO) ||
        !screenLoadBackground(BKGD_INTRO_EXTENDED) || 
        !screenLoadTiles() ||
        !screenLoadCharSet())
        errorFatal("unable to load data files: is Ultima IV installed?  See http://xu4.sourceforge.net/");

    eventKeyboardSetKeyRepeat(settings->keydelay, settings->keyinterval);
    SDL_ShowCursor(SDL_DISABLE); /* disable the mouse cursor */
}

void screenDelete() {
    if (bkgds[BKGD_BORDERS])
        SDL_FreeSurface(bkgds[BKGD_BORDERS]);
    SDL_FreeSurface(tiles);
    SDL_FreeSurface(charset);
    SDL_Quit();
}

/**
 *  Fills a rectangular screen area with the specified color.  The x,
 *  y, width and height parameters are unscaled, i.e. for 320x200.
 */
void screenFillRect(SDL_Surface *surface, int x, int y, int w, int h, int r, int g, int b) {
    SDL_Rect dest;

    dest.x = x * scale;
    dest.y = y * scale;
    dest.w = w * scale;
    dest.h = h * scale;

    if (SDL_FillRect(surface, &dest, SDL_MapRGB(surface->format, (Uint8)r, (Uint8)g, (Uint8)b)) != 0)
        errorWarning("screenFillRect: SDL_FillRect failed\n%s", SDL_GetError());
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

    if (SDL_BlitSurface(surface, &src, surface, &dest) != 0)
        errorWarning("screenCopyRect: SDL_BlitSurface failed\n%s", SDL_GetError());
}

void screenWriteScaledPixel(SDL_Surface *surface, int x, int y, int r, int g, int b) {
    int xs, ys;


    for (xs = 0; xs < scale; xs++) {
        for (ys = 0; ys < scale; ys++) {
            putPixel(surface, x*scale + xs, y*scale+ys, SDL_MapRGB(surface->format, (Uint8)r, (Uint8)g, (Uint8)b));
        }
    }
}

void screenFixIntroScreen(BackgroundType bkgd, const unsigned char *sigData) {
    int i, x, y;
    SDL_Rect src, dest;

    ASSERT(bkgds[bkgd] != NULL, "intro background must be loaded before fixing");

    /* -----------------------------------------------------------------------------
     * copy "present" to new location between "Origin Systems, Inc." and "Ultima IV"
     * ----------------------------------------------------------------------------- */

    /* we're working with an unscaled surface, so we can't use screenCopyRect, etc. */
    src.x = 136;  src.y = 0;   src.w = 55;  src.h = 5;
    dest.x = 136; dest.y = 33; dest.w = 55;  dest.h = 5;
    SDL_BlitSurface(bkgds[bkgd], &src, bkgds[bkgd], &dest);    

    /* ----------------------------
     * erase the original "present"
     * ---------------------------- */

    dest.x = 136; dest.y = 0; dest.w = 55; dest.h = 5;
    SDL_FillRect(bkgds[bkgd], &dest, SDL_MapRGB(bkgds[bkgd]->format, 0, 0, 0));    

    /* -----------------------------
     * draw "Lord British" signature
     * ----------------------------- */
    i = 0;
    while (sigData[i] != 0) {
        /*  (x/y) are unscaled coordinates, i.e. in 320x200  */
        x = sigData[i] + 0x14;
        y = 0xBF - sigData[i+1]; 
        putPixel(bkgds[bkgd], x, y, SDL_MapRGB(bkgds[bkgd]->format, 0, 255, 255));   /* cyan */
        putPixel(bkgds[bkgd], x+1, y, SDL_MapRGB(bkgds[bkgd]->format, 0, 255, 255)); /* cyan */
        i += 2;
    }

    /* --------------------------------------------------------------
     * draw the red line between "Origin Systems, Inc." and "present"
     * -------------------------------------------------------------- */
    /* we're still working with an unscaled surface */
    for (i = 86; i < 239; i++)
        putPixel(bkgds[bkgd], i, 31, SDL_MapRGB(bkgds[bkgd]->format, 128, 0, 0)); /* red */    

    /* scale and filter images now that we've completed them */
    bkgds[bkgd] = screenScale(bkgds[bkgd], scale, 1, backgroundInfo[bkgd].filter);    
}

void screenFixIntroScreenExtended(BackgroundType bkgd) {
    screenCopyRect(bkgds[bkgd], 0, 95, 0, 10, 320, 50);
    screenCopyRect(bkgds[bkgd], 0, 105, 0, 60, 320, 45);
}

/**
 * Load in a background image from a ".ega" file.
 */
int screenLoadBackground(BackgroundType bkgd) {
    int ret;
    SDL_Surface *unscaled;

    ret = 0;
    if (!forceEga && backgroundInfo[bkgd].hasVga)
        ret = screenLoadImageVga(&unscaled, 
                                 backgroundInfo[bkgd].width, 
                                 backgroundInfo[bkgd].height, 
                                 backgroundInfo[bkgd].filename, 
                                 backgroundInfo[bkgd].comp);
    if (!ret && !forceVga) {
        BackgroundType egaBkgd;

        /*
         * The original EGA rune image files are mapped to the virtues
         * differently than those provided with the VGA upgrade.  We
         * must map the VGA rune screens (0 = INF, 1 = Honesty, 2 =
         * Compassion, etc.) to their EGA equivalents (12012134 for
         * the virtues, and 5 for infinity).
         */
        switch (bkgd) {
        case BKGD_RUNE_INF:
            egaBkgd = BKGD_RUNE_INF + 5;
            break;
        case BKGD_SHRINE_HON:
        case BKGD_SHRINE_JUS:
        case BKGD_SHRINE_HNR:
            egaBkgd = BKGD_RUNE_INF + 1;
            break;
        case BKGD_SHRINE_COM:
        case BKGD_SHRINE_SAC:
            egaBkgd = BKGD_RUNE_INF + 2;
            break;
        case BKGD_SHRINE_VAL:
            egaBkgd = BKGD_RUNE_INF + 0;
            break;
        case BKGD_SHRINE_SPI:
            egaBkgd = BKGD_RUNE_INF + 3;
            break;
        case BKGD_SHRINE_HUM:
            egaBkgd = BKGD_RUNE_INF + 4;
            break;
        default:
            egaBkgd = bkgd;
            break;
        }

        ret = screenLoadImageEga(&unscaled, 
                                 backgroundInfo[bkgd].width, 
                                 backgroundInfo[bkgd].height, 
                                 backgroundInfo[egaBkgd].filename, 
                                 backgroundInfo[egaBkgd].comp);
    }

    if (!ret)
        return 0;

    /** 
     * wait to scale images that don't want to be scaled 
     * (like BKGD_INTRO, because we still have to fix it
     *  before filtering and scaling it)
     */
    if (backgroundInfo[bkgd].scaleOnLoad)
        bkgds[bkgd] = screenScale(unscaled, scale, 1, backgroundInfo[bkgd].filter);
    else
        bkgds[bkgd] = unscaled;
    
    return 1;
}

/**
 * Free up any background images used only in the animations.
 */
void screenFreeIntroBackgrounds() {
    int i;

    for (i = 0; i < BKGD_MAX; i++) {
        if ((!backgroundInfo[i].introAnim) || bkgds[i] == NULL)
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
        ret = screenLoadImageVga(&tiles, TILE_WIDTH, TILE_HEIGHT * N_TILES, "shapes.vga", COMP_NONE);

    if (!ret && !forceVga)
        ret = screenLoadImageEga(&tiles, TILE_WIDTH, TILE_HEIGHT * N_TILES, "shapes.ega", COMP_NONE);

    tiles = screenScale(tiles, scale, N_TILES, 1);

    return ret;
}

/**
 * Load the character set graphics from the "charset.ega" or "charset.vga" file.
 */
int screenLoadCharSet() {
    int ret = 0;

    if (!forceEga)
        ret = screenLoadImageVga(&charset, CHAR_WIDTH, CHAR_HEIGHT * N_CHARS, "charset.vga", COMP_NONE);

    if (!ret && !forceVga)
        ret = screenLoadImageEga(&charset, CHAR_WIDTH, CHAR_HEIGHT * N_CHARS, "charset.ega", COMP_NONE);

    charset = screenScale(charset, scale, N_CHARS, 1);

    return ret;
}

/**
 * Loads the basic EGA palette.
 */
int screenLoadPaletteEga() {
    #define setpalentry(i, red, green, blue) \
        egaPalette[i].r = red; \
        egaPalette[i].g = green; \
        egaPalette[i].b = blue;
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
 * Load the 256 color VGA palette from the given file.
 */
int screenLoadPaletteVga(const char *filename) {
    U4FILE *pal;
    int i;

    pal = u4fopen(filename);
    if (!pal)
        return 0;

    for (i = 0; i < 256; i++) {
        vgaPalette[i].r = u4fgetc(pal) * 255 / 63;
        vgaPalette[i].g = u4fgetc(pal) * 255 / 63;
        vgaPalette[i].b = u4fgetc(pal) * 255 / 63;
    }
    u4fclose(pal);

    return 1;
}

/**
 * Load an image from an ".ega" image file.
 */
int screenLoadImageEga(SDL_Surface **surface, int width, int height, const char *filename, CompressionType comp) {
    U4FILE *in;
    SDL_Surface *img;
    int x, y;
    unsigned char *compressed_data, *decompressed_data = NULL;
    long inlen, decompResult;

    in = u4fopen(filename);
    if (!in)
        return 0;

    inlen = u4flength(in);
    compressed_data = (Uint8 *) malloc(inlen);
    u4fread(compressed_data, 1, inlen, in);
    u4fclose(in);

    switch(comp) {
    case COMP_NONE:
        decompressed_data = compressed_data;
        decompResult = inlen;
        break;
    case COMP_RLE:
        decompResult = rleDecompressMemory(compressed_data, inlen, (void **) &decompressed_data);
        free(compressed_data);
        break;
    case COMP_LZW:
        decompResult = decompress_u4_memory(compressed_data, inlen, (void **) &decompressed_data);
        free(compressed_data);
        break;
    default:
        ASSERT(0, "invalid compression type %d", comp);
    }

    if (decompResult == -1) {
        if (decompressed_data)
            free(decompressed_data);
        return 0;
    }

    img = SDL_CreateRGBSurface(SDL_HWSURFACE, width, height, 8, 0, 0, 0, 0);
    if (!img) {
        if (decompressed_data)
            free(decompressed_data);
        return 0;
    }

    SDL_SetColors(img, egaPalette, 0, 16);

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x+=2) {
            putPixel(img, x, y, decompressed_data[(y * width + x) / 2] >> 4);
            putPixel(img, x + 1, y, decompressed_data[(y * width + x) / 2] & 0x0f);
        }
    }
    free(decompressed_data);

    (*surface) = img;

    return 1;
}

/**
 * Load an image from a ".vga" or ".ega" image file from the U4 VGA upgrade.
 */
int screenLoadImageVga(SDL_Surface **surface, int width, int height, const char *filename, CompressionType comp) {
    U4FILE *in;
    SDL_Surface *img;
    int x, y;
    unsigned char *compressed_data, *decompressed_data = NULL;
    long inlen, decompResult;

    in = u4fopen(filename);
    if (!in)
        return 0;

    inlen = u4flength(in);
    compressed_data = (Uint8 *) malloc(inlen);
    u4fread(compressed_data, 1, inlen, in);
    u4fclose(in);

    switch(comp) {
    case COMP_NONE:
        decompressed_data = compressed_data;
        decompResult = inlen;
        break;
    case COMP_RLE:
        decompResult = rleDecompressMemory(compressed_data, inlen, (void **) &decompressed_data);
        free(compressed_data);
        break;
    case COMP_LZW:
        decompResult = decompress_u4_memory(compressed_data, inlen, (void **) &decompressed_data);
        free(compressed_data);
        break;
    default:
        ASSERT(0, "invalid compression type %d", comp);
    }

    if (decompResult == -1 ||
        decompResult != (width * height)) {
        if (decompressed_data)
            free(decompressed_data);
        return 0;
    }

    img = SDL_CreateRGBSurface(SDL_HWSURFACE, width, height, 8, 0, 0, 0, 0);
    if (!img) {
        if (decompressed_data)
            free(decompressed_data);
        return 0;
    }

    SDL_SetColors(img, vgaPalette, 0, 256);

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++)
            putPixel(img, x, y, decompressed_data[y * width + x]);
    }
    free(decompressed_data);

    (*surface) = img;

    return 1;
}

/**
 * Draw the surrounding borders on the screen.
 */
void screenDrawBackground(BackgroundType bkgd) {
    SDL_Rect r;

    ASSERT(bkgd < BKGD_MAX, "bkgd out of range: %d", bkgd);

    if (bkgds[bkgd] == NULL) {
        if (!screenLoadBackground(bkgd))
            errorFatal("unable to load data files: is Ultima IV installed?  See http://xu4.sourceforge.net/");
    }

    r.x = 0;
    r.y = 0;
    r.w = backgroundInfo[bkgd].width * scale;
    r.h = backgroundInfo[bkgd].height * scale;
    SDL_BlitSurface(bkgds[bkgd], &r, screen, &r);
}

void screenDrawBackgroundInMapArea(BackgroundType bkgd) {
    SDL_Rect r;

    ASSERT(bkgd < BKGD_MAX, "bkgd out of range: %d", bkgd);

    if (bkgds[bkgd] == NULL) {
        if (!screenLoadBackground(bkgd))
            errorFatal("unable to load data files: is Ultima IV installed?  See http://xu4.sourceforge.net/");
    }

    r.x = BORDER_WIDTH * scale;
    r.y = BORDER_HEIGHT * scale;
    r.w = VIEWPORT_W * TILE_WIDTH * scale;
    r.h = VIEWPORT_W * TILE_WIDTH * scale;

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
void screenShowTile(unsigned char tile, int focus, int x, int y) {
    int offset, i, swaprow;
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

    if (offset != 0) {

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

    /*
     * animate flags
     */
    switch (tileGetAnimationStyle(tile)) {
    case ANIM_CITYFLAG:
        swaprow = 3;
        break;
    case ANIM_CASTLEFLAG:
    case ANIM_LCBFLAG:
        swaprow = 1;
        break;
    case ANIM_WESTSHIPFLAG:
    case ANIM_EASTSHIPFLAG:
        swaprow = 2;
        break;
    default:
        swaprow = -1;
        break;
    }

    if (swaprow != -1 && (rand() % 2)) {

        for (i = 0; i < (scale * 2) + 2; i++) {
            src.x = scale * 5;
            src.y = tile * (tiles->h / N_TILES) + (swaprow * scale) + i - 1;
            src.w = tiles->w - (scale * 5);
            src.h = 1;
            dest.x = x * tiles->w + (BORDER_WIDTH * scale) + (scale * 5);
            dest.y = y * (tiles->h / N_TILES) + (BORDER_HEIGHT * scale) + ((swaprow + 2) * scale) - i;
            dest.w = tiles->w - (scale * 5);
            dest.h = 1;

            SDL_BlitSurface(tiles, &src, screen, &dest);
        }
    }

    /*
     * finally draw the focus rectangle if the tile has the focus
     */
    if (focus && (screenCurrentCycle % 2)) {
        /* left edge */
        dest.x = x * tiles->w + (BORDER_WIDTH * scale);
        dest.y = y * (tiles->h / N_TILES) + (BORDER_HEIGHT * scale);
        dest.w = 2 * scale;
        dest.h = tiles->h / N_TILES;
        SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 0xff, 0xff, 0xff));

        /* top edge */
        dest.x = x * tiles->w + (BORDER_WIDTH * scale);
        dest.y = y * (tiles->h / N_TILES) + (BORDER_HEIGHT * scale);
        dest.w = tiles->w;
        dest.h = 2 * scale;
        SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 0xff, 0xff, 0xff));

        /* right edge */
        dest.x = (x + 1) * tiles->w + (BORDER_WIDTH * scale) - (2 * scale);
        dest.y = y * (tiles->h / N_TILES) + (BORDER_HEIGHT * scale);
        dest.w = 2 * scale;
        dest.h = tiles->h / N_TILES;
        SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 0xff, 0xff, 0xff));

        /* bottom edge */
        dest.x = x * tiles->w + (BORDER_WIDTH * scale);
        dest.y = (y + 1) * (tiles->h / N_TILES) + (BORDER_HEIGHT * scale) - (2 * scale);
        dest.w = tiles->w;
        dest.h = 2 * scale;
        SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 0xff, 0xff, 0xff));
    }
}

/**
 * Draw a tile graphic on the screen.
 */
void screenShowGemTile(unsigned char tile, int focus, int x, int y) {
    /* FIXME: show gem tile rather than some random color rectangle */
    SDL_Rect src, dest;

    src.x = 0;
    src.y = tile * (tiles->h / N_TILES);
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
 * Invert an area of the screen.
 */
void screenInvertRect(int x, int y, int w, int h) {
    SDL_Rect src;
    SDL_Surface *tmp;
    Uint32 rmask, gmask, bmask, amask;
    SDL_Color c;
    int i, j;

    src.x = x * scale;
    src.y = y * scale;
    src.w = w * scale;
    src.h = h * scale;

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

    tmp = SDL_CreateRGBSurface(SDL_SWSURFACE, src.w, src.h, 32, rmask, gmask, bmask, amask);
    if (!tmp)
        return;

    SDL_BlitSurface(screen, &src, tmp, NULL);

    for (i = 0; i < src.h; i++) {
        for (j = 0; j < src.w; j++) {
            SDL_GetRGB(getPixel(tmp, j, i), tmp->format, &c.r, &c.g, &c.b);
            putPixel(tmp, j, i, SDL_MapRGB(tmp->format, (Uint8)(0xff - c.r), (Uint8)(0xff - c.g), (Uint8)(0xff - c.b)));
        }
    }

    SDL_BlitSurface(tmp, NULL, screen, &src);
    SDL_FreeSurface(tmp);
}

/**
 * Do the tremor spell effect where the screen shakes.
 */
void screenShake(int iterations) {
    SDL_Rect dest;
    int i;

    dest.x = 0 * scale;
    dest.w = 320 * scale;
    dest.h = 200 * scale;

    for (i = 0; i < iterations; i++) {
        dest.y = 1 * scale;

        SDL_BlitSurface(screen, NULL, screen, &dest);
        SDL_UpdateRect(screen, 0, 0, 0, 0);
        eventHandlerSleep(100);

        dest.y = -1 * scale;

        SDL_BlitSurface(screen, NULL, screen, &dest);
        SDL_UpdateRect(screen, 0, 0, 0, 0);
        eventHandlerSleep(100);
    }
    /* FIXME: remove next line? doesn't seem necessary,
       just adds another screen refresh (which is visible on my screen)... */
    //screenDrawBackground(BKGD_BORDERS);
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

/**
 * Animates the moongate in the intro.  The tree intro image has two
 * overlays in the part of the image normally covered by the text.  If
 * the frame parameter is zero, the first overlay is painted over the
 * image: a moongate.  If frame is one, the second overlay is painted:
 * the circle without the moongate, but with a small white dot
 * representing the anhk and history book.
 */
void screenAnimateIntro(int frame) {
    SDL_Rect src, dest;

    ASSERT(frame == 0 || frame == 1, "invalid frame: %d", frame);

    if (frame == 0) {
        src.x = 0 * scale;
        src.y = 152 * scale;
    } else {
        src.x = 24 * scale;
        src.y = 152 * scale;
    }

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

    ASSERT(pos == 0 || pos == 1, "invalid pos: %d", pos);
    ASSERT(card < 8, "invalid card: %d", card);

    if (bkgds[card / 2 + BKGD_HONCOM] == NULL)
        screenLoadBackground(card / 2 + BKGD_HONCOM);

    src.x = ((card % 2) ? 218 : 12) * scale;
    src.y = 12 * scale;
    src.w = 90 * scale;
    src.h = 124 * scale;

    dest.x = (pos ? 218 : 12) * scale;
    dest.y = 12 * scale;
    dest.w = 90 * scale;
    dest.h = 124 * scale;

    SDL_BlitSurface(bkgds[card / 2 + BKGD_HONCOM], &src, screen, &dest);
}

/**
 * Animates the "beasties" in the intro.  The animate intro image is
 * made up frames for the two creatures in the top left and top right
 * corners of the screen.  This function draws the frame for the given
 * beastie on the screen.  vertoffset is used lower the creatures down
 * from the top of the screen.
 */
void screenShowBeastie(int beast, int vertoffset, int frame) {
    SDL_Rect src, dest;
    int col, row, destx;

    ASSERT(beast == 0 || beast == 1, "invalid beast: %d", beast);

    if (bkgds[BKGD_ANIMATE] == NULL)
        screenLoadBackground(BKGD_ANIMATE);

    row = frame % 6;
    col = frame / 6;

    if (beast == 0) {
        src.x = col * 56 * scale;
        src.w = 55 * scale;
    }
    else {
        src.x = (176 + col * 48) * scale;
        src.w = 48 * scale;
    }

    src.y = row * 32 * scale;
    src.h = 32 * scale;

    destx = beast ? (320 - 48) : 0;

    dest.x = destx * scale;
    dest.y = vertoffset * scale;
    dest.w = src.w;
    dest.h = src.h;

    SDL_BlitSurface(bkgds[BKGD_ANIMATE], &src, screen, &dest);
}

void screenGemUpdate() {
    unsigned char tile;
    int focus, x, y;

    screenFillRect(screen, BORDER_WIDTH, BORDER_HEIGHT, VIEWPORT_W * TILE_WIDTH, VIEWPORT_H * TILE_HEIGHT, 0, 0, 0);

    for (x = 0; x < GEMAREA_W; x++) {
        for (y = 0; y < GEMAREA_H; y++) {
            tile = screenViewportTile(GEMAREA_W, GEMAREA_H, x, y, &focus);
            screenShowGemTile(tile, focus, x, y);
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

    while (filter && filterScaler && (scale % 2 == 0)) {
        dest = (*filterScaler)(src, 2, n);
        scale /= 2;
        SDL_FreeSurface(src);
        src = dest;
    }
    if (scale == 3 && filterScaler == &screenScaleScale2x) {
        dest = (*filterScaler)(src, 3, n);
        scale /= 3;
        SDL_FreeSurface(src);
        src = dest;
    }

    if (scale != 1) {
        dest = screenScalePoint(src, scale, n);
        SDL_FreeSurface(src);
    }

    return dest;
}

/**
 * A simple row and column duplicating scaler.
 */
SDL_Surface *screenScalePoint(SDL_Surface *src, int scale, int n) {
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
                    putPixel(dest, x * scale + j, y * scale + i, getPixel(src, x, y));
            }
        }
    }

    return dest;
}

/**
 * A scaler that interpolates each intervening pixel from it's two
 * neighbors.
 */
SDL_Surface *screenScale2xBilinear(SDL_Surface *src, int scale, int n) {
    int i, x, y, xoff, yoff;
    SDL_Color a, b, c, d;
    SDL_Surface *dest;
    Uint32 rmask, gmask, bmask, amask;

    /* this scaler works only with images scaled by 2x */
    ASSERT(scale == 2, "invalid scale: %d", scale);

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
                putPixel(dest, x * 2 + 1, y * 2, SDL_MapRGB(dest->format, (Uint8)((a.r + b.r) >> 1), (Uint8)((a.g + b.g) >> 1), (Uint8)((a.b + b.b) >> 1)));
                putPixel(dest, x * 2, y * 2 + 1, SDL_MapRGB(dest->format, (Uint8)((a.r + c.r) >> 1), (Uint8)((a.g + c.g) >> 1), (Uint8)((a.b + c.b) >> 1)));
                putPixel(dest, x * 2 + 1, y * 2 + 1, SDL_MapRGB(dest->format, (Uint8)((a.r + b.r + c.r + d.r) >> 2), (Uint8)((a.g + b.g + c.g + d.g) >> 2), (Uint8)((a.b + b.b + c.b + d.b) >> 2)));
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

/**
 * A more sophisticated scaler that interpolates each new pixel the
 * surrounding pixels.
 */
SDL_Surface *screenScale2xSaI(SDL_Surface *src, int scale, int N) {
    int ii, x, y, xoff0, xoff1, xoff2, yoff0, yoff1, yoff2;
    SDL_Color a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p;
    Uint32 prod0, prod1, prod2;
    SDL_Surface *dest;
    Uint32 rmask, gmask, bmask, amask;

    /* this scaler works only with images scaled by 2x */
    ASSERT(scale == 2, "invalid scale: %d", scale);

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
                        prod0 = SDL_MapRGB(dest->format, (Uint8)((a.r + b.r) >> 1), (Uint8)((a.g + b.g) >> 1), (Uint8)((a.b + b.b) >> 1));
                    if ((colorEqual(a, g) && colorEqual(c, o)) ||
                        (colorEqual(a, b) && colorEqual(a, h) && !colorEqual(g, c) && colorEqual(c, m)))
                        prod1 = SDL_MapRGB(dest->format, a.r, a.g, a.b);
                    else
                        prod1 = SDL_MapRGB(dest->format, (Uint8)((a.r + c.r) >> 1), (Uint8)((a.g + c.g) >> 1), (Uint8)((a.b + c.b) >> 1));
                    
                    prod2 = SDL_MapRGB(dest->format, a.r, a.g, a.b);
                }
                else if (colorEqual(b, c) && !colorEqual(a, d)) {
                    if ((colorEqual(b, f) && colorEqual(a, h)) ||
                        (colorEqual(b, e) && colorEqual(b, d) && !colorEqual(a, f) && colorEqual(a, i)))
                        prod0 = SDL_MapRGB(dest->format, b.r, b.g, b.b);
                    else
                        prod0 = SDL_MapRGB(dest->format, (Uint8)((a.r + b.r) >> 1), (Uint8)((a.g + b.g) >> 1), (Uint8)((a.b + b.b) >> 1));
                    if ((colorEqual(c, h) && colorEqual(a, f)) ||
                        (colorEqual(c, g) && colorEqual(c, d) && !colorEqual(a, h) && colorEqual(a, i)))
                        prod1 = SDL_MapRGB(dest->format, c.r, c.g, c.b);
                    else
                        prod1 = SDL_MapRGB(dest->format, (Uint8)((a.r + c.r) >> 1), (Uint8)((a.g + c.g) >> 1), (Uint8)((a.b + c.b) >> 1));
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
                        prod0 = SDL_MapRGB(dest->format, (Uint8)((a.r + b.r) >> 1), (Uint8)((a.g + b.g) >> 1), (Uint8)((a.b + b.b) >> 1));
                        prod1 = SDL_MapRGB(dest->format, (Uint8)((a.r + c.r) >> 1), (Uint8)((a.g + c.g) >> 1), (Uint8)((a.b + c.b) >> 1));

                        r += _2xSaI_GetResult1(a, b, g, e);
                        r += _2xSaI_GetResult2(b, a, k, f);
                        r += _2xSaI_GetResult2(b, a, h, n);
                        r += _2xSaI_GetResult1(a, b, l, o);

                        if (r > 0)
                            prod2 = SDL_MapRGB(dest->format, a.r, a.g, a.b);
                        else if (r < 0)
                            prod2 = SDL_MapRGB(dest->format, b.r, b.g, b.b);
                        else
                            prod2 = SDL_MapRGB(dest->format, (Uint8)((a.r + b.r + c.r + d.r) >> 2), (Uint8)((a.g + b.g + c.g + d.g) >> 2), (Uint8)((a.b + b.b + c.b + d.b) >> 2));
                    }
                }
                else {
                    if (colorEqual(a, c) && colorEqual(a, f) && !colorEqual(b, e) && colorEqual(b, j))
                        prod0 = SDL_MapRGB(dest->format, a.r, a.g, a.b);
                    else if (colorEqual(b, e) && colorEqual(b, d) && !colorEqual(a, f) && colorEqual(a, i))
                        prod0 = SDL_MapRGB(dest->format, b.r, b.g, b.b);
                    else
                        prod0 = SDL_MapRGB(dest->format, (Uint8)((a.r + b.r) >> 1), (Uint8)((a.g + b.g) >> 1), (Uint8)((a.b + b.b) >> 1));

                    if (colorEqual(a, b) && colorEqual(a, h) && !colorEqual(g, c) && colorEqual(c, m))
                        prod1 = SDL_MapRGB(dest->format, a.r, a.g, a.b);
                    else if (colorEqual(c, g) && colorEqual(c, d) && !colorEqual(a, h) && colorEqual(a, i))
                        prod1 = SDL_MapRGB(dest->format, c.r, c.g, c.b);
                    else
                        prod1 = SDL_MapRGB(dest->format, (Uint8)((a.r + c.r) >> 1), (Uint8)((a.g + c.g) >> 1), (Uint8)((a.b + c.b) >> 1));

                    prod2 = SDL_MapRGB(dest->format, (Uint8)((a.r + b.r + c.r + d.r) >> 2), (Uint8)((a.g + b.g + c.g + d.g) >> 2), (Uint8)((a.b + b.b + c.b + d.b) >> 2));
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

/**
 * A more sophisticated scaler that doesn't interpolate, but avoids
 * the stair step effect by detecting angles.
 */
SDL_Surface *screenScaleScale2x(SDL_Surface *src, int scale, int n) {
    int ii, x, y, xoff0, xoff1, yoff0, yoff1;
    SDL_Color a, b, c, d, e, f, g, h, i;
    SDL_Color e0, e1, e2, e3;
    SDL_Surface *dest;
    Uint32 rmask, gmask, bmask, amask;

    /* this scaler works only with images scaled by 2x or 3x */
    ASSERT(scale == 2 || scale == 3, "invalid scale: %d", scale);

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
     * Each pixel in the source image is translated into four (or
     * nine) in the destination.  The destination pixels are dependant
     * on the pixel itself, and the eight surrounding pixels (E is the
     * original pixel):
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
                
                if (scale == 2) {
                    putPixel(dest, x * 2, y * 2, SDL_MapRGB(dest->format, e0.r, e0.g, e0.b));
                    putPixel(dest, x * 2 + 1, y * 2, SDL_MapRGB(dest->format, e1.r, e1.g, e1.b));
                    putPixel(dest, x * 2, y * 2 + 1, SDL_MapRGB(dest->format, e2.r, e2.g, e2.b));
                    putPixel(dest, x * 2 + 1, y * 2 + 1, SDL_MapRGB(dest->format, e3.r, e3.g, e3.b));
                } else if (scale == 3) {
                    putPixel(dest, x * 3, y * 3, SDL_MapRGB(dest->format, e0.r, e0.g, e0.b));
                    putPixel(dest, x * 3 + 1, y * 3, SDL_MapRGB(dest->format, e1.r, e1.g, e1.b));
                    putPixel(dest, x * 3 + 2, y * 3, SDL_MapRGB(dest->format, e1.r, e1.g, e1.b));
                    putPixel(dest, x * 3, y * 3 + 1, SDL_MapRGB(dest->format, e0.r, e0.g, e0.b));
                    putPixel(dest, x * 3 + 1, y * 3 + 1, SDL_MapRGB(dest->format, e1.r, e1.g, e1.b));
                    putPixel(dest, x * 3 + 2, y * 3 + 1, SDL_MapRGB(dest->format, e1.r, e1.g, e1.b));
                    putPixel(dest, x * 3, y * 3 + 2, SDL_MapRGB(dest->format, e2.r, e2.g, e2.b));
                    putPixel(dest, x * 3 + 1, y * 3 + 2, SDL_MapRGB(dest->format, e3.r, e3.g, e3.b));
                    putPixel(dest, x * 3 + 2, y * 3 + 2, SDL_MapRGB(dest->format, e3.r, e3.g, e3.b));
                }
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

