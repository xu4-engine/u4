#include <stdio.h>

#include "savegame.h"

void showSaveGame(SaveGame *sg);
void showSaveGamePlayerRecord(SaveGamePlayerRecord *rec);
char *weaponName(WeaponType weapon);
char *armorName(ArmorType armor);
char *className(ClassType klass);
char *itemsString(unsigned short items);

int main(int argc, char *argv[]) {
    SaveGame sg;
    FILE *in;

    if (argc != 2) {
        fprintf(stderr, "usage: %s party.sav", argv[0]);
        exit(1);
    }

    in = fopen(argv[1], "r");
    if (!in) {
        perror(argv[1]);
        exit(1);
    }

    saveGameRead(&sg, in);

    showSaveGame(&sg);

    return 0;
}

void showSaveGame(SaveGame *sg) {
    int i;

    printf("???: %x\n", sg->unknown1);
    printf("moves: %-4d food: %-5g gold: %d\n", 
           sg->moves, ((double)sg->food) / 100.0, sg->gold);

    printf("karma: [ ");
    for (i = 0; i < 8; i++)
        printf("%d ", sg->karma[i]);
    printf("]\n");

    printf("torches: %-2d gems: %-5d keys: %-5d sextants: %d\n", 
           sg->torches, sg->gems, sg->keys, sg->sextants);

    printf("armor: [ ");
    for (i = 0; i < ARMR_MAX; i++)
        printf("%d ", sg->armor[i]);
    printf("]\n");

    printf("weapons: [ ");
    for (i = 0; i < WEAP_MAX; i++)
        printf("%d ", sg->weapons[i]);
    printf("]\n");

    printf("reagents: [ ");
    for (i = 0; i < REAG_MAX; i++)
        printf("%d ", sg->reagents[i]);
    printf("]\n");

    printf("mixtures: [ ");
    for (i = 0; i < 26; i++)
        printf("%d ", sg->mixtures[i]);
    printf("]\n");

    printf("items: %s\n", itemsString(sg->items));

    printf("x: %-8d y: %d\n", sg->x, sg->y);

    printf("stones: %-3x runes %x\n", sg->stones, sg->runes);

    printf("party members: %d\n", sg->members);
    printf("transport: %x\n", sg->transport);
    printf("balloon state: %x\n", sg->balloonstate);
    printf("???: %x\n", sg->unknown2);
    printf("???: %x\n", sg->unknown3);
    printf("???: %x\n", sg->unknown4);
    printf("lbintro: %d\n", sg->lbintro);
    printf("???: %x\n", sg->unknown5);
    printf("???: %x\n", sg->unknown6);
    printf("???: %x\n", sg->unknown7);
    printf("???: %x\n", sg->unknown8);
    printf("dngx: %-5d dngy: %-5d orientation: %d dnglevel: %d\n", sg->dngx, sg->dngy, sg->orientation, sg->dnglevel);

    printf("???: %d\n", sg->unknown9);

    for (i = 0; i < 8; i++) {
        printf("player %d\n", i);
        showSaveGamePlayerRecord(&(sg->players[i]));
    }
}

void showSaveGamePlayerRecord(SaveGamePlayerRecord *rec) {
    printf("  name: %-17s hp: %-7d hpMax: %-4d xp: %d\n", 
           rec->name, rec->hp, rec->hpMax, rec->xp);
    printf("  str: %-6d dex: %-6d intel: %-4d mp: %-7d ???: %d\n", 
           rec->str, rec->dex, rec->intel, rec->mp, rec->unknown);
    printf("  weapon: %-15s armor: %s\n", weaponName(rec->weapon), armorName(rec->armor));
    printf("  sex: %-6s class: %-16s status: %c\n", 
           rec->sex == 11 ? "M" : "F", className(rec->klass), rec->status);
}

