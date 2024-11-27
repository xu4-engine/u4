/*
 * maploader.cpp
 */

#include <cassert>
#include <cstring>
#include "u4.h"

#include "city.h"
#include "config.h"
#include "debug.h"
#include "dungeon.h"
#include "error.h"
#include "mapmgr.h"
#include "person.h"
#include "u4file.h"
#include "xu4.h"

#ifdef GPU_RENDER
#include "tileset.h"
#endif


#ifdef CONF_MODULE
// Map header written by pack-xu4 (CDI 0xDA7A1FC0).
struct Xu4MapHeader {
    uint8_t  idM;           // 'm'
    uint8_t  idVersion;
    uint16_t w;             // Total tile width.
    uint16_t h;             // Total tile height.
    uint8_t  levels;
    uint8_t  chunkDim;      // If non-zero then map is chunked.
    uint16_t flags;         // zero, reserved for future features.
    uint16_t npcCount;
    uint32_t npcOffset;
    // uint8_t grid[w * h];
    // Xu4MapNpc npc[npcCount];
};

struct Xu4MapNpc {
    uint16_t role;
    uint16_t talkId;
    uint16_t tile;
    uint16_t x;
    uint16_t y;
    uint16_t movement;
};
#endif

#ifdef U5_DAT
static bool isChunkCompressed(Map *map, int chunk) {
    return map->compressed_chunks[ chunk ] == 255;
}
#endif

/**
 * Loads raw data from the given file.
 */
bool loadMapData(Map *map, U4FILE *uf, Symbol borderTile) {
    unsigned int x, xch, y, ych, di;
    unsigned int chunkCols, chunkRows;
    size_t chunkLen;
    uint8_t* chunk;
    uint8_t* cp;
    const UltimaSaveIds* usaveIds = xu4.config->usaveIds();
    bool ok = false;
#ifdef U5_DAT
    Symbol sym_sea = SYM_UNSET;
#endif

    map->boundMaxX = map->width;
    map->boundMaxY = map->height;

    if (map->chunk_height == 0)
        map->chunk_height = map->height;
    if (map->chunk_width == 0)
        map->chunk_width = map->width;

    chunkCols = map->width / map->chunk_width;
    chunkRows = map->height / map->chunk_height;

    chunkLen = map->chunk_width * map->chunk_height;
    chunk = new uint8_t[chunkLen];

#ifdef GPU_RENDER
    bool addBorder = false;
    if (map->border_behavior == Map::BORDER_EXIT2PARENT && borderTile) {
        // gpu_drawMap() always wraps the map so adding border chunks to the
        // bottom & right edges will render these borders on all edges.
        map->width  += map->chunk_width;
        map->height += map->chunk_height;
        addBorder = true;
    }

    // gpu_drawMap() requires chunk_width & chunk_height to be the same, so
    // the size is adjusted to be square.
    // This code assumes the width is never less than height and handles the
    // intro 19x5 map.
    int padRows = 0;
    if (map->chunk_height < map->chunk_width)
        padRows = map->chunk_width - map->chunk_height;

    /* allocate the space we need for the map data */
    map->data = new TileId[map->width * (map->height + padRows)];

    if (addBorder) {
        // Fill the entire map with borderTile.
        TileId borderId = Tileset::findTileByName(borderTile)->id;
        TileId* it = map->data;
        TileId* end = it + map->width * map->height;
        while (it != end)
            *it++ = borderId;
    }
#else
    /* allocate the space we need for the map data */
    map->data = new TileId[map->width * map->height];
#endif

    if (map->offset)
        u4fseek(uf, map->offset, SEEK_CUR);

    for(ych = 0; ych < chunkRows; ++ych) {
        for(xch = 0; xch < chunkCols; ++xch) {
            di = (xch * map->chunk_width) +
                 (ych * map->chunk_height * map->width);

#ifdef U5_DAT
            if (isChunkCompressed(map, ych * map->chunk_width + xch)) {
                if (! sym_sea)
                    sym_sea = xu4.config->intern("sea");
                MapTile water = map->tileset->getByName(sym_sea)->getId();
                for(y = 0; y < map->chunk_height; ++y) {
                    for(x = 0; x < map->chunk_width; ++x) {
                        map->data[x + (y * map->width) + di] = water.id;
                    }
                }
            }
            else
#endif
            {
                size_t n = u4fread(chunk, 1, chunkLen, uf);
                if (n != chunkLen)
                    goto cleanup;

                cp = chunk;
                for(y = 0; y < map->chunk_height; ++y) {
                    for(x = 0; x < map->chunk_width; ++x) {
                        int c = *cp++;
                        MapTile mt = usaveIds->moduleId(c);
                        map->data[x + (y * map->width) + di] = mt.id;
                    }
                }
            }
        }
    }
    ok = true;

#ifdef GPU_RENDER
    if (padRows) {
        // Force square chunks and clear padded rows.
        memset(map->data + map->width * map->height, 0,
               sizeof(TileId) * map->width * padRows);
        map->height += padRows;
        map->chunk_height = map->chunk_width;
    }
#endif

cleanup:
    delete[] chunk;
    return ok;
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
    bool ok = false;

    /* the map must be 32x32 to be read from an .ULT file */
    ASSERT(city->width == CITY_WIDTH, "map width is %d, should be %d", city->width, CITY_WIDTH);
    ASSERT(city->height == CITY_HEIGHT, "map height is %d, should be %d", city->height, CITY_HEIGHT);

    if (! loadMapData(city, ult, Tile::sym.grass))
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
            per->prevTile = usaveIds->moduleId( pd[PD_PREV_TILE] );

            Coords& pos = per->getStart();
            pos.x = pd[PD_X];
            pos.y = pd[PD_Y];
            pos.z = 0;

            if ((j = moveBehavior( pd[PD_MOVE] )) < 0)
                goto cleanup;
            per->movement = (ObjectMovement) j;

            city->persons.push_back(per);
            people[i] = per;
        } else {
            people[i] = NULL;
        }
        ++pd;
    }

    {
    const uint8_t* conv_idx = data + PD_CONV;
    const char* err;
    int count;

    err = discourse_load(&city->disc, xu4.config->confString(city->tlk_fname));
    if (err)
        errorFatal(err);

    count = city->disc.convCount;
    for (i = 0; i < count; i++) {
        /*
         * Match up dialogues with their respective people. Multiple people
         * can share the same dialogue. Some NPCs, like Isaac the ghost in
         * Skara Brae, will attach to their dialogue later.
         */
        for (j = 0; j < CITY_MAX_PERSONS; j++) {
            if (conv_idx[j] == i+1)
                people[j]->setDiscourseId(i);
        }
    }
    }

    /*
     * Assign roles to certain people
     */
    {
    vector<PersonRole>::iterator ri;
    foreach (ri, city->personroles) {
        per = people[ (*ri).id - 1 ];
        if (per)
            per->setNpcType(static_cast<PersonNpcType>((*ri).role));
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
            cm->creature_start[i] = Coords(u4fgetc(uf));

        for (i = 0; i < AREA_CREATURES; i++)
            cm->creature_start[i].y = u4fgetc(uf);

        for (i = 0; i < AREA_PLAYERS; i++)
            cm->player_start[i] = Coords(u4fgetc(uf));

        for (i = 0; i < AREA_PLAYERS; i++)
            cm->player_start[i].y = u4fgetc(uf);

        u4fseek(uf, 16L, SEEK_CUR);
    }

    return loadMapData(map, uf, SYM_UNSET);
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
    cmap->boundMaxX = cmap->boundMaxY = 11;
    cmap->music = MUSIC_COMBAT;
    cmap->type = Map::COMBAT;
    cmap->flags |= NO_LINE_OF_SIGHT;
    cmap->tileset = xu4.config->tileset();

    // Copy map data.
    size_t tcount = cmap->width * cmap->height;
    cmap->data = new TileId[tcount];
    memcpy(cmap->data, &dng->rooms[room].map_data[0], tcount * sizeof(TileId));
}

