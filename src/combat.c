/*
 * $Id$
 */

#include <stdlib.h>
#include <time.h>
#include "u4.h"

#include "combat.h"

#include "annotation.h"
#include "context.h"
#include "death.h"
#include "debug.h"
#include "dungeon.h"
#include "event.h"
#include "game.h"
#include "location.h"
#include "mapmgr.h"
#include "menu.h"
#include "monster.h"
#include "movement.h"
#include "names.h"
#include "object.h"
#include "player.h"
#include "screen.h"
#include "settings.h"
#include "spell.h"
#include "stats.h"
#include "ttype.h"
#include "weapon.h"

CombatInfo combatInfo;

int combatAttackAtCoord(int x, int y, int distance, void *data);
int combatMonsterRangedAttack(int x, int y, int distance, void *data);
int combatReturnWeaponToOwner(int x, int y, int distance, void *data);
int combatIsWon(void);
int combatIsLost(void);
void combatEnd(int adjustKarma);
void combatMoveMonsters(void);
int combatFindTargetForMonster(const Object *monster, int *distance, int ranged);
int combatChooseWeaponDir(int key, void *data);
int combatChooseWeaponRange(int key, void *data);
void combatApplyMonsterTileEffects(void);
int combatDivideMonster(const Object *monster);
int combatNearestPartyMember(const Object *obj, int *dist);
int combatHideOrShowCamouflageMonster(Object *monster);

/**
 * Initializes the CombatInfo structure with combat information
 */
void combatInit(const struct _Monster *m, struct _Object *monsterObj, unsigned char mapid, unsigned char camping) {
    int i;
    const Map *map = c->location->map;
    MonsterCombatInfo *monsters = &combatInfo.monsters[0];
    PartyCombatInfo *party = &combatInfo.party[0];

    combatInfo.monsterObj = monsterObj;

    combatInfo.placeMonsters = 1;
    combatInfo.placeParty = 1;
    combatInfo.camping = 0;
    combatInfo.winOrLose = 1;
    combatInfo.dungeonRoom = 0;    

    /* new map for combat */
    if (mapid > 0) {
        map = combatInfo.newCombatMap = mapMgrGetById(mapid);
        ASSERT(combatInfo.newCombatMap != NULL, "bad map id: %d", mapid);        
    }
    else combatInfo.newCombatMap = NULL;
    
    /* initialize monster info */
    for (i = 0; i < AREA_MONSTERS; i++) {
        (monsters+i)->obj = NULL;
        combatInfo.monsterTable[i] = NULL;
        combatInfo.monster = NULL;
        combatInfo.monsterStartCoords[i].x = 0;
        combatInfo.monsterStartCoords[i].y = 0;
    }

    /* fill the monster table if a monster was provided to create */    
    combatFillMonsterTable(m);

    /* initialize party members */
    {
        FOCUS = 0;
        for (i = 0; i < AREA_PLAYERS; i++) {
            party[i].obj = NULL;
            party[i].player = &c->saveGame->players[i];
            combatInfo.partyStartCoords[i].x = 0;
            combatInfo.partyStartCoords[i].y = 0;
            party[i].status = STAT_GOOD;
        }
    }    

    /* party is camping */
    if (camping) {
        combatInfo.placeMonsters = 0;
        combatInfo.camping = 1;
    }

    if (map->type != MAPTYPE_DUNGEON) {
        /* setup player starting positions */
        for (i = 0; i < AREA_PLAYERS; i++) {
            combatInfo.partyStartCoords[i].x = map->area->player_start[i].x;
            combatInfo.partyStartCoords[i].y = map->area->player_start[i].y;
        }
        /* setup monster starting positions */
        for (i = 0; i < AREA_MONSTERS; i++) {
            combatInfo.monsterStartCoords[i].x = map->area->monster_start[i].x;
            combatInfo.monsterStartCoords[i].y = map->area->monster_start[i].y;
        }
    }
    else {
    }
}

/**
 * Initializes information for camping
 */
void combatInitCamping(void) {
    if (c->location->context & CTX_DUNGEON)
        combatInit(NULL, NULL, MAP_DUNGEON_CON, 1); /* FIXME: use dungeon camping map */    
    else
        combatInit(NULL, NULL, MAP_CAMP_CON, 1);   
}

/**
 * Initializes dungeon room combat
 */
void combatInitDungeonRoom(int room, Direction from) {
    int offset, i;    
    combatInit(NULL, NULL, 0, 0);    
    
    if (c->location->context & CTX_DUNGEON) {
        Dungeon *dng = c->location->map->dungeon;
        unsigned char 
            *party_x = &dng->rooms[room].party_north_start_x[0], 
            *party_y = &dng->rooms[room].party_north_start_y[0];

        /* load the dungeon room */
        dungeonLoadRoom(dng, room);
        combatInfo.newCombatMap = dng->room;
        combatInfo.winOrLose = 0;
        combatInfo.dungeonRoom = 0xD0 | room;
        combatInfo.exitDir = DIR_NONE;
        
        /* load in monsters and monster start coordinates */
        for (i = 0; i < AREA_MONSTERS; i++) {
            if (dng->rooms[room].monster_tiles[i] > 0)
                combatInfo.monsterTable[i] = monsterForTile(dng->rooms[room].monster_tiles[i]);
            combatInfo.monsterStartCoords[i].x = dng->rooms[room].monster_start_x[i];
            combatInfo.monsterStartCoords[i].y = dng->rooms[room].monster_start_y[i];
        }
        
        /* figure out party start coordinates */
        switch(from) {
        case DIR_WEST: offset = 3; break;
        case DIR_NORTH: offset = 0; break;
        case DIR_EAST: offset = 1; break;
        case DIR_SOUTH: offset = 2; break;
        default: break;
        }

        for (i = 0; i < AREA_PLAYERS; i++) {
            combatInfo.partyStartCoords[i].x = *(party_x + (offset * AREA_PLAYERS * 2) + i);
            combatInfo.partyStartCoords[i].y = *(party_y + (offset * AREA_PLAYERS * 2) + i);
        }
    }
}

/**
 * Begin combat
 */
void combatBegin() {
    int i;
    int partyIsReadyToFight = 0;
    PartyCombatInfo *party          = combatInfo.party;
    MonsterCombatInfo *monsters     = combatInfo.monsters;
    SaveGamePlayerRecord* players   = c->saveGame->players;
    
    /* set the new combat map if a new map was provided */
    if (combatInfo.newCombatMap != NULL) {
        gameSetMap(c, combatInfo.newCombatMap, 1, NULL);
    }
    
    /* place party members on the map */
    if (combatInfo.placeParty)        
        combatPlacePartyMembers();    

    /* place monsters on the map */
    if (combatInfo.placeMonsters)
        combatPlaceMonsters();

    /* camping, make sure everyone's asleep */
    if (combatInfo.camping) {
        for (i = 0; i < c->saveGame->members; i++)
            combatPutPlayerToSleep(i);        
    }    

    /* Use the combat key handler */
    eventHandlerPushKeyHandler(&combatBaseKeyHandler);

    /* if there are monsters around, start combat! */    
    if (combatInfo.placeMonsters && combatInfo.winOrLose) {
        screenMessage("\n**** COMBAT ****\n\n");        
    }

    /* FIXME: there should be a better way to accomplish this */
    if (!combatInfo.camping) {
        musicPlay();
    }

    /* Set focus to the first activate party member, if there is one */ 
    for (i = 0; i < AREA_PLAYERS; i++) {
        if (combatSetActivePlayer(i)) {
            partyIsReadyToFight = 1;
            break;
        }
    }    
}

