/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
