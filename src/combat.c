/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "u4.h"

#include "combat.h"

#include "annotation.h"
#include "context.h"
#include "death.h"
#include "debug.h"
#include "event.h"
#include "game.h"
#include "location.h"
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

extern Map brick_map;
extern Map bridge_map;
extern Map brush_map;
extern Map camp_map;
extern Map dng0_map;
extern Map dng1_map;
extern Map dng2_map;
extern Map dng3_map;
extern Map dng4_map;
extern Map dng5_map;
extern Map dng6_map;
extern Map dungeon_map;
extern Map forest_map;
extern Map grass_map;
extern Map hill_map;
extern Map inn_map;
extern Map marsh_map;
extern Map shipsea_map;
extern Map shipship_map;
extern Map shipshor_map;
extern Map shore_map;
extern Map shorship_map;

CombatInfo combatInfo;

int combatAttackAtCoord(int x, int y, int distance, void *data);
int combatMonsterRangedAttack(int x, int y, int distance, void *data);
int combatReturnWeaponToOwner(int x, int y, int distance, void *data);
int combatIsWon(void);
int combatIsLost(void);
void combatEnd(void);
void combatMoveMonsters(void);
int combatFindTargetForMonster(const Object *monster, int *distance, int ranged);
int movePartyMember(Direction dir, int member);
int combatChooseWeaponDir(int key, void *data);
int combatChooseWeaponRange(int key, void *data);
void combatApplyMonsterTileEffects(void);

void combatBegin(Map *map, Object *monster, int isNormalCombat) {
    int i, j;
    int nmonsters;    
    int partyIsReadyToFight = 0;
    
    /* setup the combatInfo struct */
    combatInfo.monsterObj = monster;
    combatInfo.monster = (monster && (monster->objType == OBJECT_MONSTER)) ?
        monster->monster :
        (monster) ? monsterForTile(monster->tile) : NULL;
    combatInfo.isNormalCombat = isNormalCombat;
    combatInfo.isCamping = (!isNormalCombat && !monster);

    gameSetMap(c, map, 1, NULL);    
        
    /* place party members on the map */
    for (i = 0; i < c->saveGame->members; i++) {     

        if (c->saveGame->players[i].status != STAT_DEAD) {
            combatInfo.party[i] = mapAddObject(c->location->map, tileForClass(c->saveGame->players[i].klass), tileForClass(c->saveGame->players[i].klass), c->location->map->area->player_start[i].x, c->location->map->area->player_start[i].y, c->location->z);
        
            /* Replace the party mamber with a sleeping person if they're asleep */
            if (c->saveGame->players[i].status == STAT_SLEEPING) {
                combatInfo.party_status[i] = STAT_GOOD;
                combatInfo.party[i]->tile = CORPSE_TILE;
            }
            else partyIsReadyToFight = 1;
        }
        else
            combatInfo.party[i] = NULL;
    }
    for (; i < 8; i++)
        combatInfo.party[i] = NULL;

    /* normal combat situation */
    if (!combatInfo.isCamping) {
        i = 0;
        while ((combatInfo.party[i] == NULL) && (c->saveGame->players[i].status != STAT_SLEEPING))
            i++;
        combatInfo.focus = i;
        combatInfo.party[i]->hasFocus = 1;
    }
    
    /* camping, make sure everyone's asleep */
    else {
        partyIsReadyToFight = 0;
        for (i = 0; i < c->saveGame->members; i++) {
            /* save their stats before putting them to sleep */
            combatInfo.party_status[i] = c->saveGame->players[i].status;

            if (c->saveGame->players[i].status != STAT_DEAD) {
                c->saveGame->players[i].status = STAT_SLEEPING;
                combatInfo.party[i]->tile = CORPSE_TILE;
            }
        }
    }

    for (i = 0; i < AREA_MONSTERS; i++)
        combatInfo.monsters[i] = NULL;

    /* check if the battlefield has monsters on it.  If we're camping, we don't want this yet */
    if (combatInfo.monster) {
        nmonsters = combatInitialNumberOfMonsters(combatInfo.monster);    
        for (i = 0; i < nmonsters; i++) {
            /* find a random free slot in the monster table */
            do {j = rand() % AREA_MONSTERS;} while (combatInfo.monsters[j] != NULL);
            combatCreateMonster(j, i != (nmonsters - 1));
        }
    }

    /* Use the combat key handler */
    eventHandlerPushKeyHandler(&combatBaseKeyHandler);

    /* if the combat is a normal combat situation, treat it as such. */
    if (isNormalCombat) {
        screenMessage("\n**** COMBAT ****\n\n");    
        musicPlay();
    }

    /* if the party is ready to fight, show info for the first active player */
    if (partyIsReadyToFight) {
        screenMessage("%s with %s\n\020", c->saveGame->players[combatInfo.focus].name, weaponGetName(c->saveGame->players[combatInfo.focus].weapon));
        statsHighlightCharacter(combatInfo.focus);
        statsUpdate(); /* If a character was awakened inbetween world view and combat, this fixes stats info */
    }
}