/**
 * Sets the active player for combat, showing which weapon they're weilding, etc.
 */
int combatSetActivePlayer(int player) {
    PartyCombatInfo *party = combatInfo.party;

    if (!playerIsDisabled(c->saveGame, player) && party[player].obj) {
        if (party[FOCUS].obj)
            party[FOCUS].obj->hasFocus = 0;
        party[player].obj->hasFocus = 1;
        FOCUS = player;

        screenMessage("%s with %s\n\020", c->saveGame->players[FOCUS].name, weaponGetName(c->saveGame->players[FOCUS].weapon));
        statsUpdate(); /* If a character was awakened inbetween world view and combat, this fixes stats info */
        statsHighlightCharacter(FOCUS);
        return 1;
    }
    return 0;
}

/**
 * Puts player 'player' to sleep in combat
 */
int combatPutPlayerToSleep(int player) {
    PartyCombatInfo *party = combatInfo.party;
    
    if (!playerIsDisabled(c->saveGame, player) && party[player].obj) {
        party[player].status = party[player].player->status; /* save old status */
        party[player].player->status = STAT_SLEEPING;
        party[player].obj->tile = CORPSE_TILE;
        return 1;
    }
    return 0;
}

int combatAddMonster(const Monster *m, int x, int y, int z) {
    int i;
    MonsterCombatInfo *monsters = combatInfo.monsters;

    if (m != NULL) {
        for (i = 0; i < AREA_MONSTERS; i++) {
            /* find a free spot to place the monster */
            if (monsters[i].obj == NULL) {
                /* place the monster! */
                monsters[i].obj = mapAddMonsterObject(c->location->map, m, x, y, z);
                monsters[i].hp = monsterGetInitialHp(monsters[i].obj->monster);
                monsters[i].status = STAT_GOOD;

                return 1;
            }
        }
    }

    return 0;
}

/**
 * Fills the combat monster table with the monsters that the party will be facing.
 * The monster table only contains *which* monsters will be encountered and
 * *where* they are placed (by position in the table).  Information like
 * hit points and monster status will be created when the monster is actually placed
 */
void combatFillMonsterTable(const Monster *monster) {
    int i, j;
    
    if (monster != NULL) {        
        const Monster *baseMonster = monster, *current;
        int numMonsters = combatInitialNumberOfMonsters(monster);
        
        combatInfo.monster = monster;

        if (baseMonster->id == PIRATE_ID)
            baseMonster = monsterById(ROGUE_ID);

        for (i = 0; i < numMonsters; i++) {
            current = baseMonster;

            /* find a free spot in the monster table */
            do {j = rand() % AREA_MONSTERS;} while (combatInfo.monsterTable[j] != NULL);
            
            /* see if monster is a leader or leader's leader */
            if (monsterById(baseMonster->leader) != baseMonster && /* leader is a different monster */
                i != (numMonsters - 1)) { /* must have at least 1 monster of type encountered */
                
                if (rand() % 32 == 0)       /* leader's leader */
                    current = monsterById(monsterById(baseMonster->leader)->leader);
                else if (rand() % 8 == 0)   /* leader */
                    current = monsterById(baseMonster->leader);
            }

            /* place this monster in the monster table */
            combatInfo.monsterTable[j] = current;
        }
    }
}

/**
 * Places the party members on the map
 */
void combatPlacePartyMembers(void) {
    int i;
    for (i = 0; i < c->saveGame->members; i++) {
        PartyCombatInfo *party = combatInfo.party;
        StatusType playerStatus = party[i].player->status;
        unsigned char playerTile = tileForClass(party[i].player->klass);

        /* don't place dead party members */
        if (playerStatus != STAT_DEAD) {
            
            /* add the party member to the map */
            party[i].obj = mapAddMonsterObject(c->location->map, monsterForTile(playerTile), 
                (int)combatInfo.partyStartCoords[i].x,
                (int)combatInfo.partyStartCoords[i].y,
                c->location->z);

            /* change the tile for the object to a sleeping person if necessary */
            if (playerStatus == STAT_SLEEPING)
                party[i].obj->tile = CORPSE_TILE;            
        }
    }
}

/**
 * Places monsters on the map from the monster table and from monsterStart_x and monsterStart_y
 */
void combatPlaceMonsters(void) {
    int i;    

    for (i = 0; i < AREA_MONSTERS; i++) {
        const Monster *m = combatInfo.monsterTable[i];
        /* make sure there's a monster in this position in the monster table */
        combatAddMonster(m, 
            (int)combatInfo.monsterStartCoords[i].x,
            (int)combatInfo.monsterStartCoords[i].y,
            c->location->z);        
    }    
}

int combatPartyMemberAt(int x, int y, int z) {
    int i;
    PartyCombatInfo *party = combatInfo.party;

    for (i = 0; i < AREA_PLAYERS; i++) {
        if (party[i].obj && 
            party[i].obj->x == x &&
            party[i].obj->y == y &&
            party[i].obj->z == z)
            return i;
    }
    return -1;
}

int combatMonsterAt(int x, int y, int z) {
    int i;
    MonsterCombatInfo *monsters = combatInfo.monsters;

    for (i = 0; i < AREA_MONSTERS; i++) {
        if (monsters[i].obj &&
            monsters[i].obj->x == x &&
            monsters[i].obj->y == y &&
            monsters[i].obj->z == z)
            return i;
    }
    return -1;
}

unsigned char combatMapForTile(unsigned char groundTile, unsigned char transport, Object *obj) {
    int i;
    int fromShip = 0,
        toShip = 0;
    Object *objUnder = mapObjectAt(c->location->map, c->location->x, c->location->y, c->location->z);

    static const struct {
        unsigned char tile;
        unsigned char mapid;
    } tileToMap[] = {
        { HORSE1_TILE,  MAP_GRASS_CON },
        { HORSE2_TILE,  MAP_GRASS_CON },
        { SWAMP_TILE,   MAP_MARSH_CON },
        { GRASS_TILE,   MAP_GRASS_CON },
        { BRUSH_TILE,   MAP_BRUSH_CON },
        { FOREST_TILE,  MAP_FOREST_CON },
        { HILLS_TILE,   MAP_HILL_CON },
        { DUNGEON_TILE, MAP_HILL_CON },
        { CITY_TILE,    MAP_GRASS_CON },
        { CASTLE_TILE,  MAP_GRASS_CON },
        { TOWN_TILE,    MAP_GRASS_CON },
        { LCB2_TILE,    MAP_GRASS_CON },
        { BRIDGE_TILE,  MAP_BRIDGE_CON },
        { BALLOON_TILE, MAP_GRASS_CON },
        { NORTHBRIDGE_TILE, MAP_BRIDGE_CON },
        { SOUTHBRIDGE_TILE, MAP_BRIDGE_CON },
        { SHRINE_TILE,  MAP_GRASS_CON },
        { CHEST_TILE,   MAP_GRASS_CON },
        { BRICKFLOOR_TILE, MAP_BRICK_CON },
        { MOONGATE0_TILE, MAP_GRASS_CON },
        { MOONGATE1_TILE, MAP_GRASS_CON },
        { MOONGATE2_TILE, MAP_GRASS_CON },
        { MOONGATE3_TILE, MAP_GRASS_CON }
    };

    if (tileIsShip(transport) || (objUnder && tileIsShip(objUnder->tile)))
        fromShip = 1;
    if (tileIsPirateShip(obj->tile))
        toShip = 1;

    if (fromShip && toShip)
        return MAP_SHIPSHIP_CON;

    /* We can fight monsters and townsfolk */       
    if (obj->objType != OBJECT_UNKNOWN) {
        unsigned char tileUnderneath = (*c->location->tileAt)(c->location->map, obj->x, obj->y, obj->z);

        if (toShip)
            return MAP_SHORSHIP_CON;
        else if (fromShip && tileIsWater(tileUnderneath))
            return MAP_SHIPSEA_CON;
        else if (tileIsWater(tileUnderneath))
            return MAP_SHORE_CON;
        else if (fromShip && !tileIsWater(tileUnderneath))
            return MAP_SHIPSHOR_CON;        
    }

    for (i = 0; i < sizeof(tileToMap) / sizeof(tileToMap[0]); i++) {
        if (tileToMap[i].tile == groundTile)
            return tileToMap[i].mapid;
    }    

    return MAP_BRICK_CON;
}

