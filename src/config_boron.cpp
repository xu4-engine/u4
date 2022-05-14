/*
 * config_boron.cpp
 */

#include <cassert>
#include <cstdio>
#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <cctype>

#include <boron/boron.h>

#include "config.h"
#include "city.h"
#include "dungeon.h"
#include "error.h"
#include "imageloader.h"
#include "imagemgr.h"
#include "map.h"
#include "module.h"
#include "portal.h"
#include "screen.h"
#include "settings.h"
#include "shrine.h"
#include "sound.h"
#include "tileanim.h"
#include "tileset.h"
#include "weapon.h"
#include "u4file.h"
#include "xu4.h"

#include "config_data.cpp"

// Order matches config context in pack-xu4.b.
enum ConfigValues
{
    CI_ARMORS,
    CI_WEAPONS,
    CI_CREATURES,
    CI_GRAPHICS,
    CI_TILEANIM,
    CI_LAYOUTS,
    CI_MAPS,
    CI_TILE_RULES,
    CI_TILESET,
    CI_U4SAVEIDS,
    CI_MUSIC,
    CI_SOUND,
    CI_VENDORS,
    CI_EGA_PALETTE,

    CI_COUNT
};

Config::~Config() {}

#if 0
// For future expansion...
const char** Config::getGames() {
    return &"Ultima IV";
}

void Config::setGame(const char* name) {
}
#endif

//--------------------------------------

#define TALK_CACHE_SIZE 2

struct NpcTalkCache
{
    UIndex blkN;
    uint32_t appId[TALK_CACHE_SIZE];
    int lastUsed;
};

static void npcTalk_init(NpcTalkCache* tc, UThread* ut) {
    memset(tc, 0, sizeof(NpcTalkCache));

    tc->blkN = ur_makeBlock(ut, TALK_CACHE_SIZE);
    ur_hold(tc->blkN);      // Keep forever.

    UBuffer* blk = ur_buffer(tc->blkN);
    ur_blkAppendNew(blk, UT_NONE);
    ur_blkAppendNew(blk, UT_NONE);
}

//--------------------------------------
// Boron Backend

struct ConfigData
{
    vector<const char*> sarray;   // Temp. buffer for const char** values.
    vector<Layout> layouts;
    Armor* armors;
    vector<Weapon*> weapons;
    vector<Creature *> creatures;
    uint16_t* creatureTileIndex;
    vector<Map *> mapList;
    vector<Coords> moongateList;    // Moon phase map coordinates.

    TileRule* tileRules;
    uint16_t tileRuleCount;
    int16_t  tileRuleDefault;
    Tileset* tileset;
    UltimaSaveIds usaveIds;
    NpcTalkCache talk;

    uint16_t armorCount;
};

struct ConfigBoron : public Config {
    ConfigBoron(const char* renderPath, const char* modulePath);
    ~ConfigBoron();
    const UBuffer* buffer(int value, int dataType) const;
    const UBuffer* blockIt(UBlockIt* bi, int value) const;
    //const UBuffer* blockBuffer(int value, uint32_t n, int dataType) const;

    UThread* ut;
    Module mod;
    UIndex configN;
    UIndex itemIdN;         // item-id context!
    size_t tocUsed;
    ConfigData xcd;
    UBuffer evalBuf;
    Symbol sym_hitFlash;
    Symbol sym_missFlash;
    Symbol sym_random;
    Symbol sym_shrine;
    Symbol sym_abyss;
    Symbol sym_imageset;
    Symbol sym_tileanims;
    Symbol sym_Ucel;
    Symbol sym_rect;
};

#define CB  static_cast<ConfigData*>(backend)
#define CX  static_cast<const ConfigBoron*>(this)

const UBuffer* ConfigBoron::buffer(int value, int dataType) const
{
    const UBuffer* buf = ur_buffer(configN);
    const UCell* cell = ur_ctxCell(buf, value);
    if (ur_is(cell, dataType))
        return ur_buffer(cell->series.buf);
    return NULL;
}

const UBuffer* ConfigBoron::blockIt(UBlockIt* bi, int value) const
{
    const UBuffer* blk = buffer(value, UT_BLOCK);
    if (blk) {
        bi->it  = blk->ptr.cell;
        bi->end = bi->it + blk->used;
    }
    return blk;
}

/*
const UBuffer* ConfigBoron::blockBuffer(int value, uint32_t n, int dataType) const
{
    const UBuffer* buf = ur_buffer(configN);
    const UCell* cell = ur_ctxCell(buf, value);
    if (ur_is(cell, UT_BLOCK)) {
        buf = ur_buffer(cell->series.buf);
        if (n < (uint32_t) buf->used) {
            cell = buf->ptr.cell + n;
            if (ur_is(cell, dataType))
                return ur_bufferSer(cell);
        }
    }
    return NULL;
}
*/

#define WORD_NONE   (UT_BI_COUNT + UT_WORD)
#define COORD_NONE  (UT_BI_COUNT + UT_COORD)
#define STRING_NONE (UT_BI_COUNT + UT_STRING)
#define FILE_NONE   (UT_BI_COUNT + UT_FILE)
#define BLOCK_NONE  (UT_BI_COUNT + UT_BLOCK)

static bool validParam(const UBlockIt& bi, int count, const uint8_t* dtype)
{
    int type, req, i;

    if ((bi.end - bi.it) < count)
        return false;

    for (i = 0; i < count; ++i) {
        type = ur_type(bi.it + i);
        req  = dtype[i];

        if (req > UT_BI_COUNT) {
            if (type == UT_NONE || type == (req - UT_BI_COUNT))
                continue;
            return false;
        }
        if (type != req)
            return false;
    }
    return true;
}

// Assign String (ensure string! is NUL terminated)
#define ASTR(N)     assignStr(cfg->ut, N)
#define ASTR_UT(N)  assignStr(ut, N)

static UIndex assignStr(UThread* ut, UIndex strN)
{
    ur_strTermNull( ur_buffer(strN) );
    return strN;
}

//--------------------------------------
// Layouts

