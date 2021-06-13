/*
 * maploader.cpp
 */

#include <cstring>
#include "u4.h"

#include "city.h"
#include "combat.h"
#include "config.h"
#include "dialogueloader.h"
#include "debug.h"
#include "dungeon.h"
#include "error.h"
#include "map.h"
#include "mapmgr.h"
#include "person.h"
#include "tileset.h"
#include "u4file.h"
#include "xu4.h"


static bool isChunkCompressed(Map *map, int chunk) {
    CompressedChunkList::iterator i;
    for (i = map->compressed_chunks.begin(); i != map->compressed_chunks.end(); i++) {
        if (chunk == *i)
            return true;
    }
    return false;
}

/**
 * Loads raw data from the given file.
 */
static bool loadMapData(Map *map, U4FILE *uf) {
    unsigned int x, xch, y, ych;
    const UltimaSaveIds* usaveIds = xu4.config->usaveIds();
    Symbol sym_sea = SYM_UNSET;

    /* allocate the space we need for the map data */
    map->data.resize(map->height * map->width, MapTile(0));

    if (map->chunk_height == 0)
        map->chunk_height = map->height;
    if (map->chunk_width == 0)
        map->chunk_width = map->width;

    u4fseek(uf, map->offset, SEEK_CUR);

#ifdef USE_GL
    size_t chunkLen = map->chunk_width * map->chunk_height;
    map->chunks = new uint8_t[map->width * map->height];
    uint8_t* cp = map->chunks;
#endif
    for(ych = 0; ych < (map->height / map->chunk_height); ++ych) {
        for(xch = 0; xch < (map->width / map->chunk_width); ++xch) {
            if (isChunkCompressed(map, ych * map->chunk_width + xch)) {
                if (! sym_sea)
                    sym_sea = xu4.config->intern("sea");
                MapTile water = map->tileset->getByName(sym_sea)->getId();
                for(y = 0; y < map->chunk_height; ++y) {
                    for(x = 0; x < map->chunk_width; ++x) {
                        map->data[x + (y * map->width) + (xch * map->chunk_width) + (ych * map->chunk_height * map->width)] = water;
                    }
                }
            }
            else {
#ifdef USE_GL
                size_t n = u4fread(cp, 1, chunkLen, uf);
                if (n != chunkLen)
                    return false;
#endif
                for(y = 0; y < map->chunk_height; ++y) {
                    for(x = 0; x < map->chunk_width; ++x) {
#ifdef USE_GL
                        int c = *cp++;
#else
                        int c = u4fgetc(uf);
                        if (c == EOF)
                            return false;
#endif
                        MapTile mt = usaveIds->moduleId(c);
                        map->data[x + (y * map->width) + (xch * map->chunk_width) + (ych * map->chunk_height * map->width)] = mt;
                    }
                }
            }
        }
    }

    return true;
}

enum PersonDataOffset {
    PD_TILE      = 0,
    PD_X         = 1*CITY_MAX_PERSONS,
    PD_Y         = 2*CITY_MAX_PERSONS,
    PD_PREV_TILE = 3*CITY_MAX_PERSONS,
    PD_MOVE      = 6*CITY_MAX_PERSONS,
    PD_CONV      = 7*CITY_MAX_PERSONS,
    PD_SIZE      = 8*CITY_MAX_PERSONS
};

static int moveBehavior(int ultValue) {
    switch(ultValue) {
        case 0:     return MOVEMENT_FIXED;
        case 1:     return MOVEMENT_WANDER;
        case 0x80:  return MOVEMENT_FOLLOW_AVATAR;
        case 0xFF:  return MOVEMENT_ATTACK_AVATAR;
    }
    return -1;
}

/**
 * Load city data from 'ult' and 'tlk' files.
 */