void combatFinishTurn() {    
    PartyCombatInfo *party = combatInfo.party;    

    /* return to party overview */
    c->statsItem = STATS_PARTY_OVERVIEW;
    statsUpdate();

    if (combatIsWon() && combatInfo.winOrLose) {
        eventHandlerPopKeyHandler();
        combatEnd(1);
        return;
    }
    
    /* make sure the player with the focus is still in battle (hasn't fled or died) */
    if (party[FOCUS].obj) {
        /* apply effects from tile player is standing on */
        playerApplyEffect(c->saveGame, tileGetEffect((*c->location->tileAt)(c->location->map, party[FOCUS].obj->x, party[FOCUS].obj->y, c->location->z)), FOCUS);
    }

    /* check to see if the player gets to go again (and is still alive) */
    if ((c->aura != AURA_QUICKNESS) || (rand() % 2 == 0) || (c->saveGame->players[FOCUS].hp <= 0)){    

        do {            
            annotationCycle();

            /* put a sleeping person in place of the player,
               or restore an awakened member to their original state */            
            if (party[FOCUS].obj) {
                /* wake up! */
                if (party[FOCUS].player->status == STAT_SLEEPING && (rand() % 8 == 0)) {
                    party[FOCUS].player->status = party[FOCUS].status;                    
                    statsUpdate();
                }

                /* display a sleeping person or an awake person */                
                if (party[FOCUS].player->status == STAT_SLEEPING)
                    party[FOCUS].obj->tile = CORPSE_TILE;
                else party[FOCUS].obj->tile = tileForClass(party[FOCUS].player->klass);

                /* remove focus from the current party member */
                party[FOCUS].obj->hasFocus = 0;
            }

            /* eat some food */
            if (party[FOCUS].player->status != STAT_DEAD)
                playerAdjustFood(c->saveGame, -1);                

            /* put the focus on the next party member */
            FOCUS++;           

            /* move monsters and wrap around at end */
            if (FOCUS >= c->saveGame->members) {   
                
                /* reset the focus to the avatar and start the party's turn over again */
                FOCUS = 0;

                gameUpdateScreen();
                eventHandlerSleep(50); /* give a slight pause in case party members are asleep for awhile */

                /* adjust moves */
                playerEndTurn();

                /* check if aura has expired */
                if (c->auraDuration > 0) {
                    if (--c->auraDuration == 0)
                        c->aura = AURA_NONE;
                }                

                /** 
                 * ====================
                 * HANDLE MONSTER STUFF
                 * ====================
                 */
            
                /* first, move all the monsters */
                combatMoveMonsters();

                /* then, apply tile effects to monsters */
                combatApplyMonsterTileEffects();                

                /* check to see if combat is over */
                if (combatIsLost()) {
                    eventHandlerPopKeyHandler();
                    combatEnd(1);
                    return;
                }

                /* end combat immediately if the enemy has fled */
                else if (combatIsWon() && combatInfo.winOrLose) {
                    eventHandlerPopKeyHandler();
                    combatEnd(1);
                    return;
                }                
            }
        } while (!party[FOCUS].obj ||    /* dead */
                 c->saveGame->players[FOCUS].status == STAT_SLEEPING);
    }
    else annotationCycle();

    /* FIXME: there is probably a cleaner way to do this:
       make sure the active player is back to their normal self before acting */
    party[FOCUS].obj->tile = tileForClass(party[FOCUS].player->klass);
    combatSetActivePlayer(FOCUS);    
}

