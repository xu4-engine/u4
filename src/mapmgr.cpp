/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include <vector>

#include "u4.h"

#include "annotation.h"
#include "city.h"
#include "combat.h"
#include "debug.h"
#include "dungeon.h"
#include "error.h"
#include "map.h"
#include "maploader.h"
#include "mapmgr.h"
#include "moongate.h"
#include "person.h"
#include "portal.h"
#include "shrine.h"
#include "types.h"
#include "u4file.h"
#include "config.h"

using std::vector;

MapMgr *MapMgr::instance = NULL;

extern bool isAbyssOpened(const Portal *p);
extern bool shrineCanEnter(const Portal *p);

MapMgr *MapMgr::getInstance() {
    if (instance == NULL)
        instance = new MapMgr();
    return instance;
}

MapMgr::MapMgr() {
    logger = new Debug("debug/mapmgr.txt", "MapMgr"); 
    TRACE(*logger, "creating MapMgr");

    const Config *config = Config::getInstance();
    Map *map;

    vector<ConfigElement> maps = config->getElement("/config/maps").getChildren();
    for (std::vector<ConfigElement>::iterator i = maps.begin(); i != maps.end(); i++) {
        map = initMapFromConf(*i);

        /* map actually gets loaded later, when it's needed */        
        registerMap(map);
    }
}

Map *MapMgr::initMap(Map::Type type) {
    Map *map;

    switch(type) {    
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
        break;
    }
    
    return map;
}

Map *MapMgr::get(MapId id) {    
    /* if the map hasn't been loaded yet, load it! */
    if (!mapList[id]->data.size()) {
        MapLoader *loader = MapLoader::getLoader(mapList[id]->type);
        if (loader == NULL)
            errorFatal("can't load map of type \"%d\"", mapList[id]->type);

        TRACE_LOCAL(*logger, string("loading map data for map \'") + mapList[id]->fname + "\'");

        loader->load(mapList[id]);
    }
    return mapList[id];
}

void MapMgr::registerMap(Map *map) {
    if (mapList.size() <= map->id)
        mapList.resize(map->id + 1, NULL);

    if (mapList[map->id] != NULL)
        errorFatal("Error: A map with id '%d' already exists", map->id);

    mapList[map->id] = map;
}

Map *MapMgr::initMapFromConf(const ConfigElement &mapConf) {
    Map *map;
    static const char *mapTypeEnumStrings[] = { "world", "city", "shrine", "combat", "dungeon", NULL };
    static const char *borderBehaviorEnumStrings[] = { "wrap", "exit", "fixed", NULL };

    map = initMap(static_cast<Map::Type>(mapConf.getEnum("type", mapTypeEnumStrings)));
    if (!map)
        return NULL;

    map->id = static_cast<MapId>(mapConf.getInt("id"));
    map->type = static_cast<Map::Type>(mapConf.getEnum("type", mapTypeEnumStrings));
    map->fname = mapConf.getString("fname");
    map->width = mapConf.getInt("width");
    map->height = mapConf.getInt("height");
    map->levels = mapConf.getInt("levels");
    map->chunk_width = mapConf.getInt("chunkwidth");
    map->chunk_height = mapConf.getInt("chunkheight");
    map->border_behavior = static_cast<Map::BorderBehavior>(mapConf.getEnum("borderbehavior", borderBehaviorEnumStrings));

    TRACE_LOCAL(*logger, string("loading configuration for map \'") + map->fname + "\'");

    if (mapConf.getBool("showavatar"))
        map->flags |= SHOW_AVATAR;

    if (mapConf.getBool("nolineofsight"))
        map->flags |= NO_LINE_OF_SIGHT;
    
    if (mapConf.getBool("firstperson"))
        map->flags |= FIRST_PERSON;

    map->music = static_cast<Music>(mapConf.getInt("music"));

    vector<ConfigElement> children = mapConf.getChildren();
    for (std::vector<ConfigElement>::iterator i = children.begin(); i != children.end(); i++) {
        if (i->getName() == "city") {
            City *city = dynamic_cast<City*>(map);
            initCityFromConf(*i, city);            
        }
        else if (i->getName() == "shrine") {
            Shrine *shrine = dynamic_cast<Shrine*>(map);
            initShrineFromConf(*i, shrine);
        }            
        else if (i->getName() == "dungeon") {
            Dungeon *dungeon = dynamic_cast<Dungeon*>(map);
            initDungeonFromConf(*i, dungeon);
        }
        else if (i->getName() == "portal")
            map->portals.push_back(initPortalFromConf(*i));
        else if (i->getName() == "moongate")
            createMoongateFromConf(*i);
        else if (i->getName() == "compressedchunk")
            map->compressed_chunks.push_back(initCompressedChunkFromConf(*i));
    }
    
    return map;
}

