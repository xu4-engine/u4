/*
 * $Id$
 */

#ifndef ANNOTATION_H
#define ANNOTATION_H

#include <list>

#include "coords.h"
#include "types.h"

class Annotation;

/**
 * Annotation are updates to a map.
 * There are three types of annotations:
 * - permanent: lasts until annotationClear is called
 * - turn based: lasts a given number of cycles
 * - time based: lasts a given number of time units (1/4 seconds)
 */
class Annotation {
public:
    typedef std::list<Annotation> List;

    Annotation(const Coords &coords, MapTile tile, bool visual = false, bool coverUp = false);

    void debug_output() const;

    // Getters
    const Coords& getCoords() const {return coords; } /**< Returns the coordinates of the annotation */
    MapTile& getTile()              {return tile;   } /**< Returns the annotation's tile */
    bool isVisualOnly() const {return visual; } /**< Returns true for visual-only annotations */
    int getTTL() const        {return ttl;    } /**< Returns the number of turns the annotation has left to live */
    bool isCoverUp()                {return coverUp;}

    // Setters
    void setCoords(const Coords &c) {coords = c;    } /**< Sets the coordinates for the annotation */
    void setTile(const MapTile &t)  {tile = t;      } /**< Sets the tile for the annotation */
    void setVisualOnly(bool v)      {visual = v;    } /**< Sets whether or not the annotation is visual-only */
    void setTTL(int turns)          {ttl = turns;   } /**< Sets the number of turns the annotation will live */
    void passTurn()                 {if (ttl > 0) ttl--; } /**< Passes a turn for the annotation */

    bool operator==(const Annotation&) const;

    // Properties
private:
    Coords coords;
    MapTile tile;
    bool visual;
    int ttl;
    bool coverUp;

    friend class Map;
};

/**
 * Manages annotations for the current map.  This includes
 * adding and removing annotations, as well as finding annotations
 * and managing their existence.
 */
class AnnotationMgr {
public:
    AnnotationMgr();

    Annotation       *add(Coords coords, MapTile tile, bool visual = false, bool isCoverUp = false);
    Annotation::List allAt(Coords pos);
    std::list<Annotation *> ptrsToAllAt(Coords pos);
    void             clear();
    void             passTurn();
    void             remove(Coords pos, MapTile tile);
    void             remove(Annotation&);
    void             remove(Annotation::List);
    int              size();

private:
    Annotation::List  annotations;

    friend class Map;
};

#endif
