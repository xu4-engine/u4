/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "u4.h"

#include "game.h"

#include "annotation.h"
#include "armor.h"
#include "camp.h"
#include "city.h"
#include "dungeon.h"
#include "combat.h"
#include "context.h"
#include "death.h"
#include "debug.h"
#include "direction.h"
#include "error.h"
#include "event.h"
#include "item.h"
#include "location.h"
#include "mapmgr.h"
#include "monster.h"
#include "moongate.h"
#include "movement.h"
#include "music.h"
#include "names.h"
#include "person.h"
#include "player.h"
#include "portal.h"
#include "savegame.h"
#include "screen.h"
#include "settings.h"
#include "shrine.h"
#include "sound.h"
#include "spell.h"
#include "stats.h"
#include "ttype.h"
#include "vendor.h"
#include "weapon.h"

int gameSave(void);
void gameLostEighth(Virtue virtue);
void gameAdvanceLevel(const SaveGamePlayerRecord *player);
void gamePartyStarving(void);
void gameSpellEffect(unsigned int spell, int player, int hzSound);
void gameCastSpell(unsigned int spell, int caster, int param);
void gameInnHandler(void);
int gameCheckPlayerDisabled(int player);
void gameGetPlayerForCommand(int (*commandFn)(int player));
int moveAvatar(Direction dir, int userEvent);
int attackAtCoord(int x, int y, int distance, void *data);
int castForPlayer2(int spell, void *data);
int castForPlayerGetDestPlayer(int player);
int castForPlayerGetDestDir(Direction dir);
int castForPlayerGetPhase(int phase);
int destroyAtCoord(int x, int y, int distance, void *data);
int getChestTrapHandler(int player);
int jimmyAtCoord(int x, int y, int distance, void *data);
int mixReagentsForSpell(int spell, void *data);
int mixReagentsForSpell2(int choice);
int newOrderForPlayer(int player);
int newOrderForPlayer2(int player2);
int openAtCoord(int x, int y, int distance, void *data);
int gemHandleChoice(int choice);
int peerCityHandleChoice(int choice);
int readyForPlayer(int player);
int talkAtCoord(int x, int y, int distance, void *data);
int talkHandleBuffer(const char *message);
int talkHandleChoice(int choice);
void talkShowReply(int showPrompt);
int wearForPlayer(int player);
int wearForPlayer2(int armor, void *data);
int ztatsFor(int player);
int cmdHandleAnyKey(int key, void *data);
int windCmdKeyHandler(int key, void *data);
void gameCheckBridgeTrolls(void);
void gameCheckSpecialMonsters(Direction dir);
int gameCheckMoongates(void);
void gameUpdateMoons(int showmoongates);
void gameInitMoons();
void gameCheckRandomMonsters(void);
void gameFixupMonsters(void);
long gameTimeSinceLastCommand(void);
void gameMonsterAttack(Object *obj);
void gameLordBritishCheckLevels(void);
int gameSummonMonster(const char *monsterName);
void gameDestroyAllCreatures(void);
int gameCreateBalloon(Map *map);

extern Object *party[8];
Context *c = NULL;
int collisionOverride = 0;
int windLock = 0;
char itemNameBuffer[16];
char monsterNameBuffer[32];
int paused = 0;
int pausedTimer = 0;

void gameInit() {    
    FILE *saveGameFile, *monstersFile;

    /* initialize the global game context */
    c = (Context *) malloc(sizeof(Context));
    c->saveGame = (SaveGame *) malloc(sizeof(SaveGame));    
    c->annotation = NULL;    
    c->location = locationNew(0, 0, 0, mapMgrGetById(MAP_WORLD), VIEW_NORMAL, CTX_WORLDMAP, (FinishTurnCallback)&gameFinishTurn, NULL);
    c->conversation.talker = NULL;
    c->conversation.state = 0;
    c->conversation.playerInquiryBuffer[0] = '\0';
    c->conversation.reply = NULL;
    c->conversation.replyLine = 0;
    c->line = TEXT_AREA_H - 1;
    c->col = 0;
    c->statsItem = STATS_PARTY_OVERVIEW;        
    c->moonPhase = 0;
    c->windDirection = DIR_NORTH;
    c->windCounter = 0;
    c->aura = AURA_NONE;
    c->auraDuration = 0;
    c->horseSpeed = 0;
    c->opacity = 1;
    c->lastCommandTime = time(NULL);
    c->lastShip = NULL;    

    /* load in the save game */
    saveGameFile = saveGameOpenForReading();
    if (saveGameFile) {
        saveGameRead(c->saveGame, saveGameFile);
        fclose(saveGameFile);
    } else
        errorFatal("no savegame found!");

    c->location->x = c->saveGame->x;
    c->location->y = c->saveGame->y;
    c->location->z = c->saveGame->dnglevel;

    /* initialize the moons */
    gameInitMoons();

    /* load in monsters.sav */
    monstersFile = saveGameMonstersOpenForReading();
    if (monstersFile) {
        saveGameMonstersRead(&c->location->map->objects, monstersFile);
        fclose(monstersFile);
    }
    gameFixupMonsters();

    /* setup transport context */
    gameSetTransport((unsigned char)c->saveGame->transport);

    playerSetLostEighthCallback(&gameLostEighth);
    playerSetAdvanceLevelCallback(&gameAdvanceLevel);
    playerSetItemStatsChangedCallback(&statsUpdate);
    playerSetSpellEffectCallback(&gameSpellEffect);
    playerSetPartyStarvingCallback(&gamePartyStarving);
    playerSetSetTransportCallback(&gameSetTransport);
    itemSetDestroyAllMonstersCallback(&gameDestroyAllCreatures);
    vendorSetInnHandlerCallback(&innBegin);

    musicPlay();
    screenDrawBackground(BKGD_BORDERS);
    statsUpdate();
    screenPrompt();
}

/**
 * Saves the game state into party.sav and monsters.sav.
 */
int gameSave() {
    FILE *saveGameFile, *monstersFile;

    /*************************************************/
    /* Make sure the savegame struct is accurate now */
    
    c->saveGame->x = c->location->x;
    c->saveGame->y = c->location->y;
    c->saveGame->dnglevel = c->location->z;

    /* Done making sure the savegame struct is accurate */
    /****************************************************/

    saveGameFile = saveGameOpenForWriting();
    if (!saveGameFile) {
        screenMessage("Error opening party.sav\n");
        return 0;
    }

    if (!saveGameWrite(c->saveGame, saveGameFile)) {
        screenMessage("Error writing to party.sav\n");
        fclose(saveGameFile);
        return 0;
    }
    fclose(saveGameFile);

    monstersFile = saveGameMonstersOpenForWriting();
    if (!monstersFile) {
        screenMessage("Error opening monsters.sav\n");
        return 0;
    }

    /* fix monster animations so they are compatible with u4dos */
    mapResetObjectAnimations(c->location->map);

    if (!saveGameMonstersWrite(c->location->map->objects, monstersFile)) {
        screenMessage("Error opening monsters.sav\n");
        fclose(monstersFile);
        return 0;
    }
    fclose(monstersFile);

    return 1;
}

/**
 * Sets the view mode.
 */
void gameSetViewMode(ViewMode newMode) {
    c->location->viewMode = newMode;
}

void gameUpdateScreen() {
    switch (c->location->viewMode) {
    case VIEW_NORMAL:
        screenUpdate(1, 0);
        break;
    case VIEW_GEM:
        screenGemUpdate();
        break;
    case VIEW_RUNE:
        screenUpdate(0, 0);
        break;
    case VIEW_DUNGEON:
        screenUpdate(1, 0);
        break;
    case VIEW_DEAD:
        screenUpdate(1, 1);
        break;
    default:
        ASSERT(0, "invalid view mode: %d", c->location->viewMode);
    }
}

void gameSetMap(Context *ct, Map *map, int saveLocation, const Portal *portal) {
    int i, x, y, z, viewMode;    
    LocationContext context;
    FinishTurnCallback finishTurn = &gameFinishTurn;    

    if (portal) {
        x = portal->startx;
        y = portal->starty;
        z = portal->startlevel;
    } else {
        x = map->width / 2;
        y = map->height / 2;
        z = 0;
    }

    /* If we don't want to save the location, then just return to the previous location,
       as there may still be ones in the stack we want to keep */
    if (!saveLocation)
        gameExitToParentMap(ct);
    
    switch (map->type) {
    case MAPTYPE_DUNGEON:
        context = CTX_DUNGEON;
        viewMode = VIEW_DUNGEON;
        break;
    case MAPTYPE_COMBAT:        
        context = CTX_COMBAT;
        viewMode = VIEW_NORMAL;
        finishTurn = &combatFinishTurn;
        break;
    case MAPTYPE_TOWN:
    case MAPTYPE_VILLAGE:
    case MAPTYPE_CASTLE:
    case MAPTYPE_RUIN:
    default:
        context = CTX_CITY;
        viewMode = VIEW_NORMAL;
        break;
    }
    
    ct->location = locationNew(x, y, z, map, viewMode, context, finishTurn, ct->location);    

    if ((map->type == MAPTYPE_TOWN ||
         map->type == MAPTYPE_VILLAGE ||
         map->type == MAPTYPE_CASTLE ||
         map->type == MAPTYPE_RUIN) &&
         map->objects == NULL) {
        for (i = 0; i < map->city->n_persons; i++) {
            if (map->city->persons[i].tile0 != 0 &&
                !(playerCanPersonJoin(c->saveGame, map->city->persons[i].name, NULL) &&
                  playerIsPersonJoined(c->saveGame, map->city->persons[i].name)))
                mapAddPersonObject(map, &(map->city->persons[i]));
        }
    }
}

/**
 * Exits the current map and location and returns to its parent location
 * This restores all relevant information from the previous location,
 * such as the map, map position, etc. (such as exiting a city)
 **/

int gameExitToParentMap(struct _Context *ct) {

    if (ct->location->prev != NULL) {
        if (ct->location->map->id == 23) /* hythloth dungeon */
            gameCreateBalloon(ct->location->prev->map);            

        /* free map info only if previous location was on a different map */
        if (ct->location->prev->map != c->location->map) {
            annotationClear(c->location->map->id);
            mapClearObjects(c->location->map);
        }
        locationFree(&ct->location);
        
        return 1;
    }
    return 0;
}

/**
 * Terminates a game turn.  This performs the post-turn housekeeping
 * tasks like adjusting the party's food, incrementing the number of
 * moves, etc.
 */
void gameFinishTurn() {
    Object *attacker;    

    while (1) {
        /* adjust food and moves */
        playerEndTurn();

        /* check if aura has expired */
        if (c->auraDuration > 0) {
            if (--c->auraDuration == 0)
                c->aura = AURA_NONE;
        }

        gameCheckHullIntegrity();

        /* Monsters cannot spawn, move or attack while the avatar is on the balloon */
        if (!c->saveGame->balloonstate) {

            /* apply effects from tile avatar is standing on */
            playerApplyEffect(c->saveGame, tileGetEffect(mapGroundTileAt(c->location->map, c->location->x, c->location->y, c->location->z)), ALL_PLAYERS);

            /* Move monsters and see if something is attacking the avatar */
            attacker = mapMoveObjects(c->location->map, c->location->x, c->location->y, c->location->z);        

            /* Something's attacking!  Start combat! */
            if (attacker) {
                gameMonsterAttack(attacker);
                return;
            }       

            /* Spawn new monsters */
            gameCheckRandomMonsters();            
            gameCheckBridgeTrolls();
        }

        /* update map annotations and the party stats */
        annotationCycle();
        c->statsItem = STATS_PARTY_OVERVIEW;
        statsUpdate();

        if (!playerPartyImmobilized(c->saveGame))
            break;

        if (playerPartyDead(c->saveGame)) {
            deathStart(0);
            return;
        } else {            
            screenMessage("Zzzzzz\n");
        }
    }

    /* draw a prompt */
    screenPrompt();
    screenRedrawTextArea(TEXT_AREA_X, TEXT_AREA_Y, TEXT_AREA_W, TEXT_AREA_H);

    c->lastCommandTime = time(NULL);
}

/**
 * Inform a player he has lost zero or more eighths of avatarhood.
 */
void gameLostEighth(Virtue virtue) {
    screenMessage("\n Thou hast lost\n  an eighth!\n");
    statsUpdate();
}

