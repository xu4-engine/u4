/*
 * $Id$
 */

#ifndef CONTEXT_H
#define CONTEXT_H

struct _SaveGame;
struct _Map;
struct _Person;
struct _Annotation;

#define CONV_BUFFERLEN 16

typedef enum {
    STATS_PARTY_OVERVIEW,
    STATS_CHAR1,
    STATS_CHAR2,
    STATS_CHAR3,
    STATS_CHAR4,
    STATS_CHAR5, 
    STATS_CHAR6, 
    STATS_CHAR7,
    STATS_CHAR8,
    STATS_WEAPONS,
    STATS_ARMOR,
    STATS_EQUIPMENT,
    STATS_ITEMS,
    STATS_REAGENTS,
    STATS_MIXTURES
} StatsItem;

typedef enum {
    AURA_NONE,
    AURA_HORN,
    AURA_JINX,
    AURA_NEGATE,
    AURA_PROTECTION,
    AURA_QUICKNESS
} Aura;

typedef struct _Conversation {
    const struct _Person *talker;
    int state;
    char buffer[CONV_BUFFERLEN];
    int itemType;
    int itemSubtype;
    int quant;
    int price;
    int player;
} Conversation;

typedef struct _Context {
    struct _SaveGame *saveGame;
    struct _Context *parent;
    struct _Annotation *annotation;
    struct _Map *map;
    Conversation conversation;
    int line, col;
    StatsItem statsItem;
    int moonPhase;
    int windDirection;
    int windCounter;
    Aura aura;
    int auraDuration;
    int horseSpeed;
    long lastCommandTime;
} Context;

extern Context *c;

#endif
