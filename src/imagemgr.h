/*
 * imagemgr.h
 */

#ifndef IMAGEMGR_H
#define IMAGEMGR_H

#include <map>
#include <string>

#include "config.h"
#include "image.h"
#include "observer.h"
#include "u4file.h"

#define errorLoadImage(Sym) \
    errorFatal("Unable to load image \"%s\"", xu4.config->symbolName(Sym));

/*
 * The image manager is responsible for loading and keeping track of
 * the various images.
 */

#define BKGD_SHAPES         ImageMgr::sym.tiles
#define BKGD_CHARSET        ImageMgr::sym.charset
#define BKGD_BORDERS        ImageMgr::sym.borders
#define BKGD_INTRO          ImageMgr::sym.title
#define BKGD_OPTIONS_TOP    ImageMgr::sym.options_top
#define BKGD_OPTIONS_BTM    ImageMgr::sym.options_btm
#define BKGD_TREE           ImageMgr::sym.tree
#define BKGD_PORTAL         ImageMgr::sym.portal
#define BKGD_OUTSIDE        ImageMgr::sym.outside
#define BKGD_INSIDE         ImageMgr::sym.inside
#define BKGD_WAGON          ImageMgr::sym.wagon
#define BKGD_GYPSY          ImageMgr::sym.gypsy
#define BKGD_ABACUS         ImageMgr::sym.abacus
#define BKGD_HONCOM         ImageMgr::sym.honcom
#define BKGD_VALJUS         ImageMgr::sym.valjus
#define BKGD_SACHONOR       ImageMgr::sym.sachonor
#define BKGD_SPIRHUM        ImageMgr::sym.spirhum
#define BKGD_ANIMATE        ImageMgr::sym.beasties
#define BKGD_KEY            ImageMgr::sym.key
#define BKGD_HONESTY        ImageMgr::sym.honesty
#define BKGD_COMPASSN       ImageMgr::sym.compassn
#define BKGD_VALOR          ImageMgr::sym.valor
#define BKGD_JUSTICE        ImageMgr::sym.justice
#define BKGD_SACRIFIC       ImageMgr::sym.sacrific
#define BKGD_HONOR          ImageMgr::sym.honor
#define BKGD_SPIRIT         ImageMgr::sym.spirit
#define BKGD_HUMILITY       ImageMgr::sym.humility
#define BKGD_TRUTH          ImageMgr::sym.truth
#define BKGD_LOVE           ImageMgr::sym.love
#define BKGD_COURAGE        ImageMgr::sym.courage
#define BKGD_STONCRCL       ImageMgr::sym.stoncrcl
#define BKGD_RUNE_INF       ImageMgr::sym.rune0
#define BKGD_SHRINE_HON     ImageMgr::sym.rune1
#define BKGD_SHRINE_COM     ImageMgr::sym.rune2
#define BKGD_SHRINE_VAL     ImageMgr::sym.rune3
#define BKGD_SHRINE_JUS     ImageMgr::sym.rune4
#define BKGD_SHRINE_SAC     ImageMgr::sym.rune5
#define BKGD_SHRINE_HNR     ImageMgr::sym.rune6
#define BKGD_SHRINE_SPI     ImageMgr::sym.rune7
#define BKGD_SHRINE_HUM     ImageMgr::sym.rune8
#define BKGD_GEMTILES       ImageMgr::sym.gemtiles
#define IMG_MOONGATE        ImageMgr::sym.moongate
#define IMG_ITEMS           ImageMgr::sym.items
#define IMG_BLACKBEAD       ImageMgr::sym.blackbead
#define IMG_WHITEBEAD       ImageMgr::sym.whitebead

struct ImageSymbols {
    Symbol tiles;
    Symbol charset;
    Symbol borders;
    Symbol title;
    Symbol options_top;
    Symbol options_btm;
    Symbol tree;
    Symbol portal;
    Symbol outside;
    Symbol inside;
    Symbol wagon;
    Symbol gypsy;
    Symbol abacus;
    Symbol honcom;
    Symbol valjus;
    Symbol sachonor;
    Symbol spirhum;
    Symbol beasties;
    Symbol key;

    // These 8 are ordered to match enum Virtue (savegame.h)
    Symbol honesty;
    Symbol compassn;
    Symbol valor;
    Symbol justice;
    Symbol sacrific;
    Symbol honor;
    Symbol spirit;
    Symbol humility;

    // codexHandleVirtues() expects these BaseVirtue to follow Virtue.
    Symbol truth;
    Symbol love;
    Symbol courage;

    Symbol stoncrcl;
    Symbol rune0;

    // These 8 are ordered to match visionImageNames (shrine.cpp)
    Symbol rune1;
    Symbol rune2;
    Symbol rune3;
    Symbol rune4;
    Symbol rune5;
    Symbol rune6;
    Symbol rune7;
    Symbol rune8;

    Symbol gemtiles;
    Symbol moongate;
    Symbol items;
    Symbol blackbead;
    Symbol whitebead;
};

struct SubImage {
    Symbol name;
    Symbol srcImageName;
    int16_t x, y, width, height;
};

enum ImageFixup {
    FIXUP_NONE,
    FIXUP_INTRO,
    FIXUP_ABYSS,
    FIXUP_ABACUS,
    FIXUP_DUNGNS,
    FIXUP_BLACKTRANSPARENCYHACK,
    FIXUP_FMTOWNSSCREEN
};

/**
 * Image meta info.
 */
class ImageInfo {
public:
    ImageInfo();
    ~ImageInfo();

    std::string getFilename() const;

    StringId filename;
    Symbol name;
    uint16_t resGroup;          /**< resource group */
    uint16_t tiles;             /**< used to scale the without bleeding colors between adjacent tiles */
    int16_t width, height;
    int16_t subImageCount;
    uint8_t depth;
    uint8_t prescale;
    uint8_t filetype;
    uint8_t transparentIndex;   /**< color index to consider transparent */
    uint8_t fixup;              /**< a routine to do miscellaneous fixes to the image */
    Image *image;               /**< the image we're describing */
#ifdef USE_GL
    uint32_t tex;               /**< OpenGL texture name */
    const float* tileTexCoord;  /**< Indexed by VisualId */
#endif
    const SubImage* subImages;
    std::map<Symbol, int> subImageIndex;
};

class Debug;
class Settings;

class ImageSet {
public:
    ~ImageSet();

    Symbol name;
    Symbol extends;
    std::map<Symbol, ImageInfo *> info;
};

/**
 * The image manager singleton that keeps track of all the images.
 */
class ImageMgr : Observer<Settings *> {
public:
    static ImageSymbols sym;

    ImageMgr();
    ~ImageMgr();

    ImageInfo *get(Symbol name, bool returnUnscaled=false);
    const SubImage* getSubImage(Symbol name);

    uint16_t setResourceGroup(uint16_t group);
    void freeResourceGroup(uint16_t group);

    const RGBA* vgaPalette();

private:
    U4FILE * getImageFile(ImageInfo *info);
    ImageSet* scheme(Symbol setname);
    ImageInfo* getInfoFromSet(Symbol name, ImageSet *set);

    void fixupIntro(Image *im, int prescale);
    void fixupAbyssVision(Image *im, int prescale);
    void fixupAbacus(Image *im, int prescale);
    void fixupDungNS(Image *im, int prescale);
    void fixupFMTowns(Image *im, int prescale);

    void update(Settings *newSettings);

    std::map<Symbol, ImageSet *> imageSets;
    ImageSet *baseSet;
    RGBA* vgaColors;
    Debug *logger;
    uint16_t resGroup;
};

#endif /* IMAGEMGR_H */
