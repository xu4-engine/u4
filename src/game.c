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
#include "music.h"

void gameFinishTurn(void);
int gameCanMoveOntoTile(const Map *map, int x, int y);
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

void gameSetMap(Context *ct, Map *map) {
    int i;

    ct->map = map;
    ct->map->annotation = NULL;
    ct->saveGame->dngx = ct->parent->saveGame->x;
    ct->saveGame->dngy = ct->parent->saveGame->y;
    ct->saveGame->x = map->startx;
    ct->saveGame->y = map->starty;

    if (map->city) {
        for (i = 0; i < map->city->n_persons; i++) {
            if (map->city->persons[i].tile0 != 0)
                mapAddPersonObject(map, &(map->city->persons[i]));
        }
    }
}

/**
 * Terminates a game turn.  This performs the post-turn housekeeping
 * tasks like adjusting the party's food, incrementing the number of
 * moves, etc.
 */
void gameFinishTurn() {

    /* adjust food and moves */
    c->saveGame->moves++;
    c->saveGame->food -= c->saveGame->members;
    if (c->saveGame->food < 0) {
        /* FIXME: handle starving */
        c->saveGame->food = 0;
    }

    /* apply effects from tile avatar is standing on */
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

    /* update map annotations and the party stats */
    annotationCycle();
    c->statsItem = STATS_PARTY_OVERVIEW;
    statsUpdate();

    /* draw a prompt */
    screenMessage("\020");
}

/**
 * The main key handler for the game.  Interpretes each key as a
 * command - 'a' for attack, 't' for talk, etc.
 */
int gameBaseKeyHandler(int key, void *data) {
    int valid = 1;
    Object *obj;
    const Portal *portal;
    DirectedActionInfo *info;
    GetChoiceActionInfo *choiceInfo;

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

    case 'b':
        obj = mapObjectAt(c->map, c->saveGame->x, c->saveGame->y);
        if (obj && tileIsShip(obj->tile)) {
            if (c->saveGame->transport != AVATAR_TILE)
                screenMessage("Board: Can't!\n");
            else {
                c->saveGame->transport = obj->tile;
                c->saveGame->shiphull = 50;
                mapRemoveObject(c->map, obj);
                screenMessage("Board Frigate!\n");
            }
        } else if (obj && tileIsHorse(obj->tile)) {
            if (c->saveGame->transport != AVATAR_TILE)
                screenMessage("Board: Can't!\n");
            else {
                c->saveGame->transport = obj->tile;
                mapRemoveObject(c->map, obj);
                screenMessage("Mount Horse!\n");
            }
        } else
            screenMessage("Board What?\n");
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
            new->col = new->parent->col;
            new->line = new->parent->line;
            new->statsItem = new->parent->statsItem;
            new->moonPhase = new->parent->moonPhase;
            gameSetMap(new, portal->destination);
            c = new;

            if (c->map->city) {
                const char *type = NULL;
                switch (c->map->city->type) {
                case CITY_TOWN:
                    type = "towne";
                    break;
                case CITY_VILLAGE:
                    type = "village";
                    break;
                case CITY_CASTLE:
                    type = "castle";
                    break;
                case CITY_RUIN:
                    type = "ruin";
                    break;
                }
                screenMessage("Enter %s!\n\n%s\n\n", type, c->map->city->name);
            }

            play_music();
            
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
            if (c->saveGame->transport != AVATAR_TILE)
                screenMessage("Klimb\nOnly on foot!\n");
            else {
                c->map = portal->destination;
                screenMessage("Klimb!\n\n");
            }
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
        info->failedMessage = "Not Here!";
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

    case 'v':
        if (musicToggle())
            screenMessage("Volume On!\n");
        else
            screenMessage("Volume Off!\n");
        break;

    case 'w':
        eventHandlerPushKeyHandlerData(&gameGetPlayerNoKeyHandler, &wearForPlayer);
        screenMessage("Wear Armor\nfor: ");
        break;

    case 'x':
        if (tileIsShip(c->saveGame->transport) || tileIsHorse(c->saveGame->transport)) {
            mapAddObject(c->map, c->saveGame->transport, c->saveGame->transport, c->saveGame->x, c->saveGame->y);
            c->saveGame->transport = AVATAR_TILE;
            screenMessage("X-it\n");
        } else
            screenMessage("X-it What?\n");
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
        if (eventHandlerGetKeyHandler() == &gameBaseKeyHandler)
            gameFinishTurn();
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
        gameFinishTurn();
    } else if (key == ' ' || key == U4_ESC) {
        screenMessage("\n");
        eventHandlerPopKeyHandler();
        free(info);
        gameFinishTurn();
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
    gameFinishTurn();

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
        eventHandlerPopKeyHandler();
        gameFinishTurn();
        break;
    }

    statsUpdate();

    return 1;
}

