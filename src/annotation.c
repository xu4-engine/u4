/*
 * $Id$
 */

#include <stdlib.h>
#include <string.h>

#include "annotation.h"

#include "context.h"
#include "debug.h"
#include "error.h"
#include "event.h"
#include "map.h"

void annotationTimer(void *data);

/**
 * Adds an annotation to the map with the given mapid
 */
Annotation *annotationAdd(int x, int y, int z, unsigned char mapid, unsigned char tile) {
    Annotation *a = (Annotation *) malloc(sizeof(Annotation));
    a->x = x;
    a->y = y;
    a->z = z;
    a->mapid = mapid;
    a->time_to_live = -1;
    a->tile = tile;
    a->visual = 0;    
    a->next = c->annotation;
    
    c->annotation = a;

    return a;
}

/**
 * Sets the annotation as visual-only.  Visual-only annotations (attack flashes, etc.)
 * are drawn on top of most everything else, whereas normal annotations are drawn
 * under almost everything.
 */
Annotation *annotationSetVisual(Annotation *a) {
    a->visual = 1;
    return a;
}

/**
 * Sets the annotation to expire after the given number of turns
 */
Annotation *annotationSetTurnDuration(Annotation *a, int ttl) {
    a->time_to_live = ttl;    
    return a;
}

/**
 * Sets the annotation to expire after 'interval' game cycles have passed
 */
Annotation *annotationSetTimeDuration(Annotation *a, int interval) {    
    eventHandlerAddTimerCallbackData(&annotationTimer, (void *) a, interval * eventTimerGranularity);
    return a;
}

/**
 * Timer to remove timed annotations after their life-cycle has ended
 */
void annotationTimer(void *data) {
    Annotation *annotation = c->annotation, **prev;

    eventHandlerRemoveTimerCallbackData(&annotationTimer, data);

    prev = &(c->annotation);
    while (annotation) {
        if (annotation == (Annotation *) data) {
            *prev = annotation->next;
            free(annotation);
            return;
        }
        prev = &(annotation->next);
        annotation = annotation->next;
    }
    errorWarning("couldn't remove annotation %d", (int) data);
}

/**
 * Removes an annotation from the map designated by mapid, if an
 * annotation exists that matches 'tile'
 */
void annotationRemove(int x, int y, int z, unsigned char mapid, unsigned char tile) {
    int found = 0, count;
    Annotation *annotation = c->annotation, **prev;

    count = annotationCount();

    prev = &(c->annotation);
    while (annotation) {
        if (annotation->x == x &&
            annotation->y == y &&
            annotation->z == z &&
            annotation->mapid == mapid &&
            annotation->tile == tile) {
            found = 1;
            break;
        }
        prev = &(annotation->next);
        annotation = annotation->next;
    }

    if (found) {
        *prev = annotation->next;
        free(annotation);
        ASSERT(annotationCount() == (count - 1), "annotation table corrupted on remove");
    }
}

/**
 * Returns the annotation at the (x,y,z) coords.  If no
 * annotation is found at those coordinates, returns NULL.
 */
const Annotation *annotationAt(int x, int y, int z, unsigned char mapid) {
    Annotation *annotation = c->annotation;

    while (annotation) {
        if (annotation->x == x &&
            annotation->y == y &&
            annotation->z == z &&
            annotation->mapid == mapid)
            return annotation;

        annotation = annotation->next;
    }

    return NULL;
}

/**
 * Removes annotations that were set with annotationSetTurnDuration()
 * after their life cycle has ended.
 */
void annotationCycle(void) {
    Annotation *annotation = c->annotation, **prev;
    
    prev = &(c->annotation);
    while (annotation) {
        if (annotation->time_to_live == 0) {
            *prev = annotation->next;
            free(annotation);
            annotation = *prev;
            continue;
        } 
        else if (annotation->time_to_live != -1)
            annotation->time_to_live--;

        prev = &(annotation->next);
        annotation = annotation->next;
    }
}

/**
 * Removes all annotations on the map designated by mapid
 */
void annotationClear(unsigned char mapid) {
    Annotation *annotation = c->annotation, **prev;
    
    prev = &(c->annotation);
    while (annotation) {
        if (annotation->mapid == mapid) {
            eventHandlerRemoveTimerCallbackData(&annotationTimer, (void *)annotation);
            *prev = annotation->next;
            free(annotation);
            annotation = *prev;
        } else {
            prev = &(annotation->next);
            annotation = annotation->next;
        }
    }
}

/**
 * Returns the number of annotations currently in memory.
 * This applies to all maps that currently have annotations,
 * not just the current map.
 */
int annotationCount(void) {
    Annotation *annotation = c->annotation;
    int count = 0;
    
    while (annotation) {
        count++;
        annotation = annotation->next;
    }

    return count;
}
