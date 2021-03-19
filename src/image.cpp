/*
 * $Id$
 */

#include <assert.h>
#include <memory>
#include <list>
#include <utility>
#include "debug.h"
#include "image.h"
#include "settings.h"
#include "error.h"

//#define REPORT_PAL

Image::Image() {}

/**
 * Creates a new image in video card memory.
 * Scale is stored to allow drawing using U4 (320x200) coordinates, regardless
 * of the actual image scale.
 * Indexed is true for palette based images, or false for RGB images.
 */
Image *Image::create(int w, int h, bool indexed) {
    Image *im = new Image;
    im->pixels = new uint32_t[w * h];
    if (!im->pixels) {
        delete im;
        return NULL;
    }
    im->w = w;
    im->h = h;
    return im;
}

/**
 * Creates a new image in main system memory.
 */
Image *Image::createMem(int w, int h, bool indexed) {
    return create(w, h, indexed);
}

// Third copy of screen image pointer.  TODO: Consolidate these.
Image* screenImage = NULL;

/**
 * Create a special purpose image that represents the whole screen.
 *
 * NOTE: The returned pointer is unfortunately stored in two places,
 * View::screen & imageMgr->get("screen")->image (which gets the
 * ImageInfo from ImageMgr::baseSet).  It also needs to be accessed by the
 * Image::draw*On(NULL, ...) methods.
 *
 * On iOS it's even more complicated as those pointers are written over by
 * pushU4View & popU4View in U4AppDelegate.mm.
 */
Image *Image::createScreenImage() {
    return screenImage = create(320 * settings.scale,
                                200 * settings.scale);
}

/**
 * Creates a duplicate of another image
 */
Image *Image::duplicate(const Image *image) {
    Image *im = create(image->w, image->h);
    if (im) {
        image->drawOn(im, 0, 0);
        im->backgroundColor = image->backgroundColor;
    }
    return im;
}

/**
 * Frees the image.
 */
Image::~Image() {
    delete[] pixels;
}

/**
 * Sets the palette
 */
void Image::setPalette(const RGBA *colors, unsigned n_colors) {
#ifdef REPORT_PAL
    printf( "KR setPalette %d\n", n_colors );
#endif
#if 0
    ASSERT(indexed, "imageSetPalette called on non-indexed image");

    SDL_Color *sdlcolors = new SDL_Color[n_colors];
    for (unsigned i = 0; i < n_colors; i++) {
        sdlcolors[i].r = colors[i].r;
        sdlcolors[i].g = colors[i].g;
        sdlcolors[i].b = colors[i].b;
    }

    SDL_SetColors(CSURF, sdlcolors, 0, n_colors);

    delete [] sdlcolors;
#endif
}

/**
 * Copies the palette from another image.
 */
void Image::setPaletteFromImage(const Image *src) {
#ifdef REPORT_PAL
    printf("KR setPaletteFromImage\n");
#endif
#if 0
    ASSERT(indexed && src->indexed, "imageSetPaletteFromImage called on non-indexed image");
    const SDL_PixelFormat* df = CSURF->format;
    const SDL_PixelFormat* sf = ((SDL_Surface*) src->surface)->format;
    memcpy(df->palette->colors, sf->palette->colors,
           sizeof(SDL_Color) * sf->palette->ncolors);
#endif
}

// returns the color of the specified palette index
RGBA Image::getPaletteColor(int index) {
    RGBA color = RGBA(0, 0, 0, 0);
#if 0
    if (indexed)
    {
        const SDL_PixelFormat* pf = CSURF->format;
        color.r = pf->palette->colors[index].r;
        color.g = pf->palette->colors[index].g;
        color.b = pf->palette->colors[index].b;
        color.a = IM_OPAQUE;
    }
#endif
    return color;
}

/* returns the palette index of the specified RGB color */
int Image::getPaletteIndex(RGBA color) {
#if 0
    if (!indexed)
        return -1;

    const SDL_PixelFormat* pf = CSURF->format;
    for (int i = 0; i < pf->palette->ncolors; i++)
    {
        if ( (pf->palette->colors[i].r == color.r)
          && (pf->palette->colors[i].g == color.g)
          && (pf->palette->colors[i].b == color.b) )
        {
            return i;
        }

    }
#endif
    // return the proper palette index for the specified color
    return -1;
}

RGBA Image::setColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return RGBA(r, g, b, a);
}

/* sets the specified font colors */
bool Image::setFontColor(ColorFG fg, ColorBG bg) {
    if (!setFontColorFG(fg)) return false;
    if (!setFontColorBG(bg)) return false;
    return true;
}

