/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libxml/xmlmemory.h>

#include "u4.h"

#include "annotation.h"
#include "area.h"
#include "city.h"
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
#include "xml.h"

extern int isAbyssOpened(const Portal *p);
extern int shrineCanEnter(const Portal *p);

Map *mapMgrInitMapFromXml(xmlNodePtr node);
City *mapMgrInitCityFromXml(xmlNodePtr node);
PersonRole *mapMgrInitPersonRoleFromXml(xmlNodePtr node);
Portal *mapMgrInitPortalFromXml(xmlNodePtr node);
Shrine *mapMgrInitShrineFromXml(xmlNodePtr node);
Dungeon *mapMgrInitDungeonFromXml(xmlNodePtr node);
void mapMgrCreateMoongateFromXml(xmlNodePtr node);
int mapMgrInitCompressedChunkFromXml(xmlNodePtr node);

MapList mapList;

void mapMgrInit() {
    xmlDocPtr doc;
    xmlNodePtr root, node;
    Map *map;

    doc = xmlParse("maps.xml");
    root = xmlDocGetRootElement(doc);
    if (xmlStrcmp(root->name, (const xmlChar *) "maps") != 0)
        errorFatal("malformed maps.xml");

    for (node = root->xmlChildrenNode; node; node = node->next) {
        if (xmlNodeIsText(node) ||
            xmlStrcmp(node->name, (const xmlChar *) "map") != 0)
            continue;

        map = mapMgrInitMapFromXml(node);
        mapLoad(map);
        mapMgrRegister(map);

        delete map;
    }

    xmlFreeDoc(doc);
}

Map *mapMgrInitMap(void) {
    Map *map;

    map = new Map;
    if (!map)
        return NULL;
    map->annotations = new AnnotationMgr();
    map->init = NULL;
    map->objects = NULL;
    map->flags = 0;
    map->width = 0;
    map->height = 0;
    map->levels = 1;
    map->chunk_width = 0;
    map->chunk_height = 0;    
    map->id = 0;

    return map;
}

Map *mapMgrInitMapFromXml(xmlNodePtr node) {
    Map *map;
    xmlNodePtr child;    
    static const char *mapTypeEnumStrings[] = { "world", "town", "village", "castle", "ruins", "shrine", "combat", "dungeon", NULL };
    static const char *borderBehaviorEnumStrings[] = { "wrap", "exit", "fixed", NULL };

    map = mapMgrInitMap();
    if (!map)
        return NULL;

    map->id = (MapId)xmlGetPropAsInt(node, "id");

    map->type = (MapType)xmlGetPropAsEnum(node, "type", mapTypeEnumStrings);
    map->fname = xmlGetPropAsStr(node, "fname");
    map->width = xmlGetPropAsInt(node, "width");
    map->height = xmlGetPropAsInt(node, "height");
    map->levels = xmlGetPropAsInt(node, "levels");
    map->chunk_width = xmlGetPropAsInt(node, "chunkwidth");
    map->chunk_height = xmlGetPropAsInt(node, "chunkheight");
    map->border_behavior = (MapBorderBehavior)xmlGetPropAsEnum(node, "borderbehavior", borderBehaviorEnumStrings);

    if (xmlGetPropAsBool(node, "showavatar"))
        map->flags |= SHOW_AVATAR;

    if (xmlGetPropAsBool(node, "nolineofsight"))
        map->flags |= NO_LINE_OF_SIGHT;
    
    if (xmlGetPropAsBool(node, "firstperson"))
        map->flags |= FIRST_PERSON;

    map->music = (Music)xmlGetPropAsInt(node, "music");

    for (child = node->xmlChildrenNode; child; child = child->next) {
        if (xmlNodeIsText(child))
            continue;

        if (xmlStrcmp(child->name, (const xmlChar *) "city") == 0) {
            map->city = mapMgrInitCityFromXml(child);
            map->city->map = map;
        }
        else if (xmlStrcmp(child->name, (const xmlChar *) "shrine") == 0)
            map->shrine = mapMgrInitShrineFromXml(child);
        else if (xmlStrcmp(child->name, (const xmlChar *) "dungeon") == 0)
            map->dungeon = mapMgrInitDungeonFromXml(child);
        else if (xmlStrcmp(child->name, (const xmlChar *) "portal") == 0)
            map->portals.push_back(mapMgrInitPortalFromXml(child));
        else if (xmlStrcmp(child->name, (const xmlChar *) "moongate") == 0)
            mapMgrCreateMoongateFromXml(child);
        else if (xmlStrcmp(child->name, (const xmlChar *) "compressedchunk") == 0)
            map->compressed_chunks.push_back(mapMgrInitCompressedChunkFromXml(child));
    }
    
    return map;
}

