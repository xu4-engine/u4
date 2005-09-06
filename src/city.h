/*
 * $Id$
 */

#ifndef CITY_H
#define CITY_H

#include <list>
#include <string>
#include <vector>

using std::string;

class Person;

#include "map.h"

struct PersonRole {
    int role;
    int id;
};

typedef std::vector<Person *> PersonList;
typedef std::list<PersonRole*> PersonRoleList;

class City : public Map {
public:
    City() : Map() {}
    ~City();

    // Members
    virtual string getName();
    Person *addPerson(Person *p);
    void addPeople();
    void removeAllPeople();
    Person *personAt(const Coords &coords);

    // Properties
    string name;
    string type;
    PersonList persons;
    string tlk_fname;
    PersonRoleList personroles;    
};

bool isCity(Map *punknown);

#endif
