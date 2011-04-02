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

View::View(int x, int y, int width, int height) {
    this->x = x;
    this->y = y;
    this->width = width;
    this->height = height;
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
    screen->fillRect(SCALED(x), SCALED(y), SCALED(width), SCALED(height), 0, 0, 0);
}

/**
 * Update the view to the screen.
 */
void View::update() {
#ifndef IOS
    SDL_UpdateRect(SDL_GetVideoSurface(), SCALED(x), SCALED(y), SCALED(width), SCALED(height));
#else
    U4IOS::updateView();
#endif
}

/**
 * Update a piece of the view to the screen.
 */
void View::update(int x, int y, int width, int height) {
#ifndef IOS
    SDL_UpdateRect(SDL_GetVideoSurface(), SCALED(this->x + x), SCALED(this->y + y), SCALED(width), SCALED(height));
#else
    U4IOS::updateRectInView(x, y, width, height);
#endif
}

/**
 * Highlight a piece of the screen by drawing it in inverted colors.
 */ 
void View::highlight(int x, int y, int width, int height) {
    Image *screen = imageMgr->get("screen")->image;

    Image *tmp = Image::create(SCALED(width), SCALED(height), false, Image::SOFTWARE);
    if (!tmp)
        return;

    screen->drawSubRectOn(tmp, 0, 0, SCALED(this->x + x), SCALED(this->y + y), SCALED(width), SCALED(height));
    tmp->drawHighlighted();
    tmp->draw(SCALED(this->x + x), SCALED(this->y + y));
    delete tmp;

    update(x, y, width, height);
}
