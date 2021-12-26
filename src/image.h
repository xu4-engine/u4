/*
 * image.h
 */

#ifndef IMAGE_H
#define IMAGE_H

#include <cstddef>
#include "image32.h"

#if defined(IOS)
typedef struct CGImage *CGImageRef;
typedef struct CGLayer *CGLayerRef;
#endif

#define IM_OPAQUE       255
#define IM_TRANSPARENT  0

/**
 * A simple image object that can be drawn and read/written to at the
 * pixel level.
 * @todo
 *  <ul>
 *      <li>drawing methods should be pushed to Drawable subclass</li>
 *  </ul>
 */
class Image : public Image32 {
public:
    static RGBA black;
    static int enableBlend(int on);
    static Image *create(int w, int h);
    static Image *duplicate(const Image *image);
    ~Image();

    void performTransparencyHack(const RGBA& colorValue, unsigned int numFrames, unsigned int currentFrameIndex, unsigned int haloWidth, unsigned int haloOpacityIncrementByPixelDistance);

    RGBA setColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a = IM_OPAQUE);

    /* Will make the pixels that match the color disappear, with a blur halo */
    void makeColorTransparent(const RGBA& bgColor, int haloSize = 0, int shadowOpacity = 2);

    /* writing to image */
    void putPixel(int x, int y, int r, int g, int b, int a);
    void putPixelIndex(int x, int y, uint32_t index);
    void fill(const RGBA& col);
    void fillRect(int x, int y, int w, int h, int r, int g, int b, int a=IM_OPAQUE);

    /* reading from image */
    void getPixel(int x, int y, unsigned int &r, unsigned int &g, unsigned int &b, unsigned int &a) const;
    void getPixel(int x, int y, RGBA &col) const;
    void getPixelIndex(int x, int y, unsigned int &index) const;

    /* image drawing methods */

    void draw(int x, int y) const;
    void drawSubRect(int x, int y, int rx, int ry, int rw, int rh) const;
    void drawLetter(int dx, int dy, int sx, int sy, int sw, int sh,
                    const RGBA* palette, const RGBA* bg) const;

    /**
     * Draws a piece of the image onto the screen at the given offset, inverted.
     * The area of the image to draw is defined by the rectangle rx, ry,
     * rw, rh.
     */
    void drawSubRectInverted(int x, int y, int rx, int ry, int rw, int rh) const {
        drawSubRectInvertedOn(NULL, x, y, rx, ry, rw, rh);
    }

    /** Draws the image onto another image. */
    void drawOn(Image *d, int x, int y) const {
        image32_blit(d, x, y, this, blending);
    }

    /** Draws a piece of the image onto another image. */
    void drawSubRectOn(Image *d, int x, int y,
                       int rx, int ry, int rw, int rh) const {
        image32_blitRect(d, x, y, this, rx, ry, rw, rh, blending);
    }

    void drawSubRectInvertedOn(Image *d, int x, int y, int rx, int ry, int rw, int rh) const;

    int width() const { return w; }
    int height() const { return h; }
    const uint32_t* pixelData() const { return pixels; }

    void save(const char* filename) {
        image32_savePPM(this, filename);
    }
    void drawHighlighted();

#ifdef IOS
    CGLayerRef getSurface() { return surface; }
    void initWithImage(CGImageRef image);
    void clearImageContents();

private:
    mutable char *cachedImageData;
    CGLayerRef surface;
    void clearCachedImageData() const;
    void createCachedImage() const;
    friend Image *screenScale(Image *src, int scale, int n, int filter);
#endif

private:
    static int blending;

    Image() {}      /* use create method to construct images */

    // disallow assignments, copy contruction
    Image(const Image&);
    const Image &operator=(const Image&);
};

#endif /* IMAGE_H */
