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
#include "area.h"
#include "context.h"
#include "savegame.h"
#include "monster.h"

#define MAP_TILE_AT(mapptr, x, y) ((mapptr)->data[(x) + ((y) * (mapptr)->width)])

int mapRead(City *city, FILE *ult, FILE *tlk) {
    unsigned char conv_idx[CITY_MAX_PERSONS];
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

                city->persons[j].questionTrigger = (PersonQuestionTrigger) tlk_buffer[0];
                city->persons[j].questionType = (PersonQuestionType) tlk_buffer[1];
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
                city->persons[i].npcType = (PersonNpcType) (j + NPC_TALKER_COMPANION);
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

    if (map->type != MAP_SHRINE) {
        map->area = (Area *) malloc(sizeof(Area));
        for (i = 0; i < AREA_MONSTERS; i++) {
            if (!readChar(&(map->area->monster_start[i].x), con))
                return 0;
        }
        for (i = 0; i < AREA_MONSTERS; i++) {
            if (!readChar(&(map->area->monster_start[i].y), con))
                return 0;
        }
        for (i = 0; i < AREA_PLAYERS; i++) {
            if (!readChar(&(map->area->player_start[i].x), con))
                return 0;
        }
        for (i = 0; i < AREA_PLAYERS; i++) {
            if (!readChar(&(map->area->player_start[i].y), con))
                return 0;
        }
        fseek(con, 16L, SEEK_CUR);
    }

    for (i = 0; i < (CON_HEIGHT * CON_WIDTH); i++) {
        if (!readChar(&(map->data[i]), con))
            return 0;
    }

    return 1;
}

int mapReadWorld(Map *map, FILE *world) {
    int x, xch, y, ych;

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

    for(ych = 0; ych < MAP_VERT_CHUNKS; ++ych) {
        for(xch = 0; xch < MAP_HORIZ_CHUNKS; ++xch) {
            for(y = 0; y < MAP_CHUNK_HEIGHT; ++y) {
                for(x = 0; x < MAP_CHUNK_WIDTH; ++x) {
                    readChar(&(map->data[x + (y * MAP_CHUNK_WIDTH * MAP_HORIZ_CHUNKS) + (xch * MAP_CHUNK_WIDTH) + (ych * MAP_CHUNK_HEIGHT * MAP_HORIZ_CHUNKS * MAP_CHUNK_WIDTH)]), world);
                }
            }
        }
    }

    return 1;
}

Object *mapObjectAt(const Map *map, int x, int y) {
    Object *obj;

    for(obj = map->objects; obj; obj = obj->next) {
        if (obj->x == x && obj->y == y)
            return obj;
    }
    return NULL;
}

const Person *mapPersonAt(const Map *map, int x, int y) {
    Object *obj;

    obj = mapObjectAt(map, x, y);
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
    return map->id == 0;
}

Object *mapAddPersonObject(Map *map, const Person *person) {
    Object *obj = (Object *) malloc(sizeof(Object));

    obj->tile = person->tile0;
    obj->prevtile = person->tile1;
    obj->x = person->startx;
    obj->y = person->starty;
    obj->movement_behavior = person->movement_behavior;
    obj->person = person;
    obj->hasFocus = 0;
    obj->next = map->objects;

    map->objects = obj;

    return obj;
}