static int conf_loadLayout(UThread* ut, UBlockIt& bi, Layout* layout) {
    static const uint8_t lparam[6] = {
        // type  name    'tileshape  tileshape  'viewport  viewport
        UT_WORD, UT_STRING, UT_WORD, UT_COORD, UT_WORD, UT_COORD
    };
    if (! validParam(bi, sizeof(lparam), lparam))
        return 0;

    const UCell* cell = bi.it;

    const char* str = ur_wordCStr(cell);
    if (strcasecmp(str, "gem") == 0)
        layout->type = LAYOUT_GEM;
    else if (strcasecmp(str, "dungeon_gem") == 0)
        layout->type = LAYOUT_DUNGEONGEM;
    else
        layout->type = LAYOUT_STANDARD;
    ++cell;

    layout->name = ASTR_UT(cell->series.buf);
    cell += 2;

    layout->tileshape.width  = cell->coord.n[0];
    layout->tileshape.height = cell->coord.n[1];
    cell += 2;

    layout->viewport.x      = cell->coord.n[0];
    layout->viewport.y      = cell->coord.n[1];
    layout->viewport.width  = cell->coord.n[2];
    layout->viewport.height = cell->coord.n[3];
    ++cell;

    bi.it += sizeof(lparam);
    return 1;
}

//--------------------------------------
// Tiles

static int conf_tileRule(TileRule* rule, UBlockIt& bi)
{
    if ((bi.end - bi.it) < 2)
        return 0;
    if (! ur_is(bi.it, UT_WORD))
        return 0;
    if (! ur_is(bi.it+1, UT_COORD))
        return 0;

    const UCell* rval = bi.it+1;

    rule->name = ur_atom(bi.it);

    if (rval->coord.len > 5) {
        rule->mask         = rval->coord.n[4];
        rule->movementMask = rval->coord.n[5];
    } else {
        rule->mask = 0;
        rule->movementMask = 0;
    }

    rule->speed  = rval->coord.n[0];
    rule->effect = rval->coord.n[1];

    if (rval->coord.len > 3) {
        rule->walkonDirs  = rval->coord.n[2];
        rule->walkoffDirs = rval->coord.n[3];
    } else {
        rule->walkonDirs  = MASK_DIR_ALL;
        rule->walkoffDirs = MASK_DIR_ALL;
    }

    bi.it += 2;
    return 1;
}

static Tile* conf_tile(ConfigBoron* cfg, Tile* tile, int id, UBlockIt& bi)
{
    static const uint8_t tparam[6] = {
        // name   rule   image   animation   directions   numA
        UT_WORD, WORD_NONE, WORD_NONE, WORD_NONE, WORD_NONE, UT_COORD
    };
    if (! validParam(bi, sizeof(tparam), tparam))
        return NULL;

    tile->id = id;
    tile->name = ur_atom(bi.it);

    const UCell* cell = bi.it+1;

    /* Get the rule that applies to the current tile (or "default") */
    UAtom atom = 0;
    if (ur_is(cell, UT_WORD))
        atom = ur_atom(cell);
    tile->rule = cfg->tileRule(atom);
    ++cell;

    /* get the name of the image that belongs to this tile */
    if (ur_is(cell, UT_WORD))
        tile->imageName = ur_atom(cell);
    else {
        string tname("tile_");
        tname += ur_atomCStr(cfg->ut, tile->name);
        tile->imageName = ur_intern(cfg->ut, tname.c_str(), tname.size());
    }
    ++cell;

    if (ur_is(cell, UT_WORD))
        tile->animationRule = ur_atom(cell);
    ++cell;

    // numA: frames opaque flags
    const int16_t* numA = bi.it[5].coord.n;

    // Set frames before calling setDirections().
    int frames = numA[0];
    tile->frames = frames ? frames : 1;

    /* Fill directions if they are specified. */
    if (ur_is(cell, UT_WORD))
        tile->setDirections( ur_atomCStr(cfg->ut, ur_atom(cell)) );

    tile->opaque = numA[1];

    int flags = numA[2];
    tile->foreground      = flags & 2; // usesReplacementTileAsBackground
    tile->waterForeground = flags & 4; // usesWaterReplacementTileAsBackground
    tile->tiledInDungeon  = flags & 8;

    bi.it += sizeof(tparam);
    return tile;
}

static void conf_ultimaSaveIds(ConfigBoron* cfg, UltimaSaveIds* usaveIds,
                               const Tileset* ts, UBlockIt& bi)
{
    int frames = 1;
    int uid = 0;
    int count = 0;

    const UCell* start = bi.it;
    ur_foreach (bi) {
        if (ur_is(bi.it, UT_WORD))
            ++count;
    }
    bi.it = start;

    usaveIds->alloc(256, count, cfg, ts);

    ur_foreach (bi) {
        if (ur_is(bi.it, UT_INT)) {
            frames = ur_int(bi.it);
            continue;
        }
        if (! ur_is(bi.it, UT_WORD)) {
            errorWarning("u4-save-ids expected word!");
            continue;
        }

        /* find the tile this references */
        Symbol tile = ur_atom(bi.it);
        const Tile *t = ts->getByName(tile);
        if (! t)
            errorFatal("Error: tile '%s' was not found in tileset",
                       ur_atomCStr(cfg->ut, tile));

        usaveIds->addId(uid, frames, t->getId());
        uid += frames;
        frames = 1;
    }

#if 0
    printf( "ultimaIdTable[%d]", usaveIds->miCount);
    for(int i = 0; i < usaveIds->miCount; ++i) {
        if ((i & 3) == 0) printf("\n");
        printf(" %d,", usaveIds->ultimaIdTable[i]);
    }
    printf( "\nmoduleIdTable[%d]", usaveIds->uiCount);
    for(int i = 0; i < usaveIds->uiCount; ++i) {
        if ((i & 3) == 0) printf("\n");
        printf(" %d,", usaveIds->moduleIdTable[i]);
    }
#endif
}

//--------------------------------------
// Maps

static void conf_initCity(ConfigBoron* cfg, City* city, UBlockIt& bi)
{
    static const uint8_t cityParam[4] = {
        // name  type  tlk_fname  roles
        UT_STRING, UT_WORD, UT_FILE, BLOCK_NONE
    };
    if (! validParam(bi, sizeof(cityParam), cityParam))
        errorFatal("Invalid city parameters");

    city->name      = ASTR(bi.it[0].series.buf);
    city->tlk_fname = ASTR(bi.it[2].series.buf);
    city->cityType  = ur_atom(bi.it+1);

    if (ur_is(bi.it+3, UT_BLOCK)) {
        UBlockIt rit;
        PersonRole role;

        ur_blockIt(cfg->ut, &rit, bi.it+3);
        ur_foreach (rit) {
            role.role = rit.it->coord.n[0] + NPC_TALKER_COMPANION;
            role.id   = rit.it->coord.n[1];

            city->personroles.push_back(role);
        }
    }

    bi.it += sizeof(cityParam);
}

