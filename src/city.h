/*
 * $Id$
 */

#ifndef CITY_H
#define CITY_H

struct _Person;
struct _Map;

typedef enum {
    CITY_TOWN,
    CITY_VILLAGE,
    CITY_CASTLE
} CityType;

typedef struct _City {
    const char *name;
    CityType type;
    int n_persons;
    struct _Person *persons;
    const unsigned char person_types[12];
    const char *tlk_fname;
    struct _Map *map;
} City;

#endif
