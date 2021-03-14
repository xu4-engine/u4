/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include <algorithm>

#include "object.h"

#include "map.h"

using namespace std;

bool Object::setDirection(Direction d) {
    return tile.setDirection(d);
}

void Object::setMap(class Map *m) {
    if (find(maps.begin(), maps.end(), m) == maps.end())
        maps.push_back(m);
}

Map *Object::getMap() {
    if (maps.empty())
        return NULL;
    return maps.back();
}

void Object::remove() {
    unsigned int size = maps.size();
    for (unsigned int i = 0; i < size; i++) {
        if (i == size - 1)
            maps[i]->removeObject(this);
        else maps[i]->removeObject(this, false);
    }
}


#include "screen.h"
#include "game.h"
void Object::animateMovement()
{
    //TODO abstract movement - also make screen.h and game.h not required
    screenTileUpdate(&game->mapArea, prevCoords, false);
    if (screenTileUpdate(&game->mapArea, coords, false))
        screenWait(1);
}
