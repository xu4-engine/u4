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

int focus;
Object *party[8];
Object *monsters[AREA_MONSTERS];

int combatBaseKeyHandler(int key, void *data);
void combatEndTurn(void);
int combatAttackAtCoord(int x, int y);
int combatIsWon(void);
int combatIsLost(void);
void combatEnd(void);
void combatMoveMonsters();
int movePartyMember(Direction dir, int member);

void combatBegin(unsigned char partytile, unsigned short transport, unsigned char monster) {
    int i;
    int nmonsters;

    annotationClear();

    c = gameCloneContext(c);

    gameSetMap(c, getCombatMapForTile(partytile, transport), 1);
    musicPlay();

    for (i = 0; i < c->saveGame->members; i++)
        party[i] = mapAddObject(c->map, tileForClass(c->saveGame->players[i].klass), tileForClass(c->saveGame->players[i].klass), c->map->area->player_start[i].x, c->map->area->player_start[i].y);
    for (; i < 8; i++)
        party[i] = NULL;
    focus = 0;
    party[focus]->hasFocus = 1;

    nmonsters = (rand() % (AREA_MONSTERS - 1)) + 1;
    for (i = 0; i < nmonsters; i++)
        monsters[i] = mapAddObject(c->map, monster, monster, c->map->area->monster_start[i].x, c->map->area->monster_start[i].y);
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
        { SWAMP_TILE, &marsh_map },
        { GRASS_TILE, &grass_map },
        { BRUSH_TILE, &brush_map },
        { FOREST_TILE, &forest_map },
        { HILLS_TILE, &hill_map },
        { BRIDGE_TILE, &bridge_map },
        { NORTHBRIDGE_TILE, &bridge_map },
        { SOUTHBRIDGE_TILE, &bridge_map },
        { CHEST_TILE, &brick_map },
        { BRICKFLOOR_TILE, &brick_map },
        { MOONGATE0_TILE, &grass_map },
        { MOONGATE1_TILE, &grass_map },
        { MOONGATE2_TILE, &grass_map },
        { MOONGATE3_TILE, &grass_map }
    };
    
    for (i = 0; i < sizeof(tileToMap) / sizeof(tileToMap[0]); i++) {
        if (tileToMap[i].tile == partytile)
            return tileToMap[i].map;
    }

    return &brick_map;
}

void combatEndTurn() {
    if (combatIsWon()) {

        eventHandlerPopKeyHandler();
        combatEnd();
        return;
    }
    else if (combatIsLost()) {
        /* FIXME: assume for now the party fled, not died */
        playerAdjustKarma(c->saveGame, KA_FLED);

        eventHandlerPopKeyHandler();
        combatEnd();
        return;
    }

    if (party[focus])
        party[focus]->hasFocus = 0;
    do {
        focus++;
        if (focus >= c->saveGame->members) {
            combatMoveMonsters();
            focus = 0;
        }
    } while (!party[focus]);
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

    default:
        valid = 0;
        break;
    }

    if (valid) {
        if (eventHandlerGetKeyHandler() == &combatBaseKeyHandler)
            combatEndTurn();
    }

    return valid;
}

int combatAttackAtCoord(int x, int y) {
    int monster;
    const Monster *m;
    int i;

    if (x == -1 && y == -1)
        return 0;

    monster = -1;
    for (i = 0; i < AREA_MONSTERS; i++) {
        if (monsters[i] &&
            monsters[i]->x == x &&
            monsters[i]->y == y)
            monster = i;
    }

    if (monster == -1) {
        screenMessage("Missed!\n");
    } else {
        m = monsterForTile(monsters[monster]->tile);
        screenMessage("Hit!\n");

        /* FIXME: every hit is fatal for now */
        
        screenMessage("%s Killed!\nExp. %d\n", m->name, m->xp);
        c->saveGame->players[focus].xp += m->xp;
        if (monsterIsEvil(m))
            gameLostEighth(playerAdjustKarma(c->saveGame, KA_KILLED_EVIL));
        mapRemoveObject(c->map, monsters[monster]);
        monsters[monster] = NULL;
    }

    combatEndTurn();

    return 1;
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
        c->parent->line = c->line;
        c->parent->moonPhase = c->moonPhase;
        c->parent->windDirection = c->windDirection;
        c->parent->windCounter = c->windCounter;
        c = c->parent;
        c->col = 0;
        free(t);
                
        musicPlay();
    }
    
    gameFinishTurn();
}

void combatMoveMonsters() {
    int i, newx, newy, valid_dirs;
    unsigned char tile;
    Object *target, *other;

    i = 0;
    do {
        target = party[i++];
    } while (!target);

    for (i = 0; i < AREA_MONSTERS; i++) {
        if (!monsters[i])
            continue;

        newx = monsters[i]->x;
        newy = monsters[i]->y;
        valid_dirs = mapGetValidMoves(c->map, newx, newy, AVATAR_TILE);
        dirMove(dirFindPath(newx, newy, target->x, target->y, valid_dirs), &newx, &newy);

        if (newx >= 0 && newx < c->map->width &&
            newy >= 0 && newy < c->map->height) {
            if ((other = mapObjectAt(c->map, newx, newy)) != NULL)
                tile = other->tile;
            else
                tile = mapTileAt(c->map, newx, newy);
            if (tileIsWalkable(tile)) {
                if (newx != monsters[i]->x ||
                    newy != monsters[i]->y) {
                    monsters[i]->prevx = monsters[i]->x;
                    monsters[i]->prevy = monsters[i]->y;
                }
                monsters[i]->x = newx;
                monsters[i]->y = newy;
            }
        }
    }
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