void gameAdvanceLevel(const SaveGamePlayerRecord *player) {
    screenMessage("\n%s\nThou art now Level %d\n", player->name, playerGetRealLevel(player));

    (*spellEffectCallback)('r', -1, 0); // Same as resurrect spell
}

void gamePartyStarving(void) {
    int i;
    
    screenMessage("\nStarving!!!\n");
    /* FIXME: add sound effect here */

    /* Do 2 damage to each party member for starving! */
    for (i = 0; i < c->saveGame->members; i++)
        playerApplyDamage(&c->saveGame->players[i], 2);    
}

void gameSpellEffect(unsigned int spell, int player, int hzSound) {
    int time;
    SpellEffect effect = SPELLEFFECT_INVERT;
        
    if (player >= 0)
        statsHighlightCharacter(player);

    /* recalculate spell speed - based on 5/sec */
    time = settings->spellEffectSpeed * 200;

    /**
     * special effect FIXME: needs sound     
     */

    /* I'm not sure if this is how this will be done in the end...
       just showing why hzSound is included... :) */
    
    /*if (hzSound)
        soundPlay(SOUND_MAGIC_HZ);*/

    soundPlay(SOUND_MAGIC);

    switch(spell)
    {
    case 'g': /* gate */
    case 'r': /* resurrection */
        time = (time * 3) / 2;
        break;
    case 't': /* tremor */
        time = (time * 3) / 2;
        effect = SPELLEFFECT_TREMOR;        
        break;
    default:
        /* default spell effect */        
        break;
    }

    /* pause the game for enough time to complete the spell effect */
    if (!paused) {
        paused = 1;
        pausedTimer = ((time * settings->gameCyclesPerSecond) / 1000) + 1;
    }

    switch(effect)
    {
    case SPELLEFFECT_NONE: 
        break;
    case SPELLEFFECT_TREMOR:
    case SPELLEFFECT_INVERT:
        gameUpdateScreen();
        screenInvertRect(BORDER_WIDTH, BORDER_HEIGHT, VIEWPORT_W * TILE_WIDTH, VIEWPORT_H * TILE_HEIGHT);
        screenRedrawScreen();
        
        eventHandlerSleep(time);

        if (effect == SPELLEFFECT_TREMOR) {
            /* screen shaking is an xu4 enhancement -- make sure it's turned on! */
            if (settings->minorEnhancements && settings->minorEnhancementsOptions.screenShakes) {
                gameUpdateScreen();
                screenShake(10);
            }
        }

        break;
    }
    
    statsUpdate();
}

void gameCastSpell(unsigned int spell, int caster, int param) {
    SpellCastError spellError;
    const char *msg = NULL;    
    
    if (!spellCast(spell, caster, param, &spellError, 1)) {        
        msg = spellGetErrorMessage(spell, spellError);
        if (msg)
            screenMessage(msg);
    }    
}

int gameCheckPlayerDisabled(int player) {
    ASSERT(player < c->saveGame->members, "player %d, but only %d members\n", player, c->saveGame->members);

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
    CoordActionInfo *info;    
    AlphaActionInfo *alphaInfo;
    ReadBufferActionInfo *readBufferInfo;
    const ItemLocation *item;
    unsigned char tile;

    switch (key) {

    case U4_UP:
    case U4_DOWN:
    case U4_LEFT:
    case U4_RIGHT:
        /* Check to see if we're on the balloon */
        if (c->transportContext != TRANSPORT_BALLOON) {

            /* move the avatar */
            int moved = moveAvatar(keyToDirection(key), 1);
            
            /* horse doubles speed (make sure we're on the same map as the previous move first) */
            if (moved && (c->transportContext == TRANSPORT_HORSE) && c->horseSpeed) {
                gameUpdateScreen(); /* to give it a smooth look of movement */
                moveAvatar(keyToDirection(key), 0);
            }
        } else {
            screenMessage("Drift Only!\n");
        }
        break;

    case 3:                     /* ctrl-C */
        if (settings->debug) {
            screenMessage("Cmd (h = help):");
            eventHandlerPushKeyHandler(&gameSpecialCmdKeyHandler);
        }
        else valid = 0;
        break;

    case 4:                     /* ctrl-D */
        if (settings->debug) {
            info = (CoordActionInfo *) malloc(sizeof(CoordActionInfo));
            info->handleAtCoord = &destroyAtCoord;
            info->origin_x = c->location->x;
            info->origin_y = c->location->y;
            info->prev_x = info->prev_y = -1;
            info->range = 1;
            info->validDirections = MASK_DIR_ALL;
            info->blockedPredicate = NULL;
            info->blockBefore = 0;
            info->firstValidDistance = 1;
            eventHandlerPushKeyHandlerData(&gameGetCoordinateKeyHandler, info);
            screenMessage("Destroy Object\nDir: ");
        }
        else valid = 0;
        break;    

    case 8:                     /* ctrl-H */
        if (settings->debug) {
            screenMessage("Help!\n");            
            screenPrompt();
            
            /* Help! send me to Lord British (who conveniently is right around where you are)! */
            while (c->location->prev)
                gameExitToParentMap(c);

            gameSetMap(c, mapMgrGetById(100), 1, NULL);
            c->location->x = 19;
            c->location->y = 8;
            c->location->z = 1;            
        }
        else valid = 0;
        break;
    
    case ' ':
        screenMessage("Pass\n");        
        break;

    case 'a':
        screenMessage("Attack: ");

        if (c->saveGame->balloonstate)
            screenMessage("\nDrift only!\n");
        else {
            info = (CoordActionInfo *) malloc(sizeof(CoordActionInfo));
            info->handleAtCoord = &attackAtCoord;
            info->origin_x = c->location->x;
            info->origin_y = c->location->y;
            info->prev_x = info->prev_y = -1;
            info->range = 1;
            info->validDirections = MASK_DIR_ALL;
            info->blockedPredicate = NULL;
            info->blockBefore = 0;
            info->firstValidDistance = 1;
            eventHandlerPushKeyHandlerData(&gameGetCoordinateKeyHandler, info);            
        }
        break;

    case 'b':

        obj = mapObjectAt(c->location->map, c->location->x, c->location->y, c->location->z);

        if (c->transportContext != TRANSPORT_FOOT)
            screenMessage("Board: Can't!\n");
        else if (obj) {
            int valid = 1;
            
            if (tileIsShip(obj->tile)) {
                screenMessage("Board Frigate!\n");
                if (c->lastShip != obj)
                    c->saveGame->shiphull = 50;
            }
            else if (tileIsHorse(obj->tile))
                screenMessage("Mount Horse!\n");
            else if (tileIsBalloon(obj->tile))
                screenMessage("Board Balloon!\n");
            else valid = 0;

            if (valid) {
                gameSetTransport(obj->tile);
                mapRemoveObject(c->location->map, obj);
            }
            else screenMessage("Board What?\n");
        }
        else screenMessage("Board What?\n");
        break;

    case 'c':
        screenMessage("Cast Spell!\nPlayer: ");
        gameGetPlayerForCommand(&gameCastForPlayer);
        break;

    case 'd':        
        if (!usePortalAt(c->location, c->location->x, c->location->y, c->location->z, ACTION_DESCEND)) {
            if (c->transportContext == TRANSPORT_BALLOON) {
                screenMessage("Land Balloon\n");
                if (c->saveGame->balloonstate == 0)
                    screenMessage("Already Landed!\n");
                else if (tileCanLandBalloon(mapGroundTileAt(c->location->map, c->location->x, c->location->y, c->location->z))) {
                    c->saveGame->balloonstate = 0;
                    c->opacity = 1;
                }
                else screenMessage("Not Here!\n");
            }
            else screenMessage("Descend what?\n");
        }        

        break;

    case 'e':
        if (!usePortalAt(c->location, c->location->x, c->location->y, c->location->z, ACTION_ENTER)) {
            if (!mapPortalAt(c->location->map, c->location->x, c->location->y, c->location->z))
                screenMessage("Enter what?\n");
        }
        break;

    case 'f':
        if (c->transportContext == TRANSPORT_SHIP) {
            int broadsidesDirs = dirGetBroadsidesDirs(tileGetDirection((unsigned char)c->saveGame->transport));            

            info = (CoordActionInfo *) malloc(sizeof(CoordActionInfo));
            info->handleAtCoord = &fireAtCoord;
            info->origin_x = c->location->x;
            info->origin_y = c->location->y;
            info->prev_x = info->prev_y = -1;
            info->range = 3;
            info->validDirections = broadsidesDirs; /* can only fire broadsides! */
            info->player = -1;
            info->blockedPredicate = NULL; /* nothing (not even mountains!) can block cannonballs */
            info->blockBefore = 1;
            info->firstValidDistance = 1;
            eventHandlerPushKeyHandlerData(&gameGetCoordinateKeyHandler, info);
            
            screenMessage("Fire Cannon!\nDir: ");
        }
        else
            screenMessage("Fire What?\n");
        break;

    case 'g':
        screenMessage("Get Chest!\n");

        if (c->saveGame->balloonstate)
            screenMessage("Drift only!\n");
        else {
            if ((obj = mapObjectAt(c->location->map, c->location->x, c->location->y, c->location->z)) != NULL)
                tile = obj->tile;
            else
                tile = mapTileAt(c->location->map, c->location->x, c->location->y, c->location->z);
    
            if (tileIsChest(tile))
            {
                screenMessage("Who opens? ");
                gameGetPlayerForCommand(&gameGetChest);
            }
            else
                screenMessage("Not here!\n");
        }
        
        break;

    case 'h':
        if (!(c->location->context && (CTX_WORLDMAP | CTX_DUNGEON))) {
            screenMessage("Hole up & Camp\nNot here!\n");
            break;
        }
        if (c->transportContext != TRANSPORT_FOOT) {
            screenMessage("Hole up & Camp\nOnly on foot!\n");
            break;
        }
        screenMessage("Hole up & Camp!\n");
        campBegin();
        break;

    case 'i':
        screenMessage("Ignite torch!\nNot Here!\n");
        break;

    case 'j':
        info = (CoordActionInfo *) malloc(sizeof(CoordActionInfo));
        info->handleAtCoord = &jimmyAtCoord;
        info->origin_x = c->location->x;
        info->origin_y = c->location->y;
        info->prev_x = info->prev_y = -1;
        info->range = 1;
        info->validDirections = MASK_DIR_ALL;
        info->player = -1;
        info->blockedPredicate = NULL;
        info->blockBefore = 0;
        info->firstValidDistance = 1;
        eventHandlerPushKeyHandlerData(&gameGetCoordinateKeyHandler, info);
        screenMessage("Jimmy\nDir: ");
        break;

    case 'k':        
        if (!usePortalAt(c->location, c->location->x, c->location->y, c->location->z, ACTION_KLIMB)) {
            if (c->transportContext == TRANSPORT_BALLOON) {
                c->saveGame->balloonstate = 1;
                c->opacity = 0;
                screenMessage("Klimb altitude\n");            
            } else
                screenMessage("Klimb what?\n");
        }
        break;

    case 'l':
        if (c->saveGame->sextants >= 1)
            screenMessage("Locate position\nwith sextant\n Latitude: %c'%c\"\nLongitude: %c'%c\"\n",
                          c->location->y / 16 + 'A', c->location->y % 16 + 'A',
                          c->location->x / 16 + 'A', c->location->x % 16 + 'A');
        else
            screenMessage("Locate position\nwith what?\n");
        break;

    case 'm':
        screenMessage("Mix reagents\n");
        alphaInfo = (AlphaActionInfo *) malloc(sizeof(AlphaActionInfo));
        alphaInfo->lastValidLetter = 'z';
        alphaInfo->handleAlpha = mixReagentsForSpell;
        alphaInfo->prompt = "For Spell: ";
        alphaInfo->data = NULL;

        screenMessage("%s", alphaInfo->prompt);
        eventHandlerPushKeyHandlerData(&gameGetAlphaChoiceKeyHandler, alphaInfo);

        c->statsItem = STATS_MIXTURES;
        statsUpdate();
        break;

    case 'n':
        eventHandlerPushKeyHandlerData(&gameGetPlayerNoKeyHandler, (void *) &newOrderForPlayer);
        screenMessage("New Order!\nExchange # ");
        break;

    case 'o':
        if (c->saveGame->balloonstate)
            screenMessage("Open; Not Here!\n");
        else {
            info = (CoordActionInfo *) malloc(sizeof(CoordActionInfo));
            info->handleAtCoord = &openAtCoord;
            info->origin_x = c->location->x;
            info->origin_y = c->location->y;
            info->prev_x = info->prev_y = -1;
            info->range = 1;
            info->validDirections = MASK_DIR_ALL;
            info->player = -1;
            info->blockedPredicate = NULL;
            info->blockBefore = 0;
            info->firstValidDistance = 1;
            eventHandlerPushKeyHandlerData(&gameGetCoordinateKeyHandler, info);
            screenMessage("Open\nDir: ");
        }
        break;

    case 'p':
        if (c->saveGame->gems) {
            c->saveGame->gems--;
            
            gamePeerGem();            
            screenMessage("Peer at a Gem!\n");
        } else
            screenMessage("Peer at What?\n");
        break;

    case 'q':        
        screenMessage("Quit & Save...\n%d moves\n", c->saveGame->moves);
        if (mapIsWorldMap(c->location->map)) {
            gameSave();
            screenMessage("Press Alt-x to quit\n", c->saveGame->moves);
        }
        else screenMessage("Not here!\n");
        
        break;

    case 'r':
        screenMessage("Ready a weapon\nfor: ");
        gameGetPlayerForCommand(&readyForPlayer);
        break;

    case 's':
        screenMessage("Searching...\n");

        if (c->saveGame->balloonstate)
            screenMessage("Drift only!\n");
        else {
            item = itemAtLocation(c->location->map, c->location->x, c->location->y, c->location->z);
            if (item) {
                if (*item->isItemInInventory != NULL && (*item->isItemInInventory)(item->data))
                    screenMessage("Nothing Here!\n");
                else {                
                    if (item->name)
                        screenMessage("You find...\n%s!\n", item->name);
                    (*item->putItemInInventory)(item->data);
                }
            } else
                screenMessage("Nothing Here!\n");  
        }

        break;

    case 't':
        if (c->saveGame->balloonstate)
            screenMessage("Talk\nDrift only!\n");
        else {
            info = (CoordActionInfo *) malloc(sizeof(CoordActionInfo));
            info->handleAtCoord = &talkAtCoord;
            info->origin_x = c->location->x;
            info->origin_y = c->location->y;
            info->prev_x = info->prev_y = -1;
            info->range = 2;
            info->validDirections = MASK_DIR_ALL;
            info->player = -1;
            info->blockedPredicate = &tileCanTalkOver;
            info->blockBefore = 0;
            info->firstValidDistance = 1;
            eventHandlerPushKeyHandlerData(&gameGetCoordinateKeyHandler, info);
            screenMessage("Talk\nDir: ");
        }
        break;

    case 'u':
        screenMessage("Use which item:\n");
        readBufferInfo = (ReadBufferActionInfo *) malloc(sizeof(ReadBufferActionInfo));
        readBufferInfo->handleBuffer = &useItem;
        readBufferInfo->buffer = itemNameBuffer;
        readBufferInfo->bufferLen = sizeof(itemNameBuffer);
        readBufferInfo->screenX = TEXT_AREA_X + c->col;
        readBufferInfo->screenY = TEXT_AREA_Y + c->line;
        itemNameBuffer[0] = '\0';
        eventHandlerPushKeyHandlerData(&keyHandlerReadBuffer, readBufferInfo);

        if (settings->minorEnhancements) {
            /* a little xu4 enhancement: show items in inventory when prompted for an item to use */
            c->statsItem = STATS_ITEMS;
            statsUpdate();
        }
        break;

    case 'v':
        if (musicToggle())
            screenMessage("Volume On!\n");
        else
            screenMessage("Volume Off!\n");
        break;

    case 'w':
        screenMessage("Wear Armour\nfor: ");
        gameGetPlayerForCommand(&wearForPlayer);
        break;

    case 'x':
        if ((c->transportContext != TRANSPORT_FOOT) && c->saveGame->balloonstate == 0) {
            Object *obj = mapAddObject(c->location->map, (unsigned char)c->saveGame->transport, (unsigned char)c->saveGame->transport, c->location->x, c->location->y, c->location->z);
            if (c->transportContext == TRANSPORT_SHIP)
                c->lastShip = obj;

            gameSetTransport(AVATAR_TILE);
            c->horseSpeed = 0;
            screenMessage("X-it\n");
        } else
            screenMessage("X-it What?\n");
        break;

    case 'y':
        screenMessage("Yell ");
        if (c->transportContext == TRANSPORT_HORSE) {
            if (c->horseSpeed == 0) {
                screenMessage("Giddyup!\n");
                c->horseSpeed = 1;
            } else {
                screenMessage("Whoa!\n");
                c->horseSpeed = 0;
            }
        } else
            screenMessage("what?\n");
        break;

    case 'z':        
        screenMessage("Ztats for: ");
        gameGetPlayerForCommand(&ztatsFor);
        break;

    case 'v' + U4_ALT:
        screenMessage("XU4 %s\n", VERSION);        
        break;

    case 'x' + U4_ALT:
        eventHandlerSetExitFlag(1);
        valid = 0;
        break;

    default:
        valid = 0;        
        break;
    }

    if (valid) {
        if (eventHandlerGetKeyHandler() == &gameBaseKeyHandler)
            (*c->location->finishTurn)();
    }

    return valid || keyHandlerDefault(key, NULL);
}

