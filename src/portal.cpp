/*
 * portal.cpp
 */

#include "portal.h"

#include "city.h"
#include "config.h"
#include "dungeon.h"
#include "screen.h"
#include "xu4.h"

/**
 * Creates a dungeon ladder portal based on the action given
 */
void createDngLadder(Location *location, PortalTriggerAction action, Portal *p) {
    if (p) {
        p->destid = location->map->id;
        if (action == ACTION_KLIMB && location->coords.z == 0) {
            p->exitPortal = true;
            p->destid = 1;
        } else
            p->exitPortal = false;

        p->message = 0;
        p->portalConditionsMet = NULL;
        p->portalTransportRequisites = TRANSPORT_FOOT_OR_HORSE;
        p->retroActiveDest = NULL;
        p->saveLocation = false;
        p->start = location->coords;
        p->start.z += (action == ACTION_KLIMB) ? -1 : 1;
    }
}

/**
 * Finds a portal at the given (x,y,z) coords that will work with the action given
 * and uses it.  If in a dungeon and trying to use a ladder, it creates a portal
 * based on the ladder and uses it.
 */
int usePortalAt(Location *location, const Coords& coords, PortalTriggerAction action) {
    Map *destination;
    char msg[35] = {0};

    const Portal *portal = location->map->portalAt(coords, action);
    Portal dngLadder;

    /* didn't find a portal there */
    if (!portal) {

        /* if it's a dungeon, then ladders are predictable.  Create one! */
        if (location->context == CTX_DUNGEON) {
            Dungeon *dungeon = dynamic_cast<Dungeon *>(location->map);
            if ((action & ACTION_KLIMB) && dungeon->ladderUpAt(coords))
                createDngLadder(location, action, &dngLadder);
            else if ((action & ACTION_DESCEND) && dungeon->ladderDownAt(coords))
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

    destination = xu4.config->map(portal->destid);

    if (! portal->message) {
        switch(action) {
        case ACTION_DESCEND:
            sprintf(msg, "Descend down to level %d\n", portal->start.z+1);
            break;
        case ACTION_KLIMB:
            if (portal->exitPortal)
                sprintf(msg, "Klimb up!\nLeaving...\n");
            else
                sprintf(msg, "Klimb up!\nTo level %d\n", portal->start.z+1);
            break;
        case ACTION_ENTER:
            switch (destination->type) {
            case Map::CITY:
                {
                    City *city = dynamic_cast<City*>(destination);
                    screenMessage("Enter %s!\n\n%s\n\n", city->cityTypeStr(), city->getName());
                }
                break;
            case Map::SHRINE:
                screenMessage("Enter the %s!\n\n", destination->getName());
                break;
            case Map::DUNGEON:
#ifdef IOS
                U4IOS::testFlightPassCheckPoint("Enter " + destination->getName());
#endif
                screenMessage("Enter dungeon!\n\n%s\n\n", destination->getName());
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
    if (portal->message)
        screenMessage(xu4.config->confString(portal->message));
    else if (msg[0])
        screenMessage(msg);

    /* portal just exits to parent map */
    if (portal->exitPortal) {
        xu4.game->exitToParentMap();
        musicPlayLocale();
        return 1;
    }
    else if (portal->destid == location->map->id)
        location->coords = portal->start;
    else {
        xu4.game->setMap(destination, portal->saveLocation, portal);
        musicPlayLocale();
    }

    /* if the portal changes the map retroactively, do it here */
    /*
     * note that we use c->location instead of location, since
     * location has probably been invalidated above
     */
    if (portal->retroActiveDest && c->location->prev) {
        c->location->prev->coords = portal->retroActiveDest->coords;
        c->location->prev->map = xu4.config->map(portal->retroActiveDest->mapid);
    }

    if (destination->type == Map::SHRINE)
        static_cast<Shrine*>(destination)->enter();

    return 1;
}
