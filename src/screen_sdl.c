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
#include "intro.h"
#include "list.h"
#include "rle.h"
#include "savegame.h"
#include "settings.h"
#include "scale.h"
#include "screen.h"
#include "tileset.h"
#include "dngview.h"
#include "u4.h"
#include "u4_sdl.h"
#include "u4file.h"
#include "utils.h"
#include "xml.h"

long decompress_u4_file(FILE *in, long filesize, void **out);
long decompress_u4_memory(void *in, long inlen, void **out);

void screenFillRect(SDL_Surface *surface, int x, int y, int w, int h, int r, int g, int b);
void fixupIntro(Image *im);
void fixupIntroExtended(Image *im);
void fixupAbyssVision(Image *im);
void screenFreeIntroBackground();
int screenLoadTiles();
int screenLoadGemTiles();
int screenLoadPaletteEga();
int screenLoadPaletteVga(const char *filename);
Image *screenScaleDown(Image *src, int scale);

SDL_Surface *screen;
Image *bkgds[BKGD_MAX];
Image *dngGraphic[56];
Image *tiles, *gemtiles;
SDL_Color egaPalette[16];
SDL_Color vgaPalette[256];
int scale;
Scaler filterScaler;

typedef enum {
    FIXUP_NONE,
    FIXUP_INTRO,
    FIXUP_INTRO_EXTENDED,
    FIXUP_ABYSS
} ImageFixup;

typedef struct {
    int id;
    const char *filename;
    int width, height, depth;
    CompressionType filetype;
    int tiles;
    int introOnly;
    ImageFixup fixup;
} ImageInfo;

typedef struct {
    const char *name;
    const char *location;
    const char *extends;
    ImageInfo *image[BKGD_MAX];
} ImageSet;

