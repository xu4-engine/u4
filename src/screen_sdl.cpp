/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include <algorithm>
#include <functional>
#include <vector>
#include <map>
#include <SDL.h>

#include "config.h"
#include "context.h"

#if defined(MACOSX)
#include "macosx/cursors.h"
#else
#include "cursors.h"
#endif

#include "debug.h"
#include "error.h"
#include "event.h"
#include "image.h"
#include "imageloader.h"
#include "intro.h"
#include "savegame.h"
#include "settings.h"
#include "scale.h"
#include "screen.h"
#include "tileanim.h"
#include "tileset.h"
#include "dngview.h"
#include "u4.h"
#include "u4_sdl.h"
#include "u4file.h"
#include "utils.h"

using std::vector;
using std::map;

void fixupIntro(Image *im, int prescale);
void fixupIntroExtended(Image *im, int prescale);
void fixupAbyssVision(Image *im, int prescale);
void fixupAbacus(Image *im, int prescale);
void fixupDungNS(Image *im, int prescale);
void screenFreeIntroBackground();

Image *screenScale(Image *src, int scale, int n, int filter);
Image *screenScaleDown(Image *src, int scale);

Image *screen;
SDL_Cursor *cursors[5];
int scale;
Scaler filterScaler;

struct ImageSet {
    string name;
    string location;
    string extends;
    map<string, ImageInfo *> info;
};

enum LayoutType {
    LAYOUT_STANDARD,
    LAYOUT_GEM
};

struct Layout {
    string name;
    LayoutType type;
    struct {
        int width, height;
    } tileshape;
    struct {
        int x, y;
        int width, height;
    } viewport;
};

const struct {
    int x, y;
    const char *subimage;
    int x2, y2;
    const char *subimage2;
} dngGraphicInfo[] = {
    { 0,   0,   "dung0_lft_ew" },
    { 0,   0,   "dung0_lft_ns" },
    { 0,   0,   "dung0_mid_ew" },
    { 0,   0,   "dung0_mid_ns" },
    { 144, 0,   "dung0_rgt_ew" },
    { 144, 0,   "dung0_rgt_ns" },

    { 32,  32,  "dung1_lft_ew", 0, 32, "dung1_xxx_ew" },
    { 32,  32,  "dung1_lft_ns", 0, 32, "dung1_xxx_ns" },
    { 0,   32,  "dung1_mid_ew" },
    { 0,   32,  "dung1_mid_ns" },
    { 112, 32,  "dung1_rgt_ew", 144, 32, "dung1_xxx_ew" },
    { 112, 32,  "dung1_rgt_ns", 144, 32, "dung1_xxx_ns" },

    { 64,  64,  "dung2_lft_ew", 0, 64, "dung2_xxx_ew" },
    { 64,  64,  "dung2_lft_ns", 0, 64, "dung2_xxx_ns" },
    { 0,   64,  "dung2_mid_ew" },
    { 0,   64,  "dung2_mid_ns" },
    { 96,  64,  "dung2_rgt_ew", 112, 64, "dung2_xxx_ew" },
    { 96,  64,  "dung2_rgt_ns", 112, 64, "dung2_xxx_ns" },

    { 80,  80,  "dung3_lft_ew", 0, 80, "dung3_xxx_ew" },
    { 80,  80,  "dung3_lft_ns", 0, 80, "dung3_xxx_ns" },
    { 0,   80,  "dung3_mid_ew" },
    { 0,   80,  "dung3_mid_ns" },
    { 88,  80,  "dung3_rgt_ew", 96, 80, "dung3_xxx_ew" },
    { 88,  80,  "dung3_rgt_ns", 96, 80, "dung3_xxx_ns" },

    { 0,   0,   "dung0_lft_ew_door" },
    { 0,   0,   "dung0_lft_ns_door" },
    { 0,   0,   "dung0_mid_ew_door" },
    { 0,   0,   "dung0_mid_ns_door" },
    { 144, 0,   "dung0_rgt_ew_door" },
    { 144, 0,   "dung0_rgt_ns_door" },

    { 32,  32,  "dung1_lft_ew_door", 0, 32, "dung1_xxx_ew" },
    { 32,  32,  "dung1_lft_ns_door", 0, 32, "dung1_xxx_ns" },
    { 0,   32,  "dung1_mid_ew_door" },
    { 0,   32,  "dung1_mid_ns_door" },
    { 112, 32,  "dung1_rgt_ew_door", 144, 32, "dung1_xxx_ew" },
    { 112, 32,  "dung1_rgt_ns_door", 144, 32, "dung1_xxx_ns" },

    { 64,  64,  "dung2_lft_ew_door", 0, 64, "dung2_xxx_ew" },
    { 64,  64,  "dung2_lft_ns_door", 0, 64, "dung2_xxx_ns" },
    { 0,   64,  "dung2_mid_ew_door" },
    { 0,   64,  "dung2_mid_ns_door" },
    { 96,  64,  "dung2_rgt_ew_door", 112, 64, "dung2_xxx_ew" },
    { 96,  64,  "dung2_rgt_ns_door", 112, 64, "dung2_xxx_ns" },

    { 80,  80,  "dung3_lft_ew_door", 0, 80, "dung3_xxx_ew" },
    { 80,  80,  "dung3_lft_ns_door", 0, 80, "dung3_xxx_ns" },
    { 0,   80,  "dung3_mid_ew_door" },
    { 0,   80,  "dung3_mid_ns_door" },
    { 88,  80,  "dung3_rgt_ew_door", 96, 80, "dung3_xxx_ew" },
    { 88,  80,  "dung3_rgt_ns_door", 96, 80, "dung3_xxx_ns" },

    { 45,  0,   "dung0_ladderup" },
    { 64,  40,  "dung1_ladderup" },
    { 77,  68,  "dung2_ladderup" },
    { 84,  82,  "dung3_ladderup" },

    { 45,  87,  "dung0_ladderdown" },
    { 64,  86,  "dung1_ladderdown" },
    { 77,  86,  "dung2_ladderdown" },
    { 84,  88,  "dung3_ladderdown" },

    { 45,  0,   "dung0_ladderupdown" },
    { 64,  40,  "dung1_ladderupdown" },
    { 77,  68,  "dung2_ladderupdown" },
    { 84,  82,  "dung3_ladderupdown" },
};

