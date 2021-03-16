/*
 *  image_ios.mm
 *  Ultima4-iPad
 *
 * Copyright 2011 Trenton Schulz. All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification, are
 * permitted provided that the following conditions are met:
 * 
 *    1. Redistributions of source code must retain the above copyright notice, this list of
 *       conditions and the following disclaimer.
 * 
 *    2. Redistributions in binary form must reproduce the above copyright notice, this list
 *       of conditions and the following disclaimer in the documentation and/or other materials
 *       provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY TRENTON SCHULZ ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * The views and conclusions contained in the software and documentation are those of the
 * authors and should not be interpreted as representing official policies, either expressed
 * or implied, of Trenton Schulz.
 *
 */


/*
 * $Id$
 */

#include "CGContextGStateSaver.h"
#include "debug.h"
#include "image.h"
#include "U4CFHelper.h"
#import "ios_helpers.h"
#import "U4View.h"

#include <boost/scoped_array.hpp>

static CGColorSpaceRef genericColorSpace = 0;

const CGBitmapInfo U4BitmapFlags =  kCGImageAlphaPremultipliedLast | kCGBitmapByteOrderDefault;

Image::Image() : surface(0), cachedImageData(0) {}
/**
 * Creates a new image.  Scale is stored to allow drawing using U4
 * (320x200) coordinates, regardless of the actual image scale.
 * Indexed is true for palette based images, or false for RGB images.
 */
Image *Image::create(int w, int h, bool indexed) {
    if (genericColorSpace == 0)
        genericColorSpace = U4IOS::u4colorSpace();
    Image *im = new Image();
    boost::scoped_array<char> data(new char[w * h * 4]);
    {
        boost::intrusive_ptr<CGContext> context = cftypeFromCreateOrCopy(CGBitmapContextCreate(data.get(), w, h, 8, w * 4, genericColorSpace,
                                                                          U4BitmapFlags));
        CGContextClearRect(context.get(), CGRectMake(0, 0, w, h));
        im->w = w;
        im->h = h;
        im->indexed = indexed;

        assert(indexed == false);
        im->surface = CGLayerCreateWithContext(context.get(), CGSizeMake(w, h), 0);
    }
    return im;
}

Image *Image::createMem(int w, int h, bool indexed) {
    return Image::create(w, h, indexed);
}

/**
 * Create a special purpose image the represents the whole screen.
 */
Image *Image::createScreenImage() {
    Image *screen = [U4IOS::frontU4View() image];
    ASSERT(screen->surface != NULL, "SDL_GetVideoSurface() returned a NULL screen surface!");
    CGSize size = CGLayerGetSize(screen->surface);
    screen->w = int(size.width);
    screen->h = int(size.height);
    screen->indexed = false;
    return screen;
}

static void clearSurfaceToBlack(CGLayerRef surface) {
    // Clear the image since it's coming from a duplicate.
    CGContextRef context = CGLayerGetContext(surface);
    CGContextGStateSaver contextSaver(context);
    CGSize size = CGLayerGetSize(surface);
    CGContextClearRect(context, CGRectMake(0, 0, size.width, size.height));
}

/**
 * Creates a duplicate of another image
 */
Image *Image::duplicate(Image *image) {
    bool alphaOn = image->isAlphaOn();
    Image *im = create(image->width(), image->height(), image->isIndexed(), SOFTWARE);
    
    if (image->isIndexed())
        im->setPaletteFromImage(image);

    
    clearSurfaceToBlack(im->surface);
    /* Turn alpha off before blitting to non-screen surfaces */
    if (alphaOn) {
    }
    image->drawOn(im, 0, 0);
    
    if (alphaOn)
        image->alphaOn();
    
    return im;
}

/**
 * Frees the image.
 */
Image::~Image() {
    CGLayerRelease(surface);
    clearCachedImageData();
}

void Image::clearCachedImageData() const {
    delete [] cachedImageData;
    cachedImageData = 0;
}

/**
 * Sets the palette
 */
void Image::setPalette(const RGBA *colors, unsigned n_colors) {
    // Not necessary.
}

/**
 * Copies the palette from another image.
 */
void Image::setPaletteFromImage(const Image *src) {
    // Not necessary.
}

// returns the color of the specified palette index
RGBA Image::getPaletteColor(int index) {
    return RGBA(0, 0, 0, 255);
}

/* returns the palette index of the specified RGB color */
int Image::getPaletteIndex(RGBA color) {
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
    return false;
}

bool Image::getTransparentIndex(unsigned int &index) const {
    return false;
    /* ### HELP!!!
     if (!indexed || (surface->flags & SDL_SRCCOLORKEY) == 0)
     return false;
     
     index = surface->format->colorkey;
     return true;
     */
}

void Image::setTransparentIndex(unsigned int index) {
    /* ### HELP!!!
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
     */
}