extern bool isAbyssOpened(const Portal*);
extern bool shrineCanEnter(const Portal*);

Portal* conf_makePortal(ConfigBoron* cfg, UBlockIt& bi)
{
    static const uint8_t pparam[5] = {
        // message  condition  numA  numB  retroActiveDest
        STRING_NONE, WORD_NONE, UT_COORD, UT_COORD, COORD_NONE
    };
    if (! validParam(bi, sizeof(pparam), pparam))
        return NULL;

    // numA: x y z startx starty startlevel
    // numB: destmapid action savelocation transport condition
    const int16_t* numA = bi.it[2].coord.n;
    const int16_t* numB = bi.it[3].coord.n;

    Portal* portal = new Portal;

    portal->portalConditionsMet = NULL;
    portal->retroActiveDest = NULL;

    portal->coords = Coords(numA[0], numA[1], numA[2]);
    portal->start.x = numA[3];
    portal->start.y = numA[4];
    portal->start.z = numA[5];

    portal->destid = (MapId) numB[0];
    portal->trigger_action = (PortalTriggerAction) numB[1];
    portal->saveLocation   = numB[2];
    portal->portalTransportRequisites = (TransportContext) numB[3];

    if (ur_is(bi.it, UT_STRING))
        portal->message = ASTR(bi.it->series.buf);
    else
        portal->message = 0;

    if (ur_is(bi.it+1, UT_WORD)) {
        UAtom cond = ur_atom(bi.it+1);
        if (cond == cfg->sym_shrine)
            portal->portalConditionsMet = &shrineCanEnter;
        else if (cond == cfg->sym_abyss)
            portal->portalConditionsMet = &isAbyssOpened;
        else
            errorFatal("Unknown portal condition %s", ur_atomCStr(cfg->ut, cond));
    }

    portal->exitPortal = 0;     // conf.getBool("exits");

    const UCell* rad = bi.it + 4;
    if (ur_is(rad, UT_COORD)) {
        portal->retroActiveDest = new PortalDestination;
        portal->retroActiveDest->coords = Coords(rad->coord.n[0],
                                                 rad->coord.n[1],
                                                 rad->coord.n[2]);
        portal->retroActiveDest->mapid = (MapId) rad->coord.n[3];
    }

    bi.it += sizeof(pparam);
    return portal;
}

// coordC holds (phase, x, y) values.
static void conf_createMoongate(ConfigBoron* cfg, const UCell* coordC) {
    vector<Coords>& moongates = cfg->xcd.moongateList;
    int phase = coordC->coord.n[0];
    if (phase >= (int) moongates.size()) {
        size_t size = phase + 1;
        if (size < 8)
            size = 8;
        moongates.resize(size);
    }
    Coords& coords = moongates[ phase ];
    coords.x = coordC->coord.n[1];
    coords.y = coordC->coord.n[2];
}

static std::pair<Symbol, Coords> conf_initLabel(const UCell* it)
{
    assert(ur_is(it, UT_WORD));
    assert(ur_is(it+1, UT_COORD));

    const int16_t* pos = it[1].coord.n;
    int z = (it[1].coord.len > 2) ? pos[2] : 0;
    return std::pair<Symbol, Coords> (ur_atom(it), Coords(pos[0], pos[1], z));
}

static Map* conf_makeMap(ConfigBoron* cfg, Tileset* tiles, UBlockIt& bi)
{
    static const uint8_t mparam[3] = {
        // fname  numA  numB
        FILE_NONE, UT_COORD, UT_COORD
    };
    if (! validParam(bi, sizeof(mparam), mparam))
        return NULL;

    // numA: id type borderbehavior width height levels
    // numB: chunkwidth chunkheight flags music
    const int16_t* numA = bi.it[1].coord.n;
    const int16_t* numB = bi.it[2].coord.n;

    Map* map;
    Map::Type mtype = (Map::Type) numA[1];

    switch(mtype) {
        case Map::WORLD:
            map = new Map;
            break;
        case Map::COMBAT:
            map = new CombatMap;
            break;
        case Map::SHRINE:
            map = new Shrine;
            break;
        case Map::DUNGEON:
            map = new Dungeon;
            break;
        case Map::CITY:
            map = new City;
            break;
        default:
            errorFatal("Invalid map type used");
            return NULL;
    }
    if (! map)
        return NULL;

    if (ur_is(bi.it, UT_FILE))
        map->fname = ASTR(bi.it->series.buf);   // Data from original U4 file.
    else
        map->fname = UR_INVALID_BUF;            // Data from Config::mapFile.
    map->id     = (MapId) numA[0];
    map->type   = mtype;
    map->border_behavior = numA[2];
    map->width  = numA[3];
    map->height = numA[4];
    map->levels = numA[5];
    map->chunk_width  = numB[0];
    map->chunk_height = numB[1];
    map->flags        = numB[2];
    map->music        = numB[3];

    /*
    map->offset = conf.getInt("offset");

    if (isCombatMap(map)) {
        CombatMap *cm = dynamic_cast<CombatMap*>(map);
        cm->setContextual(conf.getBool("contextual"));
    }

    if (cname == "compressedchunk")
        map->compressed_chunks.push_back( (*it).getInt("index") );
    */

    if (map->type == Map::WORLD || map->type == Map::CITY)
        map->flags |= SHOW_AVATAR;

    map->tileset = tiles;

    bi.it += sizeof(mparam);

    switch(mtype) {
        case Map::SHRINE:
        {
            Shrine* sc = (Shrine*) map;
            assert(ur_is(bi.it, UT_WORD));
            sc->mantra = ur_atom(bi.it);
            ++bi.it;
            sc->virtue = (Virtue) ur_int(bi.it);
            ++bi.it;
        }
            break;
        case Map::DUNGEON:
        {
            Dungeon* sc = (Dungeon*) map;
            sc->rooms    = NULL;
            sc->roomMaps = NULL;
            assert(ur_is(bi.it, UT_STRING));
            sc->name     = ASTR(bi.it->series.buf);
            ++bi.it;
            sc->n_rooms  = ur_int(bi.it);
            ++bi.it;
        }
            break;
        case Map::CITY:
            conf_initCity(cfg, (City*) map, bi);
            break;

        default:
            break;
    }

    // Set labels, portals, & moongates if present.
    if ((bi.end - bi.it) > 2 && ! ur_is(bi.it, UT_FILE)) {
        UBlockIt ext;

        // labels
        if (ur_is(bi.it, UT_BLOCK)) {
            ur_blockIt(cfg->ut, &ext, bi.it);
            ur_foreach (ext) {
                map->labels.insert(conf_initLabel(ext.it));
                ++ext.it;
            }
        }
        ++bi.it;

        // portals
        if (ur_is(bi.it, UT_BLOCK)) {
            Portal* portal;
            ur_blockIt(cfg->ut, &ext, bi.it);
            while ((portal = conf_makePortal(cfg, ext)))
                map->portals.push_back(portal);
        }
        ++bi.it;

        // moongates
        if (ur_is(bi.it, UT_BLOCK)) {
            ur_blockIt(cfg->ut, &ext, bi.it);
            ur_foreach (ext) {
                conf_createMoongate(cfg, ext.it);
            }
        }
        ++bi.it;
    }

#ifdef DUMP_CONFIG
    dumpMap(cfg, map);
#endif
    return map;
}

