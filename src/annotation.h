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
    int x, y, z;
    unsigned char mapid;
    int time_to_live;
    unsigned char tile;
    int visual;
    struct _Annotation *next;
} Annotation;

Annotation *annotationAdd(int x, int y, int z, unsigned char mapid, unsigned char tile);
Annotation *annotationSetVisual(Annotation *a);
Annotation *annotationSetTurnDuration(Annotation *a, int ttl);
Annotation *annotationSetTimeDuration(Annotation *a, int interval);
void annotationRemove(int x, int y, int z, unsigned char mapid, unsigned char tile);
const Annotation *annotationAt(int x, int y, int z, unsigned char mapid);
void annotationCycle(void);
void annotationClear(unsigned char mapid);
int annotationCount(void);

#endif
