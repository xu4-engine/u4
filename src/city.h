/*
 * $Id$
 */

#ifndef CITY_H
#define CITY_H

struct _Person;
struct _Map;

typedef struct _City {
    char *name;
    int n_persons;
    struct _Person *persons;
    unsigned char person_types[12];
    char *tlk_fname;
    struct _Map *map;
} City;

#endif
