/*
 * $Id$
 */

#ifndef ANNOTATION_H
#define ANNOTATION_H

/*
 * Annotations are temporary or permenent updates to a map.  When one
 * is created, an x,y coordinate and time to live are provided.  A
 * time to live of -1 means permenent (the life of the context).
 */

typedef struct _Annotation {
    int x, y;
    int time_to_live;
    unsigned char tile;
    struct _Annotation *next;
} Annotation;

void annotationAdd(int x, int y, int ttl, unsigned char tile);
const Annotation *annotationAt(int x, int y);
void annotationCycle();

#endif