int combatBaseKeyHandler(int key, void *data) {
    int valid = 1;
    CoordActionInfo *info;
    AlphaActionInfo *alphaInfo;
    ReadBufferActionInfo *readBufferInfo;    
    int weapon = c->saveGame->players[FOCUS].weapon;    

    switch (key) {
    case U4_UP:
    case U4_DOWN:
    case U4_LEFT:
    case U4_RIGHT:        
        (*c->location->move)(keyToDirection(key), 1);
        break;

    case U4_ESC:
        if (settings->debug) {
            eventHandlerPopKeyHandler();
            combatEnd(0); /* don't adjust karma */
        }
        else valid=0;

        break;
        
    case ' ':
        screenMessage("Pass\n");
        break;

    case 'a':
        info = (CoordActionInfo *) malloc(sizeof(CoordActionInfo));
        info->handleAtCoord = &combatAttackAtCoord;
        info->origin_x = combatInfo.party[FOCUS].obj->x;
        info->origin_y = combatInfo.party[FOCUS].obj->y;
        info->prev_x = info->prev_y = -1;
        info->range = weaponGetRange(weapon);
        info->validDirections = MASK_DIR_ALL;
        info->player = FOCUS;        
        info->blockedPredicate = weaponCanAttackThroughObjects(weapon) ?
            NULL :
            &tileCanAttackOver;
        info->blockBefore = 1;
        info->firstValidDistance = 0;
        
        eventHandlerPushKeyHandlerData(&combatChooseWeaponDir, info);        

        screenMessage("Dir: ");        
        break;

    case 'c':
        screenMessage("Cast Spell!\n");
        gameCastForPlayer(FOCUS);
        break;

    case 'r':
        {
            extern int numWeapons;

            c->statsItem = STATS_WEAPONS;
            statsUpdate();

            alphaInfo = (AlphaActionInfo *) malloc(sizeof(AlphaActionInfo));
            alphaInfo->lastValidLetter = numWeapons + 'a' - 1;
            alphaInfo->handleAlpha = readyForPlayer2;
            alphaInfo->prompt = "Weapon: ";
            alphaInfo->data = (void *)FOCUS;

            screenMessage(alphaInfo->prompt);

            eventHandlerPushKeyHandlerData(&gameGetAlphaChoiceKeyHandler, alphaInfo);
        }
        break;

    case 'u':
        {
            extern char itemNameBuffer[16];
            screenMessage("Use which item:\n");
            readBufferInfo = (ReadBufferActionInfo *) malloc(sizeof(ReadBufferActionInfo));
            readBufferInfo->handleBuffer = &useItem;
            readBufferInfo->buffer = itemNameBuffer;
            readBufferInfo->bufferLen = sizeof(itemNameBuffer);
            readBufferInfo->screenX = TEXT_AREA_X + c->col;
            readBufferInfo->screenY = TEXT_AREA_Y + c->line;
            itemNameBuffer[0] = '\0';
            eventHandlerPushKeyHandlerData(&keyHandlerReadBuffer, readBufferInfo);

            c->statsItem = STATS_ITEMS;
            statsUpdate();

            return 1;
        }

    case 'v' + U4_ALT:
        screenMessage("XU4 %s\n", VERSION);        
        break;

    case 'z': 
        {            
            c->statsItem = (StatsItem) (STATS_CHAR1 + FOCUS);
            statsUpdate();

            /* reset the spell mix menu and un-highlight the current item,
               and hide reagents that you don't have */            
            gameResetSpellMixing();

            eventHandlerPushKeyHandler(&gameZtatsKeyHandler);
            screenMessage("Ztats\n");        
        }
        break;

    case 'g':
        gameGetChest(FOCUS);
        break;

    case 'l':
        if (settings->debug) {
            screenMessage("\nLocation:\nx:%d\ny:%d\nz:%d\n", 
                combatInfo.party[FOCUS].obj->x,
                combatInfo.party[FOCUS].obj->y,
                combatInfo.party[FOCUS].obj->z);
            screenPrompt();
            valid = 0;
            break;            
        }
    
    case 't':
        if (settings->debug && combatInfo.dungeonRoom) {
            Trigger *triggers = c->location->prev->map->dungeon->currentRoom->triggers;
            int i;

            screenMessage("Triggers!\n");

            for (i = 0; i < 4; i++) {
                screenMessage("%.1d)xy tile xy xy\n", i+1);
                screenMessage("  %.1X%.1X  %.3d %.1X%.1X %.1X%.1X\n",
                    triggers[i].x, triggers[i].y,
                    triggers[i].tile,
                    triggers[i].change_x1, triggers[i].change_y1,
                    triggers[i].change_x2, triggers[i].change_y2);                
            }
            screenPrompt();
            valid = 0;
            
            break;
        }    

    case 'b':
    case 'e':
    case 'd':
    case 'f':    
    case 'h':
    case 'i':
    case 'j':
    case 'k':
    case 'm':
    case 'n':
    case 'o':
    case 'p':
    case 'q':
    case 's':    
    case 'v':
    case 'w':
    case 'x':   
    case 'y':
        screenMessage("Not here!\n");
        break;

    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':    
        screenMessage("Bad command\n");        
        break;

    default:
        valid = 0;
        break;
    }

    if (valid) {
        c->lastCommandTime = time(NULL);
        if (eventHandlerGetKeyHandler() == &combatBaseKeyHandler)
            (*c->location->finishTurn)();
    }

    return valid;
}

int combatAttackAtCoord(int x, int y, int distance, void *data) {
    int monster;    
//    int i;
    unsigned char hittile, misstile;
    CoordActionInfo* info = (CoordActionInfo*)data;    
    int weapon = c->saveGame->players[info->player].weapon;    
    int wrongRange = weaponRangeAbsolute(weapon) && (distance != info->range);
    int oldx = info->prev_x,
        oldy = info->prev_y;  
    int attackdelay = MAX_BATTLE_SPEED - settings->battleSpeed;    
    unsigned char groundTile;
    
    info->prev_x = x;
    info->prev_y = y;

    hittile = weaponGetHitTile(weapon);
    misstile = weaponGetMissTile(weapon);

    /* Remove the last weapon annotation left behind */
    if ((distance > 0) && (oldx >= 0) && (oldy >= 0))
        annotationRemove(oldx, oldy, c->location->z, c->location->map->id, misstile);

    /* Missed */
    if (x == -1 && y == -1) {

        /* Check to see if the weapon is lost */
        if ((distance > 1 && weaponLoseWhenRanged(weapon)) || weaponLoseWhenUsed(weapon)) {
            if (!playerLoseWeapon(c->saveGame, info->player))
                screenMessage("Last One!\n");
        }

        /* Set things up so it will still display correctly */
        x = oldx;        
        y = oldy;

        /* show the 'miss' tile */
        attackFlash(x, y, misstile, 3);

        /* This goes here so messages are shown in the original order */
        screenMessage("Missed!\n");
    }
    
    /* Check to see if we might hit something */
    else {

        monster = combatMonsterAt(x, y, c->location->z);

        /* If we haven't hit a monster, or the weapon's range is absolute
           and we're testing the wrong range, stop now! */
        if (monster == -1 || wrongRange) {
        
            /* If the weapon is shown as it travels, show it now */
            if (weaponShowTravel(weapon)) {
                annotationSetVisual(annotationAdd(x, y, c->location->z, c->location->map->id, misstile));
                gameUpdateScreen();
        
                /* Based on attack speed setting in setting struct, make a delay for
                   the attack annotation */
                if (attackdelay > 0)
                    eventHandlerSleep(attackdelay * 2);
            }       

            return 0;
        }
    
        /* Check to see if the weapon is lost */
        if ((distance > 1 && weaponLoseWhenRanged(weapon)) || weaponLoseWhenUsed(weapon)) {
            if (!playerLoseWeapon(c->saveGame, info->player))
                screenMessage("Last One!\n");
        }
    
        /* Did the weapon miss? */
        if ((c->location->map->id == 24 && !weaponIsMagic(weapon)) || /* non-magical weapon in the Abyss */
            !playerAttackHit(&c->saveGame->players[FOCUS])) {         /* player naturally missed */
            screenMessage("Missed!\n");
        
            /* show the 'miss' tile */
            attackFlash(x, y, misstile, 3);

        } else { /* The weapon hit! */          

            /* show the 'hit' tile */
            attackFlash(x, y, hittile, 3);

            /* apply the damage to the monster */
            combatApplyDamageToMonster(monster, playerGetDamage(&c->saveGame->players[FOCUS]), FOCUS);

            /* monster is still alive and has the chance to divide - xu4 enhancement */
            if (rand()%2 == 0 && combatInfo.monsters[monster].obj && monsterDivides(combatInfo.monsters[monster].obj->monster))
                combatDivideMonster(combatInfo.monsters[monster].obj);
        }
    }

    /* Check to see if the weapon returns to its owner */
    if (weaponReturns(weapon))
        combatReturnWeaponToOwner(x, y, distance, data);

    /* If the weapon leaves a tile behind, do it here! (flaming oil, etc) */
    groundTile = (*c->location->tileAt)(c->location->map, x, y, c->location->z);
    if (!wrongRange && (weaponLeavesTile(weapon) && tileIsWalkable(groundTile)))
        annotationAdd(x, y, c->location->z, c->location->map->id, weaponLeavesTile(weapon));    
    
    (*c->location->finishTurn)();

    return 1;
}

