/*
 * $Id$
 */

#ifndef CITY_H
#define CITY_H

#include <list>
#include <string>
#include <vector>

using std::string;

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

    // Members
    virtual string getName();
    class Person *addPerson(class Person *p);
    void addPeople();
    void removeAllPeople();
    const class Person *personAt(MapCoords coords);    

    // Properties
    string name;
    string type;
    PersonList persons;
    string tlk_fname;
    PersonRoleList personroles;    
};

bool isCity(Map *punknown);

#endif
