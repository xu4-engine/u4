/*
 * $Id$
 */

#include "progress_bar.h"

#include "settings.h"
#include "screen.h"
#include "xu4.h"

ProgressBar::ProgressBar(int x, int y, int width, int height, int _min, int _max) :
    View(x, y, width, height),
    min(_min),
    max(_max) {
    current = min;
}

ProgressBar& ProgressBar::operator++()  { current++; draw(); return *this; }
ProgressBar& ProgressBar::operator--()  { current--; draw(); return *this; }
void ProgressBar::draw() {
    Image *bar = Image::create(width, height);
    int pos = static_cast<int>((double(current - min) / double(max - min)) * (width - (bwidth * 2)));

    // border color
    bar->fillRect(0, 0, width, height, bcolor.r, bcolor.g, bcolor.b);

    // color
    bar->fillRect(bwidth, bwidth, pos, height - (bwidth * 2), color.r, color.g, color.b);

    bar->draw(x, y);
    update();
    screenSwapBuffers();

    delete bar;
}

void ProgressBar::setBorderColor(int r, int g, int b, int a) {
    bcolor.r = r;
    bcolor.g = g;
    bcolor.b = b;
    bcolor.a = a;
}

void ProgressBar::setBorderWidth(unsigned int width) {
    bwidth = width;
}

void ProgressBar::setColor(int r, int g, int b, int a) {
    color.r = r;
    color.g = g;
    color.b = b;
    color.a = a;
}