int combatMonsterRangedAttack(int x, int y, int distance, void *data) {
    int player;
    const Monster *m;
//    int i;
    unsigned char hittile, misstile;
    CoordActionInfo* info = (CoordActionInfo*)data;    
    int oldx = info->prev_x,
        oldy = info->prev_y;  
    int attackdelay = MAX_BATTLE_SPEED - settings->battleSpeed;    
    unsigned char groundTile;
    TileEffect effect;
    
    info->prev_x = x;
    info->prev_y = y;

    hittile = combatInfo.monsters[info->player].obj->monster->rangedhittile;
    misstile = combatInfo.monsters[info->player].obj->monster->rangedmisstile;

    /* Remove the last weapon annotation left behind */
    if ((distance > 0) && (oldx >= 0) && (oldy >= 0))
        annotationRemove(oldx, oldy, c->location->z, c->location->map->id, misstile);

    /* Check to see if the monster hit a party member */
    if (x != -1 && y != -1) {   

        player = combatPartyMemberAt(x, y, c->location->z);        

        /* If we haven't hit a player, stop now */
        if (player == -1) {
        
            annotationSetVisual(annotationAdd(x, y, c->location->z, c->location->map->id, misstile));
            gameUpdateScreen();
    
            /* Based on attack speed setting in setting struct, make a delay for
               the attack annotation */
            if (attackdelay > 0)
                eventHandlerSleep(attackdelay * 2);

            return 0;
        }

        /* Get the effects of the tile the monster is using */
        effect = tileGetEffect(hittile);
  
        /* Did the weapon miss? */
        if (!playerIsHitByAttack(&c->saveGame->players[player])) {
        
            /* show the 'miss' tile */
            attackFlash(x, y, misstile, 4);

        } else { /* The weapon hit! */                   

            /* show the 'hit' tile */
            attackFlash(x, y, hittile, 4);             

            /* FIXME: Will this ever be used? */

            /* These effects require the player to be hit to affect the player */
            /*switch(effect) {
            } */
        }

        m = mapObjectAt(c->location->map, info->origin_x, info->origin_y, c->location->z)->monster;

        /* These effects happen whether or not the player was hit */
        switch(effect) {
        
        case EFFECT_ELECTRICITY:
            /* FIXME: are there any special effects here? */
            screenMessage("\n%s Electrified!\n", c->saveGame->players[player].name);
            playerApplyDamage(&c->saveGame->players[player], monsterGetDamage(m));
            break;
        
        case EFFECT_POISON:
        case EFFECT_POISONFIELD:
            
            screenMessage("\n%s Poisoned!\n", c->saveGame->players[player].name);

            /* see if the player is poisoned */
            if ((rand() % 2 == 0) && (c->saveGame->players[player].status != STAT_POISONED))
                c->saveGame->players[player].status = STAT_POISONED;
            else screenMessage("Failed.\n");
            break;
        
        case EFFECT_SLEEP:

            screenMessage("\n%s Slept!\n", c->saveGame->players[player].name);

            /* see if the player is put to sleep */
            if (rand() % 2 == 0)
                combatPutPlayerToSleep(player);            
            else screenMessage("Failed.\n");
            break;

        case EFFECT_LAVA:
        case EFFECT_FIRE:
            /* FIXME: are there any special effects here? */            
            screenMessage("\n%s %s Hit!\n", c->saveGame->players[player].name,
                effect == EFFECT_LAVA ? "Lava" : "Fiery");
            playerApplyDamage(&c->saveGame->players[player], monsterGetDamage(m));
            break;
                
        default: 
            /* show the appropriate 'hit' message */
            if (hittile == MAGICFLASH_TILE)
                screenMessage("\n%s Magical Hit!\n", c->saveGame->players[player].name);
            else screenMessage("\n%s Hit!\n", c->saveGame->players[player].name);
            playerApplyDamage(&c->saveGame->players[player], monsterGetDamage(m));
            break;
        }       

    }
    else {
        m = mapObjectAt(c->location->map, info->origin_x, info->origin_y, c->location->z)->monster;

        /* If the monster leaves a tile behind, do it here! (lava lizard, etc) */
        groundTile = (*c->location->tileAt)(c->location->map, oldx, oldy, c->location->z);
        if (monsterLeavesTile(m) && tileIsWalkable(groundTile))
            annotationAdd(oldx, oldy, c->location->z, c->location->map->id, hittile);
    }

    return 1;
}


int combatReturnWeaponToOwner(int x, int y, int distance, void *data) {
    int i, new_x, new_y, dir;
    unsigned char misstile;
    CoordActionInfo* info = (CoordActionInfo*)data;
    int weapon = c->saveGame->players[info->player].weapon;
    int attackdelay = MAX_BATTLE_SPEED - settings->battleSpeed;
    
    misstile = weaponGetMissTile(weapon);

    new_x = x;
    new_y = y;    

    /* reverse the direction of the weapon */
    dir = dirReverse(dirFromMask(info->dir));

    for (i = distance; i > 1; i--) {
        dirMove(dir, &new_x, &new_y);
        
        annotationSetVisual(annotationAdd(new_x, new_y, c->location->z, c->location->map->id, misstile));
        gameUpdateScreen();

        /* Based on attack speed setting in setting struct, make a delay for
           the attack annotation */
        if (attackdelay > 0)
            eventHandlerSleep(attackdelay * 2);
        
        annotationRemove(new_x, new_y, c->location->z, c->location->map->id, misstile);
    }
    gameUpdateScreen();

    return 1;
}

/**
 * Generate the number of monsters in a group.
 */
int combatInitialNumberOfMonsters(const Monster *monster) {
    int nmonsters;

    /* if in an unusual combat situation, generally we stick to normal encounter sizes,
       (such as encounters from sleeping in an inn, etc.) */
    if (combatInfo.camping || mapIsWorldMap(c->location->map)) {
        nmonsters = (rand() % 8) + 1;
        
        if (nmonsters == 1) {            
            if (monster && monster->encounterSize > 0)
                nmonsters = (rand() % monster->encounterSize) + monster->encounterSize + 1;
            else
                nmonsters = 8;
        }

        while (nmonsters > 2 * c->saveGame->members) {
            nmonsters = (rand() % 16) + 1;
        }
    } else {
        if (monster && monster->id == GUARD_ID)
            nmonsters = c->saveGame->members * 2;
        else
            nmonsters = 1;
    }

    return nmonsters;
}

/**
 * Returns true if the player has won.
 */
int combatIsWon() {
    int i, activeMonsters;

    activeMonsters = 0;
    for (i = 0; i < AREA_MONSTERS; i++) {
        if (combatInfo.monsters[i].obj)
            activeMonsters++;
    }

    return activeMonsters == 0;
}

/**
 * Returns true if the player has lost.
 */
int combatIsLost() {
    int i, activePlayers;

    activePlayers = 0;
    for (i = 0; i < c->saveGame->members; i++) {
        if (combatInfo.party[i].obj)
            activePlayers++;
    }

    return activePlayers == 0;
}