Map *getCombatMapForTile(unsigned char partytile, unsigned short transport, const Monster *m) {
    int i;    

    static const struct {
        unsigned char tile;
        Map *map;
    } tileToMap[] = {   
        { HORSE1_TILE,  &grass_map },
        { HORSE2_TILE,  &grass_map },
        { SWAMP_TILE,   &marsh_map },
        { GRASS_TILE,   &grass_map },
        { BRUSH_TILE,   &brush_map },
        { FOREST_TILE,  &forest_map },
        { HILLS_TILE,   &hill_map },
        { DUNGEON_TILE, &hill_map },
        { CITY_TILE,    &grass_map },
        { CASTLE_TILE,  &grass_map },
        { TOWN_TILE,    &grass_map },
        { LCB2_TILE,    &grass_map },
        { BRIDGE_TILE,  &bridge_map },
        { NORTHBRIDGE_TILE, &bridge_map },
        { SOUTHBRIDGE_TILE, &bridge_map },
        { CHEST_TILE,   &grass_map },
        { BRICKFLOOR_TILE, &brick_map },
        { MOONGATE0_TILE, &grass_map },
        { MOONGATE1_TILE, &grass_map },
        { MOONGATE2_TILE, &grass_map },
        { MOONGATE3_TILE, &grass_map }
    };

    /* We can fight monsters and townsfolk -- let's
       figure out which one we're dealing with */
    if (m) {
        
        /* check if monster is aquatic */
        if (monsterIsAquatic(m)) {
            if (tileIsPirateShip(m->tile)) {
                if (tileIsShip(transport) || tileIsShip(partytile))
                    return &shipship_map;
                else
                    return &shorship_map;
            }

            if (tileIsShip(transport) || tileIsShip(partytile))
                return &shipsea_map;
            else
                return &shore_map;
        }
    }

    if (tileIsShip(transport) || tileIsShip(partytile))
        return &shipshor_map;

    for (i = 0; i < sizeof(tileToMap) / sizeof(tileToMap[0]); i++) {
        if (tileToMap[i].tile == partytile)
            return tileToMap[i].map;
    }

    return &brick_map;
}

void combatCreateMonster(int index, int canbeleader) {    
    unsigned char mtile;

    mtile = combatInfo.monsterObj->tile;

    if (tileIsPirateShip(mtile))
        mtile = ROGUE_TILE;

    /* if mtile < 0x80, the monster can't be a leader */
    if (mtile >= 0x80) {
        /* the monster can be normal, a leader or a leader's leader */
        if (canbeleader && (rand() % 32) == 0) {
            /* leader's leader */
            mtile = monsterById(monsterForTile(mtile)->leader)->tile;
            mtile = monsterById(monsterForTile(mtile)->leader)->tile;
        }
        else if (canbeleader && (rand() % 8) == 0) {
            /* leader */
            mtile = monsterById(monsterForTile(mtile)->leader)->tile;
        }
        else
            /* normal */
            ;
    }
    combatInfo.monsters[index] = mapAddMonsterObject(c->location->map, monsterForTile(mtile), c->location->map->area->monster_start[index].x, c->location->map->area->monster_start[index].y, c->location->z);
    combatInfo.monsterHp[index] = monsterGetInitialHp(combatInfo.monsters[index]->monster);
    combatInfo.monster_status[index] = STAT_GOOD;
}

