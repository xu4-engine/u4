/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "savegame.h"

#include "io.h"
#include "object.h"
#include "types.h"

using std::list;

#define MONSTERTABLE_SIZE           32
#define MONSTERTABLE_MONSTERS_SIZE  8
#define MONSTERTABLE_OBJECTS_SIZE   (MONSTERTABLE_SIZE - MONSTERTABLE_MONSTERS_SIZE)

char *partySavFilename() {
    char *fname;
    
#if defined(MACOSX)
    char *home;

    home = getenv("HOME");
    if (home && home[0]) {
        fname = new char[strlen(home) + strlen(MACOSX_USER_FILES_PATH) + strlen(PARTY_SAV_BASE_FILENAME) + 2];
        strcpy(fname, home);
        strcat(fname, MACOSX_USER_FILES_PATH);
        strcat(fname, "/");
        strcat(fname, PARTY_SAV_BASE_FILENAME);
    } else
        fname = strdup(PARTY_SAV_BASE_FILENAME);
#else
    fname = strdup(PARTY_SAV_BASE_FILENAME);
#endif
    
    return fname;
}

char *monstersSavFilename(const char *base) {
    char *fname;
    
#if defined(MACOSX)
    char *home;

    home = getenv("HOME");
    if (home && home[0]) {
        fname = new char[strlen(home) + strlen(MACOSX_USER_FILES_PATH) + strlen(base) + 2];
        strcpy(fname, home);
        strcat(fname, MACOSX_USER_FILES_PATH);
        strcat(fname, "/");
        strcat(fname, base);
    } else
        fname = strdup(base);
#else
    fname = strdup(base);
#endif
    
    return fname;
}
    
FILE *saveGameOpenForWriting() {
    char *fname;
    FILE *f;
    
    fname = partySavFilename();
    f = fopen(fname, "wb");
    delete fname;

    return f;
}

FILE *saveGameOpenForReading() {
    char *fname;
    FILE *f;
    
    fname = partySavFilename();
    f = fopen(fname, "rb");
    delete fname;

    return f;
}

FILE *saveGameMonstersOpenForWriting(const char *filename) {
    char *fname;
    FILE *f;
    
    fname = monstersSavFilename(filename);
    f = fopen(fname, "wb");
    delete fname;

    return f;
}

FILE *saveGameMonstersOpenForReading(const char *filename) {
    char *fname;
    FILE *f;
    
    fname = monstersSavFilename(filename);
    f = fopen(fname, "rb");
    delete fname;

    return f;
}

int saveGameWrite(const SaveGame *save, FILE *f) {
    int i;

    if (!writeInt(save->unknown1, f) ||
        !writeInt(save->moves, f))
        return 0;

    for (i = 0; i < 8; i++) {
        if (!saveGamePlayerRecordWrite(&(save->players[i]), f))
            return 0;
    }

    if (!writeInt(save->food, f) ||
        !writeShort(save->gold, f))
        return 0;

    for (i = 0; i < 8; i++) {
        if (!writeShort(save->karma[i], f))
            return 0;
    }

    if (!writeShort(save->torches, f) ||
        !writeShort(save->gems, f) ||
        !writeShort(save->keys, f) ||
        !writeShort(save->sextants, f))
        return 0;

    for (i = 0; i < ARMR_MAX; i++) {
        if (!writeShort(save->armor[i], f))
            return 0;
    }

    for (i = 0; i < WEAP_MAX; i++) {
        if (!writeShort(save->weapons[i], f))
            return 0;
    }

    for (i = 0; i < REAG_MAX; i++) {
        if (!writeShort(save->reagents[i], f))
            return 0;
    }

    for (i = 0; i < 26; i++) {
        if (!writeShort(save->mixtures[i], f))
            return 0;
    }

    if (!writeShort(save->items, f) ||
        !writeChar(save->x, f) ||
        !writeChar(save->y, f) ||
        !writeChar(save->stones, f) ||
        !writeChar(save->runes, f) ||
        !writeShort(save->members, f) ||
        !writeShort(save->transport, f) ||
        !writeShort(save->balloonstate, f) ||
        !writeShort(save->trammelphase, f) ||
        !writeShort(save->feluccaphase, f) ||
        !writeShort(save->shiphull, f) ||
        !writeShort(save->lbintro, f) ||
        !writeShort(save->lastcamp, f) ||
        !writeShort(save->lastreagent, f) ||
        !writeShort(save->lastmeditation, f) ||
        !writeShort(save->lastvirtue, f) ||
        !writeChar(save->dngx, f) ||
        !writeChar(save->dngy, f) ||
        !writeShort(save->orientation, f) ||
        !writeShort(save->dnglevel, f) ||
        !writeShort(save->location, f))
        return 0;

    return 1;
}

