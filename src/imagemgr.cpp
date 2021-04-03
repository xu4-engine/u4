/*
 * $Id$
 */

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
#include "xu4.h"

using std::map;
using std::string;
using std::vector;

Image *screenScale(Image *src, int scale, int n, int filter);


ImageMgr::ImageMgr() {
    logger = new Debug("debug/imagemgr.txt", "ImageMgr");
    TRACE(*logger, "creating ImageMgr");

    update(xu4.settings);
    xu4.settings->addObserver(this);
}

ImageMgr::~ImageMgr() {
    xu4.settings->deleteObserver(this);

    std::map<string, ImageSet *>::iterator it;
    foreach (it, imageSets)
        delete it->second;

    delete logger;
}

void ImageMgr::fixupIntro(Image *im, int prescale) {
    const unsigned char *sigData;
    int i, x, y;
    RGBA color;

    sigData = xu4.intro->getSigData();
    if (xu4.settings->videoType != "VGA-ALLPNG" && xu4.settings->videoType != "new") {
        /* ----------------------------
         * update the position of "and"
         * ---------------------------- */
        im->drawSubRectOn(im, 148 * prescale, 17 * prescale,
                          153 * prescale,
                          17 * prescale,
                          11 * prescale,
                          4 * prescale);
        im->drawSubRectOn(im, 159 * prescale, 17 * prescale,
                          165 * prescale,
                          18 * prescale,
                          1 * prescale,
                          4 * prescale);
        im->drawSubRectOn(im, 160 * prescale, 17 * prescale,
                          164 * prescale,
                          17 * prescale,
                          16 * prescale,
                          4 * prescale);
        /* ---------------------------------------------
         * update the position of "Origin Systems, Inc."
         * --------------------------------------------- */
        im->drawSubRectOn(im, 86 * prescale, 21 * prescale,
                          88 * prescale,
                          21 * prescale,
                          114 * prescale,
                          9 * prescale);
        im->drawSubRectOn(im, 199 * prescale, 21 * prescale,
                          202 * prescale,
                          21 * prescale,
                          6 * prescale,
                          9 * prescale);
        im->drawSubRectOn(im, 207 * prescale, 21 * prescale,
                          208 * prescale,
                          21 * prescale,
                          28 * prescale,
                          9 * prescale);
        /* ---------------------------------------------
         * update the position of "Ultima IV"
         * --------------------------------------------- */
        // move this *prior* to moving "present"
        im->drawSubRectOn(im, 59 * prescale, 33 * prescale,
                          61 * prescale,
                          33 * prescale,
                          204 * prescale,
                          46 * prescale);
#if 0
        /*
         * NOTE: This just tweaks the kerning by a few pixels.  With the Image
         * rewrite for RGBA8-only images, "THE" disappears (dest. X must be
         * less than source X when an image blits onto itself).
         *
         * If the goal is to preserve the original experience then the image
         * should not be touched anyway, and a "recreated" experience will
         * have correct input images.
         */

        /* ---------------------------------------------
         * update the position of "Quest of the Avatar"
         * --------------------------------------------- */
        im->drawSubRectOn(im, 69 * prescale, 80 * prescale,     // quEst
                          70 * prescale,
                          80 * prescale,
                          11 * prescale,
                          13 * prescale);
        im->drawSubRectOn(im, 82 * prescale, 80 * prescale,     // queST
                          84 * prescale,
                          80 * prescale,
                          27 * prescale,
                          13 * prescale);
        im->drawSubRectOn(im, 131 * prescale, 80 * prescale,    // oF
                          132 * prescale,
                          80 * prescale,
                          11 * prescale,
                          13 * prescale);
        im->drawSubRectOn(im, 150 * prescale, 80 * prescale,    // THE
                          149 * prescale,
                          80 * prescale,
                          40 * prescale,
                          13 * prescale);
        im->drawSubRectOn(im, 166 * prescale, 80 * prescale,    // tHe
                          165 * prescale,
                          80 * prescale,
                          11 * prescale,
                          13 * prescale);
        im->drawSubRectOn(im, 200 * prescale, 80 * prescale,    // AVATAR
                          201 * prescale,
                          80 * prescale,
                          81 * prescale,
                          13 * prescale);
        im->drawSubRectOn(im, 227 * prescale, 80 * prescale,    // avAtar
                          228 * prescale,
                          80 * prescale,
                          11 * prescale,
                          13 * prescale);
#endif
    }
    /* -----------------------------------------------------------------------------
     * copy "present" to new location between "Origin Systems, Inc." and "Ultima IV"
     * ----------------------------------------------------------------------------- */
    // do this *after* moving "Ultima IV"
    im->drawSubRectOn(im, 132 * prescale, 33 * prescale,
                      135 * prescale,
                      0 * prescale,
                      56 * prescale,
                      5 * prescale);

    /* ----------------------------
     * erase the original "present"
     * ---------------------------- */
    im->fillRect(135 * prescale, 0 * prescale, 56 * prescale, 5 * prescale, 0, 0, 0);

    /* -------------------------
     * update the colors for VGA
     * ------------------------- */
    if (xu4.settings->videoType == "VGA")
    {
        ImageInfo *borderInfo = ImageMgr::get(BKGD_BORDERS, true);
//        ImageInfo *charsetInfo = ImageMgr::get(BKGD_CHARSET);
        if (!borderInfo)
            errorFatal("ERROR 1001: Unable to load the \"%s\" data file.\t\n\nIs %s installed?\n\nVisit the XU4 website for additional information.\n\thttp://xu4.sourceforge.net/", BKGD_BORDERS, xu4.settings->game.c_str());

        delete borderInfo->image;
        borderInfo->image = NULL;
        borderInfo = ImageMgr::get(BKGD_BORDERS, true);

        //borderInfo->image->save("border.png");

        // update the border appearance
        borderInfo->image->drawSubRectOn(im, 0, 96, 0, 0, 16, 56);
        for (int i=0; i < 9; i++)
        {
            borderInfo->image->drawSubRectOn(im, 16+(i*32), 96, 144, 0, 48, 48);
        }
        im->drawSubRectInvertedOn(im, 0, 144, 0, 104, 320, 40);
        im->drawSubRectOn(im, 0, 184, 0, 96, 320, 8);

        delete borderInfo->image;
        borderInfo->image = NULL;
    }

    /* -----------------------------
     * draw "Lord British" signature
     * ----------------------------- */
    color = im->setColor(0, 255, 255);  // cyan for EGA
    int blue[16] = {255, 250, 226, 226, 210, 194, 161, 161,
                    129,  97,  97,  64,  64,  32,  32,   0};
    i = 0;
    while (sigData[i] != 0) {
        /* (x/y) are unscaled coordinates, i.e. in 320x200 */
        x = sigData[i] + 0x14;
        y = 0xBF - sigData[i+1];

        if (xu4.settings->videoType != "EGA")
        {
            // yellow gradient
            color = im->setColor(255, (y == 1 ? 250 : 255), blue[y]);
        }

        im->fillRect(x * prescale, y * prescale,
                     2 * prescale, prescale,
                     color.r, color.g, color.b);
        i += 2;
    }

    /* --------------------------------------------------------------
     * draw the red line between "Origin Systems, Inc." and "present"
     * -------------------------------------------------------------- */
    /* we're still working with an unscaled surface */
    if (xu4.settings->videoType != "EGA")
    {
        color = im->setColor(0, 0, 161);    // dark blue
    }
    else
    {
        color = im->setColor(128, 0, 0);    // dark red for EGA
    }
    for (i = 84; i < 236; i++)  // 152 px wide
        im->fillRect(i * prescale, 31 * prescale,
                     prescale, prescale,
                     color.r, color.g, color.b);
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
 * The FMTowns images have a different screen dimension. This moves them up to what xu4 is accustomed to.
 * south.
 */
void ImageMgr::fixupFMTowns(Image *im, int prescale) {
    for (int y = 20; y < im->height(); y++) {
        for (int x = 0; x < im->width(); x++) {
            unsigned int index;
            im->getPixelIndex(x, y, index);
            im->putPixelIndex(x, y-20, index);
        }
    }
}

/**
 * Returns information for the given image set.
 */
ImageSet *ImageMgr::scheme(const string& name) {
    std::map<string, ImageSet *>::iterator it = imageSets.find(name);
    if (it != imageSets.end())
        return it->second;

    // The ImageSet has not been cached yet, so get it from the Config.
    const char** names = xu4.config->schemeNames();
    const char** nit = names;
    while (*nit) {
        if (name == *nit) {
            ImageSet* sp = xu4.config->newScheme(nit - names);
            if (! sp)
                break;
            imageSets[sp->name] = sp;
            return sp;
        }
        ++nit;
    }
    return NULL;
}

/**
 * Returns information for the given image set.
 */
ImageInfo *ImageMgr::getInfoFromSet(const string &name, ImageSet *imageset) {
    if (!imageset)
        return NULL;

    /* if the image set contains the image we want, we are done */
    std::map<string, ImageInfo *>::iterator i = imageset->info.find(name);
    if (i != imageset->info.end())
        return i->second;

    /* otherwise if this image set extends another, check the base image set */
    while (imageset->extends != "") {
        imageset = scheme(imageset->extends);
        return getInfoFromSet(name, imageset);
    }

    return NULL;
}

U4FILE * ImageMgr::getImageFile(ImageInfo *info)
{
    string filename = info->filename;

    /*
     * If the u4 VGA upgrade is installed (i.e. setup has been run and
     * the u4dos files have been renamed), we need to use VGA names
     * for EGA and vice versa, but *only* when the upgrade file has a
     * .old extention.  The charset and tiles have a .vga extention
     * and are not renamed in the upgrade installation process
     */
    if (u4isUpgradeInstalled()) {
        string& vgaFile = getInfoFromSet(info->name, scheme("VGA"))->filename;
        if (vgaFile.find(".old") != string::npos) {
            if (xu4.settings->videoType == "EGA")
                filename = vgaFile;
            else
                filename = getInfoFromSet(info->name, scheme("EGA"))->filename;
        }
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
    return file;
}

/**
 * Load in a background image from a ".ega" file.
 */
ImageInfo *ImageMgr::get(const string &name, bool returnUnscaled) {
    ImageInfo *info = getInfoFromSet(name, baseSet);
    if (!info)
        return NULL;

    /* return if already loaded */
    if (info->image != NULL)
        return info;

    U4FILE *file = getImageFile(info);
    Image *unscaled = NULL;
    if (file) {
        TRACE(*logger, string("loading image from file '") + info->filename + string("'"));

        unscaled = loadImage(file, info->filetype, info->width, info->height,
                             info->depth);
        u4fclose(file);

        if (! unscaled) {
            errorWarning("Can't load image \"%s\" with type %d",
                         info->filename.c_str(), info->filetype);
            return info;
        }

        if (info->width == -1) {
            // Write in the values for later use.
            info->width  = unscaled->width();
            info->height = unscaled->height();
        }
#if 0
        string out("/tmp/xu4/");
        unscaled->save(out.append(name).append(".ppm").c_str());
#endif
    }
    else
    {
        errorWarning("Failed to open file %s for reading.", info->filename.c_str());
        return NULL;
    }

    if (unscaled == NULL)
        return NULL;

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
    case FIXUP_ABYSS:
        fixupAbyssVision(unscaled, info->prescale);
        break;
    case FIXUP_ABACUS:
        fixupAbacus(unscaled, info->prescale);
        break;
    case FIXUP_DUNGNS:
        fixupDungNS(unscaled, info->prescale);
        break;
    case FIXUP_FMTOWNSSCREEN:
        fixupFMTowns(unscaled, info->prescale);
        break;
    case FIXUP_BLACKTRANSPARENCYHACK:
        //Apply transparency shadow hack to ultima4 ega and vga upgrade classic graphics.
        Image *unscaled_original = unscaled;
        unscaled = Image::duplicate(unscaled);
        delete unscaled_original;
        if (xu4.settings->enhancements &&
            xu4.settings->enhancementsOptions.u4TileTransparencyHack)
        {
            int transparency_shadow_size =xu4.settings->enhancementsOptions.u4TrileTransparencyHackShadowBreadth;
            int opacity = xu4.settings->enhancementsOptions.u4TileTransparencyHackPixelShadowOpacity;

            // NOTE: The first 16 tiles are landscape and must be fully opaque!
            int f = (name == "tiles") ? 16 : 0;
            int frames = info->tiles;
            for ( ; f < frames; ++f)
                unscaled->performTransparencyHack(0, frames, f, transparency_shadow_size, opacity);
        }
        break;
    }

#if 0
    string out2("/tmp/xu4/");
    unscaled->save(out2.append(name).append("-fixup.ppm").c_str());
#endif

    if (returnUnscaled)
    {
        info->image = unscaled;
        return info;
    }

    int imageScale = xu4.settings->scale;
    if ((imageScale % info->prescale) != 0) {
        int orig_scale = imageScale;
        xu4.settings->scale = info->prescale;
        xu4.settings->write();
        errorFatal("image %s is prescaled to an incompatible size: %d\nResetting the scale to %d. Sorry about the inconvenience, please restart.", info->filename.c_str(), orig_scale, xu4.settings->scale);
    }
    imageScale /= info->prescale;

    info->image = screenScale(unscaled, imageScale, info->tiles, 1);

    delete unscaled;

#if 0
    string out3("/tmp/xu4/");
    info->image->save(out3.append(name).append("-scale.ppm").c_str());
#endif
    return info;
}

/**
 * Returns information for the given image set.
 */
SubImage *ImageMgr::getSubImage(const string &name) {
    std::map<string, ImageInfo *>::iterator it;
    ImageSet *set = baseSet;

    while (set != NULL) {
        foreach (it, set->info) {
            ImageInfo *info = (ImageInfo *) it->second;
            std::map<string, SubImage *>::iterator j = info->subImages.find(name);
            if (j != info->subImages.end())
                return j->second;
        }
        set = scheme(set->extends);
    }

    return NULL;
}

/**
 * Free up any background images used only in the animations.
 */
void ImageMgr::freeIntroBackgrounds() {
    std::map<string, ImageSet *>::iterator si;
    std::map<string, ImageInfo *>::iterator j;

    foreach (si, imageSets) {
        foreach (j, si->second->info) {
            ImageInfo *info = j->second;
            if (info->image != NULL && info->introOnly) {
                delete info->image;
                info->image = NULL;
            }
        }
    }
}

/**
 * Find the new base image set when settings have changed.
 */
void ImageMgr::update(Settings *newSettings) {
    string setname = newSettings->videoType;
    TRACE(*logger, string("base image set is '") + setname + string("'"));
    baseSet = scheme(setname);
}

ImageSet::~ImageSet() {
    std::map<string, ImageInfo *>::iterator it;
    foreach (it, info)
        delete it->second;
}

ImageInfo::~ImageInfo() {
    std::map<string, SubImage *>::iterator it;
    foreach (it, subImages)
        delete it->second;

    delete image;
}
