/*
 * image.cpp
 */

#include <string.h>
#include <assert.h>
#include <memory>
#include <list>
#include "image.h"
#include "settings.h"
#include "xu4.h"

#include "support/image32.c"

#define CHANNEL_REMAP
#ifdef CHANNEL_REMAP
extern bool screenFormatIsABGR;

static void screenColor(RGBA* col, int r, int g, int b, int a) {
    if (screenFormatIsABGR) {
        col->r = r;
        col->b = b;
    } else {
        col->r = b;
        col->b = r;
    }
    col->g = g;
    col->a = a;
}
#endif

RGBA Image::black = {0, 0, 0, 255};
int Image::blending = 0;

/**
 * Enable blending (use alpha channel) for drawOn & drawSubRectOn.
 * Returns previous blending state.
 */
int Image::enableBlend(int on) {
    int prev = blending;
    blending = on ? 1 : 0;
    return prev;
}

/**
 * Creates a new RGBA image.
 */
Image *Image::create(int w, int h) {
    Image *im = new Image;
    image32_allocPixels(im, w, h);
    return im;
}

/**
 * Creates a duplicate of another image
 */
Image *Image::duplicate(const Image *image) {
    Image *im = new Image;
    image32_duplicatePixels(im, image);
    return im;
}

/**
 * Frees the image.
 */
Image::~Image() {
    image32_freePixels(this);
}

RGBA Image::setColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    RGBA color;
    rgba_set(color, r, g, b, a);
    return color;
}

/* sets the specified font colors */
bool Image::setFontColorFG(ColorFG fg) {
#if 0
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
#endif
    return true;
}

/* sets the specified font colors */
bool Image::setFontColorBG(ColorBG bg) {
#if 0
    switch (bg) {
        case BG_BRIGHT:
            if (!setPaletteIndex(TEXT_BG_INDEX, setColor(0,0,102)))
                return false;
            break;
        default:
            if (!setPaletteIndex(TEXT_BG_INDEX, setColor(0,0,0)))
                return false;
    }
#endif
    return true;
}

void Image::putPixel(int x, int y, int r, int g, int b, int a) {
    RGBA col;
    rgba_set(col, r, g, b, a);
    pixels[ y*w + x ] = *((uint32_t*) &col);
}

void Image::makeColorTransparent(const RGBA& bgColor, int haloSize, int shadowOpacity)
{
    performTransparencyHack(bgColor, 1, 0, haloSize,shadowOpacity);
}

void Image::performTransparencyHack(const RGBA& transColor,
        unsigned int numFrames,
        unsigned int currentFrameIndex,
        unsigned int haloWidth,
        unsigned int haloOpacityIncrementByPixelDistance)
{
    std::list<std::pair<unsigned int,unsigned int> > opaqueXYs;
    RGBA* cp;
    unsigned int x, y;
    unsigned int frameHeight = h / numFrames;
    unsigned int top = currentFrameIndex * frameHeight;
    unsigned int bottom = top + frameHeight;

    if (bottom > h)
        bottom = h;     // Keep bottom <= height.

    for (y = top; y < bottom; y++) {
        cp = (RGBA*) (pixels + y*w);
        for (x = 0; x < w; ++x, ++cp) {
            if (cp->r == transColor.r &&
                cp->g == transColor.g &&
                cp->b == transColor.b) {
                cp->a = IM_TRANSPARENT;
            } else {
                if (haloWidth)
                    opaqueXYs.push_back(std::pair<int,int>(x,y));
            }
        }
    }

    int ox, oy, alpha;
    int span = int(haloWidth);
    std::list<std::pair<unsigned int,unsigned int> >::iterator it;
    for (it = opaqueXYs.begin(); it != opaqueXYs.end(); ++it) {
        ox = it->first;
        oy = it->second;
        unsigned int x_start  = std::max(0, ox - span);
        unsigned int x_finish = std::min(int(w), ox + span + 1);
        for (x = x_start; x < x_finish; ++x) {
            unsigned int y_start  = std::max(int(top),oy - span);
            unsigned int y_finish = std::min(int(bottom), oy + span + 1);
            for (y = y_start; y < y_finish; ++y) {
                int divisor = 1 + span*2 - abs(int(ox - x)) - abs(int(oy - y));
                cp = (RGBA*) (pixels + y*w + x);
                if (cp->a != IM_OPAQUE) {
                    alpha = cp->a + haloOpacityIncrementByPixelDistance / divisor;
                    cp->a = uint8_t((alpha > IM_OPAQUE) ? IM_OPAQUE : alpha);
                }
            }
        }
    }
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
 * Fills entire image with a given color.
 */
void Image::fill(const RGBA& col) {
#ifdef CHANNEL_REMAP
    RGBA swap;
    screenColor(&swap, col.r, col.g, col.b, col.a);
    image32_fill(this, &swap);
#else
    image32_fill(this, &col);
#endif
}

/**
 * Fills a rectangle in the image with a given color.
 */
void Image::fillRect(int x, int y, int rw, int rh, int r, int g, int b, int a) {
    RGBA col;
    uint32_t icol;
    uint32_t* dp;
    uint32_t* dend;
    uint32_t* drow = pixels + w * y + x;
    int blitW, blitH;

#ifdef CHANNEL_REMAP
    screenColor(&col, r, g, b, a);
#else
    col.r = r;
    col.g = g;
    col.b = b;
    col.a = a;
#endif
    icol = *((uint32_t*) &col);

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
 * Draws the entire image onto the screen at the given offset.
 */
void Image::draw(int x, int y) const {
    image32_blit(xu4.screenImage, x, y, this, blending);
}

/**
 * Draws a piece of the image onto the screen at the given offset.
 * The area of the image to draw is defined by the rectangle rx, ry, rw, rh.
 */
void Image::drawSubRect(int x, int y, int rx, int ry, int rw, int rh) const {
    image32_blitRect(xu4.screenImage, x, y, this, rx, ry, rw, rh, blending);
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
        dest = xu4.screenImage;

    // Clip position and source rect to positive values.
    CLIP_SUB(x, rx, rw, w, dest->w)
    CLIP_SUB(y, ry, rh, h, dest->h)

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
