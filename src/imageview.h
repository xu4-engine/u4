/*
 * $Id$
 */

#ifndef IMAGEVIEW_H
#define IMAGEVIEW_H

#include "view.h"

class ImageInfo;

/**
 * A view for displaying bitmap images.
 */
class ImageView : public View {
public:
    ImageView(int x = 0, int y = 0, int width = 320, int height = 200);
    virtual ~ImageView();

    void draw(const ImageInfo* info, int sub, int ox = 0, int oy = 0);
    void draw(Symbol imageName, int x = 0, int y = 0);
};

#endif /* IMAGEVIEW_H */
