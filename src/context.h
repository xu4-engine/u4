/*
 * $Id$
 */

#ifndef CONTEXT_H
#define CONTEXT_H

#include "location.h"

#ifdef __cplusplus
extern "C" {
#endif

struct _SaveGame;
struct _Location;
struct _Reply;
struct _Person;
struct _Annotation;
struct _Object;

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

typedef enum {
    TRANSPORT_FOOT      = 0x1,
    TRANSPORT_HORSE     = 0x2,
    TRANSPORT_SHIP      = 0x4,
    TRANSPORT_BALLOON   = 0x8
} TransportContext;

#define TRANSPORT_ANY               (0xFFFF)
#define TRANSPORT_FOOT_OR_HORSE     (TRANSPORT_FOOT | TRANSPORT_HORSE)

typedef struct _Conversation {
    const struct _Person *talker;
    int state;
    char playerInquiryBuffer[CONV_BUFFERLEN];
    struct _Reply *reply;
    int replyLine;
    /* for vendor conversations */
    int itemType;
    int itemSubtype;
    int quant;
    int price;
    int player;
} Conversation;

typedef struct _Context {
    struct _SaveGame *saveGame;
    struct _Annotation *annotation;    
    struct _Location *location;
    Conversation conversation;
    int line, col;
    StatsItem statsItem;
    int moonPhase;
    int windDirection;
    int windCounter;
    Aura aura;
    int auraDuration;
    int horseSpeed;
    int opacity;
    TransportContext transportContext;
    long lastCommandTime;
    struct _Object *lastShip;
} Context;

extern Context *c;

#ifdef __cplusplus
}
#endif

#endif
