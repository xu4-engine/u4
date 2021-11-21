/*
 * event_allegro.cpp
 */

#include "screen_allegro.h"
#include "event.h"
#include "screen.h"
#include "settings.h"
#include "xu4.h"

extern bool verbose;


static void handleMouseMotionEvent(int x, int y) {
    if (! xu4.settings->mouseOptions.enabled)
        return;

    MouseArea* area = xu4.eventHandler->mouseAreaForPoint(x, y);
    screenSetMouseCursor(area ? area->cursor : MC_DEFAULT);
}

/*
static void handleActiveEvent(const ALLEGRO_EVENT* event, updateScreenCallback updateScreen) {
    if (event.active.state & SDL_APPACTIVE) {
        // application was previously iconified and is now being restored
        if (event.active.gain) {
            if (updateScreen)
                (*updateScreen)();
        }
    }
}
*/

static void handleMouseButtonDownEvent(EventHandler* handler, const ALLEGRO_EVENT* event, Controller *controller, updateScreenCallback updateScreen) {
    static const uint8_t buttonMap[4] = { 0, 0, 2, 1 };
    MouseArea* area;
    int xu4Button, keyCmd;

    if (! xu4.settings->mouseOptions.enabled)
        return;

    area = handler->mouseAreaForPoint(event->mouse.x, event->mouse.y);
    if (area) {
        // Map ALLEGRO button to what MouseArea uses.
        xu4Button = buttonMap[event->mouse.button & 3];

        keyCmd = area->command[xu4Button];
        if (keyCmd) {
            controller->keyPressed(keyCmd);
            if (updateScreen)
                (*updateScreen)();
        }
    }
}

static void handleKeyDownEvent(const ALLEGRO_EVENT* event,
                               Controller *controller,
                               updateScreenCallback updateScreen) {
    int key;
    int keycode = event->keyboard.keycode;

    switch( keycode ) {
        case ALLEGRO_KEY_UP:
            key = U4_UP;
            break;
        case ALLEGRO_KEY_DOWN:
            key = U4_DOWN;
            break;
        case ALLEGRO_KEY_LEFT:
            key = U4_LEFT;
            break;
        case ALLEGRO_KEY_RIGHT:
            key = U4_RIGHT;
            break;
        case ALLEGRO_KEY_BACKSPACE:
        case ALLEGRO_KEY_DELETE:
            key = U4_BACKSPACE;
            break;
        default:
            if (event->keyboard.unichar > 0)
                key = event->keyboard.unichar & 0x7F;
            else if (keycode >= ALLEGRO_KEY_F1 && keycode <= ALLEGRO_KEY_F12)
                key = U4_FKEY + (keycode - ALLEGRO_KEY_F1);
            else
                key = 0;

            if (event->keyboard.modifiers & ALLEGRO_KEYMOD_ALT)
                key += U4_ALT;
            else if (event->keyboard.modifiers & ALLEGRO_KEYMOD_LWIN)
                key += U4_META;
            break;
    }

#ifdef DEBUG
    xu4.eventHandler->recordKey(key);
#endif

    if (verbose) {
        printf("key event: unicode = %d, sym = %d, mod = %d; translated = %d\n",
               event->keyboard.unichar,
               event->keyboard.keycode,
               event->keyboard.modifiers,
               key);
    }

    /* handle the keypress */
    if (controller->notifyKeyPressed(key)) {
        if (updateScreen)
            (*updateScreen)();
    }
}

void EventHandler::handleInputEvents(Controller* controller,
                                     updateScreenCallback update) {
    ALLEGRO_EVENT event;

    while (al_get_next_event(SA->queue, &event)) {
        switch (event.type) {
        //case ALLEGRO_EVENT_KEY_DOWN:
        case ALLEGRO_EVENT_KEY_CHAR:
            handleKeyDownEvent(&event, controller, update);
            break;
        case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
            handleMouseButtonDownEvent(this, &event, controller, update);
            break;
        case ALLEGRO_EVENT_MOUSE_AXES:
            handleMouseMotionEvent(event.mouse.x, event.mouse.y);
            break;
        /*
        case SDL_ACTIVEEVENT:
            handleActiveEvent(event, updateScreen);
            break;
        */
        case ALLEGRO_EVENT_DISPLAY_CLOSE:
            quitGame();
            break;
#if 0
        // For mobile devices...
        case ALLEGRO_EVENT_DISPLAY_HALT_DRAWING:
        case ALLEGRO_EVENT_DISPLAY_RESUME_DRAWING:
#endif
        default:
            break;
        }
    }
}

/**
 * Sets the key-repeat characteristics of the keyboard.
 */
int EventHandler::setKeyRepeat(int delay, int interval) {
    //return SDL_EnableKeyRepeat(delay, interval);
    return 0;
}