void MapMgr::initCityFromConf(const ConfigElement &cityConf, City *city) {
    city->name = cityConf.getString("name");
    city->type = cityConf.getString("type");
    city->tlk_fname = cityConf.getString("tlk_fname");

    vector<ConfigElement> children = cityConf.getChildren();
    for (std::vector<ConfigElement>::iterator i = children.begin(); i != children.end(); i++) {
        if (i->getName() == "personrole")
            city->personroles.push_back(initPersonRoleFromConf(*i));
    }    
}

PersonRole *MapMgr::initPersonRoleFromConf(const ConfigElement &personRoleConf) {
    PersonRole *personrole;
    static const char *roleEnumStrings[] = { "companion", "weaponsvendor", "armorvendor", "foodvendor", "tavernkeeper",
                                             "reagentsvendor", "healer", "innkeeper", "guildvendor", "horsevendor",
                                             "lordbritish", "hawkwind", NULL };

    personrole = new PersonRole;

    personrole->role = personRoleConf.getEnum("role", roleEnumStrings) + NPC_TALKER_COMPANION;
    personrole->id = personRoleConf.getInt("id");

    return personrole;
}

Portal *MapMgr::initPortalFromConf(const ConfigElement &portalConf) {
    Portal *portal;

    portal = new Portal;

    portal->portalConditionsMet = NULL;
    portal->retroActiveDest = NULL;
 
    portal->coords = MapCoords(
        portalConf.getInt("x"),
        portalConf.getInt("y"),
        portalConf.getInt("z"));
    portal->destid = static_cast<MapId>(portalConf.getInt("destmapid"));
    
    portal->start.x = static_cast<unsigned short>(portalConf.getInt("startx"));
    portal->start.y = static_cast<unsigned short>(portalConf.getInt("starty"));
    portal->start.z = static_cast<unsigned short>(portalConf.getInt("startlevel"));

    string prop = portalConf.getString("action");
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
    
    prop = portalConf.getString("condition");
    if (!prop.empty()) {
        if (prop == "shrine")
            portal->portalConditionsMet = &shrineCanEnter;
        else if (prop == "abyss")
            portal->portalConditionsMet = &isAbyssOpened;
        else
            errorFatal("unknown portalConditionsMet: %s", prop.c_str());
    }

    portal->saveLocation = portalConf.getBool("savelocation");

    portal->message = portalConf.getString("message");

    prop = portalConf.getString("transport");
    if (prop == "foot")
        portal->portalTransportRequisites = TRANSPORT_FOOT;
    else if (prop == "footorhorse")
        portal->portalTransportRequisites = TRANSPORT_FOOT_OR_HORSE;
    else
        errorFatal("unknown transport: %s", prop.c_str());

    portal->exitPortal = portalConf.getBool("exits");

    vector<ConfigElement> children = portalConf.getChildren();
    for (std::vector<ConfigElement>::iterator i = children.begin(); i != children.end(); i++) {
        if (i->getName() == "retroActiveDest") {
            portal->retroActiveDest = new PortalDestination;
            
            portal->retroActiveDest->coords = MapCoords(
                i->getInt("x"),
                i->getInt("y"),
                i->getInt("z"));
            portal->retroActiveDest->mapid = static_cast<MapId>(i->getInt("mapid"));
        }
    }
    return portal;
}

void MapMgr::initShrineFromConf(const ConfigElement &shrineConf, Shrine *shrine) {    
    static const char *virtues[] = {"Honesty", "Compassion", "Valor", "Justice", "Sacrifice", "Honor", "Spirituality", "Humility", NULL};

    shrine->setVirtue(static_cast<Virtue>(shrineConf.getEnum("virtue", virtues)));
    shrine->setMantra(shrineConf.getString("mantra"));
}

void MapMgr::initDungeonFromConf(const ConfigElement &dungeonConf, Dungeon *dungeon) {
    dungeon->n_rooms = dungeonConf.getInt("rooms");
    dungeon->rooms = NULL;
    dungeon->roomMaps = NULL;
    dungeon->name = dungeonConf.getString("name");
}

void MapMgr::createMoongateFromConf(const ConfigElement &moongateConf) {
    int phase = moongateConf.getInt("phase");
    Coords coords(moongateConf.getInt("x"), moongateConf.getInt("y"));

    moongateAdd(phase, coords);
}

int MapMgr::initCompressedChunkFromConf(const ConfigElement &compressedChunkConf) {
    return compressedChunkConf.getInt("index");
}
