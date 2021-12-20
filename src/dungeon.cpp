/*
 * dungeon.cpp
 */

#include "dungeon.h"

#include "config.h"
#include "context.h"
#include "debug.h"
#include "item.h"
#include "mapmgr.h"
#include "screen.h"
#include "tileset.h"
#include "utils.h"
#include "xu4.h"

Dungeon::~Dungeon() {
    if (roomMaps) {
        CombatMap** it  = roomMaps;
        CombatMap** end = it + n_rooms;
        for (; it != end; ++it)
            delete *it;

        delete[] roomMaps;
    }

    delete[] rooms;
}

/**
 * Returns the name of the dungeon
 */
const char* Dungeon::getName() const {
    return xu4.config->confString(name);
}

/**
 * Returns the dungeon token associated with the given dungeon tile
 */
DungeonToken Dungeon::tokenForTile(TileId tid) const {
    int i;
    const Tile *t = tileset->get(tid);

    for (i = 0; Tile::sym.dungeonTiles[i]; i++) {
        if (t->name == Tile::sym.dungeonTiles[i])
            return DungeonToken(i<<4);
    }

    for (i = 0; Tile::sym.fields[i]; i++) {
        if (t->name == Tile::sym.fields[i])
            return DUNGEON_FIELD;
    }

    return (DungeonToken)0;
}

/**
 * Returns the dungeon token for the current location
 */
DungeonToken Dungeon::currentToken() {
    return tokenAt(c->location->coords);
}

/**
 * Returns the dungeon sub-token for the current location
 * The subtoken is encoded in the lower bits of the map raw data.
 * For instance, for the raw value 0x91, returns FOUNTAIN_HEALING NOTE:
 * This function will always need type-casting to the token type
 * necessary.
 */
uint8_t Dungeon::currentSubToken() {
    const Coords& co = c->location->coords;
    int index = co.x + (co.y * width) + (width * height * co.z);
    return rawMap[index] & 15;
}

/**
 * Returns the dungeon token for the given coordinates
 */
DungeonToken Dungeon::tokenAt(const Coords& coords) const {
    return tokenForTile(getTileFromData(coords));
}

static int anyAnnotation(const Annotation* ann, void* data) {
    (void) ann;
    *((bool*) data) = true;
    return Map::QueryDone;
}

/**
 * Handles 's'earching while in dungeons
 */
void dungeonSearch(void) {
    Dungeon *dungeon = dynamic_cast<Dungeon *>(c->location->map);
    DungeonToken token = dungeon->currentToken();

    bool found = false;
    dungeon->queryAnnotations(c->location->coords, anyAnnotation, &found);
    if (found)
        token = DUNGEON_CORRIDOR;

    screenMessage("Search...\n\n");

    switch (token) {
    case DUNGEON_MAGIC_ORB: /* magic orb */
        dungeonTouchOrb();
        break;

    case DUNGEON_FOUNTAIN: /* fountains */
        dungeonDrinkFountain();
        break;

    default:
        {
            /* see if there is an item at the current location (stones on altars, etc.) */
            const ItemLocation *item;
            item = itemAtLocation(dungeon, c->location->coords);
            if (item) {
                if (*item->isItemInInventory != NULL && (*item->isItemInInventory)(item->data))
                    screenMessage("Nothing Here!\n");
                else {
                    if (item->name)
                        screenMessage("You find...\n%s!\n", item->name);
                    (*item->putItemInInventory)(item->data);
                }
            } else
                screenMessage("You find Nothing!\n");
        }

        break;
    }
}

/**
 * Drink from the fountain at the current location
 */
