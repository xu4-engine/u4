/*
 * $Id$
 */

#ifndef TTYPE_H
#define TTYPE_H

typedef enum {
    ANIM_NONE,
    ANIM_SCROLL,
    ANIM_CAMPFIRE,
    ANIM_CITYFLAG,
    ANIM_CASTLEFLAG,
    ANIM_WESTSHIPFLAG,
    ANIM_EASTSHIPFLAG,
    ANIM_LCBFLAG
} AnimationStyle;

int iswalkable(unsigned char tile);
int isslow(unsigned char tile);
int isvslow(unsigned char tile);
int isdoor(unsigned char tile);
int islockeddoor(unsigned char tile);
int tileCanTalkOver(unsigned char tile);
AnimationStyle tileGetAnimationStyle(unsigned char tile);

#endif
