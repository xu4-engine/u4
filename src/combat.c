/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "combat.h"
#include "context.h"
#include "ttype.h"
#include "map.h"
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
int saved_dngx, saved_dngy;
int focus;
Object *party[8];
Object *monsters[AREA_MONSTERS];
int monsterHp[AREA_MONSTERS];

int combatBaseKeyHandler(int key, void *data);
void combatFinishTurn(void);
int combatAttackAtCoord(int x, int y);
int combatInitialNumberOfMonsters(unsigned char monster);
int combatIsWon(void);
int combatIsLost(void);
void combatEnd(void);
void combatMoveMonsters(void);
int combatFindTargetForMonster(const Object *monster, int *distance);
int movePartyMember(Direction dir, int member);

void combatBegin(unsigned char partytile, unsigned short transport, Object *monster) {
    int i;
    int nmonsters;
    int mtile;

    monsterObj = monster;

    annotationClear();

    c = gameCloneContext(c);

    saved_dngx = c->saveGame->dngx;
    saved_dngy = c->saveGame->dngy;
    gameSetMap(c, getCombatMapForTile(partytile, transport), 1);
    musicPlay();

    for (i = 0; i < c->saveGame->members; i++)
        party[i] = mapAddObject(c->map, tileForClass(c->saveGame->players[i].klass), tileForClass(c->saveGame->players[i].klass), c->map->area->player_start[i].x, c->map->area->player_start[i].y);
    for (; i < 8; i++)
        party[i] = NULL;
    focus = 0;
    party[focus]->hasFocus = 1;

    nmonsters = combatInitialNumberOfMonsters(monster->tile);
    for (i = 0; i < nmonsters; i++) {
        mtile = monster->tile;
        if (tileIsPirateShip(mtile))
            mtile = ROGUE_TILE;
        monsters[i] = mapAddObject(c->map, mtile, mtile, c->map->area->monster_start[i].x, c->map->area->monster_start[i].y);
        monsterHp[i] = monsterGetInitialHp(monsterForTile(monsters[i]->tile));
    }
    for (; i < AREA_MONSTERS; i++)
        monsters[i] = NULL;

    eventHandlerPushKeyHandler(&combatBaseKeyHandler);

    screenMessage("\n**** COMBAT ****\n\n");

    screenMessage("%s with %s\n\020", c->saveGame->players[focus].name, getWeaponName(c->saveGame->players[focus].weapon));
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
        { CHEST_TILE,   &brick_map },
        { BRICKFLOOR_TILE, &brick_map },
        { MOONGATE0_TILE, &grass_map },
        { MOONGATE1_TILE, &grass_map },
        { MOONGATE2_TILE, &grass_map },
        { MOONGATE3_TILE, &grass_map }
    };
    
    /* check if monster is aquatic */
    if (monsterForTile(monsterObj->tile)->mattr & MATTR_WATER) {
        if (tileIsPirateShip(monsterObj->tile)) {
            if (tileIsShip(transport))
                return &shipship_map;
            else
                return &shorship_map;
        }

        if (tileIsShip(transport))
            return &shipsea_map;
        else
            return &shore_map;
    }

    if (tileIsShip(transport))
        return &shipshor_map;

    for (i = 0; i < sizeof(tileToMap) / sizeof(tileToMap[0]); i++) {
        if (tileToMap[i].tile == partytile)
            return tileToMap[i].map;
    }

    return &brick_map;
}

