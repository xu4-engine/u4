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
#include "mapmgr.h"
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

ListNode *mapList = NULL;

void mapMgrInit() {
    xmlDocPtr doc;
    xmlNodePtr root, node;
    Map *map;
    U4FILE *world, *ult, *tlk, *con, *dng;

    doc = xmlParse("maps.xml");
    root = xmlDocGetRootElement(doc);
    if (xmlStrcmp(root->name, (const xmlChar *) "maps") != 0)
        errorFatal("malformed maps.xml");

    for (node = root->xmlChildrenNode; node; node = node->next) {
        if (xmlNodeIsText(node) ||
            xmlStrcmp(node->name, (const xmlChar *) "map") != 0)
            continue;

        map = mapMgrInitMapFromXml(node);

        switch (map->type) {
        case MAPTYPE_WORLD:
            world = u4fopen("world.map");
            if (!world)
                errorFatal("unable to load map data");
            mapReadWorld(map, world);
            u4fclose(world);
            break;

        case MAPTYPE_TOWN:
        case MAPTYPE_VILLAGE:
        case MAPTYPE_CASTLE:
        case MAPTYPE_RUIN:
            ult = u4fopen(map->fname);
            tlk = u4fopen(map->city->tlk_fname);
            if (!ult || !tlk)
                errorFatal("unable to load map data");

            mapRead(map->city, ult, tlk);
            u4fclose(ult);
            u4fclose(tlk);
            break;

        case MAPTYPE_SHRINE:
        case MAPTYPE_COMBAT:
            con = u4fopen(map->fname);
            if (!con)
                errorFatal("unable to load map data");
            mapReadCon(map, con);
            u4fclose(con);
            break;

        case MAPTYPE_DUNGEON:
            dng = u4fopen(map->fname);
            if (!dng)
                errorFatal("unable to load map data");
            mapReadDng(map, dng);
            u4fclose(dng);
            break;
        }
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
    map->id = 0;

    return map;
}

Map *mapMgrInitMapFromXml(xmlNodePtr node) {
    Map *map;
    char *prop;
    xmlNodePtr child;
    ListNode *portals = NULL;

    map = mapMgrInitMap();
    if (!map)
        return NULL;

    map->id = (unsigned char)xmlGetPropAsInt(node, (const xmlChar *) "id");

    prop = xmlGetPropAsStr(node, (const xmlChar *) "type");
    if (strcmp(prop, "world") == 0)
        map->type = MAPTYPE_WORLD;
    else if (strcmp(prop, "town") == 0)
        map->type = MAPTYPE_TOWN;
    else if (strcmp(prop, "village") == 0)
        map->type = MAPTYPE_VILLAGE;
    else if (strcmp(prop, "castle") == 0)
        map->type = MAPTYPE_CASTLE;
    else if (strcmp(prop, "ruins") == 0)
        map->type = MAPTYPE_RUIN;
    else if (strcmp(prop, "shrine") == 0)
        map->type = MAPTYPE_SHRINE;
    else if (strcmp(prop, "combat") == 0)
        map->type = MAPTYPE_COMBAT;
    else if (strcmp(prop, "dungeon") == 0)
        map->type = MAPTYPE_DUNGEON;
    else
        errorFatal("unknown type: %s", prop);
    xmlFree(prop);

    prop = xmlGetPropAsStr(node, (const xmlChar *) "fname");
    map->fname = strdup(prop);
    xmlFree(prop);

    map->width = xmlGetPropAsInt(node, (const xmlChar *) "width");
    map->height = xmlGetPropAsInt(node, (const xmlChar *) "height");
    map->levels = xmlGetPropAsInt(node, (const xmlChar *) "levels");

    prop = xmlGetPropAsStr(node, (const xmlChar *) "borderbehavior");
    if (strcmp(prop, "wrap") == 0)
        map->border_behavior = BORDER_WRAP;
    else if (strcmp(prop, "exit") == 0)
        map->border_behavior = BORDER_EXIT2PARENT;
    else if (strcmp(prop, "fixed") == 0)
        map->border_behavior = BORDER_FIXED;
    else
        errorFatal("unknown borderbehavoir: %s", prop);
    xmlFree(prop);
    
    if (xmlGetPropAsBool(node, (const xmlChar *) "showavatar"))
        map->flags |= SHOW_AVATAR;

    if (xmlGetPropAsBool(node, (const xmlChar *) "nolineofsight"))
        map->flags |= NO_LINE_OF_SIGHT;
    
    if (xmlGetPropAsBool(node, (const xmlChar *) "firstperson"))
        map->flags |= FIRST_PERSON;

    map->music = xmlGetPropAsInt(node, (const xmlChar *) "music");

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

    prop = xmlGetPropAsStr(node, (const xmlChar *) "name");
    city->name = strdup(prop);
    xmlFree(prop);

    prop = xmlGetPropAsStr(node, (const xmlChar *) "tlk_fname");
    city->tlk_fname = strdup(prop);
    xmlFree(prop);

    city->person_types[NPC_TALKER_COMPANION - NPC_TALKER_COMPANION] = xmlGetPropAsInt(node, (const xmlChar *) "companion");
    city->person_types[NPC_VENDOR_WEAPONS - NPC_TALKER_COMPANION] = xmlGetPropAsInt(node, (const xmlChar *) "weaponsvendor");
    city->person_types[NPC_VENDOR_ARMOR - NPC_TALKER_COMPANION] = xmlGetPropAsInt(node, (const xmlChar *) "armorvendor");
    city->person_types[NPC_VENDOR_FOOD - NPC_TALKER_COMPANION] = xmlGetPropAsInt(node, (const xmlChar *) "foodvendor");
    city->person_types[NPC_VENDOR_TAVERN - NPC_TALKER_COMPANION] = xmlGetPropAsInt(node, (const xmlChar *) "tavernkeeper");
    city->person_types[NPC_VENDOR_REAGENTS - NPC_TALKER_COMPANION] = xmlGetPropAsInt(node, (const xmlChar *) "reagentsvendor");
    city->person_types[NPC_VENDOR_HEALER - NPC_TALKER_COMPANION] = xmlGetPropAsInt(node, (const xmlChar *) "healer");
    city->person_types[NPC_VENDOR_INN - NPC_TALKER_COMPANION] = xmlGetPropAsInt(node, (const xmlChar *) "innkeeper");
    city->person_types[NPC_VENDOR_GUILD - NPC_TALKER_COMPANION] = xmlGetPropAsInt(node, (const xmlChar *) "guildvendor");
    city->person_types[NPC_VENDOR_STABLE - NPC_TALKER_COMPANION] = xmlGetPropAsInt(node, (const xmlChar *) "horsevendor");
    city->person_types[NPC_LORD_BRITISH - NPC_TALKER_COMPANION] = xmlGetPropAsInt(node, (const xmlChar *) "lordbritish");
    city->person_types[NPC_HAWKWIND - NPC_TALKER_COMPANION] = xmlGetPropAsInt(node, (const xmlChar *) "hawkwind");

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
 
    portal->x = (unsigned short) xmlGetPropAsInt(node, (const xmlChar *) "x");
    portal->y = (unsigned short) xmlGetPropAsInt(node, (const xmlChar *) "y");
    portal->z = (unsigned short) xmlGetPropAsInt(node, (const xmlChar *) "z");
    portal->destid = (unsigned char) xmlGetPropAsInt(node, (const xmlChar *) "destmapid");
    
    if (xmlGetPropAsStr(node, (const xmlChar *) "startx") != NULL)
        portal->startx = (unsigned short) xmlGetPropAsInt(node, (const xmlChar *) "startx");
    else portal->startx = portal->x;

    if (xmlGetPropAsStr(node, (const xmlChar *) "starty") != NULL)
        portal->starty = (unsigned short) xmlGetPropAsInt(node, (const xmlChar *) "starty");
    else portal->starty = portal->y;

    portal->startlevel = (unsigned short) xmlGetPropAsInt(node, (const xmlChar *) "startlevel");

    prop = xmlGetPropAsStr(node, (const xmlChar *) "action");
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
    
    prop = xmlGetPropAsStr(node, (const xmlChar *) "condition");
    if (prop) {
        if (strcmp(prop, "shrine") == 0)
            portal->portalConditionsMet = &shrineCanEnter;
        else if (strcmp(prop, "abyss") == 0)
            portal->portalConditionsMet = &isAbyssOpened;
        else
            errorFatal("unknown portalConditionsMet: %s", prop);
        xmlFree(prop);
    }

    if (xmlGetPropAsStr(node, (const xmlChar *)"savelocation") != NULL)
        portal->saveLocation = xmlGetPropAsBool(node, (const xmlChar *) "savelocation");
    else portal->saveLocation = 0;

    prop = xmlGetPropAsStr(node, (const xmlChar *) "message");
    if (prop) {
        portal->message = strdup(prop);
        xmlFree(prop);
    }

    prop = xmlGetPropAsStr(node, (const xmlChar *) "transport");
    if (strcmp(prop, "foot") == 0)
        portal->portalTransportRequisites = TRANSPORT_FOOT;
    else if (strcmp(prop, "footorhorse") == 0)
        portal->portalTransportRequisites = TRANSPORT_FOOT_OR_HORSE;
    else
        errorFatal("unknown transport: %s", prop);
    xmlFree(prop);

    if (xmlGetPropAsBool(node, (const xmlChar *) "exits"))
        portal->exitPortal = 1;
    else portal->exitPortal = 0;

    for (child = node->xmlChildrenNode; child; child = child->next) {
        if (xmlNodeIsText(child))
            continue;

        if (xmlStrcmp(child->name, (const xmlChar *) "retroActiveDest") == 0) {
            portal->retroActiveDest = malloc(sizeof(PortalDestination));
            
            portal->retroActiveDest->x = xmlGetPropAsInt(child, (const xmlChar *) "x");
            portal->retroActiveDest->y = xmlGetPropAsInt(child, (const xmlChar *) "y");
            portal->retroActiveDest->z = xmlGetPropAsInt(child, (const xmlChar *) "z");
            portal->retroActiveDest->mapid = (unsigned char) xmlGetPropAsInt(child, (const xmlChar *) "mapid");
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

    shrine->virtue = xmlGetPropAsInt(node, (const xmlChar *) "virtue");

    prop = xmlGetPropAsStr(node, (const xmlChar *) "mantra");
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
    dungeon->n_rooms = xmlGetPropAsInt(node, (const xmlChar *) "rooms");
    dungeon->rooms = NULL;

    prop = xmlGetPropAsStr(node, (const xmlChar *) "name");
    dungeon->name = strdup(prop);
    xmlFree(prop);

    return dungeon;
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
