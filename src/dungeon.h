/*
 * dungeon.h
 */

#ifndef DUNGEON_H
#define DUNGEON_H

#include <vector>

#include "combat.h"

#define DNGROOM_NTRIGGERS 4

enum StatsBonusType {
    STATSBONUS_INT = 0x1,
    STATSBONUS_DEX = 0x2,
    STATSBONUS_STR = 0x4
};

struct Trigger {
    uint8_t tile;
    uint8_t x, y;
    uint8_t change_x1, change_y1, change_x2, change_y2;
};

struct DngRoom {
    Trigger triggers[DNGROOM_NTRIGGERS];
    uint8_t creature_tiles[16];
    uint8_t creature_start_x[16];
    uint8_t creature_start_y[16];
    uint8_t party_north_start_x[8];
    uint8_t party_north_start_y[8];
    uint8_t party_east_start_x[8];
    uint8_t party_east_start_y[8];
    uint8_t party_south_start_x[8];
    uint8_t party_south_start_y[8];
    uint8_t party_west_start_x[8];
    uint8_t party_west_start_y[8];
    std::vector<TileId> map_data;
};

/**
 * Dungeon tokens
 */
enum DungeonToken {
    DUNGEON_CORRIDOR            = 0x00,
    DUNGEON_LADDER_UP           = 0x10,
    DUNGEON_LADDER_DOWN         = 0x20,
    DUNGEON_LADDER_UPDOWN       = 0x30,
    DUNGEON_CHEST               = 0x40,
    DUNGEON_CEILING_HOLE        = 0x50,
    DUNGEON_FLOOR_HOLE          = 0x60,
    DUNGEON_MAGIC_ORB           = 0x70,
    DUNGEON_TRAP                = 0x80,
    DUNGEON_FOUNTAIN            = 0x90,
    DUNGEON_FIELD               = 0xA0,
    DUNGEON_ALTAR               = 0xB0,
    DUNGEON_DOOR                = 0xC0,
    DUNGEON_ROOM                = 0xD0,
    DUNGEON_SECRET_DOOR         = 0xE0,
    DUNGEON_WALL                = 0xF0
};

class Dungeon : public Map {
public:
    Dungeon() : rooms(NULL), roomMaps(NULL) {}
    ~Dungeon();

    // Members
    virtual const char* getName() const;

    void unloadRooms();
    DungeonToken tokenForTile(TileId tid) const;
    DungeonToken currentToken();
    uint8_t currentSubToken();
    uint8_t subTokenAt(const Coords& coords) const;
    DungeonToken tokenAt(const Coords& coords) const;

    bool ladderUpAt(const Coords& coords) const;
    bool ladderDownAt(const Coords& coords) const;

    bool validTeleportLocation(const Coords& coords) const;
    uint8_t* fillRawMap();

    // Properties
    StringId name;
    std::vector<uint8_t> rawMap;
    DngRoom *rooms;
    CombatMap **roomMaps;
    unsigned int n_rooms;
    int currentRoom;
};

/**
 * Dungeon sub-tokens
 */
enum TrapType {
    TRAP_WINDS                  = 0x0,
    TRAP_FALLING_ROCK           = 0x1,
    TRAP_PIT                    = 0xe
};

enum FountainType {
    FOUNTAIN_NORMAL             = 0x0,
    FOUNTAIN_HEALING            = 0x1,
    FOUNTAIN_ACID               = 0x2,
    FOUNTAIN_CURE               = 0x3,
    FOUNTAIN_POISON             = 0x4
};

enum FieldType {
    FIELD_POISON                = 0x0,
    FIELD_ENERGY                = 0x1,
    FIELD_FIRE                  = 0x2,
    FIELD_SLEEP                 = 0x3
};

void dungeonSearch(void);
void dungeonDrinkFountain();
void dungeonTouchOrb();
bool dungeonHandleTrap(TrapType trap);

#endif