void combatFinishTurn() {    
    int focus = combatInfo.focus;

    if (combatIsWon()) {
        eventHandlerPopKeyHandler();
        combatEnd();
        return;
    }
    
    if (combatInfo.party[focus]) {
        /* apply effects from tile player is standing on */
        playerApplyEffect(c->saveGame, tileGetEffect(mapTileAt(c->location->map, combatInfo.party[focus]->x, combatInfo.party[focus]->y, c->location->z)), focus);
    }

    /* check to see if the player gets to go again (and is still alive) */
    if ((c->aura != AURA_QUICKNESS) || (rand() % 2 == 0) || (c->saveGame->players[focus].hp <= 0)){
        do {
            annotationCycle();

            /* put a sleeping person in place of the player,
               or restore an awakened member to their original state */            
            if (combatInfo.party[focus]) {
                /* wake up! */
                if (c->saveGame->players[focus].status == STAT_SLEEPING && (rand() % 8 == 0)) {
                    c->saveGame->players[focus].status = combatInfo.party_status[focus];
                    statsUpdate();
                }

                /* display a sleeping person or an awake person */                
                if (c->saveGame->players[focus].status == STAT_SLEEPING)
                    combatInfo.party[focus]->tile = CORPSE_TILE;
                else combatInfo.party[focus]->tile = tileForClass(c->saveGame->players[focus].klass);

                /* remove focus from the current party member */
                combatInfo.party[focus]->hasFocus = 0;
            }

            /* eat some food */
            if (c->saveGame->players[focus].status != STAT_DEAD)
                playerAdjustFood(c->saveGame, -1);                

            /* put the focus on the next party member */
            combatInfo.focus++; focus++;           

            /* move monsters and wrap around at end */
            if (focus >= c->saveGame->members) {            

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

                /* reset the focus to the avatar and start the party's turn over again */
                combatInfo.focus = focus = 0;

                /* check to see if combat is over */
                if (combatIsLost()) {
                    if (!playerPartyDead(c->saveGame)) {
                        if (combatInfo.monsterObj && monsterIsGood(combatInfo.monster))
                            playerAdjustKarma(c->saveGame, KA_SPARED_GOOD);
                        else
                            playerAdjustKarma(c->saveGame, KA_FLED_EVIL);
                    }

                    eventHandlerPopKeyHandler();
                    combatEnd();
                    return;
                }

                /* end combat immediately if the enemy has fled */
                else if (combatIsWon()) {
                    eventHandlerPopKeyHandler();
                    combatEnd();
                    return;
                }                
            }
        } while (!combatInfo.party[focus] ||    /* dead */
                 c->saveGame->players[focus].status == STAT_SLEEPING);
    }
    else annotationCycle();

    combatInfo.party[focus]->hasFocus = 1;

    screenMessage("%s with %s\n\020", c->saveGame->players[focus].name, weaponGetName(c->saveGame->players[focus].weapon));
    c->statsItem = STATS_PARTY_OVERVIEW;
    statsUpdate();
    statsHighlightCharacter(focus);    
}