void gameGetPlayerForCommand(int (*commandFn)(int player)) {
    if (c->saveGame->members <= 1) {
        screenMessage("1\n");
        (*commandFn)(0);
    } else
        eventHandlerPushKeyHandlerData(&gameGetPlayerNoKeyHandler, (void *) commandFn);
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
        (*c->location->finishTurn)();
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

    if (islower(key))
        key = toupper(key);

    if (key >= 'A' && key <= toupper(info->lastValidLetter)) {
        screenMessage("%c\n", key);
        eventHandlerPopKeyHandler();
        (*(info->handleAlpha))(key - 'A', info->data);
        free(info);
    } else if (key == U4_SPACE || key == U4_ESC || key == U4_ENTER) {
        screenMessage("\n");
        eventHandlerPopKeyHandler();
        free(info);
        (*c->location->finishTurn)();
    } else {
        valid = 0;
        screenMessage("\n%s", info->prompt);
        screenRedrawScreen();
    }

    return valid || keyHandlerDefault(key, NULL);
}

int gameGetDirectionKeyHandler(int key, void *data) {
    int (*handleDirection)(Direction dir) = (int(*)(Direction))data;    
    Direction dir = keyToDirection(key);    
    int valid = (dir != DIR_NONE) ? 1 : 0;
    
    switch(key) {
    case U4_ESC:
    case U4_SPACE:
    case U4_ENTER:
        eventHandlerPopKeyHandler();

        screenMessage("\n");
        (*c->location->finishTurn)();        

    default:
        if (valid) {
            eventHandlerPopKeyHandler();

            screenMessage("%s\n", getDirectionName(dir));
            (*handleDirection)(dir);
        }       
        break;
    }   

    return valid || keyHandlerDefault(key, NULL);
}

int gameGetFieldTypeKeyHandler(int key, void *data) {
    int (*handleFieldType)(int field) = (int(*)(int))data;    
    int fieldTile = 0;

    switch(tolower(key)) {
    case 'f': fieldTile = FIREFIELD_TILE; break;
    case 'l': fieldTile = LIGHTNINGFIELD_TILE; break;
    case 'p': fieldTile = POISONFIELD_TILE; break;
    case 's': fieldTile = SLEEPFIELD_TILE; break;
    default: break;
    }

    if (fieldTile > 0) {
        (*handleFieldType)(fieldTile);
        return 1;
    }

    return 0;    
}