void Image::performTransparencyHack(unsigned int colorValue, unsigned int numFrames, unsigned int currentFrameIndex,
                                    unsigned int haloWidth, unsigned int haloOpacityIncrementByPixelDistance) {
    // ### Ignore this for now since we are using already transparent things.
}

/* The iOS variant seems to have its own way of handling transparency */
void Image::initializeToBackgroundColor(RGBA backgroundColor) {
    
}

void Image::makeBackgroundColorTransparent(int haloSize, int shadowOpacity) {
    // ### Ignore this for now since we are using already transparent things.
}

bool Image::isAlphaOn() const {
    return true;
}

void Image::alphaOn() {
    // ### Help surface->flags |= SDL_SRCALPHA;
}

void Image::alphaOff() {
    // ### Help surface->flags &= ~SDL_SRCALPHA;
}

/**
 * Sets the color of a single pixel.
 */
void Image::putPixel(int x, int y, int r, int g, int b, int a) {
    fillRect(x, y, 1, 1, r, g, b, a);
}

/**
 * Sets the palette index of a single pixel.  If the image is in
 * indexed mode, then the index is simply the palette entry number.
 * If the image is RGB, it is a packed RGB triplet.
 */
void Image::putPixelIndex(int x, int y, unsigned int index) {
    fprintf(stderr, "Image::putPixelIndex: implement me!!!\n");
}

/**
 * Fills a rectangle in the image with a given color.
 */
void Image::fillRect(int x, int y, int w, int h, int r, int g, int b, int a) {
    CGContextRef context = CGLayerGetContext(surface);
    CGContextGStateSaver contextSaver(context);
    CGFloat components[] = { r / 255., g / 255., b / 255., a / 255. };
    boost::intrusive_ptr<CGColor> color = cftypeFromCreateOrCopy(CGColorCreate(genericColorSpace, components));
    CGContextSetFillColorWithColor(context, color.get());
    CGContextFillRect(context, CGRectMake(x, y, w, h));
}

void Image::createCachedImage() const {
    if (cachedImageData == 0) {
        CGSize size = CGLayerGetSize(surface);
        const size_t width = size_t(size.width);
        const size_t height = size_t(size.height);
        const int bytesPerLine = ((width * 4) + 15) & ~15;
        cachedImageData = new char[bytesPerLine * height];
        boost::intrusive_ptr<CGContext> bitmapContext = cftypeFromCreateOrCopy(CGBitmapContextCreate(cachedImageData, width, height, 8, bytesPerLine,
                                                                                                     genericColorSpace, U4BitmapFlags));
        CGContextClearRect(bitmapContext.get(), CGRectMake(0, 0, width, height));
        CGContextDrawLayerAtPoint(bitmapContext.get(), CGPointMake(0, 0), surface);
    }
}

void Image::drawHighlighted() {
    CGContextRef context = CGLayerGetContext(surface);
    CGContextGStateSaver contextSaver(context);
    CGContextSetBlendMode(context, kCGBlendModeScreen);
    CGContextSetRGBFillColor(context, 1.0, 1.0, 1.0, 0.5);
    CGContextFillRect(context, CGRectMake(0, 0, width(), height()));
}

/**
 * Gets the color of a single pixel.
 */
void Image::getPixel(int x, int y, unsigned int &r, unsigned int &g, unsigned int &b, unsigned int &a) const {
    createCachedImage();
    CGSize size = CGLayerGetSize(surface);
    const size_t width = size_t(size.width);
    const int bytesPerLine = ((width * 4) + 15) & ~15;
    const char *scanLine = cachedImageData + y * bytesPerLine;
    UInt32 pixel = reinterpret_cast<const UInt32 *>(scanLine)[x];
    r = pixel & 0xff000000;
    g = pixel & 0x00ff0000;
    b = pixel & 0x0000ff00;
    a = pixel & 0x000000ff;
}

/**
 * Gets the palette index of a single pixel.  If the image is in
 * indexed mode, then the index is simply the palette entry number.
 * If the image is RGB, it is a packed RGB triplet.
 */
void Image::getPixelIndex(int x, int y, unsigned int &index) const {
    fprintf(stderr, "Image::getPixelIndex: implement me!!!\n");
    index = 0;
}

void Image::initWithImage(CGImageRef image) {
    CGContextRef context = CGLayerGetContext(surface);
    CGRect rect = CGRectMake(0, 0, width(), height());
    U4IOS::HIViewDrawCGImage(context, &rect, image);    
}

void Image::clearImageContents() {
    CGContextRef context = CGLayerGetContext(surface);
    CGContextGStateSaver saver(context);
    CGSize size = CGLayerGetSize(surface);
    CGContextClearRect(context, CGRectMake(0, 0, size.width, size.height));
}

/**
 * Draws the image onto another image.
 */
