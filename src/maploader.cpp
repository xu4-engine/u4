/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include <ctime>
#include <string>
#include "u4.h"

#include "maploader.h"

#include "city.h"
#include "combat.h"
#include "conversation.h"
#include "dialogueloader.h"
#include "debug.h"
#include "dungeon.h"
#include "error.h"
#include "filesystem.h"
#include "map.h"
#include "maploader.h"
#include "mapmgr.h"
#include "object.h"
#include "person.h"
#include "portal.h"
#include "tilemap.h"
#include "tileset.h"
#include "u4file.h"
#include "utils.h"

std::map<Map::Type, MapLoader *> *MapLoader::loaderMap = NULL;

MapLoader *CityMapLoader::instance = MapLoader::registerLoader(new CityMapLoader, Map::CITY);
MapLoader *ConMapLoader::instance = MapLoader::registerLoader(MapLoader::registerLoader(new ConMapLoader, Map::COMBAT), Map::SHRINE);
MapLoader *DngMapLoader::instance = MapLoader::registerLoader(new DngMapLoader, Map::DUNGEON);
MapLoader *WorldMapLoader::instance = MapLoader::registerLoader(new WorldMapLoader, Map::WORLD);

MapLoader *MapLoader::getLoader(Map::Type type) {
    ASSERT(loaderMap != NULL, "loaderMap not initialized");
    if (loaderMap->find(type) == loaderMap->end())
        return NULL;
    return (*loaderMap)[type];
}

MapLoader *MapLoader::registerLoader(MapLoader *loader, Map::Type type) {
    if (loaderMap == NULL) {
        loaderMap = new std::map<Map::Type, MapLoader *>;
    }
    (*loaderMap)[type] = loader;
    return loader;
}

int MapLoader::loadData(Map *map, U4FILE *f) {
    unsigned int x, xch, y, ych;

    TileIndexMap* tileMap = TileMap::get("base");
    
    /* allocate the space we need for the map data */
    map->data.resize(map->height * map->width);

    if (map->chunk_height == 0)
        map->chunk_height = map->height;
    if (map->chunk_width == 0)
        map->chunk_width = map->width;

    clock_t total = 0;
    clock_t start = clock();
    for(ych = 0; ych < (map->height / map->chunk_height); ++ych) {
        for(xch = 0; xch < (map->width / map->chunk_width); ++xch) {
            if (isChunkCompressed(map, ych * map->chunk_width + xch)) {
                for(y = 0; y < map->chunk_height; ++y) {
                    for(x = 0; x < map->chunk_width; ++x) {
                        static int water = Tileset::findTileByName("water")->id;
                        map->data[x + (y * map->width) + (xch * map->chunk_width) + (ych * map->chunk_height * map->width)] = water;
                    }
                }
            }
            else {
                for(y = 0; y < map->chunk_height; ++y) {
                    for(x = 0; x < map->chunk_width; ++x) {                    
                        int c = u4fgetc(f);
                        if (c == EOF)
                            return 0;
                      
                        clock_t s = clock();
                        MapTile mt = (*tileMap)[c];
                        total += clock() - s;

                        map->data[x + (y * map->width) + (xch * map->chunk_width) + (ych * map->chunk_height * map->width)] = mt;
                    }
                }
            }
        }
    }
    clock_t end = clock();
    
    FILE *file = FileSystem::openFile("debug/mapLoadData.txt", "wt");
    if (file) {
        fprintf(file, "%d msecs total\n%d msecs used by Tile::translate()", int(end - start), int(total));
        fclose(file);
    }

    return 1;
}

int MapLoader::isChunkCompressed(Map *map, int chunk) {
    CompressedChunkList::iterator i;    

    for (i = map->compressed_chunks.begin(); i != map->compressed_chunks.end(); i++) {
        if (chunk == *i)
            return 1;
    }
    return 0;
}

/**
 * Load city data from 'ult' and 'tlk' files.
 */