//--------------------------------------
// Creature

static Creature* conf_creature(ConfigBoron* cfg, Tileset* ts, UBlockIt& bi)
{
    static const uint8_t cparam[4] = {
        // numA  name  tile  numB
        UT_COORD, UT_STRING, UT_WORD, UT_COORD
    };
    if (! validParam(bi, sizeof(cparam), cparam))
        return NULL;

    // numA: id leader spawnsOnDeath basehp exp encounterSize
    // numB: attr movementAttr resists flags u4SaveId
    const int16_t* numA = bi.it[0].coord.n;
    const int16_t* numB = bi.it[3].coord.n;

    int attr;
    Creature* cr = new Creature;

    cr->name = ASTR(bi.it[1].series.buf);
    cr->setTile(ts->getByName( ur_atom(bi.it+2) ));
    cr->setHitTile(cfg->sym_hitFlash);
    cr->setMissTile(cfg->sym_missFlash);
    cr->worldrangedtile = SYM_UNSET;

    cr->id      = numA[0];
    cr->spawn   = numA[2];
    cr->basehp  = numA[3];
    cr->xp      = numA[4];
    cr->encounterSize = numA[5];

    /* Get the leader if it's been included, otherwise the leader is itself */
    cr->leader = numA[1] ? numA[1] : cr->id;

    /* adjust basehp according to battle difficulty setting */
    if (xu4.settings->battleDiff == BattleDiff_Hard)
        cr->basehp *= 2;
    else if (xu4.settings->battleDiff == BattleDiff_Expert)
        cr->basehp *= 4;

    attr = ((uint16_t*) numB)[0];
    cr->movementAttr = (CreatureMovementAttrib) numB[1];
    cr->resists    = numB[2];
    cr->ranged     = numB[3] & 1;
    cr->leavestile = numB[3] & 2;
    cr->u4SaveId   = numB[4];

    if (cr->spawn)
        attr |= MATTR_SPAWNSONDEATH;

    bi.it += sizeof(cparam);

    // Set the tiles if they are present.
    if ((bi.end - bi.it) > 3 && ! ur_is(bi.it, UT_COORD)) {
        if (ur_is(bi.it, UT_WORD)) {
            if (ur_atom(bi.it) == cfg->sym_random)
                attr |= MATTR_RANDOMRANGED;
            else
                cr->rangedhittile = ur_atom(bi.it);
        }
        ++bi.it;
        if (ur_is(bi.it, UT_WORD)) {
            if (ur_atom(bi.it) == cfg->sym_random)
                attr |= MATTR_RANDOMRANGED;
            else
                cr->rangedmisstile = ur_atom(bi.it);
        }
        ++bi.it;
        if (ur_is(bi.it, UT_WORD))
            cr->camouflageTile = ur_atom(bi.it);
        ++bi.it;
        if (ur_is(bi.it, UT_WORD))
            cr->worldrangedtile = ur_atom(bi.it);
        ++bi.it;
    }

    cr->mattr = (CreatureAttrib) attr;

    /* Figure out which 'slowed' function to use */
    if (cr->sails())
        /* sailing creatures (pirate ships) */
        cr->slowedType = SLOWED_BY_WIND;
    else if (cr->flies() || cr->isIncorporeal())
        /* flying creatures (dragons, bats, etc.) and incorporeal creatures (ghosts, zorns) */
        cr->slowedType = SLOWED_BY_NOTHING;
    else
        cr->slowedType = SLOWED_BY_TILE;

#ifdef DUMP_CONFIG
    dumpCreature(cfg, cr);
#endif
    return cr;
}

//--------------------------------------
// Items (weapons & armor)

const char* Armor::getName() const {
    return xu4.config->confString(name);
}

const char* Weapon::getName() const {
    return xu4.config->confString(name);
}

const char* Weapon::getAbbrev() const {
    return xu4.config->confString(abbr);
}

static Armor* conf_armor(UThread* ut, int type, Armor* arm, UBlockIt& bi) {
    static const uint8_t aparam[] = {UT_STRING, UT_COORD};
    if (! validParam(bi, sizeof(aparam), aparam))
        return NULL;

    arm->type    = (ArmorType) type;
    arm->name    = ASTR_UT(bi.it->series.buf);
    arm->canuse  = bi.it[1].coord.n[1];
    arm->defense = bi.it[1].coord.n[0];

    bi.it += sizeof(aparam);
    return arm;
}

static Weapon* conf_weapon(const ConfigBoron* cfg, int type, UBlockIt& bi) {
    static const uint8_t wparam[] = {UT_STRING, UT_STRING, UT_COORD};
    if (! validParam(bi, sizeof(wparam), wparam))
        return NULL;

    Weapon* wpn = new Weapon;

    wpn->type   = (WeaponType) type;
    wpn->name   = ASTR(bi.it[1].series.buf);
    wpn->abbr   = ASTR(bi.it[0].series.buf);
    wpn->canuse = bi.it[2].coord.n[3];
    wpn->range  = bi.it[2].coord.n[0];
    wpn->damage = bi.it[2].coord.n[1];
    wpn->flags  = bi.it[2].coord.n[2];

    wpn->hitTile   = cfg->sym_hitFlash;
    wpn->missTile  = cfg->sym_missFlash;
    wpn->leaveTile = SYM_UNSET;

    bi.it += sizeof(wparam);

    // Set the hit, miss, & leave tiles if they are specified.
    if ((bi.end - bi.it) > 2 && ! ur_is(bi.it, UT_STRING)) {
        if (ur_is(bi.it, UT_WORD))
            wpn->hitTile = ur_atom(bi.it);
        ++bi.it;
        if (ur_is(bi.it, UT_WORD))
            wpn->missTile = ur_atom(bi.it);
        ++bi.it;
        if (ur_is(bi.it, UT_WORD))
            wpn->leaveTile = ur_atom(bi.it);
        ++bi.it;
    }

#ifdef DUMP_CONFIG
    dumpWeapon(cfg, wpn);
#endif

    return wpn;
}