/**
 * Loads a dungeon map from the 'dng' file (and dngmap.sav if provided).
 */
static bool loadDungeonMap(Map *map, U4FILE *uf, FILE *sav) {
    Dungeon *dungeon = dynamic_cast<Dungeon*>(map);
    const UltimaSaveIds* usaveIds = xu4.config->usaveIds();
    char padding[7];
    unsigned int i, j;
    uint8_t* rawMap;
    size_t bytes;

    // NOTE: Changes here must work in tandem with Dungeon::unloadRooms()!

    /* the map must be 11x11 to be read from an .CON file */
    ASSERT(dungeon->width == DNG_WIDTH, "map width is %d, should be %d", dungeon->width, DNG_WIDTH);
    ASSERT(dungeon->height == DNG_HEIGHT, "map height is %d, should be %d", dungeon->height, DNG_HEIGHT);

    map->boundMaxX = map->width;
    map->boundMaxY = map->height;

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

    if (! dungeon->data)
        dungeon->data = new TileId[bytes];

    TileId* dp = dungeon->data;
    for (i = 0; i < bytes; i++) {
        j = *rawMap++;
        MapTile tile(usaveIds->dngMapToModule(j), 0);
        *dp++ = tile.id;
        //printf( "KR dng tile %d: %d => %d\n", i, j, tile.id);
    }

    /* read in the dungeon rooms */
    dungeon->rooms = new DngRoom[dungeon->n_rooms];

    for (i = 0; i < dungeon->n_rooms; i++) {
        uint8_t room_tiles[121];

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
        u4fread(padding, 7, 1, uf);

        /* translate each creature tile to a tile id */
        uint8_t* ct = dungeon->rooms[i].creature_tiles;
        for (j = 0; j < sizeof(dungeon->rooms[i].creature_tiles); j++) {
            *ct = usaveIds->moduleId( *ct ).id;
            ++ct;
        }

        /* translate each map tile to a tile id */
        for (j = 0; j < sizeof(room_tiles); j++)
            dungeon->rooms[i].map_data.push_back(usaveIds->moduleId(room_tiles[j]).id);

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
                const uint8_t x1[8] = {0x8, 0x8, 0x9, 0x9, 0x9, 0xA, 0xA, 0xA},
                              y1[8] = {0x3, 0x2, 0x3, 0x2, 0x1, 0x3, 0x2, 0x1};
                memcpy(dungeon->rooms[i].party_east_start_x, x1, sizeof(x1));
                memcpy(dungeon->rooms[i].party_east_start_y, y1, sizeof(y1));

                // update party start positions when entering from the south
                const uint8_t x2[8] = {0x3, 0x2, 0x3, 0x2, 0x1, 0x3, 0x2, 0x1},
                              y2[8] = {0x8, 0x8, 0x9, 0x9, 0x9, 0xA, 0xA, 0xA};
                memcpy(dungeon->rooms[i].party_south_start_x, x2, sizeof(x2));
                memcpy(dungeon->rooms[i].party_south_start_y, y2, sizeof(y2));
            }
            else if (i == 0x9)
            {
                // update the starting position of monsters 7, 8, and 9
                const uint8_t x1[3] = {0x4, 0x6, 0x5},
                              y1[3] = {0x5, 0x5, 0x6};
                memcpy(dungeon->rooms[i].creature_start_x+7, x1, sizeof(x1));
                memcpy(dungeon->rooms[i].creature_start_y+7, y1, sizeof(y1));

                // update party start positions when entering from the west
                const uint8_t x2[8] = {0x2, 0x2, 0x1, 0x1, 0x1, 0x0, 0x0, 0x0},
                              y2[8] = {0x9, 0x8, 0x9, 0x8, 0x7, 0x9, 0x8, 0x7};
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
                    dungeon->rooms[i].map_data[index] = usaveIds->moduleId(tile[j].z).id;
                }
            }
        }
    }

    dungeon->roomMaps = new CombatMap *[dungeon->n_rooms];
    for (i = 0; i < dungeon->n_rooms; i++)
        initDungeonRoom(dungeon, i);

    return true;
}

