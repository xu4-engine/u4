/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "u4.h"

#include "map.h"

#include "annotation.h"
#include "area.h"
#include "city.h"
#include "context.h"
#include "debug.h"
#include "direction.h"
#include "dungeon.h"
#include "mapmgr.h"
#include "monster.h"
#include "movement.h"
#include "object.h"
#include "person.h"
#include "portal.h"
#include "savegame.h"
#include "ttype.h"

/**
 * Loads a city/town/castle/etc. map from the 'ult' file, also using
 * the 'tlk' file to load conversations for the given town.
 */
int mapRead(City *city, U4FILE *ult, U4FILE *tlk) {
    unsigned char conv_idx[CITY_MAX_PERSONS];
    unsigned char c;
    int i, j;
    char tlk_buffer[288];

    /* the map must be 32x32 to be read from an .ULT file */
    ASSERT(city->map->width == CITY_WIDTH, "map width is %d, should be %d", city->map->width, CITY_WIDTH);
    ASSERT(city->map->height == CITY_HEIGHT, "map height is %d, should be %d", city->map->height, CITY_HEIGHT);

    city->map->data = (unsigned char *) malloc(CITY_HEIGHT * CITY_WIDTH);
    if (!city->map->data)
        return 0;

    for (i = 0; i < (CITY_HEIGHT * CITY_WIDTH); i++)
        city->map->data[i] = u4fgetc(ult);

    city->persons = (Person *) malloc(sizeof(Person) * CITY_MAX_PERSONS);
    if (!city->persons)
        return 0;
    memset(&(city->persons[0]), 0, sizeof(Person) * CITY_MAX_PERSONS);

    for (i = 0; i < CITY_MAX_PERSONS; i++)
        city->persons[i].tile0 = u4fgetc(ult);

    for (i = 0; i < CITY_MAX_PERSONS; i++)
        city->persons[i].startx = u4fgetc(ult);

    for (i = 0; i < CITY_MAX_PERSONS; i++)
        city->persons[i].starty = u4fgetc(ult);

    for (i = 0; i < CITY_MAX_PERSONS; i++)
        city->persons[i].tile1 = u4fgetc(ult);

    for (i = 0; i < CITY_MAX_PERSONS * 2; i++)
        u4fgetc(ult);           /* read redundant startx/starty */

    for (i = 0; i < CITY_MAX_PERSONS; i++) {
        c = u4fgetc(ult);
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

        city->persons[i].permanent = 1; /* permanent residents (i.e. memory is allocated here and automatically freed) */
    }

    for (i = 0; i < CITY_MAX_PERSONS; i++)
        conv_idx[i] = u4fgetc(ult);

    for (i = 0; i < CITY_MAX_PERSONS; i++) {
        city->persons[i].startz = 0;
    }

    for (i = 0; ; i++) {
        if (u4fread(tlk_buffer, 1, sizeof(tlk_buffer), tlk) != sizeof(tlk_buffer))
            break;
        for (j = 0; j < CITY_MAX_PERSONS; j++) {
            /** 
             * Match the conversation to the person;
             * sometimes we'll have a rogue entry for the .tlk file -- 
             * we'll fill in the empty spaces with this conversation 
             * (such as Isaac the Ghost in Skara Brae)
             */
            if (conv_idx[j] == i+1 || (conv_idx[j] == 0 && city->persons[j].tile0 == 0)) {
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

                /* trim whitespace on keywords */
                if (strchr(city->persons[j].keyword1, ' '))
                    *strchr(city->persons[j].keyword1, ' ') = '\0';
                if (strchr(city->persons[j].keyword2, ' '))
                    *strchr(city->persons[j].keyword2, ' ') = '\0';
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

/**
 * Loads a combat map from the 'con' file
 */
int mapReadCon(Map *map, U4FILE *con) {
    int i;

    /* the map must be 11x11 to be read from an .CON file */
    ASSERT(map->width == CON_WIDTH, "map width is %d, should be %d", map->width, CON_WIDTH);
    ASSERT(map->height == CON_HEIGHT, "map height is %d, should be %d", map->height, CON_HEIGHT);

    map->data = (unsigned char *) malloc(CON_HEIGHT * CON_WIDTH);
    if (!map->data)
        return 0;

    if (map->type != MAPTYPE_SHRINE) {
        map->area = (Area *) malloc(sizeof(Area));

        for (i = 0; i < AREA_MONSTERS; i++)
            map->area->monster_start[i].x = u4fgetc(con);

        for (i = 0; i < AREA_MONSTERS; i++)
            map->area->monster_start[i].y = u4fgetc(con);

        for (i = 0; i < AREA_PLAYERS; i++)
            map->area->player_start[i].x = u4fgetc(con);

        for (i = 0; i < AREA_PLAYERS; i++)
            map->area->player_start[i].y = u4fgetc(con);

        u4fseek(con, 16L, SEEK_CUR);
    }

    for (i = 0; i < (CON_HEIGHT * CON_WIDTH); i++)
        map->data[i] = u4fgetc(con);

    return 1;
}

/**
 * Loads a dungeon map from the 'dng' file
 */
int mapReadDng(Map *map, U4FILE *dng) {
    unsigned int i;

    /* the map must be 11x11 to be read from an .CON file */
    ASSERT(map->width == DNG_WIDTH, "map width is %d, should be %d", map->width, DNG_WIDTH);
    ASSERT(map->height == DNG_HEIGHT, "map height is %d, should be %d", map->height, DNG_HEIGHT);

    map->data = (unsigned char *) malloc(DNG_HEIGHT * DNG_WIDTH * map->levels);
    if (!map->data)
        return 0;

    for (i = 0; i < (DNG_HEIGHT * DNG_WIDTH * map->levels); i++)
        map->data[i] = u4fgetc(dng);

    map->dungeon->room = NULL;
    /* read in the dungeon rooms */
    /* FIXME: needs a cleanup function to free this memory later */
    map->dungeon->rooms = (DngRoom *)malloc(sizeof(DngRoom) * map->dungeon->n_rooms);
    u4fread(map->dungeon->rooms, sizeof(DngRoom) * map->dungeon->n_rooms, 1, dng);

    return 1;
}

/**
 * Loads the world map data in from the 'world' file.
 */
int mapReadWorld(Map *map, U4FILE *world) {
    int x, xch, y, ych;

    /* the map must be 256x256 to be read from the world map file */
    ASSERT(map->width == MAP_WIDTH, "map width is %d, should be %d", map->width, MAP_WIDTH);
    ASSERT(map->height == MAP_HEIGHT, "map height is %d, should be %d", map->height, MAP_HEIGHT);

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
                for(x = 0; x < MAP_CHUNK_WIDTH; ++x)
                    map->data[x + (y * MAP_CHUNK_WIDTH * MAP_HORIZ_CHUNKS) + (xch * MAP_CHUNK_WIDTH) + (ych * MAP_CHUNK_HEIGHT * MAP_HORIZ_CHUNKS * MAP_CHUNK_WIDTH)] = u4fgetc(world);
            }
        }
    }

    return 1;
}

/**
 * Returns the object at the given (x,y,z) coords, if one exists.
 * Otherwise, returns NULL.
 */
Object *mapObjectAt(const Map *map, int x, int y, int z) {
    Object *obj;
    Object *objAt = NULL;

    for(obj = map->objects; obj; obj = obj->next) {
        if (obj->x == x && obj->y == y && obj->z == z) {
            /* get the most visible object */
            if (objAt && (objAt->objType == OBJECT_UNKNOWN) && (obj->objType != OBJECT_UNKNOWN))
                objAt = obj;
            /* give priority to objects that have the focus */
            else if (objAt && (!objAt->hasFocus) && (obj->hasFocus))
                objAt = obj;
            else if (!objAt)
                objAt = obj;
        }            
    }
    return objAt;
}

/**
 * Returns the person object at the given (x,y,z) coords, if one exists.
 * Otherwise, returns NULL.
 */
const Person *mapPersonAt(const Map *map, int x, int y, int z) {
    Object *obj;

    obj = mapObjectAt(map, x, y, z);
    if (obj && obj->objType == OBJECT_PERSON)
        return obj->person;
    else
        return NULL;
}

/**
 * Returns the portal for the correspoding action(s) given.
 * If there is no portal that corresponds to the actions flagged
 * by 'actionFlags' at the given (x,y,z) coords, it returns NULL.
 */
const Portal *mapPortalAt(const Map *map, int x, int y, int z, int actionFlags) {
    int i;

    for(i = 0; i < map->n_portals; i++) {
        if (map->portals[i].x == x &&
            map->portals[i].y == y &&
            map->portals[i].z == z &&
            (map->portals[i].trigger_action & actionFlags)) {
            return &(map->portals[i]);
        }
    }
    return NULL;
}

/**
 * Sets the raw tile for the given (x,y,z) coords for the given map.
 * These remain changed until/unless the map is reloaded from file.
 */
void mapSetTileData(const Map *map, int x, int y, int z, unsigned char tile) {
    int index;

    index = x + (y * map->width);
    if ((short)z >= 0)
        index += (map->width * map->height * z);
    map->data[index] = tile;
}

/**
 * Returns the raw tile for the given (x,y,z) coords for the given map
 */
unsigned char mapGetTileFromData(const Map *map, int x, int y, int z) {
    int index;

    index = x + (y * map->width);
    if ((short)z >= 0)
        index += (map->width * map->height * z);
    return map->data[index];    
}

/**
 * Returns the current ground tile at the given point on a map.  Visual-only
 * annotations like moongates and attack icons are ignored.  Any walkable tiles
 * are taken into account (treasure chests, ships, balloon, etc.)
 */
unsigned char mapTileAt(const Map *map, int x, int y, int z, int withObjects) {
    unsigned char tile;
    const Annotation *a = annotationAt(x, y, z, map->id);
    Object *obj = mapObjectAt(map, x, y, z);
 
    tile = mapGetTileFromData(map, x, y, z);
    if ((a != NULL) && !a->visual)
        tile = a->tile;
    else if (withObjects && obj && tileIsWalkable(obj->tile))
        tile = obj->tile;
    
    return tile;
}

/**
 * Returns true if the given map is the world map
 */
int mapIsWorldMap(const Map *map) {
    return map->id == MAP_WORLD;
}

/**
 * Adds a person object to the given map
 */
Object *mapAddPersonObject(Map *map, const Person *person) {
    Object *obj = mapAddObject(map, person->tile0, person->tile1, person->startx, person->starty, person->startz);

    obj->movement_behavior = person->movement_behavior;
    obj->person = person;
    obj->objType = OBJECT_PERSON;

    return obj;
}

/**
 * Adds a monster object to the given map
 */
Object *mapAddMonsterObject(Map *map, const Monster *monster, int x, int y, int z) {
    Object *obj = mapAddObject(map, monster->tile, monster->tile, x, y, z);

    if (monsterWanders(monster))
        obj->movement_behavior = MOVEMENT_WANDER;
    else if (monsterIsStationary(monster))
        obj->movement_behavior = MOVEMENT_FIXED;
    else obj->movement_behavior = MOVEMENT_ATTACK_AVATAR;

    /* hide camouflaged monsters from view during combat */
    if (monsterCamouflages(monster) && (map->type == MAPTYPE_COMBAT))
        obj->isVisible = 0;

    obj->monster = monster;
    obj->objType = OBJECT_MONSTER;

    return obj;
}

/**
 * Adds an object to the given map
 */
Object *mapAddObject(Map *map, unsigned char tile, unsigned char prevtile, int x, int y, int z) {
    Object *obj = (Object *) malloc(sizeof(Object));

    obj->tile = tile;
    obj->prevtile = prevtile;
    obj->x = x;
    obj->y = y;
    obj->z = z;
    obj->prevx = x;
    obj->prevy = y;
    obj->movement_behavior = MOVEMENT_FIXED;
    obj->objType = OBJECT_UNKNOWN;
    obj->hasFocus = 0;
    obj->isVisible = 1;
    obj->canAnimate = 1;
    obj->next = map->objects;

    map->objects = obj;

    return obj;
}

/**
 * Removes the object 'rem' from the given map
 */
void mapRemoveObject(Map *map, Object *rem) {
    Object *obj = map->objects, *prev;

    prev = NULL;
    while (obj) {
        if (obj == rem) {
            if (prev)
                prev->next = obj->next;
            else
                map->objects = obj->next;
 
            /* free the memory used by a non-standard person object */
            if (obj->objType == OBJECT_PERSON && !obj->person->permanent)
                free((Person *)obj->person);            

            free(obj);
            return;
        }
        prev = obj;
        obj = obj->next;
    }
}

/**
 * Removes the given Person object from the given map
 */
void mapRemovePerson(Map *map, const Person *person) {
    Object *obj = map->objects, *prev;

    prev = NULL;
    while (obj) {
        if ((obj->objType == OBJECT_PERSON) && (obj->person == person)) {
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

/**
 * Moves all of the objects on the given map.
 * Returns an attacking object if there is a monster attacking.
 * Also performs special monster actions and monster effects.
 */
Object *mapMoveObjects(Map *map, int avatarx, int avatary, int z) {        
    Object *obj = map->objects, *attacker = NULL;        

    for (obj = map->objects; obj; obj = obj->next) {                
        
        /* check if the object is an attacking monster and not
           just a normal, docile person in town or an inanimate object */
        if ((obj->objType != OBJECT_UNKNOWN) && 
           ((obj->objType != OBJECT_MONSTER) || monsterWillAttack(obj->monster)) &&
           ((obj->objType != OBJECT_PERSON) || (obj->movement_behavior == MOVEMENT_ATTACK_AVATAR))) {
            
            if (mapMovementDistance(obj->x, obj->y, avatarx, avatary) == 1) {
                attacker = obj;
                continue;
            }
        }

        /* Perform any special actions (such as pirate ships firing cannons, sea serpents' fireblast attect, etc.) */
        monsterSpecialAction(obj);

        /* Now, move the object according to its movement behavior */
        moveObject(map, obj, avatarx, avatary);        

        /* Enact any special effects of the creature (such as storms eating objects, whirlpools teleporting, etc.) */
        monsterSpecialEffect(obj);
    }

    return attacker;
}

/**
 * Animates the objects on the given map
 */
void mapAnimateObjects(Map *map) {
    Object *obj = map->objects;

    while (obj) {
        if (obj->canAnimate && rand() % 2) {
            obj->prevtile = obj->tile;   
            tileAdvanceFrame(&obj->tile);
        }

        obj = obj->next;
    }
}

/**
 * Resets object animations to a value that is acceptable for
 * savegame compatibility with u4dos.
 */
void mapResetObjectAnimations(Map *map) {
    Object *obj = map->objects;        

    while (obj) {
        if (obj->objType == OBJECT_PERSON) {
            obj->tile = obj->person->tile0;
            obj->prevtile = obj->person->tile1;
        } else if (obj->objType == OBJECT_MONSTER) {
            /* we don't want to mess with anything but the prevtile on pirate ships */
            if (obj->monster->id != PIRATE_ID)
                obj->tile = obj->monster->tile;
            obj->prevtile = obj->monster->tile;
        }

        obj = obj->next;
    }
}

/**
 * Removes all objects from the given map
 */
void mapClearObjects(Map *map) {
    while (map->objects)
        mapRemoveObject(map, map->objects);
    map->objects = NULL;
}

/**
 * Returns the number of monsters on the given map
 */
int mapNumberOfMonsters(const Map *map) {
    Object *obj = map->objects;
    int n;

    n = 0;
    while (obj) {
        if (obj->objType == OBJECT_MONSTER)
            n++;

        obj = obj->next;
    }

    return n;
}

/**
 * Returns a mask of valid moves for the given transport on the given map
 */
int mapGetValidMoves(const Map *map, int from_x, int from_y, int z, unsigned char transport) {
    int retval;
    Direction d;
    unsigned char tile, prev_tile;
    Object *obj;
    int x, y;
    const Monster *m, *to_m;
    int ontoAvatar, ontoMonster;
    int isAvatar = (from_x == c->location->x && from_y == c->location->y && z == c->location->z);

    retval = 0;
    for (d = DIR_WEST; d <= DIR_SOUTH; d++) {
        ontoAvatar = 0;
        ontoMonster = 0;
        x = from_x;
        y = from_y;

        mapDirMove(map, d, &x, &y);
        
        /* you can always walk off the edge of the map */
        if (isAvatar && MAP_IS_OOB(map, x, y)) {
            retval = DIR_ADD_TO_MASK(d, retval);
            continue;
        }        

        obj = mapObjectAt(map, x, y, z);

        /* see if it's trying to move onto the avatar */
        if ((map->flags & SHOW_AVATAR) &&
            x == c->location->x && 
            y == c->location->y) {
            ontoAvatar = 1;
        }
        /* see if it's trying to move onto a person or monster */
        else if (obj && (obj->objType != OBJECT_UNKNOWN))                 
            ontoMonster = 1;
            
        /* get the destination tile */
        if (ontoAvatar)
            tile = (unsigned char)c->saveGame->transport;
        else if (ontoMonster)
            tile = obj->tile;
        else 
            tile = (*c->location->tileAt)(map, x, y, z, WITH_OBJECTS);        

        prev_tile = (*c->location->tileAt)(map, from_x, from_y, z, WITHOUT_OBJECTS);

        /* get the monster object, if it exists (the one that's moving) */
        m = monsterForTile(transport);
        /* get the other monster object, if it exists (the one that's being moved onto) */        
        to_m = (obj) ? monsterForTile(obj->tile) : NULL;

        /* move on if unable to move onto the avatar or another monster */
        if (m && !isAvatar) { /* some monsters/persons have the same tile as the avatar, so we have to adjust */
            if ((ontoAvatar && !monsterCanMoveOntoAvatar(m)) ||
                (ontoMonster && !monsterCanMoveOntoMonsters(m) && !monsterCanMoveOnto(obj->monster)))
                continue;
        }
        /* this really only happens with the avatar */
        else if (ontoMonster && to_m && monsterCanMoveOnto(to_m)) {
            retval = DIR_ADD_TO_MASK(d, retval);
            continue;
        }

        /* avatar movement */
        if (isAvatar) {
            /* if the transport is a ship, check sailable */
            if (tileIsShip(transport) && tileIsSailable(tile))
                retval = DIR_ADD_TO_MASK(d, retval);
            /* if it is a balloon, check flyable */
            else if (tileIsBalloon(transport) && tileIsFlyable(tile))
                retval = DIR_ADD_TO_MASK(d, retval);        
            /* avatar or horseback: check walkable */
            else if (transport == AVATAR_TILE || tileIsHorse(transport)) {
                if (tileCanWalkOn(tile, d) &&
                    tileCanWalkOff(prev_tile, d))
                    retval = DIR_ADD_TO_MASK(d, retval);
            }
        }
        
        /* monster movement */
        else if (m) {
            /* flying monsters */
            if (tileIsFlyable(tile) && monsterFlies(m)) {  
                /* FIXME: flying creatures behave differently on the world map? */
                if (mapIsWorldMap(map))
                    retval = DIR_ADD_TO_MASK(d, retval);
                else if (tileIsWalkable(tile) || tileIsSwimable(tile) || tileIsSailable(tile))
                    retval = DIR_ADD_TO_MASK(d, retval);
            }
            /* swimming monsters and sailing monsters */
            else if (tileIsSwimable(tile) || tileIsSailable(tile)) {
                if (monsterSwims(m) && tileIsSwimable(tile))
                    retval = DIR_ADD_TO_MASK(d, retval);
                if (monsterSails(m) && tileIsSailable(tile))
                    retval = DIR_ADD_TO_MASK(d, retval);
            }
            /* ghosts and other incorporeal monsters */
            else if (monsterIsIncorporeal(m)) {
                /* can move anywhere but onto water, unless of course the monster can swim */
                if (!(tileIsSwimable(tile) || tileIsSailable(tile)))
                    retval = DIR_ADD_TO_MASK(d, retval);
            }
            /* walking monsters */
            else if (monsterWalks(m)) {
                if (tileCanWalkOn(tile, d) &&
                    tileCanWalkOff(prev_tile, d) &&
                    tileIsMonsterWalkable(tile))
                    retval = DIR_ADD_TO_MASK(d, retval);
            }            
        } 
    }

    return retval;
}

/**
 * Find the distance from point a to point b,
 * allowing diagonal movements to calculate
 **/
int mapDistance(int x1, int y1, int x2, int y2) {
    int dist, lowx, highx, lowy, highy;

    dist = 0;
    lowx = (x1 < x2) ? x1 : x2;
    lowy = (y1 < y2) ? y1 : y2;
    highx = (x1 > x2) ? x1 : x2;
    highy = (y1 > y2) ? y1 : y2;

    while ((lowx < highx) || (lowy < highy))
    {
        if (lowx < highx) lowx++;
        if (lowy < highy) lowy++;
        dist++;
    }

    return dist;
}

/**
 * Find the number of moves it would take to get
 * from point a to point b (no diagonals allowed)
 */
int mapMovementDistance(int x1, int y1, int x2, int y2) {
    int dx = abs(x1 - x2),
        dy = abs(y1 - y2);

    return (dx + dy);
}

/**
 * Moves x and y in 'dir' direction on the map, wrapping if necessary
 * Returns 1 if succeeded, 0 if map doesn't wrap and x or y was moved
 * beyond the borders of the map
 */
int mapDirMove(const Map *map, Direction dir, int *x, int *y) {
    int newx = *x,
        newy = *y,
        wraps = map->border_behavior == BORDER_WRAP;

    dirMove(dir, &newx, &newy);
    if (MAP_IS_OOB(map, newx, newy) && wraps)        
        mapWrapCoordinates(map, &newx, &newy);    

    *x = newx;
    *y = newy;

    return 1;
}

/**
 * Wraps x,y coordinates on a map if necessary and possible
 * Returns 1 if succeeded, 0 if not needed or not possible
 */
int mapWrapCoordinates(const Map *map, int *x, int *y) {
    if (map->border_behavior == BORDER_WRAP) {
        if (*x < 0) *x += map->width;
        if (*x >= (int)map->width) *x -= map->width;
        if (*y < 0) *y += map->height;
        if (*y >= (int)map->height) *y -= map->height;
        return 1;
    }
    return 0;
}
