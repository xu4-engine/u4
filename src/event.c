/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "u4.h"
#include "screen.h"
#include "event.h"
#include "map.h"
#include "person.h"
#include "ttype.h"
#include "context.h"
#include "annotation.h"
#include "savegame.h"
#include "stats.h"
#include "spell.h"

void moveAvatar(int dx, int dy);
int attackAtCoord(int x, int y);
int jimmyAtCoord(int x, int y);
int openAtCoord(int x, int y);
int talkAtCoord(int x, int y);

KeyHandlerNode *head = NULL;

typedef struct DirectedActionInfo {
    int (*handleAtCoord)(int, int);
    int range;
    int (*blockedPredicate)(unsigned char tile);
    const char *failedMessage;
} DirectedActionInfo;


/**
 * Push a key handler onto the top of the keyhandler stack.
 */
void eventHandlerPushKeyHandler(KeyHandler kh) {
    KeyHandlerNode *n = malloc(sizeof(KeyHandlerNode));
    if (n) {
        n->kh = kh;
        n->data = NULL;
        n->next = head;
        head = n;
    }
}

/**
 * Push a key handler onto the top of the key handler stack, and
 * provide specific data to pass to the handler.
 */
void eventHandlerPushKeyHandlerData(KeyHandler kh, void *data) {
    KeyHandlerNode *n = malloc(sizeof(KeyHandlerNode));
    if (n) {
        n->kh = kh;
        n->data = data;
        n->next = head;
        head = n;
    }
}

/**
 * Pop the top key handler off.
 */
void eventHandlerPopKeyHandler() {
    KeyHandlerNode *n = head;
    if (n) {
        head = n->next;
        free(n);
    }
}

/**
 * Get the currently active key handler of the top of the key handler
 * stack.
 */
KeyHandler eventHandlerGetKeyHandler() {
    return head->kh;
}

/**
 * Get the call data associated with the currently active key handler.
 */
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
    DirectedActionInfo *info;
    SpellCastError spellError;

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
        info = (DirectedActionInfo *) malloc(sizeof(DirectedActionInfo));
        info->handleAtCoord = &attackAtCoord;
        info->range = 1;
        info->blockedPredicate = NULL;
        info->failedMessage = "FIXME";
        eventHandlerPushKeyHandlerData(&keyHandlerGetDirection, info);
        screenMessage("Attack\nDir: ");
        break;

    case 'c':
        if (!spellCast(1, 0, &spellError)) {
            switch(spellError) {
            case CASTERR_NOMIX:
                screenMessage("None Mixed!\n");
                break;
            case CASTERR_WRONGCONTEXT:
                screenMessage("Can't Cast Here!\n");
                break;
            case CASTERR_MPTOOLOW:
                screenMessage("Not Enough MP!\n");
                break;
            case CASTERR_NOERROR:
            default:
                /* should never happen */
                assert(0);
            }
            break;
        }
        screenMessage("spell cast - not implemented yet!");
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
        info = (DirectedActionInfo *) malloc(sizeof(DirectedActionInfo));
        info->handleAtCoord = &jimmyAtCoord;
        info->range = 1;
        info->blockedPredicate = NULL;
        info->failedMessage = "Jimmy what?";
        eventHandlerPushKeyHandlerData(&keyHandlerGetDirection, info);
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
        info = (DirectedActionInfo *) malloc(sizeof(DirectedActionInfo));
        info->handleAtCoord = &openAtCoord;
        info->range = 1;
        info->blockedPredicate = NULL;
        info->failedMessage = "Open what?";
        eventHandlerPushKeyHandlerData(&keyHandlerGetDirection, info);
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
        info = (DirectedActionInfo *) malloc(sizeof(DirectedActionInfo));
        info->handleAtCoord = &talkAtCoord;
        info->range = 2;
        info->blockedPredicate = &canTalkOver;
        info->failedMessage = "Funny, no\nresponse!";
        eventHandlerPushKeyHandlerData(&keyHandlerGetDirection, info);
        screenMessage("Talk\nDir: ");
        break;

    case 'y':
        screenMessage("Yell what?\n");
        break;

    case 'z':
        eventHandlerPushKeyHandler(&keyHandlerZtats);
        screenMessage("Ztats for: ");
        break;

    default:
        valid = 0;
        break;
    }

    return valid || keyHandlerDefault(key, NULL);
}

/**
 * Handles key presses when a command that requires a direction
 * argument is invoked.  Once an arrow key is pressed, control is
 * handed off to a command specific routine.
 */
int keyHandlerGetDirection(int key, void *data) {
    DirectedActionInfo *info = (DirectedActionInfo *) data;
    int valid = 1;
    int i;
    int t_x = c->saveGame->x, t_y = c->saveGame->y;

    eventHandlerPopKeyHandler();

    switch (key) {
    case U4_UP:
        screenMessage("North\n");
        break;
    case U4_DOWN:
        screenMessage("South\n");
        break;
    case U4_LEFT:
        screenMessage("West\n");
        break;
    case U4_RIGHT:
        screenMessage("East\n");
        break;
    }

    /* 
     * try every tile in the given direction, up to the given range.
     * Stop when the command handler succeeds, the range is exceeded,
     * or the action is blocked.
     */
    for (i = 1; i <= info->range; i++) {
        switch (key) {
        case U4_UP:
            t_y--;
            break;
        case U4_DOWN:
            t_y++;
            break;
        case U4_LEFT:
            t_x--;
            break;
        case U4_RIGHT:
            t_x++;
            break;
        default:
            t_x = -1;
            t_y = -1;
            valid = 0;
            break;
        }

        if ((*(info->handleAtCoord))(t_x, t_y))
            goto success;
        if (info->blockedPredicate &&
            !(*(info->blockedPredicate))(mapTileAt(c->map, t_x, t_y)))
            break;
    }

    screenMessage("%s\n", info->failedMessage);

 success:
    free(info);

    return valid || keyHandlerDefault(key, NULL);
}