const struct {
    const char *filename;
    int width, height;
    int depth;
    int x, y;
    CompressionType comp;
} dngGraphicInfo[] = {
    { "ega/dung0la.rle", 32,  176, 4, 0,   0,   COMP_RLE },
    { "ega/dung0lb.rle", 32,  176, 4, 0,   0,   COMP_RLE },
    { "ega/dung0ma.rle", 176, 176, 4, 0,   0,   COMP_RLE },
    { "ega/dung0mb.rle", 176, 176, 4, 0,   0,   COMP_RLE },
    { "ega/dung0ra.rle", 32,  176, 4, 144, 0,   COMP_RLE },
    { "ega/dung0rb.rle", 32,  176, 4, 144, 0,   COMP_RLE },

    { "ega/dung1la.rle", 64,  112, 4, 0,   32,  COMP_RLE },
    { "ega/dung1lb.rle", 64,  112, 4, 0,   32,  COMP_RLE },
    { "ega/dung1ma.rle", 176, 112, 4, 0,   32,  COMP_RLE },
    { "ega/dung1mb.rle", 176, 112, 4, 0,   32,  COMP_RLE },
    { "ega/dung1ra.rle", 64,  112, 4, 112, 32,  COMP_RLE },
    { "ega/dung1rb.rle", 64,  112, 4, 112, 32,  COMP_RLE },

    { "ega/dung2la.rle", 80,  48,  4, 0,   64,  COMP_RLE },
    { "ega/dung2lb.rle", 80,  48,  4, 0,   64,  COMP_RLE },
    { "ega/dung2ma.rle", 176, 48,  4, 0,   64,  COMP_RLE },
    { "ega/dung2mb.rle", 176, 48,  4, 0,   64,  COMP_RLE },
    { "ega/dung2ra.rle", 80,  48,  4, 96,  64,  COMP_RLE },
    { "ega/dung2rb.rle", 80,  48,  4, 96,  64,  COMP_RLE },

    { "ega/dung3la.rle", 88,  16,  4, 0,   80,  COMP_RLE },
    { "ega/dung3lb.rle", 88,  16,  4, 0,   80,  COMP_RLE },
    { "ega/dung3ma.rle", 176, 16,  4, 0,   80,  COMP_RLE },
    { "ega/dung3mb.rle", 176, 16,  4, 0,   80,  COMP_RLE },
    { "ega/dung3ra.rle", 88,  16,  4, 88,  80,  COMP_RLE },
    { "ega/dung3rb.rle", 88,  16,  4, 88,  80,  COMP_RLE },

    { "ega/dung0la_door.rle", 32,  176, 4, 0,   0,   COMP_RLE },
    { "ega/dung0lb_door.rle", 32,  176, 4, 0,   0,   COMP_RLE },
    { "ega/dung0ma_door.rle", 176, 176, 4, 0,   0,   COMP_RLE },
    { "ega/dung0mb_door.rle", 176, 176, 4, 0,   0,   COMP_RLE },
    { "ega/dung0ra_door.rle", 32,  176, 4, 144, 0,   COMP_RLE },
    { "ega/dung0rb_door.rle", 32,  176, 4, 144, 0,   COMP_RLE },

    { "ega/dung1la_door.rle", 64,  112, 4, 0,   32,  COMP_RLE },
    { "ega/dung1lb_door.rle", 64,  112, 4, 0,   32,  COMP_RLE },
    { "ega/dung1ma_door.rle", 176, 112, 4, 0,   32,  COMP_RLE },
    { "ega/dung1mb_door.rle", 176, 112, 4, 0,   32,  COMP_RLE },
    { "ega/dung1ra_door.rle", 64,  112, 4, 112, 32,  COMP_RLE },
    { "ega/dung1rb_door.rle", 64,  112, 4, 112, 32,  COMP_RLE },

    { "ega/dung2la_door.rle", 80,  48,  4, 0,   64,  COMP_RLE },
    { "ega/dung2lb_door.rle", 80,  48,  4, 0,   64,  COMP_RLE },
    { "ega/dung2ma_door.rle", 176, 48,  4, 0,   64,  COMP_RLE },
    { "ega/dung2mb_door.rle", 176, 48,  4, 0,   64,  COMP_RLE },
    { "ega/dung2ra_door.rle", 80,  48,  4, 96,  64,  COMP_RLE },
    { "ega/dung2rb_door.rle", 80,  48,  4, 96,  64,  COMP_RLE },

    { "ega/dung3la_door.rle", 88,  16,  4, 0,   80,  COMP_RLE },
    { "ega/dung3lb_door.rle", 88,  16,  4, 0,   80,  COMP_RLE },
    { "ega/dung3ma_door.rle", 176, 16,  4, 0,   80,  COMP_RLE },
    { "ega/dung3mb_door.rle", 176, 16,  4, 0,   80,  COMP_RLE },
    { "ega/dung3ra_door.rle", 88,  16,  4, 88,  80,  COMP_RLE },
    { "ega/dung3rb_door.rle", 88,  16,  4, 88,  80,  COMP_RLE },

    { "ega/ladderup0.rle",   88,  87,  4, 45,  0,   COMP_RLE },
    { "ega/ladderup1.rle",   50,  48,  4, 64,  40,  COMP_RLE },
    { "ega/ladderup2.rle",   22,  19,  4, 77,  68,  COMP_RLE },
    { "ega/ladderup3.rle",   8,   6,   4, 84,  82,  COMP_RLE },

    { "ega/ladderdown0.rle", 88,  89,  4, 45,  87,  COMP_RLE },
    { "ega/ladderdown1.rle", 50,  50,  4, 64,  86,  COMP_RLE },
    { "ega/ladderdown2.rle", 22,  22,  4, 77,  86,  COMP_RLE },
    { "ega/ladderdown3.rle", 8,   8,   4, 84,  88,  COMP_RLE }

};

void screenLoadGraphicsFromXml(void);
ImageSet *screenLoadImageSetFromXml(xmlNodePtr node);
ImageInfo *screenLoadImageInfoFromXml(xmlNodePtr node);
ListNode *imageSets = NULL;

extern int verbose;

void screenInit() {

    screenLoadGraphicsFromXml();

    /* set up scaling parameters */
    scale = settings->scale;
    filterScaler = scalerGet(settings->filter);
    if (verbose)
        printf("using %s scaler\n", settingsFilterToString(settings->filter));

    if (scale < 1 || scale > 5)
        scale = 2;
    
    /* start SDL */
    if (u4_SDL_InitSubSystem(SDL_INIT_VIDEO) < 0)
        errorFatal("unable to init SDL: %s", SDL_GetError());    

    SDL_WM_SetCaption("Ultima IV", NULL);
#ifdef ICON_FILE
    SDL_WM_SetIcon(SDL_LoadBMP(ICON_FILE), NULL);
#endif   

    screen = SDL_SetVideoMode(320 * scale, 200 * scale, 16, SDL_SWSURFACE | SDL_ANYFORMAT | (settings->fullscreen ? SDL_FULLSCREEN : 0));
    if (!screen)
        errorFatal("unable to set video: %s", SDL_GetError());

    screenLoadPaletteEga();        
    /* see if the upgrade exists */
    if (screenLoadPaletteVga("u4vga.pal"))
        u4upgradeExists = 1;
    u4upgradeInstalled = u4isUpgradeInstalled();

    /* if we can't use vga, reset to default:ega */
    if (!u4upgradeExists && settings->videoType == VIDEO_VGA)
        settings->videoType = VIDEO_EGA;

    if (!screenLoadTiles() ||
        !screenLoadGemTiles())
        errorFatal("unable to load data files: is Ultima IV installed?  See http://xu4.sourceforge.net/");

    if (verbose)
        printf("screen initialized [screenInit()]\n");

    eventKeyboardSetKeyRepeat(settings->keydelay, settings->keyinterval);
    SDL_ShowCursor(SDL_DISABLE); /* disable the mouse cursor */
}