void screenLoadGraphicsFromConf(void);
ImageSet *screenLoadImageSetFromConf(const ConfigElement &conf);
ImageInfo *screenLoadImageInfoFromConf(const ConfigElement &conf);
SubImage *screenLoadSubImageFromConf(const ConfigElement &conf);
Layout *screenLoadLayoutFromConf(const ConfigElement &conf);
ImageInfo *screenLoadImage(const string &name);
SDL_Cursor *screenInitCursor(char *xpm[]);
int imageInfoByName(void *info1, void *info2);

map<string, ImageSet *> imageSets;
vector<Layout *> layouts;
vector<TileAnimSet *> tileanimSets;
vector<string> imageSetNames;
vector<string> gemLayoutNames;
Layout *gemlayout = NULL;
TileAnimSet *tileanims = NULL;
ImageInfo *charsetInfo = NULL;
ImageInfo *gemTilesInfo = NULL;

extern bool verbose;

void screenInit() {
    charsetInfo = NULL;    
    gemTilesInfo = NULL;

    screenLoadGraphicsFromConf();

    /* set up scaling parameters */
    scale = settings.scale;
    filterScaler = scalerGet(settings.filter);
    if (verbose)
        printf("using %s scaler\n", settings.filters.getName(settings.filter).c_str());

    if (scale < 1 || scale > 5)
        scale = 2;
    
    /* start SDL */
    if (u4_SDL_InitSubSystem(SDL_INIT_VIDEO) < 0)
        errorFatal("unable to init SDL: %s", SDL_GetError());    

    SDL_WM_SetCaption("Ultima IV", NULL);
#ifdef ICON_FILE
    SDL_WM_SetIcon(SDL_LoadBMP(ICON_FILE), NULL);
#endif   

    if (!SDL_SetVideoMode(320 * scale, 200 * scale, 16, SDL_SWSURFACE | SDL_ANYFORMAT | (settings.fullscreen ? SDL_FULLSCREEN : 0)))
        errorFatal("unable to set video: %s", SDL_GetError());

    screen = Image::createScreenImage();

    /* see if the upgrade exists */
    U4FILE *pal;
    if ((pal = u4fopen("u4vga.pal")) != NULL) {
        u4upgradeExists = 1;
        u4fclose(pal);
    }
    u4upgradeInstalled = u4isUpgradeInstalled();

    /* if we can't use vga, reset to default:ega */
    if (!u4upgradeExists && settings.videoType == "VGA")
        settings.videoType = "EGA";

    if (verbose) {
        char driver[32];
        printf("screen initialized [screenInit()], using %s video driver\n", SDL_VideoDriverName(driver, sizeof(driver)));
    }

    KeyHandler::setKeyRepeat(settings.keydelay, settings.keyinterval);

    /* enable or disable the mouse cursor */
    if (settings.mouseOptions.enabled) {
        SDL_ShowCursor(SDL_ENABLE);
        cursors[0] = SDL_GetCursor();
        cursors[1] = screenInitCursor(w_xpm);
        cursors[2] = screenInitCursor(n_xpm);
        cursors[3] = screenInitCursor(e_xpm);
        cursors[4] = screenInitCursor(s_xpm);
    } else {
        SDL_ShowCursor(SDL_DISABLE);
    }

    /* find the tile animations for our tileset */
    tileanims = NULL;
    for (std::vector<TileAnimSet *>::const_iterator i = tileanimSets.begin(); i != tileanimSets.end(); i++) {
        TileAnimSet *set = *i;
        if (set->name == settings.videoType)
            tileanims = set;
    }
    if (!tileanims)
        errorFatal("unable to find tile animations for \"%s\" video mode in graphics.xml", settings.videoType.c_str());
}

void screenDelete() {
    screenFreeImages();
    u4_SDL_QuitSubSystem(SDL_INIT_VIDEO);

    if (verbose)
        printf("screen deleted [screenDelete()]\n");
}

/**
 * Re-initializes the screen and implements any changes made in settings
 */
void screenReInit() {        
    introDelete(DONT_FREE_MENUS);  /* delete intro stuff */
    Tileset::unloadAll(); /* unload tilesets */
    screenDelete(); /* delete screen stuff */            
    screenInit();   /* re-init screen stuff (loading new backgrounds, etc.) */    
    Tileset::loadAll("tilesets.xml"); /* re-load tilesets */
    introInit();    /* re-fix the backgrounds loaded and scale images, etc. */            
}

/**
 * Attempts to iconify the screen.
 */
void screenIconify() {
    SDL_WM_IconifyWindow();
}

const vector<string> &screenGetImageSetNames() {
    return imageSetNames;
}

const vector<string> &screenGetGemLayoutNames() {
    return gemLayoutNames;
}

void screenLoadGraphicsFromConf() {
    const Config *config = Config::getInstance();

    vector<ConfigElement> graphicsConf = config->getElement("/config/graphics").getChildren();
    for (std::vector<ConfigElement>::iterator conf = graphicsConf.begin(); conf != graphicsConf.end(); conf++) {

        if (conf->getName() == "imageset") {
            ImageSet *set = screenLoadImageSetFromConf(*conf);
            imageSets[set->name] = set;
        }
        else if (conf->getName() == "layout")
            layouts.push_back(screenLoadLayoutFromConf(*conf));
        else if (conf->getName() == "tileanimset")
            tileanimSets.push_back(new TileAnimSet(*conf));
    }

    imageSetNames.clear();
    for (std::map<string, ImageSet *>::const_iterator set = imageSets.begin(); set != imageSets.end(); set++)
        imageSetNames.push_back(set->first);

    gemLayoutNames.clear();
    std::vector<Layout *>::const_iterator i;
    for (i = layouts.begin(); i != layouts.end(); i++) {
        Layout *layout = *i;
        if (layout->type == LAYOUT_GEM) {
            gemLayoutNames.push_back(layout->name);
        }
    }

    /*
     * Find gem layout to use.
     */
    for (i = layouts.begin(); i != layouts.end(); i++) {
        Layout *layout = *i;

        if (layout->type == LAYOUT_GEM && layout->name == settings.gemLayout) {
            gemlayout = layout;
            break;
        }
    }
    if (!gemlayout)
        errorFatal("no gem layout named %s found!\n", settings.gemLayout.c_str());
}

