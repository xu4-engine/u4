/*
 * person.cpp
 */

#include "person.h"

/**
 * Returns true of the object that 'punknown' points
 * to is a person object
 */
bool isPerson(const Object *punknown) {
    const Person *p;
    if ((p = dynamic_cast<const Person*>(punknown)) != NULL)
        return true;
    else
        return false;
}

Person::Person(const MapTile& tile) :
    Creature(Creature::getByTile(tile)),
    start(0, 0)
{
    objType = Object::PERSON;
    npcType = NPC_EMPTY;
    convId = 0xffff;
}

Person::Person(const Person *p) {
    *this = *p;
}

bool Person::isVendor() const {
    return npcType >= NPC_VENDOR_WEAPONS &&
           npcType <= NPC_VENDOR_STABLE;
}

void Person::initNpcType() {
    Symbol tname = tile.getTileType()->name;
    if (tname == Tile::sym.beggar)
        npcType = NPC_TALKER_BEGGAR;
    else if (tname == Tile::sym.guard)
        npcType = NPC_TALKER_GUARD;
    else
        npcType = NPC_TALKER;
}

void Person::setDiscourseId(uint16_t n) {
    convId = n;
    initNpcType();
}

void Person::setNpcType(PersonNpcType t) {
    npcType = t;
}
