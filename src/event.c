/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "u4.h"
#include "screen.h"
#include "event.h"
#include "map.h"
#include "person.h"
#include "ttype.h"
#include "context.h"
#include "savegame.h"

void moveAvatar(int dx, int dy);

KeyHandlerNode *head = NULL;


void eventHandlerPushKeyHandler(KeyHandler kh) {
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

KeyHandler eventHandlerGetKeyHandler() {
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

    case U4_UP:
        moveAvatar(0, -1);
        break;

    case U4_DOWN:
        moveAvatar(0, 1);
        break;

    case U4_LEFT:
        moveAvatar(-1, 0);
        break;

    case U4_RIGHT:
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
            new->line = new->parent->line;
            c = new;

            screenMessage("Enter towne!\n\n%s\n\020", c->map->name);
        } else
            screenMessage("Enter what?\n\020");
        break;

    case 'k':
        portal = mapPortalAt(c->map, c->saveGame->x, c->saveGame->y);
        if (portal && portal->trigger_action == ACTION_KLIMB) {

            Context *new = (Context *) malloc(sizeof(Context));
            new->parent = c;
            new->saveGame = (SaveGame *) memcpy(malloc(sizeof(SaveGame)), c->saveGame, sizeof(SaveGame));
            new->map = portal->destination;
            c = new;

            screenMessage("Klimb!\n\n\020");
        } else
            screenMessage("Klimb what?\n\020");
        break;

    case 't':
        eventHandlerPushKeyHandler(&keyHandlerTalk);
        screenMessage("Talk\nDir: ");
        break;

    case 'q':
        if (strcmp(c->map->name, "World") != 0) {
            screenMessage("Quit & save\nNot Here!\n");
        } else {
            eventHandlerPushKeyHandler(&keyHandlerQuit);
            screenMessage("Quit & save\nExit (Y/N)? ");
        }
        break;

    default:
        valid = 0;
        break;
    }

    return valid || keyHandlerDefault(key);
}

int keyHandlerTalk(int key) {
    int valid = 1;
    int t_x = -1, t_y = -1;

    switch (key) {
    case U4_UP:
        screenMessage("North\n");
        t_x = c->saveGame->x;
        t_y = c->saveGame->y - 1;
        break;
    case U4_DOWN:
        screenMessage("South\n");
        t_x = c->saveGame->x;
        t_y = c->saveGame->y + 1;
        break;
    case U4_LEFT:
        screenMessage("West\n");
        t_x = c->saveGame->x - 1;
        t_y = c->saveGame->y;
        break;
    case U4_RIGHT:
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
        c->conversation.talker = mapPersonAt(c->map, t_x, t_y);
        if (c->conversation.talker == NULL) {
            screenMessage("Funny, no\nresponse!\n");
            eventHandlerPopKeyHandler();
        }
        else {
            const Person *talker = c->conversation.talker;
            char buffer[100];

            c->conversation.question = 0;
            sprintf(buffer, "\nYou meet\n%s\n\n", talker->description);
            if (isupper(buffer[9]))
                buffer[9] = tolower(buffer[9]);
            screenMessage(buffer);
            screenMessage("%s says: I am %s\n\n:", talker->pronoun, talker->name);
            c->conversation.buffer[0] = '\0';
            eventHandlerPopKeyHandler();
            eventHandlerPushKeyHandler(&keyHandlerTalking);
        }
    }

    return valid || keyHandlerDefault(key);
}

int keyHandlerTalking(int key) {
    int valid = 1;

    if ((key >= 'a' && key <= 'z') || key == ' ') {
        int len;

        screenMessage("%c", key);
        len = strlen(c->conversation.buffer);
        if (len < 4) {
            c->conversation.buffer[len] = key;
            c->conversation.buffer[len + 1] = '\0';
        }

    } else if (key == '\n' || key == '\r') {
        int done;
        char *reply;
        int askq;

        screenMessage("\n");

        if (c->conversation.question)
            done = personGetQuestionResponse(c->conversation.talker, c->conversation.buffer, &reply, &askq);
        else
            done = personGetResponse(c->conversation.talker, c->conversation.buffer, &reply, &askq);
        screenMessage("\n%s\n", reply);
        
        c->conversation.question = askq;
        c->conversation.buffer[0] = '\0';

        if (askq)
            screenMessage("%s\n\nYou say: ", c->conversation.talker->question);
        else if (done)
            eventHandlerPopKeyHandler();
        else
            screenMessage("Your Interest:\n");

    } else {
        valid = 0;
    }

    return valid || keyHandlerDefault(key);
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
            eventHandlerPopKeyHandler();            
        }
    }

    return valid || keyHandlerDefault(key);
}

void moveAvatar(int dx, int dy) {
    int newx, newy;

    newx = c->saveGame->x + dx;
    newy = c->saveGame->y + dy;

    if (MAP_IS_OOB(c->map, newx, newy)) {
	switch (c->map->border_behavior) {
	case BORDER_WRAP:
	    if (newx < 0)
		newx += c->map->width;
	    if (newy < 0)
		newy += c->map->height;
	    if (newx >= c->map->width)
		newx -= c->map->width;
	    if (newy >= c->map->height)
		newy -= c->map->height;
	    break;

	case BORDER_EXIT2PARENT:
	    if (c->parent != NULL) {
		Context *t = c;
                c->parent->line = c->line;
		c = c->parent;
                free(t->saveGame);
		free(t);
	    }
	    return;
	    
	case BORDER_FIXED:
	    if (newx < 0 || newx >= c->map->width)
		newx = c->saveGame->x;
	    if (newy < 0 || newy >= c->map->height)
		newy = c->saveGame->y;
	    break;
	}
    }

    if (/*iswalkable(MAP_TILE_AT(c->map, newx, newy)) &&*/
        !mapPersonAt(c->map, newx, newy)) {
	c->saveGame->x = newx;
	c->saveGame->y = newy;
    }
}