City *mapMgrInitCityFromXml(xmlNodePtr node) {
    City *city;
    xmlNodePtr child;    

    city = new City;
    if (!city)
        return NULL;    
    city->map = NULL;

    city->name = xmlGetPropAsStr(node, "name");
    city->tlk_fname = xmlGetPropAsStr(node, "tlk_fname");

    for (child = node->xmlChildrenNode; child; child = child->next) {
        if (xmlNodeIsText(child))
            continue;

        if (xmlStrcmp(child->name, (const xmlChar *) "personrole") == 0)
            city->personroles.push_back(mapMgrInitPersonRoleFromXml(child));
    }    

    return city;
}

PersonRole *mapMgrInitPersonRoleFromXml(xmlNodePtr node) {
    PersonRole *personrole;
    static const char *roleEnumStrings[] = { "companion", "weaponsvendor", "armorvendor", "foodvendor", "tavernkeeper",
                                             "reagentsvendor", "healer", "innkeeper", "guildvendor", "horsevendor",
                                             "lordbritish", "hawkwind", NULL };


    personrole = new PersonRole;
    if (!personrole)
        return NULL;

    personrole->role = xmlGetPropAsEnum(node, "role", roleEnumStrings) + NPC_TALKER_COMPANION;
    personrole->id = xmlGetPropAsInt(node, "id");

    return personrole;
}

