/*
 * $Id$
 */

#ifndef OBJECT_H
#define OBJECT_H

#include "map.h"
#include "tile.h"

struct _Person;
struct _Monster;

typedef enum {
    MOVEMENT_FIXED,
    MOVEMENT_WANDER,
    MOVEMENT_FOLLOW_AVATAR,
    MOVEMENT_ATTACK_AVATAR
} ObjectMovementBehavior;

typedef enum {
    OBJECT_UNKNOWN,
    OBJECT_PERSON,
    OBJECT_MONSTER    
} ObjectType;

class Object {
public:
    Object() :
      movement_behavior(MOVEMENT_FIXED),
      objType(OBJECT_UNKNOWN),
      focused(false),
      visible(true),
      animated(true)
    {}

    // Methods
    const MapTile& getTile() const {
        return tile;
    }
    const MapTile& getPrevTile() const {
        return prevTile;
    }
    const MapCoords& getCoords() const {
        return coords;
    }
    const MapCoords& getPrevCoords() const {
        return prevCoords;
    }
    const ObjectMovementBehavior getMovementBehavior() const {
        return movement_behavior;
    }
    const ObjectType getType() const {
        return objType;
    }
    bool hasFocus() const {
        return focused;
    }
    bool isVisible() const {
        return visible;
    }
    bool isAnimated() const {
        return animated;
    }

    void setTile(MapTile t) {
        tile = t;
    }
    void setPrevTile(MapTile t) {
        prevTile = t;
    }
    void setCoords(MapCoords c) {
        prevCoords = coords;
        coords = c;
    }
    void setPrevCoords(MapCoords c) {
        prevCoords = c;
    }
    void setMovementBehavior(ObjectMovementBehavior b) {
        movement_behavior = b;
    }
    void setType(ObjectType t) {
        objType = t;
    }
    void setFocus(bool f = true) {
        focused = f;
    }
    void setVisible(bool v = true) {
        visible = v;
    }
    void setAnimated(bool a = true) {
        animated = a;
    }

    bool move(Direction d) {
        MapCoords new_coords = coords;
        if (new_coords.move(d) != coords) {
            coords = new_coords;
            return true;
        }
        return false;
    }
    bool setDirection(Direction d) {
        return tileSetDirection(&tile, d);
    }
    void advanceFrame() {
        tileAdvanceFrame(&tile);
    }
    // Properties
    Object *next;
    union {
        const struct _Person *person;
        const struct _Monster *monster;
    };

private:
    MapTile tile, prevTile;
    MapCoords coords, prevCoords;
    ObjectMovementBehavior movement_behavior;
    ObjectType objType;
    
    bool focused;
    bool visible;
    bool animated;    
};

#endif
