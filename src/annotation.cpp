/*
 * $Id$
 */
 
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
Annotation::Annotation() {};
Annotation::Annotation(MapCoords coords, MapTile tile, bool visual) {
    this->coords = coords;        
    this->tile = tile;
    this->visual = visual;
    this->ttl = -1;
}

/**
 * Members
 */ 
void Annotation::debug_output() const {        
    printf("x: %d\n", coords.x);
    printf("y: %d\n", coords.y);
    printf("z: %d\n", coords.z);
    printf("tile: %d\n", tile);
    printf("visual: %s\n", YesNo(visual));        
}

const MapCoords& Annotation::getCoords() const      { return coords;      }
const MapTile& Annotation::getTile() const          { return tile;        }
const bool Annotation::isVisualOnly() const         { return visual;      }
const int Annotation::getTTL() const                { return ttl;         }
void Annotation::setCoords(const MapCoords &c)      { coords = c;         }
void Annotation::setTile(const MapTile &t)          { tile = t;           }
void Annotation::setVisualOnly(bool v)              { visual = v;         }
void Annotation::setTTL(int turns)                  { ttl = turns;        }
void Annotation::passTurn()                         { if (ttl > 0) ttl--; }

/**
 * Operators
 */ 
bool Annotation::operator==(const Annotation &a) const {
    return ((coords == a.getCoords()) && (tile == a.getTile())) ? true : false;        
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
 * add()
 *
 * Adds an annotation to the current map
 */
Annotation *AnnotationMgr::add(MapCoords coords, MapTile tile, bool visual) {
    /* new annotations go to the front so they're handled "on top" */
    annotations.push_front(Annotation(coords, tile, visual));
    return &annotations.front();
}        

/**
 * allAt()
 *
 * Returns all annotations found at the given map coordinates
 */ 
AnnotationList AnnotationMgr::allAt(MapCoords coords) {
    AnnotationList list;        

    for (i = annotations.begin(); i != annotations.end(); i++) {
        if (i->getCoords() == coords)
            list.push_back(*i);
    }
    
    return list;
}

/**
 * clear()
 *
 * Removes all annotations on the map 
 */ 
void AnnotationMgr::clear() {
    annotations.clear();        
}    

/**
 * passTurn()
 *
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
 * remove()
 *
 * Removes an annotation from the current map
 */
void AnnotationMgr::remove(MapCoords coords, MapTile tile) {        
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
 * Remove an entire list of annotations
 */ 
void AnnotationMgr::remove(AnnotationList l) {
    AnnotationList::iterator i;
    for (i = l.begin(); i != l.end(); i++) {
        remove(&*i);
    }
}

/**
 * size()
 *
 * Returns the number of annotations on the map
 */ 
int AnnotationMgr::size() {
    return annotations.size();
}