int saveGameRead(SaveGame *save, FILE *f) {
    int i;

    if (!readInt(&(save->unknown1), f) ||
        !readInt(&(save->moves), f))
        return 0;

    for (i = 0; i < 8; i++) {
        if (!saveGamePlayerRecordRead(&(save->players[i]), f))
            return 0;
    }

    if (!readInt((unsigned int*)&(save->food), f) ||
        !readShort((unsigned short*)&(save->gold), f))
        return 0;

    for (i = 0; i < 8; i++) {
        if (!readShort((unsigned short*)&(save->karma[i]), f))
            return 0;
    }

    if (!readShort((unsigned short*)&(save->torches), f) ||
        !readShort((unsigned short*)&(save->gems), f) ||
        !readShort((unsigned short*)&(save->keys), f) ||
        !readShort((unsigned short*)&(save->sextants), f))
        return 0;

    for (i = 0; i < ARMR_MAX; i++) {
        if (!readShort((unsigned short*)&(save->armor[i]), f))
            return 0;
    }

    for (i = 0; i < WEAP_MAX; i++) {
        if (!readShort((unsigned short*)&(save->weapons[i]), f))
            return 0;
    }

    for (i = 0; i < REAG_MAX; i++) {
        if (!readShort((unsigned short*)&(save->reagents[i]), f))
            return 0;
    }

    for (i = 0; i < 26; i++) {
        if (!readShort((unsigned short*)&(save->mixtures[i]), f))
            return 0;
    }

    if (!readShort(&(save->items), f) ||
        !readChar(&(save->x), f) ||
        !readChar(&(save->y), f) ||
        !readChar(&(save->stones), f) ||
        !readChar(&(save->runes), f) ||
        !readShort(&(save->members), f) ||
        !readShort(&(save->transport), f) ||
        !readShort(&(save->balloonstate), f) ||
        !readShort(&(save->trammelphase), f) ||
        !readShort(&(save->feluccaphase), f) ||
        !readShort(&(save->shiphull), f) ||
        !readShort(&(save->lbintro), f) ||
        !readShort(&(save->lastcamp), f) ||
        !readShort(&(save->lastreagent), f) ||
        !readShort(&(save->lastmeditation), f) ||
        !readShort(&(save->lastvirtue), f) ||
        !readChar(&(save->dngx), f) ||
        !readChar(&(save->dngy), f) ||
        !readShort(&(save->orientation), f) ||
        !readShort(&(save->dnglevel), f) ||
        !readShort(&(save->location), f))
        return 0;

    /* workaround of U4DOS bug to retain savegame compatibility */
    if (save->location == 0 && save->dnglevel == 0)
        save->dnglevel = 0xFFFF;

    return 1;
}

void saveGameInit(SaveGame *save, const SaveGamePlayerRecord *avatarInfo) {
    int i;

    save->unknown1 = 0;
    save->moves = 0;

    memcpy(&(save->players[0]), avatarInfo, sizeof(SaveGamePlayerRecord));
    for (i = 1; i < 8; i++)
        saveGamePlayerRecordInit(&(save->players[i]));

    save->food = 0;
    save->gold = 0;

    for (i = 0; i < 8; i++)
        save->karma[i] = 20;

    save->torches = 0;
    save->gems = 0;
    save->keys = 0;
    save->sextants = 0;

    for (i = 0; i < ARMR_MAX; i++)
        save->armor[i] = 0;

    for (i = 0; i < WEAP_MAX; i++)
        save->weapons[i] = 0;

    for (i = 0; i < REAG_MAX; i++)
        save->reagents[i] = 0;

    for (i = 0; i < 26; i++)
        save->mixtures[i] = 0;

    save->items = 0;
    save->x = 0;
    save->y = 0;
    save->stones = 0;
    save->runes = 0;
    save->members = 1;
    save->transport = 0x1f;
    save->balloonstate = 0;
    save->trammelphase = 0;
    save->feluccaphase = 0;
    save->shiphull = 50;
    save->lbintro = 0;
    save->lastcamp = 0;
    save->lastreagent = 0;
    save->lastmeditation = 0;
    save->lastvirtue = 0;
    save->dngx = 0;
    save->dngy = 0;
    save->orientation = 0;
    save->dnglevel = 0xFFFF;
    save->location = 0;
}

