/*
 * $Id$
 */

#ifndef COMBAT_H
#define COMBAT_H

struct _Map;
struct _Object;

typedef enum {
    CA_ATTACK,
    CA_CAST_SLEEP,
    CA_ADVANCE,
    CA_RANGED,
    CA_FLEE
} CombatAction;

void combatBegin(unsigned char partytile, unsigned short transport, struct _Object *monster);
void combatFinishTurn(void);
struct _Map *getCombatMapForTile(unsigned char partytile, unsigned short transport);
void attackFlash(int x, int y, int tile, int timeFactor);

#endif
