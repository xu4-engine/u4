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

typedef struct {
    unsigned int x, y;
    struct MapTag *destination;
    PortalTriggerAction trigger_action;
} Portal;

#endif
