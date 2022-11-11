#ifdef IOS
#include "ios_helpers.h"
#endif

#include "image.h"
#include "screen.h"
#include "view.h"
#include "u4.h"
#include "xu4.h"

View::View(int x, int y, int width, int height) :
    x(x), y(y), width(width), height(height),
    highlightX(0), highlightY(0), highlightW(0), highlightH(0),
    highlighted(false)
{
    reinit();
}

/**
 * Hook for reinitializing when graphics reloaded.
 */
void View::reinit() {
    int scale = screenState()->aspectH / U4_SCREEN_H;
    screenRect[0] = x * scale;
    screenRect[1] = (xu4.screenImage->height() - (y + height)) * scale;
    screenRect[2] = width  * scale;
    screenRect[3] = height * scale;
}

/**
 * Clear the view to black.
 */
void View::clear() {
    unhighlight();
    xu4.screenImage->fillRect(x, y, width, height, 0, 0, 0);
}

/**
 * Update the view to the screen.
 */
void View::update() {
    if (highlighted)
        drawHighlighted();
#ifdef IOS
    U4IOS::updateView();
#endif
}

/**
 * Update a piece of the view to the screen.
 */
void View::update(int x, int y, int width, int height) {
    if (highlighted)
        drawHighlighted();
#ifdef IOS
    U4IOS::updateRectInView(x, y, width, height);
#endif
}

/**
 * Highlight a piece of the screen by drawing it in inverted colors.
 */
void View::highlight(int x, int y, int width, int height) {
    highlighted = true;
    highlightX = x;
    highlightY = y;
    highlightW = width;
    highlightH = height;

    update(x, y, width, height);
}

void View::unhighlight() {
    highlighted = false;
    update(highlightX, highlightY, highlightW, highlightH);
    highlightX = highlightY = highlightW = highlightH = 0;
}

void View::drawHighlighted() {
    xu4.screenImage->drawHighlight(x + highlightX, y + highlightY,
                                   highlightW, highlightH);
}
