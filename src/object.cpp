/*
 * object.cpp
 */

#include "object.h"
#include "context.h"
#include "game.h"
#include "map.h"
#include "screen.h"
#include "xu4.h"

#ifdef GPU_RENDER
#include "event.h"
#include "tileset.h"
#endif

Object::Object(Type type) :
  tile(0),
  prevTile(0),
  movement_behavior(MOVEMENT_FIXED),
  objType(type),
  animId(ANIM_UNUSED),
  focused(false),
  visible(true),
  animated(true),
  onMaps(0)
{}

Object::~Object() {
#ifdef GPU_RENDER
    // Must check if exiting game as the eventHandler may already be deleted.
    if (animId != ANIM_UNUSED && xu4.stage != StageExitGame)
        anim_setState(&xu4.eventHandler->flourishAnim, animId, ANIM_FREE);
#endif
}

bool Object::setDirection(Direction d) {
    return tile.setDirection(d);
}

/*
 * NOTE: This does not set prevCoords.
 */
void Object::placeOnMap(Map* map, const Coords& coords) {
    if (! onMaps || ! map->objectPresent(this))
        ++onMaps;

    setCoords(coords);

#ifdef GPU_RENDER
    /* Start frame animation */
    if (animId == ANIM_UNUSED) {
        const Tile* tileDef = map->tileset->get(tile.id);
        if (tileDef)
            animId = tileDef->startFrameAnim();
    }
#endif
}

/*
 * Remove object from any maps that it is a part of.
 *
 * If the object is not a PartyMember then it is also deleted.
 */
void Object::removeFromMaps() {
    Location* loc = c->location;
    while (onMaps && loc) {
        if (loc->map->removeObject(this, onMaps == 1))
            --onMaps;
        loc = loc->prev;
    }
}

void Object::animateMovement()
{
    //TODO abstract movement - also make screen.h and game.h not required
    screenTileUpdate(&xu4.game->mapArea, prevCoords);
    if (screenTileUpdate(&xu4.game->mapArea, coords))
        screenWait(1);
}