ImageSet *screenLoadImageSetFromConf(const ConfigElement &conf) {
    ImageSet *set;

    set = new ImageSet;
    set->name = conf.getString("name");
    set->location = conf.getString("location");
    set->extends = conf.getString("extends");

    vector<ConfigElement> children = conf.getChildren();
    for (std::vector<ConfigElement>::iterator i = children.begin(); i != children.end(); i++) {
        if (i->getName() == "image") {
            ImageInfo *info = screenLoadImageInfoFromConf(*i);
            set->info[info->name] = info;
        }
    }

    return set;
}

ImageInfo *screenLoadImageInfoFromConf(const ConfigElement &conf) {
    ImageInfo *info;
    static const char *fixupEnumStrings[] = { "none", "intro", "introExtended", "abyss", "abacus", "dungns", NULL };

    info = new ImageInfo;
    info->name = conf.getString("name");
    info->filename = conf.getString("filename");
    info->width = conf.getInt("width");
    info->height = conf.getInt("height");
    info->depth = conf.getInt("depth");
    info->prescale = conf.getInt("prescale");
    info->filetype = conf.getString("filetype");
    info->tiles = conf.getInt("tiles");
    info->introOnly = conf.getBool("introOnly");
    info->transparentIndex = conf.getInt("transparentIndex", -1);

    info->xu4Graphic = conf.getBool("xu4Graphic");
    info->fixup = static_cast<ImageFixup>(conf.getEnum("fixup", fixupEnumStrings));
    info->image = NULL;

    vector<ConfigElement> children = conf.getChildren();
    for (std::vector<ConfigElement>::iterator i = children.begin(); i != children.end(); i++) {
        if (i->getName() == "subimage") {
            SubImage *subimage = screenLoadSubImageFromConf(*i);
            subimage->srcImageName = info->name;
            info->subImages[subimage->name] = subimage;
        }
    }

    return info;
}

SubImage *screenLoadSubImageFromConf(const ConfigElement &conf) {
    SubImage *subimage;

    subimage = new SubImage;
    subimage->name = conf.getString("name");
    subimage->x = conf.getInt("x");
    subimage->y = conf.getInt("y");
    subimage->width = conf.getInt("width");
    subimage->height = conf.getInt("height");

    return subimage;
}

Layout *screenLoadLayoutFromConf(const ConfigElement &conf) {
    Layout *layout;
    static const char *typeEnumStrings[] = { "standard", "gem", NULL };

    layout = new Layout;
    layout->name = conf.getString("name");
    layout->type = static_cast<LayoutType>(conf.getEnum("type", typeEnumStrings));

    vector<ConfigElement> children = conf.getChildren();
    for (std::vector<ConfigElement>::iterator i = children.begin(); i != children.end(); i++) {
        if (i->getName() == "tileshape") {
            layout->tileshape.width = i->getInt("width");
            layout->tileshape.height = i->getInt("height");
        }
        else if (i->getName() == "viewport") {
            layout->viewport.x = i->getInt("x");
            layout->viewport.y = i->getInt("y");
            layout->viewport.width = i->getInt("width");
            layout->viewport.height = i->getInt("height");
        }
    }

    return layout;
}


/**
 *  Fills a rectangular screen area with the specified color.  The x,
 *  y, width and height parameters are unscaled, i.e. for 320x200.
 */
void screenFillRect(int x, int y, int w, int h, int r, int g, int b) {
    screen->fillRect(x * scale, y * scale, w * scale, h * scale, r, g, b);
}

void fixupIntro(Image *im, int prescale) {
    const unsigned char *sigData;
    int i, x, y;

    sigData = introGetSigData();

    /* -----------------------------------------------------------------------------
     * copy "present" to new location between "Origin Systems, Inc." and "Ultima IV"
     * ----------------------------------------------------------------------------- */

    im->drawSubRectOn(im, 
                      136 * prescale,
                      33 * prescale,
                      136 * prescale,
                      0 * prescale,
                      55 * prescale,
                      5 * prescale);

    /* ----------------------------
     * erase the original "present"
     * ---------------------------- */

    im->fillRect(136 * prescale, 0 * prescale, 55 * prescale, 5 * prescale, 0, 0, 0);

    /* -----------------------------
     * draw "Lord British" signature
     * ----------------------------- */
    i = 0;
    while (sigData[i] != 0) {
        /*  (x/y) are unscaled coordinates, i.e. in 320x200  */
        x = sigData[i] + 0x14;
        y = 0xBF - sigData[i+1];
        im->fillRect(x * prescale, y * prescale, prescale, prescale, 0, 255, 255); /* cyan */
        im->fillRect((x + 1) * prescale, y * prescale, prescale, prescale, 0, 255, 255); /* cyan */
        i += 2;
    }

    /* --------------------------------------------------------------
     * draw the red line between "Origin Systems, Inc." and "present"
     * -------------------------------------------------------------- */
    /* we're still working with an unscaled surface */
    for (i = 86; i < 239; i++)
        im->fillRect(i * prescale, 31 * prescale, prescale, prescale, 128, 0, 0); /* red */
}

void fixupIntroExtended(Image *im, int prescale) {
    fixupIntro(im, prescale);

    im->drawSubRectOn(im, 
                      0 * prescale,
                      10 * prescale,
                      0 * prescale,
                      95 * prescale,
                      320 * prescale,
                      50 * prescale);

    im->drawSubRectOn(im, 
                      0 * prescale,
                      60 * prescale,
                      0 * prescale, 
                      105 * prescale,
                      320 * prescale,
                      45 * prescale);
}