void screenDelete() {
    screenFreeBackgrounds();    
    u4_SDL_QuitSubSystem(SDL_INIT_VIDEO);

    if (verbose)
        printf("screen deleted [screenDelete()]\n");
}

/**
 * Re-initializes the screen and implements any changes made in settings
 */
void screenReInit() {        
    introDelete(DONT_FREE_MENUS);  /* delete intro stuff */
    tilesetDeleteAllTilesets(); /* unload tilesets */
    screenDelete(); /* delete screen stuff */            
    screenInit();   /* re-init screen stuff (loading new backgrounds, etc.) */
    tilesetLoadAllTilesetsFromXml("tilesets.xml"); /* re-load tilesets */
    introInit();    /* re-fix the backgrounds loaded and scale images, etc. */            
}

void screenLoadGraphicsFromXml() {
    xmlDocPtr doc;
    xmlNodePtr root, node;

    doc = xmlParse("graphics.xml");
    root = xmlDocGetRootElement(doc);
    if (xmlStrcmp(root->name, (const xmlChar *) "graphics") != 0)
        errorFatal("malformed .xml");

    for (node = root->xmlChildrenNode; node; node = node->next) {
        if (xmlNodeIsText(node) ||
            xmlStrcmp(node->name, (const xmlChar *) "imageset") != 0)
            continue;

        imageSets = listAppend(imageSets, screenLoadImageSetFromXml(node));
    }
}

ImageSet *screenLoadImageSetFromXml(xmlNodePtr node) {
    ImageSet *set;
    xmlNodePtr child;

    set = (ImageSet *) malloc(sizeof(ImageSet));
    memset(set, 0, sizeof(ImageSet));
    set->name = xmlGetPropAsStr(node, "name");
    set->location = xmlGetPropAsStr(node, "location");
    set->extends = xmlGetPropAsStr(node, "extends");

    for (child = node->xmlChildrenNode; child; child = child->next) {
        if (xmlNodeIsText(child))
            continue;

        if (xmlStrcmp(child->name, (const xmlChar *) "image") == 0) {
            ImageInfo *image = screenLoadImageInfoFromXml(child);
            if (image->id >= BKGD_MAX)
                errorWarning("image id out of range: %d (%s)", image->id, image->filename);
            else
                set->image[image->id] = image;
        }
    }

    return set;
}

