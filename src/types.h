/**
 * $Id$
 */

#ifndef TYPEDEFS_H
#define TYPEDEFS_H

#include "direction.h"
//#include "tileset.h"

class Tile;

typedef unsigned int TileId;
typedef unsigned char MapId;

typedef enum {
    FAST,
    SLOW,
    VSLOW,
    VVSLOW
} TileSpeed;

typedef enum {
    EFFECT_NONE,
    EFFECT_FIRE,
    EFFECT_SLEEP,
    EFFECT_POISON,
    EFFECT_POISONFIELD,
    EFFECT_ELECTRICITY,
    EFFECT_LAVA
} TileEffect;

typedef enum {
    ANIM_NONE,
    ANIM_SCROLL,
    ANIM_CAMPFIRE,
    ANIM_CITYFLAG,
    ANIM_CASTLEFLAG,
    ANIM_SHIPFLAG,
    ANIM_LCBFLAG,
    ANIM_FRAMES
} TileAnimationStyle;

/**
 * A MapTile is a specific instance of a Tile.
 */
class MapTile {
public:
    MapTile() : id(0), frame(0) {}
    MapTile(const TileId &i, unsigned char f = 0) : id(i), frame(f), freezeAnimation(false) {}
    MapTile(const MapTile &t) : id(t.id), frame(t.frame), freezeAnimation(t.freezeAnimation) {}

    TileId getId() const            {return id;}
    unsigned char getFrame() const  {return frame;}
    bool getFreezeAnimation() const {return freezeAnimation;}

    MapTile& operator=(const MapTile &m) {
        id = m.id;
        frame = m.frame;
        freezeAnimation = m.freezeAnimation;
        return *this;
    }

    bool operator==(const MapTile &m) const  { return id == m.id; }
    bool operator==(const TileId &i) const   { return id == i; }
    bool operator!=(const MapTile &m) const  { return id != m.id; }
    bool operator!=(const TileId &i) const   { return id != i; }
    bool operator<(const MapTile &m) const   { return id < m.id; } /* for std::less */

    Direction getDirection() const;
    bool setDirection(Direction d);

    const Tile *getTileType() const;

    // Properties
    TileId id;
    unsigned char frame;
    bool freezeAnimation;
};

/**
 * An Uncopyable has no default copy constructor of operator=.  A subclass may derive from
 * Uncopyable at any level of visibility, even private, and subclasses will not have a default copy
 * constructor or operator=. See also, boost::noncopyable Uncopyable (from the Boost project) and
 * Item 6 from Scott Meyers Effective C++.
 */
class Uncopyable {
protected:
    Uncopyable() {}
    ~Uncopyable() {}

private:
    Uncopyable(const Uncopyable&);
    const Uncopyable &operator=(const Uncopyable&);
};

#endif
