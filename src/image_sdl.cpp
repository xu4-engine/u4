/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include <SDL.h>

#include <memory>
#include "debug.h"
#include "image.h"

Image::Image() : surface(NULL) {
}

/**
 * Creates a new image.  Scale is stored to allow drawing using U4
 * (320x200) coordinates, regardless of the actual image scale.
 * Indexed is true for palette based images, or false for RGB images.
 * Image type determines whether to create a hardware (i.e. video ram)
 * or software (i.e. normal ram) image.
 */
Image *Image::create(int w, int h, bool indexed, Image::Type type) {
    Uint32 rmask, gmask, bmask, amask;
    Uint32 flags;
    Image *im = new Image;

    im->w = w;
    im->h = h;
    im->indexed = indexed;

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    rmask = 0xff000000;
    gmask = 0x00ff0000;
    bmask = 0x0000ff00;
    amask = 0x000000ff;
#else
    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = 0xff000000;
#endif

    if (type == Image::HARDWARE)
        flags = SDL_HWSURFACE;
    else
        flags = SDL_SWSURFACE;
    
    if (indexed)
        im->surface = SDL_CreateRGBSurface(flags, w, h, 8, rmask, gmask, bmask, amask);
    else
        im->surface = SDL_CreateRGBSurface(flags, w, h, 32, rmask, gmask, bmask, amask);

    if (!im->surface) {
        delete im;
        return NULL;
    }

    return im;
}

/**
 * Create a special purpose image the represents the whole screen.
 */
Image *Image::createScreenImage() {
    Image *screen = new Image();

    screen->surface = SDL_GetVideoSurface();
    ASSERT(screen->surface != NULL, "SDL_GetVideoSurface() returned a NULL screen surface!");
    screen->w = screen->surface->w;
    screen->h = screen->surface->h;
    screen->indexed = screen->surface->format->palette != NULL;

    return screen;
}

/**
 * Creates a duplicate of another image
 */
Image *Image::duplicate(Image *image) {    
    bool alphaOn = image->isAlphaOn();
    Image *im = create(image->width(), image->height(), image->isIndexed(), image->surface->flags & SDL_HWSURFACE ? HARDWARE : SOFTWARE);
    
    if (image->isIndexed())
        im->setPaletteFromImage(image);

    /* Turn alpha off before blitting to non-screen surfaces */
    if (alphaOn)
        image->alphaOff();
    
    image->drawOn(im, 0, 0);

    if (alphaOn)
        image->alphaOn();

    return im;
}

/**
 * Frees the image.
 */
Image::~Image() {
    SDL_FreeSurface(surface);
}

/**
 * Sets the palette
 */
void Image::setPalette(const RGBA *colors, unsigned n_colors) {
    ASSERT(indexed, "imageSetPalette called on non-indexed image");
    
    SDL_Color *sdlcolors = new SDL_Color[n_colors];
    for (unsigned i = 0; i < n_colors; i++) {
        sdlcolors[i].r = colors[i].r;
        sdlcolors[i].g = colors[i].g;
        sdlcolors[i].b = colors[i].b;
    }

    SDL_SetColors(surface, sdlcolors, 0, n_colors);

    delete [] sdlcolors;
}

/**
 * Copies the palette from another image.
 */
void Image::setPaletteFromImage(const Image *src) {
    ASSERT(indexed && src->indexed, "imageSetPaletteFromImage called on non-indexed image");
    memcpy(surface->format->palette->colors, 
           src->surface->format->palette->colors, 
           sizeof(SDL_Color) * src->surface->format->palette->ncolors);
}

/* returns the palette index of the specified RGB color */
int Image::getPaletteIndex(SDL_Color color) {
    if (!indexed)
        return -1;

    for (int i = 0; i < surface->format->palette->ncolors; i++)
    {
        if ( (surface->format->palette->colors[i].r == 255)
            && (surface->format->palette->colors[i].g == 255)
            && (surface->format->palette->colors[i].b == 255) ) {
            return i;
        }

    }

    // return the proper palette index for the specified color
    return -1;
}

