/*
 * $Id$
 */

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

char talkBuffer[5];

#define UP '['
#define DOWN '/'
#define LEFT ';'
#define RIGHT '\''

void eventHandlerMain() {
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
                key = UP;
            else if (event.key.keysym.sym == SDLK_DOWN)
                key = DOWN;
            else if (event.key.keysym.sym == SDLK_LEFT)
                key = LEFT;
            else if (event.key.keysym.sym == SDLK_RIGHT)
                key = RIGHT;
            else
                key = event.key.keysym.sym;

            switch (c->state) {
            case STATE_NORMAL:
                processed = keyHandlerNormal(key);
                break;
            case STATE_TALK:
                processed = keyHandlerTalk(key);
                break;
            case STATE_TALKING:
                processed = keyHandlerTalking(key);
                break;
            case STATE_QUIT:
                processed = keyHandlerQuit(key);
                break;
            }

            screenUpdate(c);
            screenForceRedraw();
            break;
        }
        case SDL_QUIT:
            exit(0);
            break;
        }
    }
}

void eventHandlerPushKeyHandler(KeyHandler *kh) {
    KeyHandlerNode *n = malloc(sizeof(KeyHandlerNode));
    if (n) {
        n->kh = kh;
        n->next = head;
        head = n;
    }
}

void eventHandlerPopKeyHandler() {
    KeyHandlerNode *n = head;
    if (n) {
        head = n->next;
        free(n);
    }
}

KeyHandler *eventHandlerGetKeyHandler() {
    return head->kh;
}

int keyHandlerDefault(int key) {
    int valid = 1;

    switch (key) {
    case '`':
        printf("x = %d, y = %d, tile = %d\n", c->saveGame->x, c->saveGame->y, MAP_TILE_AT(c->map, c->saveGame->x, c->saveGame->y));
        break;
    case 'm':
        screenMessage("0123456789012345\n");
        break;
    default:
        valid = 0;
        break;
    }

    return valid;
}

int keyHandlerNormal(int key) {
    int valid = 1;
    const Portal *portal;

    switch (key) {

    case UP:
        moveAvatar(0, -1);
        break;

    case DOWN:
        moveAvatar(0, 1);
        break;

    case LEFT:
        moveAvatar(-1, 0);
        break;

    case RIGHT:
        moveAvatar(1, 0);
        break;

    case 'e':
        portal = mapPortalAt(c->map, c->saveGame->x, c->saveGame->y);
        if (portal && portal->trigger_action == ACTION_ENTER) {

            Context *new = (Context *) malloc(sizeof(Context));
            new->parent = c;
            new->saveGame = (SaveGame *) memcpy(malloc(sizeof(SaveGame)), c->saveGame, sizeof(SaveGame));
            new->map = portal->destination;
            new->saveGame->x = new->map->startx;
            new->saveGame->y = new->map->starty;
            new->state = STATE_NORMAL;
            new->line = new->parent->line;
            c = new;

            screenMessage("Enter towne!\n\n%s\n\020", c->map->name);
        } else
            screenMessage("Enter what?\n\020");
        break;

    case 't':
        c->state = STATE_TALK;
        screenMessage("Talk\nDir: ");
        break;

    case 'q':
        if (strcmp(c->map->name, "World") != 0) {
            screenMessage("Quit & save\nNot Here!\n");
        } else {
            c->state = STATE_QUIT;
            screenMessage("Quit & save\nExit (Y/N)? ");
        }
        break;

    default:
        valid = 0;
        break;
    }

    return keyHandlerDefault(key) || valid;
}

int keyHandlerTalk(int key) {
    int valid = 1;
    const Person *p;
    int t_x = -1, t_y = -1;

    switch (key) {
    case UP:
        screenMessage("North\n");
        t_x = c->saveGame->x;
        t_y = c->saveGame->y - 1;
        break;
    case DOWN:
        screenMessage("South\n");
        t_x = c->saveGame->x;
        t_y = c->saveGame->y + 1;
        break;
    case LEFT:
        screenMessage("West\n");
        t_x = c->saveGame->x - 1;
        t_y = c->saveGame->y;
        break;
    case RIGHT:
        screenMessage("East\n");
        t_x = c->saveGame->x + 1;
        t_y = c->saveGame->y;
        break;
    default:
        valid = 0;
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
            talkBuffer[0] = '\0';
            c->state = STATE_TALKING;
        }
    }

    return keyHandlerDefault(key) || valid;
}

int keyHandlerTalking(int key) {
    int valid = 1;

    if (key >= 'a' &&
        key <= 'z') {

        int len;

        screenMessage("%c", key);
        len = strlen(talkBuffer);
        if (len < 4) {
            talkBuffer[len] = key;
            talkBuffer[len + 1] = '\0';
        }

    } else if (key == '\n') {

        screenMessage("\n");
        screenMessage("%s\n", talkBuffer);
        c->state = STATE_NORMAL;
            
    } else {
        valid = 0;
    }

    return keyHandlerDefault(key) || valid;
}

int keyHandlerQuit(int key) {
    FILE *saveGameFile;
    int valid = 1;
    char answer = '?';

    switch (key) {
    case 'y':
    case 'n':
        answer = key;
        break;
    default:
        valid = 0;
        break;
    }

    if (answer == 'y' || answer == 'n') {
        saveGameFile = fopen("party.sav", "w");
        if (saveGameFile) {
            saveGameWrite(c->saveGame, saveGameFile);
            fclose(saveGameFile);
        } else {
            screenMessage("Error writing to\nparty.sav\n");
            answer = '?';
        }

        if (answer == 'y')
            exit(0);
        else if (answer == 'n') {
            screenMessage("%c\n", key);
            c->state = STATE_NORMAL;
        }
    }

    return keyHandlerDefault(key) || valid;
}

