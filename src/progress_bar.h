/*
 * $Id$
 */

#ifndef PROGRESS_BAR_H
#define PROGRESS_BAR_H

#include "image.h"
#include "view.h"

class ProgressBar : public View {
public:
    ProgressBar(int x, int y, int width, int height, int min, int max);

    virtual ProgressBar& operator++();
    virtual ProgressBar& operator--();
    virtual void draw();
    void setBorderColor(int r, int g, int b, int a = IM_OPAQUE);
    void setBorderWidth(unsigned int width);
    void setColor(int r, int g, int b, int a = IM_OPAQUE);

protected:
    int min, max;
    int current;
    RGBA color, bcolor;
    int bwidth;
};

#endif