void combatEnd(int adjustKarma) {
    int i, x, y, z;
    unsigned char ground;
    
    gameExitToParentMap(c);
    musicPlay();    
    
    if (combatInfo.winOrLose) {
        if (combatIsWon()) {        

            if (combatInfo.monsterObj) {
                x = combatInfo.monsterObj->x;
                y = combatInfo.monsterObj->y;
                z = combatInfo.monsterObj->z;
                ground = (*c->location->tileAt)(c->location->map, x, y, z);

                /* add a chest, if the monster leaves one */
                if (monsterLeavesChest(combatInfo.monster) && 
                    tileIsMonsterWalkable(ground) && tileIsWalkable(ground))
                    mapAddObject(c->location->map, tileGetChestBase(), tileGetChestBase(), x, y, z);
                /* add a ship if you just defeated a pirate ship */
                else if (tileIsPirateShip(combatInfo.monsterObj->tile)) {
                    unsigned char ship = tileGetShipBase();
                    tileSetDirection(&ship, tileGetDirection(combatInfo.monsterObj->tile));
                    mapAddObject(c->location->map, ship, ship, x, y, z);
                }        
            }

            screenMessage("\nVictory!\n");
        }
        else if (!playerPartyDead(c->saveGame)) {
            /* minus points for fleeing from evil creatures */
            if (adjustKarma && combatInfo.monster && monsterIsEvil(combatInfo.monster)) {
                screenMessage("Battle is lost!\n");
                playerAdjustKarma(c->saveGame, KA_FLED_EVIL);
            }
            else if (adjustKarma && combatInfo.monster && monsterIsGood(combatInfo.monster))
                playerAdjustKarma(c->saveGame, KA_SPARED_GOOD);
        }
    }

    /* exiting a dungeon room */
    if (combatInfo.dungeonRoom) {
        screenMessage("Leave Room!\n\n");
        c->saveGame->orientation = combatInfo.exitDir;  /* face the direction exiting the room */
        (*c->location->move)(DIR_NORTH, 0);             /* advance 1 space outside of the room */
    }

    /* remove the monster */
    if (combatInfo.monsterObj)
        mapRemoveObject(c->location->map, combatInfo.monsterObj);

    /* If we were camping and were ambushed, wake everyone up! */
    if (combatInfo.camping) {
        for (i = 0; i < c->saveGame->members; i++) {
            if (c->saveGame->players[i].status == STAT_SLEEPING)
                c->saveGame->players[i].status = combatInfo.party[i].status;
        }
    }    
    
    if (playerPartyDead(c->saveGame))
        deathStart(0);
    else
        (*c->location->finishTurn)();
}

void combatMoveMonsters() {
    int i, target, distance;
    CombatAction action;
    CoordActionInfo *info;
    const Monster *m;
    PartyCombatInfo *party = combatInfo.party;
    MonsterCombatInfo *monsters = combatInfo.monsters;

    for (i = 0; i < AREA_MONSTERS; i++) {
        if (!monsters[i].obj)
            continue;
        m = monsters[i].obj->monster;

        /* see if monster wakes up if it is asleep */
        if ((monsters[i].status == STAT_SLEEPING) && (rand() % 8 == 0)) {
            monsters[i].status = STAT_GOOD;
            monsters[i].obj->canAnimate = 1;
        }

        /* if the monster is still asleep, then move on to the next monster */
        if (monsters[i].status == STAT_SLEEPING)
            continue;

        if (monsterNegates(m)) {
            c->aura = AURA_NEGATE;
            c->auraDuration = 2;
            statsUpdate();
        }

        /* default action */
        action = CA_ATTACK;        

        /* if the monster doesn't have something specific to do yet, let's try to find something! */
        if (action == CA_ATTACK) {
            /* monsters who teleport do so 1/8 of the time */
            if (monsterTeleports(m) && (rand() % 8) == 0)
                action = CA_TELEPORT;
            /* monsters who ranged attack do so 1/4 of the time.
               make sure their ranged attack is not negated! */
            else if (m->ranged != 0 && (rand() % 4) == 0 && 
                     ((m->rangedhittile != MAGICFLASH_TILE) || (c->aura != AURA_NEGATE)))
                action = CA_RANGED;
            /* monsters who cast sleep do so 1/4 of the time they don't ranged attack */
            else if (monsterCastSleep(m) && (rand() % 4) == 0)
                action = CA_CAST_SLEEP;
        
            else if (monsterGetStatus(m, monsters[i].hp) == MSTAT_FLEEING)
                action = CA_FLEE;
        }
        
        target = combatFindTargetForMonster(monsters[i].obj, &distance, action == CA_RANGED);
        if (target == -1 && action == CA_RANGED) {
            action = CA_ADVANCE;
            combatFindTargetForMonster(monsters[i].obj, &distance, 0);
        }
        if (target == -1)
            continue;

        if (action == CA_ATTACK && distance > 1)
            action = CA_ADVANCE;

        /* let's see if the monster blends into the background, or if he appears... */
        if (monsterCamouflages(m) && !combatHideOrShowCamouflageMonster(monsters[i].obj))
            continue; /* monster is hidden -- no action! */

        switch(action) {
        case CA_ATTACK:
            if (playerIsHitByAttack(&c->saveGame->players[target])) {
                
                /* steal gold if the monster steals gold */
                if (monsterStealsGold(m) && (rand() % 4 == 0))
                    playerAdjustGold(c->saveGame, -(rand() % 0x3f));
                
                /* steal food if the monster steals food */
                if (monsterStealsFood(m))
                    playerAdjustFood(c->saveGame, -2500);
                               
                attackFlash(party[target].obj->x, party[target].obj->y, HITFLASH_TILE, 3);

                playerApplyDamage(&c->saveGame->players[target], monsterGetDamage(m));
                if (c->saveGame->players[target].status == STAT_DEAD) {
                    int px, py;
                    px = party[target].obj->x;
                    py = party[target].obj->y;
                    mapRemoveObject(c->location->map, party[target].obj);
                    party[target].obj = NULL;
                    annotationSetVisual(annotationSetTurnDuration(annotationAdd(px, py, c->location->z, c->location->map->id, CORPSE_TILE), c->saveGame->members));
                    screenMessage("%s is Killed!\n", c->saveGame->players[target].name);
                }
                statsUpdate();
            } else {
                attackFlash(party[target].obj->x, party[target].obj->y, MISSFLASH_TILE, 3);
            }
            break;

        case CA_CAST_SLEEP:
            screenMessage("Sleep!\n");

            (*spellEffectCallback)('s', -1, 0); /* show the sleep spell effect */
            
            /* Apply the sleep spell to everyone still in combat */
            for (i = 0; i < 8; i++) {
                if ((party[i].obj != NULL) && (rand()%2 == 0))
                    combatPutPlayerToSleep(i);                
            }

            statsUpdate();
            break;

        case CA_TELEPORT: {
                int newx, newy,
                    valid = 0,
                    firstTry = 1;
                unsigned char tile;
                Object *obj;
            
                while (!valid) {
                    newx = rand() % c->location->map->width,
                    newy = rand() % c->location->map->height;
                    
                    tile = (*c->location->tileAt)(c->location->map, newx, newy, c->location->z);
                    obj = mapObjectAt(c->location->map, newx, newy, c->location->z);
                    if (obj)
                        tile = obj->tile;
                
                    if (tileIsMonsterWalkable(tile) && tileIsWalkable(tile)) {
                        /* If the tile would slow me down, try again! */
                        if (firstTry && tileGetSpeed(tile) != FAST)
                            firstTry = 0;
                        /* OK, good enough! */
                        else
                            valid = 1;
                    }
                }
            
                /* Teleport! */
                combatInfo.monsters[i].obj->x = newx;
                combatInfo.monsters[i].obj->y = newy;
            }

            break;

        case CA_RANGED:            
            
            info = (CoordActionInfo *) malloc(sizeof(CoordActionInfo));
            info->handleAtCoord = &combatMonsterRangedAttack;
            info->origin_x = monsters[i].obj->x;
            info->origin_y = monsters[i].obj->y;
            info->prev_x = info->prev_y = -1;
            info->range = 11;
            info->validDirections = MASK_DIR_ALL;
            info->player = i;
            info->blockedPredicate = &tileCanAttackOver;
            info->blockBefore = 1;
            info->firstValidDistance = 0;

            /* if the monster has a random tile for a ranged weapon,
               let's switch it now! */
            if (monsterHasRandomRangedAttack(combatInfo.monsters[i].obj->monster))
                monsterSetRandomRangedWeapon((Monster *)combatInfo.monsters[i].obj->monster);

            /* figure out which direction to fire the weapon */
            info->dir = dirGetRelativeDirection(
                combatInfo.monsters[i].obj->x, combatInfo.monsters[i].obj->y,
                combatInfo.party[target].obj->x, combatInfo.party[target].obj->y);            
            
            /* fire! */
            gameDirectionalAction(info);
            free(info);           

            break;

        case CA_SHOW:
            combatInfo.monsters[i].obj->isVisible = 1;
            break;

        case CA_HIDE:
            combatInfo.monsters[i].obj->isVisible = 0;
            break;

        case CA_FLEE:
        case CA_ADVANCE:
            if (moveCombatObject(action, c->location->map, combatInfo.monsters[i].obj, combatInfo.party[target].obj->x, combatInfo.party[target].obj->y)) {
                int x = combatInfo.monsters[i].obj->x,
                    y = combatInfo.monsters[i].obj->y;

                if (MAP_IS_OOB(c->location->map, x, y)) {
                    screenMessage("\n%s Flees!\n", m->name);
                    
                    /* Congrats, you have a heart! */
                    if (monsterIsGood(combatInfo.monsters[i].obj->monster))
                        playerAdjustKarma(c->saveGame, KA_SPARED_GOOD);

                    mapRemoveObject(c->location->map, combatInfo.monsters[i].obj);
                    combatInfo.monsters[i].obj = NULL;
                }
            }
            
            break;
        }
    }
}

