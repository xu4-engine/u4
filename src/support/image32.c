/*
    image32.c

    Written in 2021 by Karl Robillard <wickedsmoke@users.sf.net>

    To the extent possible under law, the author(s) have dedicated all
    copyright and related and neighboring rights to this software to the public
    domain worldwide. This software is distributed without any warranty.

    You should have received a copy of the CC0 Public Domain Dedication along
    with this software. If not, see
    <http://creativecommons.org/publicdomain/zero/1.0/>.
*/

#include <string.h>
#include <stdio.h>
#include "image32.h"

/**
 * Intialize an image struct with pixels set to NULL and w & h to zero.
 */
void image32_init(Image32* img) {
    img->pixels = NULL;
    img->w = img->h = 0;
}

/**
 * Allocate the pixels for the given size and initialize w & h.
 *
 * This function initializes all struct members so if pixels have been
 * previously allocated then image32_freePixels() should be called by the
 * user.
 *
 * \return Number of bytes allocated, or zero if malloc() fails.
 */
int image32_allocPixels(Image32* img, uint16_t w, uint16_t h)
{
    int size = w * h * sizeof(uint32_t);
    img->pixels = (uint32_t*) malloc(size);
    if (img->pixels) {
        img->w = w;
        img->h = h;
        return size;
    }
    img->w = img->h = 0;
    return 0;
}

/**
 * Free the pixels of an image and set the pointer to NULL.
 */
void image32_freePixels(Image32* img)
{
    free(img->pixels);
    img->pixels = NULL;
}

/**
 * Duplicate the pixels of an existing image.
 *
 * This function initializes all struct members so if pixels have been
 * previously allocated then image32_freePixels() should be called by the
 * user.
 *
 * \return Number of bytes allocated, or zero if malloc() fails.
 */
int image32_duplicatePixels(Image32* dest, const Image32* src)
{
    int bytes = image32_allocPixels(dest, src->w, src->h);
    if (bytes)
        memcpy(dest->pixels, src->pixels, bytes);
    return bytes;
}

/**
 * Fill an entire image with the given color.
 */
void image32_fill(Image32* img, const RGBA* color)
{
    uint32_t icol;
    uint32_t* dp   = img->pixels;
    uint32_t* dend = img->pixels + img->w * img->h;

    icol = *((uint32_t*) color);

    while (dp != dend)
        *dp++ = icol;
}

/**
 * Fill a rectangle in the image with the given color.
 */
void image32_fillRect(Image32* img, int x, int y, int rw, int rh,
                      const RGBA* color)
{
    uint32_t icol;
    uint32_t* dp;
    uint32_t* dend;
    uint32_t* drow = img->pixels + img->w * y + x;

    icol = *((uint32_t*) &color);

    if ((rw + x) > img->w)
        rw = img->w - x;
    if (rw < 1)
        return;

    if ((rh + y) > img->h)
        rh = img->h - y;
    if (rh < 1)
        return;

    while (rh--) {
        dp = drow;
        dend = dp + rw;
        while( dp != dend )
            *dp++ = icol;
        drow += img->w;
    }
}

inline uint8_t MIX(int A, int B, int alpha)
{
    return (int8_t) (A + ((B - A) * alpha / 255));
}

/**
 * Draw one image onto another.
 *
 * \param blend     If non-zero then the src alpha values determine how
 *                  strongly src RGB values will be mixed with dest.
 *                  The dest alpha is set to src alpha in either case.
 */
void image32_blit(Image32* dest, int dx, int dy, const Image32* src, int blend)
{
    uint32_t* drow;
    const uint32_t* srow = src->pixels;
    int blitW, blitH;

    blitW = src->w;
    if (dx < 0) {
        srow += -dx;
        blitW += dx;     // Subtracts from blitW.
        dx = 0;
    }
    else if ((blitW + dx) > int(dest->w)) {
        blitW = dest->w - dx;
    }
    if (blitW < 1)
        return;

    blitH = src->h;
    if (dy < 0) {
        srow += src->w * -dy;
        blitH += dy;     // Subtracts from blitH.
        dy = 0;
    }
    else if ((blitH + dy) > int(dest->h)) {
        blitH = dest->h - dy;
    }
    if (blitH < 1)
        return;

    drow = dest->pixels + dest->w * dy + dx;

    if (blend) {
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
            srow += src->w;
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
            srow += src->w;
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
 * Draw a sub-rectangle of one image onto another.
 *
 * \param blend     If non-zero then the src alpha values determine how
 *                  strongly src RGB values will be mixed with dest.
 *                  The dest alpha is set to src alpha in either case.
 */
void image32_blitRect(Image32* dest, int dx, int dy,
                      const Image32* src, int sx, int sy, int sw, int sh,
                      int blend)
{
    uint32_t* drow;
    const uint32_t* srow;

    // Clip position and source rect to positive values.
    CLIP_SUB(dx, sx, sw, src->w, dest->w)
    CLIP_SUB(dy, sy, sh, src->h, dest->h)

    srow = src->pixels + src->w * sy + sx;
    drow = dest->pixels + dest->w * dy + dx;

    if (blend) {
        uint8_t* dp;
        const uint8_t* sp;
        const uint8_t* send;
        int alpha;

        while (sh--) {
            dp = (uint8_t*) drow;
            sp = (const uint8_t*) srow;
            send = (const uint8_t*) (srow + sw);
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
            srow += src->w;
        }
    } else {
        uint32_t* dp;
        const uint32_t* sp;
        const uint32_t* send;

        while (sh--) {
            dp = drow;
            sp = srow;
            send = sp + sw;
            while( sp != send )
                *dp++ = *sp++;
            drow += dest->w;
            srow += src->w;
        }
    }
}

/**
 * Dump the image to a file in PPM format. This is mainly used for debugging.
 */
void image32_savePPM(const Image32* img, const char *filename)
{
    uint8_t* row;
    uint8_t* rowEnd;
    uint8_t* cp;
    int rowBytes = img->w * 4;
    int alpha;
    int y;
    RGBA color;
    FILE* fp;

    fp = fopen(filename, "w");
    if (! fp) {
        fprintf(stderr, "image32_save cannot open file %s\n", filename);
        return;
    }
    fprintf(fp, "P6 %d %d 255\n", img->w, img->h);

    row = (uint8_t*) img->pixels;
    for (y = 0; y < img->h; ++y) {
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
