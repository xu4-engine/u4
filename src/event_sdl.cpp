/*
 * $Id$
 */

#include <SDL.h>
#include "u4.h"

#include "event.h"

#include "context.h"
#include "screen.h"
#include "settings.h"
#include "xu4.h"

extern bool verbose;

static void handleMouseMotionEvent(const SDL_Event &event) {
    if (! xu4.settings->mouseOptions.enabled)
        return;

    MouseArea *area;
    area = xu4.eventHandler->mouseAreaForPoint(event.button.x, event.button.y);
    screenSetMouseCursor(area ? area->cursor : MC_DEFAULT);
}

static void handleActiveEvent(const SDL_Event &event, updateScreenCallback updateScreen) {
    if (event.active.state & SDL_APPACTIVE) {
        // application was previously iconified and is now being restored
        if (event.active.gain) {
            if (updateScreen)
                (*updateScreen)();
        }
    }
}

static void handleMouseButtonDownEvent(const SDL_Event &event, Controller *controller, updateScreenCallback updateScreen) {
    MouseArea* area;
    int xu4Button, keyCmd;

    if (! xu4.settings->mouseOptions.enabled)
        return;

    area = xu4.eventHandler->mouseAreaForPoint(event.button.x, event.button.y);
    if (area) {
        // Map SDL button to what MouseArea uses.
        xu4Button = event.button.button - 1;
        if (xu4Button > 2)
            xu4Button = 0;

        keyCmd = area->command[xu4Button];
        if (keyCmd) {
            controller->keyPressed(keyCmd);
            if (updateScreen)
                (*updateScreen)();
        }
    }
}

static void handleKeyDownEvent(const SDL_Event &event, Controller *controller, updateScreenCallback updateScreen) {
    int key;

    if (event.key.keysym.unicode != 0)
        key = event.key.keysym.unicode & 0x7F;
    else
        key = event.key.keysym.sym;

    if (event.key.keysym.mod & KMOD_ALT)
#if defined(MACOSX)
        key = U4_ALT + event.key.keysym.sym; // macosx translates alt keys into strange unicode chars
#else
    key += U4_ALT;
#endif
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

#if defined(MACOSX)
    // Mac OS X translates function keys weirdly too
    if ((event.key.keysym.sym >= SDLK_F1) && (event.key.keysym.sym <= SDLK_F15))
        key = U4_FKEY + (event.key.keysym.sym - SDLK_F1);
#endif

#ifdef DEBUG
    xu4.eventHandler->recordKey(key);
#endif

    if (verbose)
        printf("key event: unicode = %d, sym = %d, mod = %d; translated = %d\n",
               event.key.keysym.unicode,
               event.key.keysym.sym,
               event.key.keysym.mod,
               key);

    /* handle the keypress */
    if (controller->notifyKeyPressed(key)) {
        if (updateScreen)
            (*updateScreen)();
    }
}

/*
 * \param waitCon  Input events are passed to this controller if not NULL.
 *                 Otherwise EventHandler::getController() (the currently
 *                 active one) will be used.
 */
void EventHandler::handleInputEvents(Controller* waitCon,
                                     updateScreenCallback update) {
    SDL_Event event;
    Controller* controller = waitCon;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        default:
            break;
        case SDL_KEYDOWN:
            if (! waitCon) controller = getController();
            handleKeyDownEvent(event, controller, update);
            break;

        case SDL_MOUSEBUTTONDOWN:
            if (! waitCon) controller = getController();
            handleMouseButtonDownEvent(event, controller, update);
            break;

        case SDL_MOUSEMOTION:
            handleMouseMotionEvent(event);
            break;

        case SDL_ACTIVEEVENT:
            handleActiveEvent(event, updateScreen);
            break;

        case SDL_QUIT:
            quitGame();
            break;
        }
    }
}

/**
 * Sets the key-repeat characteristics of the keyboard.
 */
int EventHandler::setKeyRepeat(int delay, int interval) {
    return SDL_EnableKeyRepeat(delay, interval);
}
