/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libxml/xmlmemory.h>

#include "u4.h"

#include "area.h"
#include "city.h"
#include "debug.h"
#include "dungeon.h"
#include "error.h"
#include "list.h"
#include "map.h"
#include "maploader.h"
#include "mapmgr.h"
#include "moongate.h"
#include "person.h"
#include "portal.h"
#include "shrine.h"
#include "u4file.h"
#include "xml.h"

extern int isAbyssOpened(const Portal *p);
extern int shrineCanEnter(const Portal *p);

Map *mapMgrInitMapFromXml(xmlNodePtr node);
City *mapMgrInitCityFromXml(xmlNodePtr node);
Portal *mapMgrInitPortalFromXml(xmlNodePtr node);
Shrine *mapMgrInitShrineFromXml(xmlNodePtr node);
Dungeon *mapMgrInitDungeonFromXml(xmlNodePtr node);
void mapMgrCreateMoongateFromXml(xmlNodePtr node);
int mapMgrInitCompressedChunkFromXml(xmlNodePtr node);

ListNode *mapList = NULL;

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
    }

    xmlFreeDoc(doc);
}

Map *mapMgrInitMap(void) {
    Map *map;

    map = malloc(sizeof(Map));
    if (!map)
        return NULL;
    map->n_portals = 0;
    map->portals = NULL;
    map->data = NULL;
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
    ListNode *portals = NULL, *compressed_chunks = NULL;
    static const char *mapTypeEnumStrings[] = { "world", "town", "village", "castle", "ruins", "shrine", "combat", "dungeon", NULL };
    static const char *borderBehaviorEnumStrings[] = { "wrap", "exit", "fixed", NULL };

    map = mapMgrInitMap();
    if (!map)
        return NULL;

    map->id = (unsigned char)xmlGetPropAsInt(node, "id");

    map->type = xmlGetPropAsEnum(node, "type", mapTypeEnumStrings);
    map->fname = xmlGetPropAsStr(node, "fname");
    map->width = xmlGetPropAsInt(node, "width");
    map->height = xmlGetPropAsInt(node, "height");
    map->levels = xmlGetPropAsInt(node, "levels");
    map->chunk_width = xmlGetPropAsInt(node, "chunkwidth");
    map->chunk_height = xmlGetPropAsInt(node, "chunkheight");
    map->border_behavior = xmlGetPropAsEnum(node, "borderbehavior", borderBehaviorEnumStrings);

    if (xmlGetPropAsBool(node, "showavatar"))
        map->flags |= SHOW_AVATAR;

    if (xmlGetPropAsBool(node, "nolineofsight"))
        map->flags |= NO_LINE_OF_SIGHT;
    
    if (xmlGetPropAsBool(node, "firstperson"))
        map->flags |= FIRST_PERSON;

    map->music = xmlGetPropAsInt(node, "music");

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
            portals = listAppend(portals, mapMgrInitPortalFromXml(child));
        else if (xmlStrcmp(child->name, (const xmlChar *) "moongate") == 0)
            mapMgrCreateMoongateFromXml(child);
        else if (xmlStrcmp(child->name, (const xmlChar *) "compressedchunk") == 0)
            compressed_chunks = listAppend(compressed_chunks, (void *) mapMgrInitCompressedChunkFromXml(child));
    }
    if (listLength(portals) > 0) {
        ListNode *node;
        int i;

        map->portals = malloc(listLength(portals) * sizeof(Portal));
        for (node = portals, i = 0; node; node = node->next, i++) {
            map->portals[i] = *((Portal *) node->data);
            free(node->data);
        }
        map->n_portals = listLength(portals);
        listDelete(portals);
    }
    if (listLength(compressed_chunks) > 0) {
        ListNode *node;
        int i;

        map->compressed_chunks = malloc(listLength(compressed_chunks) * sizeof(int));
        for (node = compressed_chunks, i = 0; node; node = node->next, i++) {
            map->compressed_chunks[i] = (int) node->data;
        }
        map->n_compressed_chunks = listLength(compressed_chunks);
        listDelete(compressed_chunks);
    }

    return map;
}

