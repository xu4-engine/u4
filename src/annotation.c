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

Annotation *annotationAdd(int x, int y, int z, unsigned char mapid, unsigned char tile) {
    Annotation *a = (Annotation *) malloc(sizeof(Annotation));
    a->x = x;
    a->y = y;
    a->z = z;
    a->mapid = mapid;
    a->time_to_live = -1;
    a->tile = tile;
    a->visual = 0;
    a->permanent = 1;
    a->next = c->annotation;
    
    c->annotation = a;

    return a;
}

/**
 * Functions the same as annotationAdd() but marks the annotation as temporary
 */

Annotation *annotationAddTemporary(int x, int y, int z, unsigned char mapid, unsigned char tile) {
    Annotation *a = annotationAdd(x, y, z, mapid, tile);
    a->permanent = 0;
    return a;
}

Annotation *annotationSetVisual(Annotation *a) {
    a->visual = 1;
    return a;
}

Annotation *annotationSetTurnDuration(Annotation *a, int ttl) {
    a->time_to_live = ttl;    
    return a;
}

Annotation *annotationSetTimeDuration(Annotation *a, int interval) {
    a->permanent = 0;
    eventHandlerAddTimerCallbackData(&annotationTimer, (void *) a, interval * eventTimerGranularity);
    return a;
}

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

int annotationCount(void) {
    Annotation *annotation = c->annotation;
    int count = 0;
    
    while (annotation) {
        count++;
        annotation = annotation->next;
    }

    return count;
}