int combatBaseKeyHandler(int key, void *data) {
    int valid = 1;
    CoordActionInfo *info;
    AlphaActionInfo *alphaInfo; 
    int focus = combatInfo.focus;
    int weapon = c->saveGame->players[focus].weapon;    

    switch (key) {
    case U4_UP:
    case U4_DOWN:
    case U4_LEFT:
    case U4_RIGHT:
        movePartyMember(keyToDirection(key), focus);
        break;

    case U4_ESC:
        eventHandlerPopKeyHandler();
        combatEnd();
        break;
        
    case ' ':
        screenMessage("Pass!\n");
        break;

    case 'a':
        info = (CoordActionInfo *) malloc(sizeof(CoordActionInfo));
        info->handleAtCoord = &combatAttackAtCoord;
        info->origin_x = combatInfo.party[focus]->x;
        info->origin_y = combatInfo.party[focus]->y;
        info->prev_x = info->prev_y = -1;
        info->range = weaponGetRange(weapon);
        info->validDirections = MASK_DIR_ALL;
        info->player = focus;        
        info->blockedPredicate = weaponCanAttackThroughObjects(weapon) ?
            NULL :
            &tileCanAttackOver;
        info->blockBefore = 1;
        info->firstValidDistance = 0;
        
        eventHandlerPushKeyHandlerData(&combatChooseWeaponDir, info);        

        screenMessage("Dir: ");        
        break;

    case 'c':        
        gameCastForPlayer(focus);
        break;

    case 'r':
        c->statsItem = STATS_WEAPONS;
        statsUpdate();

        alphaInfo = (AlphaActionInfo *) malloc(sizeof(AlphaActionInfo));
        alphaInfo->lastValidLetter = WEAP_MAX + 'a' - 1;
        alphaInfo->handleAlpha = readyForPlayer2;
        alphaInfo->prompt = "Weapon: ";
        alphaInfo->data = (void *)focus;

        screenMessage(alphaInfo->prompt);

        eventHandlerPushKeyHandlerData(&gameGetAlphaChoiceKeyHandler, alphaInfo);
        break;

    case 'z':
        c->statsItem = (StatsItem) (STATS_CHAR1 + focus);
        statsUpdate();

        eventHandlerPushKeyHandler(&gameZtatsKeyHandler);
        screenMessage("Ztats\n");        
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
        if (eventHandlerGetKeyHandler() == &combatBaseKeyHandler)
            (*c->location->finishTurn)();
    }

    return valid;
}

int combatAttackAtCoord(int x, int y, int distance, void *data) {
    int monster;    
    int i, hittile, misstile;
    CoordActionInfo* info = (CoordActionInfo*)data;    
    int weapon = c->saveGame->players[info->player].weapon;    
    int wrongRange = weaponRangeAbsolute(weapon) && (distance != info->range);
    int oldx = info->prev_x,
        oldy = info->prev_y;  
    int attackdelay = MAX_BATTLE_SPEED - settings->battleSpeed;
    int focus = combatInfo.focus;
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

        /* This goes here so messages are shown in the original order */
        screenMessage("Missed!\n");
    }
    
    /* Check to see if we might hit something */
    else {

        monster = -1;
        for (i = 0; i < AREA_MONSTERS; i++) {
            if (combatInfo.monsters[i] &&
                combatInfo.monsters[i]->x == x &&
                combatInfo.monsters[i]->y == y)
                monster = i;
        }   

        /* If we haven't hit a monster, or the weapon's range is absolute
           and we're testing the wrong range, stop now! */
        if (monster == -1 || wrongRange) {        
        
            /* If the weapon is shown as it travels, show it now */
            if (weaponShowTravel(weapon)) {
                annotationSetVisual(annotationAddTemporary(x, y, c->location->z, c->location->map->id, misstile));
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
            !playerAttackHit(&c->saveGame->players[focus])) {         /* player naturally missed */
            screenMessage("Missed!\n");
        
            /* show the 'miss' tile */
            attackFlash(x, y, misstile, 1);

        } else { /* The weapon hit! */          

            /* show the 'hit' tile */
            attackFlash(x, y, hittile, 1);

            /* apply the damage to the monster */
            combatApplyDamageToMonster(monster, playerGetDamage(&c->saveGame->players[focus]), focus);
        }
    }

    /* Check to see if the weapon returns to its owner */
    if (weaponReturns(weapon))
        combatReturnWeaponToOwner(x, y, distance, data);

    /* If the weapon leaves a tile behind, do it here! (flaming oil, etc) */
    groundTile = mapGroundTileAt(c->location->map, x, y, c->location->z);
    if (!wrongRange && (weaponLeavesTile(weapon) && tileIsWalkable(groundTile)))
        annotationAdd(x, y, c->location->z, c->location->map->id, weaponLeavesTile(weapon));    
    
    (*c->location->finishTurn)();

    return 1;
}