int combatFindTargetForMonster(const Object *monster, int *distance, int ranged) {
    int i, curDistance;
    int closest;    
    
    *distance = 20;
    closest = -1;
    for (i = 0; i < c->saveGame->members; i++) {
        if (!combatInfo.party[i].obj)
            continue;

        /* find out how many moves it would take to get to the party member */
        if (ranged) 
            /* ranged attacks can go diagonally, so find the closest using diagonals */
            curDistance = mapDistance(monster->x, monster->y, combatInfo.party[i].obj->x, combatInfo.party[i].obj->y);
        else
            /* normal attacks are n/e/s/w, so find the distance that way */
            curDistance = mapMovementDistance(monster->x, monster->y, combatInfo.party[i].obj->x, combatInfo.party[i].obj->y);        

        /* skip target if further than current target */
        if (curDistance > (*distance))
            continue;
        /* skip target 50% of time if same distance */
        if (curDistance == (*distance) && (rand() % 2) == 0)
            continue;
        
        (*distance) = curDistance;
        closest = i;
    }

    return closest;
}

int movePartyMember(Direction dir, int userEvent) {
    int result = 1;
    int newx, newy;
    int movementMask;
    int member = combatInfo.partyFocus;    

    newx = combatInfo.party[member].obj->x;
    newy = combatInfo.party[member].obj->y;
    dirMove(dir, &newx, &newy);

    screenMessage("%s\n", getDirectionName(dir));

    if (MAP_IS_OOB(c->location->map, newx, newy)) {
        int sameExit = (!combatInfo.dungeonRoom || (combatInfo.exitDir == DIR_NONE) || (dir == combatInfo.exitDir));
        if (sameExit) {
            
            /* if in a win-or-lose battle and not camping, then it can be bad to flee while healthy */
            if (combatInfo.winOrLose && !combatInfo.camping) {
                /* A fully-healed party member fled from an evil monster :( */
                if (combatInfo.monster && monsterIsEvil(combatInfo.monster) && 
                    c->saveGame->players[member].hp == c->saveGame->players[member].hpMax)
                    playerAdjustKarma(c->saveGame, KA_HEALTHY_FLED_EVIL);
            }

            combatInfo.exitDir = dir;
            mapRemoveObject(c->location->map, combatInfo.party[member].obj);
            combatInfo.party[member].obj = NULL;
            return result;
        }
        else {
            screenMessage("All must use same exit!\n");
            return (result = 0);
        }
    }

    movementMask = mapGetValidMoves(c->location->map, combatInfo.party[member].obj->x, combatInfo.party[member].obj->y, c->location->z, combatInfo.party[member].obj->tile);
    if (!DIR_IN_MASK(dir, movementMask)) {
        screenMessage("Blocked!\n");
        return (result = 0);        
    }

    combatInfo.party[member].obj->x = newx;
    combatInfo.party[member].obj->y = newy;

    /* handle dungeon room triggers */
    if (combatInfo.dungeonRoom) {
        int i;
        Trigger *triggers = c->location->prev->map->dungeon->currentRoom->triggers;            

        for (i = 0; i < 4; i++) {
            const Monster *m = monsterForTile(triggers[i].tile);

            /* FIXME: when a monster is created by a trigger, it can be created over and over and over...
               how do we fix this? */

            /* see if we're on a trigger */
            if (triggers[i].x == newx && triggers[i].y == newy) {
                /* change the tiles! */
                if (triggers[i].change_x1 || triggers[i].change_y1) {                    
                    /*if (m) combatAddMonster(m, triggers[i].change_x1, triggers[i].change_y1, c->location->z);
                    else*/ mapSetTileData(c->location->map, triggers[i].change_x1, triggers[i].change_y1, c->location->z, triggers[i].tile);                
                }
                if (triggers[i].change_x2 || triggers[i].change_y2) {
                    /*if (m) combatAddMonster(m, triggers[i].change_x2, triggers[i].change_y2, c->location->z);
                    else*/ mapSetTileData(c->location->map, triggers[i].change_x2, triggers[i].change_y2, c->location->z, triggers[i].tile);
                }
            }
        }
    }    

    return result;
}

/**
 * Applies 'damage' amount of damage to the monster
 */

