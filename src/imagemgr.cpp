/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include <vector>

#include "config.h"
#include "debug.h"
#include "error.h"
#include "image.h"
#include "imageloader.h"
#include "imagemgr.h"
#include "intro.h"
#include "settings.h"
#include "u4file.h"

using std::map;
using std::string;
using std::vector;

Image *screenScale(Image *src, int scale, int n, int filter);

class ImageSet {
public:
    ~ImageSet();

    string name;
    string location;
    string extends;
    map<string, ImageInfo *> info;
};

ImageMgr *ImageMgr::instance = NULL;

ImageMgr *ImageMgr::getInstance() {
    if (instance == NULL) {
        instance = new ImageMgr();
        instance->init();
    }
    return instance;
}

void ImageMgr::destroy() {
    if (instance != NULL) {
        delete instance;
        instance = NULL;
    }
}

ImageMgr::ImageMgr() {
    logger = new Debug("debug/imagemgr.txt", "ImageMgr");
    TRACE(*logger, "creating ImageMgr");

    settings.addObserver(this);
}

ImageMgr::~ImageMgr() {
    settings.deleteObserver(this);

    for (map<string, ImageSet *>::iterator i = imageSets.begin(); i != imageSets.end(); i++)
        delete i->second;

    delete logger;
}

void ImageMgr::init() {
    TRACE(*logger, "initializing ImageMgr");

    /*
     * register the "screen" image representing the entire screen
     */
    Image *screen = Image::createScreenImage();
    ImageInfo *screenInfo = new ImageInfo;

    screenInfo->name = "screen";
    screenInfo->filename = "";
    screenInfo->width = screen->width();
    screenInfo->height = screen->height();
    screenInfo->depth = 0;
    screenInfo->prescale = 0;
    screenInfo->filetype = "";
    screenInfo->tiles = 0;
    screenInfo->introOnly = false;
    screenInfo->transparentIndex = -1;
    screenInfo->xu4Graphic = false;
    screenInfo->fixup = FIXUP_NONE;
    screenInfo->image = screen;

    /*
     * register all the images declared in the config files
     */
    const Config *config = Config::getInstance();
    vector<ConfigElement> graphicsConf = config->getElement("/config/graphics").getChildren();
    for (std::vector<ConfigElement>::iterator conf = graphicsConf.begin(); conf != graphicsConf.end(); conf++) {
        if (conf->getName() == "imageset") {
            ImageSet *set = loadImageSetFromConf(*conf);
            imageSets[set->name] = set;

            // all image sets include the "screen" image
            set->info[screenInfo->name] = screenInfo;
        }
    }

    imageSetNames.clear();
    for (std::map<string, ImageSet *>::const_iterator set = imageSets.begin(); set != imageSets.end(); set++)
        imageSetNames.push_back(set->first);

    update(NULL, &settings);
}

ImageSet *ImageMgr::loadImageSetFromConf(const ConfigElement &conf) {
    ImageSet *set;

    set = new ImageSet;
    set->name = conf.getString("name");
    set->location = conf.getString("location");
    set->extends = conf.getString("extends");

    TRACE(*logger, string("loading image set ") + set->name);

    vector<ConfigElement> children = conf.getChildren();
    for (std::vector<ConfigElement>::iterator i = children.begin(); i != children.end(); i++) {
        if (i->getName() == "image") {
            ImageInfo *info = loadImageInfoFromConf(*i);
            set->info[info->name] = info;
        }
    }

    return set;
}

ImageInfo *ImageMgr::loadImageInfoFromConf(const ConfigElement &conf) {
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
            SubImage *subimage = loadSubImageFromConf(info, *i);            
            info->subImages[subimage->name] = subimage;
        }
    }

    return info;
}

SubImage *ImageMgr::loadSubImageFromConf(const ImageInfo *info, const ConfigElement &conf) {
    SubImage *subimage;
    static int x = 0,
               y = 0,
               last_width = 0,
               last_height = 0;    

    subimage = new SubImage;
    subimage->name = conf.getString("name");    
    subimage->width = conf.getInt("width");
    subimage->height = conf.getInt("height");
    subimage->srcImageName = info->name;
    if (conf.exists("x") && conf.exists("y")) {
        x = subimage->x = conf.getInt("x");
        y = subimage->y = conf.getInt("y");
    }
    else {
        // Automatically increment our position through the base image
        x += last_width;
        if (x >= last_width) {
            x = 0;
            y += last_height;
        }

        subimage->x = x;
        subimage->y = y;
    }

    // "remember" the width and height of this subimage
    last_width = subimage->width;
    last_height = subimage->height;

    return subimage;
}

