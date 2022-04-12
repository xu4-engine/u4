/*
 * $Id$
 */

#include "error.h"
#include "imagemgr.h"
#include "imageview.h"
#include "settings.h"
#include "xu4.h"

ImageView::ImageView(int x, int y, int width, int height) : View(x, y, width, height) {
}

ImageView::~ImageView() {
}

/**
 * Draw the image at the optionally specified offset.
 */
void ImageView::draw(const ImageInfo* info, int sub, int ox, int oy) {
    const SubImage* subimage = info->subImages + sub;
    info->image->drawSubRect(x + ox, y + oy,
                             subimage->x, subimage->y,
                             subimage->width, subimage->height);
}

/**
 * Draw the image at the optionally specified offset.
 */
void ImageView::draw(Symbol imageName, int x, int y) {
    const SubImage* subimage;
    ImageInfo *info = xu4.imageMgr->imageInfo(imageName, &subimage);
    if (! info) {
        errorLoadImage(imageName);
    } else if (subimage) {
        info->image->drawSubRect(this->x + x, this->y + y,
                                 subimage->x, subimage->y,
                                 subimage->width, subimage->height);
    } else
        info->image->draw(this->x + x, this->y + y);
}