void combatApplyDamageToMonster(int monster, int damage, int player) {
    int xp;
    const Monster *m = combatInfo.monsters[monster].obj->monster;

    /* deal the damage */
    if (m->id != LORDBRITISH_ID)
        combatInfo.monsters[monster].hp -= damage;

    switch (monsterGetStatus(m, combatInfo.monsters[monster].hp)) {

    case MSTAT_DEAD:
        xp = m->xp;
        screenMessage("%s Killed!\nExp. %d\n", m->name, xp);
        
        /* if a player killed the creature, then award the XP,
           otherwise, it died on its own */
        if (player >= 0) {
            playerAwardXp(&c->saveGame->players[player], xp);
            if (monsterIsEvil(m))
                playerAdjustKarma(c->saveGame, KA_KILLED_EVIL);
        }

        mapRemoveObject(c->location->map, combatInfo.monsters[monster].obj);
        combatInfo.monsters[monster].obj = NULL;
        break;

    case MSTAT_FLEEING:
        screenMessage("%s Fleeing!\n", m->name);
        break;

    case MSTAT_CRITICAL:
        screenMessage("%s Critical!\n", m->name);
        break;

    case MSTAT_HEAVILYWOUNDED:
        screenMessage("%s\nHeavily Wounded!\n", m->name);
        break;

    case MSTAT_LIGHTLYWOUNDED:
        screenMessage("%s\nLightly Wounded!\n", m->name);
        break;

    case MSTAT_BARELYWOUNDED:
        screenMessage("%s\nBarely Wounded!\n", m->name);
        break;
    }
}

/**
 * Show an attack flash at x, y. This is used for 'being hit' or 'being missed' 
 * by weapons, cannon fire, spells, etc.
 */

void attackFlash(int x, int y, unsigned char tile, int timeFactor) {
    int i;
    int divisor = settings->battleSpeed;
    
    annotationSetVisual(annotationAdd(x, y, c->location->z, c->location->map->id, tile));
    for (i = 0; i < timeFactor; i++) {        
        /* do screen animations while we're pausing */
        if (i % divisor == 1)
            screenCycle();

        gameUpdateScreen();       
        eventHandlerSleep(eventTimerGranularity/divisor);
    }
    annotationRemove(x, y, c->location->z, c->location->map->id, tile);
}

/**
 * Key handler for choosing an attack direction
 */
int combatChooseWeaponDir(int key, void *data) {
    CoordActionInfo *info = (CoordActionInfo *) data;    
    Direction dir = keyToDirection(key);
    int valid = (dir != DIR_NONE);
    int weapon = c->saveGame->players[info->player].weapon;

    eventHandlerPopKeyHandler();
    info->dir = MASK_DIR(dir);

    if (valid) {
        screenMessage("%s\n", getDirectionName(dir));
        if (weaponCanChooseDistance(weapon)) {
            screenMessage("Range: ");
            eventHandlerPushKeyHandlerData(&combatChooseWeaponRange, info);
        }
        else {
            gameDirectionalAction(info);
            free(info);
        }
    } else free(info);

    return valid || keyHandlerDefault(key, NULL);
}

/**
 * Key handler for choosing the range of a wepaon
 */
int combatChooseWeaponRange(int key, void *data) {    
    CoordActionInfo *info = (CoordActionInfo *) data;    

    if ((key >= '0') && (key <= (info->range + '0'))) {
        info->range = key - '0';
        screenMessage("%d\n", info->range);
        gameDirectionalAction(info);

        eventHandlerPopKeyHandler();
        free(info);

        return 1;
    }
    
    return 0;
}

/**
 * Apply tile effects to all monsters depending on what they're standing on
 */
void combatApplyMonsterTileEffects(void) {
    int i, affected = 0;

    for (i = 0; i < AREA_MONSTERS; i++) {
        if (combatInfo.monsters[i].obj) {
            TileEffect effect;
            effect = tileGetEffect((*c->location->tileAt)(c->location->map, combatInfo.monsters[i].obj->x, combatInfo.monsters[i].obj->y, c->location->z));

            if (effect != EFFECT_NONE) {

                /* give a slight pause before enacting the tile effect */
                if (!affected) {
                    gameUpdateScreen();
                    eventHandlerSleep(100);
                    affected = 1;
                }

                switch(effect) {
                case EFFECT_SLEEP:
                    /* monster fell asleep! */
                    if ((combatInfo.monsters[i].obj->monster->resists != EFFECT_SLEEP) &&
                        ((rand() % 0xFF) >= combatInfo.monsters[i].hp)) {
                        combatInfo.monsters[i].status = STAT_SLEEPING;
                        combatInfo.monsters[i].obj->canAnimate = 0; /* freeze monster */
                    }
                    break;

                case EFFECT_LAVA:
                case EFFECT_FIRE:
                    /* deal 0 - 127 damage to the monster if it is not immune to fire damage */
                    if (!(combatInfo.monsters[i].obj->monster->resists & (EFFECT_FIRE | EFFECT_LAVA)))
                        combatApplyDamageToMonster(i, rand() % 0x7F, -1);
                    break;

                case EFFECT_POISONFIELD:
                    /* deal 0 - 127 damage to the monster if it is not immune to poison field damage */
                    if (combatInfo.monsters[i].obj->monster->resists != EFFECT_POISONFIELD)
                        combatApplyDamageToMonster(i, rand() % 0x7F, -1);
                    break;

                case EFFECT_POISON:
                default: break;
                }
            }
        }
    }
}

int combatDivideMonster(const Object *obj) {
    int dirmask = mapGetValidMoves(c->location->map, obj->x, obj->y, c->location->z, obj->tile);
    Direction d = dirRandomDir(dirmask);

    /* this is a game enhancement, make sure it's turned on! */
    if (!settings->minorEnhancements || !settings->minorEnhancementsOptions.slimeDivides)
        return 0;
    
    /* make sure there's a place to put the divided monster! */
    if (d != DIR_NONE) {
        int index;
                            
        /* find the first free slot in the monster table, if there is one */
        for (index = 0; index < AREA_MONSTERS; index++) {
            if (combatInfo.monsters[index].obj == NULL) {
                int x, y;
                
                screenMessage("%s Divides!\n", obj->monster->name);

                /* find a spot to put our new monster */
                x = obj->x;
                y = obj->y;
                dirMove(d, &x, &y);

                /* create our new monster! */
                combatInfo.monsters[index].obj = mapAddMonsterObject(c->location->map, obj->monster, x, y, c->location->z);
                combatInfo.monsters[index].hp = monsterGetInitialHp(combatInfo.monsters[index].obj->monster);
                combatInfo.monsters[index].status = STAT_GOOD;
                return 1;
            }
        }        
    }
    return 0;
}

/**
 * Returns the id of the nearest party member (0-8)
 * and fills 'dist' with the distance
 */
int combatNearestPartyMember(const Object *obj, int *dist) {
    int member, nearest = -1, d, leastDist = 0xFFFF;
    PartyCombatInfo *party = combatInfo.party;

    for (member = 0; member < c->saveGame->members; member++) {
        if (party[member].obj) {
            d = mapMovementDistance(obj->x, obj->y, party[member].obj->x, party[member].obj->y);
            if (d < leastDist) {
                nearest = member;
                leastDist = d;
            }
        }
    }

    if (nearest >= 0)
        *dist = leastDist;

    return nearest;
}

/**
 * Hides or shows a camouflaged monster, depending on its distance from
 * the nearest party member
 */
int combatHideOrShowCamouflageMonster(Object *monster) {
    /* find the nearest party member */
    int dist;
    int nearestMember = combatNearestPartyMember(monster, &dist);

    /* ok, now we've got the nearest party member.  Now, see if they're close enough */
    if (nearestMember >= 0) {
        if ((dist < 5) && !monster->isVisible)
            monster->isVisible = 1; /* show yourself */
        else if (dist >= 5)
            monster->isVisible = 0; /* hide and take no action! */
    }

    return monster->isVisible;
}
