/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "context.h"
#include "map.h"
#include "annotation.h"

void annotationAdd(int x, int y, int ttl, unsigned char tile) {
    Annotation *annotation = malloc(sizeof(Annotation));
    annotation->x = x;
    annotation->y = y;
    annotation->time_to_live = ttl;
    annotation->tile = tile;
    annotation->next = c->map->annotation;
    
    c->map->annotation = annotation;
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