void dungeonDrinkFountain() {
    screenMessage("You find a Fountain.\nWho drinks? ");
    int player = gameGetPlayer(false, false);
    if (player == -1)
        return;

    PartyMember* pc = c->party->member(player);
    Dungeon *dungeon = dynamic_cast<Dungeon *>(c->location->map);
    FountainType type = (FountainType) dungeon->currentSubToken();

    switch(type) {
    /* plain fountain */
    case FOUNTAIN_NORMAL:
        screenMessage("\nHmmm--No Effect!\n");
        break;

    /* healing fountain */
    case FOUNTAIN_HEALING:
        if (pc->heal(HT_FULLHEAL))
            screenMessage("\nAhh-Refreshing!\n");
        else
            screenMessage("\nHmmm--No Effect!\n");
        break;

    /* acid fountain */
    case FOUNTAIN_ACID:
        pc->applyDamage(dungeon, 100);      /* 100 damage to drinker */
        screenMessage("\nBleck--Nasty!\n");
        break;

    /* cure fountain */
    case FOUNTAIN_CURE:
        if (pc->heal(HT_CURE))
            screenMessage("\nHmmm--Delicious!\n");
        else
            screenMessage("\nHmmm--No Effect!\n");
        break;

    /* poison fountain */
    case FOUNTAIN_POISON:
        if (pc->getStatus() != STAT_POISONED) {
            soundPlay(SOUND_POISON_DAMAGE);
            pc->applyEffect(dungeon, EFFECT_POISON);
            pc->applyDamage(dungeon, 100);  /* 100 damage to drinker also */
            screenMessage("\nArgh-Choke-Gasp!\n");
        }
        else
            screenMessage("\nHmm--No Effect!\n");
        break;

    default:
        ASSERT(0, "Invalid call to dungeonDrinkFountain: no fountain at current location");
    }
}

/**
 * Touch the magical ball at the current location
 */
void dungeonTouchOrb() {
    screenMessage("You find a Magical Ball...\nWho touches? ");
    int player = gameGetPlayer(false, false);
    if (player == -1)
        return;

    int stats = 0;
    int damage = 0;

    /* Get current position and find a replacement tile for it */
    Location* loc = c->location;
    const Tile * orb_tile = loc->map->tileset->getByName(SYM_MAGIC_ORB);

    switch(loc->map->id) {
    case MAP_DECEIT:    stats = STATSBONUS_INT; break;
    case MAP_DESPISE:   stats = STATSBONUS_DEX; break;
    case MAP_DESTARD:   stats = STATSBONUS_STR; break;
    case MAP_WRONG:     stats = STATSBONUS_INT | STATSBONUS_DEX; break;
    case MAP_COVETOUS:  stats = STATSBONUS_DEX | STATSBONUS_STR; break;
    case MAP_SHAME:     stats = STATSBONUS_INT | STATSBONUS_STR; break;
    case MAP_HYTHLOTH:  stats = STATSBONUS_INT | STATSBONUS_DEX | STATSBONUS_STR; break;
    default: break;
    }

    /* give stats bonuses */
    if (stats & STATSBONUS_STR) {
        screenMessage("Strength + 5\n");
        AdjustValueMax(c->saveGame->players[player].str, 5, 50);
        damage += 200;
    }
    if (stats & STATSBONUS_DEX) {
        screenMessage("Dexterity + 5\n");
        AdjustValueMax(c->saveGame->players[player].dex, 5, 50);
        damage += 200;
    }
    if (stats & STATSBONUS_INT) {
        screenMessage("Intelligence + 5\n");
        AdjustValueMax(c->saveGame->players[player].intel, 5, 50);
        damage += 200;
    }

    /* deal damage to the party member who touched the orb */
    c->party->member(player)->applyDamage(loc->map, damage);

    /* remove the orb from the map */
    loc->map->setTileAt(loc->coords,
                        loc->getReplacementTile(loc->coords, orb_tile));
}

/**
 * Handles dungeon traps
 */
