/*
 * $Id$
 */

#include <stdlib.h>
#include <SDL/SDL.h>

#include "u4.h"
#include "screen.h"
#include "event.h"
#include "context.h"

void eventHandlerMain() {
    eventHandlerPushKeyHandler(&keyHandlerNormal);

    screenMessage("\020");
    while (1) {
        int processed = 0;
        SDL_Event event;

        SDL_WaitEvent(&event);

        switch (event.type) {
        case SDL_KEYDOWN: {
            int key;

            if (event.key.keysym.sym >= SDLK_a &&
                event.key.keysym.sym <= SDLK_z)
                key = event.key.keysym.sym - SDLK_a + 'a';
            else if (event.key.keysym.sym == SDLK_UP)
                key = U4_UP;
            else if (event.key.keysym.sym == SDLK_DOWN)
                key = U4_DOWN;
            else if (event.key.keysym.sym == SDLK_LEFT)
                key = U4_LEFT;
            else if (event.key.keysym.sym == SDLK_RIGHT)
                key = U4_RIGHT;
            else if (event.key.keysym.sym == SDLK_BACKSPACE ||
                     event.key.keysym.sym == SDLK_DELETE)
                key = U4_BACKSPACE;
            else
                key = event.key.keysym.sym;

            processed = (*eventHandlerGetKeyHandler())(key);

            if (processed) {
                screenUpdate(c);
                screenForceRedraw();
            }
            break;
        }
        case SDL_QUIT:
            exit(0);
            break;
        }
    }
}
