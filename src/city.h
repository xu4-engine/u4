/*
 * $Id$
 */

#ifndef CITY_H
#define CITY_H

#include "map.h"
#include "person.h"

typedef struct {
    char *name;
    int n_persons;
    Person *persons;
    Map *map;
} City;

#endif