ImageInfo *screenLoadImageInfoFromXml(xmlNodePtr node) {
    ImageInfo *image;
    static const char *filetypeEnumStrings[] = { "raw", "rle", "lzw", NULL };
    static const char *fixupEnumStrings[] = { "none", "intro", "introExtended", "abyss", NULL };

    image = (ImageInfo *) malloc(sizeof(ImageInfo));
    image->id = xmlGetPropAsInt(node, "id");
    image->filename = xmlGetPropAsStr(node, "filename");
    image->width = xmlGetPropAsInt(node, "width");
    image->height = xmlGetPropAsInt(node, "height");
    image->depth = xmlGetPropAsInt(node, "depth");
    image->filetype = xmlGetPropAsEnum(node, "filetype", filetypeEnumStrings);
    image->tiles = xmlGetPropAsInt(node, "tiles");
    image->introOnly = xmlGetPropAsBool(node, "introOnly");
    image->fixup = xmlGetPropAsEnum(node, "fixup", fixupEnumStrings);

    return image;
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

void fixupIntro(Image *im) {
    const unsigned char *sigData;
    int i, x, y;
    SDL_Rect src, dest;

    sigData = introGetSigData();

    /* -----------------------------------------------------------------------------
     * copy "present" to new location between "Origin Systems, Inc." and "Ultima IV"
     * ----------------------------------------------------------------------------- */

    /* we're working with an unscaled surface, so we can't use screenCopyRect, etc. */
    src.x = 136;  src.y = 0;   src.w = 55;  src.h = 5;
    dest.x = 136; dest.y = 33; dest.w = 55;  dest.h = 5;
    SDL_BlitSurface(im->surface, &src, im->surface, &dest);    

    /* ----------------------------
     * erase the original "present"
     * ---------------------------- */

    imageFillRect(im, 136, 0, 55, 5, 0, 0, 0);

    /* -----------------------------
     * draw "Lord British" signature
     * ----------------------------- */
    i = 0;
    while (sigData[i] != 0) {
        /*  (x/y) are unscaled coordinates, i.e. in 320x200  */
        x = sigData[i] + 0x14;
        y = 0xBF - sigData[i+1]; 
        imagePutPixel(im, x, y, 0, 255, 255, IM_OPAQUE); /* cyan */
        imagePutPixel(im, x+1, y, 0, 255, 255, IM_OPAQUE); /* cyan */
        i += 2;
    }

    /* --------------------------------------------------------------
     * draw the red line between "Origin Systems, Inc." and "present"
     * -------------------------------------------------------------- */
    /* we're still working with an unscaled surface */
    for (i = 86; i < 239; i++)
        imagePutPixel(im, i, 31, 128, 0, 0, IM_OPAQUE); /* red */
}

void fixupIntroExtended(Image *im) {
    SDL_Rect src, dest;

    fixupIntro(im);

    src.x = 0;  src.y = 95;  src.w = 320;  src.h = 50;
    dest.x = 0; dest.y = 10; dest.w = 320;  dest.h = 50;
    SDL_BlitSurface(im->surface, &src, im->surface, &dest);    

    src.x = 0;  src.y = 105;  src.w = 320;  src.h = 45;
    dest.x = 0; dest.y = 60; dest.w = 320;  dest.h = 45;
    SDL_BlitSurface(im->surface, &src, im->surface, &dest);    
}

void fixupAbyssVision(Image *im) {
    int i;
    static unsigned char *data = NULL;

    /*
     * The EGA vision components need to be made transparent, so they
     * can be overlaid on each previous image.
     */
    if (settings->videoType == VIDEO_EGA) {
        imageSetTransparentIndex(im, 0);
        return;
    }

    /*
     * Each VGA vision components must be XORed with all the previous
     * vision components to get the actual image.
     */
    if (data != NULL) {
        for (i = 0; i < im->surface->pitch * im->surface->h; i++)
            ((unsigned char *)im->surface->pixels)[i] ^= data[i];
    } else {
        data = malloc(im->surface->pitch * im->surface->h);
    }

    memcpy(data, im->surface->pixels, im->surface->pitch * im->surface->h);
}

/**
 * Returns image information for the given image set.
 */
ImageInfo *screenGetImageInfoFromSet(BackgroundType bkgd, const char *setname) {
    ListNode *setNode;
    ImageSet *set;
    
    ASSERT(imageSets != NULL, "imageSets isn't initialized");

    /* find the correct image set */
    setNode = imageSets;
    while(strcmp(((ImageSet *)setNode->data)->name, setname) != 0) {
        setNode = setNode->next;
        if (!setNode)
            return NULL;
    }
    set = setNode->data;

    /* 
     * if the image set contains the image we want, we are done;
     * otherwise, if this image set extends another, check the base
     * image set.
     */
    if (set->image[bkgd])
        return set->image[bkgd];

    if (set->extends)
        return screenGetImageInfoFromSet(bkgd, set->extends);

    return NULL;
}

ImageInfo *screenGetImageInfo(BackgroundType bkgd) {
    const char *setname;
    
    if (!u4upgradeExists || settings->videoType == VIDEO_EGA)
        setname = "EGA";
    else
        setname = "VGA";

    return screenGetImageInfoFromSet(bkgd, setname);
}

/**
 * Load in a background image from a ".ega" file.
 */
int screenLoadBackground(BackgroundType bkgd) {
    int ret;
    ImageInfo *info;
    const char *filename;
    Image *unscaled;
    U4FILE *file;

    info = screenGetImageInfo(bkgd);
    filename = info->filename;
    if ((!u4upgradeExists || settings->videoType == VIDEO_EGA) && u4upgradeInstalled)
        filename = screenGetImageInfoFromSet(bkgd, "VGA")->filename;

    ret = 0;
    /* try to load the image in VGA first */
    if (filename && info->depth == 8) {
        file = u4fopen(filename);
    
        if (file) {
            ret = screenLoadImageVga(&unscaled,
                                     info->width,
                                     info->height,
                                     file,
                                     info->filetype);
            u4fclose(file);
        }
    }
    /* failed to load VGA image, try EGA instead */
    if (!ret) {
        /* open the file */
        file = u4fopen(filename);

        if (file) {
            ret = screenLoadImageEga(&unscaled,
                                     info->width,
                                     info->height,
                                     file,
                                     info->filetype);
            u4fclose(file);
        }
    }

    if (!ret)
        return 0;

    /*
     * fixup the image before scaling it
     */
    switch (info->fixup) {
    case FIXUP_NONE:
        break;
    case FIXUP_INTRO:
        fixupIntro(unscaled);
        break;
    case FIXUP_INTRO_EXTENDED:
        fixupIntroExtended(unscaled);
        break;
    case FIXUP_ABYSS:
        fixupAbyssVision(unscaled);
        break;
    }

    bkgds[bkgd] = screenScale(unscaled, scale, info->tiles, 1);

    return 1;
}

/**
 * Free up all background images
 */
void screenFreeBackgrounds() {
    int i;

    for (i = 0; i < BKGD_MAX; i++) {
        if (bkgds[i] != NULL) {
            imageDelete(bkgds[i]);
            bkgds[i] = NULL;
        }
    }
}

/**
 * Free up any background images used only in the animations.
 */
void screenFreeIntroBackgrounds() {
    ImageInfo *info;
    int i;

    for (i = 0; i < BKGD_MAX; i++) {
        info = screenGetImageInfo(i);
        if (bkgds[i] == NULL || !info || !info->introOnly)
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

    /* load vga tiles */
    if (settings->videoType == VIDEO_VGA) {
        file = u4fopen("shapes.vga");
        if (file) {
            ret = screenLoadImageVga(&tiles, TILE_WIDTH, TILE_HEIGHT * N_TILES, file, COMP_NONE);
            u4fclose(file);
        }
    }

    /* load ega tiles (also loads if vga load fails) */
    if (!ret || settings->videoType == VIDEO_EGA) {
        file = u4fopen("shapes.ega");
        if (file) {
            ret = screenLoadImageEga(&tiles, TILE_WIDTH, TILE_HEIGHT * N_TILES, file, COMP_NONE);
            u4fclose(file);
        }
    }

    if (tiles)
        tiles = screenScale(tiles, scale, N_TILES, 1);

    return ret;
}

/**
 * Load the gem tile graphics from the "gem.ega.rle" or "gem.vga.rle"
 * file.  Note this file is not part of Ultima IV for DOS.
 */
int screenLoadGemTiles() {
    char *pathname;
    U4FILE *file;
    int ret = 0;

    /* load vga gem tiles */
    if (settings->videoType == VIDEO_VGA) {
        pathname = u4find_graphics("vga/gem.rle");
        if (pathname) {
            file = u4fopen_stdio(pathname);
            free(pathname);
            if (file) {
                ret = screenLoadImageVga(&gemtiles, GEMTILE_W, GEMTILE_H * 128, file, COMP_RLE);
                u4fclose(file);
            }
        }
    }

    /* load ega gem tiles (also loads if vga load fails) */
    if (!ret || settings->videoType == VIDEO_EGA) {
        pathname = u4find_graphics("ega/gem.rle");
        if (pathname) {
            file = u4fopen_stdio(pathname);
            free(pathname);
            if (file) {
                ret = screenLoadImageEga(&gemtiles, GEMTILE_W, GEMTILE_H * 128, file, COMP_RLE);
                u4fclose(file);
            }
        }
    }

    if (gemtiles)
        gemtiles = screenScale(gemtiles, scale, 128, 1);

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
        
        egaPalette[i].r = xmlGetPropAsInt(node, "r");
        egaPalette[i].g = xmlGetPropAsInt(node, "g");
        egaPalette[i].b = xmlGetPropAsInt(node, "b");

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

void screenDeinterlaceCga(unsigned char *data, int width, int height, int tiles, int fudge) {
    unsigned char *tmp;
    int t, x, y;
    int tileheight = height / tiles;

    tmp = malloc(width * tileheight / 4);

    for (t = 0; t < tiles; t++) {
        unsigned char *base;
        base = &(data[t * (width * tileheight / 4)]);
        
        for (y = 0; y < (tileheight / 2); y++) {
            for (x = 0; x < width; x+=4) {
                tmp[((y * 2) * width + x) / 4] = base[(y * width + x) / 4];
            }
        }
        for (y = tileheight / 2; y < tileheight; y++) {
            for (x = 0; x < width; x+=4) {
                tmp[(((y - (tileheight / 2)) * 2 + 1) * width + x) / 4] = base[(y * width + x) / 4 + fudge];
            }
        }
        for (y = 0; y < tileheight; y++) {
            for (x = 0; x < width; x+=4) {
                base[(y * width + x) / 4] = tmp[(y * width + x) / 4];
            }
        }
    }

    free(tmp);
}

/**
 * Load an image from an ".pic" CGA image file.
 */
int screenLoadImageCga(Image **image, int width, int height, U4FILE *file, CompressionType comp, int tiles) {
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

    screenDeinterlaceCga(decompressed_data, width, height, tiles, 0);

    img = imageNew(width, height, 1, 1, IMTYPE_HW);
    if (!img) {
        if (decompressed_data)
            free(decompressed_data);
        return 0;
    }

    SDL_SetColors(img->surface, egaPalette, 0, 16);

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x+=4) {
            imagePutPixelIndex(img, x, y, decompressed_data[(y * width + x) / 4] >> 6);
            imagePutPixelIndex(img, x + 1, y, (decompressed_data[(y * width + x) / 4] >> 4) & 0x03);
            imagePutPixelIndex(img, x + 2, y, (decompressed_data[(y * width + x) / 4] >> 2) & 0x03);
            imagePutPixelIndex(img, x + 3, y, (decompressed_data[(y * width + x) / 4]) & 0x03);
        }
    }
    free(decompressed_data);

    (*image) = img;

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
    Image *charset;

    if (bkgds[BKGD_CHARSET] == NULL) {
        if (!screenLoadBackground(BKGD_CHARSET))
            errorFatal("unable to load data files: is Ultima IV installed?  See http://xu4.sourceforge.net/");
    }
    charset = bkgds[BKGD_CHARSET];
    imageDrawSubRect(charset, x * charset->w, y * (CHAR_HEIGHT * scale),
                     0, chr * (CHAR_HEIGHT * scale),
                     charset->w, CHAR_HEIGHT * scale);
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
    dest.x = x * bkgds[BKGD_CHARSET]->w;
    dest.w = bkgds[BKGD_CHARSET]->w;
    dest.h = scale;
    for (i = 0; i < 8; i++) {
        if (mask & (1 << i)) {
            dest.y = y * (CHAR_HEIGHT * scale) + (i * scale);
            SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 0, 0, 0));
        }
    }
}

