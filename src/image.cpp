/*
 * image.cpp
 */

#include <assert.h>
#include <memory>
#include <list>
#include "image.h"
#include "settings.h"
#include "xu4.h"


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

static int imageBlending = 0;

/**
 * Enable blending (use alpha channel) for drawOn & drawSubRectOn.
 * Returns previous blending state.
 */
int Image::enableBlend(int on) {
    int prev = imageBlending;
    imageBlending = on ? 1 : 0;
    return prev;
}

Image::Image() {}

/**
 * Creates a new RGBA image.
 */
Image *Image::create(int w, int h) {
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
 * Creates a duplicate of another image
 */
Image *Image::duplicate(const Image *image) {
    Image *im = create(image->w, image->h);
    if (im) {
        image->drawOn(im, 0, 0);
    }
    return im;
}

/**
 * Frees the image.
 */
Image::~Image() {
    delete[] pixels;
}

RGBA Image::setColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return RGBA(r, g, b, a);
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
    RGBA col(r, g, b, a);
    pixels[ y*w + x ] = *((uint32_t*) &col);
}

void Image::makeColorTransparent(const RGBA& bgColor, int haloSize, int shadowOpacity)
{
    uint32_t icol = *((uint32_t*) &bgColor);
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
    uint32_t icol;
    uint32_t* dp = pixels;
    uint32_t* dend = pixels + w * h;

#ifdef CHANNEL_REMAP
    RGBA swap;
    if (screenFormatIsABGR) {
        screenColor(&swap, col.r, col.g, col.b, col.a);
        icol = *((uint32_t*) &swap);
    } else
        icol = *((uint32_t*) &col);
#else
    icol = *((uint32_t*) &col);
#endif

    while (dp != dend)
        *dp++ = icol;
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

inline uint8_t MIX(int A, int B, int alpha) {
    return (int8_t) (A + ((B - A) * alpha / 255));
}

/**
 * Draws the image onto another image.
 */
void Image::drawOn(Image *dest, int x, int y) const {
    uint32_t* drow;
    const uint32_t* srow = pixels;
    int blitW, blitH;

    if (dest == NULL)
        dest = xu4.screenImage;

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

    if (imageBlending) {
        uint8_t* dp;
        const uint8_t* sp;
        const uint8_t* send;
        int alpha;

        while (blitH--) {
            dp = (uint8_t*) drow;
            sp = (const uint8_t*) srow;
            send = (const uint8_t*) (srow + blitW);
            while( sp != send ) {
                alpha = sp[3];
                dp[0] = MIX(dp[0], sp[0], alpha);
                dp[1] = MIX(dp[1], sp[1], alpha);
                dp[2] = MIX(dp[2], sp[2], alpha);
                dp[3] = alpha;

                dp += 4;
                sp += 4;
            }
            drow += dest->w;
            srow += w;
        }
    } else {
        uint32_t* dp;
        const uint32_t* sp;
        const uint32_t* send;

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
}

#define CLIP_SUB(x, rx, rw, SD, DD) \
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
    if ((rw + rx) > int(SD)) \
        rw = SD - rx; \
    if (rw < 1) \
        return;

/**
 * Draws a piece of the image onto another image.
 */
void Image::drawSubRectOn(Image *dest, int x, int y, int rx, int ry, int rw, int rh) const {
    uint32_t* drow;
    const uint32_t* srow;

    if (dest == NULL)
        dest = xu4.screenImage;

    // Clip position and source rect to positive values.
    CLIP_SUB(x, rx, rw, w, dest->w)
    CLIP_SUB(y, ry, rh, h, dest->h)

    srow = pixels + w * ry + rx;
    drow = dest->pixels + dest->w * y + x;

    if (imageBlending) {
        uint8_t* dp;
        const uint8_t* sp;
        const uint8_t* send;
        int alpha;

        while (rh--) {
            dp = (uint8_t*) drow;
            sp = (const uint8_t*) srow;
            send = (const uint8_t*) (srow + rw);
            while( sp != send ) {
                alpha = sp[3];
                dp[0] = MIX(dp[0], sp[0], alpha);
                dp[1] = MIX(dp[1], sp[1], alpha);
                dp[2] = MIX(dp[2], sp[2], alpha);
                dp[3] = alpha;

                dp += 4;
                sp += 4;
            }
            drow += dest->w;
            srow += w;
        }
    } else {
        uint32_t* dp;
        const uint32_t* sp;
        const uint32_t* send;

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
 * Dumps the image to a file in PPM format. This is mainly used for debugging.
 */
void Image::save(const char *filename) {
    uint8_t* row;
    uint8_t* rowEnd;
    uint8_t* cp;
    int rowBytes = w*4;
    int alpha;
    RGBA color;
    FILE* fp;

    fp = fopen(filename, "w");
    if (! fp) {
        fprintf(stderr, "Image::save cannot open file %s\n", filename);
        return;
    }
    fprintf(fp, "P6 %d %d 255\n", w, h);

    row = (uint8_t*) pixels;
    for (unsigned y = 0; y < h; ++y) {
        cp = row;
        rowEnd = row + rowBytes;
        while (cp != rowEnd) {
            alpha = cp[3];
#if 1
            if (alpha == 255) {
                fwrite(cp, 1, 3, fp);
            } else {
                // PPM has no alpha channel so blend with pink to indicate it.
                color.r = MIX(255, cp[0], alpha);
                color.g = MIX(  0, cp[1], alpha);
                color.b = MIX(255, cp[2], alpha);
                fwrite(&color, 1, 3, fp);
            }
#else
            // Show alpha as greyscale and fully opaque as red.
            if (alpha == 255) {
                color.r = 255;
                color.g = color.b = 0;
            } else
                color.r = color.g = color.b = alpha;
            fwrite(&color, 1, 3, fp);
#endif
            cp += 4;
        }
        row += rowBytes;
    }
    fclose(fp);
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