int saveGamePlayerRecordWrite(const SaveGamePlayerRecord *record, FILE *f) {
    int i;

    if (!writeShort(record->hp, f) ||
        !writeShort(record->hpMax, f) ||
        !writeShort(record->xp, f) ||
        !writeShort(record->str, f) ||
        !writeShort(record->dex, f) ||
        !writeShort(record->intel, f) ||
        !writeShort(record->mp, f) ||
        !writeShort(record->unknown, f) ||
        !writeShort((unsigned short)record->weapon, f) ||
        !writeShort((unsigned short)record->armor, f))
        return 0;

    for (i = 0; i < 16; i++) {
        if (!writeChar(record->name[i], f))
            return 0;
    }

    if (!writeChar((unsigned char)record->sex, f) ||
        !writeChar((unsigned char)record->klass, f) ||
        !writeChar((unsigned char)record->status, f))
        return 0;

    return 1;
}

int saveGamePlayerRecordRead(SaveGamePlayerRecord *record, FILE *f) {
    int i;
    unsigned char ch;
    unsigned short s;

    if (!readShort(&(record->hp), f) ||
        !readShort(&(record->hpMax), f) ||
        !readShort(&(record->xp), f) ||
        !readShort(&(record->str), f) ||
        !readShort(&(record->dex), f) ||
        !readShort(&(record->intel), f) ||
        !readShort(&(record->mp), f) ||
        !readShort(&(record->unknown), f))
        return 0;
        
    if (!readShort(&s, f))
        return 0;
    record->weapon = (WeaponType) s;
    if (!readShort(&s, f))
        return 0;
    record->armor = (ArmorType) s;

    for (i = 0; i < 16; i++) {
        if (!readChar((unsigned char *) &(record->name[i]), f))
            return 0;
    }

    if (!readChar(&ch, f))
        return 0;
    record->sex = (SexType) ch;
    if (!readChar(&ch, f))
      return 0;
    record->klass = (ClassType) ch;
    if (!readChar(&ch, f))
        return 0;
    record->status = (StatusType) ch;

    return 1;
}

void saveGamePlayerRecordInit(SaveGamePlayerRecord *record) {
    int i;

    record->hp = 0;
    record->hpMax = 0;
    record->xp = 0;
    record->str = 0;
    record->dex = 0;
    record->intel = 0;
    record->mp = 0;
    record->unknown = 0;
    record->weapon = WEAP_HANDS;
    record->armor = ARMR_NONE;

    for (i = 0; i < 16; i++)
      record->name[i] = '\0';

    record->sex = SEX_MALE;
    record->klass = CLASS_MAGE;
    record->status = STAT_GOOD;
}

