/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include "portal.h"

#include "annotation.h"
#include "city.h"
#include "context.h"
#include "dungeon.h"
#include "game.h"
#include "location.h"
#include "mapmgr.h"
#include "names.h"
#include "screen.h"
#include "shrine.h"
#include "tile.h"

/**
 * Creates a dungeon ladder portal based on the action given
 */
void createDngLadder(Location *location, PortalTriggerAction action, Portal *p) {
    if (!p) return;
    else {
        p->destid = location->map->id;
        if (action == ACTION_KLIMB && location->coords.z == 0) {
            p->exitPortal = 1;
            p->destid = 1;
        }
        else p->exitPortal = 0;
        p->message = NULL;
        p->portalConditionsMet = NULL;
        p->portalTransportRequisites = TRANSPORT_FOOT_OR_HORSE;
        p->retroActiveDest = NULL;
        p->saveLocation = 0;
        p->start = location->coords;
        p->start.z += (action == ACTION_KLIMB) ? -1 : 1;
    }
}

/**
 * Finds a portal at the given (x,y,z) coords that will work with the action given
 * and uses it.  If in a dungeon and trying to use a ladder, it creates a portal
 * based on the ladder and uses it.
 */
int usePortalAt(Location *location, MapCoords coords, PortalTriggerAction action) {
    Map *destination;
    char msg[32] = {0};
    
    const Portal *portal = location->map->portalAt(coords, action);
    Portal dngLadder;

    /* didn't find a portal there */
    if (!portal) {
        
        /* if it's a dungeon, then ladders are predictable.  Create one! */
        if (location->context == CTX_DUNGEON) {
            if ((action & ACTION_KLIMB) && dungeonLadderUpAt(location->map, coords)) 
                createDngLadder(location, action, &dngLadder);                
            else if ((action & ACTION_DESCEND) && dungeonLadderDownAt(location->map, coords))
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
            sprintf(msg, "Descend down to level %d\n", portal->start.z+1);
            break;
        case ACTION_KLIMB:            
            if (portal->exitPortal)
                sprintf(msg, "Klimb up!\nLeaving...\n");
            else sprintf(msg, "Klimb up!\nTo level %d\n", portal->start.z+1);
            break;
        case ACTION_ENTER:
            switch (destination->type) {
            case MAPTYPE_CITY: 
                {
                    City *city = dynamic_cast<City*>(destination);
                    screenMessage("Enter %s!\n\n%s\n\n", city->type.c_str(), city->getName().c_str());
                }
                break;            
            case MAPTYPE_SHRINE:
                screenMessage("Enter the %s!\n\n", destination->getName().c_str());
                break;
            case MAPTYPE_DUNGEON:
                screenMessage("Enter dungeon!\n\n%s\n\n", destination->getName().c_str());
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
        gameExitToParentMap();
        musicPlay();
        return 1;
    }
    else if (portal->destid == location->map->id)
        location->coords = portal->start;        
    
    else {
        gameSetMap(destination, portal->saveLocation, portal);
        musicPlay();
    }

    /* if the portal changes the map retroactively, do it here */
    if (portal->retroActiveDest && location->prev) {
        location->prev->coords = portal->retroActiveDest->coords;        
        location->prev->map = mapMgrGetById(portal->retroActiveDest->mapid);
    }

    if (destination->type == MAPTYPE_SHRINE) {
        Shrine *shrine = dynamic_cast<Shrine*>(destination);
        shrine->enter();
    }

    return 1;
}
