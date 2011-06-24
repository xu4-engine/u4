/*
 * imageloader_fmtowns.h
 *
 *  Created on: Jun 23, 2011
 *      Author: Darren
 */

#ifndef IMAGELOADER_FMTOWNS_H_
#define IMAGELOADER_FMTOWNS_H_

#include "imageloader.h"

class FMTOWNSImageLoader : public ImageLoader {
    static ImageLoader *instance_pic;
    static ImageLoader *instance_tif;
public:
    virtual Image *load(U4FILE *file, int width, int height, int bpp);
    FMTOWNSImageLoader(int offset) : offset(offset){}
protected:
    int offset;
};

#endif /* IMAGELOADER_FMTOWNS_H_ */
