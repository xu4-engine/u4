/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include <SDL.h>
#include "u4.h"

#include "context.h"
#include "error.h"
#include "event.h"
#include "screen.h"
#include "settings.h"
#include "u4_sdl.h"

extern bool verbose;

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
    
    SDL_EnableUNICODE(1);

    timer = SDL_AddTimer(eventTimerGranularity, &eventCallback, NULL);
}

void eventHandlerDelete() {
    SDL_RemoveTimer(timer);
    u4_SDL_QuitSubSystem(SDL_INIT_TIMER);
}

void eventHandlerSleep(int usec) {    
    SDL_Delay(usec);    
}

static MouseArea *eventMouseAreaForPoint(int x, int y) {
    int i;
    MouseArea *areas = eventHandlerGetMouseAreaSet();

    if (!areas)
        return NULL;

    for (i = 0; areas[i].npoints != 0; i++) {
        if (screenPointInMouseArea(x, y, &(areas[i]))) {
            return &(areas[i]);
        }
    }
    return NULL;
}

void eventHandlerMain(void (*updateScreen)(void)) {
    eventHandlerSetExitFlag(false);

    if (updateScreen)
        (*updateScreen)();
    screenRedrawScreen();

    while (!eventHandlerGetExitFlag()) {
        int processed = 0;
        SDL_Event event;
        MouseArea *area;

        SDL_WaitEvent(&event);

        switch (event.type) {
        case SDL_KEYDOWN: {
            int key;

            if (event.key.keysym.unicode != 0)
                key = event.key.keysym.unicode & 0x7F;
            else
                key = event.key.keysym.sym;

            if (event.key.keysym.mod & KMOD_ALT)
                key += U4_ALT;
            if (event.key.keysym.mod & KMOD_META)
                key += U4_META;

            if (event.key.keysym.sym == SDLK_UP)
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

            if (verbose)
                printf("key event: unicode = %d, sym = %d, mod = %d; translated = %d\n", 
                       event.key.keysym.unicode, 
                       event.key.keysym.sym, 
                       event.key.keysym.mod, 
                       key);

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

        case SDL_MOUSEBUTTONDOWN: {
            int button = event.button.button - 1;

            if (!settings.mouseOptions.enabled)
                break;

            if (button > 2)
                button = 0;
            area = eventMouseAreaForPoint(event.button.x, event.button.y);
            if (!area || area->command[button] == 0)
                break;
            (*eventHandlerGetKeyHandler())(area->command[button], eventHandlerGetKeyHandlerData());
            if (updateScreen)
                (*updateScreen)();
            screenRedrawScreen();
            break;
        }

        case SDL_MOUSEMOTION:
            if (!settings.mouseOptions.enabled)
                break;

            area = eventMouseAreaForPoint(event.button.x, event.button.y);
            if (area)
                screenSetMouseCursor(area->cursor);
            else
                screenSetMouseCursor(MC_DEFAULT);
            break;

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
bool eventHandlerTimerQueueEmpty() {
    SDL_Event event;

    if (SDL_PeepEvents(&event, 1, SDL_PEEKEVENT, SDL_EVENTMASK(SDL_USEREVENT)))
        return false;
    else
        return true;
}

int eventKeyboardSetKeyRepeat(int delay, int interval) {    
    return SDL_EnableKeyRepeat(delay, interval);
}