void Image::drawOn(Image *d, int x, int y) const {
    if (d == 0) 
        d = [U4IOS::frontU4View() image];
    CGContextRef cgcontext = CGLayerGetContext(d->surface);
    CGContextGStateSaver saver(cgcontext);
    CGContextDrawLayerAtPoint(cgcontext, CGPointMake(x, y), surface);
    clearCachedImageData();
}

/**
 * Draws a piece of the image onto another image.
 */
void Image::drawSubRectOn(Image *d, int x, int y, int rx, int ry, int rw, int rh) const {
    if (d == 0)
        d = [U4IOS::frontU4View() image];
    
    if (!surface)
        return;
    
    boost::intrusive_ptr<CGLayer> currentSurface = surface;
    if (d == this) {
        // If we are drawing on ourselves, we could run into potential data integrity issues
        // (in other words, we don't know how layers are implemented so drawing could be weird).
        // So, make a copy of ourselves and use that for drawing.
        CGLayerRef mySurface = currentSurface.get();
        CGSize size = CGLayerGetSize(mySurface);
        boost::intrusive_ptr<CGLayer> tmpSurface = cftypeFromCreateOrCopy(CGLayerCreateWithContext(CGLayerGetContext(mySurface), size, 0));
        CGContextRef tmpContext = CGLayerGetContext(tmpSurface.get());
        CGContextDrawLayerAtPoint(tmpContext, CGPointMake(0, 0), mySurface);
        tmpSurface.swap(currentSurface);
    }
    CGContextRef context = CGLayerGetContext(d->surface);
    CGContextGStateSaver saver(context);
    CGRect rect = CGRectMake(x, y, rw, rh);
    CGContextClipToRect(context, rect);
    CGContextDrawLayerAtPoint(context, CGPointMake(x - rx, y - ry), currentSurface.get());
    clearCachedImageData();
}

/**
 * Draws a piece of the image onto another image, inverted.
 */
void Image::drawSubRectInvertedOn(Image *d, int x, int y, int rx, int ry, int rw, int rh) const {
    if (d == 0)
        d = [U4IOS::frontU4View() image];
    boost::intrusive_ptr<CGLayer> currentSurface = surface;
    if (d == this) {
        // If we are drawing on ourselves, we could run into potential data integrity issues
        // (in other words, we don't know how layers are implemented so drawing could be weird).
        // So, make a copy of ourselves and use that for drawing.
        CGLayerRef mySurface = currentSurface.get();
        CGSize size = CGLayerGetSize(mySurface);
        boost::intrusive_ptr<CGLayer> tmpSurface = cftypeFromCreateOrCopy(CGLayerCreateWithContext(CGLayerGetContext(mySurface), size, 0));
        CGContextRef tmpContext = CGLayerGetContext(tmpSurface.get());
        CGContextDrawLayerAtPoint(tmpContext, CGPointMake(0, 0), mySurface);
        tmpSurface.swap(currentSurface);
    }
    CGContextRef context = CGLayerGetContext(d->surface);
    CGContextGStateSaver contextSaver(context);
    CGContextClipToRect(context, CGRectMake(x, y, rw, rh));
    CGContextTranslateCTM(context, 0, rh);
    CGContextScaleCTM(context, 1, -1);
    CGContextDrawLayerAtPoint(context, CGPointMake(x - rx, y - ry), currentSurface.get());
    clearCachedImageData();
}

/**
 * Dumps the image to a file.  The file is always saved in .png
 * format.  This is mainly used for debugging.
 */
void Image::save(const string &filename) {
    boost::scoped_array<char> data(new char[width() * height() * 4]);
    boost::intrusive_ptr<CGContext> mycontext = cftypeFromCreateOrCopy(CGBitmapContextCreate(data.get(), width(), height(), 8, width() * 4, genericColorSpace, U4BitmapFlags));
    CGContextRef context = mycontext.get();
    CGContextTranslateCTM(context, 0, height());
    CGContextScaleCTM(context, 1, -1);
    CGContextSetRGBFillColor(context, 0., 0., 0., 0.);
    CGContextFillRect(context, CGRectMake(0, 0, width(), height()));
    CGContextDrawLayerAtPoint(context, CGPointMake(0, 0), surface);
    boost::intrusive_ptr<CGImage> cgimage = cftypeFromCreateOrCopy(CGBitmapContextCreateImage(context));
        
    UIImage *uiImage = [UIImage imageWithCGImage:cgimage.get()];
    NSData *pngData = UIImagePNGRepresentation(uiImage);
    if (pngData == nil) {
        NSLog(@"Image::save: PngData was nil for %s, returning", filename.c_str());
        return;
    }
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES);
    NSString *cachesDirectory = [paths objectAtIndex:0];
    NSString *finalPath = [cachesDirectory stringByAppendingPathComponent:[NSString stringWithUTF8String:filename.c_str()]];
    [pngData writeToFile:finalPath atomically:NO];
}