void fixupAbyssVision(Image *im, int prescale) {
    int i;
    static unsigned char *data = NULL;

    /*
     * Each VGA vision components must be XORed with all the previous
     * vision components to get the actual image.
     */
    if (data != NULL) {
        for (i = 0; i < im->surface->pitch * im->surface->h; i++)
            ((unsigned char *)im->surface->pixels)[i] ^= data[i];
    } else {
        data = new unsigned char[im->surface->pitch * im->surface->h];
    }

    memcpy(data, im->surface->pixels, im->surface->pitch * im->surface->h);
}

void fixupAbacus(Image *im, int prescale) {

    /* 
     * surround each bead with a row green pixels to avoid artifacts
     * when scaling
     */

    im->fillRect(7 * prescale, 186 * prescale, prescale, 14 * prescale, 0, 255, 80); /* green */
    im->fillRect(16 * prescale, 186 * prescale, prescale, 14 * prescale, 0, 255, 80); /* green */
    im->fillRect(8 * prescale, 186 * prescale, prescale * 8, prescale, 0, 255, 80); /* green */
    im->fillRect(8 * prescale, 199 * prescale, prescale * 8, prescale, 0, 255, 80); /* green */

    im->fillRect(23 * prescale, 186 * prescale, prescale, 14 * prescale, 0, 255, 80); /* green */
    im->fillRect(32 * prescale, 186 * prescale, prescale, 14 * prescale, 0, 255, 80); /* green */
    im->fillRect(24 * prescale, 186 * prescale, prescale * 8, prescale, 0, 255, 80); /* green */
    im->fillRect(24 * prescale, 199 * prescale, prescale * 8, prescale, 0, 255, 80); /* green */
}

/**
 * Swap blue and green for the dungeon walls when facing north or
 * south.
 */
void fixupDungNS(Image *im, int prescale) {
    for (int y = 0; y < im->height(); y++) {
        for (int x = 0; x < im->width(); x++) {
            unsigned int index;
            im->getPixelIndex(x, y, index);
            if (index == 1)
                im->putPixelIndex(x, y, 2);
            else if (index == 2)
                im->putPixelIndex(x, y, 1);
        }
    }
}

/**
 * Returns information for the given image set.
 */
ImageSet *screenGetImageSet(const string &setname) {
    std::map<string, ImageSet *>::iterator i = imageSets.find(setname);
    if (i != imageSets.end())
        return i->second;
    else
        return NULL;
}

/**
 * Returns information for the given image set.
 */
ImageInfo *screenGetImageInfoFromSet(const string &name, const string &setname) {
    ImageSet *set;
    
    set = screenGetImageSet(setname);
    if (!set)
        return NULL;

    /* if the image set contains the image we want, we are done */
    
    std::map<string, ImageInfo *>::iterator i = set->info.find(name);
    if (i != set->info.end())
        return i->second;

    /* otherwise if this image set extends another, check the base image set */
    if (set->extends != "")
        return screenGetImageInfoFromSet(name, set->extends);

    return NULL;
}

/**
 * Returns image information for the current image set.
 */
ImageInfo *screenGetImageInfo(const string &name) {
    string setname;
    
    if (!u4upgradeExists)
        setname = "EGA";
    else
        setname = settings.videoType;

    return screenGetImageInfoFromSet(name, setname);
}

/**
 * Returns information for the given image set.
 */
SubImage *screenGetSubImage(const string &name) {
    string setname;
    ImageSet *set;
    
    if (!u4upgradeExists)
        setname = "EGA";
    else
        setname = settings.videoType;

    set = screenGetImageSet(setname);

    while (set != NULL) {
        for (std::map<string, ImageInfo *>::iterator i = set->info.begin(); i != set->info.end(); i++) {
            ImageInfo *info = (ImageInfo *) i->second;
            std::map<string, SubImage *>::iterator j = info->subImages.find(name);
            if (j != info->subImages.end())
                return j->second;
        }

        set = screenGetImageSet(set->extends);
    }
        
    return NULL;
}


/**
 * Load in a background image from a ".ega" file.
 */
ImageInfo *screenLoadImage(const string &name) {
    ImageInfo *info = screenGetImageInfo(name);
    if (!info)
        return NULL;

    /* return if already loaded */
    if (info->image != NULL)
        return info;

    /*
     * If the u4 VGA upgrade is installed (i.e. setup has been run and
     * the u4dos files have been renamed), we need to use VGA names
     * for EGA and vice versa, but *only* when the upgrade file has a
     * .old extention.  The charset and tiles have a .vga extention
     * and are not renamed in the upgrade installation process
     */
    string filename = info->filename;
    if (u4upgradeInstalled && screenGetImageInfoFromSet(name, "VGA")->filename.find(".old") != string::npos) {
        if (settings.videoType == "EGA")
            filename = screenGetImageInfoFromSet(name, "VGA")->filename;
        else
            filename = screenGetImageInfoFromSet(name, "EGA")->filename;
    }

    if (filename == "")
        return NULL;

    U4FILE *file = NULL;
    if (info->xu4Graphic) {
        string pathname(u4find_graphics(filename));

        if (!pathname.empty())
            file = u4fopen_stdio(pathname);
    } 
    else {
        file = u4fopen(filename);
    }
    
    Image *unscaled = NULL;
    if (file) {
        ImageLoader *loader = ImageLoader::getLoader(info->filetype);
        if (loader == NULL)
            errorFatal("can't load image of type \"%s\"", info->filetype.c_str());
        loader->setDimensions(info->width, info->height, info->depth);
        unscaled = loader->load(file);
        u4fclose(file);

    }
    if (unscaled == NULL)
        return NULL;

    if (info->transparentIndex != -1)
        unscaled->setTransparentIndex(info->transparentIndex);

    if (info->prescale == 0)
        info->prescale = 1;

    /*
     * fixup the image before scaling it
     */
    switch (info->fixup) {
    case FIXUP_NONE:
        break;
    case FIXUP_INTRO:
        fixupIntro(unscaled, info->prescale);
        break;
    case FIXUP_INTRO_EXTENDED:
        fixupIntroExtended(unscaled, info->prescale);
        break;
    case FIXUP_ABYSS:
        fixupAbyssVision(unscaled, info->prescale);
        break;
    case FIXUP_ABACUS:
        fixupAbacus(unscaled, info->prescale);
        break;
    case FIXUP_DUNGNS:
        fixupDungNS(unscaled, info->prescale);
        break;
    }

    int imageScale = scale;
    if (info->prescale != 0) {
        if ((scale % info->prescale) != 0)
            errorFatal("image %s is prescaled to an incompatible size: %d\n", filename.c_str(), info->prescale);
        imageScale /= info->prescale;
    }
        
    info->image = screenScale(unscaled, imageScale, info->tiles, 1);

    delete unscaled;
    return info;
}

