/*
 * $Id$
 */

#ifndef IMAGEMGR_H
#define IMAGEMGR_H

#include <map>
#include <string>
#include <vector>

#include "image.h"
#include "observer.h"

class ConfigElement;
class Debug;
class ImageSet;
class Settings;

/*
 * The image manager is responsible for loading and keeping track of
 * the various images.
 */

#define BKGD_SHAPES "tiles"
#define BKGD_CHARSET "charset"
#define BKGD_BORDERS "borders"
#define BKGD_INTRO "title"
#define BKGD_OPTIONS_TOP "options_top"
#define BKGD_OPTIONS_BTM "options_btm"
#define BKGD_TREE "tree"
#define BKGD_PORTAL "portal"
#define BKGD_OUTSIDE "outside"
#define BKGD_INSIDE "inside"
#define BKGD_WAGON "wagon"
#define BKGD_GYPSY "gypsy"
#define BKGD_ABACUS "abacus"
#define BKGD_HONCOM "honcom"
#define BKGD_VALJUS "valjus"
#define BKGD_SACHONOR "sachonor"
#define BKGD_SPIRHUM "spirhum"
#define BKGD_ANIMATE "beasties"
#define BKGD_KEY "key"
#define BKGD_HONESTY "honesty"
#define BKGD_COMPASSN "compassn"
#define BKGD_VALOR "valor"
#define BKGD_JUSTICE "justice"
#define BKGD_SACRIFIC "sacrific"
#define BKGD_HONOR "honor"
#define BKGD_SPIRIT "spirit"
#define BKGD_HUMILITY "humility"
#define BKGD_TRUTH "truth"
#define BKGD_LOVE "love"
#define BKGD_COURAGE "courage"
#define BKGD_STONCRCL "stoncrcl"
#define BKGD_RUNE_INF "rune0"
#define BKGD_SHRINE_HON "rune1"
#define BKGD_SHRINE_COM "rune2"
#define BKGD_SHRINE_VAL "rune3"
#define BKGD_SHRINE_JUS "rune4"
#define BKGD_SHRINE_SAC "rune5"
#define BKGD_SHRINE_HNR "rune6"
#define BKGD_SHRINE_SPI "rune7"
#define BKGD_SHRINE_HUM "rune8"
#define BKGD_GEMTILES "gemtiles"

enum ImageFixup {
    FIXUP_NONE,
    FIXUP_INTRO,
/*
 * @VERIFY: no longer being used?
 *
 *    FIXUP_INTRO_EXTENDED,
 */
    FIXUP_ABYSS,
    FIXUP_ABACUS,
    FIXUP_DUNGNS
};

/**
 * Image meta info.
 */
class ImageInfo {
public:
    ~ImageInfo();

    std::string name;
    std::string filename;
    int width, height, depth;
    int prescale;
    std::string filetype;
    int tiles;                  /**< used to scale the without bleeding colors between adjacent tiles */
    bool introOnly;             /**< whether can be freed after the intro */
    int transparentIndex;       /**< color index to consider transparent */
    bool xu4Graphic;            /**< an original xu4 graphic not part of u4dos or the VGA upgrade */
    ImageFixup fixup;           /**< a routine to do miscellaneous fixes to the image */
    Image *image;               /**< the image we're describing */
    std::map<std::string, SubImage *> subImages;
};

/**
 * The image manager singleton that keeps track of all the images.
 */
class ImageMgr : Observer<Settings *> {
public:
    static ImageMgr *getInstance();
    static void destroy();

    ImageInfo *get(const std::string &name);
    SubImage *ImageMgr::getSubImage(const std::string &name);
    void freeIntroBackgrounds();
    const std::vector<std::string> &getSetNames();

private:
    ImageMgr();
    ~ImageMgr();
    void init();

    ImageSet *loadImageSetFromConf(const ConfigElement &conf);
    ImageInfo *loadImageInfoFromConf(const ConfigElement &conf);
    SubImage *loadSubImageFromConf(const ImageInfo *info, const ConfigElement &conf);

    ImageSet *getSet(const std::string &setname);
    ImageInfo *getInfo(const std::string &name);
    ImageInfo *getInfoFromSet(const string &name, ImageSet *set);

    void fixupIntro(Image *im, int prescale);
/*
 * @VERIFY: no longer being used?
 *
 *    void fixupIntroExtended(Image *im, int prescale);
 */
    void fixupAbyssVision(Image *im, int prescale);
    void fixupAbacus(Image *im, int prescale);
    void fixupDungNS(Image *im, int prescale);

    void update(Settings *newSettings);

    static ImageMgr *instance;
    std::map<std::string, ImageSet *> imageSets;
    std::vector<std::string> imageSetNames;
    ImageSet *baseSet;

    Debug *logger;
};

#define imageMgr (ImageMgr::getInstance())

#endif /* IMAGEMGR_H */
