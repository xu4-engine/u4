/*
 * $Id$
 */

#ifndef CITY_H
#define CITY_H

#include "map.h"

typedef struct _PersonRole {
    int role;
    int id;
} PersonRole;

typedef xu4_vector<Person *> PersonList;
typedef xu4_list<PersonRole*> PersonRoleList;

class City : public Map {
public:
    City() : Map() {}

    // Members
    virtual string getName();
    class Person *addPerson(class Person *p);
    void addPeople();
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
