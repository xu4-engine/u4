/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise
 
#include "annotation.h"

#include "context.h"
#include "debug.h"
#include "error.h"
#include "event.h"
#include "map.h"
#include "settings.h"

/**
 * Annotation class implementation
 */ 
/**
 * Constructors
 */ 
Annotation::Annotation(const Coords &c, MapTile t, bool v) : 
    coords(c), 
    tile(t),
    visual(v),
    ttl(-1) {}

/**
 * Members
 */ 
void Annotation::debug_output() const {        
    printf("x: %d\n", coords.x);
    printf("y: %d\n", coords.y);
    printf("z: %d\n", coords.z);
    printf("tile: %d\n", tile.id);
    printf("visual: %s\n", visual ? "Yes" : "No");
}

const Coords& Annotation::getCoords() const         { return coords;      } /**< Returns the coordinates of the annotation */
MapTile& Annotation::getTile()                      { return tile;        } /**< Returns the annotation's tile */
const bool Annotation::isVisualOnly() const         { return visual;      } /**< Returns true for visual-only annotations */
const int Annotation::getTTL() const                { return ttl;         } /**< Returns the number of turns the annotation has left to live */
void Annotation::setCoords(const Coords &c)         { coords = c;         } /**< Sets the coordinates for the annotation */
void Annotation::setTile(const MapTile &t)          { tile = t;           } /**< Sets the tile for the annotation */
void Annotation::setVisualOnly(bool v)              { visual = v;         } /**< Sets whether or not the annotation is visual-only */
void Annotation::setTTL(int turns)                  { ttl = turns;        } /**< Sets the number of turns the annotation will live */
void Annotation::passTurn()                         { if (ttl > 0) ttl--; } /**< Passes a turn for the annotation */

/**
 * Operators
 */ 
bool Annotation::operator==(const Annotation &a) const {
    return ((coords == a.getCoords()) && (tile == a.tile)) ? true : false;        
}

/**
 * AnnotationMgr implementation
 */
/**
 * Constructors
 */ 
AnnotationMgr::AnnotationMgr() {}

/**
 * Members
 */ 

/**
 * Adds an annotation to the current map
 */
Annotation *AnnotationMgr::add(Coords coords, MapTile tile, bool visual) {
    /* new annotations go to the front so they're handled "on top" */
    annotations.push_front(Annotation(coords, tile, visual));
    return &annotations.front();
}        

/**
 * Returns all annotations found at the given map coordinates
 */ 
Annotation::List AnnotationMgr::allAt(Coords coords) {
    Annotation::List list;

    for (i = annotations.begin(); i != annotations.end(); i++) {
        if (i->getCoords() == coords)
            list.push_back(*i);
    }
    
    return list;
}

/**
 * Removes all annotations on the map 
 */ 
void AnnotationMgr::clear() {
    annotations.clear();        
}    

/**
 * Passes a turn for annotations and removes any
 * annotations whose TTL has expired
 */ 
void AnnotationMgr::passTurn() {
    for (i = annotations.begin(); i != annotations.end(); i++) {
        if (i->getTTL() == 0) {
            i = annotations.erase(i);
            if (i == annotations.end())
                break;
        }
        else if (i->getTTL() > 0)
            i->passTurn();
    }
}

/**
 * Removes an annotation from the current map
 */
void AnnotationMgr::remove(Coords coords, MapTile tile) {        
    Annotation look_for(coords, tile);
    remove(&look_for);
}

void AnnotationMgr::remove(Annotation *a) {
    for (i = annotations.begin(); i != annotations.end(); i++) {
        if (*i == *a) {
            i = annotations.erase(i);
            break;
        }
    }
}

/**
 * Removes an entire list of annotations 
 */ 
void AnnotationMgr::remove(Annotation::List l) {
    Annotation::List::iterator i;
    for (i = l.begin(); i != l.end(); i++) {
        remove(&*i);
    }
}

/**
 * Returns the number of annotations on the map
 */ 
int AnnotationMgr::size() {
    return annotations.size();
}
