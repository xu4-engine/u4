/*
 * $Id$
 */

#ifndef CITY_H
#define CITY_H

struct _Person;
struct _Map;

typedef struct _PersonRole {
    int role;
    int id;
} PersonRole;

typedef struct _City {
    char *name;
    int n_persons;
    struct _Person *persons;
    char *tlk_fname;
    int n_personroles;
    PersonRole *personroles;
    struct _Map *map;
} City;

#endif
