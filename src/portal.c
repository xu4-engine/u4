/*
 * $Id$
 */

#include <string.h>

#include "portal.h"

#include "city.h"
#include "context.h"
#include "dungeon.h"
#include "game.h"
#include "location.h"
#include "names.h"
#include "screen.h"
#include "shrine.h"
#include "mapmgr.h"

/**
 * Creates a dungeon ladder portal based on the action given
 */
void createDngLadder(Location *location, PortalTriggerAction action, Portal *p) {
    if (!p) return;
    else {
        p->destid = location->map->id;
        if (action == ACTION_KLIMB && location->z == 0) {
            p->exitPortal = 1;
            p->destid = 1;
        }
        else p->exitPortal = 0;
        p->message = NULL;
        p->portalConditionsMet = NULL;
        p->portalTransportRequisites = TRANSPORT_FOOT_OR_HORSE;
        p->retroActiveDest = NULL;
        p->saveLocation = 0;
        p->startlevel = (action == ACTION_KLIMB) ? location->z - 1 : location->z + 1;
        p->startx = location->x;
        p->starty = location->y;        
    }
}

/**
 * Finds a portal at the given (x,y,z) coords that will work with the action given
 * and uses it.  If in a dungeon and trying to use a ladder, it creates a portal
 * based on the ladder and uses it.
 */
int usePortalAt(Location *location, int x, int y, int z, PortalTriggerAction action) {
    Map *destination;
    char msg[32] = {0};
    
    const Portal *portal = mapPortalAt(location->map, x, y, z, action);
    Portal dngLadder;

    /* didn't find a portal there */
    if (!portal) {
        
        /* if it's a dungeon, then ladders are predictable.  Create one! */
        if (location->context == CTX_DUNGEON) {
            unsigned char tile = mapGetTileFromData(location->map, x, y, z);
            if ((action & ACTION_KLIMB) && (tile == 0x10 || tile == 0x30)) 
                createDngLadder(location, action, &dngLadder);                
            else if ((action & ACTION_DESCEND) && (tile == 0x20 || tile == 0x30))
                createDngLadder(location, action, &dngLadder);
            else return 0;
            portal = &dngLadder;
        }
        else return 0;
    }

    /* conditions not met for portal to work */
    if (portal && portal->portalConditionsMet && !(*portal->portalConditionsMet)(portal))
        return 0;
    /* must klimb or descend on foot! */
    else if (c->transportContext & ~TRANSPORT_FOOT && (action == ACTION_KLIMB || action == ACTION_DESCEND)) {
        screenMessage("%sOnly on foot!\n", action == ACTION_KLIMB ? "Klimb\n" : "");
        return 1;
    }    
    
    destination = mapMgrGetById(portal->destid);

    if (!portal->message) {

        switch(action) {
        case ACTION_DESCEND:            
            sprintf(msg, "Descend down to level %d\n", portal->startlevel+1);
            break;
        case ACTION_KLIMB:            
            if (portal->exitPortal)
                sprintf(msg, "Klimb up!\nLeaving...\n");
            else sprintf(msg, "Klimb up!\nTo level %d\n", portal->startlevel+1);
            break;
        case ACTION_ENTER:
            switch (destination->type) {
            case MAPTYPE_TOWN:
                screenMessage("Enter towne!\n\n%s\n\n", destination->city->name);
                break;
            case MAPTYPE_VILLAGE:
                screenMessage("Enter village!\n\n%s\n\n", destination->city->name);
                break;
            case MAPTYPE_CASTLE:
                screenMessage("Enter castle!\n\n%s\n\n", destination->city->name);
                break;
            case MAPTYPE_RUIN:
                screenMessage("Enter ruin!\n\n%s\n\n", destination->city->name);
                break;
            case MAPTYPE_SHRINE:
                screenMessage("Enter the Shrine of %s!\n\n", getVirtueName(destination->shrine->virtue));
                break;
            case MAPTYPE_DUNGEON:
                screenMessage("Enter dungeon!\n\n%s\n\n", destination->dungeon->name);
                break;
            default:
                break;
            }
            break;
        case ACTION_NONE:
        default: break;
        }
    }

    /* check the transportation requisites of the portal */
    if (c->transportContext & ~portal->portalTransportRequisites) {
        screenMessage("Only on foot!\n");        
        return 1;
    }
    /* ok, we know the portal is going to work -- now display the custom message, if any */
    else if (portal->message || strlen(msg))
        screenMessage("%s", portal->message ? portal->message : msg);    

    /* portal just exits to parent map */
    if (portal->exitPortal) {        
        gameExitToParentMap(c);
        musicPlay();
        return 1;
    }
    else if (portal->destid == location->map->id) {
        location->x = portal->startx;
        location->y = portal->starty;
        location->z = portal->startlevel;
    }
    else {
        gameSetMap(c, destination, portal->saveLocation, portal);
        musicPlay();
    }

    /* if the portal changes the map retroactively, do it here */
    if (portal->retroActiveDest && location->prev) {
        location->prev->x = portal->retroActiveDest->x;
        location->prev->y = portal->retroActiveDest->y;
        location->prev->z = portal->retroActiveDest->z;
        location->prev->map = mapMgrGetById(portal->retroActiveDest->mapid);
    }

    if (destination->type == MAPTYPE_SHRINE)        
        shrineEnter(destination->shrine);

    return 1;
}
