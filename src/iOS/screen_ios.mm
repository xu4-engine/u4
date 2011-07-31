
/*
 * $Id$
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
 */


// iPad version of the screen functions. 

#include "config.h"
#include "context.h"

#include "debug.h"
#include "event.h"
#include "image.h"
#include "settings.h"
#include "scale.h"
#include "screen.h"
#include "tileset.h"
#include "u4.h"
#include "u4file.h"
#include "utils.h"
#import "u4view.h"
#import "ios_helpers.h"
#import "U4CFHelper.h"

extern bool verbose;

void screenInit_sys() {
}

void screenDelete_sys() {
    if (verbose)
        printf("screen deleted [screenDelete()]\n");
}

/**
 * Attempts to iconify the screen.
 */
void screenIconify() {
}

/**
 * Force a redraw.
 */
void screenRedrawScreen() {
    [U4IOS::frontU4View() setNeedsDisplay];
}

void screenRedrawTextArea(int x, int y, int width, int height) {
    [U4IOS::frontU4View() setNeedsDisplayInRect:CGRectMake(x * CHAR_WIDTH * settings.scale,
                                                        y * CHAR_HEIGHT * settings.scale,
                                                        width * CHAR_WIDTH * settings.scale,
                                                        height * CHAR_HEIGHT * settings.scale)];
}

static void releaseScaleData(void *info, const void *data, size_t size) {
    delete [] static_cast<const char *>(data);
}

/**
 * Scale an image up.  The resulting image will be scale * the
 * original dimensions.  The original image is no longer deleted.
 * n is the number of tiles in the image; each tile is filtered
 * seperately. filter determines whether or not to filter the
 * resulting image.
 */
Image *screenScale(Image *src, int scale, int n, int filter) {
    if (scale == 1)
        return Image::duplicate(src);

    src->createCachedImage();
    CGLayerRef srcLayer = src->getSurface();
    CGSize srcSize = CGLayerGetSize(srcLayer);
    int srcWidth = int(srcSize.width);
    int srcHeight = int(srcSize.height);
    const int bytesPerLine = ((srcWidth * 4) + 15) & ~15;
    boost::intrusive_ptr<CGDataProvider> dataProvider = cftypeFromCreateOrCopy(
                                            CGDataProviderCreateWithData(0, src->cachedImageData,
                                                                         srcHeight * bytesPerLine,
                                                                         releaseScaleData));
    boost::intrusive_ptr<CGImage> cgimage = cftypeFromCreateOrCopy(
                                                        CGImageCreate(srcWidth, srcHeight,
                                                                      8, 32, bytesPerLine,
                                                                      U4IOS::u4colorSpace(),
                                                                      kCGImageAlphaPremultipliedLast | kCGBitmapByteOrderDefault,
                                                                      dataProvider.get(),
                                                                      0, true,
                                                                      kCGRenderingIntentDefault));
    // The CGImage owns the cachedImageData now, so reset it to zero.
    src->cachedImageData = 0;
    
    Image *dest = Image::create(srcWidth * scale, srcHeight * scale, false, Image::HARDWARE);
    CGLayerRef destLayer = dest->getSurface();
    CGContextRef destContext = CGLayerGetContext(destLayer);
    CGInterpolationQuality oldQuality = CGContextGetInterpolationQuality(destContext);
    CGContextSetInterpolationQuality(destContext, kCGInterpolationHigh);
    CGContextDrawImage(destContext, CGRectMake(0., 0., srcWidth * scale, srcHeight * scale), cgimage.get());
    CGContextSetInterpolationQuality(destContext, oldQuality);
    return dest;
}

/**
 * Scale an image down.  The resulting image will be 1/scale * the
 * original dimensions.  The original image is no longer deleted.
 */
Image *screenScaleDown(Image *src, int scale) {
    int x, y;
    Image *dest;
    bool isTransparent;
    unsigned int transparentIndex;
    bool alpha = src->isAlphaOn();
    
    isTransparent = src->getTransparentIndex(transparentIndex);
    
    src->alphaOff();
    
    dest = Image::create(src->width() / scale, src->height() / scale, src->isIndexed(), Image::HARDWARE);
    if (!dest)
        return NULL;
    
    if (dest->isIndexed())
        dest->setPaletteFromImage(src);
    
    for (y = 0; y < src->height(); y+=scale) {
        for (x = 0; x < src->width(); x+=scale) {
            unsigned int index;
            src->getPixelIndex(x, y, index);
            dest->putPixelIndex(x / scale, y / scale, index);
        }
    }
    
    if (isTransparent)
        dest->setTransparentIndex(transparentIndex);
    
    if (alpha)
        src->alphaOn();
    
    return dest;
}
