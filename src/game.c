/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "u4.h"
#include "game.h"
#include "screen.h"
#include "event.h"
#include "map.h"
#include "city.h"
#include "portal.h"
#include "person.h"
#include "ttype.h"
#include "context.h"
#include "annotation.h"
#include "savegame.h"
#include "stats.h"
#include "spell.h"
#include "names.h"
#include "player.h"

int moveAvatar(int dx, int dy);
int attackAtCoord(int x, int y);
int castForPlayer(int player);
int castForPlayer2(int spell, void *data);
int jimmyAtCoord(int x, int y);
int newOrderForPlayer(int player);
int newOrderForPlayer2(int player2);
int openAtCoord(int x, int y);
int quitHandleChoice(char choice);
int readyForPlayer(int player);
int readyForPlayer2(int weapon, void *data);
int talkAtCoord(int x, int y);
void talkSetHandler(const Conversation *cnv);
int talkHandleBuffer(const char *message);
int talkHandleChoice(char choice);
int wearForPlayer(int player);
int wearForPlayer2(int armor, void *data);

int collisionOverride = 0;

/**
 * The main key handler for the game.  Interpretes each key as a
 * command - 'a' for attack, 't' for talk, etc.
 */
int gameBaseKeyHandler(int key, void *data) {
    int valid = 1;
    const Portal *portal;
    DirectedActionInfo *info;
    GetChoiceActionInfo *choiceInfo;

    switch (key) {

    case U4_UP:
        screenMessage("North\n");
        if (!moveAvatar(0, -1))
            screenMessage("Blocked!\n");
        break;

    case U4_DOWN:
        screenMessage("South\n");
        if (!moveAvatar(0, 1))
            screenMessage("Blocked!\n");
        break;

    case U4_LEFT:
        screenMessage("West\n");
        if (!moveAvatar(-1, 0))
            screenMessage("Blocked!\n");
        break;

    case U4_RIGHT:
        screenMessage("East\n");
        if (!moveAvatar(1, 0))
            screenMessage("Blocked!\n");
        break;

    case 3:                     /* ctrl-C */
        screenMessage("Cmd: ");
        eventHandlerPushKeyHandler(&gameSpecialCmdKeyHandler);
        break;

    case 'a':
        info = (DirectedActionInfo *) malloc(sizeof(DirectedActionInfo));
        info->handleAtCoord = &attackAtCoord;
        info->range = 1;
        info->blockedPredicate = NULL;
        info->failedMessage = "FIXME";
        eventHandlerPushKeyHandlerData(&gameGetDirectionKeyHandler, info);
        screenMessage("Attack\nDir: ");
        break;

    case 'c':
        eventHandlerPushKeyHandlerData(&gameGetPlayerNoKeyHandler, &castForPlayer);
        screenMessage("Cast Spell!\nPlayer: ");
        break;

    case 'd':
        portal = mapPortalAt(c->map, c->saveGame->x, c->saveGame->y);
        if (portal && portal->trigger_action == ACTION_DESCEND) {
            c->map = portal->destination;
            screenMessage("Descend!\n\n");
        }
        else
            screenMessage("Descend what?\n");
        break;

    case 'e':
        portal = mapPortalAt(c->map, c->saveGame->x, c->saveGame->y);
        if (portal && portal->trigger_action == ACTION_ENTER) {

            Context *new = (Context *) malloc(sizeof(Context));
            new->parent = c;
            new->saveGame = c->saveGame;
            new->map = portal->destination;
            new->annotation = NULL;
            new->saveGame->dngx = new->parent->saveGame->x;
            new->saveGame->dngy = new->parent->saveGame->y;
            new->saveGame->x = new->map->startx;
            new->saveGame->y = new->map->starty;
            new->col = new->parent->col;
            new->line = new->parent->line;
            new->statsItem = new->parent->statsItem;
            new->moonPhase = new->parent->moonPhase;
            c = new;

            if (c->map->city)
                screenMessage("Enter towne!\n\n%s\n\n", c->map->city->name);
        } else
            screenMessage("Enter what?\n");
        break;

    case 'j':
        info = (DirectedActionInfo *) malloc(sizeof(DirectedActionInfo));
        info->handleAtCoord = &jimmyAtCoord;
        info->range = 1;
        info->blockedPredicate = NULL;
        info->failedMessage = "Jimmy what?";
        eventHandlerPushKeyHandlerData(&gameGetDirectionKeyHandler, info);
        screenMessage("Jimmy\nDir: ");
        break;
        
    case 'k':
        portal = mapPortalAt(c->map, c->saveGame->x, c->saveGame->y);
        if (portal && portal->trigger_action == ACTION_KLIMB) {
            c->map = portal->destination;
            screenMessage("Klimb!\n\n");
        }
        else
            screenMessage("Klimb what?\n");
        break;

    case 'l':
        if (c->saveGame->sextants >= 1)
            screenMessage("Locate position\nwith sextant\n Latitude: %c'%c\"\nLongitude: %c'%c\"\n",
                          c->saveGame->y / 16 + 'A', c->saveGame->y % 16 + 'A',
                          c->saveGame->x / 16 + 'A', c->saveGame->x % 16 + 'A');
        else
            screenMessage("Locate position\nwith what?\n");
        break;

    case 'n':
        eventHandlerPushKeyHandlerData(&gameGetPlayerNoKeyHandler, &newOrderForPlayer);
        screenMessage("New Order!\nExchange # ");
        break;

    case 'o':
        info = (DirectedActionInfo *) malloc(sizeof(DirectedActionInfo));
        info->handleAtCoord = &openAtCoord;
        info->range = 1;
        info->blockedPredicate = NULL;
        info->failedMessage = "Open what?";
        eventHandlerPushKeyHandlerData(&gameGetDirectionKeyHandler, info);
        screenMessage("Open\nDir: ");
        break;

    case 'q':
        if (!mapIsWorldMap(c->map)) {
            screenMessage("Quit & save\nNot Here!\n");
        } else {
            choiceInfo = (GetChoiceActionInfo *) malloc(sizeof(GetChoiceActionInfo));
            choiceInfo->choices = "yn";
            choiceInfo->handleChoice = &quitHandleChoice;
            eventHandlerPushKeyHandlerData(&keyHandlerGetChoice, choiceInfo);
            screenMessage("Quit & Save...\n%d moves\nExit (Y/N)? ", c->saveGame->moves);
        }
        break;

    case 'r':
        eventHandlerPushKeyHandlerData(&gameGetPlayerNoKeyHandler, &readyForPlayer);
        screenMessage("Ready a weapon\nfor: ");
        break;

    case 't':
        info = (DirectedActionInfo *) malloc(sizeof(DirectedActionInfo));
        info->handleAtCoord = &talkAtCoord;
        info->range = 2;
        info->blockedPredicate = &tileCanTalkOver;
        info->failedMessage = "Funny, no\nresponse!";
        eventHandlerPushKeyHandlerData(&gameGetDirectionKeyHandler, info);
        screenMessage("Talk\nDir: ");
        break;

    case 'w':
        eventHandlerPushKeyHandlerData(&gameGetPlayerNoKeyHandler, &wearForPlayer);
        screenMessage("Wear Armor\nfor: ");
        break;

    case 'y':
        screenMessage("Yell what?\n");
        break;

    case 'z':
        eventHandlerPushKeyHandler(&gameZtatsKeyHandler);
        screenMessage("Ztats for: ");
        break;

    default:
        valid = 0;
        break;
    }

    if (valid) {
        if (eventHandlerGetKeyHandler() == &gameBaseKeyHandler) {
            c->saveGame->moves++;
            c->saveGame->food -= c->saveGame->members;
            if (c->saveGame->food < 0) {
                /* FIXME: handle starving */
                c->saveGame->food = 0;
            }
            switch (tileGetEffect(mapTileAt(c->map, c->saveGame->x, c->saveGame->y))) {
            case EFFECT_NONE:
                break;
            case EFFECT_FIRE:
                screenMessage("Burning!\n");
                break;
            case EFFECT_SLEEP:
                screenMessage("Zzzz!\n");
                break;
            case EFFECT_POISON:
                screenMessage("Poison!\n");
                break;
            default:
                assert(0);
            }

            screenMessage("\020");
            annotationCycle();
        }
        statsUpdate();
    }

    return valid || keyHandlerDefault(key, NULL);
}

