#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include <SDL.h>

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
 * Clear the view to black.
 */
void View::clear() {
    screen->fillRect(SCALED(x), SCALED(y), SCALED(width), SCALED(height), 0, 0, 0);
}

/**
 * Update the view to the screen.
 */
void View::update() {
    SDL_UpdateRect(SDL_GetVideoSurface(), SCALED(x), SCALED(y), SCALED(width), SCALED(height));    
}

/**
 * Update a piece of the view to the screen.
 */
void View::update(int x, int y, int height, int width) {
    SDL_UpdateRect(SDL_GetVideoSurface(), SCALED(this->x + x), SCALED(this->y + y), SCALED(width), SCALED(height));
}
