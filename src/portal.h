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

typedef struct _Portal {
    unsigned short x, y, z;
    struct _Map *destination;
    PortalTriggerAction trigger_action;
} Portal;

#endif
