/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "u4.h"

#include "map.h"

#include "area.h"
#include "city.h"
#include "debug.h"
#include "dungeon.h"
#include "error.h"
#include "object.h"
#include "person.h"
#include "portal.h"
#include "u4file.h"

int mapLoadCity(Map *map);
int mapLoadCon(Map *map);
int mapLoadDng(Map *map);
int mapLoadWorld(Map *map);
int mapLoadData(Map *map, U4FILE *f);

/**
 * Load map data from into map object.  The metadata in the map must
 * already be set.
 */
int mapLoad(Map *map) {
    switch (map->type) {
    case MAPTYPE_WORLD:
        return mapLoadWorld(map);
        break;

    case MAPTYPE_TOWN:
    case MAPTYPE_VILLAGE:
    case MAPTYPE_CASTLE:
    case MAPTYPE_RUIN:
        return mapLoadCity(map);
        break;

    case MAPTYPE_SHRINE:
    case MAPTYPE_COMBAT:
        return mapLoadCon(map);
        break;

    case MAPTYPE_DUNGEON:
        return mapLoadDng(map);
        break;
    }

    return 0;
}

/**
 * Load city data from 'ult' and 'tlk' files.
 */
int mapLoadCity(Map *map) {
    U4FILE *ult, *tlk;
    unsigned char conv_idx[CITY_MAX_PERSONS];
    unsigned char c;
    unsigned int i, j;
    char tlk_buffer[288];

    ult = u4fopen(map->fname);
    tlk = u4fopen(map->city->tlk_fname);
    if (!ult || !tlk)
        errorFatal("unable to load map data");

    /* the map must be 32x32 to be read from an .ULT file */
    ASSERT(map->width == CITY_WIDTH, "map width is %d, should be %d", map->width, CITY_WIDTH);
    ASSERT(map->height == CITY_HEIGHT, "map height is %d, should be %d", map->height, CITY_HEIGHT);

    if (!mapLoadData(map, ult))
        return 0;

    map->city->persons = (Person *) malloc(sizeof(Person) * CITY_MAX_PERSONS);
    if (!map->city->persons)
        return 0;
    memset(&(map->city->persons[0]), 0, sizeof(Person) * CITY_MAX_PERSONS);

    for (i = 0; i < CITY_MAX_PERSONS; i++)
        map->city->persons[i].tile0 = u4fgetc(ult);

    for (i = 0; i < CITY_MAX_PERSONS; i++)
        map->city->persons[i].startx = u4fgetc(ult);

    for (i = 0; i < CITY_MAX_PERSONS; i++)
        map->city->persons[i].starty = u4fgetc(ult);

    for (i = 0; i < CITY_MAX_PERSONS; i++)
        map->city->persons[i].tile1 = u4fgetc(ult);

    for (i = 0; i < CITY_MAX_PERSONS * 2; i++)
        u4fgetc(ult);           /* read redundant startx/starty */

    for (i = 0; i < CITY_MAX_PERSONS; i++) {
        c = u4fgetc(ult);
        if (c == 0)
            map->city->persons[i].movement_behavior = MOVEMENT_FIXED;
        else if (c == 1)
            map->city->persons[i].movement_behavior = MOVEMENT_WANDER;
        else if (c == 0x80)
            map->city->persons[i].movement_behavior = MOVEMENT_FOLLOW_AVATAR;
        else if (c == 0xFF)
            map->city->persons[i].movement_behavior = MOVEMENT_ATTACK_AVATAR;
        else
            return 0;

        map->city->persons[i].permanent = 1; /* permanent residents (i.e. memory is allocated here and automatically freed) */
    }

    for (i = 0; i < CITY_MAX_PERSONS; i++)
        conv_idx[i] = u4fgetc(ult);

    for (i = 0; i < CITY_MAX_PERSONS; i++) {
        map->city->persons[i].startz = 0;
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
            if (conv_idx[j] == i+1 || (conv_idx[j] == 0 && map->city->persons[j].tile0 == 0)) {
                char *ptr = tlk_buffer + 3;

                map->city->persons[j].questionTrigger = (PersonQuestionTrigger) tlk_buffer[0];
                map->city->persons[j].questionType = (PersonQuestionType) tlk_buffer[1];
                map->city->persons[j].turnAwayProb = tlk_buffer[2];

                map->city->persons[j].name = strdup(ptr);
                ptr += strlen(ptr) + 1;
                map->city->persons[j].pronoun = strdup(ptr);
                ptr += strlen(ptr) + 1;
                map->city->persons[j].description = strdup(ptr);
                ptr += strlen(ptr) + 1;
                map->city->persons[j].job = strdup(ptr);
                ptr += strlen(ptr) + 1;
                map->city->persons[j].health = strdup(ptr);
                ptr += strlen(ptr) + 1;
                map->city->persons[j].response1 = strdup(ptr);
                ptr += strlen(ptr) + 1;
                map->city->persons[j].response2 = strdup(ptr);
                ptr += strlen(ptr) + 1;
                map->city->persons[j].question = strdup(ptr);
                ptr += strlen(ptr) + 1;
                map->city->persons[j].yesresp = strdup(ptr);
                ptr += strlen(ptr) + 1;
                map->city->persons[j].noresp = strdup(ptr);
                ptr += strlen(ptr) + 1;
                map->city->persons[j].keyword1 = strdup(ptr);
                ptr += strlen(ptr) + 1;
                map->city->persons[j].keyword2 = strdup(ptr);

                /* trim whitespace on keywords */
                if (strchr(map->city->persons[j].keyword1, ' '))
                    *strchr(map->city->persons[j].keyword1, ' ') = '\0';
                if (strchr(map->city->persons[j].keyword2, ' '))
                    *strchr(map->city->persons[j].keyword2, ' ') = '\0';
            }
        }
    }

    map->city->n_persons = CITY_MAX_PERSONS;

    for (i = 0; i < CITY_MAX_PERSONS; i++) {
        map->city->persons[i].npcType = NPC_EMPTY;
        if (map->city->persons[i].name)
            map->city->persons[i].npcType = NPC_TALKER;
        if (map->city->persons[i].tile0 == 88 || map->city->persons[i].tile0 == 89)
            map->city->persons[i].npcType = NPC_TALKER_BEGGAR;
        if (map->city->persons[i].tile0 == 80 || map->city->persons[i].tile0 == 81)
            map->city->persons[i].npcType = NPC_TALKER_GUARD;
        for (j = 0; j < map->city->n_personroles; j++) {
            if (map->city->personroles[j].id == (i + 1))
                map->city->persons[i].npcType = map->city->personroles[j].role;
        }
    }

    u4fclose(ult);
    u4fclose(tlk);

    return 1;
}

