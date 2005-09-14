/*
 * $Id$
 */

#ifndef IMAGE_H
#define IMAGE_H

#include <string>
#include "types.h"
#include "u4file.h"

using std::string;

struct RGBA {
    unsigned int r, g, b, a;
};
bool operator==(const RGBA &lhs, const RGBA &rhs);

class Image;

struct SubImage {
    string name;
    string srcImageName;
    int x, y, width, height;
};

#define IM_OPAQUE 255
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
    void setTransparentIndex(unsigned int index);

    /* alpha handling */
    bool isAlphaOn() const;
    void alphaOn();
    void alphaOff();

    /* writing to image */
    void putPixel(int x, int y, int r, int g, int b, int a);
    void putPixelIndex(int x, int y, unsigned int index);
    void fillRect(int x, int y, int w, int h, int r, int g, int b);

    /* reading from image */
    void getPixel(int x, int y, unsigned int &r, unsigned int &g, unsigned int &b, unsigned int &a) const;
    void getPixelIndex(int x, int y, unsigned int &index) const;

    /* image drawing methods */
    void draw(int x, int y) const;
    void drawSubRect(int x, int y, int rx, int ry, int rw, int rh) const;
    void drawSubRectInverted(int x, int y, int rx, int ry, int rw, int rh) const;

    /* image drawing methods for drawing onto another image instead of the screen */
    void drawOn(Image *d, int x, int y) const;
    void drawSubRectOn(Image *d, int x, int y, int rx, int ry, int rw, int rh) const;
    void drawSubRectInvertedOn(Image *d, int x, int y, int rx, int ry, int rw, int rh) const;

    int width() const { return w; }
    int height() const { return h; }
    bool isIndexed() const { return indexed; }

private:
    int w, h;
    bool indexed;

    Image();                    /* use create method to construct images */

    // disallow assignments, copy contruction
    Image(const Image&);
    const Image &operator=(const Image&);

#ifndef _SDL_video_h
    struct SDL_Surface { int dummy; };
#endif

    SDL_Surface *surface;
};

#endif /* IMAGE_H */
