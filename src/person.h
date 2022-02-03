/*
 * person.h
 */

#ifndef PERSON_H
#define PERSON_H

#include "creature.h"

typedef enum {
   NPC_EMPTY,
   NPC_TALKER,
   NPC_TALKER_BEGGAR,
   NPC_TALKER_GUARD,
   NPC_TALKER_COMPANION,
   NPC_VENDOR_WEAPONS,
   NPC_VENDOR_ARMOR,
   NPC_VENDOR_FOOD,
   NPC_VENDOR_TAVERN,
   NPC_VENDOR_REAGENTS,
   NPC_VENDOR_HEALER,
   NPC_VENDOR_INN,
   NPC_VENDOR_GUILD,
   NPC_VENDOR_STABLE,
   NPC_LORD_BRITISH,
   NPC_HAWKWIND,
   NPC_MAX
} PersonNpcType;

class Person : public Creature {
public:
    Person(const MapTile& tile);
    Person(const Person *p);

    bool isVendor() const;
    void setDiscourseId(uint16_t n);
    uint16_t discourseId() const { return convId; }
    Coords &getStart() { return start; }
    PersonNpcType getNpcType() const { return npcType; }
    void setNpcType(PersonNpcType t);

private:
    void initNpcType();

    Coords start;
    PersonNpcType npcType;
    uint16_t convId;
};

bool isPerson(const Object *punknown);

#endif
