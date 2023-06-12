/*
 * imagemgr.h
 */

#ifndef IMAGEMGR_H
#define IMAGEMGR_H

#include <map>
#include <string>

#include "config.h"
#include "image.h"
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
#define BKGD_RUNE_INF       ImageMgr::sym.infinity
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
    Symbol infinity;

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

enum AtlasEditOpcode {
    AEDIT_NOP,
    AEDIT_BRUSH,
    AEDIT_RECT,
    AEDIT_OP_COUNT
};

struct AtlasSubImage {
    Symbol name;        // Image name or AtlasEditOpcode.
    int16_t x, y, w, h;
};

struct SubImage {
    Symbol name;
    int16_t x, y, width, height;
    uint16_t celCount;
};

enum ImageFixup {
    FIXUP_NONE,
    FIXUP_INTRO,
    FIXUP_ABYSS,
    FIXUP_ABACUS,
    FIXUP_DUNGNS,
    FIXUP_TRANSPARENT0,
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
    uint8_t filetype;
    uint8_t fixup;              /**< a routine to do miscellaneous fixes to the image */
    Image *image;               /**< the image we're describing */
    uint32_t tex;               /**< OpenGL texture name */
    const float* tileTexCoord;  /**< Indexed by VisualId */
    const SubImage* subImages;
    std::map<Symbol, int> subImageIndex;
};

class ImageSet {
public:
    ~ImageSet();

    std::map<Symbol, ImageInfo *> info;
};

/**
 * The image manager singleton that keeps track of all the images.
 */
class ImageMgr {
public:
    static ImageSymbols sym;

    ImageMgr();
    ~ImageMgr();

    ImageInfo* imageInfo(Symbol name, const SubImage** subPtr);
    ImageInfo* get(Symbol name);

    void freeResourceGroup(uint16_t group);

    const RGBA* vgaPalette();
    const RGBA* greyPalette();
    bool usingVGA() const { return vgaGraphics; }

private:
    static void notice(int, void*, void*);
    const SubImage* getSubImage(Symbol name, ImageInfo** infoPtr);
    ImageInfo* load(ImageInfo* info);
    U4FILE * getImageFile(ImageInfo *info);

    void fixupIntro(Image *im);
    void fixupAbyssVision(Image32*);
    void fixupTransparent(Image*, RGBA color);
    void fixupAbacus(Image *im);
    void fixupDungNS(Image *im);
    void fixupFMTowns(Image *im);

    ImageSet *baseSet;
    RGBA* vgaColors;
    RGBA* greyColors;
    uint8_t* visionBuf;
    bool vgaGraphics;
};

#endif /* IMAGEMGR_H */