int combatMonsterRangedAttack(int x, int y, int distance, void *data) {
    int player;
    const Monster *m;
    int i, hittile, misstile;
    CoordActionInfo* info = (CoordActionInfo*)data;    
    int oldx = info->prev_x,
        oldy = info->prev_y;  
    int attackdelay = MAX_BATTLE_SPEED - settings->battleSpeed;    
    unsigned char groundTile;
    
    info->prev_x = x;
    info->prev_y = y;

    hittile = combatInfo.monsters[info->player]->monster->rangedhittile;
    misstile = combatInfo.monsters[info->player]->monster->rangedmisstile;

    /* Remove the last weapon annotation left behind */
    if ((distance > 0) && (oldx >= 0) && (oldy >= 0))
        annotationRemove(oldx, oldy, c->location->z, c->location->map->id, misstile);

    /* Check to see if the monster hit a party member */
    if (x != -1 && y != -1) {   

        player = -1;
        for (i = 0; i < AREA_PLAYERS; i++) {
            if (combatInfo.party[i] &&
                combatInfo.party[i]->x == x &&
                combatInfo.party[i]->y == y)
                player = i;
        }   

        /* If we haven't hit a player, stop now */
        if (player == -1) {
        
            annotationSetVisual(annotationAddTemporary(x, y, c->location->z, c->location->map->id, misstile));
            gameUpdateScreen();
    
            /* Based on attack speed setting in setting struct, make a delay for
               the attack annotation */
            if (attackdelay > 0)
                eventHandlerSleep(attackdelay * 2);

            return 0;
        }    
  
        /* Did the weapon miss? */
        if (!playerIsHitByAttack(&c->saveGame->players[player])) {
        
            /* show the 'miss' tile */
            attackFlash(x, y, misstile, 2);

        } else { /* The weapon hit! */
            m = mapObjectAt(c->location->map, info->origin_x, info->origin_y, c->location->z)->monster;

            /* show the 'hit' tile */
            attackFlash(x, y, hittile, 2); 
            playerApplyDamage(&c->saveGame->players[player], monsterGetDamage(m));

            /* show the 'hit' message */
            screenMessage("\n%s Hit!\n", c->saveGame->players[player].name);
        }
    }
    else {
        m = mapObjectAt(c->location->map, info->origin_x, info->origin_y, c->location->z)->monster;

        /* If the monster leaves a tile behind, do it here! (lava lizard, etc) */
        groundTile = mapGroundTileAt(c->location->map, oldx, oldy, c->location->z);
        if (monsterLeavesTile(m) && tileIsWalkable(groundTile))
            annotationAdd(oldx, oldy, c->location->z, c->location->map->id, hittile);
    }

    return 1;
}


int combatReturnWeaponToOwner(int x, int y, int distance, void *data) {
    int i, new_x, new_y, misstile, dir;
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
        
        annotationSetVisual(annotationAddTemporary(new_x, new_y, c->location->z, c->location->map->id, misstile));
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
    if (!combatInfo.isNormalCombat || mapIsWorldMap(c->location->prev->map)) {
        nmonsters = (rand() % 8) + 1;        
        
        if (nmonsters == 1) {            
            if (monster->encounterSize > 0)
                nmonsters = (rand() % monster->encounterSize) + monster->encounterSize + 1;
            else
                nmonsters = 8;
        }

        while (nmonsters > 2 * c->saveGame->members) {
            nmonsters = (rand() % 16) + 1;
        }
    } else {
        if (monster->id == GUARD_ID)
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
        if (combatInfo.monsters[i])
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
        if (combatInfo.party[i])
            activePlayers++;
    }

    return activePlayers == 0;
}

