/*
 * $Id$
 */

#ifndef ANNOTATION_H
#define ANNOTATION_H

/*
 * Annotations are updates to a map.  There are three types:
 * - permanent: lasts until annotationClear is called
 * - turn based: lasts a given number of cycles
 * - time based: lasts a given number of time units (1/4 seconds)
 * When one is created, x and y coordinates are provided.  The
 * appropriate duration can then be added.
 */

typedef struct _Annotation {
    int x, y;
    int time_to_live;
    unsigned char tile;
    struct _Annotation *next;
} Annotation;

Annotation *annotationAdd(int x, int y, unsigned char tile);
void annotationSetTurnDuration(Annotation *a, int ttl);
void annotationSetTimeDuration(Annotation *a, int interval);
void annotationRemove(int x, int y, unsigned char tile);
const Annotation *annotationAt(int x, int y);
void annotationCycle(void);
void annotationClear(void);
int annotationCount(void);

#endif
