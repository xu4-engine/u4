/*
 * $Id$
 */

#include "portal.h"

#include "city.h"
#include "context.h"
#include "dungeon.h"
#include "game.h"
#include "location.h"
#include "names.h"
#include "screen.h"
#include "shrine.h"

int usePortalAt(Location *location, int x, int y, int z, PortalTriggerAction action) {
    char *msg = NULL;
    
    /* find a portal at the specified location */
    const Portal *portal = mapPortalAt(c->location->map, x, y, z);

    /* didn't find a portal there */
    if (!portal)
        return 0;
    /* action didn't work for that portal */
    else if (portal && (portal->trigger_action != action) && (portal->trigger_action != ACTION_NONE))
        return 0;
    /* conditions not met for portal to work */
    else if (portal && portal->portalConditionsMet && !(*portal->portalConditionsMet)(portal))
        return 0;
    
    if (portal->message)
        screenMessage("%s", portal->message);
    else {
        switch(action) {
        case ACTION_DESCEND:    msg = "Descend to first floor!\n"; break;
        case ACTION_KLIMB:      msg = "Klimb to second floor!\n"; break;
        case ACTION_ENTER:
            switch (portal->destination->type) {
            case MAP_TOWN:
                screenMessage("Enter towne!\n\n%s\n\n", portal->destination->city->name);
                break;
            case MAP_VILLAGE:
                screenMessage("Enter village!\n\n%s\n\n", portal->destination->city->name);
                break;
            case MAP_CASTLE:
                screenMessage("Enter castle!\n\n%s\n\n", portal->destination->city->name);
                break;
            case MAP_RUIN:
                screenMessage("Enter ruin!\n\n%s\n\n", portal->destination->city->name);
                break;
            case MAP_SHRINE:
                screenMessage("Enter the Shrine of %s!\n\n", getVirtueName(portal->destination->shrine->virtue));
                break;
            case MAP_DUNGEON:
                screenMessage("Enter dungeon!\n\n%s\n\n", portal->destination->dungeon->name);
                break;
            default:
                break;
            }
            break;
        case ACTION_NONE:
        default: break;
        }
    }

    /* must use portals on foot or on horse */
    if ((c->transportContext & ~TRANSPORT_FOOT_OR_HORSE) ||
        (portal->destination->type == MAP_DUNGEON && c->transportContext & ~TRANSPORT_FOOT)) {  
        screenMessage("Only on foot!\n");
        return 1;
    }
    
    gameSetMap(c, portal->destination, portal->saveLocation, portal);
    musicPlay();

    /* if the portal changes the map retroactively, do it here */
    if (portal->retroActiveDest && c->location->prev) {
        c->location->prev->x = portal->retroActiveDest->x;
        c->location->prev->y = portal->retroActiveDest->y;
        c->location->prev->z = portal->retroActiveDest->z;
        c->location->prev->map = portal->retroActiveDest->map;
    }

    if (portal->destination->type == MAP_SHRINE)        
        shrineEnter(portal->destination->shrine);

    return 1;
}