/* sets the specified font colors */
bool Image::setFontColorFG(ColorFG fg) {
    switch (fg) {
        case FG_GREY:
            if (!setPaletteIndex(TEXT_FG_PRIMARY_INDEX,   setColor(153,153,153))) return false;
            if (!setPaletteIndex(TEXT_FG_SECONDARY_INDEX, setColor(102,102,102))) return false;
            if (!setPaletteIndex(TEXT_FG_SHADOW_INDEX,    setColor(51,51,51))) return false;
            break;
        case FG_BLUE:
            if (!setPaletteIndex(TEXT_FG_PRIMARY_INDEX,   setColor(102,102,255))) return false;
            if (!setPaletteIndex(TEXT_FG_SECONDARY_INDEX, setColor(51,51,204))) return false;
            if (!setPaletteIndex(TEXT_FG_SHADOW_INDEX,    setColor(51,51,51))) return false;
            break;
        case FG_PURPLE:
            if (!setPaletteIndex(TEXT_FG_PRIMARY_INDEX,   setColor(255,102,255))) return false;
            if (!setPaletteIndex(TEXT_FG_SECONDARY_INDEX, setColor(204,51,204))) return false;
            if (!setPaletteIndex(TEXT_FG_SHADOW_INDEX,    setColor(51,51,51))) return false;
            break;
        case FG_GREEN:
            if (!setPaletteIndex(TEXT_FG_PRIMARY_INDEX,   setColor(102,255,102))) return false;
            if (!setPaletteIndex(TEXT_FG_SECONDARY_INDEX, setColor(0,153,0))) return false;
            if (!setPaletteIndex(TEXT_FG_SHADOW_INDEX,    setColor(51,51,51))) return false;
            break;
        case FG_RED:
            if (!setPaletteIndex(TEXT_FG_PRIMARY_INDEX,   setColor(255,102,102))) return false;
            if (!setPaletteIndex(TEXT_FG_SECONDARY_INDEX, setColor(204,51,51))) return false;
            if (!setPaletteIndex(TEXT_FG_SHADOW_INDEX,    setColor(51,51,51))) return false;
            break;
        case FG_YELLOW:
            if (!setPaletteIndex(TEXT_FG_PRIMARY_INDEX,   setColor(255,255,51))) return false;
            if (!setPaletteIndex(TEXT_FG_SECONDARY_INDEX, setColor(204,153,51))) return false;
            if (!setPaletteIndex(TEXT_FG_SHADOW_INDEX,    setColor(51,51,51))) return false;
            break;
        default:
            if (!setPaletteIndex(TEXT_FG_PRIMARY_INDEX,   setColor(255,255,255))) return false;
            if (!setPaletteIndex(TEXT_FG_SECONDARY_INDEX, setColor(204,204,204))) return false;
            if (!setPaletteIndex(TEXT_FG_SHADOW_INDEX,    setColor(68,68,68))) return false;
    }
    return true;
}

/* sets the specified font colors */
bool Image::setFontColorBG(ColorBG bg) {
    switch (bg) {
        case BG_BRIGHT:
            if (!setPaletteIndex(TEXT_BG_INDEX, setColor(0,0,102)))
                return false;
            break;
        default:
            if (!setPaletteIndex(TEXT_BG_INDEX, setColor(0,0,0)))
                return false;
    }
    return true;
}

/* sets the specified palette index to the specified RGB color */
bool Image::setPaletteIndex(unsigned int index, RGBA color) {
#ifdef REPORT_PAL
    printf( "KR setPaletteIndex\n" );
#endif
#if 0
    if (!indexed)
        return false;

    const SDL_PixelFormat* pf = CSURF->format;
    pf->palette->colors[index].r = color.r;
    pf->palette->colors[index].g = color.g;
    pf->palette->colors[index].b = color.b;

    // success
    return true;
#else
    return false;
#endif
}

bool Image::getTransparentIndex(unsigned int &index) const {
#ifdef REPORT_PAL
    printf( "KR getTransparentIndex\n" );
#endif
#if 0
    if (!indexed || (CSURF->flags & SDL_SRCCOLORKEY) == 0)
        return false;

    index = CSURF->format->colorkey;
    return true;
#else
    return false;
#endif
}

