/*
 * city.h
 */

#ifndef CITY_H
#define CITY_H

#include "map.h"
#include "discourse.h"

struct PersonRole {
    int16_t role;   // PersonNpcType
    int16_t id;
};

class Person;

typedef std::vector<Person *> PersonList;

class City : public Map {
public:
    City();
    ~City();

    // Members
    virtual const char* getName() const;
    const char* cityTypeStr() const;
    Person *addPerson(Person *p);
    void addPeople();
    void removeAllPeople();
    Person *personAt(const Coords &coords);

    // Properties
    StringId name;
    StringId tlk_fname;
    Symbol cityType;
    PersonList persons;
    std::vector<PersonRole> personroles;
    Discourse disc;
};

#endif
