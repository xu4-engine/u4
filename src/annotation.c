/*
 * $Id$
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "context.h"
#include "map.h"
#include "annotation.h"
#include "event.h"
#include "error.h"

void annotationTimer(void *data);

Annotation *annotationAdd(int x, int y, unsigned char tile) {
    Annotation *a = (Annotation *) malloc(sizeof(Annotation));
    a->x = x;
    a->y = y;
    a->time_to_live = -1;
    a->tile = tile;
    a->next = c->map->annotation;
    
    c->map->annotation = a;

    return a;
}

void annotationSetTurnDuration(Annotation *a, int ttl) {
    a->time_to_live = ttl;
}

void annotationSetTimeDuration(Annotation *a, int interval) {
    eventHandlerAddTimerCallbackData(&annotationTimer, (void *) a, interval);
}

void annotationTimer(void *data) {
    Annotation *annotation = c->map->annotation, **prev;

    eventHandlerRemoveTimerCallbackData(&annotationTimer, data);

    prev = &(c->map->annotation);
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

void annotationRemove(int x, int y, unsigned char tile) {
    int found = 0, count;
    Annotation *annotation = c->map->annotation, **prev;

    count = annotationCount();

    prev = &(c->map->annotation);
    while (annotation) {
        if (annotation->x == x &&
            annotation->y == y &&
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
        assert(annotationCount() == (count - 1));
    }
}

const Annotation *annotationAt(int x, int y) {
    Annotation *annotation = c->map->annotation;

    while (annotation) {
        if (annotation->x == x &&
            annotation->y == y)
            return annotation;

        annotation = annotation->next;
    }

    return NULL;
}

void annotationCycle(void) {
    Annotation *annotation = c->map->annotation, **prev;
    
    prev = &(c->map->annotation);
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

void annotationClear(void) {
    Annotation *annotation = c->map->annotation, *tmp;
    
    while (annotation) {
        eventHandlerRemoveTimerCallbackData(&annotationTimer, (void *)annotation);
        tmp = annotation->next;
        free(annotation);
        annotation = tmp;
    }
    c->map->annotation = NULL;
}

int annotationCount(void) {
    Annotation *annotation = c->map->annotation;
    int count = 0;
    
    while (annotation) {
        count++;
        annotation = annotation->next;
    }

    return count;
}
