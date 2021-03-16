/*
 *  imageloader_png_ios.cpp
 *  xu4
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
#include <CoreGraphics/CoreGraphics.h>
#include <cassert>
#include "debug.h"
#include "image.h"
#include "imageloader.h"
#include "imageloader_png.h"
#include "U4CFHelper.h"

ImageLoader *PngImageLoader::instance = ImageLoader::registerLoader(new PngImageLoader, "image/png");

static void releaseData(void *, const void *data, size_t)
{
    delete [] static_cast<const char *>(data);
}


/**
 * Loads in the PNG with the libpng library.
 */
Image *PngImageLoader::load(U4FILE *file, int width, int height, int bpp) {
    int fileLength = file->length();
    char *raw = new char[fileLength];
    int check = file->read(raw, 1, fileLength);
    assert(check == fileLength);
    boost::intrusive_ptr<CGDataProvider> dataProvider = cftypeFromCreateOrCopy(CGDataProviderCreateWithData(this, raw, fileLength, releaseData));
    boost::intrusive_ptr<CGImage> cgimage = cftypeFromCreateOrCopy(CGImageCreateWithPNGDataProvider(dataProvider.get(), 0, false, kCGRenderingIntentDefault));
    
    int imageWidth = CGImageGetWidth(cgimage.get());
    int imageHeight = CGImageGetHeight(cgimage.get());
    Image *image = Image::create(imageWidth, imageHeight, false);
    image->initWithImage(cgimage.get());
    return image;
}