/**
 * Draw a tile graphic on the screen.
 */
void screenShowTile(Tileset *tileset, unsigned char tile, int focus, int x, int y) {
    int offset, i, swaprow;
    SDL_Rect src, dest;
    int unscaled_x, unscaled_y;
    /* FIXME: remove global "Image *tiles" definition entirely */
    Image *tiles = tileset->tileGraphic; /* get the graphic to use for the tile */

    if (tileGetAnimationStyle(tile) == ANIM_SCROLL)
        offset = screenCurrentCycle * 4 / SCR_CYCLE_PER_SECOND * scale;
    else
        offset = 0;
    
    /* FIXME: work dynamically with the width/height of tiles in tileset */
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

    if (tileGetAnimationStyle(tile) == ANIM_CAMPFIRE) {
        /* FIXME: animate campfire */
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

    if (swaprow != -1 && xu4_random(2) == 0) {

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
    if (focus && ((screenCurrentCycle * 4 / SCR_CYCLE_PER_SECOND) % 2)) {
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
    SDL_Rect src, dest;

    dest.x = (GEMAREA_X + (x * GEMTILE_W)) * scale;
    dest.y = (GEMAREA_Y + (y * GEMTILE_H)) * scale;
    dest.w = GEMTILE_W * scale;
    dest.h = GEMTILE_H * scale;

    if (tile < 128) {
        src.x = 0;
        src.y = tile * GEMTILE_H * scale;
        src.w = GEMTILE_W * scale;
        src.h = GEMTILE_H * scale;

        SDL_BlitSurface(gemtiles->surface, &src, screen, &dest);
    }

    else {
        SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 0, 0, 0));
    }
}

/**
 * Scroll the text in the message area up one position.
 */
void screenScrollMessageArea() {
    SDL_Rect src, dest;
        
    ASSERT(bkgds[BKGD_CHARSET] != NULL, "charset not initialized!");

    src.x = TEXT_AREA_X * bkgds[BKGD_CHARSET]->w;
    src.y = (TEXT_AREA_Y + 1) * CHAR_HEIGHT * scale;
    src.w = TEXT_AREA_W * bkgds[BKGD_CHARSET]->w;
    src.h = (TEXT_AREA_H - 1) * CHAR_HEIGHT * scale;

    dest.x = src.x;
    dest.y = src.y - CHAR_HEIGHT * scale;
    dest.w = src.w;
    dest.h = src.h;

    SDL_BlitSurface(screen, &src, screen, &dest);

    dest.y += dest.h;
    dest.h = CHAR_HEIGHT * scale;

    SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 0, 0, 0));
    SDL_UpdateRect(screen, 0, 0, 0, 0);
}

