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
#include "image.h"
#include "rle.h"
#include "savegame.h"
#include "settings.h"
#include "scale.h"
#include "screen.h"
#include "ttype.h"
#include "dngview.h"
#include "u4.h"
#include "u4file.h"
#include "xml.h"

typedef enum {
    COMP_NONE,
    COMP_RLE,
    COMP_LZW
} CompressionType;

long decompress_u4_file(FILE *in, long filesize, void **out);
long decompress_u4_memory(void *in, long inlen, void **out);

void screenFillRect(SDL_Surface *surface, int x, int y, int w, int h, int r, int g, int b);
void screenCopyRect(SDL_Surface *surface, int srcX, int srcY, int destX, int destY, int w, int h);
int screenLoadBackground(BackgroundType bkgd);
void screenFreeIntroBackground();
int screenLoadTiles();
int screenLoadCharSet();
int screenLoadPaletteEga();
int screenLoadPaletteVga(const char *filename);
int screenLoadImageEga(Image **image, int width, int height, U4FILE *file, CompressionType comp);
int screenLoadImageVga(Image **image, int width, int height, U4FILE *file, CompressionType comp);
Image *screenScale(Image *src, int scale, int n, int filter);

SDL_Surface *screen;
Image *bkgds[BKGD_MAX];
Image *dngGraphic[36];
Image *tiles, *charset;
SDL_Color egaPalette[16];
SDL_Color vgaPalette[256];
int scale, forceEga, forceVga;
Scaler filterScaler;

const struct {
    const char *filename, *filenameOld;
    int width, height;
    int hasVga;
    CompressionType comp;
    int filter;
    int introAnim;
    int scaleOnLoad;
} backgroundInfo[] = {
    /* main game borders */
    { "start.ega",    "start.old", 320, 200, 1, COMP_RLE, 1, 0, 1 },

    /* introduction screen images */
    { "title.ega",    NULL, 320, 200, 0, COMP_LZW, 1, 1, 0 },
    { "title.ega",    NULL, 320, 200, 0, COMP_LZW, 1, 1, 0 },
    { "tree.ega",     NULL, 320, 200, 0, COMP_LZW, 1, 1, 1 },
    { "portal.ega",   NULL, 320, 200, 0, COMP_LZW, 1, 1, 1 },
    { "outside.ega",  NULL, 320, 200, 0, COMP_LZW, 1, 1, 1 },
    { "inside.ega",   NULL, 320, 200, 0, COMP_LZW, 1, 1, 1 },
    { "wagon.ega",    NULL, 320, 200, 0, COMP_LZW, 1, 1, 1 },
    { "gypsy.ega",    NULL, 320, 200, 0, COMP_LZW, 1, 1, 1 },
    { "abacus.ega",   NULL, 320, 200, 0, COMP_LZW, 1, 1, 1 },
    { "honcom.ega",   NULL, 320, 200, 0, COMP_LZW, 1, 1, 1 },
    { "valjus.ega",   NULL, 320, 200, 0, COMP_LZW, 1, 1, 1 },
    { "sachonor.ega", NULL, 320, 200, 0, COMP_LZW, 1, 1, 1 },
    { "spirhum.ega",  NULL, 320, 200, 0, COMP_LZW, 1, 1, 1 },
    { "animate.ega",  NULL, 320, 200, 0, COMP_LZW, 1, 1, 1 },

    /* abyss vision images */
    { "compassn.ega", "compassn.old", 320, 200, 1, COMP_RLE, 1, 0, 1 },
    { "courage.ega",  "courage.old",  320, 200, 1, COMP_RLE, 1, 0, 1 },
    { "honesty.ega",  "honesty.old",  320, 200, 1, COMP_RLE, 1, 0, 1 },
    { "honor.ega",    "honor.old",    320, 200, 1, COMP_RLE, 1, 0, 1 },
    { "humility.ega", "humility.old", 320, 200, 1, COMP_RLE, 1, 0, 1 },
    { "justice.ega",  "justice.old",  320, 200, 1, COMP_RLE, 1, 0, 1 },
    { "love.ega",     "love.old",     320, 200, 1, COMP_RLE, 1, 0, 1 },
    { "sacrific.ega", "sacrific.old", 320, 200, 1, COMP_RLE, 1, 0, 1 },
    { "spirit.ega",   "spirit.old",   320, 200, 1, COMP_RLE, 1, 0, 1 },
    { "truth.ega",    "truth.old",    320, 200, 1, COMP_RLE, 1, 0, 1 },
    { "valor.ega",    "valor.old",    320, 200, 1, COMP_RLE, 1, 0, 1 },

    /* shrine vision images */
    { "rune_0.ega",   "rune_0.old", 320, 200, 1, COMP_RLE, 1, 0, 1 },
    { "rune_1.ega",   "rune_1.old", 320, 200, 1, COMP_RLE, 1, 0, 1 },
    { "rune_2.ega",   "rune_2.old", 320, 200, 1, COMP_RLE, 1, 0, 1 },
    { "rune_3.ega",   "rune_3.old", 320, 200, 1, COMP_RLE, 1, 0, 1 },
    { "rune_4.ega",   "rune_4.old", 320, 200, 1, COMP_RLE, 1, 0, 1 },
    { "rune_5.ega",   "rune_5.old", 320, 200, 1, COMP_RLE, 1, 0, 1 },
    { "rune_6.ega",   "rune_6.old", 320, 200, 1, COMP_RLE, 1, 0, 1 },
    { "rune_7.ega",   "rune_7.old", 320, 200, 1, COMP_RLE, 1, 0, 1 },
    { "rune_8.ega",   "rune_8.old", 320, 200, 1, COMP_RLE, 1, 0, 1 }
};

