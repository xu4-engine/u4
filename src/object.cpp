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
    for (unsigned int i = 0; i < maps.size(); i++) {
        if (i == maps.size() - 1)
            maps[i]->removeObject(this);
        else maps[i]->removeObject(this, false);
    }
}
