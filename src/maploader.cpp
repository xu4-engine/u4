/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include <string>
#include "u4.h"

#include "maploader.h"

#include "city.h"
#include "combat.h"
#include "debug.h"
#include "dungeon.h"
#include "error.h"
#include "map.h"
#include "object.h"
#include "person.h"
#include "portal.h"
#include "screen.h"
#include "tileset.h"
#include "u4file.h"
#include "utils.h"

int mapLoadCity(City *map);
int mapLoadCon(Map *map);
int mapLoadDng(Dungeon *map);
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

    case MAPTYPE_CITY:
        return mapLoadCity(dynamic_cast<City*>(map));
        break;

    case MAPTYPE_SHRINE:
    case MAPTYPE_COMBAT:
        return mapLoadCon(map);
        break;

    case MAPTYPE_DUNGEON:
        return mapLoadDng(dynamic_cast<Dungeon*>(map));
        break;
    }

    return 0;
}

/**
 * Load city data from 'ult' and 'tlk' files.
 */
int mapLoadCity(City *city) {
    U4FILE *ult, *tlk;
    unsigned char conv_idx[CITY_MAX_PERSONS];
    unsigned char c;
    unsigned int i, j;
    char tlk_buffer[288];
    Person *people[CITY_MAX_PERSONS];    

    ult = u4fopen(city->fname);
    tlk = u4fopen(city->tlk_fname);
    if (!ult || !tlk)
        errorFatal("unable to load map data");

    /* the map must be 32x32 to be read from an .ULT file */
    ASSERT(city->width == CITY_WIDTH, "map width is %d, should be %d", city->width, CITY_WIDTH);
    ASSERT(city->height == CITY_HEIGHT, "map height is %d, should be %d", city->height, CITY_HEIGHT);

    if (!mapLoadData(dynamic_cast<Map*>(city), ult))
        return 0;

    /* Properly construct people for the city */       
    for (i = 0; i < CITY_MAX_PERSONS; i++)
        people[i] = new Person(Tile::translate(u4fgetc(ult)));

    for (i = 0; i < CITY_MAX_PERSONS; i++)
        people[i]->start.x = u4fgetc(ult);

    for (i = 0; i < CITY_MAX_PERSONS; i++)
        people[i]->start.y = u4fgetc(ult);

    for (i = 0; i < CITY_MAX_PERSONS; i++)
        people[i]->setPrevTile(Tile::translate(u4fgetc(ult)));

    for (i = 0; i < CITY_MAX_PERSONS * 2; i++)
        u4fgetc(ult);           /* read redundant startx/starty */

    for (i = 0; i < CITY_MAX_PERSONS; i++) {
        c = u4fgetc(ult);
        if (c == 0)
            people[i]->setMovementBehavior(MOVEMENT_FIXED);
        else if (c == 1)
            people[i]->setMovementBehavior(MOVEMENT_WANDER);
        else if (c == 0x80)
            people[i]->setMovementBehavior(MOVEMENT_FOLLOW_AVATAR);
        else if (c == 0xFF)
            people[i]->setMovementBehavior(MOVEMENT_ATTACK_AVATAR);
        else
            return 0;        
    }

    for (i = 0; i < CITY_MAX_PERSONS; i++)
        conv_idx[i] = u4fgetc(ult);

    for (i = 0; i < CITY_MAX_PERSONS; i++) {
        people[i]->start.z = 0;        
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
            if (conv_idx[j] == i+1 || (conv_idx[j] == 0 && people[j]->getTile() == 0)) {
                char *ptr = tlk_buffer + 3;

                people[j]->questionTrigger = (PersonQuestionTrigger) tlk_buffer[0];
                people[j]->questionType = (PersonQuestionType) tlk_buffer[1];
                people[j]->turnAwayProb = tlk_buffer[2];

                people[j]->name = strdup(ptr);
                ptr += strlen(ptr) + 1;
                people[j]->pronoun = strdup(ptr);
                ptr += strlen(ptr) + 1;
                people[j]->description = strdup(ptr);
                ptr += strlen(ptr) + 1;
                people[j]->job = strdup(ptr);
                ptr += strlen(ptr) + 1;
                people[j]->health = strdup(ptr);
                ptr += strlen(ptr) + 1;
                people[j]->response1 = strdup(ptr);
                ptr += strlen(ptr) + 1;
                people[j]->response2 = strdup(ptr);
                ptr += strlen(ptr) + 1;
                people[j]->question = strdup(ptr);
                ptr += strlen(ptr) + 1;
                people[j]->yesresp = strdup(ptr);
                ptr += strlen(ptr) + 1;
                people[j]->noresp = strdup(ptr);
                ptr += strlen(ptr) + 1;
                people[j]->keyword1 = strdup(ptr);
                ptr += strlen(ptr) + 1;
                people[j]->keyword2 = strdup(ptr);

                /* trim whitespace on keywords */
                trim(&people[j]->keyword1);
                trim(&people[j]->keyword2);                
            }
        }
    }    

    /**
     * Assign roles to certain people
     */ 
    for (i = 0; i < CITY_MAX_PERSONS; i++) {
        PersonRoleList::iterator current;
        
        people[i]->npcType = NPC_EMPTY;
        if (!people[i]->name.empty())
            people[i]->npcType = NPC_TALKER;        
        if (people[i]->getTile() == 88 || people[i]->getTile() == 89)
            people[i]->npcType = NPC_TALKER_BEGGAR;
        if (people[i]->getTile() == 80 || people[i]->getTile() == 81)
            people[i]->npcType = NPC_TALKER_GUARD;
        
        for (current = city->personroles.begin(); current != city->personroles.end(); current++) {
            if ((unsigned)(*current)->id == (i + 1))
                people[i]->npcType = (PersonNpcType)(*current)->role;
        }
    }

    /**
     * Add the people to the city structure
     */ 
    for (i = 0; i < CITY_MAX_PERSONS; i++) {
        if (people[i]->getTile() != 0)            
            city->persons.push_back(people[i]);        
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
        CombatMap *cm = getCombatMap(map);

        for (i = 0; i < AREA_CREATURES; i++)
            cm->creature_start[i] = MapCoords(u4fgetc(con));        

        for (i = 0; i < AREA_CREATURES; i++)
            cm->creature_start[i].y = u4fgetc(con);

        for (i = 0; i < AREA_PLAYERS; i++)
            cm->player_start[i] = MapCoords(u4fgetc(con));

        for (i = 0; i < AREA_PLAYERS; i++)
            cm->player_start[i].y = u4fgetc(con);

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
int mapLoadDng(Dungeon *dungeon) {
    U4FILE *dng;
    unsigned int i, j;

    dng = u4fopen(dungeon->fname);
    if (!dng)
        errorFatal("unable to load map data");

    /* the map must be 11x11 to be read from an .CON file */
    ASSERT(dungeon->width == DNG_WIDTH, "map width is %d, should be %d", dungeon->width, DNG_WIDTH);
    ASSERT(dungeon->height == DNG_HEIGHT, "map height is %d, should be %d", dungeon->height, DNG_HEIGHT);

    /* load the dungeon map */
    for (i = 0; i < (DNG_HEIGHT * DNG_WIDTH * dungeon->levels); i++) {
        unsigned char mapData = u4fgetc(dng);
        MapTile tile = Tile::translate(mapData, "dungeon");
        
        /* determine what type of tile it is */
        tile.type = mapData % 16;
        dungeon->data.push_back(tile);
    }

    dungeon->room = NULL;
    /* read in the dungeon rooms */
    /* FIXME: needs a cleanup function to free this memory later */
    dungeon->rooms = new DngRoom[dungeon->n_rooms];

    for (i = 0; i < dungeon->n_rooms; i++) {
        for (j = 0; j < DNGROOM_NTRIGGERS; j++) {
            int tmp;

            dungeon->rooms[i].triggers[j].tile = Tile::translate(u4fgetc(dng)).id;

            tmp = u4fgetc(dng);
            if (tmp == EOF)
                return 0;
            dungeon->rooms[i].triggers[j].x = (tmp >> 4) & 0x0F;
            dungeon->rooms[i].triggers[j].y = tmp & 0x0F;

            tmp = u4fgetc(dng);
            if (tmp == EOF)
                return 0;
            dungeon->rooms[i].triggers[j].change_x1 = (tmp >> 4) & 0x0F;
            dungeon->rooms[i].triggers[j].change_y1 = tmp & 0x0F;
            
            tmp = u4fgetc(dng);
            if (tmp == EOF)
                return 0;
            dungeon->rooms[i].triggers[j].change_x2 = (tmp >> 4) & 0x0F;
            dungeon->rooms[i].triggers[j].change_y2 = tmp & 0x0F;
        }

        u4fread(dungeon->rooms[i].creature_tiles, sizeof(dungeon->rooms[i].creature_tiles), 1, dng);
        u4fread(dungeon->rooms[i].creature_start_x, sizeof(dungeon->rooms[i].creature_start_x), 1, dng);
        u4fread(dungeon->rooms[i].creature_start_y, sizeof(dungeon->rooms[i].creature_start_y), 1, dng);
        u4fread(dungeon->rooms[i].party_north_start_x, sizeof(dungeon->rooms[i].party_north_start_x), 1, dng);
        u4fread(dungeon->rooms[i].party_north_start_y, sizeof(dungeon->rooms[i].party_north_start_y), 1, dng);
        u4fread(dungeon->rooms[i].party_east_start_x, sizeof(dungeon->rooms[i].party_east_start_x), 1, dng);
        u4fread(dungeon->rooms[i].party_east_start_y, sizeof(dungeon->rooms[i].party_east_start_y), 1, dng);
        u4fread(dungeon->rooms[i].party_south_start_x, sizeof(dungeon->rooms[i].party_south_start_x), 1, dng);
        u4fread(dungeon->rooms[i].party_south_start_y, sizeof(dungeon->rooms[i].party_south_start_y), 1, dng);
        u4fread(dungeon->rooms[i].party_west_start_x, sizeof(dungeon->rooms[i].party_west_start_x), 1, dng);
        u4fread(dungeon->rooms[i].party_west_start_y, sizeof(dungeon->rooms[i].party_west_start_y), 1, dng);
        u4fread(dungeon->rooms[i].map_data, sizeof(dungeon->rooms[i].map_data), 1, dng);
        u4fread(dungeon->rooms[i].buffer, sizeof(dungeon->rooms[i].buffer), 1, dng);

        /* translate each creature tile to a tile id */
        for (j = 0; j < sizeof(dungeon->rooms[i].creature_tiles); j++)
            dungeon->rooms[i].creature_tiles[j] = Tile::translate(dungeon->rooms[i].creature_tiles[j]).id;

        /* translate each map tile to a tile id */
        for (j = 0; j < sizeof(dungeon->rooms[i].map_data); j++)
            dungeon->rooms[i].map_data[j] = Tile::translate(dungeon->rooms[i].map_data[j]).id;
    }
    u4fclose(dng);

    return 1;
}

int mapIsChunkCompressed(Map *map, int chunk) {
    CompressedChunkList::iterator i;    

    for (i = map->compressed_chunks.begin(); i != map->compressed_chunks.end(); i++) {
        if (chunk == *i)
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
    
    /* allocate the space we need for the map data */
    map->data.resize(map->height * map->width);

    if (map->chunk_height == 0)
        map->chunk_height = map->height;
    if (map->chunk_width == 0)
        map->chunk_width = map->width;

    for(ych = 0; ych < (map->height / map->chunk_height); ++ych) {
        for(xch = 0; xch < (map->width / map->chunk_width); ++xch) {
            for(y = 0; y < map->chunk_height; ++y) {
                for(x = 0; x < map->chunk_width; ++x) {

                    if (mapIsChunkCompressed(map, ych * map->chunk_width + xch))
                        map->data[x + (y * map->width) + (xch * map->chunk_width) + (ych * map->chunk_height * map->width)] = Tile::findByName("water")->id;

                    else {
                        int c;                        
                        c = u4fgetc(f);
                        if (c == EOF)
                            return 0;
                        
                        MapTile mt = Tile::translate(c);
                        map->data[x + (y * map->width) + (xch * map->chunk_width) + (ych * map->chunk_height * map->width)] = mt;
                    }
                }
            }
        }
    }

    return 1;
}
