#ifndef ARMOR_H
#define ARMOR_H

#define MAX_ARMORS     128

typedef struct _Armor {
    const char *name;    
    const char *canwear;
    const char *cantwear;
    int defense;
    unsigned short mask;
} Armor;

char *armorGetName(int weapon);
int armorGetDefense(int armor);
int armorCanWear(int armor, const char *className);

#endif