/**
 * Handles key presses for a command requiring a player number
 * argument.  Once a number key is pressed, control is handed off to a
 * command specific routine.
 */
int gameGetPlayerNoKeyHandler(int key, void *data) {
    int (*handlePlayerNo)(int player) = (int(*)(int))data;
    int valid = 1;

    eventHandlerPopKeyHandler();

    if (key >= '1' &&
        key <= ('0' + c->saveGame->members)) {
        screenMessage("%c\n", key);
        handlePlayerNo(key - '1');
    } else {
        screenMessage("None\n");
        valid = 0;
    }

    return valid || keyHandlerDefault(key, NULL);
}

/**
 * Handles key presses for a command requiring a letter argument.
 * Once a valid key is pressed, control is handed off to a command
 * specific routine.
 */
int gameGetAlphaChoiceKeyHandler(int key, void *data) {
    AlphaActionInfo *info = (AlphaActionInfo *) data;
    int valid = 1;


    if (isupper(key))
        key = tolower(key);

    if (key >= 'a' && key <= info->lastValidLetter) {
        screenMessage("%c\n", key);
        (*(info->handleAlpha))(key - 'a', info->data);
        eventHandlerPopKeyHandler();
        free(info);
        c->statsItem = STATS_PARTY_OVERVIEW;
        statsUpdate();
    } else if (key == ' ' || key == U4_ESC) {
        eventHandlerPopKeyHandler();
        free(info);
        screenMessage("\n\020");
        c->statsItem = STATS_PARTY_OVERVIEW;
        statsUpdate();
    } else {
        valid = 0;
        screenMessage("\n%s", info->prompt);
        screenForceRedraw();
    }

    return valid || keyHandlerDefault(key, NULL);
}