int gameGetPhaseKeyHandler(int key, void *data) {    
    int (*handlePhase)(int) = (int(*)(int))data;
    int valid = 1;

    eventHandlerPopKeyHandler();

    if (key >= '1' && key <= '8') {
        screenMessage("%c\n", key);
        (*handlePhase)(key - '1');
    } else {
        screenMessage("None\n");
        (*c->location->finishTurn)();
        valid = 0;
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
    Direction dir = keyToDirection(key);
    int valid = (dir != DIR_NONE);
    info->dir = MASK_DIR(dir);

    switch(key) {
    case U4_ESC:
    case U4_SPACE:
    case U4_ENTER:
        eventHandlerPopKeyHandler();

        screenMessage("\n");
        free(info);
        (*c->location->finishTurn)();        

    default:
        if (valid) {
            eventHandlerPopKeyHandler();
            screenMessage("%s\n", getDirectionName(dir));
            gameDirectionalAction(info);
            free(info);
        }        
        break;
    }    

    return valid || keyHandlerDefault(key, NULL);
}

/**
 * Handles key presses while Ztats are being displayed.
 */
int gameZtatsKeyHandler(int key, void *data) {
    switch (key) {
    case U4_UP:
    case U4_LEFT:
        statsPrevItem();
        break;
    case U4_DOWN:
    case U4_RIGHT:
        statsNextItem();
        break;
    default:
        eventHandlerPopKeyHandler();
        (*c->location->finishTurn)();
        break;
    }

    statsUpdate();

    return 1;
}

int gameSpecialCmdKeyHandler(int key, void *data) {
    int i;
    ReadBufferActionInfo *readBufferInfo;
    const Moongate *moongate;
    int valid = 1;

    switch (key) {
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
        screenMessage("Gate %d!\n", key - '0');

        if (mapIsWorldMap(c->location->map)) {
            moongate = moongateGetGateForPhase(key - '1');
            c->location->x = moongate->x;
            c->location->y = moongate->y;            
        }
        else screenMessage("Not here!\n");
        screenPrompt();
        break;

    case 'a':
        {
            int newTrammelphase = c->saveGame->trammelphase + 1;
            if (newTrammelphase > 7)
                newTrammelphase = 0;

            screenMessage("Advance Moons!\n");
            while (c->saveGame->trammelphase != newTrammelphase)
                gameUpdateMoons(0);

            screenPrompt();
        }
        break;

    case 'c':
        collisionOverride = !collisionOverride;
        screenMessage("Collision detection %s!\n", collisionOverride ? "off" : "on");
        screenPrompt();
        break;

    case 'e':
        {
            extern int numWeapons;            

            screenMessage("Equipment!\n");
            screenPrompt();
            for (i = ARMR_NONE + 1; i < ARMR_MAX; i++)
                c->saveGame->armor[i] = 8;
            for (i = WEAP_HANDS + 1; i < numWeapons; i++) {
                if (weaponLoseWhenUsed(i) || weaponLoseWhenRanged(i))
                    c->saveGame->weapons[i] = 99;
                else c->saveGame->weapons[i] = 8;
            }
        }
        break;

    case 'h':
        screenMessage("Help:\n"
                      "1-8 - gate\n"
                      "a - Adv. Moons\n"
                      "c - Collision\n"
                      "e - Equipment\n"
                      "h - Help\n"
                      "i - Items\n"
                      "k - Show Karma\n"
                      "l - Location\n"
                      "m - Mixtures\n"                      
                      "o - Opacity\n"                      
                      "(more)");
        eventHandlerPopKeyHandler();
        eventHandlerPushKeyHandler(&cmdHandleAnyKey);
        return 1;

    case 'i':
        screenMessage("Items!\n");
        screenPrompt();
        c->saveGame->torches = 99;
        c->saveGame->gems = 99;
        c->saveGame->keys = 99;
        c->saveGame->sextants = 1;
        c->saveGame->items = ITEM_SKULL | ITEM_CANDLE | ITEM_BOOK | ITEM_BELL | ITEM_KEY_C | ITEM_KEY_L | ITEM_KEY_T | ITEM_HORN | ITEM_WHEEL;
        c->saveGame->stones = 0xff;
        c->saveGame->runes = 0xff;
        c->saveGame->food = 999900;
        c->saveGame->gold = 9999;
        statsUpdate();
        break;

    case 'k':        
        screenMessage("Karma!\n\n");
        for (i = 0; i < 8; i++) {
            unsigned int j;
            screenMessage("%s:", getVirtueName(i));
            for (j = 13; j > strlen(getVirtueName(i)); j--)
                screenMessage(" ");
            if (c->saveGame->karma[i] > 0)                
                screenMessage("%.2d\n", c->saveGame->karma[i]);            
            else screenMessage("--");
        }
        screenPrompt();

        break;

    case 'l':
        if (mapIsWorldMap(c->location->map))
            screenMessage("\nLocation:\n%s\nx: %d\ny: %d\n", "World Map", c->location->x, c->location->y);
        else
            screenMessage("\nLocation:\n%s\nx: %d\ny: %d\nz: %d\n", c->location->map->fname, c->location->x, c->location->y, c->location->z);
        screenPrompt();
        break;

    case 'm':
        screenMessage("Mixtures!\n");
        screenPrompt();
        for (i = 0; i < SPELL_MAX; i++)
            c->saveGame->mixtures[i] = 99;
        break;    

    case 'o':
        c->opacity = !c->opacity;
        screenMessage("Opacity %s!\n", c->opacity ? "on" : "off");
        screenPrompt();
        break;

    case 'p':        
        screenMessage("\nPeer at a Gem!\n");
        eventHandlerPopKeyHandler();
        gamePeerGem();
        return 1;

    case 'r':
        screenMessage("Reagents!\n");
        screenPrompt();
        for (i = 0; i < REAG_MAX; i++)
            c->saveGame->reagents[i] = 99;
        break;

    case 's':
        screenMessage("Summon!\n");
        eventHandlerPopKeyHandler();

        readBufferInfo = (ReadBufferActionInfo *) malloc(sizeof(ReadBufferActionInfo));
        readBufferInfo->handleBuffer = &gameSummonMonster;
        readBufferInfo->buffer = monsterNameBuffer;
        readBufferInfo->bufferLen = sizeof(monsterNameBuffer);
        readBufferInfo->screenX = TEXT_AREA_X + c->col;
        readBufferInfo->screenY = TEXT_AREA_Y + c->line;
        monsterNameBuffer[0] = '\0';
        eventHandlerPushKeyHandlerData(&keyHandlerReadBuffer, readBufferInfo);
        screenMessage("What?\n");
        return 1;

    case 't':
        if (mapIsWorldMap(c->location->map)) {
            mapAddObject(c->location->map, tileGetHorseBase(), tileGetHorseBase(), 84, 106, -1);
            mapAddObject(c->location->map, tileGetShipBase(), tileGetShipBase(), 88, 109, -1);
            mapAddObject(c->location->map, tileGetBalloonBase(), tileGetBalloonBase(), 85, 105, -1);
            screenMessage("Transports: Ship, Horse and Balloon created!\n");
            screenPrompt();
        }
        break;

    case 'w':        
        screenMessage("Wind Dir ('l' to lock):\n");
        eventHandlerPopKeyHandler();
        eventHandlerPushKeyHandler(&windCmdKeyHandler);
        return 1;

    case 'x':
        screenMessage("\nX-it!\n");        
        if (!gameExitToParentMap(c))
            screenMessage("Not Here!\n");
        musicPlay();
        screenPrompt();
        break;

    case '\033':
    case '\015':
    case ' ':
        screenMessage("Nothing\n");
        screenPrompt();
        break;

    default:
        valid = 0;
        break;
    }

    if (valid)
        eventHandlerPopKeyHandler();

    return valid || keyHandlerDefault(key, NULL);
}

int cmdHandleAnyKey(int key, void *data) {
    eventHandlerPopKeyHandler();

    screenMessage("\n"
                  "p - Peer\n"
                  "r - Reagents\n"
                  "s - Summon\n"
                  "t - Transports\n"
                  "w - Change Wind\n"
                  "x - Exit Map\n");
    screenPrompt();
    return 1;
}

int windCmdKeyHandler(int key, void *data) {
    switch (key) {
    case U4_UP:
    case U4_LEFT:
    case U4_DOWN:
    case U4_RIGHT:
        c->windDirection = keyToDirection(key);
        screenMessage("Wind %s!\n", getDirectionName(c->windDirection));
        break;

    case 'l':
        windLock = !windLock;
        screenMessage("Wind direction is %slocked!\n", windLock ? "" : "un");
        break;
    }

    eventHandlerPopKeyHandler();
    statsUpdate();
    screenPrompt();

    return 1;
}

/**
 * Attempts to attack a creature at map coordinates x,y.  If no
 * creature is present at that point, zero is returned.
 */
int attackAtCoord(int x, int y, int distance, void *data) {
    Object *obj, *under;
    unsigned char ground;
    Object *temp;
    const Monster *m;

    /* attack failed: finish up */
    if (x == -1 && y == -1) {        
        screenMessage("Nothing to Attack!\n");
        (*c->location->finishTurn)();
        return 0;
    }

    obj = mapObjectAt(c->location->map, x, y, c->location->z);
    /* nothing attackable: move on to next tile */
    if (obj == NULL ||
        (obj->objType == OBJECT_UNKNOWN) ||
        (obj->objType == OBJECT_MONSTER && !monsterIsAttackable(obj->monster)) ||
        (obj->objType == OBJECT_PERSON && !monsterForTile(obj->tile)) || 
        /* can't attack horse transport */
        (tileIsHorse(obj->tile) && obj->movement_behavior == MOVEMENT_FIXED)) {
        return 0;
    }

    /* attack successful */
    ground = mapGroundTileAt(c->location->map, c->location->x, c->location->y, c->location->z);
    if ((under = mapObjectAt(c->location->map, c->location->x, c->location->y, c->location->z)) &&
        tileIsShip(under->tile))
        ground = under->tile;

    /* You're attacking a townsperson!  Alert the guards! */
    if ((obj->objType != OBJECT_MONSTER) && (obj->movement_behavior != MOVEMENT_ATTACK_AVATAR)) {
        
        /* switch all the guards to attack mode */
        for (temp = c->location->map->objects; temp; temp = temp->next) {            
            m = monsterForTile(temp->tile);
            if (m && (m->id == GUARD_ID || m->id == LORDBRITISH_ID))
                temp->movement_behavior = MOVEMENT_ATTACK_AVATAR;
        }       
    }

    /* not good karma to be killing the innocent.  Bad avatar! */
    m = monsterForTile(obj->tile);
    if (monsterIsGood(m) || /* attacking a good monster */
        /* attacking a docile (although possibly evil) person in town */
        ((obj->objType != OBJECT_MONSTER) && (obj->movement_behavior != MOVEMENT_ATTACK_AVATAR))) 
        playerAdjustKarma(c->saveGame, KA_ATTACKED_GOOD);

    combatBegin(combatMapForTile(ground, (unsigned char)c->saveGame->transport, obj), obj, 1);
    return 1;
}

int castPlayer;
unsigned int castSpell;

int gameCastForPlayer(int player) {
    AlphaActionInfo *info;

    castPlayer = player;

    if (gameCheckPlayerDisabled(player)) {
        (*c->location->finishTurn)();
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

    /* If we can't really cast this spell, skip the extra parameters */
    if ((spellGetRequiredMP(spell) > c->saveGame->players[castPlayer].mp) || /* not enough mp */
        ((spellGetContext(spell) & c->location->context) == 0) ||            /* wrong context */
        (c->saveGame->mixtures[spell] == 0) ||                               /* none mixed! */
        ((spellGetTransportContext(spell) & c->transportContext) == 0)) {    /* invalid transportation for spell */
        
        gameCastSpell(castSpell, castPlayer, 0);
        (*c->location->finishTurn)();
        return 1;
    }

    /* Get the final parameters for the spell */
    switch (spellGetParamType(spell)) {
    case SPELLPRM_NONE:
        gameCastSpell(castSpell, castPlayer, 0);
        (*c->location->finishTurn)();
        break;
    case SPELLPRM_PHASE:
        screenMessage("To Phase: ");
        eventHandlerPushKeyHandlerData(&gameGetPhaseKeyHandler, (void *) &castForPlayerGetPhase);        
        break;
    case SPELLPRM_PLAYER:
        screenMessage("Who: ");
        gameGetPlayerForCommand(&castForPlayerGetDestPlayer);        
        break;
    case SPELLPRM_DIR:
    case SPELLPRM_TYPEDIR:
        screenMessage("Dir: ");
        eventHandlerPushKeyHandlerData(&gameGetDirectionKeyHandler, (void *) &castForPlayerGetDestDir);
        break;
    case SPELLPRM_FROMDIR:
        screenMessage("From Dir: ");
        eventHandlerPushKeyHandlerData(&gameGetDirectionKeyHandler, (void *) &castForPlayerGetDestDir);
        break;
    }    

    return 1;
}

int castForPlayerGetDestPlayer(int player) {
    gameCastSpell(castSpell, castPlayer, player);
    (*c->location->finishTurn)();
    return 1;
}

int castForPlayerGetDestDir(Direction dir) {
    gameCastSpell(castSpell, castPlayer, (int) dir);
    (*c->location->finishTurn)();
    return 1;
}

int castForPlayerGetPhase(int phase) {
    gameCastSpell(castSpell, castPlayer, phase);
    (*c->location->finishTurn)();
    return 1;
}

int destroyAtCoord(int x, int y, int distance, void *data) {
    Object *obj = mapObjectAt(c->location->map, x, y, c->location->z);

    screenPrompt();

    if (obj) {
        mapRemoveObject(c->location->map, obj);
        return 1;
    }
    return 0;
}

int fireAtCoord(int x, int y, int distance, void *data) {
    
    CoordActionInfo* info = (CoordActionInfo*)data;
    int oldx = info->prev_x,
        oldy = info->prev_y;  
    int attackdelay = MAX_BATTLE_SPEED - settings->battleSpeed;
    int validObject = 0;
    int hitsAvatar = 0;
    int originAvatar = (info->origin_x == c->location->x && info->origin_y == c->location->y);
    
    info->prev_x = x;
    info->prev_y = y;

    /* Remove the last weapon annotation left behind */
    if ((distance > 0) && (oldx >= 0) && (oldy >= 0))
        annotationRemove(oldx, oldy, c->location->z, c->location->map->id, MISSFLASH_TILE);
    
    if (x == -1 && y == -1) {
        if (distance == 0)
            screenMessage("Broadsides Only!\n");

        /* The avatar's ship was firing */
        if (originAvatar)
            (*c->location->finishTurn)();

        return 1;
    }
    else {
        Object *obj = NULL;

        obj = mapObjectAt(c->location->map, x, y, c->location->z);
                
        /* FIXME: there's got to be a better way make whirlpools and storms impervious to cannon fire */
        if (obj && (obj->objType == OBJECT_MONSTER) &&
                (obj->monster->id != WHIRLPOOL_ID) && (obj->monster->id != STORM_ID))
            validObject = 1;
        /* See if it's an object to be destroyed (the avatar cannot destroy the balloon) */
        else if (obj && (obj->objType == OBJECT_UNKNOWN) && !(tileIsBalloon(obj->tile) && originAvatar))
            validObject = 1;
        
        /* Does the cannon hit the avatar? */
        if (x == c->location->x && y == c->location->y) {
            validObject = 1;
            hitsAvatar = 1;
        }        

        if (validObject)
        {
            /* always displays as a 'hit' though the object may not be destroyed */                        
            
            /* Is is a pirate ship firing at US? */
            if (hitsAvatar) {
                attackFlash(x, y, HITFLASH_TILE, 2);

                if (c->transportContext == TRANSPORT_SHIP)
                    gameDamageShip(-1, 10);
                else gameDamageParty(10, 25); /* party gets hurt between 10-25 damage */
            }          
            /* inanimate objects get destroyed instantly, while monsters get a chance */
            else if (obj->objType == OBJECT_UNKNOWN) {
                attackFlash(x, y, HITFLASH_TILE, 2);
                mapRemoveObject(c->location->map, obj);
            }
            
            /* only the avatar can hurt other monsters with cannon fire */
            else if (originAvatar) {
                attackFlash(x, y, HITFLASH_TILE, 2);
                if (rand() % 2 == 0)
                    mapRemoveObject(c->location->map, obj);
            }
            
            if (originAvatar)
                (*c->location->finishTurn)();

            return 1;
        }
        
        annotationSetVisual(annotationAddTemporary(x, y, c->location->z, c->location->map->id, MISSFLASH_TILE));
        gameUpdateScreen();

        /* Based on attack speed setting in setting struct, make a delay for
           the attack annotation */
        if (attackdelay > 0)
            eventHandlerSleep(attackdelay * 4);
    }

    return 0;
}

/**
 * Get the chest at the current x,y of the current context for player 'player'
 */

int gameGetChest(int player) {
    Object *obj;
    unsigned char tile;
    
    if ((player >= 0) && playerIsDisabled(c->saveGame, player)) {
        screenMessage("Disabled!\n");
        (*c->location->finishTurn)();
        return 0;
    }

    if ((obj = mapObjectAt(c->location->map, c->location->x, c->location->y, c->location->z)) != NULL)
        tile = obj->tile;
    else
        tile = mapTileAt(c->location->map, c->location->x, c->location->y, c->location->z);
    
    if (tileIsChest(tile)) {
        if (obj)
            mapRemoveObject(c->location->map, obj);
        else
            annotationAdd(c->location->x, c->location->y, c->location->z, c->location->map->id, BRICKFLOOR_TILE);        
        
        getChestTrapHandler(player);
        screenMessage("The Chest Holds: %d Gold\n", playerGetChest(c->saveGame));

        statsUpdate();
        
        if (obj == NULL)
            playerAdjustKarma(c->saveGame, KA_STOLE_CHEST);
    }    
    else
        screenMessage("Not Here!\n");        

    (*c->location->finishTurn)();    
    return 1;
}

/**
 * Called by gameGetChest() to handle possible traps on chests
 **/

int getChestTrapHandler(int player) {            
    TileEffect trapType;
    int dex = c->saveGame->players[player].dex;
    int randNum = rand() & 3;
    int member = player;
    
    /* Do we use u4dos's way of trap-determination, or the original intended way? */
    int passTest = (settings->minorEnhancements && settings->minorEnhancementsOptions.c64chestTraps) ?
        ((rand() % 2) == 0) : /* xu4-enhanced */
        ((randNum & 1) == 0); /* u4dos original way (only allows even numbers through, so only acid and poison show) */

    /* Chest is trapped! 50/50 chance */
    if (passTest)
    {   
        /* Figure out which trap the chest has */
        switch(randNum & rand()) {
        case 0: trapType = EFFECT_FIRE; break;   /* acid trap (56% chance - 9/16) */
        case 1: trapType = EFFECT_SLEEP; break;  /* sleep trap (19% chance - 3/16) */
        case 2: trapType = EFFECT_POISON; break; /* poison trap (19% chance - 3/16) */
        case 3: trapType = EFFECT_LAVA; break;   /* bomb trap (6% chance - 1/16) */
        default: trapType = EFFECT_FIRE; break;
        }

        /* apply the effects from the trap */
        if (trapType == EFFECT_FIRE)
            screenMessage("Acid Trap!\n");            
        else if (trapType == EFFECT_POISON)
            screenMessage("Poison Trap!\n");            
        else if (trapType == EFFECT_SLEEP)
            screenMessage("Sleep Trap!\n");            
        else if (trapType == EFFECT_LAVA) {
            screenMessage("Bomb Trap!\n");
            member = ALL_PLAYERS;            
        }

        /* See if the trap was evaded! */           
        if ((dex + 25) < ((rand() & 0xFF) % 100) && /* test player's dex */            
            (player >= 0)) {                        /* player is < 0 during the 'O'pen spell (immune to traps) */                         
            playerApplyEffect(c->saveGame, trapType, member);
            statsUpdate();
        }
        else screenMessage("Evaded!\n");

        return 1;
    }

    return 0;
}

/**
 * Attempts to jimmy a locked door at map coordinates x,y.  The locked
 * door is replaced by a permanent annotation of an unlocked door
 * tile.
 */
int jimmyAtCoord(int x, int y, int distance, void *data) {    

    if (x == -1 && y == -1) {
        screenMessage("Jimmy what?\n");
        (*c->location->finishTurn)();
        return 0;
    }

    if (!tileIsLockedDoor(mapTileAt(c->location->map, x, y, c->location->z)))
        return 0;
        
    if (c->saveGame->keys) {
        c->saveGame->keys--;
        annotationAdd(x, y, c->location->z, c->location->map->id, 0x3b);
        screenMessage("\nUnlocked!\n");
    } else
        screenMessage("No keys left!\n");

    (*c->location->finishTurn)();

    return 1;
}

/**
 * Readies a weapon for the given player.  Prompts the use for a
 * weapon.
 */
int readyForPlayer(int player) {
    AlphaActionInfo *info;
    extern int numWeapons;

    c->statsItem = STATS_WEAPONS;
    statsUpdate();

    info = (AlphaActionInfo *) malloc(sizeof(AlphaActionInfo));
    //info->lastValidLetter = WEAP_MAX + 'a' - 1;
    info->lastValidLetter = numWeapons + 'a' - 1;
    info->handleAlpha = readyForPlayer2;
    info->prompt = "Weapon: ";
    info->data = (void *) player;

    screenMessage("%s", info->prompt);

    eventHandlerPushKeyHandlerData(&gameGetAlphaChoiceKeyHandler, info);

    return 1;
}

int readyForPlayer2(int w, void *data) {
    int player = (int) data;
    WeaponType weapon = (WeaponType) w, oldWeapon;
    char *weaponName = weaponGetName(weapon);    

    // Return view to party overview
    c->statsItem = STATS_PARTY_OVERVIEW;
    statsUpdate();

    if (weapon != WEAP_HANDS && c->saveGame->weapons[weapon] < 1) {
        screenMessage("None left!\n");
        (*c->location->finishTurn)();
        return 0;
    }

    if (!weaponCanReady(weapon, c->saveGame->players[player].klass)) {
        const char *indef_article;

        switch(tolower(weaponName[0])) {
        case 'a':
        case 'e':
        case 'i':
        case 'o':
        case 'u':
        case 'y':
            indef_article = "an"; break;
        default: indef_article = "a"; break;
        }

        screenMessage("\nA %s may NOT use %s\n%s\n",
            getClassName(c->saveGame->players[player].klass),
            indef_article,
            weaponName);
        (*c->location->finishTurn)();
        return 0;
    }

    oldWeapon = c->saveGame->players[player].weapon;
    if (oldWeapon != WEAP_HANDS)
        c->saveGame->weapons[oldWeapon]++;
    if (weapon != WEAP_HANDS)
        c->saveGame->weapons[weapon]--;
    c->saveGame->players[player].weapon = weapon;

    screenMessage("%s\n", weaponName);

    (*c->location->finishTurn)();

    return 1;
}

/* FIXME */
Mixture *mix;
int mixSpell;

/**
 * Mixes reagents for a spell.  Prompts for reagents.
 */
int mixReagentsForSpell(int spell, void *data) {
    GetChoiceActionInfo *info;

    mixSpell = spell;
    mix = mixtureNew();

    screenMessage("%s\nReagent: ", spellGetName(spell));

    c->statsItem = STATS_REAGENTS;
    statsUpdate();

    info = (GetChoiceActionInfo *) malloc(sizeof(GetChoiceActionInfo));
    info->choices = "abcdefgh\n\r \033";
    info->handleChoice = &mixReagentsForSpell2;
    eventHandlerPushKeyHandlerData(&keyHandlerGetChoice, info);

    return 0;
}

int mixReagentsForSpell2(int choice) {
    GetChoiceActionInfo *info;
    AlphaActionInfo *alphaInfo;

    eventHandlerPopKeyHandler();

    if (choice == '\n' || choice == '\r' || choice == ' ') {
        screenMessage("\n\nYou mix the Reagents, and...\n");

        if (spellMix(mixSpell, mix))
            screenMessage("Success!\n\n");
        else
            screenMessage("It Fizzles!\n\n");

        mixtureDelete(mix);

        screenMessage("Mix reagents\n");
        alphaInfo = (AlphaActionInfo *) malloc(sizeof(AlphaActionInfo));
        alphaInfo->lastValidLetter = 'z';
        alphaInfo->handleAlpha = mixReagentsForSpell;
        alphaInfo->prompt = "For Spell: ";
        alphaInfo->data = NULL;

        screenMessage("%s", alphaInfo->prompt);
        eventHandlerPushKeyHandlerData(&gameGetAlphaChoiceKeyHandler, alphaInfo);

        c->statsItem = STATS_MIXTURES;
        statsUpdate();

        return 1;
    }

    else if (choice == '\033') {

        mixtureRevert(mix);
        mixtureDelete(mix);

        screenMessage("\n\n");
        (*c->location->finishTurn)();
        return 1;
    }

    else {
        screenMessage("%c\n", toupper(choice));

        if (mixtureAddReagent(mix, choice - 'a'))
            statsUpdate();
        else
            screenMessage("None Left!\n");

        screenMessage("Reagent: ");

        info = (GetChoiceActionInfo *) malloc(sizeof(GetChoiceActionInfo));
        info->choices = "abcdefgh\n\r \033";
        info->handleChoice = &mixReagentsForSpell2;
        eventHandlerPushKeyHandlerData(&keyHandlerGetChoice, info);


        return 1;
    }
}

/* FIXME: must be a better way.. */
int newOrderTemp;

/**
 * Exchanges the position of two players in the party.  Prompts the
 * use for the second player number.
 */
int newOrderForPlayer(int player) {
    if (player == 0) {
        screenMessage("%s, You must lead!\n", c->saveGame->players[0].name);
        (*c->location->finishTurn)();
        return 0;
    }

    screenMessage("    with # ");

    newOrderTemp = player;
    eventHandlerPushKeyHandlerData(&gameGetPlayerNoKeyHandler, (void *) &newOrderForPlayer2);

    return 1;
}

int newOrderForPlayer2(int player2) {
    int player1 = newOrderTemp;
    SaveGamePlayerRecord tmp;

    if (player2 == 0) {
        screenMessage("%s, You must lead!\n", c->saveGame->players[0].name);
        (*c->location->finishTurn)();
        return 0;
    } else if (player1 == player2) {
        screenMessage("What?\n");
        (*c->location->finishTurn)();
        return 0;
    }

    tmp = c->saveGame->players[player1];
    c->saveGame->players[player1] = c->saveGame->players[player2];
    c->saveGame->players[player2] = tmp;

    statsUpdate();

    return 1;
}

/**
 * Attempts to open a door at map coordinates x,y.  The door is
 * replaced by a temporary annotation of a floor tile for 4 turns.
 */
int openAtCoord(int x, int y, int distance, void *data) {
    if (x == -1 && y == -1) {
        screenMessage("Not Here!\n");
        (*c->location->finishTurn)();
        return 0;
    }

    if (!tileIsDoor(mapTileAt(c->location->map, x, y, c->location->z)) &&
        !tileIsLockedDoor(mapTileAt(c->location->map, x, y, c->location->z)))
        return 0;

    if (tileIsLockedDoor(mapTileAt(c->location->map, x, y, c->location->z))) {
        screenMessage("Can't!\n");
        (*c->location->finishTurn)();
        return 1;
    }

    annotationSetTurnDuration(annotationAdd(x, y, c->location->z, c->location->map->id, BRICKFLOOR_TILE), 4);

    screenMessage("\nOpened!\n");
    (*c->location->finishTurn)();

    return 1;
}

/**
 * Waits for space bar to return from gem mode.
 */
int gemHandleChoice(int choice) {
    eventHandlerPopKeyHandler();

    screenEnableCursor();    
    
    c->location->viewMode = VIEW_NORMAL;
    (*c->location->finishTurn)();    

    /* unpause the game */
    paused = 0;

    return 1;
}

/**
 * Peers at a city from A-P (Lycaeum telescope) and functions like a gem
 */
int gamePeerCity(int city, void *data) {
    GetChoiceActionInfo *choiceInfo;    
    Map *peerMap;

    peerMap = mapMgrGetById((unsigned char)(city+1));

    if (peerMap)
    {
        gameSetMap(c, peerMap, 1, NULL);
        c->location->viewMode = VIEW_GEM;

        screenDisableCursor();
            
        // Wait for player to hit a key
        choiceInfo = (GetChoiceActionInfo *) malloc(sizeof(GetChoiceActionInfo));
        choiceInfo->choices = "\015 \033";
        choiceInfo->handleChoice = &peerCityHandleChoice;
        eventHandlerPushKeyHandlerData(&keyHandlerGetChoice, choiceInfo);
        return 1;
    }
    return 0;
}

/**
 * Peers at a gem
 */
void gamePeerGem(void) {
    GetChoiceActionInfo *choiceInfo;

    paused = 1;
    pausedTimer = 0;
    screenDisableCursor();
    
    c->location->viewMode = VIEW_GEM;
    choiceInfo = (GetChoiceActionInfo *) malloc(sizeof(GetChoiceActionInfo));
    choiceInfo->choices = "\015 \033";
    choiceInfo->handleChoice = &gemHandleChoice;
    eventHandlerPushKeyHandlerData(&keyHandlerGetChoice, choiceInfo);
}

/**
 * Wait for space bar to return from gem mode and returns map to normal
 */
int peerCityHandleChoice(int choice) {
    eventHandlerPopKeyHandler();
    gameExitToParentMap(c);
    screenEnableCursor();    
    
    (*c->location->finishTurn)();

    return 1;
}

/**
 * Begins a conversation with the NPC at map coordinates x,y.  If no
 * NPC is present at that point, zero is returned.
 */
int talkAtCoord(int x, int y, int distance, void *data) {
    const Person *talker;
    extern int personIsVendor(const Person *person);

    if (x == -1 && y == -1) {
        screenMessage("Funny, no\nresponse!\n");
        (*c->location->finishTurn)();        
        return 0;
    }

    c->conversation.talker = mapPersonAt(c->location->map, x, y, c->location->z);

    /* some persons in some towns exists as a 'person' object, but they
       really are not someone you can talk to.  These persons have mostly null fields */
    if (c->conversation.talker == NULL || 
        ((c->conversation.talker->name == NULL) && (c->conversation.talker->npcType <= NPC_TALKER_COMPANION)))
        return 0;

    talker = c->conversation.talker;
    c->conversation.state = CONV_INTRO;
    c->conversation.reply = personGetConversationText(&c->conversation, "");
    c->conversation.replyLine = 0;
    c->conversation.playerInquiryBuffer[0] = '\0';

    talkShowReply(0);

    return 1;
}

/**
 * Handles a query while talking to an NPC.
 */
int talkHandleBuffer(const char *message) {
    eventHandlerPopKeyHandler();

    c->conversation.reply = personGetConversationText(&c->conversation, message);
    c->conversation.replyLine = 0;
    c->conversation.playerInquiryBuffer[0] = '\0';

    talkShowReply(1);

    return 1;
}

int talkHandleChoice(int choice) {
    char message[2];

    eventHandlerPopKeyHandler();

    message[0] = choice;
    message[1] = '\0';

    c->conversation.reply = personGetConversationText(&c->conversation, message);
    c->conversation.replyLine = 0;
    c->conversation.playerInquiryBuffer[0] = '\0';

    talkShowReply(1);

    return 1;
}

int talkHandleAnyKey(int key, void *data) {
    int showPrompt = (int) data;

    eventHandlerPopKeyHandler();

    talkShowReply(showPrompt);

    return 1;
}

/**
 * Shows the conversation reply and sets up a key handler to handle
 * the current conversation state.
 */
void talkShowReply(int showPrompt) {
    char *prompt;
    ReadBufferActionInfo *rbInfo;
    GetChoiceActionInfo *gcInfo;
    int bufferlen;

    screenMessage("%s", c->conversation.reply->chunk[c->conversation.replyLine]);
    c->conversation.replyLine++;

    /* if all chunks haven't been shown, wait for a key and process next chunk*/
    if (c->conversation.replyLine < c->conversation.reply->nchunks) {
        eventHandlerPushKeyHandlerData(&talkHandleAnyKey, (void *) showPrompt);
        return;
    }

    /* otherwise, free current reply and proceed based on conversation state */
    replyDelete(c->conversation.reply);
    c->conversation.reply = NULL;

    if (c->conversation.state == CONV_DONE) {
        (*c->location->finishTurn)();
        return;
    }
    
    /* When Lord British heals the party */
    else if (c->conversation.state == CONV_FULLHEAL) {
        int i;
        
        for (i = 0; i < c->saveGame->members; i++) {
            playerHeal(c->saveGame, HT_CURE, i);        // cure the party
            playerHeal(c->saveGame, HT_FULLHEAL, i);    // heal the party
        }        
        (*spellEffectCallback)('r', -1, 0); // same spell effect as 'r'esurrect

        statsUpdate();
        c->conversation.state = CONV_TALK;
    }
    /* When Lord British checks and advances each party member's level */
    else if (c->conversation.state == CONV_ADVANCELEVELS) {
        gameLordBritishCheckLevels();
        c->conversation.state = CONV_TALK;
    }

    if (showPrompt &&
        (prompt = personGetPrompt(&c->conversation)) != NULL) {
        screenMessage("%s", prompt);
        free(prompt);
    }

    switch (personGetInputRequired(&c->conversation, &bufferlen)) {
    case CONVINPUT_STRING:
        rbInfo = (ReadBufferActionInfo *) malloc(sizeof(ReadBufferActionInfo));
        rbInfo->buffer = c->conversation.playerInquiryBuffer;
        rbInfo->bufferLen = bufferlen;
        rbInfo->handleBuffer = &talkHandleBuffer;
        rbInfo->screenX = TEXT_AREA_X + c->col;
        rbInfo->screenY = TEXT_AREA_Y + c->line;
        eventHandlerPushKeyHandlerData(&keyHandlerReadBuffer, rbInfo);
        break;

    case CONVINPUT_CHARACTER:
        gcInfo = (GetChoiceActionInfo *) malloc(sizeof(GetChoiceActionInfo));
        gcInfo->choices = personGetChoices(&c->conversation);
        gcInfo->handleChoice = &talkHandleChoice;
        eventHandlerPushKeyHandlerData(&keyHandlerGetChoice, gcInfo);
        break;

    case CONVINPUT_NONE:
        /* no handler: conversation done! */
        break;
    }
}

int useItem(const char *itemName) {
    eventHandlerPopKeyHandler();

    itemUse(itemName);

    (*c->location->finishTurn)();

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

int wearForPlayer2(int a, void *data) {
    int player = (int) data;
    ArmorType armor = (ArmorType) a, oldArmor;

    if (armor != ARMR_NONE && c->saveGame->armor[armor] < 1) {
        screenMessage("None left!\n");
        (*c->location->finishTurn)();
        return 0;
    }

    if (!armorCanWear(armor, c->saveGame->players[player].klass)) {
        screenMessage("\nA %s may NOT use\n%s\n", getClassName(c->saveGame->players[player].klass), armorGetName(armor));
        (*c->location->finishTurn)();
        return 0;
    }

    oldArmor = c->saveGame->players[player].armor;
    if (oldArmor != ARMR_NONE)
        c->saveGame->armor[oldArmor]++;
    if (armor != ARMR_NONE)
        c->saveGame->armor[armor]--;
    c->saveGame->players[player].armor = armor;

    screenMessage("%s\n", armorGetName(armor));

    (*c->location->finishTurn)();

    return 1;
}

/**
 * Called when the player selects a party member for ztats
 */
int ztatsFor(int player) {
    c->statsItem = (StatsItem) (STATS_CHAR1 + player);
    statsUpdate();

    eventHandlerPushKeyHandler(&gameZtatsKeyHandler);    
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
    int slowed = 0;
    unsigned char temp;
    SlowedType slowedType = SLOWED_BY_TILE;
    Object *destObj;
    
    if (c->transportContext == TRANSPORT_SHIP)
        slowedType = SLOWED_BY_WIND;
    else if (c->transportContext == TRANSPORT_BALLOON)
        slowedType = SLOWED_BY_NOTHING;

    /*musicPlayEffect();*/

    /* if you're on ship, you must turn first! */
    if (c->transportContext == TRANSPORT_SHIP) {
        if (tileGetDirection((unsigned char)c->saveGame->transport) != dir) {
	        temp = (unsigned char)c->saveGame->transport;
            tileSetDirection(&temp, dir);
	        c->saveGame->transport = temp;            
            if (!settings->filterMoveMessages)
                screenMessage("Turn %s!\n", getDirectionName(dir));
            return result;
        }
    }
    
    /* if you're in a dungeon, you must turn first! */
    else if (c->location->context == CTX_DUNGEON) {
        /* figure out what our real direction is */
        Direction realDir = dir,
                  temp = c->saveGame->orientation;

        while (temp != DIR_NORTH) {
            temp = dirRotateCW(temp);
            realDir = dirRotateCCW(realDir);
        }       
        
        if (c->saveGame->orientation != realDir) {
            c->saveGame->orientation = realDir;
            if (!settings->filterMoveMessages)
                screenMessage("Turn %s!\n", getDirectionName(realDir));
            return result;
        }

        dir = realDir;
    }

    if (c->transportContext == TRANSPORT_HORSE) {
        if ((dir == DIR_WEST || dir == DIR_EAST) &&
            tileGetDirection((unsigned char)c->saveGame->transport) != dir) {
	    temp = (unsigned char)c->saveGame->transport;
            tileSetDirection(&temp, dir);
	    c->saveGame->transport = temp;
        }
    }

    newx = c->location->x;
    newy = c->location->y;

    dirMove(dir, &newx, &newy);

    if (!settings->filterMoveMessages && userEvent) {
        if (c->transportContext == TRANSPORT_SHIP)
            screenMessage("Sail %s!\n", getDirectionName(dir));
        else if (c->transportContext != TRANSPORT_BALLOON)
            screenMessage("%s\n", getDirectionName(dir));
    }

    if (MAP_IS_OOB(c->location->map, newx, newy)) {
        switch (c->location->map->border_behavior) {        
        case BORDER_WRAP:
            mapWrapCoordinates(c->location->map, &newx, &newy);
            break;

        case BORDER_EXIT2PARENT:
            screenMessage("Leaving...\n");
            gameExitToParentMap(c);
            musicPlay();
            return (result = 0);

        case BORDER_FIXED:
            if (newx < 0 || newx >= (int) c->location->map->width)
                newx = c->location->x;
            if (newy < 0 || newy >= (int) c->location->map->height)
                newy = c->location->y;
            break;
        }
    }

    if (!collisionOverride && !c->saveGame->balloonstate) {
        int movementMask;

        movementMask = mapGetValidMoves(c->location->map, c->location->x, c->location->y, c->location->z, (unsigned char)c->saveGame->transport);
        if (!DIR_IN_MASK(dir, movementMask)) {

            if (settings->shortcutCommands) {
                if (tileIsDoor(mapTileAt(c->location->map, newx, newy, c->location->z))) {
                    openAtCoord(newx, newy, 1, NULL);
                    return result;
                } else if (tileIsLockedDoor(mapTileAt(c->location->map, newx, newy, c->location->z))) {
                    jimmyAtCoord(newx, newy, 1, NULL);
                    return result;
                } /*else if (mapPersonAt(c->location->map, newx, newy, c->location->z) != NULL) {
                    talkAtCoord(newx, newy, 1, NULL);
                    return result;
                    }*/
            }

            screenMessage("Blocked!\n");            
            return (result = 0);
        }

        /* Are we slowed by terrain or by wind direction? */
        switch(slowedType) {
        case SLOWED_BY_TILE:
            slowed = slowedByTile(mapTileAt(c->location->map, newx, newy, c->location->z));
            break;
        case SLOWED_BY_WIND:
            slowed = slowedByWind(dir);
            break;
        case SLOWED_BY_NOTHING:
        default:
            break;
        }
        
        if (slowed) {
            if (!settings->filterMoveMessages)
                screenMessage("Slow progress!\n");            
            return (result = 0);
        }
    }    

    c->location->x = newx;
    c->location->y = newy;

    /* if the avatar moved onto a monster (whirlpool, twister), then do the monster's special effect */
    destObj = mapObjectAt(c->location->map, newx, newy, c->location->z);    
    if (destObj && destObj->objType == OBJECT_MONSTER)
        monsterSpecialEffect(destObj);

    /* things that happen while not on board the balloon */    
    if (c->transportContext & ~TRANSPORT_BALLOON)
        gameCheckSpecialMonsters(dir);
    /* things that happen while on foot or horseback */
    if (c->transportContext & TRANSPORT_FOOT_OR_HORSE) {
        if (gameCheckMoongates())
            return (result = 0);
    }

    return result;
}

/**
 * This function is called every quarter second.
 */
void gameTimer(void *data) {

    if (pausedTimer > 0) {
        pausedTimer--;
        if (pausedTimer <= 0) {
            pausedTimer = 0;
            paused = 0; /* unpause the game */
        }
    }
    
    if (!paused && !pausedTimer) {
        Direction dir = DIR_WEST;  

        if (++c->windCounter >= MOON_SECONDS_PER_PHASE * 4) {
            if ((rand() % 4) == 1 && !windLock)
                c->windDirection = dirRandomDir(MASK_DIR_ALL);
            c->windCounter = 0;        
        }

        /* balloon moves about 4 times per second */
        if ((c->transportContext == TRANSPORT_BALLOON) &&
            c->saveGame->balloonstate) {
            dir = dirReverse((Direction) c->windDirection);
            moveAvatar(dir, 0);
        }

        gameUpdateMoons(1);

        mapAnimateObjects(c->location->map);

        screenCycle();

        /*
         * refresh the screen only if the timer queue is empty --
         * i.e. drop a frame if another timer event is about to be fired
         */
        if (eventHandlerTimerQueueEmpty())
            gameUpdateScreen();

        /*
         * force pass if no commands within last 20 seconds
         */
        if ((eventHandlerGetKeyHandler() == &gameBaseKeyHandler ||
             eventHandlerGetKeyHandler() == &combatBaseKeyHandler) &&
             gameTimeSinceLastCommand() > 20) {
         
            /* pass the turn, and redraw the text area so the prompt is shown */
            (*eventHandlerGetKeyHandler())(' ', NULL);
            screenRedrawTextArea(TEXT_AREA_X, TEXT_AREA_Y, TEXT_AREA_W, TEXT_AREA_H);
        }
    }

}

/**
 * Updates the phases of the moons and shows
 * the visual moongates on the map, if desired
 */
void gameUpdateMoons(int showmoongates)
{
    int realMoonPhase,
        oldTrammel,
        trammelSubphase;        
    const Moongate *gate;

    if (mapIsWorldMap(c->location->map) && c->location->viewMode == VIEW_NORMAL) {        
        oldTrammel = c->saveGame->trammelphase;

        if (++c->moonPhase >= MOON_PHASES * MOON_SECONDS_PER_PHASE * 4)
            c->moonPhase = 0;
        
        trammelSubphase = c->moonPhase % (MOON_SECONDS_PER_PHASE * 4 * 3);
        realMoonPhase = (c->moonPhase / (4 * MOON_SECONDS_PER_PHASE));

        c->saveGame->trammelphase = realMoonPhase / 3;
        c->saveGame->feluccaphase = realMoonPhase % 8;

        if (c->saveGame->trammelphase > 7)
            c->saveGame->trammelphase = 7;        
        
        if (showmoongates)
        {
            /* update the moongates if trammel changed */
            if (trammelSubphase == 0) {
                gate = moongateGetGateForPhase(oldTrammel);
                annotationRemove(gate->x, gate->y, c->location->z, c->location->map->id, MOONGATE0_TILE);
                gate = moongateGetGateForPhase(c->saveGame->trammelphase);
                annotationSetVisual(annotationAdd(gate->x, gate->y, c->location->z, c->location->map->id, MOONGATE0_TILE));
            }
            else if (trammelSubphase == 1) {
                gate = moongateGetGateForPhase(c->saveGame->trammelphase);
                annotationRemove(gate->x, gate->y, c->location->z, c->location->map->id, MOONGATE0_TILE);
                annotationSetVisual(annotationAdd(gate->x, gate->y, c->location->z, c->location->map->id, MOONGATE1_TILE));
            }
            else if (trammelSubphase == 2) {
                gate = moongateGetGateForPhase(c->saveGame->trammelphase);
                annotationRemove(gate->x, gate->y, c->location->z, c->location->map->id, MOONGATE1_TILE);
                annotationSetVisual(annotationAdd(gate->x, gate->y, c->location->z, c->location->map->id, MOONGATE2_TILE));
            }
            else if (trammelSubphase == 3) {
                gate = moongateGetGateForPhase(c->saveGame->trammelphase);
                annotationRemove(gate->x, gate->y, c->location->z, c->location->map->id, MOONGATE2_TILE);
                annotationSetVisual(annotationAdd(gate->x, gate->y, c->location->z, c->location->map->id, MOONGATE3_TILE));
            }
            else if ((trammelSubphase > 3) && (trammelSubphase < (MOON_SECONDS_PER_PHASE * 4 * 3) - 3)) {
                gate = moongateGetGateForPhase(c->saveGame->trammelphase);
                annotationRemove(gate->x, gate->y, c->location->z, c->location->map->id, MOONGATE3_TILE);
                annotationSetVisual(annotationAdd(gate->x, gate->y, c->location->z, c->location->map->id, MOONGATE3_TILE));
            }
            else if (trammelSubphase == (MOON_SECONDS_PER_PHASE * 4 * 3) - 3) {
                gate = moongateGetGateForPhase(c->saveGame->trammelphase);
                annotationRemove(gate->x, gate->y, c->location->z, c->location->map->id, MOONGATE3_TILE);
                annotationSetVisual(annotationAdd(gate->x, gate->y, c->location->z, c->location->map->id, MOONGATE2_TILE));
            }
            else if (trammelSubphase == (MOON_SECONDS_PER_PHASE * 4 * 3) - 2) {
                gate = moongateGetGateForPhase(c->saveGame->trammelphase);
                annotationRemove(gate->x, gate->y, c->location->z, c->location->map->id, MOONGATE2_TILE);
                annotationSetVisual(annotationAdd(gate->x, gate->y, c->location->z, c->location->map->id, MOONGATE1_TILE));
            }
            else if (trammelSubphase == (MOON_SECONDS_PER_PHASE * 4 * 3) - 1) {
                gate = moongateGetGateForPhase(c->saveGame->trammelphase);
                annotationRemove(gate->x, gate->y, c->location->z, c->location->map->id, MOONGATE1_TILE);
                annotationSetVisual(annotationAdd(gate->x, gate->y, c->location->z, c->location->map->id, MOONGATE0_TILE));
            }
        }
    }
}

/**
 * Initializes the moon state according to the savegame file. This method of
 * initializing the moons (rather than just setting them directly) is necessary
 * to make sure trammel and felucca stay in sync
 */
void gameInitMoons()
{
    int trammelphase = c->saveGame->trammelphase,
        feluccaphase = c->saveGame->feluccaphase;        

    ASSERT(c != NULL, "Game context doesn't exist!");
    ASSERT(c->saveGame != NULL, "Savegame doesn't exist!");
    ASSERT(mapIsWorldMap(c->location->map) && c->location->viewMode == VIEW_NORMAL, "Can only call gameInitMoons() from the world map!");

    c->saveGame->trammelphase = c->saveGame->feluccaphase = 0;
    c->moonPhase = 0;

    while ((c->saveGame->trammelphase != trammelphase) ||
           (c->saveGame->feluccaphase != feluccaphase))
        gameUpdateMoons(0);    
}

/**
 * Handles trolls under bridges
 */
void gameCheckBridgeTrolls() {
    if (!mapIsWorldMap(c->location->map) ||
        mapTileAt(c->location->map, c->location->x, c->location->y, c->location->z) != BRIDGE_TILE ||
        (rand() % 8) != 0)
        return;

    screenMessage("\nBridge Trolls!\n");

    combatBegin(MAP_BRIDGE_CON, mapAddMonsterObject(c->location->map, monsterById(TROLL_ID), c->location->x, c->location->y, c->location->z), 1);
}

/**
 * Checks the hull integrity of the ship and handles
 * the ship sinking, if necessary
 */
void gameCheckHullIntegrity() {
    int i;

    /* see if the ship has sunk */
    if ((c->transportContext == TRANSPORT_SHIP) && c->saveGame->shiphull <= 0)
    {
        screenMessage("\nThy ship sinks!\n\n");        

        for (i = 0; i < c->saveGame->members; i++)
        {
            c->saveGame->players[i].hp = 0;
            c->saveGame->players[i].status = 'D';
        }
        statsUpdate();   

        screenRedrawScreen();        
        deathStart(5);
    }
}

/**
 * Checks for valid conditions and handles
 * special monsters guarding the entrance to the
 * abyss and to the shrine of spirituality
 */
void gameCheckSpecialMonsters(Direction dir) {
    int i;
    Object *obj;    
    static const struct {
        int x, y;
        Direction dir;
    } pirateInfo[] = {
        { 224, 220, DIR_EAST }, /* N'M" O'A" */
        { 224, 228, DIR_EAST }, /* O'E" O'A" */
        { 226, 220, DIR_EAST }, /* O'E" O'C" */
        { 227, 228, DIR_EAST }, /* O'E" O'D" */
        { 228, 227, DIR_SOUTH }, /* O'D" O'E" */
        { 229, 225, DIR_SOUTH }, /* O'B" O'F" */
        { 229, 223, DIR_NORTH }, /* N'P" O'F" */
        { 228, 222, DIR_NORTH } /* N'O" O'E" */
    };

    /*
     * if heading east into pirates cove (O'A" N'N"), generate pirate
     * ships
     */
    if (dir == DIR_EAST &&
        c->location->x == 0xdd &&
        c->location->y == 0xe0) {
        for (i = 0; i < 8; i++) {        
            obj = mapAddMonsterObject(c->location->map, monsterById(PIRATE_ID), pirateInfo[i].x, pirateInfo[i].y, c->location->z);
            tileSetDirection(&obj->tile, pirateInfo[i].dir);            
        }
    }

    /*
     * if heading south towards the shrine of humility, generate
     * daemons unless horn has been blown
     */
    if (dir == DIR_SOUTH &&
        c->location->x >= 229 &&
        c->location->x < 234 &&
        c->location->y >= 212 &&
        c->location->y < 217 &&
        c->aura != AURA_HORN) {
        for (i = 0; i < 8; i++)            
            obj = mapAddMonsterObject(c->location->map, monsterById(DAEMON_ID), 231, c->location->y + 1, c->location->z);                    
    }
}

/**
 * Checks for and handles when the avatar steps on a moongate
 */
int gameCheckMoongates(void) {
    int destx, desty;
    
    if (moongateFindActiveGateAt(c->saveGame->trammelphase, c->saveGame->feluccaphase,
                                 c->location->x, c->location->y, &destx, &desty)) {

        (*spellEffectCallback)(-1, -1, 0); // Default spell effect (screen inversion without 'spell' sound effects)
        
        if ((c->location->x != destx) && 
            (c->location->y != desty)) {
            
            c->location->x = destx;    
            c->location->y = desty;
            (*spellEffectCallback)(-1, -1, 0); // Again, after arriving
        }

        if (moongateIsEntryToShrineOfSpirituality(c->saveGame->trammelphase, c->saveGame->feluccaphase)) {
            Map *shrine_spirituality_map;

            shrine_spirituality_map = mapMgrGetById(MAP_SHRINE_SPIRITUALITY);

            if (!playerCanEnterShrine(c->saveGame, shrine_spirituality_map->shrine->virtue))
                return 1;
            
            gameSetMap(c, shrine_spirituality_map, 1, NULL);
            musicPlay();

            shrineEnter(shrine_spirituality_map->shrine);

            annotationClear(c->location->map->id);  /* clear out world map annotations */            
        }

        return 1;
    }

    return 0;
}

/**
 * Checks monster conditions and spawns new monsters if necessary
 */
void gameCheckRandomMonsters() {
    /* remove monsters that are too far away from the avatar */
    gameMonsterCleanup();
    
    /* If there are too many monsters already,
       or we're not on the world map, don't worry about it! */
    if (!mapIsWorldMap(c->location->map) ||
        mapNumberOfMonsters(c->location->map) >= MAX_MONSTERS_ON_MAP ||        
        (rand() % 32) != 0)
        return;    
    
    gameSpawnMonster(NULL);
}

/**
 * Fixes objects initially loaded by saveGameMonstersRead,
 * and alters movement behavior accordingly to match the monster
 */
void gameFixupMonsters() {
    Object *obj;

    for (obj = c->location->map->objects; obj; obj = obj->next) {
        /* translate unknown objects into monster objects if necessary */
        if (obj->objType == OBJECT_UNKNOWN && monsterForTile(obj->tile) != NULL &&
            obj->movement_behavior != MOVEMENT_FIXED) {
            obj->objType = OBJECT_MONSTER;
            obj->monster = monsterForTile(obj->tile);
        }

        /* fix monster behaviors */
        if (obj->movement_behavior == MOVEMENT_ATTACK_AVATAR) {
            const Monster *m = monsterForTile(obj->tile);
            if (m && monsterWanders(m))
                obj->movement_behavior = MOVEMENT_WANDER;
            else if (m && monsterIsStationary(m))
                obj->movement_behavior = MOVEMENT_FIXED;
        }
    }
}

long gameTimeSinceLastCommand() {
    return time(NULL) - c->lastCommandTime;
}

/**
 * Handles what happens when a monster attacks you
 */
void gameMonsterAttack(Object *obj) {
    Object *under;
    unsigned char ground;    
    
    screenMessage("\nAttacked by %s\n", monsterForTile(obj->tile)->name);

    ground = mapGroundTileAt(c->location->map, c->location->x, c->location->y, c->location->z);
    if ((under = mapObjectAt(c->location->map, c->location->x, c->location->y, c->location->z)) &&
        tileIsShip(under->tile))
        ground = under->tile;
    combatBegin(combatMapForTile(ground, (unsigned char)c->saveGame->transport, obj), obj, 1);
}

/**
 * Performs a ranged attack for the monster at x,y on the world map
 */
int monsterRangeAttack(int x, int y, int distance, void *data) {
    CoordActionInfo* info = (CoordActionInfo*)data;
    int oldx = info->prev_x,
        oldy = info->prev_y;  
    int attackdelay = MAX_BATTLE_SPEED - settings->battleSpeed;   
    Object *obj;
    const Monster *m;
    unsigned char tile;

    info->prev_x = x;
    info->prev_y = y;

    /* Find the monster that made the range attack */
    obj = mapObjectAt(c->location->map, info->origin_x, info->origin_y, c->location->z);
    m = (obj && obj->objType == OBJECT_MONSTER) ? obj->monster : NULL;

    /* Figure out what the ranged attack should look like */
    tile = (m && (m->worldrangedtile > 0)) ? m->worldrangedtile : HITFLASH_TILE;

    /* Remove the last weapon annotation left behind */
    if ((distance > 0) && (oldx >= 0) && (oldy >= 0))
        annotationRemove(oldx, oldy, c->location->z, c->location->map->id, tile);
    
    /* Attack missed, stop now */
    if (x == -1 && y == -1) {
        return 1;
    }
    
    /* See if the attack hits the avatar */
    else {
        Object *obj = NULL;

        obj = mapObjectAt(c->location->map, x, y, c->location->z);        
        
        /* Does the attack hit the avatar? */
        if (x == c->location->x && y == c->location->y) {
            /* always displays as a 'hit' */
            attackFlash(x, y, tile, 2);

            /* FIXME: check actual damage from u4dos -- values here are guessed */
            if (c->transportContext == TRANSPORT_SHIP)
                gameDamageShip(-1, 10);
            else gameDamageParty(10, 25);

            return 1;
        }
        /* Destroy objects that were hit */
        else if (obj) {
            if (((obj->objType == OBJECT_MONSTER) &&
                (obj->monster->id != WHIRLPOOL_ID) && (obj->monster->id != STORM_ID)) ||
                obj->objType == OBJECT_UNKNOWN) {
                
                attackFlash(x, y, tile, 2);
                mapRemoveObject(c->location->map, obj);

                return 1;
            }            
        }
        
        /* Show the attack annotation */
        annotationSetVisual(annotationAddTemporary(x, y, c->location->z, c->location->map->id, tile));
        gameUpdateScreen();

        /* Based on attack speed setting in setting struct, make a delay for
           the attack annotation */
        if (attackdelay > 0)
            eventHandlerSleep(attackdelay * 4);
    }

    return 0;    
}

/**
 * Perform an action in the given direction, using the 'handleAtCoord'
 * function of the CoordActionInfo struct.  The 'blockedPredicate'
 * function is used to determine whether or not the action is blocked
 * by the tile it passes over.
 */
int gameDirectionalAction(CoordActionInfo *info) {
    int distance = 0,        
        t_x = info->origin_x,
        t_y = info->origin_y,
        succeeded = 0,
        dirx = DIR_NONE,
        diry = DIR_NONE;
    unsigned char tile;

    /* Figure out which direction the action is going */
    if (DIR_IN_MASK(DIR_WEST, info->dir)) dirx = DIR_WEST;
    else if (DIR_IN_MASK(DIR_EAST, info->dir)) dirx = DIR_EAST;
    if (DIR_IN_MASK(DIR_NORTH, info->dir)) diry = DIR_NORTH;
    else if (DIR_IN_MASK(DIR_SOUTH, info->dir)) diry = DIR_SOUTH;

    /*
     * try every tile in the given direction, up to the given range.
     * Stop when the command handler succeeds, the range is exceeded,
     * or the action is blocked.
     */
    
    if ((dirx <= 0 || DIR_IN_MASK(dirx, info->validDirections)) && 
        (diry <= 0 || DIR_IN_MASK(diry, info->validDirections))) {
        for (distance = 0; distance <= info->range;
             distance++, dirMove(dirx, &t_x, &t_y), dirMove(diry, &t_x, &t_y)) {
            if (distance >= info->firstValidDistance) {

                mapWrapCoordinates(c->location->map, &t_x, &t_y);
            
                /* make sure our action isn't taking us off the map */
                if (MAP_IS_OOB(c->location->map, t_x, t_y))
                    break;

                tile = mapGroundTileAt(c->location->map, t_x, t_y, c->location->z);

                /* should we see if the action is blocked before trying it? */       
                if (info->blockBefore && info->blockedPredicate &&
                    !(*(info->blockedPredicate))(tile))
                    break;

                if ((*(info->handleAtCoord))(t_x, t_y, distance, info)) {
                    succeeded = 1;
                    break;
                }                

                /* see if the action was blocked only if it did not succeed */
                if (!info->blockBefore && info->blockedPredicate &&
                    !(*(info->blockedPredicate))(tile))
                    break;
            }
        }
    }

    if (!succeeded)
        (*info->handleAtCoord)(-1, -1, distance, info);

    return 0;
}

/**
 * Deals an amount of damage between 'minDamage' and 'maxDamage'
 * to each party member, with a 50% chance for each member to 
 * avoid the damage.  If (minDamage == -1) or (minDamage >= maxDamage),
 * deals 'maxDamage' damage to each member.
 */
void gameDamageParty(int minDamage, int maxDamage) {
    int i;
    int damage;

    for (i = 0; i < c->saveGame->members; i++) {
        if (rand() % 2 == 0) {
            damage = ((minDamage >= 0) && (minDamage < maxDamage)) ?
                rand() % ((maxDamage + 1) - minDamage) + minDamage :
                maxDamage;
            playerApplyDamage(&c->saveGame->players[i], damage);
        }
    }
}

/**
 * Deals an amount of damage between 'minDamage' and 'maxDamage'
 * to the ship.  If (minDamage == -1) or (minDamage >= maxDamage),
 * deals 'maxDamage' damage to the ship.
 */
void gameDamageShip(int minDamage, int maxDamage) {
    int damage;

    if (c->transportContext == TRANSPORT_SHIP) {
        damage = ((minDamage >= 0) && (minDamage < maxDamage)) ?
            rand() % ((maxDamage + 1) - minDamage) + minDamage :
            maxDamage;

        c->saveGame->shiphull -= damage;
        if ((short)c->saveGame->shiphull < 0)
            c->saveGame->shiphull = 0;
        statsUpdate();
        gameCheckHullIntegrity();
    }
}

/**
 * Removes monsters from the current map if they are too far away from the avatar
 */
void gameMonsterCleanup(void) {
    Object *obj, *prev;
    
    for (obj = c->location->map->objects; obj != NULL; prev = obj)
    {        
        if ((obj->objType == OBJECT_MONSTER) && (obj->z == c->location->z) &&
             mapDistance(obj->x, obj->y, c->location->x, c->location->y) > MAX_MONSTER_DISTANCE) {                
            
            /* make sure our pointer doesn't get destroyed by mapRemoveObject */
            obj = obj->next;
            mapRemoveObject(c->location->map, prev);                
        }
        else obj = obj->next;        
    }
}

/**
 * Sets the transport for the avatar
 */
void gameSetTransport(unsigned char tile) {       
    
    if (tileIsHorse(tile))
        c->transportContext = TRANSPORT_HORSE;
    else if (tileIsShip(tile))
        c->transportContext = TRANSPORT_SHIP;
    else if (tileIsBalloon(tile))
        c->transportContext = TRANSPORT_BALLOON;
    else c->transportContext = TRANSPORT_FOOT;

    c->saveGame->transport = tile;
}

/**
 * Check the levels of each party member while talking to Lord British
 */
void gameLordBritishCheckLevels(void) {
    int i;
    int levelsRaised = 0;

    for (i = 0; i < c->saveGame->members; i++) {
        if (playerGetRealLevel(&c->saveGame->players[i]) <
            playerGetMaxLevel(&c->saveGame->players[i]))

            if (!levelsRaised) {
                /* give an extra space to separate these messages */
                screenMessage("\n");
                levelsRaised = 1;
            }

            playerAdvanceLevel(&c->saveGame->players[i]);
    }
 
    screenMessage("\nWhat would thou\nask of me?\n");
}

int gameSummonMonster(const char *monsterName) {
    extern Monster monsters[];
    extern unsigned int numMonsters;
    unsigned int i, j, id;

    eventHandlerPopKeyHandler();

    if (monsterName == NULL)
        return 0;
    
    /* find the monster by its id and spawn it */
    id = atoi(monsterName);
    if (id > 0) {
        for (i = 0; i < numMonsters; i++) {
            if (monsters[i].id == id) {
                screenMessage("\n%s summoned!\n", monsters[i].name);
                screenPrompt();
                gameSpawnMonster(&monsters[i]);
                return 1;
            }                
        }
    }

    /* find the monster by its name and spawn it */
    for (i = 0; i < numMonsters; i++) {        
        int match = 1;

        for (j = 0; j < strlen(monsterName); j++) {
            if (tolower(monsterName[j]) != tolower(monsters[i].name[j]))
                match = 0;           
        }

        if (match) {
            screenMessage("\n%s summoned!\n", monsterName);
            screenPrompt();
            gameSpawnMonster(&monsters[i]);
            return 1;
        }
    }

    screenMessage("\n%s not found\n", monsterName);
    screenPrompt();
    return 0;
}

void gameSpawnMonster(const Monster *m) {
    int x, y, dx, dy, t;
    const Monster *monster;

    dx = 7;
    dy = rand() % 7;

    if (rand() % 2)
        dx = -dx;
    if (rand() % 2)
        dy = -dy;
    if (rand() % 2) {
        t = dx;
        dx = dy;
        dy = t;
    }

    x = c->location->x + dx;
    y = c->location->y + dy;
    
    /* wrap the coordinates around the map if necessary */
    mapWrapCoordinates(c->location->map, &x, &y);    

    /* figure out what monster to spawn */
    if (m)
        monster = m;
    else
        monster = monsterRandomForTile(mapTileAt(c->location->map, x, y, c->location->z));

    if (monster) mapAddMonsterObject(c->location->map, monster, x, y, c->location->z);    
}

void gameDestroyAllCreatures(void) {
    int i;
    extern CombatInfo combatInfo;
    
    (*spellEffectCallback)('t', -1, 0); /* same effect as tremor */
    
    if (c->location->context == CTX_COMBAT) {
        /* destroy all monsters in combat */
        for (i = 0; i < AREA_MONSTERS; i++) {
            mapRemoveObject(c->location->map, combatInfo.monsters[i]);
            combatInfo.monsters[i] = NULL;
        }
    }    
    else {
        /* destroy all monsters on the map */
        Object *obj;
        for (obj = c->location->map->objects; obj; obj = obj->next) {
            if (obj->objType != OBJECT_UNKNOWN) {
                const Monster *m = (obj->objType == OBJECT_MONSTER) ? obj->monster : monsterForTile(obj->tile);
                /* the skull does not destroy Lord British */
                if (m && m->id != LORDBRITISH_ID)
                    mapRemoveObject(c->location->map, obj);
            }
        }
    }
}

int gameCreateBalloon(Map *map) {
    Object *obj;    

    /* see if the balloon has already been created (and not destroyed) */
    for (obj = map->objects; obj; obj = obj->next)
        if (tileIsBalloon(obj->tile))
            return 0;

    mapAddObject(map, BALLOON_TILE, BALLOON_TILE, 233, 242, -1);
    return 1;
}