#ifdef CONF_MODULE
static bool loadCityXu4(Map* map, U4FILE* uf, size_t npcCount) {
    if (! loadMapData(map, uf, Tile::sym.grass))
        return false;

    bool ok = false;
    const UltimaSaveIds* usaveIds = xu4.config->usaveIds();
    Xu4MapNpc* npcBuffer = new Xu4MapNpc[npcCount];

    size_t n = u4fread(npcBuffer, sizeof(Xu4MapNpc), npcCount, uf);
    if (n == npcCount) {
        City* city = dynamic_cast<City*>(map);
        const Xu4MapNpc* npc = npcBuffer;
        for (size_t i = 0; i < npcCount; ++i) {
            //printf("NPC %ld %d,%d %d\n", i, npc->x, npc->y, npc->role);

            MapTile tile = usaveIds->moduleId(npc->tile);
            Person* per = new Person(tile);
            per->prevTile = tile;

            Coords& pos = per->getStart();
            pos.x = npc->x;
            pos.y = npc->y;
            pos.z = 0;

            per->movement = (ObjectMovement) npc->movement;

            per->setDiscourseId(npc->talkId);   // Sets npcType.

            if (npc->role)
                per->setNpcType((PersonNpcType) npc->role);

            city->persons.push_back(per);
            ++npc;
        }
        ok = true;

        const char* err = discourse_load(&city->disc,
                                    xu4.config->confString(city->tlk_fname));
        if (err)
            errorFatal(err);
    }

    delete[] npcBuffer;
    return ok;
}
#endif

bool loadMap(Map *map, FILE* sav) {
    U4FILE* uf;
    bool ok = false;

#ifdef CONF_MODULE
    if (map->fname) {
        std::string fname( xu4.config->confString(map->fname) );
        uf = u4fopen(fname);
    } else {
        const CDIEntry* ent = xu4.config->mapFile(map->id);
        if (ent) {
            uf = u4fopen_stdio(xu4.config->modulePath(ent));
            if (uf) {
                Xu4MapHeader head;

                u4fseek(uf, ent->offset, SEEK_SET);
                if (u4fread(&head, 1, sizeof(head), uf) != sizeof(head))
                    goto done;
                if (head.idM != 'm' || head.idVersion != 1)
                    goto done;

                map->width  = head.w;
                map->height = head.h;
                map->chunk_width  =
                map->chunk_height = head.chunkDim;

                if (map->type == Map::CITY) {
                    ok = loadCityXu4(map, uf, head.npcCount);
                    goto done;
                }
            }
        } else
            uf = NULL;
    }
#else
    std::string fname( xu4.config->confString(map->fname) );
    uf = u4fopen(fname);
#endif
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
                ok = loadMapData(map, uf, SYM_UNSET);
                break;
        }
#ifdef CONF_MODULE
done:
#endif
        u4fclose(uf);
    }
    return ok;
}
