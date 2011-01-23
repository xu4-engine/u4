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

    void             debug_output() const;
    const Coords&    getCoords() const;
    MapTile&         getTile();
    const bool       isVisualOnly() const;
    const int        getTTL() const;
    void             setCoords(const Coords &);
    void             setTile(const MapTile &);
    void             setVisualOnly(bool visual = true);
    void             setTTL(int turns);
    void             passTurn();
    bool			 isCoverUp();

    bool operator==(const Annotation&) const;    

    // Properties
private:        
    Coords coords;
    MapTile tile;        
    bool visual;
    int ttl;
    bool coverUp;

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
    Annotation::List::iterator i;
};

#endif