/**
 * Loads a combat map from the 'con' file
 */
int mapLoadCon(Map *map) {
    U4FILE *con;
    int i;

    con = u4fopen(map->fname);
    if (!con)
        errorFatal("unable to load map data");

    /* the map must be 11x11 to be read from an .CON file */
    ASSERT(map->width == CON_WIDTH, "map width is %d, should be %d", map->width, CON_WIDTH);
    ASSERT(map->height == CON_HEIGHT, "map height is %d, should be %d", map->height, CON_HEIGHT);

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

    if (!mapLoadData(map, con))
        return 0;

    u4fclose(con);

    return 1;
}

/**
 * Loads a dungeon map from the 'dng' file
 */
int mapLoadDng(Map *map) {
    U4FILE *dng;
    unsigned int i;

    dng = u4fopen(map->fname);
    if (!dng)
        errorFatal("unable to load map data");

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

    u4fclose(dng);

    return 1;
}

int mapIsChunkCompressed(Map *map, int chunk) {
    int i;

    for (i = 0; i < map->n_compressed_chunks; i++) {
        if (chunk == map->compressed_chunks[i])
            return 1;
    }
    return 0;
}

/**
 * Loads the world map data in from the 'world' file.
 */
int mapLoadWorld(Map *map) {
    U4FILE *world;

    world = u4fopen(map->fname);
    if (!world)
        errorFatal("unable to load map data");

    if (!mapLoadData(map, world))
        return 0;

     u4fclose(world);

     return 1;
}

int mapLoadData(Map *map, U4FILE *f) {
    unsigned int x, xch, y, ych;

    map->data = (unsigned char *) malloc(map->height * map->width);
    if (!map->data)
        return 0;

    if (map->chunk_height == 0)
        map->chunk_height = map->height;
    if (map->chunk_width == 0)
        map->chunk_width = map->width;

    for(ych = 0; ych < (map->height / map->chunk_height); ++ych) {
        for(xch = 0; xch < (map->width / map->chunk_width); ++xch) {
            for(y = 0; y < map->chunk_height; ++y) {
                for(x = 0; x < map->chunk_width; ++x) {

                    if (mapIsChunkCompressed(map, ych * map->chunk_width + xch))
                        map->data[x + (y * map->width) + (xch * map->chunk_width) + (ych * map->chunk_height * map->width)] = 1;

                    else {
                        int c;
                        c = u4fgetc(f);
                        if (c == EOF)
                            return 0;

                        map->data[x + (y * map->width) + (xch * map->chunk_width) + (ych * map->chunk_height * map->width)] = c;
                    }
                }
            }
        }
    }

    return 1;
}
