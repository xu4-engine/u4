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

    if (!writeShort(save->items, f) ||
        !writeChar(save->x, f) ||
        !writeChar(save->y, f) ||
        !writeChar(save->stones, f) ||
        !writeChar(save->runes, f) ||
        !writeShort(save->members, f) ||
        !writeShort(save->transport, f) ||
        !writeShort(save->balloonstate, f) ||
        !writeShort(save->unknown2, f) ||
        !writeShort(save->unknown3, f) ||
        !writeShort(save->unknown4, f) ||
        !writeShort(save->lbintro, f) ||
        !writeShort(save->unknown5, f) ||
        !writeShort(save->unknown6, f) ||
        !writeShort(save->unknown7, f) ||
        !writeShort(save->unknown8, f) ||
        !writeChar(save->dngx, f) ||
        !writeChar(save->dngy, f) ||
        !writeShort(save->orientation, f) ||
        !writeShort(save->dnglevel, f) ||
        !writeShort(save->unknown9, f))
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

    if (!readShort(&(save->items), f) ||
        !readChar(&(save->x), f) ||
        !readChar(&(save->y), f) ||
        !readChar(&(save->stones), f) ||
        !readChar(&(save->runes), f) ||
        !readShort(&(save->members), f) ||
        !readShort(&(save->transport), f) ||
        !readShort(&(save->balloonstate), f) ||
        !readShort(&(save->unknown2), f) ||
        !readShort(&(save->unknown3), f) ||
        !readShort(&(save->unknown4), f) ||
        !readShort(&(save->lbintro), f) ||
        !readShort(&(save->unknown5), f) ||
        !readShort(&(save->unknown6), f) ||
        !readShort(&(save->unknown7), f) ||
        !readShort(&(save->unknown8), f) ||
        !readChar(&(save->dngx), f) ||
        !readChar(&(save->dngy), f) ||
        !readShort(&(save->orientation), f) ||
        !readShort(&(save->dnglevel), f) ||
        !readShort(&(save->unknown9), f))
        return 0;

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

    save->items = 0;
    save->x = x;
    save->y = y;
    save->stones = 0;
    save->runes = 0;
    save->members = 1;
    save->transport = 0x1f;
    save->balloonstate = 0;
    save->unknown2 = 0;
    save->unknown3 = 0;
    save->unknown4 = 0;
    save->lbintro = 0;
    save->unknown5 = 0;
    save->unknown6 = 0;
    save->unknown7 = 0;
    save->unknown8 = 0;
    save->dngx = 0;
    save->dngy = 0;
    save->orientation = 0;
    save->dnglevel = 0;
    save->unknown9 = 0;
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
    if (!readChar(&ch, f))
      return 0;
    record->klass = ch;
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
    record->klass = CLASS_MAGE;
    record->status = STAT_GOOD;
}
