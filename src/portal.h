/*
 * $Id$
 */

#ifndef PORTAL_H
#define PORTAL_H

#include "context.h"

struct _Map;
struct _Portal;
struct _Location;

typedef enum {
    ACTION_NONE,
    ACTION_ENTER,
    ACTION_KLIMB,
    ACTION_DESCEND    
} PortalTriggerAction;

typedef int (*PortalConditionsMet)(const struct _Portal *p);

typedef struct _PortalDestination {
    int x, y, z;
    struct _Map *map;
} PortalDestination;

typedef struct _Portal {
    unsigned short x, y, z;
    struct _Map *destination;
    unsigned short startx, starty, startlevel;
    PortalTriggerAction trigger_action;
    PortalConditionsMet portalConditionsMet;
    struct _PortalDestination *retroActiveDest;
    int saveLocation;
    const char *message;
    TransportContext portalTransportRequisites;
} Portal;

int usePortalAt(struct _Location *location, int x, int y, int z, PortalTriggerAction action);

#endif