/**
 * Free up all images
 */
void screenFreeImages() {
    for (std::map<string, ImageSet *>::iterator i = imageSets.begin(); i != imageSets.end(); i++) {
        ImageSet *set = i->second;
        for (std::map<string, ImageInfo *>::iterator j = set->info.begin(); j != set->info.end(); j++) {
            ImageInfo *info = j->second;
            if (info->image != NULL) {
                delete info->image;                
                info->image = NULL;
            }
        }
    }
}

/**
 * Free up any background images used only in the animations.
 */
void screenFreeIntroBackgrounds() {
    for (std::map<string, ImageSet *>::iterator i = imageSets.begin(); i != imageSets.end(); i++) {
        ImageSet *set = i->second;
        for (std::map<string, ImageInfo *>::iterator j = set->info.begin(); j != set->info.end(); j++) {
            ImageInfo *info = j->second;
            if (info->image != NULL && info->introOnly) {
                delete info->image;
                info->image = NULL;
            }
        }
    }
}

#if 0
void screenDeinterlaceCga(unsigned char *data, int width, int height, int tiles, int fudge) {
    unsigned char *tmp;
    int t, x, y;
    int tileheight = height / tiles;

    tmp = new unsigned char[width * tileheight / 4];

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

    delete [] tmp;
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

    img = Image::create(width, height, true, Image::HARDWARE);
    if (!img) {
        if (decompressed_data)
            free(decompressed_data);
        return 0;
    }

    img->setPalette(egaPalette, 16);

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x+=4) {
            img->putPixelIndex(x, y, decompressed_data[(y * width + x) / 4] >> 6);
            img->putPixelIndex(x + 1, y, (decompressed_data[(y * width + x) / 4] >> 4) & 0x03);
            img->putPixelIndex(x + 2, y, (decompressed_data[(y * width + x) / 4] >> 2) & 0x03);
            img->putPixelIndex(x + 3, y, (decompressed_data[(y * width + x) / 4]) & 0x03);
        }
    }
    free(decompressed_data);

    (*image) = img;

    return 1;
}
#endif

/**
 * Draw an image or subimage on the screen.
 */
void screenDrawImage(const string &name, int x, int y) {
    ImageInfo *info = screenLoadImage(name);
    if (info) {
        info->image->draw(x, y);
        return;
    }

    SubImage *subimage = screenGetSubImage(name);
    if (subimage) {
        info = screenLoadImage(subimage->srcImageName);

        if (info) {
            info->image->drawSubRect(x, y, 
                                     subimage->x * (scale / info->prescale),
                                     subimage->y * (scale / info->prescale),
                                     subimage->width * (scale / info->prescale),
                                     subimage->height * (scale / info->prescale));
            return;
        }
    }

    errorFatal("unable to load image \"%s\": is Ultima IV installed?  See http://xu4.sourceforge.net/", name.c_str());
}

void screenDrawImageInMapArea(const string &name) {
    ImageInfo *info;

    info = screenLoadImage(name);
    if (!info)
        errorFatal("unable to load data files: is Ultima IV installed?  See http://xu4.sourceforge.net/");

    info->image->drawSubRect(BORDER_WIDTH * scale, BORDER_HEIGHT * scale,
                             BORDER_WIDTH * scale, BORDER_HEIGHT * scale,
                             VIEWPORT_W * TILE_WIDTH * scale, 
                             VIEWPORT_H * TILE_HEIGHT * scale);
}


/**
 * Draw a character from the charset onto the screen.
 */
void screenShowChar(int chr, int x, int y) {
    if (charsetInfo == NULL) {
        charsetInfo = screenLoadImage(BKGD_CHARSET);
        if (!charsetInfo)
            errorFatal("unable to load data files: is Ultima IV installed?  See http://xu4.sourceforge.net/");
    }

    charsetInfo->image->drawSubRect(x * charsetInfo->image->width(), y * (CHAR_HEIGHT * scale),
                                    0, chr * (CHAR_HEIGHT * scale),
                                    charsetInfo->image->width(), CHAR_HEIGHT * scale);
}

/**
 * Draw a character from the charset onto the screen, but mask it with
 * horizontal lines.  This is used for the avatar symbol in the
 * statistics area, where a line is masked out for each virtue in
 * which the player is not an avatar.
 */
void screenShowCharMasked(int chr, int x, int y, unsigned char mask) {
    int i;

    screenShowChar(chr, x, y);
    for (i = 0; i < 8; i++) {
        if (mask & (1 << i)) {
            screen->fillRect(x * charsetInfo->image->width(),
                             y * (CHAR_HEIGHT * scale) + (i * scale),
                             charsetInfo->image->width(),
                             scale,
                             0, 0, 0);
        }
    }
}

/**
 * Draws a tile to the screen
 */
void Tile::draw(MapTile *mapTile, int x, int y) {
    int scale = ::scale;

    if (image == NULL)
        loadImage();

    if (anim) {
        // First, create our animated version of the tile
        anim->draw(this, mapTile, DIR_NONE);        

        // Then draw it to the screen
        animated->drawSubRect(x * w + (BORDER_WIDTH * scale),
            y * h + (BORDER_HEIGHT * scale),
            0, 0, w, h);
    }
    else {
        image->drawSubRect(x * w + (BORDER_WIDTH * scale), y * h + (BORDER_HEIGHT * scale),
            0, mapTile->frame * h, w, h);    
    }
}

