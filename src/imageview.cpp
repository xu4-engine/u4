/*
 * $Id$
 */

#include "debug.h"
#include "error.h"
#include "image.h"
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
    SCALED_VAR
    const SubImage* subimage = info->subImages + sub;
    info->image->drawSubRect(SCALED(x + ox), SCALED(y + oy),
                             SCALED(subimage->x) / info->prescale,
                             SCALED(subimage->y) / info->prescale,
                             SCALED(subimage->width) / info->prescale,
                             SCALED(subimage->height) / info->prescale);
}

/**
 * Draw the image at the optionally specified offset.
 */
void ImageView::draw(Symbol imageName, int x, int y) {
    SCALED_VAR
    ImageInfo *info = xu4.imageMgr->get(imageName);
    if (info) {
        info->image->draw(SCALED(this->x + x), SCALED(this->y + y));
        return;
    }

    const SubImage* subimage = xu4.imageMgr->getSubImage(imageName);
    if (subimage) {
        info = xu4.imageMgr->get(subimage->srcImageName);

        if (info) {
            info->image->drawSubRect(SCALED(this->x + x), SCALED(this->y + y),
                                     SCALED(subimage->x) / info->prescale,
                                     SCALED(subimage->y) / info->prescale,
                                     SCALED(subimage->width) / info->prescale,
                                     SCALED(subimage->height) / info->prescale);
            return;
        }
    }
    errorLoadImage(imageName);
}