//--------------------------------------

#include "script_boron.cpp"

const void* Config::scriptEvalArg(const char* fmt, ...)
{
    UBuffer* buf = &static_cast<ConfigBoron*>(this)->evalBuf;
    int bufSize = ur_avail(buf);
    va_list arg;
    int n;

    va_start(arg, fmt);
    n = vsnprintf(buf->ptr.c, bufSize, fmt, arg);
    va_end(arg);

    if (n > 0 && n < bufSize)
        return script_eval(CX->ut, buf->ptr.c, n);
    return NULL;
}

//--------------------------------------

static void mergeGraphics(ConfigBoron* cfg, const UCell* rep)
{
    UBlockIt dest;
    UBlockIt src;
    UBuffer* destBlk;
    UCell* d2;
    UAtom name;
    const int paramLen = 5;

    // See imageParam (below) for expected values.

    destBlk = (UBuffer*) cfg->blockIt(&dest, CI_GRAPHICS);
    if (destBlk && ur_is(rep, UT_BLOCK)) {
        ur_blockIt(cfg->ut, &src, rep);
        for ( ; src.it != src.end; src.it += paramLen) {
            if (! ur_is(src.it, UT_WORD))
                continue;

            name = ur_atom(src.it);
            d2 = (UCell*) dest.it;
            for ( ; d2 != dest.end; d2 += paramLen) {
                if (ur_atom(d2) == name)
                    break;
            }
            if (d2 == dest.end) {
                // Name not found - append new entry.
                ur_blkAppendCells(destBlk, src.it, paramLen);

                // Reacquire pointers.
                dest.it  = destBlk->ptr.cell;
                dest.end = dest.it + destBlk->used;
            } else {
                // Name found - overwrite existing entry.
                memcpy(d2, src.it, sizeof(UCell) * paramLen);
            }
        }
    }
}

// Load and merge config.
static const char* confLoader(FILE* fp, const CDIEntry* ent, void* user)
{
    UCell* res;
    ConfigBoron* cfg = (ConfigBoron*) user;
    UThread* ut = cfg->ut;
    uint8_t* confBuf = cdi_loadPakChunk(fp, ent);
    if (! confBuf)
        return "Read CONF failed";

    res = ur_stackTop(ut);
    if (ur_unserialize(ut, confBuf, confBuf + ent->bytes, res) == UR_OK) {
        res = ur_buffer(res->series.buf)->ptr.cell;
        if (ur_is(res, UT_CONTEXT)) {
            if (cfg->configN == UR_INVALID_BUF) {
                cfg->configN = res->series.buf;
                ur_hold(cfg->configN);  // Keep forever.
            } else {
                UBuffer* cur = ur_buffer(cfg->configN);
                UBuffer* ctx = ur_buffer(res->series.buf);
                int i;

                // Replace existing config context values (except for music).
                for (i = 0; i < CI_COUNT; ++i) {
                    if (i == CI_MUSIC)
                        continue;

                    res = ur_ctxCell(ctx, i);

                    if (i == CI_GRAPHICS) {
                        mergeGraphics(cfg, res);
                        continue;
                    }

                    if (! ur_is(res, UT_NONE)) {
                        cur->ptr.cell[i] = *res;
                        //printf("KR config overlay %d\n", i);
                    }
                }
            }
        } else
            return "Serialized CONF context not found";
    } else
        return "Unserialize CONF failed";

    free(confBuf);
    return NULL;
}

ConfigBoron::ConfigBoron(const char* renderPath, const char* modulePath)
{
    UBlockIt bi;
    const char* error = NULL;


    backend = &xcd;
    xcd.creatureTileIndex = NULL;
    xcd.tileset = NULL;
    memset(&xcd.usaveIds, 0, sizeof(xcd.usaveIds));
    ur_binInit(&evalBuf, 1024);

    {
    UEnvParameters param;
    ut = boron_makeEnv( boron_envParam(&param) );
    if (! ut)
        errorFatal("boron_makeEnv failed");
    }

    ur_internAtoms(ut, "hit_flash miss_flash random shrine abyss"
                       " imageset tileanims _cel rect", &sym_hitFlash);

    mod_init(&mod, 3);
    configN = UR_INVALID_BUF;

    if (renderPath) {
        error = mod_addLayer(&mod, renderPath, NULL, NULL, NULL);
        if (error)
            errorFatal("%s (%s)", error, renderPath);
    }

    error = mod_addLayer(&mod, modulePath, NULL, confLoader, this);
    if (error)
        errorFatal("%s (%s)", error, modulePath);

    npcTalk_init(&xcd.talk, ut);


    // Load primary elements.

    // layouts
    if (blockIt(&bi, CI_LAYOUTS)) {
        Layout lo;
        while (conf_loadLayout(ut, bi, &lo))
            xcd.layouts.push_back(lo);
    }

    // armors
    if (blockIt(&bi, CI_ARMORS)) {
        int n = 0;
        xcd.armors = new Armor[ (bi.end - bi.it) / 2 ];
        while ((conf_armor(ut, n, xcd.armors + n, bi))) {
#ifdef DUMP_CONFIG
            dumpArmor(this, xcd.armors + n);
#endif
            ++n;
        }
        xcd.armorCount = n;
    } else {
        xcd.armors = NULL;
        xcd.armorCount = 0;
    }

    // weapons
    if (blockIt(&bi, CI_WEAPONS)) {
        Weapon* wpn;
        while ((wpn = conf_weapon(this, xcd.weapons.size(), bi)))
            xcd.weapons.push_back(wpn);
    }

    // tileRules
    if (blockIt(&bi, CI_TILE_RULES)) {
        xcd.tileRuleCount = (bi.end - bi.it) / 2;
        xcd.tileRuleDefault = 0;
        TileRule* rule = xcd.tileRules = new TileRule[ xcd.tileRuleCount ];
        while (conf_tileRule(rule, bi)) {
#ifdef DUMP_CONFIG
            dumpTileRule(this, rule);
#endif
            ++rule;
        }
    }

    // tileset (requires tileRules)
    if (blockIt(&bi, CI_TILESET)) {
        int moduleId;
        int tileCount = (bi.end - bi.it) / 6;
        Tileset* ts = xcd.tileset = new Tileset(tileCount);
        Tile* tile = ts->tiles;

        for (moduleId = 0; moduleId < tileCount; ++moduleId) {
            if (! conf_tile(this, tile, moduleId, bi))
                break;
            ts->nameMap[tile->name] = tile;     // Add tile to nameMap
            ++tile;
        }
        ts->tileCount = moduleId;
    }

    // u4-save-ids
    if (blockIt(&bi, CI_U4SAVEIDS)) {
        conf_ultimaSaveIds(this, &xcd.usaveIds, xcd.tileset, bi);
        // TODO: Free u4-save-ids block.
    }

    // mapList
    if (blockIt(&bi, CI_MAPS)) {
        xcd.mapList.resize((bi.end - bi.it) / 3, NULL);
        Map* map;
        while ((map = conf_makeMap(this, xcd.tileset, bi))) {
            /* Register map; the contents get loaded later, as needed. */
            if (xcd.mapList[map->id])
                errorFatal("A map with id '%d' already exists", map->id);
            xcd.mapList[map->id] = map;
        }
    }

    // creatures
    if (blockIt(&bi, CI_CREATURES)) {
        Creature* cr;
        int id;
        int last = 0;

        xcd.creatures.resize((bi.end - bi.it) / 4, NULL);
        while ((cr = conf_creature(this, xcd.tileset, bi))) {
            id = cr->getId();
            if (id > last)
                last = id;
            xcd.creatures[id] = cr;
        }
        xcd.creatures.resize(last + 1);

        xcd.creatureTileIndex = makeCreatureTileIndex(xcd.creatures,
                                                      xcd.tileset,
                                                      xcd.usaveIds);
    }

    // vendors
    {
    const UBuffer* ctx = ur_buffer(configN);
    const UCell* cell = ur_ctxCell(ctx, CI_VENDORS);
    if (ur_is(cell, UT_BLOCK)) {
        itemIdN = script_init(ut, cell);
        ur_setId(cell, UT_NONE);    // Let block be recycled.
    }
    }
}

