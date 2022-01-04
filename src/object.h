/*
 * object.h
 */

#ifndef OBJECT_H
#define OBJECT_H

#include "anim.h"
#include "coords.h"
#include "tile.h"

typedef enum {
    MOVEMENT_FIXED,
    MOVEMENT_WANDER,
    MOVEMENT_FOLLOW_AVATAR,
    MOVEMENT_ATTACK_AVATAR,
    MOVEMENT_FOLLOW_PAUSE   // Pause a turn then resume MOVEMENT_FOLLOW_AVATAR
} ObjectMovement;

class Map;

class Object {
public:
    enum Type {
        UNKNOWN,
        CREATURE,
        PERSON
    };

    Object(Type type = UNKNOWN);
    virtual ~Object();

    // Methods
    void setTile(const Tile *t) { tile = t->getId(); }

    void updateCoords(const Coords& c) {
        prevCoords = coords;
        coords = c;
    }

    void placeOnMap(Map*, const Coords&);
    void removeFromMaps();
    bool setDirection(Direction d);
    void animateMovement();
    void animControl(int animState);

    // Properties
    MapTile tile, prevTile;
    Coords coords, prevCoords;
    ObjectMovement movement;
    Type objType;
    AnimId animId;
    bool focused;
    bool visible;
    bool animated;
    uint8_t onMaps;
};

#endif