Object *mapAddObject(Map *map, unsigned int tile, unsigned int prevtile, unsigned short x, unsigned short y) {
    Object *obj = (Object *) malloc(sizeof(Object));

    obj->tile = tile;
    obj->prevtile = prevtile;
    obj->x = x;
    obj->y = y;
    obj->prevx = x;
    obj->prevy = y;
    obj->movement_behavior = MOVEMENT_FIXED;
    obj->person = NULL;
    obj->hasFocus = 0;
    obj->next = map->objects;

    map->objects = obj;

    return obj;
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
    int newx, newy;
    unsigned char tile;
    int slow;
    Object *obj = map->objects, *other;
    const Monster *m;

    for (obj = map->objects; obj; obj = obj->next) {
        newx = obj->x;
        newy = obj->y;

        m = monsterForTile(obj->tile);

        switch (obj->movement_behavior) {
        case MOVEMENT_FIXED:
            break;

        case MOVEMENT_WANDER:
            if (rand() % 2 == 0)
                dirMove(dirRandomDir(mapGetValidMoves(map, newx, newy, obj->tile)), &newx, &newy);
            break;
                
        case MOVEMENT_ATTACK_AVATAR:
        case MOVEMENT_FOLLOW_AVATAR:
            dirMove(dirFindPath(newx, newy, avatarx, avatary, mapGetValidMoves(map, newx, newy, obj->tile)), &newx, &newy);
            break;
        }

        switch (tileGetSpeed(mapTileAt(map, newx, newy))) {
        case FAST:
            slow = 0;
            break;
        case SLOW:
            slow = (rand() % 8) == 0;
            break;
        case VSLOW:
            slow = (rand() % 4) == 0;
            break;
        case VVSLOW:
            slow = (rand() % 2) == 0;
            break;
        }
        if (slow)
            continue;

        if ((newx != obj->x || newy != obj->y) &&
            newx >= 0 && newx < map->width &&
            newy >= 0 && newy < map->height) {

            if (c->saveGame->x == newx && c->saveGame->y == newy)
                tile = c->saveGame->transport;
            else if ((other = mapObjectAt(map, newx, newy)) != NULL)
                tile = other->tile;
            else
                tile = mapTileAt(map, newx, newy);

            if (m && (m->mattr & MATTR_WATER) != 0) {
                if (!tileIsSailable(tile))
                    continue;
            }

            else if (!tileIsWalkable(tile))
                continue;

            if (newx != obj->x ||
                newy != obj->y) {
                obj->prevx = obj->x;
                obj->prevy = obj->y;
            }
            obj->x = newx;
            obj->y = newy;
        }
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

int mapGetValidMoves(const Map *map, int from_x, int from_y, unsigned char transport) {
    int retval;
    Direction d;
    unsigned char tile;
    Object *obj;
    int x, y;
    const Monster *m;

    retval = 0;
    for (d = DIR_WEST; d <= DIR_SOUTH; d++) {
        x = from_x;
        y = from_y;

        dirMove(d, &x, &y);

        if (MAP_IS_OOB(map, x, y)) {
            if (map->border_behavior == BORDER_WRAP) {
                if (x < 0)
                    x += map->width;
                if (y < 0)
                    y += map->height;
                if (x >= (int) map->width)
                    x -= map->width;
                if (y >= (int) map->height)
                    y -= map->height;
            }
            else {
                retval = DIR_ADD_TO_MASK(d, retval);
                continue;
            }
        }

        if ((map->flags & SHOW_AVATAR) &&
            x == c->saveGame->x && 
            y == c->saveGame->y)
            tile = c->saveGame->transport;
        else if ((obj = mapObjectAt(map, x, y)) != NULL)
            tile = obj->tile;
        else
            tile = mapTileAt(map, x, y);

        /* 
         * check special cases for tile 0x0e: the center tile of the
         * castle of lord british, which is walkable from the south
         * only
         */
        if (tile == 0x0e && d == DIR_SOUTH)
            continue;
        if (mapTileAt(map, from_x, from_y) == 0x0e && d == DIR_NORTH)
            continue;

        /* if the transport is a ship, check sailable */
        if (tileIsShip(transport) || tileIsPirateShip(transport)) {
            if (tileIsSailable(tile))
                retval = DIR_ADD_TO_MASK(d, retval);
        }
        /* aquatic monster */
        else if ((m = monsterForTile(transport)) && (m->mattr & MATTR_WATER)) {
            if (tileIsSwimable(tile))
                retval = DIR_ADD_TO_MASK(d, retval);
        }
        /* if it is a balloon, check flyable */
        else if (tileIsBalloon(transport)) {
            if (tileIsFlyable(tile))
                retval = DIR_ADD_TO_MASK(d, retval);
        }
        /* otherwise check walkable */
        else if (tileIsWalkable(tile))
            retval = DIR_ADD_TO_MASK(d, retval);
    }

    return retval;
}