static bool loadCityMap(Map *map, U4FILE *ult) {
    City *city = dynamic_cast<City*>(map);
    int i, j;
    uint8_t* data;
    uint8_t* pd;
    Person* per;
    Person *people[CITY_MAX_PERSONS];
    const UltimaSaveIds* usaveIds = xu4.config->usaveIds();
    Dialogue* dlg;
    bool ok = false;

    /* the map must be 32x32 to be read from an .ULT file */
    ASSERT(city->width == CITY_WIDTH, "map width is %d, should be %d", city->width, CITY_WIDTH);
    ASSERT(city->height == CITY_HEIGHT, "map height is %d, should be %d", city->height, CITY_HEIGHT);

    if (! loadMapData(city, ult))
        return false;

    data = new uint8_t[PD_SIZE];
    i = u4fread(data, 1, PD_SIZE, ult);
    if (i != PD_SIZE)
        goto cleanup;


    /* Properly construct people for the city */
    pd = data;
    for (i = 0; i < CITY_MAX_PERSONS; i++) {
        if (pd[PD_TILE]) {
            per = new Person(usaveIds->moduleId( pd[PD_TILE] ));
            per->setPrevTile(usaveIds->moduleId( pd[PD_PREV_TILE] ));

            MapCoords& pos = per->getStart();
            pos.x = pd[PD_X];
            pos.y = pd[PD_Y];
            pos.z = 0;

            if ((j = moveBehavior( pd[PD_MOVE] )) < 0)
                goto cleanup;
            per->setMovementBehavior((ObjectMovementBehavior) j);

            city->persons.push_back(per);
            people[i] = per;
        } else {
            people[i] = NULL;
        }
        ++pd;
    }

    {
    const uint8_t* conv_idx = data + PD_CONV;
    U4FILE *tlk = u4fopen(city->tlk_fname);
    if (! tlk)
        errorFatal("Unable to open .TLK file");

    DialogueLoader *dlgLoader = DialogueLoader::getLoader("application/x-u4tlk");

    // NOTE: Ultima 4 .TLK files only have 16 conversations, but this`loop
    // will support mods with more.
    for (i = 0; i < CITY_MAX_PERSONS; i++) {
        dlg = dlgLoader->load(tlk);
        if (! dlg)
            break;

        /*
         * Match up dialogues with their respective people. Multiple people
         * can share the same dialogue.
         */
        bool found = false;
        for (j = 0; j < CITY_MAX_PERSONS; j++) {
            if (conv_idx[j] == i+1) {
                people[j]->setDialogue(dlg);
                found = true;
            }
        }
        /*
         * if the dialogue doesn't match up with a person, attach it to the
         * city; Isaac the ghost in Skara Brae is handled like this
         */
        if (! found)
            city->extraDialogues.push_back(dlg);
        else
            city->dialogueStore.push_back(dlg);
    }

    u4fclose(tlk);
    }

    /*
     * Assign roles to certain people
     */
    {
    PersonRoleList::iterator ri;
    foreach (ri, city->personroles) {
        per = people[ (*ri)->id - 1 ];
        if (per) {
            if ((*ri)->role == NPC_LORD_BRITISH) {
                dlg = DialogueLoader::getLoader("application/x-u4lbtlk")->load(NULL);
set_dialog:
                per->setDialogue(dlg);
                city->dialogueStore.push_back(dlg);
            } else if ((*ri)->role == NPC_HAWKWIND) {
                dlg = DialogueLoader::getLoader("application/x-u4hwtlk")->load(NULL);
                goto set_dialog;
            }
            per->setNpcType(static_cast<PersonNpcType>((*ri)->role));
        }
    }
    }
    ok = true;

cleanup:
    delete[] data;
    return ok;
}

/**
 * Loads a combat (or shrine) map from the 'con' file
 */
static bool loadCombatMap(Map *map, U4FILE *uf) {
    int i;

    /* the map must be 11x11 to be read from an .CON file */
    ASSERT(map->width == CON_WIDTH, "map width is %d, should be %d", map->width, CON_WIDTH);
    ASSERT(map->height == CON_HEIGHT, "map height is %d, should be %d", map->height, CON_HEIGHT);

    if (map->type != Map::SHRINE) {
        CombatMap *cm = getCombatMap(map);

        for (i = 0; i < AREA_CREATURES; i++)
            cm->creature_start[i] = MapCoords(u4fgetc(uf));

        for (i = 0; i < AREA_CREATURES; i++)
            cm->creature_start[i].y = u4fgetc(uf);

        for (i = 0; i < AREA_PLAYERS; i++)
            cm->player_start[i] = MapCoords(u4fgetc(uf));

        for (i = 0; i < AREA_PLAYERS; i++)
            cm->player_start[i].y = u4fgetc(uf);

        u4fseek(uf, 16L, SEEK_CUR);
    }

    return loadMapData(map, uf);
}

/**
 * Loads a dungeon room into map->dungeon->room
 */
static void initDungeonRoom(Dungeon *dng, int room) {
    CombatMap* cmap = new CombatMap;
    dng->roomMaps[room] = cmap;

    cmap->id = 0;
    cmap->border_behavior = Map::BORDER_FIXED;
    cmap->width = cmap->height = 11;
    cmap->data = dng->rooms[room].map_data; // Load map data
    cmap->music = MUSIC_COMBAT;
    cmap->type = Map::COMBAT;
    cmap->flags |= NO_LINE_OF_SIGHT;
    cmap->tileset = xu4.config->tileset();
}