int CityMapLoader::load(Map *map) {
    City *city = dynamic_cast<City*>(map);

    unsigned int i, j;
    Person *people[CITY_MAX_PERSONS];    
    Dialogue *dialogues[CITY_MAX_PERSONS];
    DialogueLoader *dlgLoader = DialogueLoader::getLoader("application/x-u4tlk");

    U4FILE *ult = u4fopen(city->fname);
    U4FILE *tlk = u4fopen(city->tlk_fname);
    if (!ult || !tlk)
        errorFatal("unable to load map data");

    /* the map must be 32x32 to be read from an .ULT file */
    ASSERT(city->width == CITY_WIDTH, "map width is %d, should be %d", city->width, CITY_WIDTH);
    ASSERT(city->height == CITY_HEIGHT, "map height is %d, should be %d", city->height, CITY_HEIGHT);

    if (!loadData(city, ult))
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
        unsigned char c = u4fgetc(ult);
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

    unsigned char conv_idx[CITY_MAX_PERSONS];
    for (i = 0; i < CITY_MAX_PERSONS; i++) {
        conv_idx[i] = u4fgetc(ult);
        people[i]->dialogue = NULL;
    }

    for (i = 0; i < CITY_MAX_PERSONS; i++) {
        people[i]->start.z = 0;        
    }

    for (i = 0; i < CITY_MAX_PERSONS; i++) {
        dialogues[i] = dlgLoader->load(tlk);        
        
        /*
         * Match up dialogues with their respective people
         */
        for (j = 0; j < CITY_MAX_PERSONS; j++) {
            if (conv_idx[j] == i+1) {
                people[j]->dialogue = dialogues[i];
                people[j]->name = dialogues[i]->getName();
            }
        }
    }    

    /*
     * Assign roles to certain people
     */ 
    for (i = 0; i < CITY_MAX_PERSONS; i++) {
        PersonRoleList::iterator current;
        Tileset *tileset = Tileset::get();
        
        people[i]->npcType = NPC_EMPTY;
        if (!people[i]->name.empty())
            people[i]->npcType = NPC_TALKER;        
        /* FIXME: this type of thing should be done in xml */
        if (people[i]->getTile() == tileset->findTileByName("beggar")->id)
            people[i]->npcType = NPC_TALKER_BEGGAR;
        if (people[i]->getTile() == tileset->findTileByName("guard")->id)
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
int ConMapLoader::load(Map *map) {
    int i;

    U4FILE *con = u4fopen(map->fname);
    if (!con)
        errorFatal("unable to load map data");

    /* the map must be 11x11 to be read from an .CON file */
    ASSERT(map->width == CON_WIDTH, "map width is %d, should be %d", map->width, CON_WIDTH);
    ASSERT(map->height == CON_HEIGHT, "map height is %d, should be %d", map->height, CON_HEIGHT);

    if (map->type != Map::SHRINE) {
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

    if (!loadData(map, con))
        return 0;

    u4fclose(con);

    return 1;
}

/**
 * Loads a dungeon map from the 'dng' file
 */
int DngMapLoader::load(Map *map) {
    Dungeon *dungeon = dynamic_cast<Dungeon*>(map);    

    U4FILE *dng = u4fopen(dungeon->fname);
    if (!dng)
        errorFatal("unable to load map data");

    /* the map must be 11x11 to be read from an .CON file */
    ASSERT(dungeon->width == DNG_WIDTH, "map width is %d, should be %d", dungeon->width, DNG_WIDTH);
    ASSERT(dungeon->height == DNG_HEIGHT, "map height is %d, should be %d", dungeon->height, DNG_HEIGHT);

    /* load the dungeon map */
    unsigned int i, j;
    for (i = 0; i < (DNG_HEIGHT * DNG_WIDTH * dungeon->levels); i++) {
        unsigned char mapData = u4fgetc(dng);
        MapTile tile = Tile::translate(mapData, "dungeon");
        
        /* determine what type of tile it is */
        tile.type = mapData % 16;
        dungeon->data.push_back(tile);
    }

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

    dungeon->roomMaps = new CombatMap *[dungeon->n_rooms];
    for (i = 0; i < dungeon->n_rooms; i++)
        initDungeonRoom(dungeon, i);

    return 1;
}

/**
 * Loads a dungeon room into map->dungeon->room
 */
void DngMapLoader::initDungeonRoom(Dungeon *dng, int room) {
    dng->roomMaps[room] = dynamic_cast<CombatMap *>(mapMgr->initMap(Map::COMBAT));

    dng->roomMaps[room]->id = 0;
    dng->roomMaps[room]->border_behavior = Map::BORDER_FIXED;
    dng->roomMaps[room]->width = dng->roomMaps[room]->height = 11;

    for (unsigned int y = 0; y < dng->roomMaps[room]->height; y++) {
        for (unsigned int x = 0; x < dng->roomMaps[room]->width; x++) {
            dng->roomMaps[room]->data.push_back(MapTile(dng->rooms[room].map_data[x + (y * dng->roomMaps[room]->width)]));
        }
    }

    dng->roomMaps[room]->music = Music::COMBAT;
    dng->roomMaps[room]->type = Map::COMBAT;
    dng->roomMaps[room]->flags |= NO_LINE_OF_SIGHT;
}

/**
 * Loads the world map data in from the 'world' file.
 */
int WorldMapLoader::load(Map *map) {
    U4FILE *world = u4fopen(map->fname);
    if (!world)
        errorFatal("unable to load map data");

    if (!loadData(map, world))
        return 0;

    u4fclose(world);

    return 1;
    
}
