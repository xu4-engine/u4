/*
 * $Id$
 */

#include "object.h"

bool Object::setDirection(Direction d) {
    return tileSetDirection(&tile, d);
}

void Object::advanceFrame() {
    tileAdvanceFrame(&tile);
}
