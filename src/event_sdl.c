/*
 * $Id$
 */

#include <stdlib.h>
#include <ctype.h>
#include <SDL.h>

#include "u4.h"
#include "event.h"
#include "screen.h"
#include "context.h"

int eventDone;

Uint32 eventCallback(Uint32 interval, void *param) {
    SDL_Event event;

    event.type = SDL_USEREVENT;
    event.user.code = 0;
    event.user.data1 = NULL;
    event.user.data2 = NULL;
    SDL_PushEvent(&event);

    return interval;
}

void eventHandlerInit() {
    SDL_AddTimer(250, &eventCallback, NULL);
}

void eventHandlerMain(void (*updateScreen)(void)) {
    eventHandlerSetExitFlag(0);

    if (updateScreen)
        (*updateScreen)();
    screenForceRedraw();

    while (!eventHandlerGetExitFlag()) {
        int processed = 0;
        SDL_Event event;

        SDL_WaitEvent(&event);

        switch (event.type) {
        case SDL_KEYDOWN: {
            int key;

            if (event.key.keysym.sym >= SDLK_a &&
                event.key.keysym.sym <= SDLK_z) {
                key = event.key.keysym.sym - SDLK_a + 'a';
                if (event.key.keysym.mod & (KMOD_LSHIFT | KMOD_RSHIFT))
                    key = toupper(key);
                else if (event.key.keysym.mod & (KMOD_LCTRL | KMOD_RCTRL))
                    key = event.key.keysym.sym - SDLK_a + 1;
            }
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

            processed = (*eventHandlerGetKeyHandler())(key, eventHandlerGetKeyHandlerData());

            if (processed) {
                if (updateScreen)
                    (*updateScreen)();
                screenForceRedraw();
            }
            break;
        }
        case SDL_USEREVENT:
            eventHandlerCallTimerCallbacks();
            break;

        case SDL_QUIT:
            exit(0);
            break;
        }
    }
}

