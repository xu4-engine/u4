/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include "savegame.h"

#include "io.h"
#include "object.h"
#include "types.h"

#define MONSTERTABLE_SIZE           32
#define MONSTERTABLE_CREATURES_SIZE  8
#define MONSTERTABLE_OBJECTS_SIZE   (MONSTERTABLE_SIZE - MONSTERTABLE_CREATURES_SIZE)

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

int SaveGame::write(FILE *f) const {
    int i;

    if (!writeInt(unknown1, f) ||
        !writeInt(moves, f))
        return 0;

    for (i = 0; i < 8; i++) {
        if (!players[i].write(f))
            return 0;
    }

    if (!writeInt(food, f) ||
        !writeShort(gold, f))
        return 0;

    for (i = 0; i < 8; i++) {
        if (!writeShort(karma[i], f))
            return 0;
    }

    if (!writeShort(torches, f) ||
        !writeShort(gems, f) ||
        !writeShort(keys, f) ||
        !writeShort(sextants, f))
        return 0;

    for (i = 0; i < ARMR_MAX; i++) {
        if (!writeShort(armor[i], f))
            return 0;
    }

    for (i = 0; i < WEAP_MAX; i++) {
        if (!writeShort(weapons[i], f))
            return 0;
    }

    for (i = 0; i < REAG_MAX; i++) {
        if (!writeShort(reagents[i], f))
            return 0;
    }

    for (i = 0; i < SPELL_MAX; i++) {
        if (!writeShort(mixtures[i], f))
            return 0;
    }

    if (!writeShort(items, f) ||
        !writeChar(x, f) ||
        !writeChar(y, f) ||
        !writeChar(stones, f) ||
        !writeChar(runes, f) ||
        !writeShort(members, f) ||
        !writeShort(transport, f) ||
        !writeShort(balloonstate, f) ||
        !writeShort(trammelphase, f) ||
        !writeShort(feluccaphase, f) ||
        !writeShort(shiphull, f) ||
        !writeShort(lbintro, f) ||
        !writeShort(lastcamp, f) ||
        !writeShort(lastreagent, f) ||
        !writeShort(lastmeditation, f) ||
        !writeShort(lastvirtue, f) ||
        !writeChar(dngx, f) ||
        !writeChar(dngy, f) ||
        !writeShort(orientation, f) ||
        !writeShort(dnglevel, f) ||
        !writeShort(location, f))
        return 0;

    return 1;
}

int SaveGame::read(FILE *f) {
    int i;

    if (!readInt(&unknown1, f) ||
        !readInt(&moves, f))
        return 0;

    for (i = 0; i < 8; i++) {
        if (!players[i].read(f))
            return 0;
    }

    if (!readInt((unsigned int*)&food, f) ||
        !readShort((unsigned short*)&gold, f))
        return 0;

    for (i = 0; i < 8; i++) {
        if (!readShort((unsigned short*)&(karma[i]), f))
            return 0;
    }

    if (!readShort((unsigned short*)&torches, f) ||
        !readShort((unsigned short*)&gems, f) ||
        !readShort((unsigned short*)&keys, f) ||
        !readShort((unsigned short*)&sextants, f))
        return 0;

    for (i = 0; i < ARMR_MAX; i++) {
        if (!readShort((unsigned short*)&(armor[i]), f))
            return 0;
    }

    for (i = 0; i < WEAP_MAX; i++) {
        if (!readShort((unsigned short*)&(weapons[i]), f))
            return 0;
    }

    for (i = 0; i < REAG_MAX; i++) {
        if (!readShort((unsigned short*)&(reagents[i]), f))
            return 0;
    }

    for (i = 0; i < SPELL_MAX; i++) {
        if (!readShort((unsigned short*)&(mixtures[i]), f))
            return 0;
    }

    if (!readShort(&items, f) ||
        !readChar(&x, f) ||
        !readChar(&y, f) ||
        !readChar(&stones, f) ||
        !readChar(&runes, f) ||
        !readShort(&members, f) ||
        !readShort(&transport, f) ||
        !readShort(&balloonstate, f) ||
        !readShort(&trammelphase, f) ||
        !readShort(&feluccaphase, f) ||
        !readShort(&shiphull, f) ||
        !readShort(&lbintro, f) ||
        !readShort(&lastcamp, f) ||
        !readShort(&lastreagent, f) ||
        !readShort(&lastmeditation, f) ||
        !readShort(&lastvirtue, f) ||
        !readChar(&dngx, f) ||
        !readChar(&dngy, f) ||
        !readShort(&orientation, f) ||
        !readShort(&dnglevel, f) ||
        !readShort(&location, f))
        return 0;

    /* workaround of U4DOS bug to retain savegame compatibility */
    if (location == 0 && dnglevel == 0)
        dnglevel = 0xFFFF;

    return 1;
}

