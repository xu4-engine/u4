/*
 * $Id$
 */

#ifndef COMBAT_H
#define COMBAT_H

struct _Map;

void combatBegin(unsigned char partytile, unsigned short transport, unsigned char monster);
struct _Map *getCombatMapForTile(unsigned char partytile, unsigned short transport);

#endif
