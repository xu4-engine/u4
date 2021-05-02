/*
 * $Id$
 */

#include <cstdio>
#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <cctype>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/valid.h>
#include <libxml/xmlIO.h>
#include <libxml/xinclude.h>
#include <libxml/xpath.h>

#if defined(MACOSX) || defined(IOS)
#include <CoreFoundation/CoreFoundation.h>
#endif

// we rely on xinclude support
#ifndef LIBXML_XINCLUDE_ENABLED
#error "xinclude not available: libxml2 must be compiled with xinclude support"
#endif

#include "config.h"
#include "city.h"
#include "dungeon.h"
#include "error.h"
#include "imageloader.h"
#include "imagemgr.h"
#include "map.h"
#include "moongate.h"
#include "names.h"
#include "portal.h"
#include "savegame.h"
#include "settings.h"
#include "shrine.h"
#include "sound.h"
#include "tile.h"
#include "tileset.h"
#include "weapon.h"
#include "u4file.h"
#include "xu4.h"
#include "support/SymbolTable.h"

using namespace std;

extern bool verbose;

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
// XML Backend

struct XMLConfig
{
    SymbolTable sym;
    xmlDocPtr doc;
    string sbuf;        // Temporary buffer for const char* return values.
    vector<const char*> sarray;   // Temp. buffer for const char** values.
    RGBA* egaColors;
    vector<string> musicFiles;
    vector<string> soundFiles;
    vector<string> schemeNames;
    vector<Armor*> armors;
    vector<Weapon*> weapons;
    vector<Creature *> creatures;
    vector<Map *> mapList;

    TileRule* tileRules;
    uint16_t tileRuleCount;
    int16_t  tileRuleDefault;
    Tileset* tileset;
    UltimaSaveIds usaveIds;
};

struct ConfigXML : public Config {
    ConfigXML();
    ~ConfigXML();

    XMLConfig xcd;
};

#define CB  static_cast<XMLConfig*>(backend)

static const char* configXmlPath = "config.xml";

ConfigElement Config::getElement(const string &name) const {
    xmlXPathContextPtr context;
    xmlXPathObjectPtr result;

    string path = "/config/" + name;
    context = xmlXPathNewContext(CB->doc);
    result = xmlXPathEvalExpression(reinterpret_cast<const xmlChar *>(path.c_str()), context);
    if(xmlXPathNodeSetIsEmpty(result->nodesetval))
        errorFatal("no match for xpath %s\n", path.c_str());

    xmlXPathFreeContext(context);

    if (result->nodesetval->nodeNr > 1)
        errorWarning("more than one match for xpath %s\n", path.c_str());

    xmlNodePtr node = result->nodesetval->nodeTab[0];
    xmlXPathFreeObject(result);

    return ConfigElement(node);
}

static Symbol propertySymbol(SymbolTable& sym, const ConfigElement& ce, const char* name) {
    Symbol ns;
    xmlChar* prop = xmlGetProp(ce.getNode(), (const xmlChar*) name);
    if (! prop)
        return SYM_UNSET;
    ns = sym.intern((const char*) prop);
    xmlFree(prop);
    return ns;
}

Symbol Config::propSymbol(const ConfigElement& ce, const char* name) const {
    return propertySymbol(CB->sym, ce, name);
}

static void *conf_fileOpen(const char *filename) {
    void *result;
    string pathname(u4find_conf(filename));

    if (pathname.empty())
        return NULL;
    result = xmlFileOpen(pathname.c_str());

    if (verbose)
        printf("xml parser opened %s: %s\n", pathname.c_str(), result ? "success" : "failed");

    return result;
}

static void conf_accumError(void *l, const char *fmt, ...) {
    string* errorMessage = static_cast<string *>(l);
    char buffer[1000];
    va_list args;

    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    errorMessage->append(buffer);
}

//--------------------------------------
// Tiles

