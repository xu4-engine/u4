/*
 * annotation.cpp
 */

#include "annotation.h"

/**
 * Adds an annotation to the current map
 */
Annotation *AnnotationList::add(const Coords& coords, const MapTile& tile,
                                bool visual, bool isCoverUp) {
    /* new annotations go to the front so they're handled "on top" */
    Annotation ann;
    ann.coords  = coords;
    ann.tile    = tile;
    ann.ttl     = -1;
    ann.visualOnly = visual;
    ann.coverUp = isCoverUp;
    push_front(ann);
    return &front();
}

/**
 * Returns all annotations found at the given map coordinates
 */
AnnotationList AnnotationList::allAt(Coords coords) {
    AnnotationList list;
    AnnotationList::iterator i;

    for (i = begin(); i != end(); i++) {
        if (i->coords == coords)
            list.push_back(*i);
    }

    return list;
}

/**
 * Returns pointers to all annotations found at the given map coordinates
 */
std::list<Annotation *> AnnotationList::ptrsToAllAt(const Coords& coords) {
    std::list<Annotation *> list;
    iterator i;

    for (i = begin(); i != end(); i++) {
        if (i->coords == coords)
            list.push_back(&(*i));
    }

    return list;
}

/**
 * Passes a turn for annotations and removes any
 * annotations whose TTL has expired
 */
void AnnotationList::passTurn() {
    iterator i = begin();
    while (i != end()) {
        if (i->ttl == 0) {
            i = erase(i);
        } else {
            if (i->ttl > 0)
                --i->ttl;       // Passes a turn for the annotation.
            ++i;
        }
    }
}

/**
 * Removes an annotation from the current map
 */
void AnnotationList::remove(const Coords& coords, const MapTile& tile) {
    iterator i;
    for (i = begin(); i != end(); i++) {
        if (i->coords == coords && i->tile == tile) {
            erase(i);
            break;
        }
    }
}

/**
 * Removes all annotations at a specific position.
 */
void AnnotationList::removeAllAt(const Coords& pos) {
    iterator it = begin();
    while (it != end()) {
        if (it->coords == pos)
            it = erase(it);
        else
            ++it;
    }
}