void Tile::drawInDungeon(MapTile *mapTile, int distance, Direction orientation, bool large) {    
    Image *tmp, *scaled;
    const static int nscale[] = { 8, 4, 2, 1 }, doffset[] = { 96, 96, 88, 88 };
    const static int lscale[] = { 22, 14, 6, 2 };
    const int *dscale = large ? lscale : nscale;
    int scale = ::scale;

    if (image == NULL)
        loadImage();

    // create our animated version of the tile
    if (anim) {
        anim->draw(this, mapTile, orientation);
        tmp = Image::duplicate(animated);
    }
    else tmp = Image::duplicate(image);

    /* scale is based on distance; 1 means half size, 2 regular, 4 means scale by 2x, etc. */
    if (dscale[distance] == 1)
        scaled = screenScaleDown(tmp, 2);
    else
        scaled = screenScale(tmp, dscale[distance] / 2, 1, 1);

    scaled->drawSubRect((VIEWPORT_W * w / 2) + (BORDER_WIDTH * scale) - (scaled->width() / 2),
        large ? (VIEWPORT_H * h / 2) + (BORDER_HEIGHT * scale) - (scaled->height() / 2) :
        (doffset[distance] + BORDER_HEIGHT) * scale,
        0,
        0,
        scaled->width(),
        scaled->height());
        
    delete scaled, tmp;
}

/**
 * Draw a focus rectangle around the tile
 */
void Tile::drawFocus(int x, int y) const {
    int scale = ::scale;
    /**
     * draw the focus rectangle around the tile
     */
    if ((screenCurrentCycle * 4 / SCR_CYCLE_PER_SECOND) % 2) {
        /* left edge */
        screen->fillRect(x * w + (BORDER_WIDTH * scale),
                         y * h + (BORDER_HEIGHT * scale),
                         2 * scale,
                         h, 
                         0xff, 0xff, 0xff);

        /* top edge */
        screen->fillRect(x * w + (BORDER_WIDTH * scale),
                         y * h + (BORDER_HEIGHT * scale),
                         w,
                         2 * scale,
                         0xff, 0xff, 0xff);

        /* right edge */
        screen->fillRect((x + 1) * w + (BORDER_WIDTH * scale) - (2 * scale),
                         y * h + (BORDER_HEIGHT * scale),
                         2 * scale,
                         h,
                         0xff, 0xff, 0xff);

        /* bottom edge */
        screen->fillRect(x * w + (BORDER_WIDTH * scale),
                         (y + 1) * h + (BORDER_HEIGHT * scale) - (2 * scale),
                         w,
                         2 * scale,
                         0xff, 0xff, 0xff);
    }
}

/**
 * Loads the tileset image into the global tileset image info
 */ 
void Tileset::loadImage() {
    if (tilesInfo == NULL || tilesInfo->name != getImageName()) {
        tilesInfo = screenLoadImage(getImageName());
                    
        tilesInfo->image->alphaOff(); /* turn alpha off here to preserve it */            

        if (!tilesInfo)
            errorFatal("unable to load tileset images: is Ultima IV installed?  See http://xu4.sourceforge.net/");
    }
}

/**
 * Loads the tile image
 */ 
void Tile::loadImage() {
    if (!image) {
        Tile *tile = NULL;

        if (!looks_like.empty()) {
            // load the tile that looks just like ours
            tile = Tileset::findTileByName(looks_like);
            tile->loadImage();
        }
        
        /* make sure the tileset image is loaded */
        tileset->loadImage();

        Image* tiles = Tileset::getImageInfo()->image;
        scale = ::scale;

        /* load the tile that this looks like */
        if (tile) {
            w = tile->w;
            h = tile->h;
            image = Image::duplicate(tile->image);
            animated = Image::create(w, h, false, Image::HARDWARE);
        }
        else {
            /* create the image for our tile */
            w = tiles->width();
            h = tiles->height() / tileset->numFrames();
            image = Image::create(w, frames * h, false, Image::HARDWARE);
            animated = Image::create(w, h, false, Image::HARDWARE);

            /* draw the tile from the main image to our tile */
            tiles->drawSubRectOn(image, 0, 0, 0, index * h, w, image->height());
        }

        /* if we have animations, we always used 'animated' to draw from */
        if (anim)
            image->alphaOff();        

        if (image == NULL)
            errorFatal("Error: not all tile images loaded correctly, aborting...");
    }
}

/**
 * Draw a tile graphic on the screen.
 */
void screenShowTile(MapTile *mapTile, bool focus, int x, int y) {
    Tileset *t = Tileset::get();    
    Tile *tile = t->get(mapTile->id);    
    
    /**
     * Draw the tile to the screen
     */
    tile->draw(mapTile, x, y);    
    
    /* draw the focus around the tile if it has the focus */
    if (focus)
        tile->drawFocus(x, y);
}

/**
 * Draw a tile graphic on the screen.
 */
void screenShowGemTile(MapTile *mapTile, bool focus, int x, int y) {
    // Make sure we account for tiles that look like other tiles (dungeon tiles, mainly)
    Tile *t = Tileset::get()->get(mapTile->id);
    if (!t->looks_like.empty())
        t = Tileset::findTileByName(t->looks_like);

    unsigned int tile = Tile::getIndex(t->id);

    if (gemTilesInfo == NULL) {
        gemTilesInfo = screenLoadImage(BKGD_GEMTILES);
        if (!gemTilesInfo)
            errorFatal("unable to load data files: is Ultima IV installed?  See http://xu4.sourceforge.net/");
    }

    if (tile < 128) {
        gemTilesInfo->image->drawSubRect((gemlayout->viewport.x + (x * gemlayout->tileshape.width)) * scale,
                                         (gemlayout->viewport.y + (y * gemlayout->tileshape.height)) * scale,
                                         0, 
                                         tile * gemlayout->tileshape.height * scale, 
                                         gemlayout->tileshape.width * scale,
                                         gemlayout->tileshape.height * scale);
    } else {
        screen->fillRect((gemlayout->viewport.x + (x * gemlayout->tileshape.width)) * scale,
                         (gemlayout->viewport.y + (y * gemlayout->tileshape.height)) * scale,
                         gemlayout->tileshape.width * scale,
                         gemlayout->tileshape.height * scale,
                         0, 0, 0);
    }
}

/**
 * Scroll the text in the message area up one position.
 */