const struct {
    const char *filename;
    int width, height;
    int depth;
    int x, y;
    CompressionType comp;
} dngGraphicInfo[] = {
    { "dung0l.rle", 32,  176, 4, 0,   0,   COMP_RLE },
    { "dung0m.rle", 176, 176, 4, 0,   0,   COMP_RLE },
    { "dung0r.rle", 32,  176, 4, 144, 0,   COMP_RLE },

    { "dung1l.rle", 64,  112, 4, 0,   32,  COMP_RLE },
    { "dung1m.rle", 176, 112, 4, 0,   32,  COMP_RLE },
    { "dung1r.rle", 64,  112, 4, 112, 32,  COMP_RLE },

    { "dung2l.rle", 80,  48,  4, 0,   64,  COMP_RLE },
    { "dung2m.rle", 176, 48,  4, 0,   64,  COMP_RLE },
    { "dung2r.rle", 80,  48,  4, 96,  64,  COMP_RLE },

    { "dung3l.rle", 88,  16,  4, 0,   80,  COMP_RLE },
    { "dung3m.rle", 176, 16,  4, 0,   80,  COMP_RLE },
    { "dung3r.rle", 88,  16,  4, 88,  80,  COMP_RLE },

    { "dung0l_door.rle", 32,  176, 4, 0,   0,   COMP_RLE },
    { NULL },
    { "dung0r_door.rle", 32,  176, 4, 144, 0,   COMP_RLE },

    { "dung1l_door.rle", 64,  112, 4, 0,   32,  COMP_RLE },
    { "dung1m_door.rle", 176, 112, 4, 0,   32,  COMP_RLE },
    { "dung1r_door.rle", 64,  112, 4, 112, 32,  COMP_RLE },

    { "dung2l_door.rle", 80,  48,  4, 0,   64,  COMP_RLE },
    { "dung2m_door.rle", 176, 48,  4, 0,   64,  COMP_RLE },
    { "dung2r_door.rle", 80,  48,  4, 96,  64,  COMP_RLE },

    { "dung3l_door.rle", 88,  16,  4, 0,   80,  COMP_RLE },
    { "dung3m_door.rle", 176, 16,  4, 0,   80,  COMP_RLE },
    { "dung3r_door.rle", 88,  16,  4, 88,  80,  COMP_RLE },

    { "ladderup0.rle",   88,  87,  4, 45,  0,   COMP_RLE },
    { "ladderup1.rle",   50,  48,  4, 64,  40,  COMP_RLE },
    { "ladderup2.rle",   22,  19,  4, 77,  68,  COMP_RLE },
    { "ladderup3.rle",   8,   6,   4, 84,  82,  COMP_RLE },

    { "ladderdown0.rle", 88,  89,  4, 45,  87,  COMP_RLE }

};

extern int verbose;