ConfigBoron::~ConfigBoron()
{
    vector<Map *>::iterator mit;
    foreach (mit, xcd.mapList)
        delete *mit;

    vector<Creature *>::iterator cit;
    foreach (cit, xcd.creatures)
        delete *cit;
    delete[] xcd.creatureTileIndex;

    delete[] xcd.armors;

    vector<Weapon *>::iterator wit;
    foreach (wit, xcd.weapons)
        delete *wit;

    delete[] xcd.tileRules;
    delete xcd.tileset;
    xcd.usaveIds.free();
    ur_binFree(&evalBuf);

    mod_free(&mod);
    boron_freeEnv( ut );
}

//--------------------------------------
// Config Service API

extern "C" int u4find_pathc(const char*, const char*, char*, size_t);

// Create Config service.
Config* configInit(const char* module) {
    const char* ext;
    char rpath[512];
    char mpath[512];

    int renderFound = u4find_pathc("render.pak", "", rpath, sizeof(rpath));

    int len = strlen(module) - 4;
    ext = (len > 0 && strcmp(module + len, ".mod") == 0) ? "" : ".mod";
    if (! u4find_pathc(module, ext, mpath, sizeof(mpath)))
        errorFatal("Cannot find module %s", module);

    return new ConfigBoron(renderFound ? rpath : NULL, mpath);
}

void configFree(Config* conf) {
    delete conf;
}

const char* Config::symbolName( Symbol s ) const {
    return ur_atomCStr(CX->ut, s);
}

Symbol Config::intern( const char* name ) {
    return ur_intern(CX->ut, name, strlen(name));
}

/**
 * Get symbols of the given names.
 *
 * \param table     Return area for symbols of the names.
 * \param count     Maximum number of symbols table can hold.
 * \param names     String containing names separated by whitespace.
 */
void Config::internSymbols(Symbol* table, uint16_t count, const char* names) {
    (void) count;
    ur_internAtoms(CX->ut, names, table);
}

/*
 * Return string of configuration StringId.
 */
const char* Config::confString( StringId id ) const {
    UThread* ut = CX->ut;
    UBuffer* str = ur_buffer(id);
    assert(str->type == UT_STRING);
    return str->ptr.c;
}

/*
 * Return pointer to 16 RGBA values.
 */
const RGBA* Config::egaPalette() {
    const UBuffer* buf = CX->buffer(CI_EGA_PALETTE, UT_BINARY);
    if (buf)
        return (RGBA*) buf->ptr.b;
    return NULL;
}

const Layout* Config::layouts( uint32_t* plen ) const {
    *plen = CB->layouts.size();
    return &CB->layouts.front();
}

UThread* Config::boronThread() const {
    return CX->ut;
}

/*
 * Return internal item-id of Symbol.  Used by script functions.
 */
int Config::scriptItemId(Symbol name) {
    UThread* ut = CX->ut;
    const UBuffer* ctx = ur_buffer(CX->itemIdN);
    int n = ur_ctxLookup(ctx, name);
    if (n >= 0) {
        const UCell* cell = ur_ctxCell(ctx, n);
        assert(ur_is(cell, UT_INT));
        return ur_int(cell);
    }
    return 0;
}

/*
 * Load an NPC Talk chunk from the game module (or overlay).
 * Return block buffer index or UR_INVALID_BUF.
 */
int32_t Config::npcTalk(uint32_t appId) {
    NpcTalkCache& talk = CB->talk;
    UThread* ut = CX->ut;
    UCell* talkCell = ur_buffer(talk.blkN)->ptr.cell;
    int n;
    for (n = 0; n < TALK_CACHE_SIZE; ++n) {
        if (talk.appId[n] == appId) {
            talk.lastUsed = n;
            return talkCell[n].series.buf;
        }
    }

    const CDIEntry* ent = mod_findAppId(&CX->mod, appId);
    if (ent) {
        uint8_t* buf;
        UStatus ok;
        FILE* fp = fopen(mod_path(&CX->mod, ent), "rb");
        if (fp) {
            buf = cdi_loadPakChunk(fp, ent);
            fclose(fp);
            if (buf) {
                talk.lastUsed ^= 1;
                talkCell += talk.lastUsed;
                ok = ur_unserialize(ut, buf, buf + ent->bytes, talkCell);
                free(buf);
                return (ok == UR_OK) ? talkCell->series.buf : UR_INVALID_BUF;
            }
        }
    }
    return UR_INVALID_BUF;
}