/**
 * Handles key presses for a command requiring a direction argument.
 * Once an arrow key is pressed, control is handed off to a command
 * specific routine.
 */
int gameGetDirectionKeyHandler(int key, void *data) {
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
    c->statsItem = STATS_PARTY_OVERVIEW;
    statsUpdate();

 success:
    free(info);

    return valid || keyHandlerDefault(key, NULL);
}

/**
 * Handles key presses when the Ztats prompt is active.
 */
int gameZtatsKeyHandler(int key, void *data) {
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
        eventHandlerPushKeyHandler(&gameZtatsKeyHandler2);
    }

    return valid || keyHandlerDefault(key, NULL);
}

/**
 * Handles key presses while Ztats are being displayed.
 */
int gameZtatsKeyHandler2(int key, void *data) {
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

int gameSpecialCmdKeyHandler(int key, void *data) {
    int i;
    int valid = 1;

    eventHandlerPopKeyHandler();

    switch (key) {
    case 'c':
        collisionOverride = !collisionOverride;
        screenMessage("Collision detection %s!\n\020", collisionOverride ? "off" : "on");
        break;
    case 'e':
        screenMessage("Equipment!\n\020");
        for (i = ARMR_NONE + 1; i < ARMR_MAX; i++)
            c->saveGame->armor[i] = 8;
        for (i = WEAP_HANDS + 1; i < WEAP_MAX; i++)
            c->saveGame->weapons[i] = 8;
        break;
    case 'h':
        screenMessage("Help:\nc - Collision\ne - Equipment\nh - Help\ni - Items\nk - Show Karma\nm - Mixtures\nr - Reagents\n\020");
        break;
    case 'i':
        screenMessage("Items!\n\020");
        c->saveGame->torches = 99;
        c->saveGame->gems = 99;
        c->saveGame->keys = 99;
        c->saveGame->items = ITEM_SKULL | ITEM_CANDLE | ITEM_BOOK | ITEM_BELL | ITEM_KEY_C | ITEM_KEY_L | ITEM_KEY_T | ITEM_HORN | ITEM_WHEEL;
        c->saveGame->stones = 0xff;
        c->saveGame->runes = 0xff;
        c->saveGame->food = 99900;
        c->saveGame->gold = 999;
        statsUpdate();
        break;
    case 'k':
        screenMessage("Karma:\nH C V J S H S H\n%02x%02x%02x%02x%02x%02x%02x%02x\n\020", c->saveGame->karma[0], c->saveGame->karma[1], c->saveGame->karma[2],
                      c->saveGame->karma[3], c->saveGame->karma[4], c->saveGame->karma[5], c->saveGame->karma[6], c->saveGame->karma[7]);
        break;
    case 'm':
        screenMessage("Mixtures!\n\020");
        for (i = 0; i < 26; i++)
            c->saveGame->mixtures[i] = 99;
        break;
    case 'r':
        screenMessage("Reagents!\n\020");
        for (i = 0; i < REAG_MAX; i++)
            c->saveGame->reagents[i] = 99;
        break;
    default:
        valid = 0;
        break;
    }

    return valid || keyHandlerDefault(key, NULL);
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

int castForPlayer(int player) {
    AlphaActionInfo *info;

    c->statsItem = STATS_MIXTURES;
    statsUpdate();

    info = (AlphaActionInfo *) malloc(sizeof(AlphaActionInfo));
    info->lastValidLetter = 'z';
    info->handleAlpha = castForPlayer2;
    info->prompt = "Spell: ";
    info->data = (void *) player;

    screenMessage("%s", info->prompt);

    eventHandlerPushKeyHandlerData(&gameGetAlphaChoiceKeyHandler, info);

    return 1;
}

int castForPlayer2(int spell, void *data) {
    int player = (int) data;
    SpellCastError spellError;

    if (!spellCast(spell, player, &spellError)) {
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
    }
    screenMessage("\n\020");

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
        !tileIsLockedDoor(mapTileAt(c->map, x, y)))
        return 0;


    screenMessage("door at %d, %d unlocked!\n", x, y);
    annotationAdd(x, y, -1, 0x3b);

    return 1;
}

/**
 * Readies a weapon for the given player.  Prompts the use for a
 * weapon.
 */
int readyForPlayer(int player) {
    AlphaActionInfo *info;

    c->statsItem = STATS_WEAPONS;
    statsUpdate();

    info = (AlphaActionInfo *) malloc(sizeof(AlphaActionInfo));
    info->lastValidLetter = WEAP_MAX + 'a' - 1;
    info->handleAlpha = readyForPlayer2;
    info->prompt = "Weapon: ";
    info->data = (void *) player;

    screenMessage("%s", info->prompt);

    eventHandlerPushKeyHandlerData(&gameGetAlphaChoiceKeyHandler, info);

    return 1;
}

int readyForPlayer2(int weapon, void *data) {
    int player = (int) data;
    int oldWeapon;

    if (weapon != WEAP_HANDS && c->saveGame->weapons[weapon] < 1) {
        screenMessage("None left!\n\020");
        return 0;
    }

    if (!playerCanReady(&(c->saveGame->players[player]), weapon)) {
        screenMessage("\nA %s may NOT\nuse\n%s\n\020", getClassName(c->saveGame->players[player].klass), getWeaponName(weapon));
        return 0;
    }

    oldWeapon = c->saveGame->players[player].weapon;
    if (oldWeapon != WEAP_HANDS)
        c->saveGame->weapons[oldWeapon]++;
    if (weapon != WEAP_HANDS)
        c->saveGame->weapons[weapon]--;
    c->saveGame->players[player].weapon = weapon;

    screenMessage("%s\n\020", getWeaponName(weapon));

    return 1;
}

/* FIXME: must be a better way.. */
int newOrderTemp;

/**
 * Exchanges the position of two players in the party.  Prompts the
 * use for the second player number.
 */
int newOrderForPlayer(int player) {
    if (player == 0) {
        screenMessage("%s, You must\nlead!\n\020", c->saveGame->players[0].name);
        return 0;
    }

    screenMessage("    with # ");

    newOrderTemp = player;
    eventHandlerPushKeyHandlerData(&gameGetPlayerNoKeyHandler, &newOrderForPlayer2);

    return 1;
}

int newOrderForPlayer2(int player2) {
    int player1 = newOrderTemp;
    SaveGamePlayerRecord tmp;

    if (player2 == 0) {
        screenMessage("%s, You must\nlead!\n\020", c->saveGame->players[0].name);
        return 0;
    } else if (player1 == player2) {
        screenMessage("\020");
        return 0;
    }

    tmp = c->saveGame->players[player1];
    c->saveGame->players[player1] = c->saveGame->players[player2];
    c->saveGame->players[player2] = tmp;

    statsUpdate();

    return 1;
}

/**
 * Attempts to open a door at map coordinates x,y.  If no door is
 * present at that point, zero is returned.  The door is replaced by a
 * temporary annotation of a floor tile for 4 turns.
 */
int openAtCoord(int x, int y) {
    if ((x == -1 && y == -1) ||
        !tileIsDoor(mapTileAt(c->map, x, y)))
        return 0;

    screenMessage("door at %d, %d, opened!\n", x, y);
    annotationAdd(x, y, 4, 0x3e);

    return 1;
}

/**
 * Handles the Exit (Y/N) choice.
 */
int quitHandleChoice(char choice) {
    FILE *saveGameFile;

    eventHandlerPopKeyHandler();

    saveGameFile = fopen("party.sav", "wb");
    if (saveGameFile) {
        saveGameWrite(c->saveGame, saveGameFile);
        fclose(saveGameFile);
    } else {
        screenMessage("Error writing to\nparty.sav\n");
        choice = 'n';
    }

    switch (choice) {
    case 'y':
        eventHandlerSetExitFlag(1);
        break;
    case 'n':
        screenMessage("%c\n\020", choice);
        break;
    default:
        assert(0);              /* shouldn't happen */
    }

    return 1;
}

/**
 * Begins a conversation with the NPC at map coordinates x,y.  If no
 * NPC is present at that point, zero is returned.
 */
int talkAtCoord(int x, int y) {
    const Person *talker;
    char *text;

    if (x == -1 && y == -1)
        return 0;

    c->conversation.talker = mapPersonAt(c->map, x, y);
    if (c->conversation.talker == NULL)
        return 0;

    talker = c->conversation.talker;
    c->conversation.state = CONV_INTRO;
    c->conversation.buffer[0] = '\0';
    
    personGetConversationText(&c->conversation, "", &text);
    screenMessage("\n\n%s", text);
    free(text);
    talkSetHandler(&c->conversation);

    return 1;
}

/**
 * Set up a key handler to handle the current conversation state.
 */
void talkSetHandler(const Conversation *cnv) {
    ReadBufferActionInfo *rbInfo;
    GetChoiceActionInfo *gcInfo;

    switch (cnv->state) {
    case CONV_TALK:
    case CONV_ASK:
        rbInfo = (ReadBufferActionInfo *) malloc(sizeof(ReadBufferActionInfo));
        rbInfo->buffer = c->conversation.buffer;
        rbInfo->bufferLen = CONV_BUFFERLEN;
        rbInfo->handleBuffer = &talkHandleBuffer;
        rbInfo->screenX = TEXT_AREA_X + c->col;
        rbInfo->screenY = TEXT_AREA_Y + c->line;
        eventHandlerPushKeyHandlerData(&keyHandlerReadBuffer, rbInfo);
        break;
    
    case CONV_BUYSELL:
        gcInfo = (GetChoiceActionInfo *) malloc(sizeof(GetChoiceActionInfo));
        gcInfo->choices = "bs";
        gcInfo->handleChoice = &talkHandleChoice;
        eventHandlerPushKeyHandlerData(&keyHandlerGetChoice, gcInfo);
        break;

    case CONV_DONE:
        /* no handler: conversation done! */
        break;

    default:
        assert(0);              /* shouldn't happen */
    }
}

/**
 * Handles a query while talking to an NPC.
 */
int talkHandleBuffer(const char *message) {
    char *reply, *prompt;

    eventHandlerPopKeyHandler();

    personGetConversationText(&c->conversation, message, &reply);
    screenMessage("\n\n%s\n", reply);
    free(reply);
        
    c->conversation.buffer[0] = '\0';

    if (c->conversation.state == CONV_DONE) {
        screenMessage("\020");
        return 1;
    }

    personGetPrompt(&c->conversation, &prompt);
    screenMessage("%s", prompt);
    free(prompt);

    talkSetHandler(&c->conversation);

    return 1;
}

int talkHandleChoice(char choice) {
    char message[2];
    char *reply, *prompt;

    eventHandlerPopKeyHandler();

    message[0] = choice;
    message[1] = '\0';

    personGetConversationText(&c->conversation, message, &reply);
    screenMessage("\n\n%s\n", reply);
    free(reply);
        
    c->conversation.buffer[0] = '\0';

    if (c->conversation.state == CONV_DONE) {
        screenMessage("\020");
        return 1;
    }

    personGetPrompt(&c->conversation, &prompt);
    screenMessage("%s", prompt);
    free(prompt);

    talkSetHandler(&c->conversation);

    return 1;
}

/**
 * Changes armor for the given player.  Prompts the use for the armor.
 */
int wearForPlayer(int player) {
    AlphaActionInfo *info;

    c->statsItem = STATS_ARMOR;
    statsUpdate();

    info = (AlphaActionInfo *) malloc(sizeof(AlphaActionInfo));
    info->lastValidLetter = ARMR_MAX + 'a' - 1;
    info->handleAlpha = wearForPlayer2;
    info->prompt = "Armour: ";
    info->data = (void *) player;

    screenMessage("%s", info->prompt);

    eventHandlerPushKeyHandlerData(&gameGetAlphaChoiceKeyHandler, info);

    return 1;
}

int wearForPlayer2(int armor, void *data) {
    int player = (int) data;
    int oldArmor;

    if (armor != ARMR_NONE && c->saveGame->armor[armor] < 1) {
        screenMessage("None left!\n\020");
        return 0;
    }

    if (!playerCanWear(&(c->saveGame->players[player]), armor)) {
        screenMessage("\nA %s may NOT\nuse\n%s\n\020", getClassName(c->saveGame->players[player].klass), getArmorName(armor));
        return 0;
    }

    oldArmor = c->saveGame->players[player].armor;
    if (oldArmor != ARMR_NONE)
        c->saveGame->armor[oldArmor]++;
    if (armor != ARMR_NONE)
        c->saveGame->armor[armor]--;
    c->saveGame->players[player].armor = armor;

    screenMessage("%s\n\020", getArmorName(armor));

    return 1;
}

/**
 * Attempt to move the avatar.  The number of tiles to move is given
 * by dx (horizontally) and dy (vertically); negative indicates
 * right/down, positive indicates left/up.  Returns zero if the avatar
 * is blocked.
 */
int moveAvatar(int dx, int dy) {
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
                c->parent->saveGame->x = c->saveGame->dngx;
                c->parent->saveGame->y = c->saveGame->dngy;
                c->parent->line = c->line;
                c->parent->moonPhase = c->moonPhase;
		c = c->parent;
		free(t);
	    }
	    return 1;
	    
	case BORDER_FIXED:
	    if (newx < 0 || newx >= c->map->width)
		newx = c->saveGame->x;
	    if (newy < 0 || newy >= c->map->height)
		newy = c->saveGame->y;
	    break;
	}
    }

    if (!collisionOverride) {
        if (!tileIsWalkable(mapTileAt(c->map, newx, newy)))
            return 0;
        if (mapPersonAt(c->map, newx, newy))
            return 0;
        /* 
         * special cases for tile 0x0e: the center tile of the castle
         * of lord british, which is walkable from the south only
         */
        if (mapTileAt(c->map, newx, newy) == 0x0e && dy == 1)
            return 0;
        if (mapTileAt(c->map, c->saveGame->x, c->saveGame->y) == 0x0e && dy == -1)
            return 0;
    }

    c->saveGame->x = newx;
    c->saveGame->y = newy;
    return 1;
}

/**
 * This function is called every quarter second.
 */
void gameTimer() {
    screenCycle();
    screenUpdate();
    screenForceRedraw();

    if (++c->moonPhase >= (MOON_SECONDS_PER_PHASE * 4 * MOON_PHASES))
        c->moonPhase = 0;
}


