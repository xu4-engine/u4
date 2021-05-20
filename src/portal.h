/*
 * portal.h
 */

#ifndef PORTAL_H
#define PORTAL_H

#include <string.h>
#include "context.h"
#include "map.h"

typedef enum {
    ACTION_NONE         = 0x0,
    ACTION_ENTER        = 0x1,
    ACTION_KLIMB        = 0x2,
    ACTION_DESCEND      = 0x4,
    ACTION_EXIT_NORTH   = 0x8,
    ACTION_EXIT_EAST    = 0x10,
    ACTION_EXIT_SOUTH   = 0x20,
    ACTION_EXIT_WEST    = 0x40
} PortalTriggerAction;

typedef bool (*PortalConditionsMet)(const Portal *p);

struct PortalDestination {
    MapCoords coords;
    MapId mapid;
};

struct Portal {
    PortalConditionsMet portalConditionsMet;
    PortalDestination* retroActiveDest;
    MapCoords coords;
    MapCoords start;
    StringId message;
    PortalTriggerAction trigger_action;
    TransportContext portalTransportRequisites;
    MapId destid;
    bool saveLocation;
    bool exitPortal;
};

void createDngLadder(Location *location, PortalTriggerAction action, Portal *p);
int usePortalAt(Location *location, MapCoords coords, PortalTriggerAction action);

#endif
