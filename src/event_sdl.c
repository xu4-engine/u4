/*
 * $Id$
 */

#include <stdlib.h>
#include <ctype.h>
#include <SDL.h>
#include "u4.h"

#include "context.h"
#include "error.h"
#include "event.h"
#include "screen.h"
#include "settings.h"
#include "u4_sdl.h"

SDL_TimerID timer;

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
    extern int eventTimerGranularity;    

    if (timer != NULL)
        SDL_RemoveTimer(timer);

    /* start the SDL timer */    
    if (u4_SDL_InitSubSystem(SDL_INIT_TIMER) < 0)
        errorFatal("unable to init SDL: %s", SDL_GetError());    
    
    timer = SDL_AddTimer(eventTimerGranularity, &eventCallback, NULL);
}

void eventHandlerDelete() {
    SDL_RemoveTimer(timer);
    u4_SDL_QuitSubSystem(SDL_INIT_TIMER);
}

void eventHandlerSleep(int usec) {    
    SDL_Delay(usec);    
}

void eventHandlerMain(void (*updateScreen)(void)) {
    eventHandlerSetExitFlag(0);

    if (updateScreen)
        (*updateScreen)();
    screenRedrawScreen();

    while (!eventHandlerGetExitFlag()) {
        int processed = 0;
        SDL_Event event;

        SDL_WaitEvent(&event);

        switch (event.type) {
        case SDL_KEYDOWN: {
            int key;

            if (event.key.keysym.sym >= SDLK_a &&
                event.key.keysym.sym <= SDLK_z) {

                if (settings->germanKbd) {
                    if (event.key.keysym.sym == SDLK_z)
                        event.key.keysym.sym = SDLK_y;
                    else if (event.key.keysym.sym == SDLK_y)
                        event.key.keysym.sym = SDLK_z;
                }

                key = event.key.keysym.sym - SDLK_a + 'a';
                if (event.key.keysym.mod & KMOD_SHIFT)
                    key = toupper(key);
                else if (event.key.keysym.mod & KMOD_CTRL)
                    key = event.key.keysym.sym - SDLK_a + 1;
                else if (event.key.keysym.mod & KMOD_ALT)
                    key = event.key.keysym.sym - SDLK_a + 'a' + U4_ALT;
                else if (event.key.keysym.mod & KMOD_META)
                    key = event.key.keysym.sym - SDLK_a + 'a' + U4_META;
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

            /* see if the key was ignored */
            if (!eventHandlerIsKeyIgnored(key)) {
                processed = eventHandlerUniversalKeyHandler(key);
                if (!processed)
                    processed = (*eventHandlerGetKeyHandler())(key, eventHandlerGetKeyHandlerData());                    
            }            

            if (processed) {
                if (updateScreen)
                    (*updateScreen)();
                screenRedrawScreen();
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

/**
 * Returns true if there are no more timer messages currently in the
 * event queue.
 */
int eventHandlerTimerQueueEmpty() {
    SDL_Event event;

    if (SDL_PeepEvents(&event, 1, SDL_PEEKEVENT, SDL_EVENTMASK(SDL_USEREVENT)))
        return 0;
    else
        return 1;
}

int eventKeyboardSetKeyRepeat(int delay, int interval) {    
    return SDL_EnableKeyRepeat(delay, interval);
}
