/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise
#include "object.h"

bool Object::setDirection(Direction d) {
    return tile.setDirection(d);
}

void Object::advanceFrame() {
    tile.advanceFrame();
}