/**
 * Invert an area of the screen.
 */
void screenInvertRect(int x, int y, int w, int h) {
    SDL_Rect src;
    Image *tmp;
    RGBA c;
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
            imageGetPixel(tmp, j, i, &c.r, &c.g, &c.b, &c.a);
            imagePutPixel(tmp, j, i, 0xff - c.r, 0xff - c.g, 0xff - c.b, c.a);
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
            eventHandlerSleep(settings->shakeInterval);

            dest.y = -1 * scale;

            SDL_BlitSurface(screen, NULL, screen, &dest);
            SDL_UpdateRect(screen, 0, 0, 0, 0);
            eventHandlerSleep(settings->shakeInterval);
        }
        /* FIXME: remove next line? doesn't seem necessary,
           just adds another screen refresh (which is visible on my screen)... */
        //screenDrawBackground(BKGD_BORDERS);
        SDL_UpdateRect(screen, 0, 0, 0, 0);
    }
}

int screenDungeonGraphicIndex(int xoffset, int distance, Direction orientation, DungeonGraphicType type) {
    int index;

    index = 0;

    if (type == DNGGRAPHIC_LADDERUP && xoffset == 0)
        return 48 + distance;

    if (type == DNGGRAPHIC_LADDERDOWN && xoffset == 0)
        return 52 + distance;

    /* FIXME */
    if (type != DNGGRAPHIC_WALL && type != DNGGRAPHIC_DOOR)
        return -1;

    if (type == DNGGRAPHIC_DOOR)
        index += 24;

    index += (xoffset + 1) * 2;

    index += distance * 6;

    if (DIR_IN_MASK(orientation, MASK_DIR_SOUTH | MASK_DIR_NORTH))
        index++;

    return index;
}