void combatFinishTurn() {
    if (combatIsWon()) {

        eventHandlerPopKeyHandler();
        combatEnd();
        return;
    }
    if (party[focus])
        party[focus]->hasFocus = 0;
    do {
        /* put the focus on the next party member */
        focus++;

        /* move monsters and wrap around at end */
        if (focus >= c->saveGame->members) {

            combatMoveMonsters();

            focus = 0;
            if (combatIsLost()) {
                if (!playerPartyDead(c->saveGame))
                    playerAdjustKarma(c->saveGame, KA_FLED);
                eventHandlerPopKeyHandler();
                combatEnd();
                if (playerPartyDead(c->saveGame))
                    deathStart();
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

    screenMessage("%s with %s\n\020", c->saveGame->players[focus].name, getWeaponName(c->saveGame->players[focus].weapon));
}

int combatBaseKeyHandler(int key, void *data) {
    int valid = 1;
    CoordActionInfo *info;

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
        info->range = 1;
        info->blockedPredicate = NULL;
        info->failedMessage = "FIXME";
        eventHandlerPushKeyHandlerData(&gameGetCoordinateKeyHandler, info);
        screenMessage("Dir: ");
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

int combatAttackAtCoord(int x, int y) {
    int monster;
    const Monster *m;
    int i, xp;

    if (x == -1 && y == -1)
        return 0;

    monster = -1;
    for (i = 0; i < AREA_MONSTERS; i++) {
        if (monsters[i] &&
            monsters[i]->x == x &&
            monsters[i]->y == y)
            monster = i;
    }

    if (monster == -1 || !playerAttackHit(&c->saveGame->players[focus])) {
        screenMessage("Missed!\n");

        annotationSetTimeDuration(annotationAdd(x, y, MISSFLASH_TILE), 2);

    } else {
        m = monsterForTile(monsters[monster]->tile);

        annotationSetTimeDuration(annotationAdd(x, y, HITFLASH_TILE), 2);

        if (m->tile != LORDBRITISH_TILE)
            monsterHp[monster] -= playerGetDamage(&c->saveGame->players[focus]);

        switch (monsterGetStatus(m, monsterHp[monster])) {

        case MSTAT_DEAD:
            xp = monsterGetXp(m);
            screenMessage("%s Killed!\nExp. %d\n", m->name, xp);
            c->saveGame->players[focus].xp += xp;
            if (monsterIsEvil(m))
                gameLostEighth(playerAdjustKarma(c->saveGame, KA_KILLED_EVIL));
            mapRemoveObject(c->map, monsters[monster]);
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

int combatInitialNumberOfMonsters(unsigned char monster) {
    if (monster != GUARD_TILE &&
        !mapIsWorldMap(c->parent->map))
        return 1;

    return (rand() % (AREA_MONSTERS - 1)) + 1;
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
    if (c->parent != NULL) {
        Context *t = c;
        annotationClear();
        mapClearObjects(c->map);
        c->parent->saveGame->x = c->saveGame->dngx;
        c->parent->saveGame->y = c->saveGame->dngy;
        c->parent->saveGame->dngx = saved_dngx;
        c->parent->saveGame->dngy = saved_dngy;
        c->parent->line = c->line;
        c->parent->moonPhase = c->moonPhase;
        c->parent->windDirection = c->windDirection;
        c->parent->windCounter = c->windCounter;
        c->parent->aura = c->aura;
        c->parent->auraDuration = c->auraDuration;
        c->parent->horseSpeed = c->horseSpeed;
        c = c->parent;
        c->col = 0;
        free(t);
                
        musicPlay();
    }
    
    if (combatIsWon()) {
        if ((monsterForTile(monsterObj->tile)->mattr & MATTR_WATER) == 0)
            mapAddObject(c->map, CHEST_TILE, CHEST_TILE, monsterObj->x, monsterObj->y);
        else if (tileIsPirateShip(monsterObj->tile)) {
            unsigned short ship = 16;
            tileSetDirection(&ship, tileGetDirection(monsterObj->tile));
            mapAddObject(c->map, ship, ship, monsterObj->x, monsterObj->y);
        }
    }   
    mapRemoveObject(c->map, monsterObj);
    
    if (!playerPartyDead(c->saveGame))
        gameFinishTurn();
}

void combatMoveMonsters() {
    int i, newx, newy, valid_dirs, target, distance;
    CombatAction action;
    const Monster *m;
    int slow;

    for (i = 0; i < AREA_MONSTERS; i++) {
        if (!monsters[i])
            continue;
        m = monsterForTile(monsters[i]->tile);

        target = combatFindTargetForMonster(monsters[i], &distance);
        if (target == -1)
            continue;

        if (monsterCastSleep(m))
            action = CA_CAST_SLEEP;
        else if (distance == 1)
            action = CA_ATTACK;
        else
            action = CA_ADVANCE;

        switch(action) {
        case CA_ATTACK:
            if (playerIsHitByAttack(&c->saveGame->players[target])) {
                playerApplyDamage(&c->saveGame->players[target], monsterGetDamage(m));
                if (c->saveGame->players[target].hp == 0) {
                    mapRemoveObject(c->map, party[target]);
                    party[target] = NULL;
                }
                statsUpdate();
            }
            break;

        case CA_CAST_SLEEP:
            screenMessage("Sleep!\n");
            playerApplySleepSpell(c->saveGame);
            statsUpdate();
            break;

        case CA_FLEE:
            /* FIXME */
            break;

        case CA_ADVANCE:
            newx = monsters[i]->x;
            newy = monsters[i]->y;
            valid_dirs = mapGetValidMoves(c->map, newx, newy, monsters[i]->tile);
            dirMove(dirFindPath(newx, newy, party[target]->x, party[target]->y, valid_dirs), &newx, &newy);

            switch (tileGetSpeed(mapTileAt(c->map, newx, newy))) {
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
            }
            break;
        }
    }
}

int combatFindTargetForMonster(const Object *monster, int *distance) {
    int i, dx, dy;
    int closest;

    *distance = 1000;
    closest = -1;
    for (i = 0; i < c->saveGame->members; i++) {
        if (!party[i])
            continue;

        dx = abs(monster->x - party[i]->x);
        dx *= dx;
        dy = abs(monster->y - party[i]->y);
        dy *= dy;

        if (dx + dy < (*distance) ||
            (dx + dy == (*distance) && (rand() % 2) == 0)) {
            (*distance) = dx + dy;
            closest = i;
        }
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

    if (MAP_IS_OOB(c->map, newx, newy)) {
        mapRemoveObject(c->map, party[member]);
        party[member] = NULL;
        return result;
    }

    movementMask = mapGetValidMoves(c->map, party[member]->x, party[member]->y, party[member]->tile);
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