int saveGameMonstersWrite(list<Object *> &objs, FILE *f) {
    list<Object *>::iterator current;
    const Object *obj;
    const Object *monsterTable[MONSTERTABLE_SIZE];
    list<const Object*> whirlpools_storms;
    list<const Object*> other_monsters;
    list<const Object*> inanimate_objects;    
    
    int nMonsters = 0;
    int nObjects = 0;    
    int i, r;

    memset((void *)monsterTable, 0, MONSTERTABLE_SIZE * sizeof(Object *));

    /**
     * First, categorize all the objects we have
     */ 
    for (current = objs.begin(); current != objs.end(); current++) {
        obj = *current;

        /* moving objects first */
        if ((obj->getType() == OBJECT_MONSTER) && (obj->getMovementBehavior() != MOVEMENT_FIXED)) {
            /* whirlpools and storms are separated from other moving objects */
            if ((obj->getTile() == 140) || (obj->getTile() == 142))
                whirlpools_storms.push_back(obj);
            else other_monsters.push_back(obj);
        }
        else inanimate_objects.push_back(obj);
    }

    /**
     * OK, whirlpools and storms go first so they behave correctly in u4dos
     */     
    while (whirlpools_storms.size() && nMonsters < 4) {        
        monsterTable[nMonsters++] = whirlpools_storms.front();
        whirlpools_storms.pop_front();
    }
    /**
     * Then, fill up the rest of the "moving object" section with monsters
     */
    while (other_monsters.size() && nMonsters < MONSTERTABLE_MONSTERS_SIZE) {
        monsterTable[nMonsters++] = other_monsters.front();
        other_monsters.pop_front();
    }
    /**
     * Finally, add inanimate objects
     */
    while (inanimate_objects.size() && nObjects < MONSTERTABLE_OBJECTS_SIZE) {
        monsterTable[MONSTERTABLE_MONSTERS_SIZE + nObjects++] = inanimate_objects.front();
        inanimate_objects.pop_front();
    }

    /* tile for each object */
    for (i = 0; i < MONSTERTABLE_SIZE; i++) {
        if (monsterTable[i])
            r = writeChar(monsterTable[i]->getTile(), f);
        else
            r = writeChar(0, f);
        if (!r) return 0;
    }
        
    /* x location for each object */
    for (i = 0; i < MONSTERTABLE_SIZE; i++) {
        if (monsterTable[i])
            r = writeChar((unsigned char)monsterTable[i]->getCoords().x, f);
        else
            r = writeChar(0, f);
        if (!r) return 0;
    }
        
    /* y location for each object */
    for (i = 0; i < MONSTERTABLE_SIZE; i++) {
        if (monsterTable[i])
            r = writeChar((unsigned char)monsterTable[i]->getCoords().y, f);
        else
            r = writeChar(0, f);
        if (!r) return 0;
    }
        
    /* previous tile for each object */
    for (i = 0; i < MONSTERTABLE_SIZE; i++) {
        if (monsterTable[i])
            r = writeChar((unsigned char)monsterTable[i]->getPrevTile(), f);
        else
            r = writeChar(0, f);
        if (!r) return 0;
    }
        
    /* previous x location for each object */
    for (i = 0; i < MONSTERTABLE_SIZE; i++) {
        if (monsterTable[i])
            r = writeChar((unsigned char)monsterTable[i]->getPrevCoords().x, f);
        else
            r = writeChar(0, f);
        if (!r) return 0;
    }
        
    /* previous y location for each object */
    for (i = 0; i < MONSTERTABLE_SIZE; i++) {
        if (monsterTable[i])
            r = writeChar((unsigned char)monsterTable[i]->getPrevCoords().y, f);
        else
            r = writeChar(0, f);
        if (!r) return 0;
    }
        
    /* unused space */
    for (i = 0; i < MONSTERTABLE_SIZE; i++) {
        if (!writeChar(0, f))
            return 0;
    }   

    /* unused space, part II */
    for (i = 0; i < MONSTERTABLE_SIZE; i++) {
        if (!writeChar(0, f))
            return 0;
    }

    return 1;
}

int saveGameMonstersRead(list<Object *> *objs, FILE *f) {    
    Object *obj;
    Object monsterTable[MONSTERTABLE_SIZE];
    int i;
    unsigned char ch;
    Coords coords[MONSTERTABLE_SIZE];
    bool isEmpty[MONSTERTABLE_SIZE];    

    for (i = 0; i < MONSTERTABLE_SIZE; i++) {
        if (!readChar(&ch, f))
            return 0;
        monsterTable[i].setPrevTile(ch);
    }

    for (i = 0; i < MONSTERTABLE_SIZE; i++) {
        if (!readChar(&ch, f))
            return 0;
        coords[i].x = ch;
    }

    for (i = 0; i < MONSTERTABLE_SIZE; i++) {
        if (!readChar(&ch, f))
            return 0;
        coords[i].y = ch;
    }

    for (i = 0; i < MONSTERTABLE_SIZE; i++) {
        monsterTable[i].setCoords(coords[i]);
    }

    for (i = 0; i < MONSTERTABLE_SIZE; i++) {
        if (!readChar(&ch, f))
            return 0;
        if (ch > 0)
            monsterTable[i].setTile(ch);
        isEmpty[i] = (ch == 0) ? true : false;
    }

    for (i = 0; i < MONSTERTABLE_SIZE; i++) {
        if (!readChar(&ch, f))
            return 0;
        coords[i].x = ch;
    }

    for (i = 0; i < MONSTERTABLE_SIZE; i++) {
        if (!readChar(&ch, f))
            return 0;
        coords[i].y = ch;
    }

    for (i = 0; i < MONSTERTABLE_SIZE; i++) {
        monsterTable[i].setPrevCoords(coords[i]);
    }

    /* empty out our object list first */
    objs->clear();    
    
    for (i = 0; i < MONSTERTABLE_SIZE; i++) {
        /* check to see if this is a non-empty entry */
        if (!isEmpty[i]) {
            obj = new Object;
            *obj = monsterTable[i];
            
            if (i < MONSTERTABLE_MONSTERS_SIZE)
                obj->setMovementBehavior(MOVEMENT_ATTACK_AVATAR);
            else
                obj->setMovementBehavior(MOVEMENT_FIXED);

            /* add it to the list! */
            objs->push_back(obj);            
        }
    }    
    
    return 1;
}