void SaveGame::init(const SaveGamePlayerRecord *avatarInfo) {
    int i;

    unknown1 = 0;
    moves = 0;

    memcpy(&(players[0]), avatarInfo, sizeof(SaveGamePlayerRecord));
    for (i = 1; i < 8; i++)
        players[i].init();

    food = 0;
    gold = 0;

    for (i = 0; i < 8; i++)
        karma[i] = 20;

    torches = 0;
    gems = 0;
    keys = 0;
    sextants = 0;

    for (i = 0; i < ARMR_MAX; i++)
        armor[i] = 0;

    for (i = 0; i < WEAP_MAX; i++)
        weapons[i] = 0;

    for (i = 0; i < REAG_MAX; i++)
        reagents[i] = 0;

    for (i = 0; i < SPELL_MAX; i++)
        mixtures[i] = 0;

    items = 0;
    x = 0;
    y = 0;
    stones = 0;
    runes = 0;
    members = 1;
    transport = 0x1f;
    balloonstate = 0;
    trammelphase = 0;
    feluccaphase = 0;
    shiphull = 50;
    lbintro = 0;
    lastcamp = 0;
    lastreagent = 0;
    lastmeditation = 0;
    lastvirtue = 0;
    dngx = 0;
    dngy = 0;
    orientation = 0;
    dnglevel = 0xFFFF;
    location = 0;
}

int SaveGamePlayerRecord::write(FILE *f) const {
    int i;

    if (!writeShort(hp, f) ||
        !writeShort(hpMax, f) ||
        !writeShort(xp, f) ||
        !writeShort(str, f) ||
        !writeShort(dex, f) ||
        !writeShort(intel, f) ||
        !writeShort(mp, f) ||
        !writeShort(unknown, f) ||
        !writeShort((unsigned short)weapon, f) ||
        !writeShort((unsigned short)armor, f))
        return 0;

    for (i = 0; i < 16; i++) {
        if (!writeChar(name[i], f))
            return 0;
    }

    if (!writeChar((unsigned char)sex, f) ||
        !writeChar((unsigned char)klass, f) ||
        !writeChar((unsigned char)status, f))
        return 0;

    return 1;
}

int SaveGamePlayerRecord::read(FILE *f) {
    int i;
    unsigned char ch;
    unsigned short s;

    if (!readShort(&hp, f) ||
        !readShort(&hpMax, f) ||
        !readShort(&xp, f) ||
        !readShort(&str, f) ||
        !readShort(&dex, f) ||
        !readShort(&intel, f) ||
        !readShort(&mp, f) ||
        !readShort(&unknown, f))
        return 0;
        
    if (!readShort(&s, f))
        return 0;
    weapon = (WeaponType) s;
    if (!readShort(&s, f))
        return 0;
    armor = (ArmorType) s;

    for (i = 0; i < 16; i++) {
        if (!readChar((unsigned char *) &(name[i]), f))
            return 0;
    }

    if (!readChar(&ch, f))
        return 0;
    sex = (SexType) ch;
    if (!readChar(&ch, f))
      return 0;
    klass = (ClassType) ch;
    if (!readChar(&ch, f))
        return 0;
    status = (StatusType) ch;

    return 1;
}