int screenDungeonLoadGraphic(int xoffset, int distance, Direction orientation, DungeonGraphicType type) {
    char *pathname;
    U4FILE *file;
    int index, ret;
    Image *unscaled;

    ret = 0;
    index = screenDungeonGraphicIndex(xoffset, distance, orientation, type);
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
    imageSetTransparentIndex(dngGraphic[index], 0);

    return 1;
}

void screenDungeonDrawTile(int distance, unsigned char tile) {
    SDL_Rect src, dest;
    Image *tmp, *scaled;
    const static int dscale[] = { 8, 4, 2, 1 }, doffset[] = { 96, 96, 88, 88 };
    int offset;
    int savedflags;

    tmp = imageNew(tiles->w, tiles->h / N_TILES, 1, tiles->indexed, IMTYPE_SW);
    if (tiles->indexed)
        imageSetPaletteFromImage(tmp, tiles);

    src.x = 0;
    src.y = tile * (tiles->h / N_TILES);
    src.w = tiles->w;
    src.h = tiles->h / N_TILES;
    dest.x = 0;
    dest.y = 0;
    dest.w = tiles->w;
    dest.h = tiles->h / N_TILES;

    /* have to turn off alpha on tiles before blitting: why? */
    savedflags = tiles->surface->flags;
    tiles->surface->flags &= ~SDL_SRCALPHA;

    SDL_BlitSurface(tiles->surface, &src, tmp->surface, &dest);

    tiles->surface->flags = savedflags;

    /* scale is based on distance; 1 means half size, 2 regular, 4 means scale by 2x, etc. */
    if (dscale[distance] == 1)
        scaled = screenScaleDown(tmp, 2);
    else
        scaled = screenScale(tmp, dscale[distance] / 2, 1, 1);

    /* FIXME: get animation flag properly */
    if (/*tileGetAnimationStyle(tile) == ANIM_SCROLL*/ tile == 2)
        offset = screenCurrentCycle * 4 / SCR_CYCLE_PER_SECOND * scale * dscale[distance] / 2;
    else
        offset = 0;

    src.x = 0;
    src.y = 0;
    src.w = scaled->w;
    src.h = scaled->h - offset;
    dest.x = (VIEWPORT_W * tiles->w / 2) + (BORDER_WIDTH * scale) - (scaled->w / 2);
    dest.y = ((doffset[distance] + BORDER_HEIGHT) * scale) + offset;
    dest.w = scaled->w;
    dest.h = scaled->h;

    SDL_BlitSurface(scaled->surface, &src, screen, &dest);

    if (offset != 0) {

        src.x = 0;
        src.y = scaled->h - offset;
        src.w = scaled->w;
        src.h = offset;
        dest.x = (VIEWPORT_W * tiles->w / 2) + (BORDER_WIDTH * scale) - (scaled->w / 2);
        dest.y = ((doffset[distance] + BORDER_HEIGHT) * scale);
        dest.w = scaled->w;
        dest.h = scaled->h;

        SDL_BlitSurface(scaled->surface, &src, screen, &dest);
    }

    imageDelete(scaled);
}

