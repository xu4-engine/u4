/*
 * object.cpp
 */

#include <algorithm>

#include "object.h"
#include "game.h"
#include "map.h"
#include "screen.h"
#include "xu4.h"

#ifdef GPU_RENDER
#include "event.h"
#include "tileset.h"
#endif

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
    if (find(maps.begin(), maps.end(), map) == maps.end())
        maps.push_back(map);

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
    size_t size = maps.size();
    for (size_t i = 0; i < size; i++) {
        bool lastMap = (i == size - 1);
        maps[i]->removeObject(this, lastMap);
    }
}

void Object::animateMovement()
{
    //TODO abstract movement - also make screen.h and game.h not required
    screenTileUpdate(&xu4.game->mapArea, prevCoords);
    if (screenTileUpdate(&xu4.game->mapArea, coords))
        screenWait(1);
}