int gameSpecialCmdKeyHandler(int key, void *data) {
    int i;
    int valid = 1;

    switch (key) {
    case 'b':
        if (mapIsWorldMap(c->map)) {
            mapAddObject(c->map, 16, 16, 88, 109);
            screenMessage("Boat created!\n\020");
        }
        break;
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
        screenMessage("Help:\nb - Boat\nc - Collision\ne - Equipment\nh - Help\ni - Items\nk - Show Karma\nm - Mixtures\no - Horse\nr - Reagents\n\020");
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
    case 'o':
        if (mapIsWorldMap(c->map)) {
            mapAddObject(c->map, 20, 20, 84, 106);
            screenMessage("Horse created!\n\020");
        }
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

    if (valid)
        eventHandlerPopKeyHandler();

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
    gameFinishTurn();

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
    gameFinishTurn();

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


    annotationAdd(x, y, -1, 0x3b);
    screenMessage("\nUnlocked!\n");
    gameFinishTurn();

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
        screenMessage("None left!\n");
        gameFinishTurn();
        return 0;
    }

    if (!playerCanReady(&(c->saveGame->players[player]), weapon)) {
        screenMessage("\nA %s may NOT\nuse\n%s\n", getClassName(c->saveGame->players[player].klass), getWeaponName(weapon));
        gameFinishTurn();
        return 0;
    }

    oldWeapon = c->saveGame->players[player].weapon;
    if (oldWeapon != WEAP_HANDS)
        c->saveGame->weapons[oldWeapon]++;
    if (weapon != WEAP_HANDS)
        c->saveGame->weapons[weapon]--;
    c->saveGame->players[player].weapon = weapon;

    screenMessage("%s\n", getWeaponName(weapon));

    gameFinishTurn();

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
        screenMessage("%s, You must\nlead!\n", c->saveGame->players[0].name);
        gameFinishTurn();
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
        screenMessage("%s, You must\nlead!\n", c->saveGame->players[0].name);
        gameFinishTurn();
        return 0;
    } else if (player1 == player2) {
        gameFinishTurn();
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
    if (x == -1 && y == -1)
        return 0;
    if (tileIsLockedDoor(mapTileAt(c->map, x, y))) {
        screenMessage("Can't!\n");
        gameFinishTurn();
        return 1;
    }
    if (!tileIsDoor(mapTileAt(c->map, x, y)))
        return 0;

    annotationAdd(x, y, 4, 0x3e);
    screenMessage("\nOpened!\n");
    gameFinishTurn();

    return 1;
}

/**
 * Handles the Exit (Y/N) choice.
 */
int quitHandleChoice(char choice) {
    FILE *saveGameFile, *monstersFile;

    eventHandlerPopKeyHandler();

    saveGameFile = fopen("party.sav", "wb");
    if (saveGameFile) {
        saveGameWrite(c->saveGame, saveGameFile);
        fclose(saveGameFile);
    } else {
        screenMessage("Error writing to\nparty.sav\n");
        choice = 'n';
    }

    monstersFile = fopen("monsters.sav", "wb");
    if (monstersFile) {
        saveGameMonstersWrite(c->map->objects, monstersFile);
        fclose(monstersFile);
    } else {
        screenMessage("Error writing to\nmonsters.sav\n");
        choice = 'n';
    }

    switch (choice) {
    case 'y':
        eventHandlerSetExitFlag(1);
        break;
    case 'n':
        screenMessage("%c\n", choice);
        gameFinishTurn();
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
    case CONV_QUANTITY:
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

    case CONV_BUY:
    case CONV_SELL:
        gcInfo = (GetChoiceActionInfo *) malloc(sizeof(GetChoiceActionInfo));
        gcInfo->choices = "abcdefghijklmnopqrstuvwxyz";
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
        gameFinishTurn();
        return 1;
    }

    personGetPrompt(&c->conversation, &prompt);
    if (prompt) {
        screenMessage("%s", prompt);
        free(prompt);
    }

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
        gameFinishTurn();
        return 1;
    }

    personGetPrompt(&c->conversation, &prompt);
    if (prompt) {
        screenMessage("%s", prompt);
        free(prompt);
    }

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
        screenMessage("None left!\n");
        gameFinishTurn();
        return 0;
    }

    if (!playerCanWear(&(c->saveGame->players[player]), armor)) {
        screenMessage("\nA %s may NOT\nuse\n%s\n", getClassName(c->saveGame->players[player].klass), getArmorName(armor));
        gameFinishTurn();
        return 0;
    }

    oldArmor = c->saveGame->players[player].armor;
    if (oldArmor != ARMR_NONE)
        c->saveGame->armor[oldArmor]++;
    if (armor != ARMR_NONE)
        c->saveGame->armor[armor]--;
    c->saveGame->players[player].armor = armor;

    screenMessage("%s\n", getArmorName(armor));

    gameFinishTurn();

    return 1;
}

