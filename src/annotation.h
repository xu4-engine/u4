/*
 * annotation.h
 */

#ifndef ANNOTATION_H
#define ANNOTATION_H

#include <list>

#include "coords.h"
#include "types.h"

/**
 * Annotation are updates to a map.
 * There are three types of annotations:
 * - permanent: lasts until annotationClear is called
 * - turn based: lasts a given number of cycles
 * - time based: lasts a given number of time units (1/4 seconds)
 */
struct Annotation {
    Coords coords;
    MapTile tile;
    int16_t ttl;        /**< The number of turns the annotation will live */
    bool visualOnly;    /**< True if the annotation is visual-only */
    bool coverUp;       /**< True if this hides everything underneath */
};

/**
 * Manages annotations for the current map.  This includes
 * adding and removing annotations, as well as finding annotations
 * and managing their existence.
 */
class AnnotationList : public std::list<Annotation> {
public:
    Annotation* add(const Coords& coords, const MapTile& tile,
                    bool visual = false, bool isCoverUp = false);
    AnnotationList allAt(Coords pos);
    std::list<Annotation *> ptrsToAllAt(const Coords& pos);
    void passTurn();
    void remove(const Coords& pos, const MapTile& tile);
    void remove(const Annotation& a) { remove(a.coords, a.tile); }
    void removeAllAt(const Coords& pos);
};

#endif
