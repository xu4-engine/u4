/*
 * $Id$
 */

#ifndef CITY_H
#define CITY_H

struct _Person;
struct _Map;

typedef struct _City {
    const char *name;
    int n_persons;
    struct _Person *persons;
    const unsigned char person_types[13];
    const char *tlk_fname;
    struct _Map *map;
} City;

#endif
