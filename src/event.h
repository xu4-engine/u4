/*
 * $Id$
 */

#ifndef EVENT_H
#define EVENT_H

typedef enum _State {
    STATE_NORMAL,
    STATE_TALK,
    STATE_TALKING,
    STATE_QUIT
} State;

void eventHandlerMain();
int eventHandlerDefault(SDL_Event *event);
int eventHandlerNormal(SDL_Event *event);
int eventHandlerTalk(SDL_Event *event);
int eventHandlerTalking(SDL_Event *event);
int eventHandlerQuit(SDL_Event *event);

#endif
