/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "u4.h"
#include "direction.h"
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
#include "moongate.h"
#include "music.h"
#include "item.h"
#include "shrine.h"
#include "death.h"
#include "combat.h"
#include "camp.h"

void gameCastSpell(unsigned int spell, int caster, int param);
int gameCheckPlayerDisabled(int player);
int gameCanMoveOntoTile(const Map *map, int x, int y);
int moveAvatar(Direction dir, int userEvent);
int attackAtCoord(int x, int y);
int castForPlayer(int player);
int castForPlayer2(int spell, void *data);
int castForPlayerGetDestPlayer(int player);
int castForPlayerGetDestDir(Direction dir);
int jimmyAtCoord(int x, int y);
int mixReagentsForSpell(int spell, void *data);
int mixReagentsForSpell2(int spell, void *data);
int newOrderForPlayer(int player);
int newOrderForPlayer2(int player2);
int openAtCoord(int x, int y);
int gemHandleChoice(char choice);
int quitHandleChoice(char choice);
int readyForPlayer(int player);
int readyForPlayer2(int weapon, void *data);
int talkAtCoord(int x, int y);
void talkSetHandler(const Conversation *cnv);
int talkHandleBuffer(const char *message);
int talkHandleChoice(char choice);
int wearForPlayer(int player);
int wearForPlayer2(int armor, void *data);
void gameCheckMoongates(void);

int collisionOverride = 0;
ViewMode viewMode = VIEW_NORMAL;

/**
 * Sets the view mode.
 */
void gameSetViewMode(ViewMode newMode) {
    viewMode = newMode;
}

void gameUpdateScreen() {
    switch (viewMode) {
    case VIEW_NORMAL:
        screenUpdate(1);
        break;
    case VIEW_GEM:
        screenGemUpdate();
        break;
    case VIEW_DEAD:
        screenUpdate(0);
        break;
    default:
        assert(0);              /* shouldn't happen */
    }
}

