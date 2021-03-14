#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#ifndef IOS
#include <SDL.h>
#else
#include "ios_helpers.h"
#endif

#include "image.h"
#include "imagemgr.h"
#include "settings.h"
#include "view.h"

Image *View::screen = NULL;

View::View(int x, int y, int width, int height)
: x(x), y(y), width(width), height(height), highlighted(false), highlightX(0), highlightY(0), highlightW(0), highlightH(0)
{
    if (screen == NULL)
        screen = imageMgr->get("screen")->image;
}

/**
 * Hook for reinitializing when graphics reloaded.
 */
void View::reinit() {
    screen = imageMgr->get("screen")->image;
}

/**
 * Clear the view to black.
 */
void View::clear() {
    unhighlight();
    screen->fillRect(SCALED(x), SCALED(y), SCALED(width), SCALED(height), 0, 0, 0);
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
    Image *screen = imageMgr->get("screen")->image;

    Image *tmp = Image::create(SCALED(highlightW), SCALED(highlightH), false, Image::SOFTWARE);
    if (!tmp)
        return;

    screen->drawSubRectOn(tmp, 0, 0, SCALED(this->x + highlightX), SCALED(this->y + highlightY), SCALED(highlightW), SCALED(highlightH));
    tmp->drawHighlighted();
    tmp->draw(SCALED(this->x + highlightX), SCALED(this->y + highlightY));
    delete tmp;
}
