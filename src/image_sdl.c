/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <SDL.h>

#include "debug.h"
#include "image.h"

/**
 * Creates a new image.  Scale is stored to allow drawing using U4
 * (320x200) coordinates, regardless of the actual image scale.
 * Indexed is true for palette based images, or false for RGB images.
 * Image type determines whether to create a hardware (i.e. video ram)
 * or software (i.e. normal ram) image.
 */
Image *imageNew(int w, int h, int scale, int indexed, ImageType type) {
    Uint32 rmask, gmask, bmask, amask;
    Uint32 flags;
    Image *im = malloc(sizeof(Image));

    im->w = w;
    im->h = h;
    im->scale = scale;
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

    if (type == IMTYPE_HW)
        flags = SDL_HWSURFACE;
    else
        flags = SDL_SWSURFACE;
    
    if (indexed)
        im->surface = SDL_CreateRGBSurface(flags, w, h, 8, rmask, gmask, bmask, amask);
    else
        im->surface = SDL_CreateRGBSurface(flags, w, h, 32, rmask, gmask, bmask, amask);

    if (!im->surface) {
        free(im);
        return NULL;
    }

    return im;
}

/**
 * Frees an image.
 */
void imageDelete(Image *im) {
    SDL_FreeSurface(im->surface);
    free(im);
}

/**
 * Copies the palette of another image into the given image.
 */
void imageSetPaletteFromImage(Image *im, Image *src) {
    ASSERT(im->indexed && src->indexed, "imageSetPaletteFromImage called on non-indexed image");
    memcpy(im->surface->format->palette->colors, 
           src->surface->format->palette->colors, 
           sizeof(SDL_Color) * src->surface->format->palette->ncolors);
}

int imageGetTransparentIndex(Image *im, unsigned int *index) {
    if (!im->indexed || (im->surface->flags & SDL_SRCCOLORKEY) == 0)
        return 0;
        
    *index = im->surface->format->colorkey;
    return 1;
}

void imageSetTransparentIndex(Image *im, unsigned int index) {

    SDL_SetAlpha(im->surface, SDL_SRCALPHA, SDL_ALPHA_OPAQUE);

    if (im->indexed) {
        SDL_SetColorKey(im->surface, SDL_SRCCOLORKEY, index);
    } else {
        int x, y;
        Uint8 t_r, t_g, t_b;

        SDL_GetRGB(index, im->surface->format, &t_r, &t_g, &t_b);

        for (y = 0; y < im->h; y++) {
            for (x = 0; x < im->w; x++) {
                unsigned int r, g, b, a;
                imageGetPixel(im, x, y, &r, &g, &b, &a);
                if (r == t_r &&
                    g == t_g &&
                    b == t_b) {
                    imagePutPixel(im, x, y, r, g, b, IM_TRANSPARENT);
                }
            }
        }
    }
}

/**
 * Sets the color of a single pixel.
 */
void imagePutPixel(Image *im, int x, int y, int r, int g, int b, int a) {
    imagePutPixelIndex(im, x, y, SDL_MapRGBA(im->surface->format, (Uint8)r, (Uint8)g, (Uint8)b, (Uint8)a));
}

/**
 * Sets the palette index of a single pixel.  If the image is in
 * indexed mode, then the index is simply the palette entry number.
 * If the image is RGB, it is a packed RGB triplet.
 */
void imagePutPixelIndex(Image *im, int x, int y, unsigned int index) {
    int bpp;
    Uint8 *p;

    bpp = im->surface->format->BytesPerPixel;
    p = (Uint8 *)im->surface->pixels + y * im->surface->pitch + x * bpp;

    switch(bpp) {
    case 1:
        *p = index;
        break;

    case 2:
        *(Uint16 *)p = index;
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
        *(Uint32 *)p = index;
        break;
    }
}

/**
 * Sets the color of a "U4" scale pixel, which may be more than one
 * actual pixel.
 */
void imagePutPixelScaled(Image *im, int x, int y, int r, int g, int b, int a) {
    int xs, ys;

    for (xs = 0; xs < im->scale; xs++) {
        for (ys = 0; ys < im->scale; ys++) {
            imagePutPixel(im, x * im->scale + xs, y * im->scale + ys, r, g, b, a);
        }
    }
}

/**
 * Gets the color of a single pixel.
 */
void imageGetPixel(Image *im, int x, int y, int *r, int *g, int *b, int *a) {
    unsigned int index;
    Uint8 r1, g1, b1, a1;

    imageGetPixelIndex(im, x, y, &index);

    SDL_GetRGBA(index, im->surface->format, &r1, &g1, &b1, &a1);
    *r = r1;
    *g = g1;
    *b = b1;
    *a = a1;
}

/**
 * Gets the palette index of a single pixel.  If the image is in
 * indexed mode, then the index is simply the palette entry number.
 * If the image is RGB, it is a packed RGB triplet.
 */
void imageGetPixelIndex(Image *im, int x, int y, unsigned int *index) {
    int bpp = im->surface->format->BytesPerPixel;

    Uint8 *p = (Uint8 *) im->surface->pixels + y * im->surface->pitch + x * bpp;

    switch(bpp) {
    case 1:
        (*index) = *p;
        break;

    case 2:
        (*index) = *(Uint16 *)p;
        break;

    case 3:
        if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
            (*index) =  p[0] << 16 | p[1] << 8 | p[2];
        else
            (*index) =  p[0] | p[1] << 8 | p[2] << 16;
        break;

    case 4:
        (*index) = *(Uint32 *)p;

    default:
        return;
    }
}

/**
 * Fills a rectangle in the image with a given color.
 */
void imageFillRect(Image *im, int x, int y, int w, int h, int r, int g, int b) {
    SDL_Rect dest;
    Uint32 pixel;

    pixel = SDL_MapRGB(im->surface->format, (Uint8)r, (Uint8)g, (Uint8)b);
    dest.x = x;
    dest.y = y;
    dest.w = w;
    dest.h = h;

    SDL_FillRect(im->surface, &dest, pixel);
}

/**
 * Draws the entire image onto the screen at the given offset.
 */
void imageDraw(const Image *im, int x, int y) {
    SDL_Rect r;

    r.x = x;
    r.y = y;
    r.w = im->w;
    r.h = im->h;
    SDL_BlitSurface(im->surface, NULL, SDL_GetVideoSurface(), &r);
}

/**
 * Draws a piece of the image onto the screen at the given offset.
 * The area of the image to draw is defined by the rectangle rx, ry,
 * rw, rh.
 */
void imageDrawSubRect(const Image *im, int x, int y, int rx, int ry, int rw, int rh) {
    SDL_Rect src, dest;

    src.x = rx;
    src.y = ry;
    src.w = rw;
    src.h = rh;

    dest.x = x;
    dest.y = y;
    /* dest w & h unused */

    SDL_BlitSurface(im->surface, &src, SDL_GetVideoSurface(), &dest);
}
