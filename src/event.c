#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <SDL/SDL.h>

#include "u4.h"
#include "screen.h"
#include "event.h"
#include "map.h"
#include "person.h"
#include "ttype.h"
#include "context.h"
#include "savegame.h"

void eventHandlerMain() {
    screenMessage("\020");
    while (1) {
        int processed = 0;
        SDL_Event event;

        SDL_WaitEvent(&event);

        switch (c->state) {
        case STATE_NORMAL:
            processed = eventHandlerNormal(&event);
            break;
        case STATE_TALK:
            processed = eventHandlerTalk(&event);
            break;
        case STATE_TALKING:
            processed = eventHandlerTalking(&event);
            break;
        case STATE_QUIT:
            processed = eventHandlerQuit(&event);
            break;
        }
    }
}

int eventHandlerDefault(SDL_Event *event) {
    int processed = 1;

    switch (event->type) {
    case SDL_KEYDOWN:
        switch (event->key.keysym.sym) {
        case SDLK_HOME:
            printf("x = %d, y = %d, tile = %d\n", c->x, c->y, MAP_TILE_AT(c->map, c->x, c->y));
            break;
        case SDLK_m:
            screenMessage("0123456789012345\n");
            break;
        default:
            processed = 0;
            break;
        }
        screenUpdate(c);
        SDL_UpdateRect(screen, 0, 0, 0, 0);
        break;

    case SDL_QUIT:
        exit(0);
        break;

    default:
        processed = 0;
        break;
    }

    return processed;
}

int eventHandlerNormal(SDL_Event *event) {
    int processed = 1;
    const Portal *portal;

    switch (event->type) {
    case SDL_KEYDOWN:
        switch (event->key.keysym.sym) {

        case SDLK_UP:
            move(0, -1);
            break;

        case SDLK_DOWN:
            move(0, 1);
            break;

        case SDLK_LEFT:
            move(-1, 0);
            break;

        case SDLK_RIGHT:
            move(1, 0);
            break;

        case SDLK_e:
            portal = mapPortalAt(c->map, c->x, c->y);
            if (portal && portal->trigger_action == ACTION_ENTER) {

                Context *new = (Context *) malloc(sizeof(Context));
                new->parent = c;
                new->map = portal->destination;
                new->x = new->map->startx;
                new->y = new->map->starty;
                new->state = STATE_NORMAL;
                new->line = new->parent->line;
                c = new;

                screenMessage("Enter towne!\n\n%s\n\020", c->map->name);
            } else
                screenMessage("Enter what?\n\020");
            break;

        case SDLK_t:
            c->state = STATE_TALK;
            screenMessage("Talk\nDir: ");
            break;

        case SDLK_q:
            if (strcmp(c->map->name, "World") != 0) {
                screenMessage("Quit & save\nNot Here!\n");
            } else {
                c->state = STATE_QUIT;
                screenMessage("Quit & save\nExit (Y/N)? ");
            }
            break;

        default:
            processed = 0;
            break;
        }
        break;

    default:
        processed = 0;
        break;
    }

    return eventHandlerDefault(event) || processed;
}

int eventHandlerTalk(SDL_Event *event) {
    int processed = 1;
    const Person *p;
    int t_x = -1, t_y = -1;

    switch (event->type) {
    case SDL_KEYDOWN:
        switch (event->key.keysym.sym) {
        case SDLK_UP:
            screenMessage("North\n");
            t_x = c->x;
            t_y = c->y - 1;
            break;
        case SDLK_DOWN:
            screenMessage("South\n");
            t_x = c->x;
            t_y = c->y + 1;
            break;
        case SDLK_LEFT:
            screenMessage("West\n");
            t_x = c->x - 1;
            t_y = c->y;
            break;
        case SDLK_RIGHT:
            screenMessage("East\n");
            t_x = c->x + 1;
            t_y = c->y;
            break;
        default:
            processed = 0;
            break;
        }
        if (t_x != -1 ||
            t_y != -1) {
            p = mapPersonAt(c->map, t_x, t_y);
            if (p == NULL) {
                screenMessage("Funny, no\nresponse!\n");
                c->state = STATE_NORMAL;
            }
            else {
                char buffer[100];
                sprintf(buffer, "You meet\n%s\n\n", p->description);
                if (isupper(buffer[9]))
                    buffer[9] = tolower(buffer[9]);
                screenMessage(buffer);
                sprintf(buffer, "%s says: I am %s\n\n", p->pronoun, p->name);
                screenMessage(buffer);
                c->state = STATE_TALKING;
            }
        }

        break;

    default:
        processed = 0;
        break;
    }

    return eventHandlerDefault(event) || processed;
}

int eventHandlerTalking(SDL_Event *event) {
    int processed = 1;

    switch (event->type) {
    case SDL_KEYDOWN:
        if (event->key.keysym.sym >= SDLK_a &&
            event->key.keysym.sym <= SDLK_z) {

            screenMessage("%c", event->key.keysym.sym - SDLK_a + 'a');

        } else if (event->key.keysym.sym == SDLK_RETURN) {

            screenMessage("\n");
            c->state = STATE_NORMAL;
            
        } else {
            processed = 0;
        }

        break;

    default:
        processed = 0;
        break;
    }

    return eventHandlerDefault(event) || processed;
}

int eventHandlerQuit(SDL_Event *event) {
    FILE *saveGameFile;
    int processed = 1;
    char answer = '?';

    switch (event->type) {
    case SDL_KEYDOWN:
        switch (event->key.keysym.sym) {
        case SDLK_y:
            answer = 'y';
            break;
        case SDLK_n:
            answer = 'n';
            break;
        default:
            processed = 0;
            break;
        }
        break;

    default:
        processed = 0;
        break;
    }

    if (answer == 'y' || answer == 'n') {
        saveGameFile = fopen("party.sav", "w");
        if (saveGameFile) {
            c->saveGame->x = c->x;
            c->saveGame->y = c->y;
            saveGameWrite(c->saveGame, saveGameFile);
            fclose(saveGameFile);
        } else {
            screenMessage("Error writing to\nparty.sav\n");
            answer = '?';
        }

        if (answer == 'y')
            exit(0);
        else if (answer == 'n') {
            screenMessage("%c\n", event->key.keysym.sym - SDLK_a + 'a');
            c->state = STATE_NORMAL;
        }
    }
        
    return eventHandlerDefault(event) || processed;
}