static void conf_tileRule(SymbolTable& sym, TileRule* rule, const ConfigElement &conf) {
    unsigned int i;

    static const struct {
        const char *name;
        unsigned int mask;
    } booleanAttributes[] = {
        { "dispel", MASK_DISPEL },
        { "talkover", MASK_TALKOVER },
        { "door", MASK_DOOR },
        { "lockeddoor", MASK_LOCKEDDOOR },
        { "chest", MASK_CHEST },
        { "ship", MASK_SHIP },
        { "horse", MASK_HORSE },
        { "balloon", MASK_BALLOON },
        { "canattackover", MASK_ATTACKOVER },
        { "canlandballoon", MASK_CANLANDBALLOON },
        { "replacement", MASK_REPLACEMENT },
        { "foreground", MASK_FOREGROUND },
        { "onWaterOnlyReplacement", MASK_WATER_REPLACEMENT},
        { "livingthing", MASK_LIVING_THING }

    };

    static const struct {
        const char *name;
        unsigned int mask;
    } movementBooleanAttr[] = {
        { "swimable", MASK_SWIMABLE },
        { "sailable", MASK_SAILABLE },
        { "unflyable", MASK_UNFLYABLE },
        { "creatureunwalkable", MASK_CREATURE_UNWALKABLE }
    };
    static const char *speedEnumStrings[] = { "fast", "slow", "vslow", "vvslow", NULL };
    static const char *effectsEnumStrings[] = { "none", "fire", "sleep", "poison", "poisonField", "electricity", "lava", NULL };

    rule->mask = 0;
    rule->movementMask = 0;
    rule->speed = FAST;
    rule->effect = EFFECT_NONE;
    rule->walkonDirs = MASK_DIR_ALL;
    rule->walkoffDirs = MASK_DIR_ALL;
    rule->name = propertySymbol(sym, conf, "name");

    for (i = 0; i < sizeof(booleanAttributes) / sizeof(booleanAttributes[0]); i++) {
        if (conf.getBool(booleanAttributes[i].name))
            rule->mask |= booleanAttributes[i].mask;
    }

    for (i = 0; i < sizeof(movementBooleanAttr) / sizeof(movementBooleanAttr[0]); i++) {
        if (conf.getBool(movementBooleanAttr[i].name))
            rule->movementMask |= movementBooleanAttr[i].mask;
    }

    string cantwalkon = conf.getString("cantwalkon");
    if (cantwalkon == "all")
        rule->walkonDirs = 0;
    else if (cantwalkon == "west")
        rule->walkonDirs = DIR_REMOVE_FROM_MASK(DIR_WEST, rule->walkonDirs);
    else if (cantwalkon == "north")
        rule->walkonDirs = DIR_REMOVE_FROM_MASK(DIR_NORTH, rule->walkonDirs);
    else if (cantwalkon == "east")
        rule->walkonDirs = DIR_REMOVE_FROM_MASK(DIR_EAST, rule->walkonDirs);
    else if (cantwalkon == "south")
        rule->walkonDirs = DIR_REMOVE_FROM_MASK(DIR_SOUTH, rule->walkonDirs);
    else if (cantwalkon == "advance")
        rule->walkonDirs = DIR_REMOVE_FROM_MASK(DIR_ADVANCE, rule->walkonDirs);
    else if (cantwalkon == "retreat")
        rule->walkonDirs = DIR_REMOVE_FROM_MASK(DIR_RETREAT, rule->walkonDirs);

    string cantwalkoff = conf.getString("cantwalkoff");
    if (cantwalkoff == "all")
        rule->walkoffDirs = 0;
    else if (cantwalkoff == "west")
        rule->walkoffDirs = DIR_REMOVE_FROM_MASK(DIR_WEST, rule->walkoffDirs);
    else if (cantwalkoff == "north")
        rule->walkoffDirs = DIR_REMOVE_FROM_MASK(DIR_NORTH, rule->walkoffDirs);
    else if (cantwalkoff == "east")
        rule->walkoffDirs = DIR_REMOVE_FROM_MASK(DIR_EAST, rule->walkoffDirs);
    else if (cantwalkoff == "south")
        rule->walkoffDirs = DIR_REMOVE_FROM_MASK(DIR_SOUTH, rule->walkoffDirs);
    else if (cantwalkoff == "advance")
        rule->walkoffDirs = DIR_REMOVE_FROM_MASK(DIR_ADVANCE, rule->walkoffDirs);
    else if (cantwalkoff == "retreat")
        rule->walkoffDirs = DIR_REMOVE_FROM_MASK(DIR_RETREAT, rule->walkoffDirs);

    rule->speed = static_cast<TileSpeed>(conf.getEnum("speed", speedEnumStrings));
    rule->effect = static_cast<TileEffect>(conf.getEnum("effect", effectsEnumStrings));
}

static void conf_tileLoad(const Config* cfg, Tile* tile, const ConfigElement &conf) {
    tile->name = conf.getString("name"); /* get the name of the tile */
    if (tile->name == "brick_floor")
        Tile::dungeonFloorId = tile->getId();

    /* get the animation for the tile, if one is specified */
    if (conf.exists("animation"))
        tile->animationRule = conf.getString("animation");

    /* see if the tile is opaque */
    tile->opaque = conf.getBool("opaque");

    tile->foreground = conf.getBool("usesReplacementTileAsBackground");
    tile->waterForeground = conf.getBool("usesWaterReplacementTileAsBackground");

    /* Get the rule that applies to the current tile (or "default") */
    Symbol sym = SYM_UNSET;
    if (conf.exists("rule"))
        sym = cfg->propSymbol(conf, "rule");
    tile->rule = cfg->tileRule(sym);

    /* get the number of frames the tile has */
    tile->frames = conf.getInt("frames", 1);

    /* get the name of the image that belongs to this tile */
    if (conf.exists("image"))
        tile->imageName = conf.getString("image");
    else
        tile->imageName = string("tile_") + tile->name;

    tile->tiledInDungeon = conf.getBool("tiledInDungeon");

    /* Fill directions if they are specified. */
    if (conf.exists("directions"))
        tile->setDirections(conf.getString("directions"));
}

static void conf_tilesetLoad(Config* cfg, Tileset* ts, const ConfigElement& conf) {
    //ts->name = conf.getString("name");
    if (conf.exists("imageName"))
        ts->imageName = conf.getString("imageName");

    int index = 0;
    int moduleId = 0;
    vector<ConfigElement> children = conf.getChildren();
    vector<ConfigElement>::iterator it;
    foreach (it, children) {
        if (it->getName() != "tile")
            continue;

        Tile* tile = new Tile(moduleId++);
        conf_tileLoad(cfg, tile, *it);

        /* add the tile to our tileset */
        ts->tiles.push_back( tile );
        ts->nameMap[tile->getName()] = tile;

        index += tile->getFrames();
    }
    ts->totalFrames = index;
}