void screenScrollMessageArea() {
    ASSERT(charsetInfo != NULL && charsetInfo->image != NULL, "charset not initialized!");

    screen->drawSubRectOn(screen, 
                          TEXT_AREA_X * charsetInfo->image->width(), 
                          TEXT_AREA_Y * CHAR_HEIGHT * scale,
                          TEXT_AREA_X * charsetInfo->image->width(),
                          (TEXT_AREA_Y + 1) * CHAR_HEIGHT * scale,
                          TEXT_AREA_W * charsetInfo->image->width(),
                          (TEXT_AREA_H - 1) * CHAR_HEIGHT * scale);


    screen->fillRect(TEXT_AREA_X * charsetInfo->image->width(),
                     TEXT_AREA_Y * CHAR_HEIGHT * scale + (TEXT_AREA_H - 1) * CHAR_HEIGHT * scale,
                     TEXT_AREA_W * charsetInfo->image->width(),
                     CHAR_HEIGHT * scale,
                     0, 0, 0);
                     
    screenRedrawScreen();
}

/**
 * Inverts the colors of a screen area.
 */
void screenInvertRect(int x, int y, int w, int h) {
    Image *tmp;
    RGBA c;
    int i, j;

    tmp = Image::create(w * scale, h * scale, false, Image::SOFTWARE);
    if (!tmp)
        return;

    screen->drawSubRectOn(tmp, 0, 0, x * scale, y * scale, w * scale, h * scale);

    for (i = 0; i < h * scale; i++) {
        for (j = 0; j < w * scale; j++) {
            tmp->getPixel(j, i, c.r, c.g, c.b, c.a);
            tmp->putPixel(j, i, 0xff - c.r, 0xff - c.g, 0xff - c.b, c.a);
        }
    }

    tmp->draw(x * scale, y * scale);
    delete tmp;

    screenRedrawScreen();
}

/**
 * Do the tremor spell effect where the screen shakes.
 */
void screenShake(int iterations) {
    int x, y, w, h;
    int i;

    if (settings.screenShakes) {
        x = 0 * scale;
        w = 320 * scale;
        h = 200 * scale;

        for (i = 0; i < iterations; i++) {
            y = 1 * scale;

            screen->drawSubRectOn(screen, x, y, 0, 0, w, h);
            screenRedrawScreen();
            EventHandler::sleep(settings.shakeInterval);

            y = -1 * scale;

            screen->drawSubRectOn(screen, x, y, 0, 0, w, h);
            screenRedrawScreen();
            EventHandler::sleep(settings.shakeInterval);
        }
        /* FIXME: remove next line? doesn't seem necessary,
           just adds another screen refresh (which is visible on my screen)... */
        //screenDrawBackground(BKGD_BORDERS);
        screenRedrawScreen();
    }
}

