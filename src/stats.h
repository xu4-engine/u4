/*
 * $Id$
 */

#ifndef STATS_H
#define STATS_H

#define STATS_AREA_WIDTH 15
#define STATS_AREA_HEIGHT 8
#define STATS_AREA_X TEXT_AREA_X
#define STATS_AREA_Y 1

void statsAreaClear();
void statsPrevItem(void);
void statsNextItem(void);
void statsUpdate(void);
void statsHighlightCharacter(int c);
void statsShowPartyView(int member);

#endif