/**
 * Loads a dungeon map from the 'dng' file (and dngmap.sav if provided).
 */
static bool loadDungeonMap(Map *map, U4FILE *uf, FILE *sav) {
    Dungeon *dungeon = dynamic_cast<Dungeon*>(map);
    const UltimaSaveIds* usaveIds = xu4.config->usaveIds();
    unsigned int i, j;
    uint8_t* rawMap;
    size_t bytes;

    /* the map must be 11x11 to be read from an .CON file */
    ASSERT(dungeon->width == DNG_WIDTH, "map width is %d, should be %d", dungeon->width, DNG_WIDTH);
    ASSERT(dungeon->height == DNG_HEIGHT, "map height is %d, should be %d", dungeon->height, DNG_HEIGHT);

    /* load the dungeon map */
    bytes = DNG_HEIGHT * DNG_WIDTH * dungeon->levels;
    dungeon->rawMap.reserve(bytes);
    rawMap = &dungeon->rawMap.front();
    if (sav) {
        i = fread(rawMap, 1, bytes, sav);
        u4fseek(uf, bytes, SEEK_CUR);
    } else {
        i = u4fread(rawMap, 1, bytes, uf);
    }
    if (i != bytes)
        return false;

    for (i = 0; i < bytes; i++) {
        j = *rawMap++;
        MapTile tile(usaveIds->dngMapToModule(j), 0);
        dungeon->data.push_back(tile);
        //printf( "KR dng tile %d: %d => %d\n", i, j, tile.id);
    }

    /* read in the dungeon rooms */
    /* FIXME: needs a cleanup function to free this memory later */
    dungeon->rooms = new DngRoom[dungeon->n_rooms];

    for (i = 0; i < dungeon->n_rooms; i++) {
        unsigned char room_tiles[121];

        for (j = 0; j < DNGROOM_NTRIGGERS; j++) {
            int tmp;

            dungeon->rooms[i].triggers[j].tile = usaveIds->moduleId(u4fgetc(uf)).id;

            tmp = u4fgetc(uf);
            if (tmp == EOF)
                return false;
            dungeon->rooms[i].triggers[j].x = (tmp >> 4) & 0x0F;
            dungeon->rooms[i].triggers[j].y = tmp & 0x0F;

            tmp = u4fgetc(uf);
            if (tmp == EOF)
                return false;
            dungeon->rooms[i].triggers[j].change_x1 = (tmp >> 4) & 0x0F;
            dungeon->rooms[i].triggers[j].change_y1 = tmp & 0x0F;

            tmp = u4fgetc(uf);
            if (tmp == EOF)
                return false;
            dungeon->rooms[i].triggers[j].change_x2 = (tmp >> 4) & 0x0F;
            dungeon->rooms[i].triggers[j].change_y2 = tmp & 0x0F;
        }

        u4fread(dungeon->rooms[i].creature_tiles, sizeof(dungeon->rooms[i].creature_tiles), 1, uf);
        u4fread(dungeon->rooms[i].creature_start_x, sizeof(dungeon->rooms[i].creature_start_x), 1, uf);
        u4fread(dungeon->rooms[i].creature_start_y, sizeof(dungeon->rooms[i].creature_start_y), 1, uf);
        u4fread(dungeon->rooms[i].party_north_start_x, sizeof(dungeon->rooms[i].party_north_start_x), 1, uf);
        u4fread(dungeon->rooms[i].party_north_start_y, sizeof(dungeon->rooms[i].party_north_start_y), 1, uf);
        u4fread(dungeon->rooms[i].party_east_start_x, sizeof(dungeon->rooms[i].party_east_start_x), 1, uf);
        u4fread(dungeon->rooms[i].party_east_start_y, sizeof(dungeon->rooms[i].party_east_start_y), 1, uf);
        u4fread(dungeon->rooms[i].party_south_start_x, sizeof(dungeon->rooms[i].party_south_start_x), 1, uf);
        u4fread(dungeon->rooms[i].party_south_start_y, sizeof(dungeon->rooms[i].party_south_start_y), 1, uf);
        u4fread(dungeon->rooms[i].party_west_start_x, sizeof(dungeon->rooms[i].party_west_start_x), 1, uf);
        u4fread(dungeon->rooms[i].party_west_start_y, sizeof(dungeon->rooms[i].party_west_start_y), 1, uf);
        u4fread(room_tiles, sizeof(room_tiles), 1, uf);
        u4fread(dungeon->rooms[i].buffer, sizeof(dungeon->rooms[i].buffer), 1, uf);

        /* translate each creature tile to a tile id */
        for (j = 0; j < sizeof(dungeon->rooms[i].creature_tiles); j++)
            dungeon->rooms[i].creature_tiles[j] = usaveIds->moduleId(dungeon->rooms[i].creature_tiles[j]).id;

        /* translate each map tile to a tile id */
        for (j = 0; j < sizeof(room_tiles); j++)
            dungeon->rooms[i].map_data.push_back(usaveIds->moduleId(room_tiles[j]));

        //
        // dungeon room fixup
        //
        if (map->id == MAP_HYTHLOTH)
        {
            // A couple rooms in hythloth have NULL player start positions,
            // which causes the entire party to appear in the upper-left
            // tile when entering the dungeon room.
            //
            // Also, one dungeon room is apparently supposed to be connected
            // to another, although the the connection does not exist in the
            // DOS U4 dungeon data file.  This was fixed by removing a few
            // wall tiles, and relocating a chest and the few monsters around
            // it to the center of the room.
            //
            if (i == 0x7)
            {
                // update party start positions when entering from the east
                const unsigned char x1[8] = { 0x8, 0x8, 0x9, 0x9, 0x9, 0xA, 0xA, 0xA },
                                    y1[8] = { 0x3, 0x2, 0x3, 0x2, 0x1, 0x3, 0x2, 0x1 };
                memcpy(dungeon->rooms[i].party_east_start_x, x1, sizeof(x1));
                memcpy(dungeon->rooms[i].party_east_start_y, y1, sizeof(y1));

                // update party start positions when entering from the south
                const unsigned char x2[8] = { 0x3, 0x2, 0x3, 0x2, 0x1, 0x3, 0x2, 0x1 },
                                    y2[8] = { 0x8, 0x8, 0x9, 0x9, 0x9, 0xA, 0xA, 0xA };
                memcpy(dungeon->rooms[i].party_south_start_x, x2, sizeof(x2));
                memcpy(dungeon->rooms[i].party_south_start_y, y2, sizeof(y2));
            }
            else if (i == 0x9)
            {
                // update the starting position of monsters 7, 8, and 9
                const unsigned char x1[3] = { 0x4, 0x6, 0x5 },
                                    y1[3] = { 0x5, 0x5, 0x6 };
                memcpy(dungeon->rooms[i].creature_start_x+7, x1, sizeof(x1));
                memcpy(dungeon->rooms[i].creature_start_y+7, y1, sizeof(y1));

                // update party start positions when entering from the west
                const unsigned char x2[8] = { 0x2, 0x2, 0x1, 0x1, 0x1, 0x0, 0x0, 0x0 },
                                    y2[8] = { 0x9, 0x8, 0x9, 0x8, 0x7, 0x9, 0x8, 0x7 };
                memcpy(dungeon->rooms[i].party_west_start_x, x2, sizeof(x2));
                memcpy(dungeon->rooms[i].party_west_start_y, y2, sizeof(y2));

                // update the map data, moving the chest to the center of the room,
                // and removing the walls at the lower-left corner thereby creating
                // a connection to room 8
                const Coords tile[] = { Coords(5, 5, 0x3C),  // chest
                                        Coords(0, 7, 0x16),  // floor
                                        Coords(1, 7, 0x16),
                                        Coords(0, 8, 0x16),
                                        Coords(1, 8, 0x16),
                                        Coords(0, 9, 0x16) };

                for (int j=0; j < int(sizeof(tile)/sizeof(Coords)); j++)
                {
                    const int index = (tile[j].y * CON_WIDTH) + tile[j].x;
                    dungeon->rooms[i].map_data[index] = usaveIds->moduleId(tile[j].z);
                }
            }
        }
    }

    dungeon->roomMaps = new CombatMap *[dungeon->n_rooms];
    for (i = 0; i < dungeon->n_rooms; i++)
        initDungeonRoom(dungeon, i);

    return true;
}

bool loadMap(Map *map, FILE* sav) {
    bool ok = false;
    U4FILE* uf = u4fopen(map->fname);
    if (uf) {
        switch (map->type) {
            case Map::CITY:
                ok = loadCityMap(map, uf);
                break;

            case Map::COMBAT:
            case Map::SHRINE:
                ok = loadCombatMap(map, uf);
                break;

            case Map::DUNGEON:
                ok = loadDungeonMap(map, uf, sav);
                break;

            case Map::WORLD:
                ok = loadMapData(map, uf);
                break;
        }
        u4fclose(uf);
    }
    return ok;
}
