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
    ACTION_NONE     = 0x0,
    ACTION_ENTER    = 0x1,
    ACTION_KLIMB    = 0x2,
    ACTION_DESCEND  = 0x4   
} PortalTriggerAction;

typedef int (*PortalConditionsMet)(const struct _Portal *p);

typedef struct _PortalDestination {
    int x, y, z;
    unsigned char mapid;
} PortalDestination;

typedef struct _Portal {
    unsigned short x, y, z;
    unsigned char destid;
    unsigned short startx, starty, startlevel;
    PortalTriggerAction trigger_action;
    PortalConditionsMet portalConditionsMet;
    struct _PortalDestination *retroActiveDest;
    int saveLocation;
    char *message;
    TransportContext portalTransportRequisites;
    int exitPortal;
} Portal;

int usePortalAt(struct _Location *location, int x, int y, int z, PortalTriggerAction action);

#endif
