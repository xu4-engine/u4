/*
 * $Id$
 */

#include <stdio.h>
#include <string.h>

#include "savegame.h"

static int writeInt(unsigned int i, FILE *f) {
    if (fputc(i & 0xff, f) == EOF ||
        fputc((i >> 8) & 0xff, f) == EOF ||
        fputc((i >> 16) & 0xff, f) == EOF ||
        fputc((i >> 24) & 0xff, f) == EOF)
        return 0;
    return 1;
}

static int writeShort(unsigned short s, FILE *f) {
    if (fputc(s & 0xff, f) == EOF ||
        fputc((s >> 8) & 0xff, f) == EOF)
        return 0;
    return 1;
}

static int writeChar(unsigned char c, FILE *f) {
    if (fputc(c, f) == EOF)
        return 0;
    return 1;
}

static int readInt(unsigned int *i, FILE *f) {
    *i = fgetc(f);
    *i |= (fgetc(f) << 8);
    *i |= (fgetc(f) << 16);
    *i |= (fgetc(f) << 24);
    
    return 1;
}

static int readShort(unsigned short *s, FILE *f) {
    *s = fgetc(f);
    *s |= (fgetc(f) << 8);

    return 1;
}

static int readChar(unsigned char *c, FILE *f) {
    *c = fgetc(f);

    return 1;
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

    if (!writeShort(save->unknown2, f) ||
        !writeChar(save->x, f) ||
        !writeChar(save->y, f))
        return 0;

    for (i = 0; i < 32; i++) {
        if (!writeChar(save->unknown3[i], f))
            return 0;
    }

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

    if (!readInt(&(save->food), f) ||
        !readShort(&(save->gold), f))
        return 0;

    for (i = 0; i < 8; i++) {
        if (!readShort(&(save->karma[i]), f))
            return 0;
    }

    if (!readShort(&(save->torches), f) ||
        !readShort(&(save->gems), f) ||
        !readShort(&(save->keys), f) ||
        !readShort(&(save->sextants), f))
        return 0;

    for (i = 0; i < ARMR_MAX; i++) {
        if (!readShort(&(save->armor[i]), f))
            return 0;
    }

    for (i = 0; i < WEAP_MAX; i++) {
        if (!readShort(&(save->weapons[i]), f))
            return 0;
    }

    for (i = 0; i < REAG_MAX; i++) {
        if (!readShort(&(save->reagents[i]), f))
            return 0;
    }

    for (i = 0; i < 26; i++) {
        if (!readShort(&(save->mixtures[i]), f))
            return 0;
    }

    if (!readShort(&(save->unknown2), f) ||
        !readChar(&(save->x), f) ||
        !readChar(&(save->y), f))
        return 0;

    for (i = 0; i < 32; i++) {
        if (!readChar(&(save->unknown3[i]), f))
            return 0;
    }

    return 1;
}

void saveGameInit(SaveGame *save, int x, int y, const SaveGamePlayerRecord *avatarInfo) {
    int i;

    save->unknown1 = 0;
    save->moves = 0;

    memcpy(&(save->players[0]), avatarInfo, sizeof(SaveGamePlayerRecord));
    for (i = 1; i < 8; i++)
        saveGamePlayerRecordInit(&(save->players[i]));

    save->food = 300;
    save->gold = 200;

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

    save->unknown2 = 0;
    save->x = x;
    save->y = y;

    for (i = 0; i < 32; i++)
        save->unknown3[i] = 0;
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
        !writeChar(record->class, f) ||
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
    record->weapon = s;
    if (!readShort(&s, f))
        return 0;
    record->armor = s;

    for (i = 0; i < 16; i++) {
        if (!readChar(&(record->name[i]), f))
            return 0;
    }

    if (!readChar(&ch, f))
        return 0;
    record->sex = ch;
    if (!readChar(&(record->class), f))
        return 0;
    if (!readChar(&ch, f))
        return 0;
    record->status = ch;

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
    record->class = 0;
    record->status = STAT_GOOD;
}