void Image::initializeToBackgroundColor(RGBA bgColor)
{
    backgroundColor = bgColor;
    fillRect(0, 0, w, h, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
}

bool Image::isAlphaOn() const
{
#if 0
    return CSURF->flags & SDL_SRCALPHA;
#else
#ifdef REPORT_PAL
    printf( "KR isAlphaOn\n" );
#endif
    return false;
#endif
}

void Image::alphaOn()
{
#ifdef REPORT_PAL
    printf( "KR alphaOn\n" );
#endif
    //CSURF->flags |= SDL_SRCALPHA;
}

void Image::alphaOff()
{
#ifdef REPORT_PAL
    printf( "KR alphaOff\n" );
#endif
    //CSURF->flags &= ~SDL_SRCALPHA;
}

void Image::putPixel(int x, int y, int r, int g, int b, int a) {
    RGBA col(r, g, b, a);
    pixels[ y*w + x ] = *((uint32_t*) &col);
}

void Image::makeBackgroundColorTransparent(int haloSize, int shadowOpacity)
{
    uint32_t icol = *((uint32_t*) &backgroundColor);
    performTransparencyHack(icol, 1, 0, haloSize,shadowOpacity);
}

//TODO Separate functionalities found in here
void Image::performTransparencyHack(unsigned int colorValue, unsigned int numFrames, unsigned int currentFrameIndex, unsigned int haloWidth, unsigned int haloOpacityIncrementByPixelDistance)
{
    std::list<std::pair<unsigned int,unsigned int> > opaqueXYs;
    unsigned int x, y;
    uint8_t t_r, t_g, t_b;

    {
    RGBA* col = (RGBA*) &colorValue;
    t_r = col->r;
    t_g = col->g;
    t_b = col->b;
    }

    unsigned int frameHeight = h / numFrames;
    //Min'd so that they never go out of range (>=h)
    unsigned int top = std::min(h, currentFrameIndex * frameHeight);
    unsigned int bottom = std::min(h, top + frameHeight);

    for (y = top; y < bottom; y++) {

        for (x = 0; x < w; x++) {
            unsigned int r, g, b, a;
            getPixel(x, y, r, g, b, a);
            if (r == t_r &&
                g == t_g &&
                b == t_b) {
                putPixel(x, y, r, g, b, IM_TRANSPARENT);
            } else {
                putPixel(x, y, r, g, b, a);
                if (haloWidth)
                    opaqueXYs.push_back(std::pair<int,int>(x,y));
            }
        }
    }
    int ox, oy;
    for (std::list<std::pair<unsigned int,unsigned int> >::iterator xy = opaqueXYs.begin();
            xy != opaqueXYs.end();
            ++xy)
    {
        ox = xy->first;
        oy = xy->second;
        int span = int(haloWidth);
        unsigned int x_start = std::max(0,ox - span);
        unsigned int x_finish = std::min(int(w), ox + span + 1);
        for (x = x_start; x < x_finish; ++x)
        {
            unsigned int y_start = std::max(int(top),oy - span);
            unsigned int y_finish = std::min(int(bottom), oy + span + 1);
            for (y = y_start; y < y_finish; ++y) {

                int divisor = 1 + span * 2 - abs(int(ox - x)) - abs(int(oy - y));

                unsigned int r, g, b, a;
                getPixel(x, y, r, g, b, a);
                if (a != IM_OPAQUE) {
                    putPixel(x, y, r, g, b, std::min((unsigned int)IM_OPAQUE, a + haloOpacityIncrementByPixelDistance / divisor));
                }
            }
        }
    }
}

void Image::setTransparentIndex(unsigned int index)//, unsigned int numFrames, unsigned int currentFrameIndex, int shadowOutlineWidth, int shadowOpacityOverride)
{
#if 0
    if (indexed) {
        SDL_SetColorKey(CSURF, SDL_SRCCOLORKEY, index);
    } else {
        //errorWarning("Setting transparent index for non indexed");
    }
#endif
}

/**
 * Sets the palette index of a single pixel.  If the image is in
 * indexed mode, then the index is simply the palette entry number.
 * If the image is RGB, it is a packed RGB triplet.
 */
void Image::putPixelIndex(int x, int y, uint32_t index) {
    pixels[ y*w + x ] = index;
}

/**
 * Fills a rectangle in the image with a given color.
 */
void Image::fillRect(int x, int y, int rw, int rh, int r, int g, int b, int a) {
    RGBA col(r, g, b, a);
    uint32_t icol = *((uint32_t*) &col);
    uint32_t* dp;
    uint32_t* dend;
    uint32_t* drow = pixels + w * y + x;
    int blitW, blitH;

    blitW = rw;
    if ((blitW + x) > int(w))
        blitW = w - x;
    if (blitW < 1)
        return;

    blitH = rh;
    if ((blitH + y) > int(h))
        blitH = h - y;
    if (blitH < 1)
        return;

    while (blitH--) {
        dp = drow;
        dend = dp + blitW;
        while( dp != dend )
            *dp++ = icol;
        drow += w;
    }
}

/**
 * Gets the color of a single pixel.
 */
void Image::getPixel(int x, int y, unsigned int &r, unsigned int &g, unsigned int &b, unsigned int &a) const {
    const RGBA* col = (RGBA*) (pixels + y*w + x);
    r = col->r;
    g = col->g;
    b = col->b;
    a = col->a;
}

void Image::getPixel(int x, int y, RGBA &col) const {
    col = *((RGBA*) (pixels + y*w + x));
}

/**
 * Gets the palette index of a single pixel.  If the image is in
 * indexed mode, then the index is simply the palette entry number.
 * If the image is RGB, it is a packed RGB triplet.
 */
void Image::getPixelIndex(int x, int y, unsigned int &index) const {
    index = pixels[ y*w + x ];
}

/**
 * Draws the image onto another image.
 */
void Image::drawOn(Image *dest, int x, int y) const {
    uint32_t* dp;
    uint32_t* drow;
    const uint32_t* sp;
    const uint32_t* send;
    const uint32_t* srow = pixels;
    int blitW, blitH;

    if (dest == NULL)
        dest = screenImage;

    drow = dest->pixels + dest->w * y + x;

    blitW = w;
    if (x < 0) {
        srow += -x;
        blitW += x;     // Subtracts from blitW.
    }
    else if ((blitW + x) > int(dest->w)) {
        blitW = dest->w - x;
    }
    if (blitW < 1)
        return;

    blitH = h;
    if (y < 0) {
        srow += w*-y;
        blitH += y;     // Subtracts from blitH.
    }
    else if ((blitH + y) > int(dest->h)) {
        blitH = dest->h - y;
    }
    if (blitH < 1)
        return;

    while (blitH--) {
        dp = drow;
        sp = srow;
        send = sp + blitW;
        while( sp != send )
            *dp++ = *sp++;
        drow += dest->w;
        srow += w;
    }
}

#define CLIP_SUB(x, rx, rw, DD) \
    if (rx < 0) { \
        x -= rx; \
        rw += rx; \
        rx = 0; \
    } \
    if (x < 0) { \
        rx -= x; \
        rw += x; \
        x = 0; \
    } \
    if ((rw + x) > int(DD)) \
        rw = DD - x; \
    if (rw < 1) \
        return;

/**
 * Draws a piece of the image onto another image.
 */
void Image::drawSubRectOn(Image *dest, int x, int y, int rx, int ry, int rw, int rh) const {
    uint32_t* dp;
    uint32_t* drow;
    const uint32_t* sp;
    const uint32_t* send;
    const uint32_t* srow;

    if (dest == NULL)
        dest = screenImage;

    // Clip position and source rect to positive values.
    CLIP_SUB(x, rx, rw, dest->w)
    CLIP_SUB(y, ry, rh, dest->h)

    srow = pixels + w * ry + rx;
    drow = dest->pixels + dest->w * y + x;

    while (rh--) {
        dp = drow;
        sp = srow;
        send = sp + rw;
        while( sp != send )
            *dp++ = *sp++;
        drow += dest->w;
        srow += w;
    }
}

/**
 * Draws a piece of the image flipped vertically onto another image.
 */
void Image::drawSubRectInvertedOn(Image *dest, int x, int y, int rx, int ry, int rw, int rh) const {
    uint32_t* dp;
    uint32_t* drow;
    const uint32_t* sp;
    const uint32_t* send;
    const uint32_t* srow;

    if (dest == NULL)
        dest = screenImage;

    // Clip position and source rect to positive values.
    CLIP_SUB(x, rx, rw, dest->w)
    CLIP_SUB(y, ry, rh, dest->h)

    srow = pixels + w * ry + rx;
    drow = dest->pixels + dest->w * y + x;

    srow += w * (rh - 1);
    while (rh--) {
        dp = drow;
        sp = srow;
        send = sp + rw;
        while( sp != send )
            *dp++ = *sp++;
        drow += dest->w;
        srow -= w;
    }
}

/**
 * Dumps the image to a file in PPM format. This is mainly used for debugging.
 */
void Image::save(const string &filename) {
#if 0
    SDL_SaveBMP(CSURF, filename.c_str());
#else
    uint8_t* row;
    uint8_t* rowEnd;
    uint8_t* cp;
    int rowBytes = w*4;
    FILE* fp;

    // TODO: Need to save in a format that supports the alpha channel.

    fp = fopen(filename.c_str(), "w");
    if (! fp) {
        fprintf(stderr, "Image::save cannot open file %s\n", filename.c_str());
        return;
    }
    fprintf(fp, "P6 %d %d 255\n", w, h);

    row = (uint8_t*) pixels;
    for (unsigned y = 0; y < h; ++y) {
        cp = row;
        rowEnd = row + rowBytes;
        while (cp != rowEnd) {
            fwrite(cp, 1, 3, fp);
            cp += 4;
        }
        row += rowBytes;
    }
    fclose(fp);
#endif
}


/**
 * Invert the RGB values of image.
 */
void Image::drawHighlighted() {
    RGBA* col = (RGBA*) pixels;
    RGBA* end = col + w*h;
    while (col != end) {
        col->r = 0xff - col->r;
        col->g = 0xff - col->g;
        col->b = 0xff - col->b;
        ++col;
    }
}