void SaveGamePlayerRecord::init() {
    int i;

    hp = 0;
    hpMax = 0;
    xp = 0;
    str = 0;
    dex = 0;
    intel = 0;
    mp = 0;
    unknown = 0;
    weapon = WEAP_HANDS;
    armor = ARMR_NONE;

    for (i = 0; i < 16; i++)
      name[i] = '\0';

    sex = SEX_MALE;
    klass = CLASS_MAGE;
    status = STAT_GOOD;
}

int saveGameMonstersWrite(std::deque<Object *> &objs, FILE *f) {
    std::deque<Object *>::iterator current;
    const Object *obj;
    const Object *monsterTable[MONSTERTABLE_SIZE];
    std::deque<const Object*> whirlpools_storms;
    std::deque<const Object*> other_creatures;
    std::deque<const Object*> inanimate_objects;    
    
    int nCreatures = 0;
    int nObjects = 0;    
    int i, r;

    memset((void *)monsterTable, 0, MONSTERTABLE_SIZE * sizeof(Object *));

    /**
     * First, categorize all the objects we have
     */ 
    for (current = objs.begin(); current != objs.end(); current++) {
        obj = *current;

        /* moving objects first */
        if ((obj->getType() == OBJECT_CREATURE) && (obj->getMovementBehavior() != MOVEMENT_FIXED)) {
            int tile = obj->getTile(). getIndex();
            /* whirlpools and storms are separated from other moving objects */
            if ((tile == 140) || (tile == 142))
                whirlpools_storms.push_back(obj);
            else other_creatures.push_back(obj);
        }
        else inanimate_objects.push_back(obj);
    }

    /**
     * OK, whirlpools and storms go first so they behave correctly in u4dos
     */     
    while (whirlpools_storms.size() && nCreatures < 4) {        
        monsterTable[nCreatures++] = whirlpools_storms.front();
        whirlpools_storms.pop_front();
    }
    /**
     * Then, fill up the rest of the "moving object" section with creatures
     */
    while (other_creatures.size() && nCreatures < MONSTERTABLE_CREATURES_SIZE) {
        monsterTable[nCreatures++] = other_creatures.front();
        other_creatures.pop_front();
    }
    /**
     * Finally, add inanimate objects
     */
    while (inanimate_objects.size() && nObjects < MONSTERTABLE_OBJECTS_SIZE) {
        monsterTable[MONSTERTABLE_CREATURES_SIZE + nObjects++] = inanimate_objects.front();
        inanimate_objects.pop_front();
    }

    /* tile for each object */
    for (i = 0; i < MONSTERTABLE_SIZE; i++) {
        if (monsterTable[i])
            r = writeChar(monsterTable[i]->getTile().getIndex(), f);
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
            r = writeChar((unsigned char)monsterTable[i]->getPrevTile().getIndex(), f);
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

int saveGameMonstersRead(std::deque<Object *> *objs, FILE *f) {    
    Object *obj;
    Object monsterTable[MONSTERTABLE_SIZE];
    int i;
    unsigned char ch;
    Coords coords[MONSTERTABLE_SIZE];
    bool isEmpty[MONSTERTABLE_SIZE];    

    for (i = 0; i < MONSTERTABLE_SIZE; i++) {
        if (!readChar(&ch, f))
            return 0;
        monsterTable[i].setPrevTile(Tile::getMapTile(ch));
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
            monsterTable[i].setTile(Tile::getMapTile(ch));
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
            
            if (i < MONSTERTABLE_CREATURES_SIZE)
                obj->setMovementBehavior(MOVEMENT_ATTACK_AVATAR);
            else
                obj->setMovementBehavior(MOVEMENT_FIXED);

            /* add it to the list! */
            objs->push_back(obj);            
        }
    }    
    
    return 1;
}
