/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "u4.h"
#include "map.h"
#include "io.h"
#include "annotation.h"

#define MAP_TILE_AT(mapptr, x, y) ((mapptr)->data[(x) + ((y) * (mapptr)->width)])

int mapRead(Map *map, FILE *ult, FILE *tlk) {
    char conv_idx[CITY_MAX_PERSONS];
    unsigned char c;
    int i, j;
    char tlk_buffer[288];

    /* the map must be 32x32 to be read from an .ULT file */
    assert(map->width == CITY_WIDTH);
    assert(map->height == CITY_HEIGHT);

    map->data = (unsigned char *) malloc(CITY_HEIGHT * CITY_WIDTH);
    if (!map->data)
        return 0;

    for (i = 0; i < (CITY_HEIGHT * CITY_WIDTH); i++) {
        if (!readChar(&(map->data[i]), ult))
            return 0;
    }

    map->persons = (Person *) malloc(sizeof(Person) * CITY_MAX_PERSONS);
    if (!map->persons)
        return 0;
    memset(&(map->persons[0]), 0, sizeof(Person) * CITY_MAX_PERSONS);

    for (i = 0; i < CITY_MAX_PERSONS; i++) {
        if (!readChar(&c, ult))
            return 0;
        map->persons[i].tile0 = c;
    }

    for (i = 0; i < CITY_MAX_PERSONS; i++) {
        if (!readChar(&c, ult))
            return 0;
        map->persons[i].startx = c;
    }

    for (i = 0; i < CITY_MAX_PERSONS; i++) {
        if (!readChar(&c, ult))
            return 0;
        map->persons[i].starty = c;
    }

    for (i = 0; i < CITY_MAX_PERSONS; i++) {
        if (!readChar(&c, ult))
            return 0;
        map->persons[i].tile1 = c;
    }

    for (i = 0; i < CITY_MAX_PERSONS * 2; i++) {
        if (!readChar(&c, ult)) /* read redundant startx/starty */
            return 0;
    }

    for (i = 0; i < CITY_MAX_PERSONS; i++) {
        if (!readChar(&c, ult))
            return 0;
        if (c == 0)
            map->persons[i].movement_behavior = MOVEMENT_FIXED;
        else if (c == 1)
            map->persons[i].movement_behavior = MOVEMENT_WANDER;
        else if (c == 0x80)
            map->persons[i].movement_behavior = MOVEMENT_FOLLOW_AVATAR;
        else if (c == 0xFF)
            map->persons[i].movement_behavior = MOVEMENT_ATTACK_AVATAR;
        else 
            return 0;
    }

    for (i = 0; i < CITY_MAX_PERSONS; i++) {
        if (!readChar(&(conv_idx[i]), ult))
            return 0;
    }

    for (i = 0; ; i++) {
        if (fread(tlk_buffer, 1, sizeof(tlk_buffer), tlk) != sizeof(tlk_buffer))
            break;
        for (j = 0; j < CITY_MAX_PERSONS; j++) {
            if (conv_idx[j] == i+1) {
                char *ptr = tlk_buffer + 3;

                map->persons[j].questionTrigger = tlk_buffer[0];
                map->persons[j].questionType = tlk_buffer[1];
                map->persons[j].turnAwayProb = tlk_buffer[2];

                map->persons[j].name = strdup(ptr);
                ptr += strlen(ptr) + 1;
                map->persons[j].pronoun = strdup(ptr);
                ptr += strlen(ptr) + 1;
                map->persons[j].description = strdup(ptr);
                ptr += strlen(ptr) + 1;
                map->persons[j].job = strdup(ptr);
                ptr += strlen(ptr) + 1;
                map->persons[j].health = strdup(ptr);
                ptr += strlen(ptr) + 1;
                map->persons[j].response1 = strdup(ptr);
                ptr += strlen(ptr) + 1;
                map->persons[j].response2 = strdup(ptr);
                ptr += strlen(ptr) + 1;
                map->persons[j].question = strdup(ptr);
                ptr += strlen(ptr) + 1;
                map->persons[j].yesresp = strdup(ptr);
                ptr += strlen(ptr) + 1;
                map->persons[j].noresp = strdup(ptr);
                ptr += strlen(ptr) + 1;
                map->persons[j].keyword1 = strdup(ptr);
                ptr += strlen(ptr) + 1;
                map->persons[j].keyword2 = strdup(ptr);
            }
        }
    }

    map->n_persons = CITY_MAX_PERSONS;

    return 1;
}

int mapReadCon(Map *map, FILE *con, int header) {
    int i;

    /* the map must be 11x11 to be read from an .CON file */
    assert(map->width == CON_WIDTH);
    assert(map->height == CON_HEIGHT);

    map->data = (unsigned char *) malloc(CON_HEIGHT * CON_WIDTH);
    if (!map->data)
        return 0;

    if (header)
        fseek(con, 64L, SEEK_SET);

    for (i = 0; i < (CON_HEIGHT * CON_WIDTH); i++) {
        if (!readChar(&(map->data[i]), con))
            return 0;
    }

    return 1;
}

int mapReadWorld(Map *map, FILE *world) {
    int x, xch, y, ych, i;

    /* the map must be 256x256 to be read from the world map file */
    assert(map->width == MAP_WIDTH);
    assert(map->height == MAP_WIDTH);

    map->data = (unsigned char *) malloc(MAP_HEIGHT * MAP_WIDTH);
    if (!map->data)
        return 0;

    xch = 0;
    ych = 0;
    x = 0;
    y = 0;
    for (i = 0; i < (MAP_VERT_CHUNKS * MAP_CHUNK_HEIGHT * MAP_HORIZ_CHUNKS * MAP_CHUNK_WIDTH); i++) {
        readChar(&(map->data[x + (y * MAP_CHUNK_WIDTH * MAP_HORIZ_CHUNKS) + (xch * MAP_CHUNK_WIDTH) + (ych * MAP_CHUNK_HEIGHT * MAP_HORIZ_CHUNKS * MAP_CHUNK_WIDTH)]), world);
        x++;
        if (x >= MAP_CHUNK_WIDTH) {
            x = 0;
            y++;
            if (y >= MAP_CHUNK_HEIGHT) {
                y = 0;
                xch++;
                if (xch >= MAP_HORIZ_CHUNKS) {
                    xch = 0;
                    ych++;
                }
            }
        }
    }

    return 1;
}

const Person *mapPersonAt(const Map *map, int x, int y) {
    int i;

    for(i = 0; i < map->n_persons; i++) {
        if (map->persons[i].tile0 != 0 &&
            map->persons[i].startx == x &&
            map->persons[i].starty == y) {
            return &(map->persons[i]);
        }
    }
    return NULL;
}

const Portal *mapPortalAt(const Map *map, int x, int y) {
    int i;

    for(i = 0; i < map->n_portals; i++) {
        if (map->portals[i].x == x &&
            map->portals[i].y == y) {
            return &(map->portals[i]);
        }
    }
    return NULL;
}

unsigned char mapTileAt(const Map *map, int x, int y) {
    unsigned char tile;
    const Annotation *a;
 
    tile = MAP_TILE_AT(map, x, y);
    if ((a = annotationAt(x, y)) != NULL)
        tile = a->tile;
    
    return tile;
}