City *mapMgrInitCityFromXml(xmlNodePtr node) {
    City *city;
    char *prop;

    city = malloc(sizeof(City));
    if (!city)
        return NULL;
    city->n_persons = 0;
    city->persons = NULL;
    memset(city->person_types, 0, sizeof(city->person_types));
    city->map = NULL;

    prop = xmlGetPropAsStr(node, "name");
    city->name = strdup(prop);
    xmlFree(prop);

    prop = xmlGetPropAsStr(node, "tlk_fname");
    city->tlk_fname = strdup(prop);
    xmlFree(prop);

    city->person_types[NPC_TALKER_COMPANION - NPC_TALKER_COMPANION] = xmlGetPropAsInt(node, "companion");
    city->person_types[NPC_VENDOR_WEAPONS - NPC_TALKER_COMPANION] = xmlGetPropAsInt(node, "weaponsvendor");
    city->person_types[NPC_VENDOR_ARMOR - NPC_TALKER_COMPANION] = xmlGetPropAsInt(node, "armorvendor");
    city->person_types[NPC_VENDOR_FOOD - NPC_TALKER_COMPANION] = xmlGetPropAsInt(node, "foodvendor");
    city->person_types[NPC_VENDOR_TAVERN - NPC_TALKER_COMPANION] = xmlGetPropAsInt(node, "tavernkeeper");
    city->person_types[NPC_VENDOR_REAGENTS - NPC_TALKER_COMPANION] = xmlGetPropAsInt(node, "reagentsvendor");
    city->person_types[NPC_VENDOR_HEALER - NPC_TALKER_COMPANION] = xmlGetPropAsInt(node, "healer");
    city->person_types[NPC_VENDOR_INN - NPC_TALKER_COMPANION] = xmlGetPropAsInt(node, "innkeeper");
    city->person_types[NPC_VENDOR_GUILD - NPC_TALKER_COMPANION] = xmlGetPropAsInt(node, "guildvendor");
    city->person_types[NPC_VENDOR_STABLE - NPC_TALKER_COMPANION] = xmlGetPropAsInt(node, "horsevendor");
    city->person_types[NPC_LORD_BRITISH - NPC_TALKER_COMPANION] = xmlGetPropAsInt(node, "lordbritish");
    city->person_types[NPC_HAWKWIND - NPC_TALKER_COMPANION] = xmlGetPropAsInt(node, "hawkwind");

    return city;
}

Portal *mapMgrInitPortalFromXml(xmlNodePtr node) {
    xmlNodePtr child;
    Portal *portal;
    char *prop;

    portal = malloc(sizeof(Portal));
    if (!portal)
        return NULL;
    portal->portalConditionsMet = NULL;
    portal->message = NULL;
    portal->retroActiveDest = NULL;
 
    portal->x = (unsigned short) xmlGetPropAsInt(node, "x");
    portal->y = (unsigned short) xmlGetPropAsInt(node, "y");
    portal->z = (unsigned short) xmlGetPropAsInt(node, "z");
    portal->destid = (unsigned char) xmlGetPropAsInt(node, "destmapid");
    
    if (xmlPropExists(node, "startx"))
        portal->startx = (unsigned short) xmlGetPropAsInt(node, "startx");
    else portal->startx = portal->x;

    if (xmlPropExists(node, "starty"))
        portal->starty = (unsigned short) xmlGetPropAsInt(node, "starty");
    else portal->starty = portal->y;

    portal->startlevel = (unsigned short) xmlGetPropAsInt(node, "startlevel");

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
            portal->retroActiveDest = malloc(sizeof(PortalDestination));
            
            portal->retroActiveDest->x = xmlGetPropAsInt(child, "x");
            portal->retroActiveDest->y = xmlGetPropAsInt(child, "y");
            portal->retroActiveDest->z = xmlGetPropAsInt(child, "z");
            portal->retroActiveDest->mapid = (unsigned char) xmlGetPropAsInt(child, "mapid");
        }
    }    
    return portal;
}

Shrine *mapMgrInitShrineFromXml(xmlNodePtr node) {
    Shrine *shrine;
    char *prop;

    shrine = malloc(sizeof(Shrine));
    if (!shrine)
        return NULL;

    shrine->virtue = xmlGetPropAsInt(node, "virtue");

    prop = xmlGetPropAsStr(node, "mantra");
    shrine->mantra = strdup(prop);
    xmlFree(prop);

    return shrine;
}

Dungeon *mapMgrInitDungeonFromXml(xmlNodePtr node) {
    Dungeon *dungeon;
    char *prop;

    dungeon = malloc(sizeof(Dungeon));
    if (!dungeon)
        return NULL;
    dungeon->n_rooms = xmlGetPropAsInt(node, "rooms");
    dungeon->rooms = NULL;

    prop = xmlGetPropAsStr(node, "name");
    dungeon->name = strdup(prop);
    xmlFree(prop);

    return dungeon;
}

void mapMgrCreateMoongateFromXml(xmlNodePtr node) {
    int phase;
    unsigned char x, y;

    phase = xmlGetPropAsInt(node, "phase");
    x = (unsigned char) xmlGetPropAsInt(node, "x");
    y = (unsigned char) xmlGetPropAsInt(node, "y");

    moongateAdd(phase, x, y);
}

int mapMgrInitCompressedChunkFromXml(xmlNodePtr node) {
    return xmlGetPropAsInt(node, "index");
}

void mapMgrRegister(Map *map) {
    ASSERT(mapMgrGetById(map->id) == NULL, "duplicate map id: %d (fname = %s)", map->id, map->fname);
    mapList = listAppend(mapList, map);
}

int mapCompareToId(Map *map, unsigned int id) {
    return map->id - id;
}

Map *mapMgrGetById(unsigned char id) {
    ListNode *n;
    n = listFind(mapList, (void *) (unsigned int) id, (ListComparator)&mapCompareToId);
    if (!n)
        return NULL;

    return n->data;
}
