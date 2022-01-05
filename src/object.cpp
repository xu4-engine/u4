/*
 * object.cpp
 */

#include "object.h"
#include "event.h"
#include "context.h"
#include "game.h"
#include "map.h"
#include "screen.h"
#include "tileset.h"
#include "xu4.h"

extern bool isPartyMember(const Object*);

Object::Object(Type type) :
  tile(0),
  prevTile(0),
  movement(MOVEMENT_FIXED),
  objType(type),
  animId(ANIM_UNUSED),
  focused(false),
  visible(true),
  animated(true),
  onMaps(0)
{}

Object::~Object() {
    // Must check if exiting game as the eventHandler may already be deleted.
    if (animId != ANIM_UNUSED && xu4.stage != StageExitGame)
        anim_setState(&xu4.eventHandler->flourishAnim, animId, ANIM_FREE);
}

bool Object::setDirection(Direction d) {
    return tile.setDirection(d);
}

/*
 * Sets Object coords & prevCoords to the specified position.
 */
void Object::placeOnMap(Map* map, const Coords& pos) {
    if (! onMaps || ! map->objectPresent(this))
        ++onMaps;

    coords = prevCoords = pos;

    /* Start frame animation */
    if (animId == ANIM_UNUSED) {
        const Tile* tileDef = map->tileset->get(tile.id);
        if (tileDef)
            animId = tileDef->startFrameAnim();
    }
}

/*
 * Remove object from any maps that it is a part of.
 *
 * If the object is not a PartyMember then it is also deleted.
 */
void Object::removeFromMaps() {
    Location* loc = c->location;
    while (onMaps && loc) {
        if (loc->map->removeObject(this, false))
            --onMaps;
        loc = loc->prev;
    }

    if (! isPartyMember(this))
        delete this;
}

void Object::animateMovement()
{
    //TODO abstract movement - also make screen.h and game.h not required
    screenTileUpdate(&xu4.game->mapArea, prevCoords);
    if (screenTileUpdate(&xu4.game->mapArea, coords))
        screenWait(1);
}

/*
 * Set frame animation state.
 */
void Object::animControl(int animState)
{
    if (animId != ANIM_UNUSED)
        anim_setState(&xu4.eventHandler->flourishAnim, animId, animState);
}
