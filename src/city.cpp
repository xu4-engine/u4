/*
 * city.cpp
 */

#include "city.h"

#include "config.h"
#include "context.h"
#include "party.h"
#include "xu4.h"


City::City() {
    discourse_init(&disc);
}

City::~City() {
    for (PersonList::iterator i = persons.begin(); i != persons.end(); i++)
        delete *i;

    discourse_free(&disc);
}

/**
 * Returns the name of the city
 */
const char* City::getName() const {
    return xu4.config->confString(name);
}

const char* City::cityTypeStr() const {
    return xu4.config->symbolName(cityType);
}

/**
 * Adds a person object to the map
 */
Person *City::addPerson(Person *person) {
    // Make a copy of the person before adding them, so
    // things like angering the guards, etc. will be
    // forgotten the next time you visit :)
    Person *p = new Person(person);

    /* set the start coordinates for the person */
    p->placeOnMap(this, p->getStart());

    objects.push_back(p);
    return p;
}

/**
 * Add people to the map
 */
void City::addPeople() {
    // Make sure the city has no people in it already
    removeAllPeople();

    PersonList::iterator it;
    for (it = persons.begin(); it != persons.end(); it++) {
        Person *p = *it;
        if ((p->tile != 0) &&
            ! (p->getNpcType() == NPC_TALKER_COMPANION &&
               c->party->isPersonJoined(p->getName())))
            addPerson(p);
    }
}

/**
 * Removes all people from the current map
 */
void City::removeAllPeople() {
    ObjectDeque::iterator obj;
    for (obj = objects.begin(); obj != objects.end();) {
        if (isPerson(*obj))
            obj = removeObject(obj);
        else obj++;
    }
}

/**
 * Returns the person object at the given (x,y,z) coords, if one exists.
 * Otherwise, returns NULL.
 */
Person *City::personAt(const Coords &coords) {
    Object *obj;

    obj = objectAt(coords);
    if (isPerson(obj))
        return dynamic_cast<Person*>(obj);
    else
        return NULL;
}