/**
 * Handles key presses when a conversation is active.  The keystrokes
 * are buffered up into a word rather than invoking the usual
 * commands.
 */
int keyHandlerTalking(int key, void *data) {
    int valid = 1;

    if ((key >= 'a' && key <= 'z') || key == ' ') {
        int len;

        len = strlen(c->conversation.buffer);
        if (len < CONV_BUFFERLEN - 1) {
            screenMessage("%c", key);
            c->conversation.buffer[len] = key;
            c->conversation.buffer[len + 1] = '\0';
        }

    } else if (key == U4_BACKSPACE) {
        int len;

        len = strlen(c->conversation.buffer);
        c->conversation.buffer[len - 1] = '\0';
        c->col = 0;
        screenMessage("%s ", c->conversation.buffer);

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
            screenMessage("\nYour Interest:\n");

    } else {
        valid = 0;
    }

    return valid || keyHandlerDefault(key, NULL);
}

/**
 * Handles key presses when the Exit (Y/N) prompt is active.
 */
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
        saveGameFile = fopen("party.sav", "wb");
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

/**
 * Handles key presses when the Ztats prompt is active.
 */
int keyHandlerZtats(int key, void *data) {
    int valid = 1;

    screenMessage("%c\n", key);

    if (key == '0')
        c->statsItem = STATS_WEAPONS;
    else if (key >= '1' && key <= '8' && (key - '1' + 1) <= c->saveGame->members)
        c->statsItem = STATS_CHAR1 + (key - '1');
    else
        valid = 0;

    if (valid) {
        statsUpdate();
        eventHandlerPopKeyHandler();
        eventHandlerPushKeyHandler(&keyHandlerZtats2);
    }

    return valid || keyHandlerDefault(key, NULL);
}

/**
 * Handles key presses while Ztats are being displayed.
 */
int keyHandlerZtats2(int key, void *data) {
    switch (key) {
    case U4_UP:
    case U4_LEFT:
        c->statsItem--;
        if (c->statsItem < STATS_CHAR1)
            c->statsItem = STATS_MIXTURES;
        if (c->statsItem <= STATS_CHAR8 &&
            (c->statsItem - STATS_CHAR1 + 1) > c->saveGame->members)
            c->statsItem = STATS_CHAR1 - 1 + c->saveGame->members;
        break;
    case U4_DOWN:
    case U4_RIGHT:
        c->statsItem++;
        if (c->statsItem > STATS_MIXTURES)
            c->statsItem = STATS_CHAR1;
        if (c->statsItem <= STATS_CHAR8 &&
            (c->statsItem - STATS_CHAR1 + 1) > c->saveGame->members)
            c->statsItem = STATS_WEAPONS;
        break;
    default:
        c->statsItem = STATS_PARTY_OVERVIEW;
        eventHandlerPopKeyHandler();
        screenMessage("\020");
        break;
    }

    statsUpdate();

    return 1;
}

/**
 * Attempts to attack a creature at map coordinates x,y.  If no
 * creature is present at that point, zero is returned.
 */
int attackAtCoord(int x, int y) {
    if (x == -1 && y == -1)
        return 0;

    screenMessage("attack at %d, %d\n(not implemented)\n", x, y);

    return 1;
}

/**
 * Attempts to jimmy a locked door at map coordinates x,y.  If no
 * locked door is present at that point, zero is returned.  The locked
 * door is replaced by a permanent annotation of an unlocked door
 * tile.
 */
int jimmyAtCoord(int x, int y) {
    if ((x == -1 && y == -1) ||
        !islockeddoor(mapTileAt(c->map, x, y)))
        return 0;


    screenMessage("door at %d, %d unlocked!\n", x, y);
    annotationAdd(x, y, -1, 0x3b);

    return 1;
}

/**
 * Attempts to open a door at map coordinates x,y.  If no door is
 * present at that point, zero is returned.  The door is replaced by a
 * temporary annotation of a floor tile for 4 turns.
 */
int openAtCoord(int x, int y) {
    if ((x == -1 && y == -1) ||
        !isdoor(mapTileAt(c->map, x, y)))
        return 0;

    screenMessage("door at %d, %d, opened!\n", x, y);
    annotationAdd(x, y, 4, 0x3e);

    return 1;
}

/**
 * Begins a conversation with the NPC at map coordinates x,y.  If no
 * NPC is present at that point, zero is returned.
 */
int talkAtCoord(int x, int y) {
    char buffer[100];
    const Person *talker;

    if (x == -1 && y == -1)
        return 0;

    c->conversation.talker = mapPersonAt(c->map, x, y);
    if (c->conversation.talker == NULL)
        return 0;

    talker = c->conversation.talker;
    c->conversation.question = 0;
    sprintf(buffer, "\nYou meet\n%s\n\n", talker->description);
    if (isupper(buffer[9]))
        buffer[9] = tolower(buffer[9]);
    screenMessage(buffer);
    screenMessage("%s says: I am %s\n\n:", talker->pronoun, talker->name);
    c->conversation.buffer[0] = '\0';
    eventHandlerPushKeyHandler(&keyHandlerTalking);

    return 1;
}

/**
 * Attempt to move the avatar.  The number of tiles to move is given
 * by dx (horizontally) and dy (vertically); negative indicates
 * right/down, positive indicates left/up.
 */
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