Portal *mapMgrInitPortalFromXml(xmlNodePtr node) {
    xmlNodePtr child;
    Portal *portal;
    char *prop;

    portal = new Portal;
    if (!portal)
        return NULL;
    portal->portalConditionsMet = NULL;
    portal->message = NULL;
    portal->retroActiveDest = NULL;
 
    portal->coords = MapCoords(
        (unsigned short) xmlGetPropAsInt(node, "x"),
        (unsigned short) xmlGetPropAsInt(node, "y"),
        (unsigned short) xmlGetPropAsInt(node, "z"));
    portal->destid = (MapId) xmlGetPropAsInt(node, "destmapid");
        
    portal->start = portal->coords;
    
    if (xmlPropExists(node, "startx"))
        portal->start.x = (unsigned short) xmlGetPropAsInt(node, "startx");    

    if (xmlPropExists(node, "starty"))
        portal->start.y = (unsigned short) xmlGetPropAsInt(node, "starty");    

    portal->start.z = (unsigned short) xmlGetPropAsInt(node, "startlevel");

    prop = xmlGetPropAsStr(node, "action");
    if (strcmp(prop, "none") == 0)
        portal->trigger_action = ACTION_NONE;
    else if (strcmp(prop, "enter") == 0)
        portal->trigger_action = ACTION_ENTER;
    else if (strcmp(prop, "klimb") == 0)
        portal->trigger_action = ACTION_KLIMB;
    else if (strcmp(prop, "descend") == 0)
        portal->trigger_action = ACTION_DESCEND;
    else if (strcmp(prop, "exit_north") == 0)
        portal->trigger_action = ACTION_EXIT_NORTH;
    else if (strcmp(prop, "exit_east") == 0)
        portal->trigger_action = ACTION_EXIT_EAST;
    else if (strcmp(prop, "exit_south") == 0)
        portal->trigger_action = ACTION_EXIT_SOUTH;
    else if (strcmp(prop, "exit_west") == 0)
        portal->trigger_action = ACTION_EXIT_WEST;
    else
        errorFatal("unknown trigger_action: %s", prop);
    xmlFree(prop);
    
    prop = xmlGetPropAsStr(node, "condition");
    if (prop) {
        if (strcmp(prop, "shrine") == 0)
            portal->portalConditionsMet = &shrineCanEnter;
        else if (strcmp(prop, "abyss") == 0)
            portal->portalConditionsMet = &isAbyssOpened;
        else
            errorFatal("unknown portalConditionsMet: %s", prop);
        xmlFree(prop);
    }

    if (xmlPropExists(node, "savelocation"))    
        portal->saveLocation = xmlGetPropAsBool(node, "savelocation");
    else portal->saveLocation = 0;

    prop = xmlGetPropAsStr(node, "message");
    if (prop) {
        portal->message = strdup(prop);
        xmlFree(prop);
    }

    prop = xmlGetPropAsStr(node, "transport");
    if (strcmp(prop, "foot") == 0)
        portal->portalTransportRequisites = TRANSPORT_FOOT;
    else if (strcmp(prop, "footorhorse") == 0)
        portal->portalTransportRequisites = TRANSPORT_FOOT_OR_HORSE;
    else
        errorFatal("unknown transport: %s", prop);
    xmlFree(prop);

    if (xmlGetPropAsBool(node, "exits"))
        portal->exitPortal = 1;
    else portal->exitPortal = 0;

    for (child = node->xmlChildrenNode; child; child = child->next) {
        if (xmlNodeIsText(child))
            continue;

        if (xmlStrcmp(child->name, (const xmlChar *) "retroActiveDest") == 0) {
            portal->retroActiveDest = new PortalDestination;
            
            portal->retroActiveDest->coords = MapCoords(
                xmlGetPropAsInt(child, "x"),
                xmlGetPropAsInt(child, "y"),
                xmlGetPropAsInt(child, "z"));
            portal->retroActiveDest->mapid = (MapId) xmlGetPropAsInt(child, "mapid");
        }
    }    
    return portal;
}

Shrine *mapMgrInitShrineFromXml(xmlNodePtr node) {
    Shrine *shrine;
    char *prop;

    static const char *virtues[] = {"Honesty", "Compassion", "Valor", "Justice", "Sacrifice", "Honor", "Spirituality", "Humility", NULL};

    shrine = new Shrine;
    if (!shrine)
        return NULL;

    shrine->virtue = (Virtue)xmlGetPropAsEnum(node, "virtue", virtues);

    prop = xmlGetPropAsStr(node, "mantra");
    shrine->mantra = strdup(prop);
    xmlFree(prop);

    return shrine;
}

Dungeon *mapMgrInitDungeonFromXml(xmlNodePtr node) {
    Dungeon *dungeon;    

    dungeon = new Dungeon;
    if (!dungeon)
        return NULL;
    dungeon->n_rooms = xmlGetPropAsInt(node, "rooms");
    dungeon->rooms = NULL;

    dungeon->name = xmlGetPropAsStr(node, "name");        

    return dungeon;
}

void mapMgrCreateMoongateFromXml(xmlNodePtr node) {
    int phase;
    MapCoords coords;

    phase = xmlGetPropAsInt(node, "phase");
    coords = MapCoords(xmlGetPropAsInt(node, "x"), xmlGetPropAsInt(node, "y"));

    moongateAdd(phase, coords);
}

int mapMgrInitCompressedChunkFromXml(xmlNodePtr node) {
    return xmlGetPropAsInt(node, "index");
}

void mapMgrRegister(Map *map) {
    if (!mapList.insert(MapList::value_type(map->id, *map)).second)
        errorFatal("Error: A map with id '%d' already exists", map->id);
}

Map *mapMgrGetById(MapId id) {    
    MapList::iterator current;

    current = mapList.find(id);
    if (current != mapList.end())
        return &current->second;
    else return NULL;
}
