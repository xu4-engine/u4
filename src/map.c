/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "u4.h"
#include "map.h"
#include "direction.h"
#include "city.h"
#include "portal.h"
#include "object.h"
#include "person.h"
#include "io.h"
#include "annotation.h"
#include "ttype.h"

#define MAP_TILE_AT(mapptr, x, y) ((mapptr)->data[(x) + ((y) * (mapptr)->width)])

int mapRead(City *city, FILE *ult, FILE *tlk) {
    char conv_idx[CITY_MAX_PERSONS];
    unsigned char c;
    int i, j;
    char tlk_buffer[288];

    /* the map must be 32x32 to be read from an .ULT file */
    assert(city->map->width == CITY_WIDTH);
    assert(city->map->height == CITY_HEIGHT);

    city->map->data = (unsigned char *) malloc(CITY_HEIGHT * CITY_WIDTH);
    if (!city->map->data)
        return 0;

    for (i = 0; i < (CITY_HEIGHT * CITY_WIDTH); i++) {
        if (!readChar(&(city->map->data[i]), ult))
            return 0;
    }

    city->persons = (Person *) malloc(sizeof(Person) * CITY_MAX_PERSONS);
    if (!city->persons)
        return 0;
    memset(&(city->persons[0]), 0, sizeof(Person) * CITY_MAX_PERSONS);

    for (i = 0; i < CITY_MAX_PERSONS; i++) {
        if (!readChar(&city->persons[i].tile0, ult))
            return 0;
    }

    for (i = 0; i < CITY_MAX_PERSONS; i++) {
        if (!readChar(&c, ult))
            return 0;
        city->persons[i].startx = c;
    }

    for (i = 0; i < CITY_MAX_PERSONS; i++) {
        if (!readChar(&c, ult))
            return 0;
        city->persons[i].starty = c;
    }

    for (i = 0; i < CITY_MAX_PERSONS; i++) {
        if (!readChar(&city->persons[i].tile1, ult))
            return 0;
    }

    for (i = 0; i < CITY_MAX_PERSONS * 2; i++) {
        if (!readChar(&c, ult)) /* read redundant startx/starty */
            return 0;
    }

    for (i = 0; i < CITY_MAX_PERSONS; i++) {
        if (!readChar(&c, ult))
            return 0;
        if (c == 0)
            city->persons[i].movement_behavior = MOVEMENT_FIXED;
        else if (c == 1)
            city->persons[i].movement_behavior = MOVEMENT_WANDER;
        else if (c == 0x80)
            city->persons[i].movement_behavior = MOVEMENT_FOLLOW_AVATAR;
        else if (c == 0xFF)
            city->persons[i].movement_behavior = MOVEMENT_ATTACK_AVATAR;
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

                city->persons[j].questionTrigger = tlk_buffer[0];
                city->persons[j].questionType = tlk_buffer[1];
                city->persons[j].turnAwayProb = tlk_buffer[2];

                city->persons[j].name = strdup(ptr);
                ptr += strlen(ptr) + 1;
                city->persons[j].pronoun = strdup(ptr);
                ptr += strlen(ptr) + 1;
                city->persons[j].description = strdup(ptr);
                ptr += strlen(ptr) + 1;
                city->persons[j].job = strdup(ptr);
                ptr += strlen(ptr) + 1;
                city->persons[j].health = strdup(ptr);
                ptr += strlen(ptr) + 1;
                city->persons[j].response1 = strdup(ptr);
                ptr += strlen(ptr) + 1;
                city->persons[j].response2 = strdup(ptr);
                ptr += strlen(ptr) + 1;
                city->persons[j].question = strdup(ptr);
                ptr += strlen(ptr) + 1;
                city->persons[j].yesresp = strdup(ptr);
                ptr += strlen(ptr) + 1;
                city->persons[j].noresp = strdup(ptr);
                ptr += strlen(ptr) + 1;
                city->persons[j].keyword1 = strdup(ptr);
                ptr += strlen(ptr) + 1;
                city->persons[j].keyword2 = strdup(ptr);
            }
        }
    }

    city->n_persons = CITY_MAX_PERSONS;
 
    for (i = 0; i < CITY_MAX_PERSONS; i++) {
        city->persons[i].npcType = NPC_EMPTY;
        if (city->persons[i].name)
            city->persons[i].npcType = NPC_TALKER;
        if (city->persons[i].tile0 == 88 || city->persons[i].tile0 == 89)
            city->persons[i].npcType = NPC_TALKER_BEGGAR;
        if (city->persons[i].tile0 == 80 || city->persons[i].tile0 == 81)
            city->persons[i].npcType = NPC_TALKER_GUARD;
        for (j = 0; j < 12; j++) {
            if (city->person_types[j] == (i + 1))
                city->persons[i].npcType = j + NPC_TALKER_COMPANION;
        }
    }

    return 1;
}

