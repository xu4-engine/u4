/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "combat.h"
#include "context.h"
#include "ttype.h"
#include "location.h"
#include "object.h"
#include "annotation.h"
#include "event.h"
#include "savegame.h"
#include "game.h"
#include "area.h"
#include "monster.h"
#include "screen.h"
#include "names.h"
#include "player.h"
#include "death.h"
#include "stats.h"
#include "weapon.h"
#include "debug.h"

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

Object *monsterObj;
int focus;
Object *party[8];
Object *monsters[AREA_MONSTERS];
int monsterHp[AREA_MONSTERS];

void combatCreateMonster(int index, int canbeleader);
int combatBaseKeyHandler(int key, void *data);
int combatZtatsKeyHandler(int key, void *data);
int combatReadyForPlayer(int weapon, void *data);
void combatFinishTurn(void);
int combatAttackAtCoord(int x, int y, int distance, void *data);
int combatInitialNumberOfMonsters(unsigned char monster);
int combatIsWon(void);
int combatIsLost(void);
void combatEnd(void);
void combatMoveMonsters(void);
int combatFindTargetForMonster(const Object *monster, int *distance, int ranged);
int movePartyMember(Direction dir, int member);

void combatBegin(unsigned char partytile, unsigned short transport, Object *monster) {
    int i, j;
    int nmonsters;

    monsterObj = monster;

    gameSetMap(c, getCombatMapForTile(partytile, transport), 1, NULL);
    musicPlay();

    for (i = 0; i < c->saveGame->members; i++) {
        if (c->saveGame->players[i].status != STAT_DEAD)
            party[i] = mapAddObject(c->location->map, tileForClass(c->saveGame->players[i].klass), tileForClass(c->saveGame->players[i].klass), c->location->map->area->player_start[i].x, c->location->map->area->player_start[i].y, c->location->z);
        else
            party[i] = NULL;
    }
    for (; i < 8; i++)
        party[i] = NULL;

    i = 0;
    while (party[i] == NULL)
        i++;
    focus = i;
    party[i]->hasFocus = 1;

    nmonsters = combatInitialNumberOfMonsters(monster->tile);
    for (i = 0; i < AREA_MONSTERS; i++)
        monsters[i] = NULL;
    for (i = 0; i < nmonsters; i++) {
        /* find a random free slot in the monster table */
        do {j = rand() % AREA_MONSTERS;} while (monsters[j] != NULL);
        combatCreateMonster(j, i != (nmonsters - 1));
    }

    eventHandlerPushKeyHandler(&combatBaseKeyHandler);

    screenMessage("\n**** COMBAT ****\n\n");

    screenMessage("%s with %s\n\020", c->saveGame->players[focus].name, weaponGetName(c->saveGame->players[focus].weapon));
    statsHighlightCharacter(focus);
}