void ImageMgr::fixupIntro(Image *im, int prescale) {
    const unsigned char *sigData;
    int i, x, y;

    sigData = intro->getSigData();

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

void ImageMgr::fixupIntroExtended(Image *im, int prescale) {
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

void ImageMgr::fixupAbyssVision(Image *im, int prescale) {
    static unsigned int *data = NULL;

    /*
     * Each VGA vision components must be XORed with all the previous
     * vision components to get the actual image.
     */
    if (data != NULL) {
        for (int y = 0; y < im->height(); y++) {
            for (int x = 0; x < im->width(); x++) {
                unsigned int index;
                im->getPixelIndex(x, y, index);
                index ^= data[y * im->width() + x];
                im->putPixelIndex(x, y, index);
            }
        }
    } else {
        data = new unsigned int[im->width() * im->height()];
    }

    for (int y = 0; y < im->height(); y++) {
        for (int x = 0; x < im->width(); x++) {
            unsigned int index;
            im->getPixelIndex(x, y, index);
            data[y * im->width() + x] = index;
        }
    }
}

void ImageMgr::fixupAbacus(Image *im, int prescale) {

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
void ImageMgr::fixupDungNS(Image *im, int prescale) {
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
ImageSet *ImageMgr::getSet(const string &setname) {
    std::map<string, ImageSet *>::iterator i = imageSets.find(setname);
    if (i != imageSets.end())
        return i->second;
    else
        return NULL;
}

/**
 * Returns image information for the current image set.
 */
ImageInfo *ImageMgr::getInfo(const string &name) {
    return getInfoFromSet(name, baseSet);
}

/**
 * Returns information for the given image set.
 */
ImageInfo *ImageMgr::getInfoFromSet(const string &name, ImageSet *set) {
    if (!set)
        return NULL;

    /* if the image set contains the image we want, we are done */
    std::map<string, ImageInfo *>::iterator i = set->info.find(name);
    if (i != set->info.end())
        return i->second;

    /* otherwise if this image set extends another, check the base image set */
    if (set->extends != "") {
        set = getSet(set->extends);
        return getInfoFromSet(name, set);
    }

    return NULL;
}

/**
 * Load in a background image from a ".ega" file.
 */
ImageInfo *ImageMgr::get(const string &name) {
    ImageInfo *info = getInfo(name);
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
    if (u4isUpgradeInstalled() && getInfoFromSet(name, getSet("VGA"))->filename.find(".old") != string::npos) {
        if (settings.videoType == "EGA")
            filename = getInfoFromSet(name, getSet("VGA"))->filename;
        else
            filename = getInfoFromSet(name, getSet("EGA"))->filename;
    }

    if (filename == "")
        return NULL;

    TRACE(*logger, string("loading image from file '") + filename + string("'"));

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

    int imageScale = settings.scale;
    if ((settings.scale % info->prescale) != 0)
        errorFatal("image %s is prescaled to an incompatible size: %d\n", filename.c_str(), info->prescale);
    imageScale /= info->prescale;
        
    info->image = screenScale(unscaled, imageScale, info->tiles, 1);

    delete unscaled;
    return info;
}

/**
 * Returns information for the given image set.
 */
SubImage *ImageMgr::getSubImage(const string &name) {
    string setname;

    ImageSet *set = baseSet;

    while (set != NULL) {
        for (std::map<string, ImageInfo *>::iterator i = set->info.begin(); i != set->info.end(); i++) {
            ImageInfo *info = (ImageInfo *) i->second;
            std::map<string, SubImage *>::iterator j = info->subImages.find(name);
            if (j != info->subImages.end())
                return j->second;
        }

        set = getSet(set->extends);
    }
        
    return NULL;
}

/**
 * Free up any background images used only in the animations.
 */
void ImageMgr::freeIntroBackgrounds() {
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

const vector<string> &ImageMgr::getSetNames() {
    return imageSetNames;
}

/**
 * Find the new base image set when settings have changed.
 */
void ImageMgr::update(Observable<Settings *> *o, Settings *newSettings) {
    string setname;

    if (!u4isUpgradeAvailable())
        setname = "EGA";
    else
        setname = newSettings->videoType;

    TRACE(*logger, string("base image set is '") + setname + string("'"));

    baseSet = getSet(setname);
}

ImageSet::~ImageSet() {
    for (map<string, ImageInfo *>::iterator i = info.begin(); i != info.end(); i++) {
        ImageInfo *imageInfo = i->second;
        if (imageInfo->name != "screen")
            delete imageInfo;
    }
}

ImageInfo::~ImageInfo() {
    for (map<string, SubImage *>::iterator i = subImages.begin(); i != subImages.end(); i++)
        delete i->second;
    if (image != NULL)
        delete image;
}
