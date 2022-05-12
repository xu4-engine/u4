/*
 * event_allegro.cpp
 */

#include "screen_allegro.h"
#include "event.h"
#include "xu4.h"

extern bool verbose;


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
#ifdef _WIN32
            else if (event->keyboard.modifiers & ALLEGRO_KEYMOD_CTRL)
                key = keycode;  // On Linux unichar is the same as keycode here
#endif
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

/*
 * \param waitCon  Input events are passed to this controller if not NULL.
 *                 Otherwise EventHandler::getController() (the currently
 *                 active one) will be used.
 */
void EventHandler::handleInputEvents(Controller* waitCon,
                                     updateScreenCallback update) {
    static const uint8_t mouseButtonMap[4] = { 0, 1, 3, 2 };
    ALLEGRO_EVENT event;
    InputEvent ie;
    Controller* controller = waitCon;

    while (al_get_next_event(SA->queue, &event)) {
        switch (event.type) {
        //case ALLEGRO_EVENT_KEY_DOWN:
        case ALLEGRO_EVENT_KEY_CHAR:
            if (! waitCon)
                controller = getController();
            handleKeyDownEvent(&event, controller, update);
            break;

        case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
            ie.type = CIE_MOUSE_PRESS;
mouse_button:
            if (event.mouse.button < 4)
                ie.n = mouseButtonMap[event.mouse.button];
            else
                ie.n = event.mouse.button;
mouse_pos:
            ie.x = event.mouse.x;
            ie.y = event.mouse.y;
mouse_event:
            ie.state = 0;
            if (! waitCon)
                controller = getController();
            controller->inputEvent(&ie);
            break;

        case ALLEGRO_EVENT_MOUSE_BUTTON_UP:
            ie.type = CIE_MOUSE_RELEASE;
            goto mouse_button;

        case ALLEGRO_EVENT_MOUSE_AXES:
            ie.n = 0;
            if (event.mouse.dz || event.mouse.dw) {
                ie.type = CIE_MOUSE_WHEEL;
                ie.x = event.mouse.dw;
                ie.y = event.mouse.dz;
                goto mouse_event;
            }
            ie.type = CIE_MOUSE_MOVE;
            goto mouse_pos;

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
