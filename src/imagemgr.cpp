/*
 * imagemgr.cpp
 */

#include <string.h>

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

#ifdef USE_GL
#include "gpu.h"
#endif

using std::string;

Image *screenScale(Image *src, int scale, int n, int filter);

//#define dprint  printf


ImageSymbols ImageMgr::sym;

ImageMgr::ImageMgr() : vgaColors(NULL), resGroup(0) {
    logger = new Debug("debug/imagemgr.txt", "ImageMgr");
    TRACE(*logger, "creating ImageMgr");

    update(xu4.settings);
    xu4.settings->addObserver(this);

    xu4.config->internSymbols(&sym.tiles, 45,
        "tiles charset borders title options_top\n"
        "options_btm tree portal outside inside\n"
        "wagon gypsy abacus honcom valjus\n"
        "sachonor spirhum beasties key honesty\n"
        "compassn valor justice sacrific honor\n"
        "spirit humility truth love courage\n"
        "stoncrcl rune0 rune1 rune2 rune3\n"
        "rune4 rune5 rune6 rune7 rune8\n"
        "gemtiles moongate items blackbead whitebead");
}

ImageMgr::~ImageMgr() {
    xu4.settings->deleteObserver(this);

    std::map<Symbol, ImageSet *>::iterator it;
    foreach (it, imageSets)
        delete it->second;

    delete[] vgaColors;
    delete logger;
}

#ifdef USE_GL
#define PRESCALE(A, B, C, D, E, F)  A, B, C, D, E, F
#define PRESCALE_4(A, B, C, D)      A, B, C, D
#else
#define PRESCALE(A, B, C, D, E, F) \
    A*prescale, B*prescale, C*prescale, D*prescale, E*prescale, F*prescale
#define PRESCALE_4(A, B, C, D) \
    A*prescale, B*prescale, C*prescale, D*prescale
#endif