void screenInit() {

    /* set up scaling parameters */
    scale = settings->scale;
    filterScaler = scalerGet(settings->filter);
    if (verbose)
        printf("using %s scaler\n", settingsFilterToString(settings->filter));

    if (scale < 1 || scale > 5)
        scale = 2;

    forceEga = settings->videoType == VIDEO_EGA;
    forceVga = settings->videoType == VIDEO_VGA;    

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
    if (!screenLoadPaletteVga("u4vga.pal")) {
        /* must use Ega graphics */
        forceEga = 1;
        forceVga = 0;
    }

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
        imageDelete(bkgds[BKGD_BORDERS]);
    imageDelete(tiles);
    imageDelete(charset);
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
    SDL_BlitSurface(bkgds[bkgd]->surface, &src, bkgds[bkgd]->surface, &dest);    

    /* ----------------------------
     * erase the original "present"
     * ---------------------------- */

    dest.x = 136; dest.y = 0; dest.w = 55; dest.h = 5;
    imageFillRect(bkgds[bkgd], 136, 0, 55, 5, 0, 0, 0);

    /* -----------------------------
     * draw "Lord British" signature
     * ----------------------------- */
    i = 0;
    while (sigData[i] != 0) {
        /*  (x/y) are unscaled coordinates, i.e. in 320x200  */
        x = sigData[i] + 0x14;
        y = 0xBF - sigData[i+1]; 
        imagePutPixel(bkgds[bkgd], x, y, 0, 255, 255); /* cyan */
        imagePutPixel(bkgds[bkgd], x+1, y, 0, 255, 255); /* cyan */
        i += 2;
    }

    /* --------------------------------------------------------------
     * draw the red line between "Origin Systems, Inc." and "present"
     * -------------------------------------------------------------- */
    /* we're still working with an unscaled surface */
    for (i = 86; i < 239; i++)
        imagePutPixel(bkgds[bkgd], i, 31, 128, 0, 0); /* red */    

    /* scale and filter images now that we've completed them */
    bkgds[bkgd] = screenScale(bkgds[bkgd], scale, 1, backgroundInfo[bkgd].filter);
}

void screenFixIntroScreenExtended(BackgroundType bkgd) {
    screenCopyRect(bkgds[bkgd]->surface, 0, 95, 0, 10, 320, 50);
    screenCopyRect(bkgds[bkgd]->surface, 0, 105, 0, 60, 320, 45);
}

/**
 * Load in a background image from a ".ega" file.
 */
int screenLoadBackground(BackgroundType bkgd) {
    int ret;
    Image *unscaled;
    U4FILE *file;

    //const char *vgaFilename = usingZipFiles ? backgroundInfo[bkgd].filenameOld : backgroundInfo[bkgd].filename;
    //const char *egaFilename = usingZipFiles ? backgroundInfo[bkgd].filename : backgroundInfo[bkgd].filenameOld;
    const char *vgaFilename = backgroundInfo[bkgd].filename;
    const char *egaFilename = backgroundInfo[bkgd].filename;

    ret = 0;
    if (!forceEga && backgroundInfo[bkgd].hasVga) {
        file = u4fopen(vgaFilename);
    
        if (file) {
            ret = screenLoadImageVga(&unscaled,
                                     backgroundInfo[bkgd].width,
                                     backgroundInfo[bkgd].height,
                                     file,
                                     backgroundInfo[bkgd].comp);
            u4fclose(file);
        }
    }
    if (!ret && (!forceVga || !backgroundInfo[bkgd].hasVga)) {
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

        /* open the correct file for what we're trying to do */
        file = egaFilename ?
            u4fopen(egaFilename) : /* is there a different file for ega stuff? */
            u4fopen(vgaFilename);  /* no? then use the normal file */

        if (file) {
            ret = screenLoadImageEga(&unscaled,
                                     backgroundInfo[bkgd].width,
                                     backgroundInfo[bkgd].height,
                                     file,
                                     backgroundInfo[egaBkgd].comp);
            u4fclose(file);
        }
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
        imageDelete(bkgds[i]);
        bkgds[i] = NULL;
    }
}

/**
 * Load the tiles graphics from the "shapes.ega" or "shapes.vga" file.
 */