bool dungeonHandleTrap(TrapType trap) {
    Dungeon *dungeon = dynamic_cast<Dungeon *>(c->location->map);
    switch((TrapType)dungeon->currentSubToken()) {
    case TRAP_WINDS:
        screenMessage("\nWinds!\n");
        soundPlay(SOUND_WIND_GUST);
        c->party->quenchTorch();
        break;
    case TRAP_FALLING_ROCK:
        // Treat falling rocks and pits like bomb traps
        // XXX: That's a little harsh.
        screenMessage("\nFalling Rocks!\n");
        goto pit_damage;
    case TRAP_PIT:
        screenMessage("\nPit!\n");
pit_damage:
        soundPlay(SOUND_STONE_FALLING);
        c->party->applyEffect(dungeon, EFFECT_LAVA);
        screenShake(3);     // NOTE: In the DOS version only the view shakes.
        break;
    default: break;
    }

    return true;
}

struct Contains {
    TileId tid;
    bool found;
};

static int contains(const Annotation* ann, void* data) {
    Contains* con = (Contains*) data;
    if (ann->tile.id == con->tid) {
        con->found = true;
        return Map::QueryDone;
    }
    return Map::QueryContinue;
}

/**
 * Returns true if a ladder-up is found at the given coordinates
 */
bool Dungeon::ladderUpAt(const Coords& coords) const {
    if (tokenAt(coords) == DUNGEON_LADDER_UP ||
        tokenAt(coords) == DUNGEON_LADDER_UPDOWN)
        return true;

    Contains query;
    query.tid = tileset->getByName(SYM_UP_LADDER)->getId();
    query.found = false;
    queryAnnotations(coords, contains, &query);
    return query.found;
}

/**
 * Returns true if a ladder-down is found at the given coordinates
 */
bool Dungeon::ladderDownAt(const Coords& coords) const {
    if (tokenAt(coords) == DUNGEON_LADDER_DOWN ||
        tokenAt(coords) == DUNGEON_LADDER_UPDOWN)
        return true;

    Contains query;
    query.tid = tileset->getByName(SYM_DOWN_LADDER)->getId();
    query.found = false;
    queryAnnotations(coords, contains, &query);
    return query.found;
}

bool Dungeon::validTeleportLocation(const Coords& coords) const {
    const Tile* tile = tileTypeAt(coords, WITH_OBJECTS);
    return tokenForTile(tile->id) == DUNGEON_CORRIDOR;
}


static const uint8_t ultima4DngMapMonster[16] = {
    0,          RAT_ID,     BAT_ID,          GIANT_SPIDER_ID,
    GHOST_ID,   SLIME_ID,   TROLL_ID,        GREMLIN_ID,
    MIMIC_ID,   REAPER_ID,  INSECT_SWARM_ID, GAZER_ID,
    PHANTOM_ID, ORC_ID,     SKELETON_ID,     ROGUE_ID
};

/* Map creatures to u4dos dungeon creature Ids */
static int u4DngMonster(CreatureId cid) {
    int i;
    for (i = 1; i < 16; ++i) {
        if (cid == ultima4DngMapMonster[i])
            return i;
    }
    return 0;
}

uint8_t* Dungeon::fillRawMap() {
    uint32_t x, y, z;
    TileId tid;
    const UltimaSaveIds* usaveIds = xu4.config->usaveIds();
    uint8_t* dp = (uint8_t*) &rawMap.front();
    int uid, dngId;

    for (z = 0; z < levels; z++) {
        for (y = 0; y < height; y++) {
            for (x = 0; x < width; x++) {
                // Don't touch traps, fountains, or rooms.
                uid = *dp & 0xF0;
                if (uid == DUNGEON_TRAP ||
                    uid == DUNGEON_FOUNTAIN ||
                    uid == DUNGEON_ROOM) {
                    ++dp;
                    continue;
                }

                tid = getTileFromData(Coords(x, y, z));
                dngId = usaveIds->moduleToDngMap(tid);
                //printf("KR %d,%d,%d %d => %d\n", x, y, z, tid, dngId);

                // Add the creature to the tile
                const Object *obj = objectAt(Coords(x, y, z));
                if (obj && obj->objType == Object::CREATURE) {
                    const Creature *m = static_cast<const Creature*>(obj);
                    uid = u4DngMonster(m->getId());
                    if (uid)
                        dngId |= uid;
                }

                *dp++ = dngId;
            }
        }
    }

    return &rawMap.front();
}
