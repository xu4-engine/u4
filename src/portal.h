/*
 * $Id$
 */

#ifndef PORTAL_H
#define PORTAL_H

typedef enum {
    ACTION_ENTER,
    ACTION_KLIMB,
    ACTION_DESCEND
} PortalTriggerAction;

typedef int (*PortalConditionsMet)(void);

typedef struct _Portal {
    unsigned short x, y, z;
    struct _Map *destination;
    unsigned short startx, starty, startlevel;
    PortalTriggerAction trigger_action;
    PortalConditionsMet portalConditionsMet;
} Portal;

#endif