void gameSetMap(Context *ct, Map *map, int setStartPos) {
    int i;

    ct->map = map;
    ct->map->annotation = NULL;
    if (setStartPos) {
        ct->saveGame->dngx = ct->parent->saveGame->x;
        ct->saveGame->dngy = ct->parent->saveGame->y;
        ct->saveGame->x = map->startx;
        ct->saveGame->y = map->starty;
    }

    if ((map->type == MAP_TOWN || 
         map->type == MAP_VILLAGE ||
         map->type == MAP_CASTLE ||
         map->type == MAP_RUIN) && 
        map->objects == NULL) {
        for (i = 0; i < map->city->n_persons; i++) {
            if (map->city->persons[i].tile0 != 0 &&
                !(personIsJoinable(&(map->city->persons[i])) && personIsJoined(&(map->city->persons[i]))))
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

    /* apply effects from tile avatar is standing on */
    playerApplyEffect(c->saveGame, tileGetEffect(mapTileAt(c->map, c->saveGame->x, c->saveGame->y)));

    while (1) {
        /* adjust food and moves */
        playerEndTurn(c->saveGame);

        mapMoveObjects(c->map, c->saveGame->x, c->saveGame->y);

        /* update map annotations and the party stats */
        annotationCycle();
        c->statsItem = STATS_PARTY_OVERVIEW;
        statsUpdate();

        if (!playerPartyImmobilized(c->saveGame))
            break;

        if (playerPartyDead(c->saveGame)) {
            deathStart();
            return;
        } else {
            screenMessage("Zzzzzz\n");
        }
    }

    /* draw a prompt */
    screenMessage("\020");
}

/**
 * Inform a player he has lost zero or more eighths of avatarhood.
 */
void gameLostEighth(int eighths) {
    int i;

    for (i = 0; i < eighths; i++) {
        screenMessage("Thou hast lost an eighth!\n");
    }

    if (eighths)
        statsUpdate();
}

Context *gameCloneContext(Context *ctx) {
    Context *new;

    new = (Context *) malloc(sizeof(Context));
    new->parent = ctx;
    new->saveGame = new->parent->saveGame;
    new->col = new->parent->col;
    new->line = new->parent->line;
    new->statsItem = new->parent->statsItem;
    new->moonPhase = new->parent->moonPhase;
    new->windDirection = new->parent->windDirection;
    new->windCounter = new->parent->windCounter;
    new->moonPhase = new->parent->moonPhase;

    return new;
}

void gameCastSpell(unsigned int spell, int caster, int param) {
    SpellCastError spellError;

    if (!spellCast(spell, caster, param, &spellError)) {
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
        case CASTERR_FAILED:
            screenMessage("Failed!\n");
            break;
        case CASTERR_NOERROR:
        default:
            /* should never happen */
            assert(0);
        }
    }
}

int gameCheckPlayerDisabled(int player) {
    assert(player < c->saveGame->members);

    if (c->saveGame->players[player].status == STAT_DEAD ||
        c->saveGame->players[player].status == STAT_SLEEPING) {
        screenMessage("Disabled!\n");
        return 1;
    }

    return 0;
}


/**
 * The main key handler for the game.  Interpretes each key as a
 * command - 'a' for attack, 't' for talk, etc.
 */
int gameBaseKeyHandler(int key, void *data) {
    int valid = 1;
    Object *obj;
    const Portal *portal;
    CoordActionInfo *info;
    GetChoiceActionInfo *choiceInfo;
    AlphaActionInfo *alphaInfo;
    const ItemLocation *item;
    unsigned char tile;

    switch (key) {

    case U4_UP:
        moveAvatar(DIR_NORTH, 1);
        break;

    case U4_DOWN:
        moveAvatar(DIR_SOUTH, 1);
        break;

    case U4_LEFT:
        moveAvatar(DIR_WEST, 1);
        break;

    case U4_RIGHT:
        moveAvatar(DIR_EAST, 1);
        break;

    case 3:                     /* ctrl-C */
        screenMessage("Cmd: ");
        eventHandlerPushKeyHandler(&gameSpecialCmdKeyHandler);
        break;

    case 'a':
        info = (CoordActionInfo *) malloc(sizeof(CoordActionInfo));
        info->handleAtCoord = &attackAtCoord;
        info->range = 1;
        info->blockedPredicate = NULL;
        info->failedMessage = "FIXME";
        eventHandlerPushKeyHandlerData(&gameGetCoordinateKeyHandler, info);
        screenMessage("Attack\nDir: ");
        break;

    case 'b':
        obj = mapObjectAt(c->map, c->saveGame->x, c->saveGame->y);
        if (obj) {
            if (tileIsShip(obj->tile)) {
                if (c->saveGame->transport != AVATAR_TILE)
                    screenMessage("Board: Can't!\n");
                else {
                    c->saveGame->transport = obj->tile;
                    c->saveGame->shiphull = 50;
                    mapRemoveObject(c->map, obj);
                    screenMessage("Board Frigate!\n");
                }
            } else if (tileIsHorse(obj->tile)) {
                if (c->saveGame->transport != AVATAR_TILE)
                    screenMessage("Board: Can't!\n");
                else {
                    c->saveGame->transport = obj->tile;
                    mapRemoveObject(c->map, obj);
                    screenMessage("Mount Horse!\n");
                }
            } else if (tileIsBalloon(obj->tile)) {
                if (c->saveGame->transport != AVATAR_TILE)
                    screenMessage("Board: Can't!\n");
                else {
                    c->saveGame->transport = obj->tile;
                    mapRemoveObject(c->map, obj);
                    screenMessage("Board Balloon!\n");
                }
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
            gameSetMap(c, portal->destination, 0);
            screenMessage("Descend!\n\n");
        } else if (tileIsBalloon(c->saveGame->transport)) {
            screenMessage("Land Balloon\n");
            if (c->saveGame->balloonstate == 0)
                screenMessage("Already Landed!\n");
            c->saveGame->balloonstate = 0;
        } else
            screenMessage("Descend what?\n");
        break;

    case 'e':
        portal = mapPortalAt(c->map, c->saveGame->x, c->saveGame->y);
        if (portal && portal->trigger_action == ACTION_ENTER) {
            switch (portal->destination->type) {
            case MAP_TOWN:
                screenMessage("Enter towne!\n\n%s\n\n", portal->destination->city->name);
                break;
            case MAP_VILLAGE:
                screenMessage("Enter village!\n\n%s\n\n", portal->destination->city->name);
                break;
            case MAP_CASTLE:
                screenMessage("Enter castle!\n\n%s\n\n", portal->destination->city->name);
                break;
            case MAP_RUIN:
                screenMessage("Enter ruin!\n\n%s\n\n", portal->destination->city->name);
                break;
            case MAP_SHRINE:
                screenMessage("Enter the Shrine of %s!\n\n", getVirtueName(portal->destination->shrine->virtue));
                break;
            default:
                break;
            }

            /*
             * if trying to enter a shrine, ensure the player is
             * allowed in
             */
            if (portal->destination->type == MAP_SHRINE) {
                if (!playerCanEnterShrine(c->saveGame, portal->destination->shrine->virtue)) {
                    screenMessage("Thou dost not bear the rune of entry!  A strange force keeps you out!\n");
                    break;
                } else {
                    shrineEnter(portal->destination->shrine);
                }
            }

            annotationClear();  /* clear out world map annotations */

            c = gameCloneContext(c);

            gameSetMap(c, portal->destination, 1);
            musicPlay();

        } else
            screenMessage("Enter what?\n");
        break;

    case 'g':
        screenMessage("Get Chest!\n");
        if ((obj = mapObjectAt(c->map, c->saveGame->x, c->saveGame->y)) != NULL)
            tile = obj->tile;
        else
            tile = mapTileAt(c->map, c->saveGame->x, c->saveGame->y);
        if (tile == CHEST_TILE) {
            if (obj)
                mapRemoveObject(c->map, obj);
            else
                annotationAdd(c->saveGame->x, c->saveGame->y, -1, BRICKFLOOR_TILE);
            playerGetChest(c->saveGame);
            if (obj == NULL)
                gameLostEighth(playerAdjustKarma(c->saveGame, KA_STOLE_CHEST));
        } else
            screenMessage("Not Here!\n");
        break;

    case 'h':
        screenMessage("Hole up & Camp!\n");
        if (mapIsWorldMap(c->map))
            campBegin();
        else 
            screenMessage("Not here!\n");
        break;

    case 'i':
        screenMessage("Ignite torch!\nNot Here!\n");
        break;

    case 'j':
        info = (CoordActionInfo *) malloc(sizeof(CoordActionInfo));
        info->handleAtCoord = &jimmyAtCoord;
        info->range = 1;
        info->blockedPredicate = NULL;
        info->failedMessage = "Jimmy what?";
        eventHandlerPushKeyHandlerData(&gameGetCoordinateKeyHandler, info);
        screenMessage("Jimmy\nDir: ");
        break;
        
    case 'k':
        portal = mapPortalAt(c->map, c->saveGame->x, c->saveGame->y);
        if (portal && portal->trigger_action == ACTION_KLIMB) {
            if (c->saveGame->transport != AVATAR_TILE)
                screenMessage("Klimb\nOnly on foot!\n");
            else {
                gameSetMap(c, portal->destination, 0);
                screenMessage("Klimb!\n\n");
            }
        } else if (tileIsBalloon(c->saveGame->transport)) {
            c->saveGame->balloonstate = 1;
            screenMessage("Klimb Altitude!\n");
        } else
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

    case 'm':
        screenMessage("Mix reagents!\n");
        alphaInfo = (AlphaActionInfo *) malloc(sizeof(AlphaActionInfo));
        alphaInfo->lastValidLetter = 'z';
        alphaInfo->handleAlpha = mixReagentsForSpell;
        alphaInfo->prompt = "Spell: ";
        alphaInfo->data = NULL;

        screenMessage("%s", alphaInfo->prompt);
        eventHandlerPushKeyHandlerData(&gameGetAlphaChoiceKeyHandler, alphaInfo);

        c->statsItem = STATS_MIXTURES;
        statsUpdate();
        break;

    case 'n':
        eventHandlerPushKeyHandlerData(&gameGetPlayerNoKeyHandler, &newOrderForPlayer);
        screenMessage("New Order!\nExchange # ");
        break;

    case 'o':
        info = (CoordActionInfo *) malloc(sizeof(CoordActionInfo));
        info->handleAtCoord = &openAtCoord;
        info->range = 1;
        info->blockedPredicate = NULL;
        info->failedMessage = "Not Here!";
        eventHandlerPushKeyHandlerData(&gameGetCoordinateKeyHandler, info);
        screenMessage("Open\nDir: ");
        break;

    case 'p':
        if (c->saveGame->gems) {
            c->saveGame->gems--;
            viewMode = VIEW_GEM;
            choiceInfo = (GetChoiceActionInfo *) malloc(sizeof(GetChoiceActionInfo));
            choiceInfo->choices = " \033";
            choiceInfo->handleChoice = &gemHandleChoice;
            eventHandlerPushKeyHandlerData(&keyHandlerGetChoice, choiceInfo);
            screenMessage("Peer at a Gem!\n");
        } else
            screenMessage("Peer at What?\n");
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

    case 's':
        screenMessage("Searching...\n");
        item = itemAtLocation(c->map, c->saveGame->x, c->saveGame->y);
        if (item) {
            if ((*item->isItemInInventory)(item->data))
                screenMessage("Nothing Here!\n");
            else {
                (*item->putItemInInventory)(item->data);
                screenMessage("You find...\n%s!\n", item->name);
            }
        } else
            screenMessage("Nothing Here!\n");
        break;

    case 't':
        info = (CoordActionInfo *) malloc(sizeof(CoordActionInfo));
        info->handleAtCoord = &talkAtCoord;
        info->range = 2;
        info->blockedPredicate = &tileCanTalkOver;
        info->failedMessage = "Funny, no\nresponse!";
        eventHandlerPushKeyHandlerData(&gameGetCoordinateKeyHandler, info);
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
        if (c->saveGame->transport != AVATAR_TILE && c->saveGame->balloonstate == 0) {
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
        (*handlePlayerNo)(key - '1');
    } else {
        screenMessage("None\n");
        gameFinishTurn();
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
        eventHandlerPopKeyHandler();
        (*(info->handleAlpha))(key - 'a', info->data);
        free(info);
    } else if (key == ' ' || key == U4_ESC) {
        screenMessage("\n");
        eventHandlerPopKeyHandler();
        free(info);
        gameFinishTurn();
    } else {
        valid = 0;
        screenMessage("\n%s", info->prompt);
        screenRedrawScreen();
    }

    return valid || keyHandlerDefault(key, NULL);
}

int gameGetDirectionKeyHandler(int key, void *data) {
    int (*handleDirection)(Direction dir) = (int(*)(Direction))data;
    int valid = 1;
    Direction dir;

    switch (key) {
    case U4_UP:
        dir = DIR_NORTH;
        break;
    case U4_DOWN:
        dir = DIR_SOUTH;
        break;
    case U4_LEFT:
        dir = DIR_WEST;
        break;
    case U4_RIGHT:
        dir = DIR_EAST;
        break;
    default:
        valid = 0;
        break;
    }

    if (valid) {
        eventHandlerPopKeyHandler();

        screenMessage("%s\n", getDirectionName(dir));
        (*handleDirection)(dir);
    }

    return valid || keyHandlerDefault(key, NULL);
}

/**
 * Handles key presses for a command requiring a direction argument.
 * Once an arrow key is pressed, control is handed off to a command
 * specific routine.
 */
int gameGetCoordinateKeyHandler(int key, void *data) {
    CoordActionInfo *info = (CoordActionInfo *) data;
    int valid = 1;
    int i;
    Direction dir;
    int t_x = c->saveGame->x, t_y = c->saveGame->y;

    eventHandlerPopKeyHandler();

    switch (key) {
    case U4_UP:
        dir = DIR_NORTH;
        break;
    case U4_DOWN:
        dir = DIR_SOUTH;
        break;
    case U4_LEFT:
        dir = DIR_WEST;
        break;
    case U4_RIGHT:
        dir = DIR_EAST;
        break;
    default:
        valid = 0;
        break;
    }

    if (valid) {
        screenMessage("%s\n", getDirectionName(dir));

        /* 
         * try every tile in the given direction, up to the given
         * range.  Stop when the command handler succeeds, the range
         * is exceeded, or the action is blocked.
         */
        for (i = 1; i <= info->range; i++) {
            dirMove(dir, &t_x, &t_y);

            if ((*(info->handleAtCoord))(t_x, t_y))
                goto success;
            if (info->blockedPredicate &&
                !(*(info->blockedPredicate))(mapTileAt(c->map, t_x, t_y)))
                break;
        }
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

    if (key == '0')
        c->statsItem = STATS_WEAPONS;
    else if (key >= '1' && key <= '8' && (key - '1' + 1) <= c->saveGame->members)
        c->statsItem = STATS_CHAR1 + (key - '1');
    else if (key == '\033') {
        screenMessage("\n");
        eventHandlerPopKeyHandler();
        gameFinishTurn();
        return 1;
    }
    else
        valid = 0;

    if (valid) {
        screenMessage("%c\n", key);
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
    const Moongate *moongate;
    int valid = 1;

    switch (key) {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
        moongate = moongateGetGateForPhase(key - '0');
        c->saveGame->x = moongate->x;
        c->saveGame->y = moongate->y;
        screenMessage("Gate %d!\n", key - '0');
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
        screenMessage("Help:\n"
                      "0-7 - gate to city\n"
                      "c - Collision\ne - Equipment\nh - Help\ni - Items\nk - Show Karma\n"
                      "m - Mixtures\nr - Reagents\nt - Transports\nw - Winds\n"
                      "\020");
        break;
    case 'i':
        screenMessage("Items!\n\020");
        c->saveGame->torches = 99;
        c->saveGame->gems = 99;
        c->saveGame->keys = 99;
        c->saveGame->sextants = 1;
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
        for (i = 0; i < SPELL_MAX; i++)
            c->saveGame->mixtures[i] = 99;
        break;
    case 'r':
        screenMessage("Reagents!\n\020");
        for (i = 0; i < REAG_MAX; i++)
            c->saveGame->reagents[i] = 99;
        break;
    case 't':
        if (mapIsWorldMap(c->map)) {
            mapAddObject(c->map, 20, 20, 84, 106);
            mapAddObject(c->map, 16, 16, 88, 109);
            mapAddObject(c->map, 24, 24, 85, 105);
            screenMessage("Transports: Ship, Horse and Balloon created!\n\020");
        }
        break;
    case 'w':
        c->windDirection++;
        if (c->windDirection >= 4)
            c->windDirection = 0;
        screenMessage("Change Wind Direction\n");
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

    combatBegin(mapTileAt(c->map, c->saveGame->x, c->saveGame->y), c->saveGame->transport);

    return 1;
}

int castPlayer;
unsigned int castSpell;

int castForPlayer(int player) {
    AlphaActionInfo *info;

    castPlayer = player;

    if (gameCheckPlayerDisabled(player)) {
        gameFinishTurn();
        return 0;
    }

    c->statsItem = STATS_MIXTURES;
    statsUpdate();

    info = (AlphaActionInfo *) malloc(sizeof(AlphaActionInfo));
    info->lastValidLetter = 'z';
    info->handleAlpha = castForPlayer2;
    info->prompt = "Spell: ";
    info->data = NULL;

    screenMessage("%s", info->prompt);

    eventHandlerPushKeyHandlerData(&gameGetAlphaChoiceKeyHandler, info);

    return 1;
}

int castForPlayer2(int spell, void *data) {
    castSpell = spell;

    screenMessage("%s!\n", spellGetName(spell));

    c->statsItem = STATS_PARTY_OVERVIEW;
    statsUpdate();

    switch (spellGetParamType(spell)) {
    case SPELLPRM_NONE:
    case SPELLPRM_PHASE:
        gameCastSpell(castSpell, castPlayer, 0);
        gameFinishTurn();
        break;
    case SPELLPRM_PLAYER:
        screenMessage("Player: ");
        eventHandlerPushKeyHandlerData(&gameGetPlayerNoKeyHandler, &castForPlayerGetDestPlayer);
        break;
    case SPELLPRM_DIR:
    case SPELLPRM_TYPEDIR:
        screenMessage("Dir: ");
        eventHandlerPushKeyHandlerData(&gameGetDirectionKeyHandler, &castForPlayerGetDestDir);
        break;
    case SPELLPRM_FROMDIR:
        screenMessage("From Dir: ");
        eventHandlerPushKeyHandlerData(&gameGetDirectionKeyHandler, &castForPlayerGetDestDir);
        break;
    }

    return 1;
}

int castForPlayerGetDestPlayer(int player) {
    gameCastSpell(castSpell, castPlayer, player);
    gameFinishTurn();
    return 1;
}

int castForPlayerGetDestDir(Direction dir) {
    gameCastSpell(castSpell, castPlayer, (int) dir);
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

/* FIXME */
Mixture *mix;
int mixSpell;

/**
 * Mixes reagents for a spell.  Prompts for reagents.
 */
int mixReagentsForSpell(int spell, void *data) {
    AlphaActionInfo *info;

    mixSpell = spell;
    mix = mixtureNew();

    screenMessage("%s!\n", spellGetName(spell));

    c->statsItem = STATS_REAGENTS;
    statsUpdate();

    info = (AlphaActionInfo *) malloc(sizeof(AlphaActionInfo));
    info->lastValidLetter = REAG_MAX + 'a' - 1;
    info->handleAlpha = mixReagentsForSpell2;
    info->prompt = "Reagent: ";
    info->data = (void *) spell;

    eventHandlerPushKeyHandlerData(&gameGetAlphaChoiceKeyHandler, info);

    return 0;
}

int mixReagentsForSpell2(int spell, void *data) {
    AlphaActionInfo *info;

    info = (AlphaActionInfo *) malloc(sizeof(AlphaActionInfo));
    info->lastValidLetter = REAG_MAX + 'a' - 1;
    info->handleAlpha = mixReagentsForSpell2;
    info->prompt = "Reagent: ";
    info->data = (void *) spell;

    eventHandlerPushKeyHandlerData(&gameGetAlphaChoiceKeyHandler, info);

    gameFinishTurn();
    return 0;
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

    annotationAdd(x, y, 4, BRICKFLOOR_TILE);
    screenMessage("\nOpened!\n");
    gameFinishTurn();

    return 1;
}

/**
 * Waits for space bar to return from gem mode.
 */
int gemHandleChoice(char choice) {
    eventHandlerPopKeyHandler();

    viewMode = VIEW_NORMAL;
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

    if (x == -1 && y == -1) {
        gameFinishTurn();
        return 0;
    }

    c->conversation.talker = mapPersonAt(c->map, x, y);
    if (c->conversation.talker == NULL) {
        return 0;
    }

    talker = c->conversation.talker;
    c->conversation.state = CONV_INTRO;
    c->conversation.buffer[0] = '\0';
    
    personGetConversationText(&c->conversation, "", &text);
    screenMessage("\n\n%s", text);
    free(text);
    if (c->conversation.state == CONV_DONE)
        gameFinishTurn();
    else
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
    case CONV_BUY_QUANTITY:
    case CONV_SELL_QUANTITY:
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

    case CONV_BUY_ITEM:
    case CONV_SELL_ITEM:
        gcInfo = (GetChoiceActionInfo *) malloc(sizeof(GetChoiceActionInfo));
        gcInfo->choices = "bcdefghijklmnopqrstuvwxyz\033";
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
    if (c->saveGame->x == x && c->saveGame->y == y)
        tile = c->saveGame->transport;
    else if ((obj = mapObjectAt(map, x, y)) != NULL)
        tile = obj->tile;
    else
        tile = mapTileAt(map, x, y);

    /* if the party in is a ship, check sailable, otherwise walkable */
    if (tileIsShip(c->saveGame->transport)) {
        if (!tileIsSailable(tile))
            return 0;
    }
    else if (tileIsBalloon(c->saveGame->transport)) {
        if (!tileIsFlyable(tile))
            return 0;
    }
    else if (!tileIsWalkable(tile))
        return 0;

    return 1;
}

/**
 * Attempt to move the avatar in the given direction.  User event
 * should be set if the avatar is being moved in response to a
 * keystroke.  Returns zero if the avatar is blocked.
 */
int moveAvatar(Direction dir, int userEvent) {
    int result = 1;
    int newx, newy;

    if (tileIsBalloon(c->saveGame->transport) && userEvent) {
        screenMessage("Drift Only!\n");
        goto done;
    }

    if (tileIsShip(c->saveGame->transport)) {
        if (tileGetDirection(c->saveGame->transport) != dir) {
            tileSetDirection(&c->saveGame->transport, dir);
            screenMessage("Turn %s!\n", getDirectionName(dir));
            goto done;
        }
    }

    if (tileIsHorse(c->saveGame->transport)) {
        if ((dir == DIR_WEST || dir == DIR_EAST) &&
            tileGetDirection(c->saveGame->transport) != dir) {
            tileSetDirection(&c->saveGame->transport, dir);
        }
    }

    newx = c->saveGame->x;
    newy = c->saveGame->y;
    dirMove(dir, &newx, &newy);

    if (tileIsShip(c->saveGame->transport))
        screenMessage("Sail %s!\n", getDirectionName(dir));
    else if (!tileIsBalloon(c->saveGame->transport))
        screenMessage("%s\n", getDirectionName(dir));

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
                c->parent->windDirection = c->windDirection;
                c->parent->windCounter = c->windCounter;
		c = c->parent;
                c->col = 0;
		free(t);
                
                musicPlay();
	    }
            goto done;
	    
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
            result = 0;
            goto done;
        }

        /* 
         * special cases for tile 0x0e: the center tile of the castle
         * of lord british, which is walkable from the south only
         */
        if (mapTileAt(c->map, newx, newy) == 0x0e && dir == DIR_SOUTH) {
            screenMessage("Blocked!\n");
            result = 0;
            goto done;
        }
        if (mapTileAt(c->map, c->saveGame->x, c->saveGame->y) == 0x0e && dir == DIR_NORTH) {
            screenMessage("Blocked!\n");
            result = 0;
            goto done;
        }
    }

    c->saveGame->x = newx;
    c->saveGame->y = newy;

    gameCheckMoongates();

 done:

    return result;
}

/**
 * This function is called every quarter second.
 */
void gameTimer() {
    int oldTrammel, trammelSubphase;
    const Moongate *gate;

    Direction dir = DIR_WEST;
    if (++c->windCounter >= MOON_SECONDS_PER_PHASE * 4) {
        if ((rand() % 4) == 1)
            c->windDirection = rand() % 4;
        c->windCounter = 0;
        if (tileIsBalloon(c->saveGame->transport) &&
            c->saveGame->balloonstate) {
            dir = dirReverse(c->windDirection);
            moveAvatar(dir, 0);
        }
    }

    /* update moon phases */
    if (mapIsWorldMap(c->map)) {
        oldTrammel = c->saveGame->trammelphase;

        if (++c->moonPhase >= (MOON_SECONDS_PER_PHASE * 4 * MOON_PHASES))
            c->moonPhase = 0;

        c->saveGame->trammelphase = c->moonPhase / (MOON_SECONDS_PER_PHASE * 4) / 3;
        c->saveGame->feluccaphase = c->moonPhase / (MOON_SECONDS_PER_PHASE * 4) % 8;

        if (--c->saveGame->trammelphase > 7)
            c->saveGame->trammelphase = 7;
        if (--c->saveGame->feluccaphase > 7)
            c->saveGame->feluccaphase = 7;

        trammelSubphase = c->moonPhase % (MOON_SECONDS_PER_PHASE * 4 * 3);

        /* update the moongates if trammel changed */
        if (trammelSubphase == 0) {
            gate = moongateGetGateForPhase(oldTrammel);
            annotationRemove(gate->x, gate->y, MOONGATE0_TILE);
            gate = moongateGetGateForPhase(c->saveGame->trammelphase);
            annotationAdd(gate->x, gate->y, -1, MOONGATE0_TILE);
        }
        else if (trammelSubphase == 1) {
            gate = moongateGetGateForPhase(c->saveGame->trammelphase);
            annotationRemove(gate->x, gate->y, MOONGATE0_TILE);
            annotationAdd(gate->x, gate->y, -1, MOONGATE1_TILE);
        }
        else if (trammelSubphase == 2) {
            gate = moongateGetGateForPhase(c->saveGame->trammelphase);
            annotationRemove(gate->x, gate->y, MOONGATE1_TILE);
            annotationAdd(gate->x, gate->y, -1, MOONGATE2_TILE);
        }
        else if (trammelSubphase == 3) {
            gate = moongateGetGateForPhase(c->saveGame->trammelphase);
            annotationRemove(gate->x, gate->y, MOONGATE2_TILE);
            annotationAdd(gate->x, gate->y, -1, MOONGATE3_TILE);
        }
        else if (trammelSubphase == (MOON_SECONDS_PER_PHASE * 4 * 3) - 3) {
            gate = moongateGetGateForPhase(c->saveGame->trammelphase);
            annotationRemove(gate->x, gate->y, MOONGATE3_TILE);
            annotationAdd(gate->x, gate->y, -1, MOONGATE2_TILE);
        }
        else if (trammelSubphase == (MOON_SECONDS_PER_PHASE * 4 * 3) - 2) {
            gate = moongateGetGateForPhase(c->saveGame->trammelphase);
            annotationRemove(gate->x, gate->y, MOONGATE2_TILE);
            annotationAdd(gate->x, gate->y, -1, MOONGATE1_TILE);
        }
        else if (trammelSubphase == (MOON_SECONDS_PER_PHASE * 4 * 3) - 1) {
            gate = moongateGetGateForPhase(c->saveGame->trammelphase);
            annotationRemove(gate->x, gate->y, MOONGATE1_TILE);
            annotationAdd(gate->x, gate->y, -1, MOONGATE0_TILE);
        }
    }

    mapAnimateObjects(c->map);

    screenCycle();
    gameUpdateScreen();
}

void gameCheckMoongates() {
    int destx, desty;
    extern Map shrine_spirituality_map;
    
    if (moongateFindActiveGateAt(c->saveGame->trammelphase, c->saveGame->feluccaphase, 
                                 c->saveGame->x, c->saveGame->y, &destx, &desty)) {
        
        /* FIXME: special effect here */
        c->saveGame->x = destx;
        c->saveGame->y = desty;

        if (moongateIsEntryToShrineOfSpirituality(c->saveGame->trammelphase, c->saveGame->feluccaphase)) {
            if (!playerCanEnterShrine(c->saveGame, shrine_spirituality_map.shrine->virtue))
                return;
            else
                shrineEnter(shrine_spirituality_map.shrine);

            annotationClear();  /* clear out world map annotations */

            c = gameCloneContext(c);

            gameSetMap(c, &shrine_spirituality_map, 1);
            musicPlay();
        }
    }
}