static void conf_ultimaSaveIds(UltimaSaveIds* usaveIds, Tileset* ts, const ConfigElement &conf) {
    int frames;
    int uid = 0;

    usaveIds->alloc(256, 135);

    vector<ConfigElement> children = conf.getChildren();
    vector<ConfigElement>::iterator it;
    foreach (it, children) {
        if (it->getName() != "mapping")
            continue;

        /* find the tile this references */
        string tile = it->getString("tile");
        const Tile *t = ts->getByName(tile);
        if (! t)
            errorFatal("Error: tile '%s' was not found in tileset", tile.c_str());

        if (it->exists("index"))
            uid = it->getInt("index");

        if (it->exists("frames"))
            frames = it->getInt("frames");
        else
            frames = 1;

        usaveIds->addId(uid, frames, t->getId());
        uid += frames;
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

extern bool isAbyssOpened(const Portal*);
extern bool shrineCanEnter(const Portal*);

static PersonRole* conf_initPersonRole(const ConfigElement& conf) {
    static const char *roleEnumStrings[] = {
        "companion", "weaponsvendor", "armorvendor", "foodvendor", "tavernkeeper",
        "reagentsvendor", "healer", "innkeeper", "guildvendor", "horsevendor",
        "lordbritish", "hawkwind", NULL
    };

    PersonRole* role = new PersonRole;
    role->role = conf.getEnum("role", roleEnumStrings) + NPC_TALKER_COMPANION;
    role->id   = conf.getInt("id");
    return role;
}

static void conf_initCity(const ConfigElement& conf, City *city) {
    city->name = conf.getString("name");
    city->type = conf.getString("type");
    city->tlk_fname = conf.getString("tlk_fname");

    vector<ConfigElement> children = conf.getChildren();
    vector<ConfigElement>::iterator it;
    foreach (it, children) {
        if (it->getName() == "personrole")
            city->personroles.push_back(conf_initPersonRole(*it));
    }
}

Portal* conf_initPortal(const ConfigElement& conf) {
    Portal* portal = new Portal;

    portal->portalConditionsMet = NULL;
    portal->retroActiveDest = NULL;

    portal->coords = MapCoords(
        conf.getInt("x"),
        conf.getInt("y"),
        conf.getInt("z", 0));
    portal->destid = static_cast<MapId>(conf.getInt("destmapid"));

    portal->start.x = static_cast<unsigned short>(conf.getInt("startx"));
    portal->start.y = static_cast<unsigned short>(conf.getInt("starty"));
    portal->start.z = static_cast<unsigned short>(conf.getInt("startlevel", 0));

    string prop = conf.getString("action");
    if (prop == "none")
        portal->trigger_action = ACTION_NONE;
    else if (prop == "enter")
        portal->trigger_action = ACTION_ENTER;
    else if (prop == "klimb")
        portal->trigger_action = ACTION_KLIMB;
    else if (prop == "descend")
        portal->trigger_action = ACTION_DESCEND;
    else if (prop == "exit_north")
        portal->trigger_action = ACTION_EXIT_NORTH;
    else if (prop == "exit_east")
        portal->trigger_action = ACTION_EXIT_EAST;
    else if (prop == "exit_south")
        portal->trigger_action = ACTION_EXIT_SOUTH;
    else if (prop == "exit_west")
        portal->trigger_action = ACTION_EXIT_WEST;
    else
        errorFatal("unknown trigger_action: %s", prop.c_str());

    prop = conf.getString("condition");
    if (!prop.empty()) {
        if (prop == "shrine")
            portal->portalConditionsMet = &shrineCanEnter;
        else if (prop == "abyss")
            portal->portalConditionsMet = &isAbyssOpened;
        else
            errorFatal("unknown portalConditionsMet: %s", prop.c_str());
    }

    portal->saveLocation = conf.getBool("savelocation");

    portal->message = conf.getString("message");

    prop = conf.getString("transport");
    if (prop == "foot")
        portal->portalTransportRequisites = TRANSPORT_FOOT;
    else if (prop == "footorhorse")
        portal->portalTransportRequisites = TRANSPORT_FOOT_OR_HORSE;
    else
        errorFatal("unknown transport: %s", prop.c_str());

    portal->exitPortal = conf.getBool("exits");

    vector<ConfigElement> children = conf.getChildren();
    vector<ConfigElement>::iterator it;
    foreach (it, children) {
        if (it->getName() == "retroActiveDest") {
            portal->retroActiveDest = new PortalDestination;

            portal->retroActiveDest->coords = MapCoords(
                it->getInt("x"),
                it->getInt("y"),
                it->getInt("z", 0));
            portal->retroActiveDest->mapid = static_cast<MapId>(it->getInt("mapid"));
        }
    }
    return portal;
}

static void conf_initShrine(const ConfigElement& conf, Shrine *shrine) {
    static const char *virtues[] = {
        "Honesty", "Compassion", "Valor", "Justice", "Sacrifice",
        "Honor", "Spirituality", "Humility", NULL
    };

    shrine->setVirtue(static_cast<Virtue>(conf.getEnum("virtue", virtues)));
    shrine->setMantra(conf.getString("mantra"));
}

static void conf_initDungeon(const ConfigElement &conf, Dungeon *dungeon) {
    dungeon->n_rooms = conf.getInt("rooms");
    dungeon->rooms = NULL;
    dungeon->roomMaps = NULL;
    dungeon->name = conf.getString("name");
}

static void conf_createMoongate(const ConfigElement& conf) {
    int phase = conf.getInt("phase");
    Coords coords(conf.getInt("x"), conf.getInt("y"));

    moongateAdd(phase, coords);
}

static pair<string, MapCoords> conf_initLabel(const ConfigElement& conf) {
    return pair<string, MapCoords> (conf.getString("name"),
         MapCoords(conf.getInt("x"), conf.getInt("y"), conf.getInt("z", 0)));
}

static Map* conf_makeMap(Tileset* tiles, const ConfigElement& conf) {
    static const char *mapTypeStrings[] = {
        "world", "city", "shrine", "combat", "dungeon", NULL
    };
    static const char *borderBehaviorStrings[] = {
        "wrap", "exit", "fixed", NULL
    };
    Map* map;
    Map::Type mtype = (Map::Type) conf.getEnum("type", mapTypeStrings);

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
            errorFatal("Error: invalid map type used");
            return NULL;
    }
    if (! map)
        return NULL;

    map->id     = static_cast<MapId>(conf.getInt("id"));
    map->type   = mtype;
    map->fname  = conf.getString("fname");
    map->width  = conf.getInt("width");
    map->height = conf.getInt("height");
    map->levels = conf.getInt("levels");
    map->chunk_width  = conf.getInt("chunkwidth");
    map->chunk_height = conf.getInt("chunkheight");
    map->offset       = conf.getInt("offset");
    map->border_behavior = static_cast<Map::BorderBehavior>(conf.getEnum("borderbehavior", borderBehaviorStrings));

    if (isCombatMap(map)) {
        CombatMap *cm = dynamic_cast<CombatMap*>(map);
        cm->setContextual(conf.getBool("contextual"));
    }

    if (map->type == Map::WORLD || map->type == Map::CITY)
        map->flags |= SHOW_AVATAR;

    if (conf.getBool("nolineofsight"))
        map->flags |= NO_LINE_OF_SIGHT;

    if (conf.getBool("firstperson"))
        map->flags |= FIRST_PERSON;

    map->music = conf.getInt("music");

    map->tileset = tiles;

    vector<ConfigElement> children = conf.getChildren();
    vector<ConfigElement>::iterator it;
    foreach (it, children) {
        const string& cname = it->getName();
        if (cname == "city") {
            City *city = dynamic_cast<City*>(map);
            conf_initCity(*it, city);
        }
        else if (cname == "shrine") {
            Shrine *shrine = dynamic_cast<Shrine*>(map);
            conf_initShrine(*it, shrine);
        }
        else if (cname == "dungeon") {
            Dungeon *dungeon = dynamic_cast<Dungeon*>(map);
            conf_initDungeon(*it, dungeon);
        }
        else if (cname == "portal")
            map->portals.push_back(conf_initPortal(*it));
        else if (cname == "moongate")
            conf_createMoongate(*it);
        else if (cname == "compressedchunk")
            map->compressed_chunks.push_back( (*it).getInt("index") );
        else if (cname == "label")
            map->labels.insert( conf_initLabel(*it) );
    }

    return map;
}

//--------------------------------------
// Creature

struct Attribute {
    const char *name;
    unsigned int mask;
};

static void conf_creatureLoad(Creature* cr, Tileset* ts, const ConfigElement &conf) {
    static const Attribute booleanAttributes[] = {
        { "undead",         MATTR_UNDEAD },
        { "good",           MATTR_GOOD },
        { "swims",          MATTR_WATER },
        { "sails",          MATTR_WATER },
        { "cantattack",     MATTR_NONATTACKABLE },
        { "camouflage",     MATTR_CAMOUFLAGE },
        { "wontattack",     MATTR_NOATTACK },
        { "ambushes",       MATTR_AMBUSHES },
        { "incorporeal",    MATTR_INCORPOREAL },
        { "nochest",        MATTR_NOCHEST },
        { "divides",        MATTR_DIVIDES },
        { "forceOfNature",  MATTR_FORCE_OF_NATURE }
    };
    static const Attribute steals[] = {
        { "food", MATTR_STEALFOOD },
        { "gold", MATTR_STEALGOLD }
    };
    static const Attribute casts[] = {
        { "sleep",  MATTR_CASTS_SLEEP },
        { "negate", MATTR_NEGATE }
    };
    static const Attribute movement[] = {
        { "none",    MATTR_STATIONARY },
        { "wanders", MATTR_WANDERS }
    };
    /* boolean attributes that affect movement */
    static const Attribute movementBoolean[] = {
        { "swims", MATTR_SWIMS },
        { "sails", MATTR_SAILS },
        { "flies", MATTR_FLIES },
        { "teleports", MATTR_TELEPORT },
        { "canMoveOntoCreatures", MATTR_CANMOVECREATURES },
        { "canMoveOntoAvatar", MATTR_CANMOVEAVATAR }
    };
    static const struct {
        const char *name;
        TileEffect effect;
    } effects[] = {
        { "fire",   EFFECT_FIRE },
        { "poison", EFFECT_POISONFIELD },
        { "sleep",  EFFECT_SLEEP }
    };

    unsigned int idx;
    int attr = 0;
    int moveAttr = 0;


    cr->name = conf.getString("name");
    cr->id = conf.getInt("id");

    /* Get the leader if it's been included, otherwise the leader is itself */
    cr->leader = conf.getInt("leader", cr->id);

    cr->xp = conf.getInt("exp");
    cr->ranged = conf.getBool("ranged");
    cr->setTile(ts->getByName(conf.getString("tile")));

    cr->setHitTile("hit_flash");
    cr->setMissTile("miss_flash");

    cr->resists = 0;

    /* get the encounter size */
    cr->encounterSize = conf.getInt("encounterSize", 0);

    /* get the base hp */
    cr->basehp = conf.getInt("basehp", 0);
    /* adjust basehp according to battle difficulty setting */
    if (xu4.settings->battleDiff == BattleDiff_Hard)
        cr->basehp *= 2;
    else if (xu4.settings->battleDiff == BattleDiff_Expert)
        cr->basehp *= 4;

    /* get the camouflaged tile */
    if (conf.exists("camouflageTile"))
        cr->camouflageTile = conf.getString("camouflageTile");

    /* get the ranged tile for world map attacks */
    if (conf.exists("worldrangedtile"))
        cr->worldrangedtile = conf.getString("worldrangedtile");

    /* get ranged hit tile */
    if (conf.exists("rangedhittile")) {
        if (conf.getString("rangedhittile") == "random")
            attr |= MATTR_RANDOMRANGED;
        else
            cr->setHitTile(conf.getString("rangedhittile"));
    }

    /* get ranged miss tile */
    if (conf.exists("rangedmisstile")) {
        if (conf.getString("rangedmisstile") ==  "random")
            attr |= MATTR_RANDOMRANGED;
        else
            cr->setMissTile(conf.getString("rangedmisstile"));
    }

    /* find out if the creature leaves a tile behind on ranged attacks */
    cr->leavestile = conf.getBool("leavestile");

    /* get effects that this creature is immune to */
    for (idx = 0; idx < sizeof(effects) / sizeof(effects[0]); idx++) {
        if (conf.getString("resists") == effects[idx].name)
            cr->resists = effects[idx].effect;
    }

    /* Load creature attributes */
    for (idx = 0; idx < sizeof(booleanAttributes) / sizeof(booleanAttributes[0]); idx++) {
        if (conf.getBool(booleanAttributes[idx].name))
            attr |= booleanAttributes[idx].mask;
    }

    /* Load boolean attributes that affect movement */
    for (idx = 0; idx < sizeof(movementBoolean) / sizeof(movementBoolean[0]); idx++) {
        if (conf.getBool(movementBoolean[idx].name))
            moveAttr |= movementBoolean[idx].mask;
    }

    for (idx = 0; idx < sizeof(movement) / sizeof(movement[0]); idx++) {
        if (conf.getString("movement") == movement[idx].name)
            moveAttr |= movement[idx].mask;
    }

    for (idx = 0; idx < sizeof(steals) / sizeof(steals[0]); idx++) {
        if (conf.getString("steals") == steals[idx].name)
            attr |= steals[idx].mask;
    }

    for (idx = 0; idx < sizeof(casts) / sizeof(casts[0]); idx++) {
        if (conf.getString("casts") == casts[idx].name)
            attr |= casts[idx].mask;
    }

    if (conf.exists("spawnsOnDeath")) {
        attr |= MATTR_SPAWNSONDEATH;
        cr->spawn = static_cast<unsigned char>(conf.getInt("spawnsOnDeath"));
    }

    cr->mattr = static_cast<CreatureAttrib>(attr);
    cr->movementAttr = static_cast<CreatureMovementAttrib>(moveAttr);

    /* Figure out which 'slowed' function to use */
    if (cr->sails())
        /* sailing creatures (pirate ships) */
        cr->slowedType = SLOWED_BY_WIND;
    else if (cr->flies() || cr->isIncorporeal())
        /* flying creatures (dragons, bats, etc.) and incorporeal creatures (ghosts, zorns) */
        cr->slowedType = SLOWED_BY_NOTHING;
    else
        cr->slowedType = SLOWED_BY_TILE;
}

//--------------------------------------

static Armor*  conf_armor(int type, const ConfigElement&);
static Weapon* conf_weapon(int type, const ConfigElement&);

ConfigXML::ConfigXML() {
    backend = &xcd;

    xcd.tileset = NULL;
    memset(&xcd.usaveIds, 0, sizeof(xcd.usaveIds));

    xcd.sym.intern("unset!");   // Symbol 0 can be used as nil/unset/unknown.

    xmlRegisterInputCallbacks(&xmlFileMatch, &conf_fileOpen, xmlFileRead, xmlFileClose);

    xcd.doc = xmlParseFile(configXmlPath);
    if (!xcd.doc) {
        printf("Failed to read main configuration file '%s'", configXmlPath);
        errorFatal("error parsing main config.");
    }

    xmlXIncludeProcess(xcd.doc);

    if (xu4.settings->validateXml && xcd.doc->intSubset) {
        string errorMessage;
        xmlValidCtxt cvp;

        if (verbose)
            printf("validating config.xml\n");

        cvp.userData = &errorMessage;
        cvp.error = &conf_accumError;

        // Error changed to not fatal due to regression in libxml2
        if (!xmlValidateDocument(&cvp, xcd.doc))
            errorWarning("xml validation error:\n%s", errorMessage.c_str());
    }

    // Load primary elements.

    // egaPalette (load on demand)
    xcd.egaColors = NULL;

    // musicFile
    {
    xcd.musicFiles.reserve(MUSIC_MAX);
    xcd.musicFiles.push_back("");    // filename for MUSIC_NONE;
    vector<ConfigElement> ce = getElement("music").getChildren();
    vector<ConfigElement>::const_iterator it  = ce.begin();
    vector<ConfigElement>::const_iterator end = ce.end();
    for (; it != end; ++it) {
        if (it->getName() == "track") {
            xcd.musicFiles.push_back(it->getString("file"));
        }
    }
    }

    // soundFile
    {
    xcd.soundFiles.reserve(SOUND_MAX);
    vector<ConfigElement> ce = getElement("sound").getChildren();
    vector<ConfigElement>::const_iterator it  = ce.begin();
    vector<ConfigElement>::const_iterator end = ce.end();
    for (; it != end; ++it) {
        if (it->getName() == "track") {
            xcd.soundFiles.push_back(it->getString("file"));
        }
    }
    }

    // schemeNames
    {
    vector<ConfigElement> ce = getElement("graphics").getChildren();
    vector<ConfigElement>::const_iterator it  = ce.begin();
    vector<ConfigElement>::const_iterator end = ce.end();
    for (; it != end; ++it) {
        if (it->getName() == "imageset") {
            xcd.schemeNames.push_back(it->getString("name"));

            /*
            // register all the images declared in the config files
            ImageSet *set = loadImageSet(ce);
            imageSets[set->name] = set;
            */
        }
    }
    }

    {
    vector<ConfigElement> ce;
    vector<ConfigElement>::const_iterator it;

    // armors
    ce = getElement("armors").getChildren();
    foreach (it, ce) {
        if (it->getName() == "armor")
            xcd.armors.push_back(conf_armor(xcd.armors.size(), *it));
    }

    // weapons
    ce = getElement("weapons").getChildren();
    foreach (it, ce) {
        if (it->getName() == "weapon")
            xcd.weapons.push_back(conf_weapon(xcd.weapons.size(), *it));
    }

    // tileRules
    {
    TileRule* rule;
    Symbol symDefault = xcd.sym.intern("default");

    ce = getElement("tileRules").getChildren();
    xcd.tileRuleCount = ce.size();
    xcd.tileRuleDefault = -1;
    rule = xcd.tileRules = new TileRule[ xcd.tileRuleCount ];
    foreach (it, ce) {
        conf_tileRule(xcd.sym, rule, *it);
        if (rule->name == symDefault )
            xcd.tileRuleDefault = rule - xcd.tileRules;
        ++rule;
    }
    if (xcd.tileRuleDefault < 0)
        errorFatal("no 'default' rule found in tile rules");
    }

    // tileset (requires tileRules)
    ce = getElement("tilesets").getChildren();
    foreach (it, ce) {
        if (it->getName() == "tileset") {
            xcd.tileset = new Tileset;
            conf_tilesetLoad(this, xcd.tileset, *it);
            break;      // Only support one tileset.
        }
    }

    // usaveIds (requires tileset)
    if (xcd.tileset) {
        foreach (it, ce) {
            if (it->getName() == "tilemap") {
                conf_ultimaSaveIds(&xcd.usaveIds, xcd.tileset, *it);
                break;
            }
        }
    }

    // mapList
    ce = getElement("maps").getChildren();
    xcd.mapList.resize(ce.size(), NULL);
    foreach (it, ce) {
        /* Register map; the contents get loaded later, as needed. */
        Map* map = conf_makeMap(xcd.tileset, *it);
        if (xcd.mapList[map->id])
            errorFatal("A map with id '%d' already exists", map->id);
        xcd.mapList[map->id] = map;
    }

    // creatures
    ce = getElement("creatures").getChildren();
    xcd.creatures.resize(ce.size(), NULL);
    foreach (it, ce) {
        if (it->getName() != "creature")
            continue;
        Creature* cr = new Creature;
        conf_creatureLoad(cr, xcd.tileset, *it);
        xcd.creatures[cr->getId()] = cr;
    }
    }
}

ConfigXML::~ConfigXML() {
    xmlFreeDoc(xcd.doc);

    delete[] xcd.egaColors;

    vector<Map *>::iterator mit;
    foreach (mit, xcd.mapList)
        delete *mit;

    vector<Creature *>::iterator cit;
    foreach (cit, xcd.creatures)
        delete *cit;

    vector<Armor *>::iterator ait;
    foreach (ait, xcd.armors)
        delete *ait;

    vector<Weapon *>::iterator wit;
    foreach (wit, xcd.weapons)
        delete *wit;

    delete[] xcd.tileRules;
    delete xcd.tileset;
    xcd.usaveIds.free();
}

//--------------------------------------

ConfigElement::ConfigElement(xmlNodePtr xmlNode) :
    node(xmlNode), name(reinterpret_cast<const char *>(xmlNode->name)) {
}

ConfigElement::ConfigElement(const ConfigElement &e) :
    node(e.node), name(e.name) {
}

ConfigElement &ConfigElement::operator=(const ConfigElement &e) {
    if (&e != this) {
        node = e.node;
        name = e.name;
    }
    return *this;
}

/**
 * Returns true if the property exists in the current config element
 */
bool ConfigElement::exists(const std::string &name) const {
    xmlChar *prop = xmlGetProp(node, reinterpret_cast<const xmlChar *>(name.c_str()));
    bool exists = prop != NULL;
    xmlFree(prop);

    return exists;
}

string ConfigElement::getString(const string &name) const {
    xmlChar *prop = xmlGetProp(node, reinterpret_cast<const xmlChar *>(name.c_str()));
    if (!prop)
        return "";

    string result(reinterpret_cast<const char *>(prop));
    xmlFree(prop);

    return result;
}

int ConfigElement::getInt(const string &name, int defaultValue) const {
    long result;
    xmlChar *prop;

    prop = xmlGetProp(node, reinterpret_cast<const xmlChar *>(name.c_str()));
    if (!prop)
        return defaultValue;

    result = strtol(reinterpret_cast<const char *>(prop), NULL, 0);
    xmlFree(prop);

    return static_cast<int>(result);
}

bool ConfigElement::getBool(const string &name) const {
    int result;

    xmlChar *prop = xmlGetProp(node, reinterpret_cast<const xmlChar *>(name.c_str()));
    if (!prop)
        return false;

    if (xmlStrcmp(prop, reinterpret_cast<const xmlChar *>("true")) == 0)
        result = true;
    else
        result = false;

    xmlFree(prop);

    return result;
}

int ConfigElement::getEnum(const string &name, const char *enumValues[]) const {
    int result = -1, i;
    xmlChar *prop;

    prop = xmlGetProp(node, reinterpret_cast<const xmlChar *>(name.c_str()));
    if (!prop)
        return 0;

    for (i = 0; enumValues[i]; i++) {
        if (xmlStrcmp(prop, reinterpret_cast<const xmlChar *>(enumValues[i])) == 0)
        result = i;
    }

    if (result == -1)
        errorFatal("invalid enum value for %s: %s", name.c_str(), prop);

    xmlFree(prop);

    return result;
}

vector<ConfigElement> ConfigElement::getChildren() const {
    vector<ConfigElement> result;

    for (xmlNodePtr child = node->children; child; child = child->next) {
        if (child->type == XML_ELEMENT_NODE)
            result.push_back(ConfigElement(child));
    }

    return result;
}

//--------------------------------------

const char* Config::symbolName( Symbol s ) const {
    return CB->sym.name(s);
}

/*
 * Return pointer to 16 RGBA values.
 */
const RGBA* Config::egaPalette() {
    if (! CB->egaColors) {
        RGBA* col = CB->egaColors = new RGBA[16];
        RGBA* end = col + 16;

        vector<ConfigElement> ce = getElement("egaPalette").getChildren();
        vector<ConfigElement>::iterator it;
        foreach (it, ce) {
            if (it->getName() != "color")
                continue;
            col->r = it->getInt("red");
            col->g = it->getInt("green");
            col->b = it->getInt("blue");
            col->a = 255;
            if (++col == end)
                break;
        }
    }
    return CB->egaColors;
}

/*
 * Return a filename pointer for the given MusicTrack id (see sound.h)
 * The value is a C string as that is what low-level audio APIs require.
 * The pointer is valid until the next musicFile or soundFile call.
 */
const char* Config::musicFile( uint32_t id ) {
    if (id < CB->musicFiles.size()) {
        CB->sbuf = u4find_music(CB->musicFiles[id]);
        return CB->sbuf.c_str();
    }
    return NULL;
}

/*
 * Return a filename pointer for the given Sound id (see sound.h)
 * The value is a C string as that is what low-level audio APIs require.
 * The pointer is valid until the next musicFile or soundFile call.
 */
const char* Config::soundFile( uint32_t id ) {
    if (id < CB->soundFiles.size()) {
        CB->sbuf = u4find_sound(CB->soundFiles[id]);
        return CB->sbuf.c_str();
    }
    return NULL;
}

/*
 * Return a C string pointer array for the available Image scheme names.
 */
const char** Config::schemeNames() {
    vector<const char*>& sarray = CB->sarray;
    sarray.clear();
    vector<string>::iterator it;
    for (it = CB->schemeNames.begin(); it != CB->schemeNames.end(); ++it)
        sarray.push_back( (*it).c_str() );
    sarray.push_back(NULL);
    return &sarray.front();
}

/*
 * Return an Armor pointer for the given ArmorType id (see savegame.h)
 */
const Armor* Config::armor( uint32_t id ) {
    if (id < CB->armors.size())
        return CB->armors[id];
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
    vector<Armor*>::const_iterator it;
    foreach (it, CB->armors) {
        if ((*it)->name == name)
            return it - CB->armors.begin();
    }
    return -1;
}

int Config::weaponType( const char* name ) {
    vector<Weapon*>::const_iterator it;
    foreach (it, CB->weapons) {
        if ((*it)->name == name)
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
    if (! rmap->data.size()) {
        if (! loadMap(rmap, NULL))
            errorFatal("loadMap failed to read \"%s\" (type %d)",
                       rmap->fname.c_str(), rmap->type);
    }
    return rmap;
}

// Load map from saved game.
Map* Config::restoreMap(uint32_t id) {
    if (id >= CB->mapList.size())
        return NULL;

    Map* rmap = CB->mapList[id];
    if (! rmap->data.size()) {
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
                       rmap->fname.c_str(), rmap->type);
    }
    return rmap;
}

void Config::unloadMap(uint32_t id) {
    delete CB->mapList[id];

    vector<ConfigElement> maps = getElement("maps").getChildren();
    vector<ConfigElement>::const_iterator it;
    foreach (it, maps) {
        if (id == (uint32_t) (*it).getInt("id")) {
            Map* map = conf_makeMap(CB->tileset, *it);
            CB->mapList[id] = map;
            break;
        }
    }
}

//--------------------------------------
// Graphics config

static SubImage* loadSubImage(const ImageInfo *info, const ConfigElement &conf) {
    SubImage *subimage;
    static int x = 0,
               y = 0,
               last_width = 0,
               last_height = 0;

    subimage = new SubImage;
    subimage->name = conf.getString("name");
    subimage->width = conf.getInt("width");
    subimage->height = conf.getInt("height");
    subimage->srcImageName = info->name;
    if (conf.exists("x") && conf.exists("y")) {
        x = subimage->x = conf.getInt("x");
        y = subimage->y = conf.getInt("y");
    }
    else {
        // Automatically increment our position through the base image
        x += last_width;
        if (x >= last_width) {
            x = 0;
            y += last_height;
        }

        subimage->x = x;
        subimage->y = y;
    }

    // "remember" the width and height of this subimage
    last_width = subimage->width;
    last_height = subimage->height;

    return subimage;
}

static uint8_t mapFiletype(const string& str, const string& file) {
    if (! str.empty()) {
        if (str == "image/x-u4raw")
            return FTYPE_U4RAW;
        if (str == "image/x-u4rle")
            return FTYPE_U4RLE;
        if (str == "image/x-u4lzw")
            return FTYPE_U4LZW;
        if (str == "image/x-u5lzw")
            return FTYPE_U5LZW;
        if (str == "image/png")
            return FTYPE_PNG;
        if (str == "image/fmtowns")
            return FTYPE_FMTOWNS;
        if (str == "image/fmtowns-pic")
            return FTYPE_FMTOWNS_PIC;
        if (str == "image/fmtowns-tif")
            return FTYPE_FMTOWNS_TIF;
    }

    // Guess at type based on filename.
    size_t length = file.length();
    if (length >= 4 && file.compare(length - 4, 4, ".png") == 0)
        return FTYPE_PNG;

    errorWarning("Unknown image filetype %s", str.c_str());
    return FTYPE_UNKNOWN;
}

static ImageInfo* loadImageInfo(const ConfigElement &conf) {
    ImageInfo *info;
    static const char *fixupEnumStrings[] = { "none", "intro", "abyss", "abacus", "dungns", "blackTransparencyHack", "fmtownsscreen", NULL };

    info = new ImageInfo;
    info->name = conf.getString("name");
    info->filename = conf.getString("filename");
    info->resGroup = 0;
    info->width = conf.getInt("width", -1);
    info->height = conf.getInt("height", -1);
    info->depth = conf.getInt("depth", -1);
    info->prescale = conf.getInt("prescale");
    info->filetype = mapFiletype(conf.getString("filetype"), info->filename);
    info->tiles = conf.getInt("tiles");
    info->transparentIndex = conf.getInt("transparentIndex", -1);
    info->fixup = static_cast<ImageFixup>(conf.getEnum("fixup", fixupEnumStrings));
    info->image = NULL;

    vector<ConfigElement> children = conf.getChildren();
    for (std::vector<ConfigElement>::iterator i = children.begin(); i != children.end(); i++) {
        if (i->getName() == "subimage") {
            SubImage *subimage = loadSubImage(info, *i);
            info->subImages[subimage->name] = subimage;
        }
    }

    return info;
}

static ImageSet* loadImageSet(const ConfigElement &conf) {
    ImageSet *set = new ImageSet;

    set->name    = conf.getString("name");
    set->extends = conf.getString("extends");

    std::vector<ConfigElement>::iterator it;
    std::map<string, ImageInfo *>::iterator dup;
    vector<ConfigElement> children = conf.getChildren();

    foreach (it, children) {
        if (it->getName() == "image") {
            ImageInfo *info = loadImageInfo(*it);
            dup = set->info.find(info->name);
            if (dup != set->info.end()) {
                delete dup->second;
                set->info.erase(dup);
            }
            set->info[info->name] = info;
        }
    }

    return set;
}

/*
 * Return ImageSet pointer which caller must delete.
 */
ImageSet* Config::newScheme( uint32_t id ) {
    uint32_t n = 0;
    vector<ConfigElement> ce = getElement("graphics").getChildren();
    vector<ConfigElement>::const_iterator it  = ce.begin();
    vector<ConfigElement>::const_iterator end = ce.end();
    for (; it != end; ++it) {
        if (it->getName() == "imageset") {
            if( n == id )
                return loadImageSet(*it);
            ++n;
        }
    }
    return NULL;
}

//--------------------------------------
// Items (weapons & armor)

static Armor* conf_armor(int type, const ConfigElement& conf) {
    Armor* arm = new Armor;

    arm->type    = (ArmorType) type;
    arm->name    = conf.getString("name");
    arm->canuse  = 0xFF;
    arm->defense = conf.getInt("defense");
    arm->mask    = 0;

    vector<ConfigElement> contraintConfs = conf.getChildren();
    std::vector<ConfigElement>::iterator it;
    foreach (it, contraintConfs) {
        if (it->getName() != "constraint")
            continue;

        unsigned char mask = 0;
        for (int cl = 0; cl < 8; cl++) {
            if (strcasecmp(it->getString("class").c_str(), getClassName(static_cast<ClassType>(cl))) == 0)
                mask = (1 << cl);
        }
        if (mask == 0 && strcasecmp(it->getString("class").c_str(), "all") == 0)
            mask = 0xFF;
        if (mask == 0) {
            errorFatal("malformed armor.xml file: constraint has unknown class %s",
                       it->getString("class").c_str());
        }
        if (it->getBool("canuse"))
            arm->canuse |= mask;
        else
            arm->canuse &= ~mask;
    }
    return arm;
}

static Weapon* conf_weapon(int type, const ConfigElement& conf) {
    static const struct {
        const char *name;
        unsigned int flag;
    } booleanAttributes[] = {
        { "lose", WEAP_LOSE },
        { "losewhenranged", WEAP_LOSEWHENRANGED },
        { "choosedistance", WEAP_CHOOSEDISTANCE },
        { "alwayshits", WEAP_ALWAYSHITS },
        { "magic", WEAP_MAGIC },
        { "attackthroughobjects", WEAP_ATTACKTHROUGHOBJECTS },
        { "returns", WEAP_RETURNS },
        { "dontshowtravel", WEAP_DONTSHOWTRAVEL }
    };
    Weapon* wpn = new Weapon;

    wpn->type   = (WeaponType) type;
    wpn->name   = conf.getString("name");
    wpn->abbr   = conf.getString("abbr");
    wpn->canuse = 0xFF;
    wpn->range  = 0;
    wpn->damage = conf.getInt("damage");
    wpn->hittile   = "hit_flash";
    wpn->misstile  = "miss_flash";
    //wpn->leavetile = "";
    wpn->flags = 0;


    /* Get the range of the weapon, whether it is absolute or normal range */
    string _range = conf.getString("range");
    if (_range.empty()) {
        _range = conf.getString("absolute_range");
        if (!_range.empty())
            wpn->flags |= WEAP_ABSOLUTERANGE;
    }
    if (_range.empty())
        errorFatal("malformed weapons.xml file: range or absolute_range not found for weapon %s", wpn->name.c_str());

    wpn->range = atoi(_range.c_str());

    /* Load weapon attributes */
    for (unsigned at = 0; at < sizeof(booleanAttributes) / sizeof(booleanAttributes[0]); at++) {
        if (conf.getBool(booleanAttributes[at].name))
            wpn->flags |= booleanAttributes[at].flag;
    }

    /* Load hit tiles */
    if (conf.exists("hittile"))
        wpn->hittile = conf.getString("hittile");

    /* Load miss tiles */
    if (conf.exists("misstile"))
        wpn->misstile = conf.getString("misstile");

    /* Load leave tiles */
    if (conf.exists("leavetile"))
        wpn->leavetile = conf.getString("leavetile");

    vector<ConfigElement> contraintConfs = conf.getChildren();
    for (std::vector<ConfigElement>::iterator i = contraintConfs.begin(); i != contraintConfs.end(); i++) {
        unsigned char mask = 0;

        if (i->getName() != "constraint")
            continue;

        for (int cl = 0; cl < 8; cl++) {
            if (strcasecmp(i->getString("class").c_str(), getClassName(static_cast<ClassType>(cl))) == 0)
                mask = (1 << cl);
        }
        if (mask == 0 && strcasecmp(i->getString("class").c_str(), "all") == 0)
            mask = 0xFF;
        if (mask == 0) {
            errorFatal("malformed weapons.xml file: constraint has unknown class %s",
                       i->getString("class").c_str());
        }
        if (i->getBool("canuse"))
            wpn->canuse |= mask;
        else
            wpn->canuse &= ~mask;
    }
    return wpn;
}

//--------------------------------------
// Config Service API

// Create configService.
Config* configInit() {
    // Here's where we can compile the program with alternate back-ends
    // (e.g. SQL, JSON, ... or something better.)

    return new ConfigXML;
}

void configFree(Config* conf) {
    delete conf;
}