int screenLoadTiles() {
    U4FILE *file;
    int ret = 0;

    if (!forceEga) {
        file = u4fopen("shapes.vga");
        if (file) {
            ret = screenLoadImageVga(&tiles, TILE_WIDTH, TILE_HEIGHT * N_TILES, file, COMP_NONE);
            u4fclose(file);
        }
    }

    if (!ret && !forceVga) {
        file = u4fopen("shapes.ega");
        if (file) {
            ret = screenLoadImageEga(&tiles, TILE_WIDTH, TILE_HEIGHT * N_TILES, file, COMP_NONE);
            u4fclose(file);
        }
    }

    tiles = screenScale(tiles, scale, N_TILES, 1);

    return ret;
}

/**
 * Load the character set graphics from the "charset.ega" or "charset.vga" file.
 */
int screenLoadCharSet() {
    U4FILE *file;
    int ret = 0;


    if (!forceEga) {
        file = u4fopen("charset.vga");
        if (file) {
            ret = screenLoadImageVga(&charset, CHAR_WIDTH, CHAR_HEIGHT * N_CHARS, file, COMP_NONE);
            u4fclose(file);
        }
    }

    if (!ret && !forceVga) {
        file = u4fopen("charset.ega");
        if (file) {
            ret = screenLoadImageEga(&charset, CHAR_WIDTH, CHAR_HEIGHT * N_CHARS, file, COMP_NONE);
            u4fclose(file);
        }
    }

    charset = screenScale(charset, scale, N_CHARS, 1);

    return ret;
}

/**
 * Loads the basic EGA palette from egaPalette.xml
 */
int screenLoadPaletteEga() {
    xmlDocPtr doc;
    xmlNodePtr root, node;    
    int i = 0;

    doc = xmlParse("egaPalette.xml");
    root = xmlDocGetRootElement(doc);
    if (xmlStrcmp(root->name, (const xmlChar *) "egaPalette") != 0)
        errorFatal("malformed egaPalette.xml");
    
    for (node = root->xmlChildrenNode; node; node = node->next) {
        if (xmlNodeIsText(node) ||
            xmlStrcmp(node->name, (const xmlChar *) "color") != 0)
            continue;
        
        egaPalette[i].r = xmlGetPropAsInt(node, (const xmlChar *)"r");
        egaPalette[i].g = xmlGetPropAsInt(node, (const xmlChar *)"g");
        egaPalette[i].b = xmlGetPropAsInt(node, (const xmlChar *)"b");

        i++;
    }

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
int screenLoadImageEga(Image **image, int width, int height, U4FILE *file, CompressionType comp) {
    Image *img;
    int x, y;
    unsigned char *compressed_data, *decompressed_data = NULL;
    long inlen, decompResult;

    inlen = u4flength(file);
    compressed_data = (Uint8 *) malloc(inlen);
    u4fread(compressed_data, 1, inlen, file);

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

    img = imageNew(width, height, 1, 1, IMTYPE_HW);
    if (!img) {
        if (decompressed_data)
            free(decompressed_data);
        return 0;
    }

    SDL_SetColors(img->surface, egaPalette, 0, 16);

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x+=2) {
            imagePutPixelIndex(img, x, y, decompressed_data[(y * width + x) / 2] >> 4);
            imagePutPixelIndex(img, x + 1, y, decompressed_data[(y * width + x) / 2] & 0x0f);
        }
    }
    free(decompressed_data);

    (*image) = img;

    return 1;
}

/**
 * Load an image from a ".vga" or ".ega" image file from the U4 VGA upgrade.
 */
int screenLoadImageVga(Image **image, int width, int height, U4FILE *file, CompressionType comp) {
    Image *img;
    int x, y;
    unsigned char *compressed_data, *decompressed_data = NULL;
    long inlen, decompResult;

    inlen = u4flength(file);
    compressed_data = (Uint8 *) malloc(inlen);
    u4fread(compressed_data, 1, inlen, file);

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

    img = imageNew(width, height, 1, 1, IMTYPE_HW);
    if (!img) {
        if (decompressed_data)
            free(decompressed_data);
        return 0;
    }

    SDL_SetColors(img->surface, vgaPalette, 0, 256);

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++)
            imagePutPixelIndex(img, x, y, decompressed_data[y * width + x]);
    }
    free(decompressed_data);

    (*image) = img;

    return 1;
}

/**
 * Draw the surrounding borders on the screen.
 */
