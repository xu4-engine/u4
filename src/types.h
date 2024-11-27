/**
 * $Id$
 */

#ifndef TYPEDEFS_H
#define TYPEDEFS_H

#include <stdint.h>
#include "direction.h"

#ifdef _MSC_VER
#if _MSC_VER > 1600
#define strcasecmp  _stricmp
#define strncasecmp _strnicmp
#endif
#endif

class Tile;
typedef uint16_t TileId;
typedef uint16_t VisualId;
typedef uint8_t  MapId;

// StringId and Symbol are similar, but symbol names do not allow whitespace.
typedef uint32_t StringId;
typedef uint16_t Symbol;

#define SYM_UNSET   0

// VisualId macros
#define MAKE_VID(bank,idx)  ((bank << 12) | (idx))
#define VID_BANK(vid)       (vid >> 12)
#define VID_INDEX(vid)      (vid & 0xfff)
#define VID_UNSET   0xffff

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
    EFFECT_LAVA,
    EFFECT_ROCKS
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
    MapTile(const TileId &i, uint8_t f = 0) : id(i), frame(f), freezeAnimation(false) {}
    MapTile(const MapTile &t) : id(t.id), frame(t.frame), freezeAnimation(t.freezeAnimation) {}

    TileId getId() const            {return id;}
    uint8_t getFrame() const        {return frame;}
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
    uint8_t frame;
    bool freezeAnimation;
};

#endif
