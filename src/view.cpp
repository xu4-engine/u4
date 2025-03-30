#include "image.h"
#include "screen.h"
#include "view.h"
#include "u4.h"
#include "xu4.h"

View::View(int x, int y, int width, int height) :
    x(x), y(y), width(width), height(height),
    highlightX(0), highlightY(0), highlightW(0), highlightH(0)
{
    reinit();
}

/**
 * Hook for reinitializing when graphics reloaded.
 */
void View::reinit() {
#ifdef GPU_RENDER
    int scale = screenState()->aspectH / U4_SCREEN_H;
    screenRect[0] = x * scale;
    screenRect[1] = (xu4.screenImage->height() - (y + height)) * scale;
    screenRect[2] = width  * scale;
    screenRect[3] = height * scale;
#endif
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
    drawHighlighted();
}

/**
 * Update a piece of the view to the screen.
 */
void View::update(int x, int y, int width, int height) {
    drawHighlighted();
}

/**
 * Set highlight (inverted colors) rectangle.
 */
void View::setHighlight(int x, int y, int width, int height) {
    highlightX = x;
    highlightY = y;
    highlightW = width;
    highlightH = height;
}

/**
 * Highlight a piece of the screen by drawing it in inverted colors.
 */
void View::highlight(int x, int y, int width, int height) {
    setHighlight(x, y, width, height);
    update(x, y, width, height);
}

void View::unhighlight() {
    update(highlightX, highlightY, highlightW, highlightH);
    highlightW = highlightH = 0;
}

void View::drawHighlighted() {
    if (highlightW > 0)
        xu4.screenImage->drawHighlight(x + highlightX, y + highlightY,
                                       highlightW, highlightH);
}