/*
 * Return the data from a given source filename.
 * The caller must free() this buffer when finished with it.
 */
void* Config::loadFile(const char* sourceFilename) const {
    const CDIEntry* ent = mod_fileEntry(&CX->mod, sourceFilename);
    if (ent) {
        void* data = malloc(ent->bytes);
        if (data) {
            FILE* fp = fopen(mod_path(&CX->mod, ent), "rb");
            if (fp) {
                fseek(fp, ent->offset, SEEK_SET);
                size_t len = fread(data, 1, ent->bytes, fp);
                fclose(fp);
                if (len == ent->bytes)
                    return data;
            }
            free(data);
        }
    }
    return NULL;
}

const char* Config::modulePath(const CDIEntry* ent) const {
    return mod_path(&CX->mod, ent);
}

/*
 * Return the CDIEntry pointer for a given source filename.
 */
const CDIEntry* Config::fileEntry(const char* sourceFilename) const {
    return mod_fileEntry(&CX->mod, sourceFilename);
}

/*
 * Return a CDIEntry pointer for the given image id (ImageInfo::filename)
 */
const CDIEntry* Config::imageFile( const char* id ) const {
    uint32_t appId = CDI32(id[0], id[1], id[2], id[3]);
    return mod_findAppId(&CX->mod, appId);
}

/*
 * Return a CDIEntry pointer for the given Map::id.
 */
const CDIEntry* Config::mapFile( uint32_t id ) const {
    uint32_t appId = CDI32('M', 'A', (id >> 8), (id & 255));
    return mod_findAppId(&CX->mod, appId);
}

/*
 * Return a CDIEntry pointer for the given MusicTrack id (see sound.h)
 */
const CDIEntry* Config::musicFile( uint32_t id ) const {
    --id;       // Music file numbering starts at 0.
    uint32_t appId = CDI32('M', 'U', (id >> 8), (id & 255));
    return mod_findAppId(&CX->mod, appId);
}

/*
 * Return a CDIEntry pointer for the given Sound id (see sound.h)
 */
const CDIEntry* Config::soundFile( uint32_t id ) const {
    uint32_t appId = CDI32('S', 'O', (id >> 8), (id & 255));
    return mod_findAppId(&CX->mod, appId);
}

/*
 * Return an Armor pointer for the given ArmorType id (see savegame.h)
 */
const Armor* Config::armor( uint32_t id ) {
    if (id < CB->armorCount)
        return CB->armors + id;
    return NULL;
}

/*
 * Return a Weapon pointer for the given WeaponType id (see savegame.h)
 */
const Weapon* Config::weapon( uint32_t id ) {
    if (id < CB->weapons.size())
        return CB->weapons[id];
    return NULL;
}

int Config::armorType( const char* name ) {
    const Armor* it  = CB->armors;
    const Armor* end = it + CB->armorCount;
    for( ; it != end; ++it) {
        if (strcasecmp(confString(it->name), name) == 0)
            return it - CB->armors;
    }
    return -1;
}

int Config::weaponType( const char* name ) {
    vector<Weapon*>::const_iterator it;
    foreach (it, CB->weapons) {
        if (strcasecmp(confString((*it)->name), name) == 0)
            return it - CB->weapons.begin();
    }
    return -1;
}

/**
 * Returns the creature of the corresponding id or NULL if not found.
 */
const Creature* Config::creature( uint32_t id ) const {
    if (id < CB->creatures.size())
        return CB->creatures[id];
    return NULL;
}

/**
 * Returns the creature of the corresponding TileId or NULL if not found.
 */
const Creature* Config::creatureOfTile( TileId tid ) const {
    const ConfigData* cd = CB;
    assert(tid < cd->tileset->tileCount);

    uint16_t n = cd->creatureTileIndex[tid];
    if (n < cd->creatures.size())
        return cd->creatures[n];
    //printf("creatureOfTile %d NULL\n", tid);
    return NULL;
}

const Creature* const* Config::creatureTable( uint32_t* plen ) const {
    *plen = CB->creatures.size();
    return &CB->creatures.front();
}

/*
 * Get rule by name.  If there is no such named rule, then the "default" rule
 * is returned.
 */
const TileRule* Config::tileRule( Symbol name ) const {
    const TileRule* it = CB->tileRules;
    const TileRule* end = it + CB->tileRuleCount;
    for (; it != end; ++it) {
        if (it->name == name)
            return it;
    }
    return CB->tileRules + CB->tileRuleDefault;
}

const Tileset* Config::tileset() const {
    return CB->tileset;
}

const UltimaSaveIds* Config::usaveIds() const {
    return &CB->usaveIds;
}

extern bool loadMap(Map *map, FILE* sav);

Map* Config::map(uint32_t id) {
    if (id >= CB->mapList.size())
        return NULL;

    Map* rmap = CB->mapList[id];
    /* if the map hasn't been loaded yet, load it! */
    if (! rmap->data) {
        if (! loadMap(rmap, NULL))
            errorFatal("loadMap failed to read \"%s\" (type %d)",
                       confString(rmap->fname), rmap->type);
    }
    return rmap;
}

// Load map from saved game.
Map* Config::restoreMap(uint32_t id) {
    if (id >= CB->mapList.size())
        return NULL;

    Map* rmap = CB->mapList[id];
    if (! rmap->data) {
        FILE* sav = NULL;
        bool ok;

        if (rmap->type == Map::DUNGEON) {
            string path(xu4.settings->getUserPath() + DNGMAP_SAV);
            sav = fopen(path.c_str(), "rb");
        }
        ok = loadMap(rmap, sav);
        if (sav)
            fclose(sav);
        if (! ok)
            errorFatal("loadMap failed to read \"%s\" (type %d)",
                       confString(rmap->fname), rmap->type);
    }
    return rmap;
}

const Coords* Config::moongateCoords(int phase) const {
    if (phase < (int) CB->moongateList.size())
        return &CB->moongateList[ phase ];
    return NULL;
}

//--------------------------------------
// Graphics config

