/*
 * $Id$
 */

#ifndef COMBAT_H
#define COMBAT_H

struct _Map;

typedef enum {
    CA_ATTACK,
    CA_CAST_SLEEP,
    CA_ADVANCE,
    CA_FLEE
} CombatAction;

void combatBegin(unsigned char partytile, unsigned short transport, unsigned char monster);
struct _Map *getCombatMapForTile(unsigned char partytile, unsigned short transport);

#endif
