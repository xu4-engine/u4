/*
 * $Id$
 */

#ifndef ANNOTATION_H
#define ANNOTATION_H

/*
 * Annotations are temporary or permanent updates to a map.  When one
 * is created, an x,y coordinate and time to live are provided.  The
 * time to live is number of cycles the annotation will exist before
 * being automatically removed.  A time to live of -1 means permanent
 * (the life of the context, or until removed or cleared).
 */

typedef struct _Annotation {
    int x, y;
    int time_to_live;
    unsigned char tile;
    struct _Annotation *next;
} Annotation;

void annotationAdd(int x, int y, int ttl, unsigned char tile);
void annotationRemove(int x, int y, unsigned char tile);
const Annotation *annotationAt(int x, int y);
void annotationCycle(void);
void annotationClear(void);
int annotationCount(void);

#endif
