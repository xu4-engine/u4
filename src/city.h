/*
 * $Id$
 */

#ifndef CITY_H
#define CITY_H

#include "types.h"

struct _Map;

typedef struct _PersonRole {
    int role;
    int id;
} PersonRole;

typedef xu4_vector<struct _Person *> PersonList;
typedef xu4_list<PersonRole*> PersonRoleList;

typedef struct _City {
    string name;
    /*int n_persons;
    struct _Person *persons;*/
    PersonList persons;
    string tlk_fname;
    PersonRoleList personroles;
    /*int n_personroles;
    PersonRole *personroles;*/
    struct _Map *map;
} City;

#endif