void screenDrawBackground(BackgroundType bkgd) {
    ASSERT(bkgd < BKGD_MAX, "bkgd out of range: %d", bkgd);

    if (bkgds[bkgd] == NULL) {
        if (!screenLoadBackground(bkgd))
            errorFatal("unable to load data files: is Ultima IV installed?  See http://xu4.sourceforge.net/");
    }

    imageDraw(bkgds[bkgd], 0, 0);
}

void screenDrawBackgroundInMapArea(BackgroundType bkgd) {
    ASSERT(bkgd < BKGD_MAX, "bkgd out of range: %d", bkgd);

    if (bkgds[bkgd] == NULL) {
        if (!screenLoadBackground(bkgd))
            errorFatal("unable to load data files: is Ultima IV installed?  See http://xu4.sourceforge.net/");
    }

    imageDrawSubRect(bkgds[bkgd], BORDER_WIDTH * scale, BORDER_HEIGHT * scale,
                     BORDER_WIDTH * scale, BORDER_HEIGHT * scale,
                     VIEWPORT_W * TILE_WIDTH * scale, 
                     VIEWPORT_H * TILE_HEIGHT * scale);
}

/**
 * Draw a character from the charset onto the screen.
 */
void screenShowChar(int chr, int x, int y) {
    imageDrawSubRect(charset, x * charset->w, y * (charset->h / N_CHARS),
                     0, chr * (charset->h / N_CHARS),
                     charset->w, charset->h / N_CHARS);
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
    int offset, i, j, swaprow;
    SDL_Rect src, dest;
    int unscaled_x, unscaled_y;

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

    unscaled_x = x * (tiles->w / scale) + BORDER_WIDTH;
    unscaled_y = y * ((tiles->h / scale) / N_TILES) + BORDER_HEIGHT;

    /* FIXME: sucks */
    if (tileGetAnimationStyle(tile) == ANIM_CAMPFIRE) {
        for (i = 0; i < tiles->w; i++) {
            for (j = (tile * (tiles->h / N_TILES)); j < (tile + 1) * (tiles->h / N_TILES); j++) {
                int r, g, b;
                imageGetPixel(tiles, i, j, &r, &g, &b);
                if ((r > 128) && (g + b + 256 < (r << 1)))
                    imagePutPixel(tiles, i, j, (rand()%64) + 192, (rand()%64)+48, rand()%32);
            }
            //imagePutPixel(tiles, rand()%tiles->w, (tile * (tiles->h / N_TILES)) + (rand()%(tiles->h / N_TILES)), (rand()%128) + 128, rand()%64, rand()%64);
            //imagePutPixelScaled(tiles, rand()%16, ((tile * (tiles->h / N_TILES)) / scale) + rand()%16, rand()%255, rand()%64, rand()%32);  
        }            
    }

    SDL_BlitSurface(tiles->surface, &src, screen, &dest);    

    if (offset != 0) {

        src.x = 0;
        src.y = (tile + 1) * (tiles->h / N_TILES) - offset;
        src.w = tiles->w;
        src.h = offset;
        dest.x = x * tiles->w + (BORDER_WIDTH * scale);
        dest.y = y * (tiles->h / N_TILES) + (BORDER_HEIGHT * scale);
        dest.w = tiles->w;
        dest.h = tiles->h / N_TILES;

        SDL_BlitSurface(tiles->surface, &src, screen, &dest);
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

            SDL_BlitSurface(tiles->surface, &src, screen, &dest);            
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

    SDL_BlitSurface(tiles->surface, &src, screen, &dest);
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
    Image *tmp;
    RGB c;
    int i, j;

    src.x = x * scale;
    src.y = y * scale;
    src.w = w * scale;
    src.h = h * scale;

    tmp = imageNew(src.w, src.h, 1, 0, IMTYPE_SW);
    if (!tmp)
        return;

    SDL_BlitSurface(screen, &src, tmp->surface, NULL);

    for (i = 0; i < src.h; i++) {
        for (j = 0; j < src.w; j++) {
            imageGetPixel(tmp, j, i, &c.r, &c.g, &c.b);
            imagePutPixel(tmp, j, i, 0xff - c.r, 0xff - c.g, 0xff - c.b);
        }
    }

    SDL_BlitSurface(tmp->surface, NULL, screen, &src);
    imageDelete(tmp);
    SDL_UpdateRect(screen, 0, 0, 0, 0);
}

/**
 * Do the tremor spell effect where the screen shakes.
 */
void screenShake(int iterations) {
    SDL_Rect dest;
    int i;

    if (settings->screenShakes) {
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
}

int screenDungeonGraphicIndex(int xoffset, int distance, DungeonGraphicType type) {
    int index;

    index = 0;

    if (type == DNGGRAPHIC_LADDERUP && xoffset == 0)
        return 24 + distance;

    if (type == DNGGRAPHIC_LADDERDOWN && xoffset == 0 && distance == 0)
        return 28 + distance;

    /* FIXME */
    if (type != DNGGRAPHIC_WALL && type != DNGGRAPHIC_DOOR)
        return -1;

    if (type == DNGGRAPHIC_DOOR && ((xoffset == 0 && distance != 0) || (xoffset != 0)))
        index += 12;

    index += xoffset + 1;

    index += distance * 3;

    return index;
}

int screenDungeonLoadGraphic(int xoffset, int distance, DungeonGraphicType type) {
    char *pathname;
    U4FILE *file;
    int index, ret;
    Image *unscaled;

    ret = 0;
    index = screenDungeonGraphicIndex(xoffset, distance, type);
    ASSERT(index != -1, "invalid graphic paramters provided");

    pathname = u4find_graphics(dngGraphicInfo[index].filename);
    if (!pathname)
        return 0;

    file = u4fopen_stdio(pathname);
    free(pathname);
    if (!file)
        return 0;

    if (dngGraphicInfo[index].depth == 8)
        ret = screenLoadImageVga(&unscaled, 
                                 dngGraphicInfo[index].width, 
                                 dngGraphicInfo[index].height, 
                                 file, 
                                 dngGraphicInfo[index].comp);
    else {
        ret = screenLoadImageEga(&unscaled, 
                                 dngGraphicInfo[index].width, 
                                 dngGraphicInfo[index].height, 
                                 file, 
                                 dngGraphicInfo[index].comp);
    }
    u4fclose(file);

    if (!ret)
        return 0;

    dngGraphic[index] = screenScale(unscaled, scale, 1, 1);
    dngGraphic[index]->surface->format->colorkey = 0;
    dngGraphic[index]->surface->flags |= SDL_SRCCOLORKEY;
    
    return 1;
}

void screenDungeonDrawWall(int xoffset, int distance, DungeonGraphicType type) {
    int index;

    index = screenDungeonGraphicIndex(xoffset, distance, type);
    if (index == -1)
        return;

    if (dngGraphic[index] == NULL) {
        if (!screenDungeonLoadGraphic(xoffset, distance, type))
            errorFatal("unable to load data files: is Ultima IV installed?  See http://xu4.sourceforge.net/");
    }

    imageDraw(dngGraphic[index], (8 + dngGraphicInfo[index].x) * scale, (8 + dngGraphicInfo[index].y) * scale);
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

    SDL_BlitSurface(bkgds[BKGD_TREE]->surface, &src, screen, &dest);
}

void screenEraseMapArea() {
    SDL_Rect dest;

    dest.x = BORDER_WIDTH * scale;
    dest.y = BORDER_WIDTH * scale;
    dest.w = VIEWPORT_W * TILE_WIDTH * scale;
    dest.h = VIEWPORT_H * TILE_HEIGHT * scale;

    SDL_FillRect(screen, &dest, 0);
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

    SDL_BlitSurface(bkgds[card / 2 + BKGD_HONCOM]->surface, &src, screen, &dest);
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

    SDL_BlitSurface(bkgds[BKGD_ANIMATE]->surface, &src, screen, &dest);
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

Image *screenScale(Image *src, int scale, int n, int filter) {
    Image *dest;

    dest = src;

    while (filter && filterScaler && (scale % 2 == 0)) {
        dest = (*filterScaler)(src, 2, n);
        scale /= 2;
        imageDelete(src);
        src = dest;
    }
    if (scale == 3 && scaler3x(settings->filter)) {
        dest = (*filterScaler)(src, 3, n);
        scale /= 3;
        imageDelete(src);
        src = dest;
    }

    if (scale != 1) {
        dest = (*scalerGet(SCL_POINT))(src, scale, n);
        imageDelete(src);
    }

    return dest;
}
