/*
 * $Id$
 */

#ifndef COMBAT_H
#define COMBAT_H

struct _Map;
struct _Object;
struct _Monster;

typedef enum {
    CA_ATTACK,
    CA_CAST_SLEEP,
    CA_ADVANCE,
    CA_RANGED,
    CA_FLEE,
    CA_TELEPORT
} CombatAction;

void combatBegin(struct _Map *map, struct _Object *monster, int isNormal);
void combatFinishTurn(void);
void combatCreateMonster(int index, int canbeleader);
int combatBaseKeyHandler(int key, void *data);
int combatInitialNumberOfMonsters(const struct _Monster *monster);
struct _Map *getCombatMapForTile(unsigned char partytile, unsigned short transport, const struct _Monster *monster);
void attackFlash(int x, int y, int tile, int timeFactor);

#endif