void combatEnd() {
    
    gameExitToParentMap(c);
    musicPlay();
    
    if (combatIsWon()) {

        /* added chest or captured ship object */        
        if (combatInfo.monster && combatInfo.isNormalCombat) {
            if (!monsterIsAquatic(combatInfo.monster))
                mapAddObject(c->location->map, tileGetChestBase(), tileGetChestBase(), combatInfo.monsterObj->x, combatInfo.monsterObj->y, c->location->z);
            else if (tileIsPirateShip(combatInfo.monsterObj->tile)) {
                unsigned char ship = tileGetShipBase();
                tileSetDirection(&ship, tileGetDirection(combatInfo.monsterObj->tile));
                mapAddObject(c->location->map, ship, ship, combatInfo.monsterObj->x, combatInfo.monsterObj->y, c->location->z);
            }
        }

        screenMessage("\nVictory!\n");
    }

    else if (!playerPartyDead(c->saveGame))
        screenMessage("Battle is lost!\n");

    if (combatInfo.monsterObj)
        mapRemoveObject(c->location->map, combatInfo.monsterObj);
    
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

    for (i = 0; i < AREA_MONSTERS; i++) {
        if (!combatInfo.monsters[i])
            continue;
        m = combatInfo.monsters[i]->monster;

        /* see if monster wakes up if it is asleep */
        if ((combatInfo.monster_status[i] == STAT_SLEEPING) && (rand() % 8 == 0)) {
            combatInfo.monster_status[i] = STAT_GOOD;
            combatInfo.monsters[i]->canAnimate = 1;
        }

        /* if the monster is still asleep, then move on to the next monster */
        if (combatInfo.monster_status[i] == STAT_SLEEPING)
            continue;

        if (monsterNegates(m)) {
            c->aura = AURA_NEGATE;
            c->auraDuration = 2;
            statsUpdate();
        }

        /* default action */
        action = CA_ATTACK;

        /* let's see if the monster blends into the background, or if he appears... */
        if (monsterCamouflages(m)) {            
            int member, nearest = 0, dist, leastDist = 0xFFFF;

            /* find the nearest party member */
            for (member = 0; member < c->saveGame->members; member++) {
                if (combatInfo.party[member]) {
                    dist = mapMovementDistance(combatInfo.monsters[i]->x, combatInfo.monsters[i]->y,
                                               combatInfo.party[member]->x, combatInfo.party[member]->y);
                    if (dist < leastDist) {
                        nearest = member;
                        leastDist = dist;
                    }
                }
            }

            /* ok, now we've got the nearest party member.  Now, see if they're close enough */
            if ((leastDist <= 5) && !combatInfo.monsters[i]->isVisible)
                action = CA_SHOW; /* show yourself! */           
            else if (leastDist > 5)
                action = CA_HIDE; /* hide and take no action! */
        }

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
        
            else if (monsterGetStatus(m, combatInfo.monsterHp[i]) == MSTAT_FLEEING)
                action = CA_FLEE;
        }
        
        target = combatFindTargetForMonster(combatInfo.monsters[i], &distance, action == CA_RANGED);
        if (target == -1 && action == CA_RANGED) {
            action = CA_ADVANCE;
            combatFindTargetForMonster(combatInfo.monsters[i], &distance, 0);
        }
        if (target == -1)
            continue;

        if (action == CA_ATTACK && distance > 1)
            action = CA_ADVANCE;

        switch(action) {
        case CA_ATTACK:
            if (playerIsHitByAttack(&c->saveGame->players[target])) {
                
                /* steal gold if the monster steals gold */
                if (monsterStealsGold(m) && (rand() % 4 == 0))
                    playerAdjustGold(c->saveGame, -(rand() % 0x3f));
                
                /* steal food if the monster steals food */
                if (monsterStealsFood(m))
                    playerAdjustFood(c->saveGame, -2500);
                               
                attackFlash(combatInfo.party[target]->x, combatInfo.party[target]->y, HITFLASH_TILE, 1);

                playerApplyDamage(&c->saveGame->players[target], monsterGetDamage(m));
                if (c->saveGame->players[target].status == STAT_DEAD) {
                    int px, py;
                    px = combatInfo.party[target]->x;
                    py = combatInfo.party[target]->y;
                    mapRemoveObject(c->location->map, combatInfo.party[target]);
                    combatInfo.party[target] = NULL;
                    annotationSetVisual(annotationSetTurnDuration(annotationAdd(px, py, c->location->z, c->location->map->id, CORPSE_TILE), c->saveGame->members));
                    screenMessage("%s is Killed!\n", c->saveGame->players[target].name);
                }
                statsUpdate();
            } else {
                attackFlash(combatInfo.party[target]->x, combatInfo.party[target]->y, MISSFLASH_TILE, 1);
            }
            break;

        case CA_CAST_SLEEP:
            screenMessage("Sleep!\n");

            (*spellCallback)('s', -1); /* show the sleep spell effect */
            
            /* Apply the sleep spell to everyone still on combat */
            for (i = 0; i < 8; i++) {
                if (combatInfo.party[i] != NULL) {
                    /* save the original status for the party member, we'll need it later! */
                    if (c->saveGame->players[i].status != STAT_SLEEPING)
                        combatInfo.party_status[i] = c->saveGame->players[i].status;

                    playerApplySleepSpell(&c->saveGame->players[i]);                
                    /* display a sleeping person if they were put to sleep */
                    if (c->saveGame->players[i].status == STAT_SLEEPING)
                        combatInfo.party[i]->tile = CORPSE_TILE;                
                }
            }

            statsUpdate();
            break;

        case CA_TELEPORT: {
                int newx, newy, tile,
                    valid = 0,
                    firstTry = 1;
            
                while (!valid) {
                    newx = rand() % c->location->map->width,
                    newy = rand() % c->location->map->height;
                    tile = mapTileAt(c->location->map, newx, newy, c->location->z);
                
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
                combatInfo.monsters[i]->x = newx;
                combatInfo.monsters[i]->y = newy;
            }

            break;

        case CA_RANGED:            
            
            info = (CoordActionInfo *) malloc(sizeof(CoordActionInfo));
            info->handleAtCoord = &combatMonsterRangedAttack;
            info->origin_x = combatInfo.monsters[i]->x;
            info->origin_y = combatInfo.monsters[i]->y;
            info->prev_x = info->prev_y = -1;
            info->range = 11;
            info->validDirections = MASK_DIR_ALL;
            info->player = i;
            info->blockedPredicate = &tileCanAttackOver;
            info->blockBefore = 1;
            info->firstValidDistance = 0;

            /* if the monster has a random tile for a ranged weapon,
               let's switch it now! */
            if (monsterHasRandomRangedAttack(combatInfo.monsters[i]->monster))
                monsterSetRandomRangedWeapon((Monster *)combatInfo.monsters[i]->monster);

            /* figure out which direction to fire the weapon */
            info->dir = dirGetRelativeDirection(
                combatInfo.monsters[i]->x, combatInfo.monsters[i]->y,
                combatInfo.party[target]->x, combatInfo.party[target]->y);            
            
            /* fire! */
            gameDirectionalAction(info);
            free(info);           

            break;

        case CA_SHOW:
            combatInfo.monsters[i]->isVisible = 1;            
            break;

        case CA_HIDE:
            combatInfo.monsters[i]->isVisible = 0;            
            break;

        case CA_FLEE:
        case CA_ADVANCE:
            if (moveCombatObject(action, c->location->map, combatInfo.monsters[i], combatInfo.party[target]->x, combatInfo.party[target]->y)) {
                if (MAP_IS_OOB(c->location->map, (int)combatInfo.monsters[i]->x, (int)combatInfo.monsters[i]->y)) {
                    screenMessage("\n%s Flees!\n", m->name);
                    
                    /* Congrats, you have a heart! */
                    if (monsterIsGood(combatInfo.monsters[i]->monster))
                        playerAdjustKarma(c->saveGame, KA_SPARED_GOOD);

                    mapRemoveObject(c->location->map, combatInfo.monsters[i]);
                    combatInfo.monsters[i] = NULL;
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
        if (!combatInfo.party[i])
            continue;

        /* find out how many moves it would take to get to the party member */
        if (ranged) 
            /* ranged attacks can go diagonally, so find the closest using diagonals */
            curDistance = mapDistance(monster->x, monster->y, combatInfo.party[i]->x, combatInfo.party[i]->y);
        else
            /* normal attacks are n/e/s/w, so find the distance that way */
            curDistance = mapMovementDistance(monster->x, monster->y, combatInfo.party[i]->x, combatInfo.party[i]->y);        

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

int movePartyMember(Direction dir, int member) {
    int result = 1;
    int newx, newy;
    int movementMask;

    newx = combatInfo.party[member]->x;
    newy = combatInfo.party[member]->y;
    dirMove(dir, &newx, &newy);

    screenMessage("%s\n", getDirectionName(dir));

    if (MAP_IS_OOB(c->location->map, newx, newy)) {
        mapRemoveObject(c->location->map, combatInfo.party[member]);
        combatInfo.party[member] = NULL;
        return result;
    }

    movementMask = mapGetValidMoves(c->location->map, combatInfo.party[member]->x, combatInfo.party[member]->y, c->location->z, combatInfo.party[member]->tile);
    if (!DIR_IN_MASK(dir, movementMask)) {
        screenMessage("Blocked!\n");
        return (result = 0);        
    }

    combatInfo.party[member]->x = newx;
    combatInfo.party[member]->y = newy; 

    return result;
}

/**
 * Applies 'damage' amount of damage to the monster
 */

void combatApplyDamageToMonster(int monster, int damage, int player) {
    int xp;
    const Monster *m = combatInfo.monsters[monster]->monster;

    /* deal the damage */
    if (m->id != LORDBRITISH_ID)
        combatInfo.monsterHp[monster] -= damage;

    switch (monsterGetStatus(m, combatInfo.monsterHp[monster])) {

    case MSTAT_DEAD:
        xp = monsterGetXp(m);
        screenMessage("%s Killed!\nExp. %d\n", m->name, xp);
        
        /* if a player killed the creature, then award the XP,
           otherwise, it died on its own */
        if (player >= 0) {
            playerAwardXp(&c->saveGame->players[player], xp);
            if (monsterIsEvil(m))
                playerAdjustKarma(c->saveGame, KA_KILLED_EVIL);
        }

        mapRemoveObject(c->location->map, combatInfo.monsters[monster]);
        combatInfo.monsters[monster] = NULL;
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

void attackFlash(int x, int y, int tile, int timeFactor) {
    int attackdelay = MAX_BATTLE_SPEED - settings->battleSpeed;
    int divisor = 8 * timeFactor;
    int mult = 12 * timeFactor;    

    annotationSetVisual(annotationSetTimeDuration(annotationAdd(x, y, c->location->z, c->location->map->id, tile), (attackdelay + divisor)/divisor));
    gameUpdateScreen();
    eventHandlerSleep((attackdelay+2) * mult);
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
        if (combatInfo.monsters[i]) {
            TileEffect effect;
            effect = tileGetEffect(mapTileAt(c->location->map, combatInfo.monsters[i]->x, combatInfo.monsters[i]->y, c->location->z));

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
                    if ((combatInfo.monsters[i]->monster->resists != EFFECT_SLEEP) &&
                        ((rand() % 0xFF) >= combatInfo.monsterHp[i])) {
                        combatInfo.monster_status[i] = STAT_SLEEPING;
                        combatInfo.monsters[i]->canAnimate = 0; /* freeze monster */
                    }
                    break;

                case EFFECT_FIRE:
                    /* deal 0 - 127 damage to the monster if it is not immune to fire damage */
                    if (combatInfo.monsters[i]->monster->resists != EFFECT_FIRE)
                        combatApplyDamageToMonster(i, rand() % 0x7F, -1);
                    break;

                case EFFECT_POISONFIELD:
                    /* deal 0 - 127 damage to the monster if it is not immune to poison field damage */
                    if (combatInfo.monsters[i]->monster->resists != EFFECT_POISONFIELD)
                        combatApplyDamageToMonster(i, rand() % 0x7F, -1);
                    break;

                case EFFECT_POISON:
                default: break;
                }
            }
        }
    }
}