SDL_Color Image::setColor(Uint8 r, Uint8 g, Uint8 b) {
    SDL_Color color = {r, g, b, 0};
    return color;
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
bool Image::setPaletteIndex(unsigned int index, SDL_Color color) {
    if (!indexed)
        return false;

    surface->format->palette->colors[index].r = color.r;
    surface->format->palette->colors[index].g = color.g;
    surface->format->palette->colors[index].b = color.b;

    // success
    return true;
}

bool Image::getTransparentIndex(unsigned int &index) const {
    if (!indexed || (surface->flags & SDL_SRCCOLORKEY) == 0)
        return false;
        
    index = surface->format->colorkey;
    return true;
}

void Image::setTransparentIndex(unsigned int index) {

    SDL_SetAlpha(surface, SDL_SRCALPHA, SDL_ALPHA_OPAQUE);

    if (indexed) {
        SDL_SetColorKey(surface, SDL_SRCCOLORKEY, index);
    } else {
        int x, y;
        Uint8 t_r, t_g, t_b;

        SDL_GetRGB(index, surface->format, &t_r, &t_g, &t_b);

        for (y = 0; y < h; y++) {
            for (x = 0; x < w; x++) {
                unsigned int r, g, b, a;
                getPixel(x, y, r, g, b, a);
                if (r == t_r &&
                    g == t_g &&
                    b == t_b) {
                    putPixel(x, y, r, g, b, IM_TRANSPARENT);
                }
            }
        }
    }
}

bool Image::isAlphaOn() const {
    return (surface->flags & SDL_SRCALPHA) ? true : false;
}

void Image::alphaOn() {
    surface->flags |= SDL_SRCALPHA;    
}

void Image::alphaOff() {
    surface->flags &= ~SDL_SRCALPHA;    
}

/**
 * Sets the color of a single pixel.
 */
void Image::putPixel(int x, int y, int r, int g, int b, int a) {
    putPixelIndex(x, y, SDL_MapRGBA(surface->format, static_cast<Uint8>(r), static_cast<Uint8>(g), static_cast<Uint8>(b), static_cast<Uint8>(a)));
}

/**
 * Sets the palette index of a single pixel.  If the image is in
 * indexed mode, then the index is simply the palette entry number.
 * If the image is RGB, it is a packed RGB triplet.
 */
void Image::putPixelIndex(int x, int y, unsigned int index) {
    int bpp;
    Uint8 *p;

    bpp = surface->format->BytesPerPixel;
    p = static_cast<Uint8 *>(surface->pixels) + y * surface->pitch + x * bpp;

    switch(bpp) {
    case 1:
        *p = index;
        break;

    case 2:
        *reinterpret_cast<Uint16 *>(p) = index;
        break;

    case 3:
        if(SDL_BYTEORDER == SDL_BIG_ENDIAN) {
            p[0] = (index >> 16) & 0xff;
            p[1] = (index >> 8) & 0xff;
            p[2] = index & 0xff;
        } else {
            p[0] = index & 0xff;
            p[1] = (index >> 8) & 0xff;
            p[2] = (index >> 16) & 0xff;
        }
        break;

    case 4:
        *reinterpret_cast<Uint32 *>(p) = index;
        break;
    }
}

/**
 * Fills a rectangle in the image with a given color.
 */
void Image::fillRect(int x, int y, int w, int h, int r, int g, int b) {
    SDL_Rect dest;
    Uint32 pixel;

    pixel = SDL_MapRGB(surface->format, static_cast<Uint8>(r), static_cast<Uint8>(g), static_cast<Uint8>(b));
    dest.x = x;
    dest.y = y;
    dest.w = w;
    dest.h = h;

    SDL_FillRect(surface, &dest, pixel);
}

/**
 * Gets the color of a single pixel.
 */
void Image::getPixel(int x, int y, unsigned int &r, unsigned int &g, unsigned int &b, unsigned int &a) const {
    unsigned int index;
    Uint8 r1, g1, b1, a1;

    getPixelIndex(x, y, index);

    SDL_GetRGBA(index, surface->format, &r1, &g1, &b1, &a1);
    r = r1;
    g = g1;
    b = b1;
    a = a1;
}

/**
 * Gets the palette index of a single pixel.  If the image is in
 * indexed mode, then the index is simply the palette entry number.
 * If the image is RGB, it is a packed RGB triplet.
 */
void Image::getPixelIndex(int x, int y, unsigned int &index) const {
    int bpp = surface->format->BytesPerPixel;

    Uint8 *p = static_cast<Uint8 *>(surface->pixels) + y * surface->pitch + x * bpp;

    switch(bpp) {
    case 1:
        index = *p;
        break;

    case 2:
        index = *reinterpret_cast<Uint16 *>(p);
        break;

    case 3:
        if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
            index = p[0] << 16 | p[1] << 8 | p[2];
        else
            index = p[0] | p[1] << 8 | p[2] << 16;
        break;

    case 4:
        index = *reinterpret_cast<Uint32 *>(p);

    default:
        return;
    }
}

/**
 * Draws the entire image onto the screen at the given offset.
 */
void Image::draw(int x, int y) const {
    drawOn(NULL, x, y);
}

/**
 * Draws a piece of the image onto the screen at the given offset.
 * The area of the image to draw is defined by the rectangle rx, ry,
 * rw, rh.
 */
void Image::drawSubRect(int x, int y, int rx, int ry, int rw, int rh) const {
    drawSubRectOn(NULL, x, y, rx, ry, rw, rh);
}

/**
 * Draws a piece of the image onto the screen at the given offset, inverted.
 * The area of the image to draw is defined by the rectangle rx, ry,
 * rw, rh.
 */
void Image::drawSubRectInverted(int x, int y, int rx, int ry, int rw, int rh) const {
    drawSubRectInvertedOn(NULL, x, y, rx, ry, rw, rh);
}

/**
 * Draws the image onto another image.
 */
void Image::drawOn(Image *d, int x, int y) const {
    SDL_Rect r;
    SDL_Surface *destSurface;

    if (d == NULL)
        destSurface = SDL_GetVideoSurface();
    else
        destSurface = d->surface;

    r.x = x;
    r.y = y;
    r.w = w;
    r.h = h;
    SDL_BlitSurface(surface, NULL, destSurface, &r);
}

/**
 * Draws a piece of the image onto another image.
 */
void Image::drawSubRectOn(Image *d, int x, int y, int rx, int ry, int rw, int rh) const {
    SDL_Rect src, dest;
    SDL_Surface *destSurface;

    if (d == NULL)
        destSurface = SDL_GetVideoSurface();
    else
        destSurface = d->surface;

    src.x = rx;
    src.y = ry;
    src.w = rw;
    src.h = rh;

    dest.x = x;
    dest.y = y;
    /* dest w & h unused */

    SDL_BlitSurface(surface, &src, destSurface, &dest);
}

/**
 * Draws a piece of the image onto another image, inverted.
 */
void Image::drawSubRectInvertedOn(Image *d, int x, int y, int rx, int ry, int rw, int rh) const {
    int i;
    SDL_Rect src, dest;
    SDL_Surface *destSurface;

    if (d == NULL)
        destSurface = SDL_GetVideoSurface();
    else
        destSurface = d->surface;

    for (i = 0; i < rh; i++) {
        src.x = rx;
        src.y = ry + i;
        src.w = rw;
        src.h = 1;

        dest.x = x;
        dest.y = y + rh - i - 1;
        /* dest w & h unused */

        SDL_BlitSurface(surface, &src, destSurface, &dest);
    }
}

/**
 * Dumps the image to a file.  The file is always saved in .bmp
 * format.  This is mainly used for debugging.
 */
void Image::save(const string &filename) {
    SDL_SaveBMP(surface, filename.c_str());
}
