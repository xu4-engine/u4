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
char talkBuffer[5];


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
        c->talker = mapPersonAt(c->map, t_x, t_y);
        if (c->talker == NULL) {
            screenMessage("Funny, no\nresponse!\n");
            eventHandlerPopKeyHandler();
        }
        else {
            char buffer[100];
            sprintf(buffer, "\nYou meet\n%s\n\n", c->talker->description);
            if (isupper(buffer[9]))
                buffer[9] = tolower(buffer[9]);
            screenMessage(buffer);
            screenMessage("%s says: I am %s\n\n:", c->talker->pronoun, c->talker->name);
            talkBuffer[0] = '\0';
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
        len = strlen(talkBuffer);
        if (len < 4) {
            talkBuffer[len] = key;
            talkBuffer[len + 1] = '\0';
        }

    } else if (key == '\n' || key == '\r') {

        screenMessage("\n");
        if (talkBuffer[0] == '\0' ||
            strcasecmp(talkBuffer, "bye") == 0) {
            eventHandlerPopKeyHandler();
            screenMessage("Bye.\n");
            return 1;
        } else if (strcasecmp(talkBuffer, "look") == 0) {
            screenMessage("%s says: %s\n", c->talker->pronoun, c->talker->description);
        } else if (strcasecmp(talkBuffer, "name") == 0) {
            screenMessage("%s says: I am %s\n", c->talker->pronoun, c->talker->name);
        } else if (strcasecmp(talkBuffer, "job") == 0) {
            screenMessage("%s\n", c->talker->job);
            if (c->talker->questionTrigger == QTRIGGER_JOB)
                screenMessage("%s\n", c->talker->question);
        } else if (strcasecmp(talkBuffer, "heal") == 0) {
            screenMessage("%s\n", c->talker->health);
            if (c->talker->questionTrigger == QTRIGGER_HEALTH)
                screenMessage("%s\n", c->talker->question);
        } else if (strcasecmp(talkBuffer, c->talker->keyword1) == 0) {
            screenMessage("%s\n", c->talker->response1);
            if (c->talker->questionTrigger == QTRIGGER_KEYWORD1)
                screenMessage("%s\n", c->talker->question);
        } else if (strcasecmp(talkBuffer, c->talker->keyword2) == 0) {
            screenMessage("%s\n", c->talker->response2);
            if (c->talker->questionTrigger == QTRIGGER_KEYWORD2)
                screenMessage("%s\n", c->talker->question);
        } else {
            screenMessage("That I cannot\nhelp thee with.\n");
        }

        screenMessage("\nYour Interest:\n");
        talkBuffer[0] = '\0';
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