Map *getCombatMapForTile(unsigned char partytile, unsigned short transport) {
    int i;
    static const struct {
        unsigned char tile;
        Map *map;
    } tileToMap[] = {
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
        { BRICKFLOOR_TILE, &brick_map },
        { MOONGATE0_TILE, &grass_map },
        { MOONGATE1_TILE, &grass_map },
        { MOONGATE2_TILE, &grass_map },
        { MOONGATE3_TILE, &grass_map }
    };
    
    /* check if monster is aquatic */
    if (monsterForTile(monsterObj->tile)->mattr & MATTR_WATER) {
        if (tileIsPirateShip(monsterObj->tile)) {
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

    mtile = monsterObj->tile;

    if (tileIsPirateShip(mtile))
        mtile = ROGUE_TILE;

    /* if mtile < 0x80, the monster can't be a leader */
    if (mtile >= 0x80) {
        /* the monster can be normal, a leader or a leader's leader */
        if (canbeleader && (rand() % 32) == 0) {
            /* leader's leader */
            mtile = monsterForTile(mtile)->leader;
            mtile = monsterForTile(mtile)->leader;
        }
        else if (canbeleader && (rand() % 8) == 0) {
            /* leader */
            mtile = monsterForTile(mtile)->leader;
        }
        else
            /* normal */
            ;
    }
    monsters[index] = mapAddObject(c->location->map, mtile, mtile, c->location->map->area->monster_start[index].x, c->location->map->area->monster_start[index].y, c->location->z);

    monsterHp[index] = monsterGetInitialHp(monsterForTile(monsters[index]->tile));
}

void combatFinishTurn() {
    if (combatIsWon()) {
        eventHandlerPopKeyHandler();
        combatEnd();
        return;
    }

    if (party[focus]) {
        /* apply effects from tile player is standing on */
        playerApplyEffect(c->saveGame, tileGetEffect(mapTileAt(c->location->map, party[focus]->x, party[focus]->y, c->location->z)), focus);

        /* remove focus */
        party[focus]->hasFocus = 0;
    }

    do {
        /* put the focus on the next party member */
        focus++;
        annotationCycle();

        /* move monsters and wrap around at end */
        if (focus >= c->saveGame->members) {
            
            combatMoveMonsters();

            focus = 0;
            if (combatIsLost()) {
                if (!playerPartyDead(c->saveGame))
                    playerAdjustKarma(c->saveGame, KA_FLED);
                eventHandlerPopKeyHandler();
                combatEnd();
                return;
            }
            /* End combat immediately if the enemy has fled */
            else if (combatIsWon()) {
                eventHandlerPopKeyHandler();
                combatEnd();
                return;
            }

            /* adjust food and moves */
            playerEndTurn(c->saveGame);

            /* check if aura has expired */
            if (c->auraDuration > 0) {
                if (--c->auraDuration == 0)
                    c->aura = AURA_NONE;
            }

        }
    } while (!party[focus] ||    /* dead */
             c->saveGame->players[focus].status == STAT_SLEEPING);
    party[focus]->hasFocus = 1;

    screenMessage("%s with %s\n\020", c->saveGame->players[focus].name, weaponGetName(c->saveGame->players[focus].weapon));
    statsUpdate();
    statsHighlightCharacter(focus);    
}

int combatBaseKeyHandler(int key, void *data) {
    int valid = 1;
    CoordActionInfo *info;
    AlphaActionInfo *alphaInfo;

    switch (key) {
    case U4_UP:
        movePartyMember(DIR_NORTH, focus);
        break;

    case U4_DOWN:
        movePartyMember(DIR_SOUTH, focus);
        break;

    case U4_LEFT:
        movePartyMember(DIR_WEST, focus);
        break;

    case U4_RIGHT:
        movePartyMember(DIR_EAST, focus);
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
        info->origin_x = party[focus]->x;
        info->origin_y = party[focus]->y;
        info->range = weaponGetRange(c->saveGame->players[focus].weapon);
        info->validDirections = MASK_DIR_ALL;
        info->player = focus;
        info->blockedPredicate = &tileCanAttackOver;
        eventHandlerPushKeyHandlerData(&gameGetCoordinateKeyHandler, info);
        screenMessage("Dir: ");        
        break;

    case 'r':
        c->statsItem = STATS_WEAPONS;
        statsUpdate();

        alphaInfo = (AlphaActionInfo *) malloc(sizeof(AlphaActionInfo));
        alphaInfo->lastValidLetter = WEAP_MAX + 'a' - 1;
        alphaInfo->handleAlpha = combatReadyForPlayer;
        alphaInfo->prompt = "Weapon: ";
        alphaInfo->data = (void *) focus;

        screenMessage(alphaInfo->prompt);

        eventHandlerPushKeyHandlerData(&gameGetAlphaChoiceKeyHandler, alphaInfo);
        break;

    case 'z':
        eventHandlerPushKeyHandler(&combatZtatsKeyHandler);
        screenMessage("Ztats\n");
        c->statsItem = (StatsItem) (STATS_CHAR1 + focus);
        statsUpdate();
        break;

    case 'x' + U4_ALT:
        eventHandlerSetExitFlag(1);
        break;

    default:
        valid = 0;
        break;
    }

    if (valid) {
        if (eventHandlerGetKeyHandler() == &combatBaseKeyHandler)
            combatFinishTurn();
    }

    return valid;
}

int combatZtatsKeyHandler(int key, void *data) {
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
                
        c->statsItem = STATS_PARTY_OVERVIEW;
        statsUpdate();

        combatFinishTurn();
        break;
    }

    statsUpdate();

    return 1;
}

int combatReadyForPlayer(int w, void *data) {    
    int player = (int) data;
    WeaponType weapon = (WeaponType) w, oldWeapon;

    // Return view to party overview
    c->statsItem = STATS_PARTY_OVERVIEW;
    statsUpdate();

    if (weapon != WEAP_HANDS && c->saveGame->weapons[weapon] < 1) {
        screenMessage("None left!\n");
        combatFinishTurn();
        return 0;
    }    

    if (!weaponCanReady(weapon, getClassName(c->saveGame->players[player].klass))) {
        screenMessage("\nA %s may NOT\nuse\n%s\n", getClassName(c->saveGame->players[player].klass), weaponGetName(weapon));
        combatFinishTurn();
        return 0;
    }

    oldWeapon = c->saveGame->players[player].weapon;
    if (oldWeapon != WEAP_HANDS)
        c->saveGame->weapons[oldWeapon]++;
    if (weapon != WEAP_HANDS)
        c->saveGame->weapons[weapon]--;
    c->saveGame->players[player].weapon = weapon;

    screenMessage("%s\n", weaponGetName(weapon));

    combatFinishTurn();

    return 1;    
}

int combatAttackAtCoord(int x, int y, int distance, void *data) {
    int monster;
    const Monster *m;
    int i, xp, hittile, misstile;
    CoordActionInfo* info = (CoordActionInfo*)data;
    int weapon = c->saveGame->players[info->player].weapon;    

    hittile = weaponGetHitTile(weapon);
    misstile = weaponGetMissTile(weapon);    

    if (x == -1 && y == -1) {        
        combatFinishTurn();
        return 1;
    }

    monster = -1;
    for (i = 0; i < AREA_MONSTERS; i++) {
        if (monsters[i] &&
            monsters[i]->x == x &&
            monsters[i]->y == y)
            monster = i;
    }

    if (monster == -1) {
        annotationSetVisual(annotationSetTimeDuration(annotationAdd(x, y, c->location->z, c->location->map->id, misstile), 1));
        return 0;
    }
    /* If the weapon leaves a tile behind, do it here! (flaming oil) */
    else if (weaponLeavesTile(weapon))
        annotationAdd(x, y, c->location->z, c->location->map->id, weaponLeavesTile(weapon));       

    if (!playerAttackHit(&c->saveGame->players[focus])) {
        screenMessage("Missed!\n");

        annotationSetVisual(annotationSetTimeDuration(annotationAdd(x, y, c->location->z, c->location->map->id, misstile), 2));

    } else {
        m = monsterForTile(monsters[monster]->tile);

        annotationSetVisual(annotationSetTimeDuration(annotationAdd(x, y, c->location->z, c->location->map->id, hittile), 2));

        if (m->tile != LORDBRITISH_TILE)
            monsterHp[monster] -= playerGetDamage(&c->saveGame->players[focus]);

        switch (monsterGetStatus(m, monsterHp[monster])) {

        case MSTAT_DEAD:
            xp = monsterGetXp(m);
            screenMessage("%s Killed!\nExp. %d\n", m->name, xp);
            playerAwardXp(&c->saveGame->players[focus], xp);
            if (monsterIsEvil(m))
                playerAdjustKarma(c->saveGame, KA_KILLED_EVIL);
            mapRemoveObject(c->location->map, monsters[monster]);
            monsters[monster] = NULL;
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
    
    combatFinishTurn();

    return 1;
}

/**
 * Generate the number of monsters in a group.
 */
int combatInitialNumberOfMonsters(unsigned char monster) {
    int nmonsters;

    if (mapIsWorldMap(c->location->prev->map)) {
        nmonsters = (rand() % 8) + 1;
        if (nmonsters == 1) {
            /* file offset 116DDh, 36 bytes, indexed by monster number */
            /*nmonsters = rand() % EncounterSize(monster encountered) + EncounterSize(monster encountered) + 1*/
            nmonsters = 8;
        }

        while (nmonsters > 2 * c->saveGame->members) {
            nmonsters = (rand() % 16) + 1;
        }
    }

    else {
        if (monster == GUARD_TILE)
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
        if (monsters[i])
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
        if (party[i])
            activePlayers++;
    }

    return activePlayers == 0;
}

void combatEnd() {
    gameExitToParentMap(c);
    
    if (combatIsWon()) {

        /* added chest or captured ship object */
        if ((monsterForTile(monsterObj->tile)->mattr & MATTR_WATER) == 0)
            mapAddObject(c->location->map, tileGetChestBase(), tileGetChestBase(), monsterObj->x, monsterObj->y, c->location->z);
        else if (tileIsPirateShip(monsterObj->tile)) {
            unsigned short ship = tileGetShipBase();
            tileSetDirection(&ship, tileGetDirection(monsterObj->tile));
            mapAddObject(c->location->map, ship, ship, monsterObj->x, monsterObj->y, c->location->z);
        }

        screenMessage("\nVictory!\n");
    }

    else if (!playerPartyDead(c->saveGame))
        screenMessage("Battle is lost!\n");

    mapRemoveObject(c->location->map, monsterObj);
    
    if (playerPartyDead(c->saveGame))
        deathStart(0);
    else
        gameFinishTurn();
}

void combatMoveMonsters() {
    int i, newx, newy, valid_dirs, target, distance;
    CombatAction action;
    const Monster *m;
    Direction dir;
    int slow;

    for (i = 0; i < AREA_MONSTERS; i++) {
        if (!monsters[i])
            continue;
        m = monsterForTile(monsters[i]->tile);

        if (m->mattr & MATTR_NEGATE) {
            c->aura = AURA_NEGATE;
            c->auraDuration = 2;
            statsUpdate();
        }

        if (m->ranged != 0 && (rand() % 4) == 0)
            action = CA_RANGED;
        else if (monsterCastSleep(m) && (rand() % 4) == 0)
            action = CA_CAST_SLEEP;
        else if (monsterGetStatus(m, monsterHp[i]) == MSTAT_FLEEING)
            action = CA_FLEE;
        else
            action = CA_ATTACK;

        target = combatFindTargetForMonster(monsters[i], &distance, action == CA_RANGED);
        if (target == -1 && action == CA_RANGED) {
            action = CA_ADVANCE;
            combatFindTargetForMonster(monsters[i], &distance, action == CA_RANGED);
        }
        if (target == -1)
            continue;

        if (action == CA_ATTACK && distance > 1)
            action = CA_ADVANCE;

        switch(action) {
        case CA_ATTACK:
            if (playerIsHitByAttack(&c->saveGame->players[target])) {

                annotationSetVisual(annotationSetTimeDuration(annotationAdd(party[target]->x, party[target]->y, c->location->z, c->location->map->id, HITFLASH_TILE), 2));

                playerApplyDamage(&c->saveGame->players[target], monsterGetDamage(m));
                if (c->saveGame->players[target].status == STAT_DEAD) {
                    int px, py;
                    px = party[target]->x;
                    py = party[target]->y;
                    mapRemoveObject(c->location->map, party[target]);
                    party[target] = NULL;
                    annotationSetVisual(annotationSetTurnDuration(annotationAdd(px, py, c->location->z, c->location->map->id, CORPSE_TILE), c->saveGame->members));
                    screenMessage("%s is Killed!\n", c->saveGame->players[target].name);
                }
                statsUpdate();
            } else {
                annotationSetVisual(annotationSetTimeDuration(annotationAdd(party[target]->x, party[target]->y, c->location->z, c->location->map->id, MISSFLASH_TILE), 2));
            }
            break;

        case CA_CAST_SLEEP:
            screenMessage("Sleep!\n");
            playerApplySleepSpell(c->saveGame);
            statsUpdate();
            break;

        case CA_RANGED:
            printf("%s does a ranged attack!\n", m->name);
            /* FIXME */
            break;

        case CA_FLEE:
        case CA_ADVANCE:
            newx = monsters[i]->x;
            newy = monsters[i]->y;
            valid_dirs = mapGetValidMoves(c->location->map, newx, newy, c->location->z, monsters[i]->tile);
            if (action == CA_FLEE)
                dir = dirFindPathToEdge(newx, newy, c->location->map->width, c->location->map->height, valid_dirs);
            else
            {
                // If they're not fleeing, make sure they don't flee on accident
                if (newx == 0)
                    valid_dirs = DIR_REMOVE_FROM_MASK(DIR_WEST, valid_dirs);
                if (newx == c->location->map->width - 1)
                    valid_dirs = DIR_REMOVE_FROM_MASK(DIR_EAST, valid_dirs);
                if (newy == 0)
                    valid_dirs = DIR_REMOVE_FROM_MASK(DIR_NORTH, valid_dirs);
                if (newy == c->location->map->height - 1)
                    valid_dirs = DIR_REMOVE_FROM_MASK(DIR_SOUTH, valid_dirs);

                dir = dirFindPath(newx, newy, party[target]->x, party[target]->y, valid_dirs);
            }
            dirMove(dir, &newx, &newy);

            switch (tileGetSpeed(mapTileAt(c->location->map, newx, newy, c->location->z))) {
            case FAST:
                slow = 0;
                break;
            case SLOW:
                slow = (rand() % 8) == 0;
                break;
            case VSLOW:
                slow = (rand() % 4) == 0;
                break;
            case VVSLOW:
                slow = (rand() % 2) == 0;
                break;
            }
            if (!slow) {
                if (newx != monsters[i]->x ||
                    newy != monsters[i]->y) {
                    monsters[i]->prevx = monsters[i]->x;
                    monsters[i]->prevy = monsters[i]->y;
                }
                monsters[i]->x = newx;
                monsters[i]->y = newy;

                if (MAP_IS_OOB(c->location->map, newx, newy)) {
                    screenMessage("\n%s Flees!\n", m->name);
                    mapRemoveObject(c->location->map, monsters[i]);
                    monsters[i] = NULL;                    
                }
            }
            break;
        }
    }
}

int combatFindTargetForMonster(const Object *monster, int *distance, int ranged) {
    int i, dx, dy;
    int closest;

    *distance = 1000;
    closest = -1;
    for (i = 0; i < c->saveGame->members; i++) {
        if (!party[i])
            continue;

        dx = monster->x - party[i]->x;
        dx *= dx;
        dy = monster->y - party[i]->y;
        dy *= dy;

        /* skip target if further than current target */
        if (dx + dy > (*distance))
            continue;
        /* skip target 50% of time if same distance */
        if (dx + dy == (*distance) && (rand() % 2) == 0)
            continue;
        /* skip target if ranged attack and column or row not shared */
        if (ranged && dx != 0 && dy != 0)
            continue;

        (*distance) = dx + dy;
        closest = i;
    }

    return closest;
}


int movePartyMember(Direction dir, int member) {
    int result = 1;
    int newx, newy;
    int movementMask;

    newx = party[member]->x;
    newy = party[member]->y;
    dirMove(dir, &newx, &newy);

    screenMessage("%s\n", getDirectionName(dir));

    if (MAP_IS_OOB(c->location->map, newx, newy)) {
        mapRemoveObject(c->location->map, party[member]);
        party[member] = NULL;
        return result;
    }

    movementMask = mapGetValidMoves(c->location->map, party[member]->x, party[member]->y, c->location->z, party[member]->tile);
    if (!DIR_IN_MASK(dir, movementMask)) {
        screenMessage("Blocked!\n");
        result = 0;
        goto done;
    }

    party[member]->x = newx;
    party[member]->y = newy;

 done:

    return result;
}