int mapReadCon(Map *map, FILE *con) {
    int i;

    /* the map must be 11x11 to be read from an .CON file */
    assert(map->width == CON_WIDTH);
    assert(map->height == CON_HEIGHT);

    map->data = (unsigned char *) malloc(CON_HEIGHT * CON_WIDTH);
    if (!map->data)
        return 0;

    if (map->type != MAP_SHRINE)
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

Object *mapObjectAt(const Map *map, int x, int y, int ignoreAvatar) {
    Object *obj;

    for(obj = map->objects; obj; obj = obj->next) {
        if (obj->x == x && obj->y == y &&
            (!ignoreAvatar || !obj->isAvatar))
            return obj;
    }
    return NULL;
}

const Person *mapPersonAt(const Map *map, int x, int y) {
    Object *obj;

    obj = mapObjectAt(map, x, y, 1);
    if (obj)
        return obj->person;
    else
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

int mapIsWorldMap(const Map *map) {
    return 
        map->width == MAP_WIDTH &&
        map->height == MAP_HEIGHT;
}

void mapAddPersonObject(Map *map, const Person *person) {
    Object *obj = (Object *) malloc(sizeof(Object));

    obj->tile = person->tile0;
    obj->prevtile = person->tile1;
    obj->x = person->startx;
    obj->y = person->starty;
    obj->movement_behavior = person->movement_behavior;
    obj->person = person;
    obj->isAvatar = 0;
    obj->next = map->objects;

    map->objects = obj;
}

void mapAddObject(Map *map, unsigned int tile, unsigned int prevtile, unsigned short x, unsigned short y) {
    Object *obj = (Object *) malloc(sizeof(Object));

    obj->tile = tile;
    obj->prevtile = prevtile;
    obj->x = x;
    obj->y = y;
    obj->prevx = x;
    obj->prevy = y;
    obj->movement_behavior = MOVEMENT_FIXED;
    obj->person = NULL;
    obj->isAvatar = 0;
    obj->next = map->objects;

    map->objects = obj;
}

void mapAddAvatarObject(Map *map, unsigned int tile, unsigned short x, unsigned short y) {
    Object *obj = (Object *) malloc(sizeof(Object));

    obj->tile = tile;
    obj->prevtile = tile;
    obj->x = x;
    obj->y = y;
    obj->prevx = x;
    obj->prevy = y;
    obj->movement_behavior = MOVEMENT_FIXED;
    obj->person = NULL;
    obj->isAvatar = 1;
    obj->next = map->objects;

    map->objects = obj;
}

void mapRemoveObject(Map *map, Object *rem) {
    Object *obj = map->objects, *prev;

    prev = NULL;
    while (obj) {
        if (obj == rem) {
            if (prev)
                prev->next = obj->next;
            else
                map->objects = obj->next;
            free(obj);
            return;
        }
        prev = obj;
        obj = obj->next;
    }
}

void mapRemoveAvatarObject(Map *map) {
    Object *obj = map->objects, *prev;

    prev = NULL;
    while (obj) {
        if (obj->isAvatar) {
            if (prev)
                prev->next = obj->next;
            else
                map->objects = obj->next;
            free(obj);
            return;
        }
        prev = obj;
        obj = obj->next;
    }
}

void mapRemovePerson(Map *map, const Person *person) {
    Object *obj = map->objects, *prev;

    prev = NULL;
    while (obj) {
        if (obj->person == person) {
            if (prev)
                prev->next = obj->next;
            else
                map->objects = obj->next;
            free(obj);
            return;
        }
        prev = obj;
        obj = obj->next;
    }
}

void mapMoveObjects(Map *map, int avatarx, int avatary) {
    int dx, dy, newx, newy;
    unsigned char tile;
    Object *obj = map->objects, *other;

    while (obj) {
        newx = obj->x;
        newy = obj->y;
        switch (obj->movement_behavior) {
        case MOVEMENT_FIXED:
            break;

        case MOVEMENT_WANDER:
            if (rand() % 2 == 0)
                dirMove(rand() % 4, &newx, &newy);
            break;
                
        case MOVEMENT_ATTACK_AVATAR:
        case MOVEMENT_FOLLOW_AVATAR:
            if (rand() % 2) {
                if (newx > avatarx)
                    dx = -1;
                else
                    dx = 1;
                if (newy > avatary)
                    dy = -1;
                else
                    dy = 1;
                if (newx != avatarx &&
                    newy != avatary) {
                    switch(rand() % 2) {
                    case 0:
                        newx += dx;
                        break;
                    case 1:
                        newy += dy;
                        break;
                    }
                } else if (newx != avatarx) {
                    newx += dx;
                } else if (obj->y != avatary) {
                    newy += dy;
                }
            }
            break;
        }

        if ((newx != obj->x || newy != obj->y) &&
            newx >= 0 && newx < map->width &&
            newy >= 0 && newy < map->height) {
            if ((other = mapObjectAt(map, newx, newy, 0)) != NULL)
                tile = obj->tile;
            else
                tile = mapTileAt(map, newx, newy);
            if (tileIsWalkable(tile)) {
                if (newx != obj->x ||
                    newy != obj->y) {
                    obj->prevx = obj->x;
                    obj->prevy = obj->y;
                }
                obj->x = newx;
                obj->y = newy;
            }
        }

        obj = obj->next;
    }
}

void mapAnimateObjects(Map *map) {
    Object *obj = map->objects;

    while (obj) {
        if (rand() % 2) {
            obj->prevtile = obj->tile;
            tileAdvanceFrame(&obj->tile);
        }

        obj = obj->next;
    }
}

void mapClearObjects(Map *map) {
    Object *obj = map->objects, *tmp;

    while (obj) {
        tmp = obj->next;
        free(obj);
        obj = tmp;
    }

    map->objects = NULL;
}
