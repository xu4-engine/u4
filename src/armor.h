/*
 * $Id$
 */

#ifndef ARMOR_H
#define ARMOR_H

#include "savegame.h"

#define MAX_ARMORS     128

typedef struct _Armor {
    const char *name;    
    unsigned char canuse;
    int defense;
    unsigned short mask;
} Armor;

char *armorGetName(int weapon);
int armorGetDefense(int armor);
int armorCanWear(int armor, ClassType klass);

#endif