void screenDungeonDrawWall(int xoffset, int distance, Direction orientation, DungeonGraphicType type) {
    int index;

    index = screenDungeonGraphicIndex(xoffset, distance, orientation, type);
    if (index == -1)
        return;

    if (dngGraphic[index] == NULL) {
        if (!screenDungeonLoadGraphic(xoffset, distance, orientation, type))
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
 * Draws the beads in the abacus during the character creation sequence
 */
void screenShowAbacusBeads(int row, int selectedVirtue, int rejectedVirtue) {
    int x, y, c;

    ASSERT(row >= 0 && row < 7, "invalid row: %d", row);
    ASSERT(selectedVirtue < 8 && selectedVirtue >= 0, "invalid virtue: %d", selectedVirtue);
    ASSERT(rejectedVirtue < 8 && rejectedVirtue >= 0, "invalid virtue: %d", rejectedVirtue);
    
    /* FIXME: Should really be blitting the beads from ABACUS.EGA at 
       (8, 188) and (24, 188) but there are ugly artifacts if you do
       this with scaling switched on. Need to get rid of the artifacts
       somehow. */

    /* For now, here's some code to draw some beads that look something
       like the beads from the Amiga version of Ultima IV (not exactly
       the same) */
       
    /* Draw black bead for the virtue that was *not* selected */
    x = 128 + (rejectedVirtue * 9);
    y = 24 + (row * 15);
    if (row > 2) y--;
    if (row > 3) y--; 
    c = 64;
    screenFillRect(screen, x+3, y, 2, 12, c, c, c);
    screenFillRect(screen, x+2, y+1, 4, 10, c, c, c);
    screenFillRect(screen, x+1, y+2, 6, 8, c, c, c);
    screenFillRect(screen, x, y+3, 8, 6, c, c, c);
    c += 32;
    screenFillRect(screen, x+3, y+1, 1, 10, c, c, c);
    screenFillRect(screen, x+2, y+2, 1, 8, c, c, c);
    screenFillRect(screen, x+1, y+3, 1, 6, c, c, c);
    
    /* Draw white bead for the virtue that was selected */
    x = 128 + (selectedVirtue * 9);
    c = 223;
    screenFillRect(screen, x+3, y, 2, 12, c, c, c);
    screenFillRect(screen, x+2, y+1, 4, 10, c, c, c);
    screenFillRect(screen, x+1, y+2, 6, 8, c, c, c);
    screenFillRect(screen, x, y+3, 8, 6, c, c, c);
    c = 255;
    screenFillRect(screen, x+3, y+1, 1, 10, c, c, c);
    screenFillRect(screen, x+2, y+2, 1, 8, c, c, c);
    screenFillRect(screen, x+1, y+3, 1, 6, c, c, c);
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

/**
 * Scale an image up.  The resulting image will be scale * the
 * original dimensions.  The original image is deleted.  n is the
 * number of tiles in the image; each tile is filtered seperately.
 * filter determines whether or not to filter the resulting image.
 */
Image *screenScale(Image *src, int scale, int n, int filter) {
    Image *dest;
    int transparent;

    if (n == 0)
        n = 1;

    transparent = (src->surface->flags & SDL_SRCCOLORKEY) != 0;

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

    if (transparent)
        imageSetTransparentIndex(dest, 0);

    return dest;
}

/**
 * Scale an image down.  The resulting image will be 1/scale * the
 * original dimensions.  The original image is deleted.
 */
Image *screenScaleDown(Image *src, int scale) {
    int x, y;
    Image *dest;
    int transparent;

    transparent = (src->surface->flags & SDL_SRCCOLORKEY) != 0;

    dest = imageNew(src->w / scale, src->h / scale, 1, src->indexed, IMTYPE_HW);
    if (!dest)
        return NULL;

    if (dest->indexed)
        imageSetPaletteFromImage(dest, src);

    for (y = 0; y < src->h; y+=scale) {
        for (x = 0; x < src->w; x+=scale) {
            unsigned int index;
            imageGetPixelIndex(src, x, y, &index);                
            imagePutPixelIndex(dest, x / scale, y / scale, index);
        }
    }
    imageDelete(src);

    if (transparent)
        imageSetTransparentIndex(dest, 0);

    return dest;
}
