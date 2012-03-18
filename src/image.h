/*
 * $Id$
 */

#ifndef IMAGE_H
#define IMAGE_H

#include <SDL.h>

#include <string>
#include <stdint.h>
#include "types.h"
#include "u4file.h"
#include "textcolor.h"

#if defined(IOS)
typedef struct CGImage *CGImageRef;
typedef struct CGLayer *CGLayerRef;
typedef CGLayerRef BackendSurface;
#else
struct SDL_Surface;
typedef SDL_Surface *BackendSurface;
#endif

using std::string;


#define DARK_GRAY_HALO RGBA(14,15,16,255)

struct RGBA {
	RGBA(int r, int g, int b, int a) : r(r), g(g), b(b), a(a){}
	RGBA() : r(0), g(0), b(0), a(255){}
    unsigned int r, g, b, a;
};
bool operator==(const RGBA &lhs, const RGBA &rhs);

class Image;

struct SubImage {
    string name;
    string srcImageName;
    int x, y, width, height;
};

#define IM_OPAQUE (unsigned int) 255
#define IM_TRANSPARENT 0

/**
 * A simple image object that can be drawn and read/written to at the
 * pixel level.
 * @todo
 *  <ul>
 *      <li>drawing methods should be pushed to Drawable subclass</li>
 *  </ul>
 */
class Image {
public:
    enum Type {
        HARDWARE,
        SOFTWARE
    };

    static Image *create(int w, int h, bool indexed, Type type);
    static Image *createScreenImage();
    static Image *duplicate(Image *image);
    ~Image();

    /* palette handling */
    void setPalette(const RGBA *colors, unsigned n_colors);
    void setPaletteFromImage(const Image *src);
    bool getTransparentIndex(unsigned int &index) const;
    void performTransparencyHack(unsigned int colorValue, unsigned int numFrames, unsigned int currentFrameIndex, unsigned int haloWidth, unsigned int haloOpacityIncrementByPixelDistance);
    void setTransparentIndex(unsigned int index);
//    void invokeTransparencyHack(ImageInfo * info);

    bool setFontColor(ColorFG fg, ColorBG bg);
    bool setFontColorFG(ColorFG fg);
    bool setFontColorBG(ColorBG bg);

    RGBA getPaletteColor(int index);       // returns the color of the specified palette index
    bool setPaletteIndex(unsigned int index, RGBA color);  // sets the specified palette index to the specified RGB color
    int getPaletteIndex(RGBA color);              // returns the palette index of the specified RGB color
    RGBA setColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a = IM_OPAQUE);


    /* alpha handling */
    bool isAlphaOn() const  {return surface->flags & SDL_SRCALPHA;}
    void alphaOn()          {surface->flags |= SDL_SRCALPHA;}
    void alphaOff()         {surface->flags &= ~SDL_SRCALPHA;}


    /* Will clear the image to the background color, and set the internal backgroundColor variable */
    void initializeToBackgroundColor(RGBA backgroundColor = DARK_GRAY_HALO);
    /* Will make the pixels that match the background color disappear, with a blur halo */
    void makeBackgroundColorTransparent(int haloSize = 0,  int shadowOpacity = 255);


    //void finalizeAlphaSurface(RGBA * key = NULL);

    /* writing to image */

    /**
     * Sets the color of a single pixel.
     */
    void putPixel(int x, int y, int r, int g, int b, int a) { //TODO Consider using &
        putPixelIndex(x, y, SDL_MapRGBA(surface->format, static_cast<Uint8>(r), static_cast<Uint8>(g), static_cast<Uint8>(b), static_cast<Uint8>(a)));
    }
    void putPixelIndex(int x, int y, unsigned int index);


    void fillRect(int x, int y, int w, int h, int r, int g, int b, int a=IM_OPAQUE);

    /* reading from image */
    void getPixel(int x, int y, unsigned int &r, unsigned int &g, unsigned int &b, unsigned int &a) const;
    void getPixelIndex(int x, int y, unsigned int &index) const;

    /* image drawing methods */
    /**
     * Draws the entire image onto the screen at the given offset.
     */
    void draw(int x, int y) const {
        drawOn(NULL, x, y);
    }

    /**
     * Draws a piece of the image onto the screen at the given offset.
     * The area of the image to draw is defined by the rectangle rx, ry,
     * rw, rh.
     */
    void drawSubRect(int x, int y, int rx, int ry, int rw, int rh) const {
        drawSubRectOn(NULL, x, y, rx, ry, rw, rh);
    }

    /**
     * Draws a piece of the image onto the screen at the given offset, inverted.
     * The area of the image to draw is defined by the rectangle rx, ry,
     * rw, rh.
     */
    void drawSubRectInverted(int x, int y, int rx, int ry, int rw, int rh) const {
        drawSubRectInvertedOn(NULL, x, y, rx, ry, rw, rh);
    }

    /* image drawing methods for drawing onto another image instead of the screen */
    void drawOn(Image *d, int x, int y) const;
    void drawSubRectOn(Image *d, int x, int y, int rx, int ry, int rw, int rh) const;
    void drawSubRectInvertedOn(Image *d, int x, int y, int rx, int ry, int rw, int rh) const;

    int width() const { return w; }
    int height() const { return h; }
    bool isIndexed() const { return indexed; }
    BackendSurface getSurface() { return surface; }
    void save(const string &filename);
#ifdef IOS
    void initWithImage(CGImageRef image);
    void clearImageContents();
#endif
    void drawHighlighted();


private:
    unsigned int w, h;
    bool indexed;
    RGBA backgroundColor;
#ifdef IOS
    mutable char *cachedImageData;
    void clearCachedImageData() const;
    void createCachedImage() const;
    friend Image *screenScale(Image *src, int scale, int n, int filter);
#endif
    Image();                    /* use create method to construct images */

    // disallow assignments, copy contruction
    Image(const Image&);
    const Image &operator=(const Image&);

    BackendSurface surface;
};

#endif /* IMAGE_H */