/**
 * Returns true if moving onto the given coordinate is possible,
 * taking into account the current party transport (on foot, ship,
 * horse, etc.). 
 */
int gameCanMoveOntoTile(const Map *map, int x, int y) {
    unsigned char tile;
    Object *obj;

    /* if an object is on the map tile in question, check it instead */
    if ((obj = mapObjectAt(map, x, y)) != NULL)
        tile = obj->tile;
    else
        tile = mapTileAt(map, x, y);

    /* if the party in is a ship, check sailable, otherwise walkable */
    if (tileIsShip(c->saveGame->transport)) {
        if (!tileIsSailable(tile))
            return 0;
    }
    else if (!tileIsWalkable(tile))
        return 0;

    return 1;
}

/**
 * Attempt to move the avatar.  The number of tiles to move is given
 * by dx (horizontally) and dy (vertically); negative indicates
 * right/down, positive indicates left/up.  Returns zero if the avatar
 * is blocked.
 */
int moveAvatar(int dx, int dy) {
    const char *fmt;
    int newx, newy;

    if (tileIsShip(c->saveGame->transport)) {
        if (dx < 0 && tileGetDirection(c->saveGame->transport) != DIR_WEST) {
            tileSetDirection(&c->saveGame->transport, DIR_WEST);
            screenMessage("Turn West!\n");
            return 1;
        }
        if (dx > 0 && tileGetDirection(c->saveGame->transport) != DIR_EAST) {
            tileSetDirection(&c->saveGame->transport, DIR_EAST);
            screenMessage("Turn East!\n");
            return 1;
        }
        if (dy < 0 && tileGetDirection(c->saveGame->transport) != DIR_NORTH) {
            tileSetDirection(&c->saveGame->transport, DIR_NORTH);
            screenMessage("Turn North!\n");
            return 1;
        }
        if (dy > 0 && tileGetDirection(c->saveGame->transport) != DIR_SOUTH) {
            tileSetDirection(&c->saveGame->transport, DIR_SOUTH);
            screenMessage("Turn South!\n");
            return 1;
        }
    }

    if (tileIsHorse(c->saveGame->transport)) {
        if (dx < 0 && tileGetDirection(c->saveGame->transport) != DIR_WEST) {
            tileSetDirection(&c->saveGame->transport, DIR_WEST);
        }
        if (dx > 0 && tileGetDirection(c->saveGame->transport) != DIR_EAST) {
            tileSetDirection(&c->saveGame->transport, DIR_EAST);
        }
    }

    newx = c->saveGame->x + dx;
    newy = c->saveGame->y + dy;

    if (tileIsShip(c->saveGame->transport))
        fmt = "Sail %s!\n";
    else
        fmt = "%s\n";
    if (newx < c->saveGame->x)
        screenMessage(fmt, "West");
    else if (newx > c->saveGame->x)
        screenMessage(fmt, "East");
    else if (newy < c->saveGame->y)
        screenMessage(fmt, "North");
    else if (newy > c->saveGame->y)
        screenMessage(fmt, "South");

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
                annotationClear();
                mapClearObjects(c->map);
                c->parent->saveGame->x = c->saveGame->dngx;
                c->parent->saveGame->y = c->saveGame->dngy;
                c->parent->line = c->line;
                c->parent->moonPhase = c->moonPhase;
		c = c->parent;
		free(t);
                
                play_music();
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
        if (!gameCanMoveOntoTile(c->map, newx, newy)) {
            screenMessage("Blocked!\n");
            return 0;
        }

        /* 
         * special cases for tile 0x0e: the center tile of the castle
         * of lord british, which is walkable from the south only
         */
        if (mapTileAt(c->map, newx, newy) == 0x0e && dy == 1) {
            screenMessage("Blocked!\n");
            return 0;
        }
        if (mapTileAt(c->map, c->saveGame->x, c->saveGame->y) == 0x0e && dy == -1) {
            screenMessage("Blocked!\n");
            return 0;
        }
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