void ImageMgr::fixupIntro(Image *im, int prescale) {
    const unsigned char *sigData;
    int i, x, y;
    RGBA color;

    sigData = xu4.intro->getSigData();
    if (xu4.settings->videoType != "VGA-ALLPNG" && xu4.settings->videoType != "new") {
        /* ----------------------------
         * update the position of "and"
         * ---------------------------- */
        im->drawSubRectOn(im, PRESCALE(148, 17, 153, 17, 11, 4));
        im->drawSubRectOn(im, PRESCALE(159, 17, 165, 18,  1, 4));
        im->drawSubRectOn(im, PRESCALE(160, 17, 164, 17, 16, 4));
        /* ---------------------------------------------
         * update the position of "Origin Systems, Inc."
         * --------------------------------------------- */
        im->drawSubRectOn(im, PRESCALE( 86, 21,  88, 21, 114, 9));
        im->drawSubRectOn(im, PRESCALE(199, 21, 202, 21,   6, 9));
        im->drawSubRectOn(im, PRESCALE(207, 21, 208, 21,  28, 9));
        /* ---------------------------------------------
         * update the position of "Ultima IV"
         * --------------------------------------------- */
        // move this *prior* to moving "present"
        im->drawSubRectOn(im, PRESCALE(59, 33, 61, 33, 204, 46));
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
        im->drawSubRectOn(im, PRESCALE( 69, 80,  70, 80, 11, 13));  // quEst
        im->drawSubRectOn(im, PRESCALE( 82, 80,  84, 80, 27, 13));  // queST
        im->drawSubRectOn(im, PRESCALE(131, 80, 132, 80, 11, 13));  // oF
        im->drawSubRectOn(im, PRESCALE(150, 80, 149, 80, 40, 13));  // THE
        im->drawSubRectOn(im, PRESCALE(166, 80, 165, 80, 11, 13));  // tHe
        im->drawSubRectOn(im, PRESCALE(200, 80, 201, 80, 81, 13));  // AVATAR
        im->drawSubRectOn(im, PRESCALE(227, 80, 228, 80, 11, 13));  // avAtar
#endif
    }
    /* -----------------------------------------------------------------------------
     * copy "present" to new location between "Origin Systems, Inc." and "Ultima IV"
     * ----------------------------------------------------------------------------- */
    // do this *after* moving "Ultima IV"
    im->drawSubRectOn(im, PRESCALE(132, 33, 135, 0, 56, 5));

    /* ----------------------------
     * erase the original "present"
     * ---------------------------- */
    im->fillRect(PRESCALE_4(135, 0, 56, 5), 0, 0, 0);

    /* -------------------------
     * update the colors for VGA
     * ------------------------- */
    if (xu4.settings->videoType == "VGA")
    {
        ImageInfo *borderInfo = ImageMgr::get(BKGD_BORDERS, true);
        if (! borderInfo)
            errorLoadImage(BKGD_BORDERS);

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

        im->fillRect(PRESCALE_4(x, y, 2, 1), color.r, color.g, color.b);
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
        im->fillRect(PRESCALE_4(i, 31, 1, 1), color.r, color.g, color.b);
}

void ImageMgr::fixupAbyssVision(Image *im) {
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

    im->fillRect(PRESCALE_4( 7, 186, 1, 14), 0, 255, 80); /* green */
    im->fillRect(PRESCALE_4(16, 186, 1, 14), 0, 255, 80); /* green */
    im->fillRect(PRESCALE_4( 8, 186, 8,  1), 0, 255, 80); /* green */
    im->fillRect(PRESCALE_4( 8, 199, 8,  1), 0, 255, 80); /* green */

    im->fillRect(PRESCALE_4(23, 186, 1, 14), 0, 255, 80); /* green */
    im->fillRect(PRESCALE_4(32, 186, 1, 14), 0, 255, 80); /* green */
    im->fillRect(PRESCALE_4(24, 186, 8,  1), 0, 255, 80); /* green */
    im->fillRect(PRESCALE_4(24, 199, 8,  1), 0, 255, 80); /* green */
}

/**
 * Swap blue and green for the dungeon walls when facing north or
 * south.
 */
void ImageMgr::fixupDungNS(Image *im) {
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
void ImageMgr::fixupFMTowns(Image *im) {
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
ImageSet *ImageMgr::scheme(Symbol name) {
    std::map<Symbol, ImageSet *>::iterator it = imageSets.find(name);
    if (it != imageSets.end())
        return it->second;

    // The ImageSet has not been cached yet, so get it from the Config.
    const char* nameStr = xu4.config->symbolName(name);
    const char** names = xu4.config->schemeNames();
    const char** nit = names;
    while (*nit) {
        if (strcmp(nameStr, *nit) == 0) {
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
ImageInfo *ImageMgr::getInfoFromSet(Symbol name, ImageSet *imageset) {
    if (!imageset)
        return NULL;

    /* if the image set contains the image we want, we are done */
    std::map<Symbol, ImageInfo *>::iterator i = imageset->info.find(name);
    if (i != imageset->info.end())
        return i->second;

    /* otherwise if this image set extends another, check the base image set */
    while (imageset->extends != SYM_UNSET) {
        imageset = scheme(imageset->extends);
        return getInfoFromSet(name, imageset);
    }

    return NULL;
}

U4FILE * ImageMgr::getImageFile(ImageInfo *info)
{
    U4FILE *file;
    const char* fn = xu4.config->confString(info->filename);

    if (strncmp(fn, "u4/", 3) == 0 ||
        strncmp(fn, "u4u/", 4) == 0) {
        // Original game data - strip off path.
        string basename = fn + ((fn[2] == '/') ? 3 : 4);

        /*
         * If the u4 VGA upgrade is installed (i.e. setup has been run and
         * the u4dos files have been renamed), we need to use VGA names
         * for EGA and vice versa, but *only* when the upgrade file has a
         * .old extention.  The charset and tiles have a .vga extention
         * and are not renamed in the upgrade installation process
         */
        if (u4isUpgradeInstalled()) {
            Symbol sname[2];
            xu4.config->internSymbols(sname, 2, "VGA EGA");

            string vgaFile = getInfoFromSet(info->name, scheme(sname[0]))->getFilename();
            if (vgaFile.find(".old") != string::npos) {
                if (xu4.settings->videoType == "EGA")
                    basename = vgaFile;
                else
                    basename = getInfoFromSet(info->name, scheme(sname[1]))->getFilename();
            }
        }

        file = u4fopen(basename);
#ifdef CONF_MODULE
    } else if (fn[0] == 'I' && fn[2] < 0x20) {
        const CDIEntry* ent = xu4.config->imageFile(fn);
        if (ent) {
            file = u4fopen_stdio(xu4.config->modulePath());
            u4fseek(file, ent->offset, SEEK_SET);
        } else
            file = NULL;
#endif
    } else {
        string filename(fn);
        string pathname(u4find_graphics(filename));
        if (pathname.empty())
            file = NULL;
        else
            file = u4fopen_stdio(pathname.c_str());
    }
    return file;
}

ImageInfo* ImageMgr::imageInfo(Symbol name, const SubImage** subPtr) {
    const SubImage* subImg = NULL;
    ImageInfo* info = get(name);
    if (! info) {
        subImg = getSubImage(name, &info);
        if (subImg) {
            if (! info->image)
                info = load(info, false);
        }
    }
    *subPtr = subImg;
    return info;
}

/**
 * Load in a background image from a ".ega" file.
 */
ImageInfo *ImageMgr::get(Symbol name, bool returnUnscaled) {
    ImageInfo *info = getInfoFromSet(name, baseSet);
    if (! info)
        return NULL;

    /* return if already loaded */
    if (info->image != NULL)
        return info;

    return load(info, returnUnscaled);
}

ImageInfo* ImageMgr::load(ImageInfo* info, bool returnUnscaled) {
    U4FILE *file = getImageFile(info);
    Image *unscaled = NULL;
    if (file) {
        TRACE(*logger, string("loading image from file '") + info->filename + string("'"));
        //dprint( "ImageMgr load %d:%s\n", resGroup, info->filename.c_str() );

        unscaled = loadImage(file, info->filetype, info->width, info->height,
                             info->depth);
        u4fclose(file);

        if (! unscaled) {
            errorWarning("Can't load image \"%s\" with type %d",
                         xu4.config->confString(info->filename), info->filetype);
            return info;
        }

        info->resGroup = resGroup;
        if (info->width == -1) {
            // Write in the values for later use.
            info->width  = unscaled->width();
            info->height = unscaled->height();
        }

#ifdef USE_GL
        // Pre-compute tile UVs.
        if (info->tiles > 1 && info->tileTexCoord == NULL ) {
            // Assuming image is one tile wide.
            float iwf = (float) unscaled->width();
            float ihf = (float) unscaled->height();
            float tileH = iwf;
            float tileY = 0.0f;
            float *uv;

            info->tileTexCoord = uv = new float[info->tiles * 4];
            for (int i = 0; i < info->tiles; ++i) {
                *uv++ = 0.0f;
                *uv++ = tileY / ihf;
                *uv++ = 1.0f;
                *uv++ = (tileY + tileH) / ihf;
                tileY += tileH;
            }
        }
        /*
        SubImage* simg = (SubImage*) info->subImages;
        SubImage* end = simg + info->subImageCount;
        while (simg != end) {
            simg->u0 = simg->x / iwf;
            simg->v0 = simg->y / ihf;
            simg->u1 = (simg->x + simg->width) / iwf;
            simg->v1 = (simg->y + simg->height) / ihf;
            ++simg;
        }
        */
#endif

#if 0
        string out("/tmp/xu4/");
        unscaled->save(out.append(name).append(".ppm").c_str());
#endif
    }
    else
    {
        errorWarning("Failed to open file %s for reading.",
                     xu4.config->confString(info->filename));
        return NULL;
    }

    if (unscaled == NULL)
        return NULL;

#ifdef USE_GL
    info->prescale = 1;
#else
    if (info->prescale == 0)
        info->prescale = 1;
#endif

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
        fixupAbyssVision(unscaled);
        break;
    case FIXUP_ABACUS:
        fixupAbacus(unscaled, info->prescale);
        break;
    case FIXUP_DUNGNS:
        fixupDungNS(unscaled);
        break;
    case FIXUP_FMTOWNSSCREEN:
        fixupFMTowns(unscaled);
        break;
    case FIXUP_BLACKTRANSPARENCYHACK:
        //Apply transparency shadow hack to ultima4 ega and vga upgrade classic graphics.
        if (xu4.settings->enhancements &&
            xu4.settings->enhancementsOptions.u4TileTransparencyHack)
        {
            int transparency_shadow_size =xu4.settings->enhancementsOptions.u4TrileTransparencyHackShadowBreadth;
            int opacity = xu4.settings->enhancementsOptions.u4TileTransparencyHackPixelShadowOpacity;

            // NOTE: The first 16 tiles are landscape and must be fully opaque!
            int f = (info->name == BKGD_SHAPES) ? 16 : 0;
            int frames = info->tiles;
            for ( ; f < frames; ++f)
                unscaled->performTransparencyHack(Image::black, frames, f, transparency_shadow_size, opacity);
        }
        break;
    }

#if 0
    string out2("/tmp/xu4/");
    unscaled->save(out2.append(name).append("-fixup.ppm").c_str());
#endif

#ifdef USE_GL
    info->image = unscaled;
    info->tex = gpu_makeTexture(info->image);
#else
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
        errorFatal("image %s is prescaled to an incompatible size: %d\n"
            "Resetting the scale to %d. Sorry about the inconvenience, please restart.",
            xu4.config->confString(info->filename), orig_scale,
            xu4.settings->scale);
    }
    imageScale /= info->prescale;

    info->image = screenScale(unscaled, imageScale, info->tiles, 1);
    delete unscaled;

#if 0
    string out3("/tmp/xu4/");
    info->image->save(out3.append(name).append("-scale.ppm").c_str());
#endif
#endif

    return info;
}

/**
 * Returns information for the given image set.
 */
const SubImage* ImageMgr::getSubImage(Symbol name, ImageInfo** infoPtr) {
    std::map<Symbol, ImageInfo *>::iterator it;
    ImageSet *set = baseSet;

    while (set != NULL) {
        foreach (it, set->info) {
            ImageInfo *info = (ImageInfo *) it->second;
            std::map<Symbol, int>::iterator j = info->subImageIndex.find(name);
            if (j != info->subImageIndex.end()) {
                *infoPtr = info;
                return info->subImages + j->second;
            }
        }
        set = scheme(set->extends);
    }

    return NULL;
}

/**
 * Set the group loaded images will belong to.
 * Return the previously set group.
 */
uint16_t ImageMgr::setResourceGroup(uint16_t group) {
    uint16_t prev = resGroup;
    resGroup = group;
    return prev;
}

/**
 * Free all images that are part of the specified group.
 */
void ImageMgr::freeResourceGroup(uint16_t group) {
    std::map<Symbol, ImageSet *>::iterator si;
    std::map<Symbol, ImageInfo *>::iterator j;

    foreach (si, imageSets) {
        foreach (j, si->second->info) {
            ImageInfo *info = j->second;
            if (info->image && (info->resGroup == group)) {
                //dprint("ImageMgr::freeRes %s\n", info->filename.c_str());
#ifdef USE_GL
                gpu_freeTexture(info->tex);
                info->tex = 0;
#endif
                delete info->image;
                info->image = NULL;
            }
        }
    }
}

/**
 * Get the 256 color VGA palette from the u4upgrad file.
 */
const RGBA* ImageMgr::vgaPalette() {
    if (vgaColors == NULL) {
        U4FILE *pal = u4fopen("u4vga.pal");
        if (!pal)
            return NULL;

        vgaColors = new RGBA[256];

        for (int i = 0; i < 256; i++) {
            vgaColors[i].r = u4fgetc(pal) * 255 / 63;
            vgaColors[i].g = u4fgetc(pal) * 255 / 63;
            vgaColors[i].b = u4fgetc(pal) * 255 / 63;
            vgaColors[i].a = 255;
        }
        u4fclose(pal);
    }
    return vgaColors;
}

/**
 * Find the new base image set when settings have changed.
 */
void ImageMgr::update(Settings *newSettings) {
    string setname = newSettings->videoType;
    TRACE(*logger, string("base image set is '") + setname + string("'"));
    baseSet = scheme( xu4.config->intern(setname.c_str()) );
}

ImageSet::~ImageSet() {
    std::map<Symbol, ImageInfo *>::iterator it;
    foreach (it, info)
        delete it->second;
}

ImageInfo::ImageInfo() {
#ifdef USE_GL
    tex = 0;
    tileTexCoord = NULL;
#endif
    subImageCount = 0;
    subImages = NULL;
}

ImageInfo::~ImageInfo() {
    delete[] subImages;
    delete image;
#ifdef USE_GL
    if (tex)
        gpu_freeTexture(tex);
    delete[] tileTexCoord;
#endif
}

string ImageInfo::getFilename() const {
    return xu4.config->confString(filename);
}