int screenDungeonGraphicIndex(int xoffset, int distance, Direction orientation, DungeonGraphicType type) {
    int index;

    index = 0;

    if (type == DNGGRAPHIC_LADDERUP && xoffset == 0)
        return 48 + distance;

    if (type == DNGGRAPHIC_LADDERDOWN && xoffset == 0)
        return 52 + distance;

    if (type == DNGGRAPHIC_LADDERUPDOWN && xoffset == 0)
        return 56 + distance;

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

void screenDungeonDrawTile(MapTile *mapTile, int distance, Direction orientation) {
    Tileset *t = Tileset::get();    
    Tile *tile = t->get(mapTile->id);
    
    // Draw the tile to the screen
    tile->drawInDungeon(mapTile, distance, orientation, tile->isLarge());
}

void screenDungeonDrawWall(int xoffset, int distance, Direction orientation, DungeonGraphicType type) {
    int index;

    index = screenDungeonGraphicIndex(xoffset, distance, orientation, type);
    if (index == -1)
        return;

    screenDrawImage(dngGraphicInfo[index].subimage, (8 + dngGraphicInfo[index].x) * scale, (8 + dngGraphicInfo[index].y) * scale);
    if (dngGraphicInfo[index].subimage2 != NULL)
        screenDrawImage(dngGraphicInfo[index].subimage2, (8 + dngGraphicInfo[index].x2) * scale, (8 + dngGraphicInfo[index].y2) * scale);
}

/**
 * Force a redraw.
 */
void screenRedrawScreen() {
    SDL_UpdateRect(SDL_GetVideoSurface(), 0, 0, 0, 0);
}

void screenRedrawMapArea() {
    SDL_UpdateRect(SDL_GetVideoSurface(), BORDER_WIDTH * scale, BORDER_HEIGHT * scale, VIEWPORT_W * TILE_WIDTH * scale, VIEWPORT_H * TILE_HEIGHT * scale);
}

/**
 * Animates the moongate in the intro.  The tree intro image has two
 * overlays in the part of the image normally covered by the text.  If
 * the frame parameter is zero, the first overlay is painted over the
 * image: a moongate.  If frame is one, the second overlay is painted:
 * the circle without the moongate, but with a small white dot
 * representing the anhk and history book.
 */
void screenAnimateIntro(const string &frame) {
    screenDrawImage(frame, 72 * scale, 68 * scale);
}

void screenEraseMapArea() {
    screen->fillRect(BORDER_WIDTH * scale, 
                     BORDER_WIDTH * scale, 
                     VIEWPORT_W * TILE_WIDTH * scale,
                     VIEWPORT_H * TILE_HEIGHT * scale,
                     0, 0, 0);
}

void screenEraseTextArea(int x, int y, int width, int height) {
    screen->fillRect(x * CHAR_WIDTH * scale,
                     y * CHAR_HEIGHT * scale,
                     width * CHAR_WIDTH * scale,
                     height * CHAR_HEIGHT * scale,
                     0, 0, 0);
}

void screenRedrawTextArea(int x, int y, int width, int height) {
    SDL_UpdateRect(SDL_GetVideoSurface(), x * CHAR_WIDTH * scale, y * CHAR_HEIGHT * scale, width * CHAR_WIDTH * scale, height * CHAR_HEIGHT * scale);
}

/**
 * Draws a card on the screen for the character creation sequence with
 * the gypsy.
 */
void screenShowCard(int pos, int card) {
    static const char *subImageNames[] = { 
        "honestycard", "compassioncard", "valorcard", "justicecard",
        "sacrificecard", "honorcard", "spiritualitycard", "humilitycard" 
    };

    ASSERT(pos == 0 || pos == 1, "invalid pos: %d", pos);
    ASSERT(card < 8, "invalid card: %d", card);

    screenDrawImage(subImageNames[card], (pos ? 218 : 12) * scale, 12 * scale);
}

/**
 * Draws the beads in the abacus during the character creation sequence
 */
void screenShowAbacusBeads(int row, int selectedVirtue, int rejectedVirtue) {
    ASSERT(row >= 0 && row < 7, "invalid row: %d", row);
    ASSERT(selectedVirtue < 8 && selectedVirtue >= 0, "invalid virtue: %d", selectedVirtue);
    ASSERT(rejectedVirtue < 8 && rejectedVirtue >= 0, "invalid virtue: %d", rejectedVirtue);
    
    screenDrawImage("whitebead", (128 + (selectedVirtue * 9)) * scale, (24 + (row * 15)) * scale);
    screenDrawImage("blackbead", (128 + (rejectedVirtue * 9)) * scale, (24 + (row * 15)) * scale);
}

/**
 * Animates the "beasties" in the intro.  The animate intro image is
 * made up frames for the two creatures in the top left and top right
 * corners of the screen.  This function draws the frame for the given
 * beastie on the screen.  vertoffset is used lower the creatures down
 * from the top of the screen.
 */
void screenShowBeastie(int beast, int vertoffset, int frame) {
    char buffer[128];
    int destx;

    ASSERT(beast == 0 || beast == 1, "invalid beast: %d", beast);

    sprintf(buffer, "beast%dframe%02d", beast, frame);

    destx = beast ? (320 - 48) : 0;
    screenDrawImage(buffer, destx * scale, vertoffset * scale);
}

void screenGemUpdate() {
    MapTile *tile;
    int x, y;
    
    const static MapTile black = Tileset::get()->getByName("black")->id;

    screen->fillRect(BORDER_WIDTH * scale, 
                     BORDER_HEIGHT * scale, 
                     VIEWPORT_W * TILE_WIDTH * scale, 
                     VIEWPORT_H * TILE_HEIGHT * scale,
                     0, 0, 0);

    for (x = 0; x < gemlayout->viewport.width; x++) {
        for (y = 0; y < gemlayout->viewport.height; y++) {
            bool focus;
            tile = screenViewportTile(gemlayout->viewport.width, gemlayout->viewport.height, x, y, focus);
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
 * original dimensions.  The original image is no longer deleted.
 * n is the number of tiles in the image; each tile is filtered 
 * seperately. filter determines whether or not to filter the 
 * resulting image.
 */
Image *screenScale(Image *src, int scale, int n, int filter) {
    Image *dest = NULL;
    bool isTransparent;
    unsigned int transparentIndex;
    bool alpha = src->isAlphaOn();

    if (n == 0)
        n = 1;    

    isTransparent = src->getTransparentIndex(transparentIndex);    
    src->alphaOff();

    while (filter && filterScaler && (scale % 2 == 0)) {
        dest = (*filterScaler)(src, 2, n);
        src = dest;
        scale /= 2;                
    }
    if (scale == 3 && scaler3x(settings.filter)) {
        dest = (*filterScaler)(src, 3, n);
        src = dest;
        scale /= 3;
    }

    if (scale != 1)
        dest = (*scalerGet(SCL_POINT))(src, scale, n);

    if (!dest)
        dest = Image::duplicate(src);

    if (isTransparent)
        dest->setTransparentIndex(transparentIndex);

    if (alpha)
        src->alphaOn();

    return dest;
}

/**
 * Scale an image down.  The resulting image will be 1/scale * the
 * original dimensions.  The original image is no longer deleted.
 */
Image *screenScaleDown(Image *src, int scale) {
    int x, y;
    Image *dest;
    bool isTransparent;
    unsigned int transparentIndex;
    bool alpha = src->isAlphaOn();

    isTransparent = src->getTransparentIndex(transparentIndex);

    src->alphaOff();

    dest = Image::create(src->width() / scale, src->height() / scale, src->isIndexed(), Image::HARDWARE);
    if (!dest)
        return NULL;

    if (dest->isIndexed())
        dest->setPaletteFromImage(src);

    for (y = 0; y < src->height(); y+=scale) {
        for (x = 0; x < src->width(); x+=scale) {
            unsigned int index;
            src->getPixelIndex(x, y, index);                
            dest->putPixelIndex(x / scale, y / scale, index);
        }
    }    

    if (isTransparent)
        dest->setTransparentIndex(transparentIndex);

    if (alpha)
        src->alphaOn();

    return dest;
}

/**
 * Create an SDL cursor object from an xpm.  Derived from example in
 * SDL documentation project.
 */
#if defined(MACOSX)
#define CURSORSIZE 16
#else
#define CURSORSIZE 32
#endif
SDL_Cursor *screenInitCursor(char *xpm[]) {
    int i, row, col;
    Uint8 data[(CURSORSIZE/8)*CURSORSIZE];
    Uint8 mask[(CURSORSIZE/8)*CURSORSIZE];
    int hot_x, hot_y;

    i = -1;
    for (row=0; row < CURSORSIZE; row++) {
        for (col=0; col < CURSORSIZE; col++) {
            if (col % 8) {
                data[i] <<= 1;
                mask[i] <<= 1;
            } else {
                i++;
                data[i] = mask[i] = 0;
            }
            switch (xpm[4+row][col]) {
            case 'X':
                data[i] |= 0x01;
                mask[i] |= 0x01;
                break;
            case '.':
                mask[i] |= 0x01;
                break;
            case ' ':
                break;
            }
        }
    }
    sscanf(xpm[4+row], "%d,%d", &hot_x, &hot_y);
    return SDL_CreateCursor(data, mask, CURSORSIZE, CURSORSIZE, hot_x, hot_y);
}

void screenSetMouseCursor(MouseCursor cursor) {
    static int current = 0;

    if (cursor != current) {
        SDL_SetCursor(cursors[cursor]);
        current = cursor;
    }
}