int Config::atlasImages(StringId spec, AtlasSubImage* images, int max) {
    UCell cell;
    UBlockIt bi;
    UAtom atomRect = CX->sym_rect;
    int prevOp = AEDIT_NOP;
    int count = 0;

    // Spec is actually a block! not a string!.
    ur_setId(&cell, UT_BLOCK);
    ur_setSeries(&cell, spec, 0);

    ur_blockIt(CX->ut, &bi, &cell);
    ur_foreach (bi) {
        if (ur_is(bi.it, UT_WORD) && ur_is(bi.it+1, UT_COORD)) {
            images->name = ur_atom(bi.it);
            ++bi.it;
next:
            images->x = bi.it->coord.n[0];
            images->y = bi.it->coord.n[1];
            ++images;
            if (++count >= max)
                break;
        }
        else if (ur_is(bi.it, UT_OPTION) && ur_is(bi.it+1, UT_COORD)) {
            images->name = prevOp =
                (ur_atom(bi.it) == atomRect) ? AEDIT_RECT : AEDIT_BRUSH;
            ++bi.it;
            images->w = bi.it->coord.n[2];
            images->h = bi.it->coord.n[3];
            goto next;
        }
        else if (ur_is(bi.it, UT_COORD)) {
            images->name = prevOp;
            images->w = bi.it->coord.n[2];
            images->h = bi.it->coord.n[3];
            goto next;
        }
    }
    return count;
}

static ImageInfo* loadImageInfo(const ConfigBoron* cfg, UBlockIt& bi) {
    static const uint8_t atlasParam[5] = {
        // name  'atlas   numA      spec
        UT_WORD, UT_WORD, UT_COORD, UT_BLOCK, UT_NONE
    };
    static const uint8_t imageParam[5] = {
        // name  filename   numA      numB
        UT_WORD, UT_STRING, UT_COORD, UT_COORD, BLOCK_NONE
    };
    int isAtlas = 0;

    if (! validParam(bi, sizeof(imageParam), imageParam)) {
        if (validParam(bi, sizeof(atlasParam), atlasParam))
            isAtlas = 1;
        else
            errorFatal("Invalid image parameters");
    }

    const int16_t* numA = bi.it[2].coord.n;     // width height (depth)

#if 0
    UThread* ut = cfg->ut;
    UBuffer* str = ur_buffer(bi.it[1].series.buf);
    uint8_t* cp = str->ptr.b;
    printf("KR image 0x%02x%02x%02x%02x\n", cp[0], cp[1], cp[2], cp[3]);
#endif

    ImageInfo* info = new ImageInfo;
    info->name     = ur_atom(bi.it);
    info->resGroup = 0;
    info->width    = numA[0];
    info->height   = numA[1];
    info->subImageCount = 0;
    info->image    = NULL;
    info->subImages = NULL;

    if (isAtlas) {
        // An atlas ImageInfo is denoted by filetype FTYPE_ATLAS.
        // The filename is the spec. block UIndex, not a string!

        info->filename = bi.it[3].series.buf;
        info->depth    = 0;
        info->filetype = FTYPE_ATLAS;
        info->tiles    = 0;
        info->fixup    = FIXUP_NONE;
    } else {
        const int16_t* numB = bi.it[3].coord.n;     // filetype tiles fixup

        info->filename = ASTR(bi.it[1].series.buf);
        info->depth    = numA[2];
        info->filetype = numB[0];
        info->tiles    = numB[1];
        info->fixup    = numB[2];
    }

    // Optional subimages block!
    if (ur_is(bi.it+4, UT_BLOCK)) {
        UBlockIt sit;
        SubImage* subimage;
        int n = 0;
        int celCount;

        ur_blockIt(cfg->ut, &sit, bi.it+4);

        info->subImageCount = (sit.end - sit.it) / 2;
        info->subImages = subimage = new SubImage[info->subImageCount];

        ur_foreach (sit) {
            subimage->name = ur_atom(sit.it);
            if (subimage->name != cfg->sym_Ucel)
                info->subImageIndex[subimage->name] = n;
            ++n;

            ++sit.it;
            numA = sit.it->coord.n;
            subimage->x      = numA[0];
            subimage->y      = numA[1];
            subimage->width  = numA[2];
            subimage->height = numA[3];
            celCount         = (sit.it->coord.len > 4) ? numA[4] : 1;

            subimage->celCount = celCount;
#ifndef GPU_RENDER
            // Animated tiles denoted by height. TODO: Eliminate this.
            if (celCount > 1)
                subimage->height *= celCount;
#endif

            ++subimage;
        }
    }

    assert(sizeof(atlasParam) == sizeof(imageParam));
    bi.it += sizeof(imageParam);
    return info;
}

/*
 * Return ImageSet pointer which caller must delete.
 */
ImageSet* Config::newImageSet() const {
    const ConfigBoron* cfg = CX;
    UBlockIt sit;

    if (! cfg->blockIt(&sit, CI_GRAPHICS))
        return NULL;

    ImageSet* set = new ImageSet;

    std::map<Symbol, ImageInfo *>::iterator dup;
    while (sit.it != sit.end) {
        ImageInfo *info = loadImageInfo(cfg, sit);
        dup = set->info.find(info->name);
        if (dup != set->info.end()) {
            delete dup->second;
            dup->second = info;
        } else
            set->info[info->name] = info;
    }

    return set;
}

static void conf_loadTileAnimSet(const ConfigBoron* cfg, TileAnimSet* tas,
                                 UBlockIt& bi)
{
    UThread* ut = cfg->ut;
    UBinaryIter bin;
    const UCell* cell;
    TileAnimTransform* tf;
    int i, count, tfN;

    if ((bi.end - bi.it) < 3)
        return;

    cell = bi.end - 1;
    if (! ur_is(cell, UT_BINARY))
        return;
    ur_binSlice(ut, &bin, cell);
    tf = (TileAnimTransform*) bin.it;

    ur_foreach (bi) {
        if (! ur_is(bi.it, UT_WORD))
            break;

        TileAnim* anim = new TileAnim;

        anim->name = ur_atom(bi.it);
        ++bi.it;

        tfN = bi.it->coord.n[0];
        anim->random = bi.it->coord.n[1];

        cell = bi.it+1;
        count = ur_is(cell, UT_WORD) ? cell[1].coord.n[0]
                                : bin.buf->used / sizeof(TileAnimTransform);
        count -= tfN;

        for (i = 0; i < count; ++tfN, ++i)
            anim->transforms.push_back(tf + tfN);

        tas->tileanims[anim->name] = anim;
    }
}

/*
 * Return TileAnimSet pointer which caller must delete.
 */
TileAnimSet* Config::newTileAnims() const {
    UBlockIt bi;
    if (CX->blockIt(&bi, CI_TILEANIM)) {
        TileAnimSet* tanim = new TileAnimSet;
        conf_loadTileAnimSet(CX, tanim, bi);
        return tanim;
    }
    return NULL;
}