char *weaponName(WeaponType weapon) {
    switch (weapon) {
    case WEAP_HANDS:
        return "hands";
    case WEAP_STAFF:
        return "staff";
    case WEAP_DAGGER:
        return "dagger";
    case WEAP_SLING:
        return "sling";
    case WEAP_MACE:
        return "mace";
    case WEAP_AXE:
        return "axe";
    case WEAP_SWORD:
        return "sword";
    case WEAP_BOW:
        return "bow";
    case WEAP_CROSSBOW:
        return "crossbow";
    case WEAP_OIL:
        return "oil";
    case WEAP_HALBERD:
        return "halberd";
    case WEAP_MAGICAXE:
        return "magic axe";
    case WEAP_MAGICSWORD:
        return "magic sword";
    case WEAP_MAGICBOW:
        return "magic bow";
    case WEAP_MAGICWAND:
        return "magic wand";
    case WEAP_MYSTICSWORD:
        return "mystic sword";
    default:
        return "???";
    }
}

char *armorName(ArmorType armor) {
    switch (armor) {
    case ARMR_NONE:
        return "none";
    case ARMR_CLOTH:
        return "cloth";
    case ARMR_LEATHER:
        return "leather";
    case ARMR_CHAIN:
        return "chain";
    case ARMR_PLATE:
        return "plate";
    case ARMR_MAGICCHAIN:
        return "magicchain";
    case ARMR_MAGICPLATE:
        return "magicplate";
    case ARMR_MYSTICROBES:
        return "mysticrobes";
    default:
        return "???";
    }
}

char *className(ClassType klass) {
    switch (klass) {
    case CLASS_MAGE:
        return "mage";
    case CLASS_BARD:
        return "bard";
    case CLASS_FIGHTER:
        return "fighter";
    case CLASS_DRUID:
        return "druid";
    case CLASS_TINKER:
        return "tinker";
    case CLASS_PALADIN:
        return "paladin";
    case CLASS_RANGER:
        return "ranger";
    case CLASS_SHEPHERD:
        return "shepherd";
    default:
        return "???";
    }
}

char *itemsString(unsigned short items) {
    static char buffer[256];
    int first = 1;

    buffer[0] = '\0';

    if (items & ITEM_SKULL) {
        strcat(strcat(buffer, first ? "" : ", "), "skull");
        first = 0;
    }
    if (items & ITEM_SKULL_DESTROYED) {
        strcat(strcat(buffer, first ? "" : ", "), "skull destroyed");
        first = 0;
    }
    if (items & ITEM_CANDLE) {
        strcat(strcat(buffer, first ? "" : ", "), "candle");
        first = 0;
    }
    if (items & ITEM_BOOK) {
        strcat(strcat(buffer, first ? "" : ", "), "book");
        first = 0;
    }
    if (items & ITEM_BELL) {
        strcat(strcat(buffer, first ? "" : ", "), "bell");
        first = 0;
    }
    if (items & ITEM_KEY_C) {
        strcat(strcat(buffer, first ? "" : ", "), "key c");
        first = 0;
    }
    if (items & ITEM_KEY_L) {
        strcat(strcat(buffer, first ? "" : ", "), "key l");
        first = 0;
    }
    if (items & ITEM_KEY_T) {
        strcat(strcat(buffer, first ? "" : ", "), "key t");
        first = 0;
    }
    if (items & ITEM_HORN) {
        strcat(strcat(buffer, first ? "" : ", "), "horn");
        first = 0;
    }
    if (items & ITEM_WHEEL) {
        strcat(strcat(buffer, first ? "" : ", "), "wheel");
        first = 0;
    }
    if (items & ITEM_CANDLE_USED) {
        strcat(strcat(buffer, first ? "" : ", "), "candle used");
        first = 0;
    }
    if (items & ITEM_BOOK_USED) {
        strcat(strcat(buffer, first ? "" : ", "), "book used");
        first = 0;
    }
    if (items & ITEM_BELL_USED) {
        strcat(strcat(buffer, first ? "" : ", "), "bell used");
        first = 0;
    }
    if (items & 0x2000) {
        strcat(strcat(buffer, first ? "" : ", "), "(bit 14)");
        first = 0;
    }
    if (items & 0x4000) {
        strcat(strcat(buffer, first ? "" : ", "), "(bit 15)");
        first = 0;
    }
    if (items & 0x8000) {
        strcat(strcat(buffer, first ? "" : ", "), "(bit 16)");
        first = 0;
    }
    
    return buffer;
}
