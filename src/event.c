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
#include "annotation.h"
#include "savegame.h"

void moveAvatar(int dx, int dy);
void attackAtCoord(int x, int y);
void jimmyAtCoord(int x, int y);
void openAtCoord(int x, int y);
void talkAtCoord(int x, int y);

KeyHandlerNode *head = NULL;


void eventHandlerPushKeyHandler(KeyHandler kh) {
    KeyHandlerNode *n = malloc(sizeof(KeyHandlerNode));
    if (n) {
        n->kh = kh;
        n->data = NULL;
        n->next = head;
        head = n;
    }
}

void eventHandlerPushKeyHandlerData(KeyHandler kh, void *data) {
    KeyHandlerNode *n = malloc(sizeof(KeyHandlerNode));
    if (n) {
        n->kh = kh;
        n->data = data;
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

void *eventHandlerGetKeyHandlerData() {
    return head->data;
}

int keyHandlerDefault(int key, void *data) {
    int valid = 1;

    switch (key) {
    case '`':
        printf("x = %d, y = %d, tile = %d\n", c->saveGame->x, c->saveGame->y, mapTileAt(c->map, c->saveGame->x, c->saveGame->y));
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

int keyHandlerNormal(int key, void *data) {
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

    case 'a':
        eventHandlerPushKeyHandlerData(&keyHandlerGetDirection, &attackAtCoord);
        screenMessage("Attack\nDir: ");
        break;

    case 'd':
        portal = mapPortalAt(c->map, c->saveGame->x, c->saveGame->y);
        if (portal && portal->trigger_action == ACTION_DESCEND) {
            c->map = portal->destination;
            screenMessage("Descend!\n\n\020");
        }
        else
            screenMessage("Descend what?\n\020");
        break;

    case 'e':
        portal = mapPortalAt(c->map, c->saveGame->x, c->saveGame->y);
        if (portal && portal->trigger_action == ACTION_ENTER) {

            Context *new = (Context *) malloc(sizeof(Context));
            new->parent = c;
            new->saveGame = (SaveGame *) memcpy(malloc(sizeof(SaveGame)), c->saveGame, sizeof(SaveGame));
            new->map = portal->destination;
            new->annotation = NULL;
            new->saveGame->x = new->map->startx;
            new->saveGame->y = new->map->starty;
            new->line = new->parent->line;
            c = new;

            screenMessage("Enter towne!\n\n%s\n\020", c->map->name);
        } else
            screenMessage("Enter what?\n\020");
        break;

    case 'j':
        eventHandlerPushKeyHandlerData(&keyHandlerGetDirection, &jimmyAtCoord);
        screenMessage("Jimmy\nDir: ");
        break;
        
    case 'k':
        portal = mapPortalAt(c->map, c->saveGame->x, c->saveGame->y);
        if (portal && portal->trigger_action == ACTION_KLIMB) {
            c->map = portal->destination;
            screenMessage("Klimb!\n\n\020");
        }
        else
            screenMessage("Klimb what?\n\020");
        break;

    case 'o':
        eventHandlerPushKeyHandlerData(&keyHandlerGetDirection, &openAtCoord);
        screenMessage("Open\nDir: ");
        break;

    case 'q':
        if (strcmp(c->map->name, "World") != 0) {
            screenMessage("Quit & save\nNot Here!\n");
        } else {
            eventHandlerPushKeyHandler(&keyHandlerQuit);
            screenMessage("Quit & save\nExit (Y/N)? ");
        }
        break;

    case 't':
        eventHandlerPushKeyHandlerData(&keyHandlerGetDirection, &talkAtCoord);
        screenMessage("Talk\nDir: ");
        break;

    case 'z':
        screenMessage("Z-stats\n\n");
        break;

    default:
        valid = 0;
        break;
    }

    return valid || keyHandlerDefault(key, NULL);
}

int keyHandlerGetDirection(int key, void *data) {
    void (*handleDir)(int, int) = (void (*)(int, int)) data;
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

    (*handleDir)(t_x, t_y);

    return valid || keyHandlerDefault(key, NULL);
}

int keyHandlerTalking(int key, void *data) {
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

    return valid || keyHandlerDefault(key, NULL);
}

int keyHandlerQuit(int key, void *data) {
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

    return valid || keyHandlerDefault(key, NULL);
}

void attackAtCoord(int x, int y) {
    if (x == -1 && y == -1)
        return;

    eventHandlerPopKeyHandler();

    screenMessage("attack at %d, %d\n(not implemented)\n", x, y);
}

void jimmyAtCoord(int x, int y) {
    if (x == -1 && y == -1)
        return;

    eventHandlerPopKeyHandler();

    if (!islockeddoor(mapTileAt(c->map, x, y))) {
        screenMessage("Jimmy what?\n");
        return;
    }

    screenMessage("door at %d, %d, unlocked!\n", x, y);
    annotationAdd(x, y, -1, 0x3b);
}

void openAtCoord(int x, int y) {
    if (x == -1 && y == -1)
        return;

    eventHandlerPopKeyHandler();

    if (!isdoor(mapTileAt(c->map, x, y))) {
        screenMessage("Open what?\n");
        return;
    }

    screenMessage("door at %d, %d, opened!\n", x, y);
    annotationAdd(x, y, 4, 0x3e);
}

void talkAtCoord(int x, int y) {
    char buffer[100];
    const Person *talker;

    if (x == -1 && y == -1)
        return;

    eventHandlerPopKeyHandler();

    c->conversation.talker = mapPersonAt(c->map, x, y);
    if (c->conversation.talker == NULL) {
        screenMessage("Funny, no\nresponse!\n");
        return;
    }

    talker = c->conversation.talker;
    c->conversation.question = 0;
    sprintf(buffer, "\nYou meet\n%s\n\n", talker->description);
    if (isupper(buffer[9]))
        buffer[9] = tolower(buffer[9]);
    screenMessage(buffer);
    screenMessage("%s says: I am %s\n\n:", talker->pronoun, talker->name);
    c->conversation.buffer[0] = '\0';
    eventHandlerPushKeyHandler(&keyHandlerTalking);
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

    if (iswalkable(mapTileAt(c->map, newx, newy)) &&
        !mapPersonAt(c->map, newx, newy)) {
	c->saveGame->x = newx;
	c->saveGame->y = newy;
    }
}
