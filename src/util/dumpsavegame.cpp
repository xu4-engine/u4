#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "savegame.h"
#include "names.h"

int verbose = 0;

void showSaveGame(SaveGame *sg);
void showSaveGamePlayerRecord(SaveGamePlayerRecord *rec);
char *itemsString(unsigned short items);

int main(int argc, char *argv[]) {
    SaveGame sg;
    FILE *in;

    if (argc != 2) {
        fprintf(stderr, "usage: %s party.sav\n", argv[0]);
        exit(1);
    }

    in = fopen(argv[1], "rb");
    if (!in) {
        perror(argv[1]);
        exit(1);
    }

    sg.read(in);

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
    printf("balloon state/torch duration: %x\n", sg->balloonstate);
    printf("trammel: %d  felucca: %d\n", sg->trammelphase, sg->feluccaphase);
    printf("shiphull: %d\n", sg->shiphull);
    printf("lbintro: %d\n", sg->lbintro);
    printf("lastcamp: %d       lastreagent: %d\n", sg->lastcamp, sg->lastreagent);
    printf("lastmeditation: %d lastvirtue: %d\n", sg->lastmeditation, sg->lastvirtue);
    printf("dngx: %-5d dngy: %-5d orientation: %d dnglevel: %d\n", sg->dngx, sg->dngy, sg->orientation, sg->dnglevel);

    printf("location: %x\n", sg->location);

    for (i = 0; i < 8; i++) {
        printf("player %d\n", i);
        showSaveGamePlayerRecord(&(sg->players[i]));
    }
}

void showSaveGamePlayerRecord(SaveGamePlayerRecord *rec) {
    static const char * const weapNames[] = {
        "Hands", "Staff", "Dagger",
        "Sling", "Mace", "Axe",
        "Sword", "Bow", "Crossbow",
        "Flaming Oil", "Halberd", "Magic Axe",
        "Magic Sword", "Magic Bow", "Magic Wand",
        "Mystic Sword"
    };

    static const char * const armorNames[] = {
        "Skin", "Cloth", "Leather", 
        "Chain Mail", "Plate Mail", 
        "Magic Chain", "Magic Plate", "Mystic Robe"
    };

    printf("  name: %-17s hp: %-7d hpMax: %-4d xp: %d\n", 
           rec->name, rec->hp, rec->hpMax, rec->xp);
    printf("  str: %-6d dex: %-6d intel: %-4d mp: %-7d ???: %d\n",
           rec->str, rec->dex, rec->intel, rec->mp, rec->unknown);
    printf("  weapon: %-15s armor: %s\n", weapNames[rec->weapon], armorNames[rec->armor]);
    printf("  sex: %-6s class: %-16s status: %c\n", 
           rec->sex == 11 ? "M" : "F", getClassName(rec->klass), rec->status);
}

char *itemsString(unsigned short items) {
    static char buffer[256];
    int first = 1;

    buffer[0] = '\0';

    if (items & ITEM_SKULL) {
        strcat(strcat(buffer, first ? "" : ", "), getItemName(ITEM_SKULL));
        first = 0;
    }
    if (items & ITEM_SKULL_DESTROYED) {
        strcat(strcat(buffer, first ? "" : ", "), "skull destroyed");
        first = 0;
    }
    if (items & ITEM_CANDLE) {
        strcat(strcat(buffer, first ? "" : ", "), getItemName(ITEM_CANDLE));
        first = 0;
    }
    if (items & ITEM_BOOK) {
        strcat(strcat(buffer, first ? "" : ", "), getItemName(ITEM_BOOK));
        first = 0;
    }
    if (items & ITEM_BELL) {
        strcat(strcat(buffer, first ? "" : ", "), getItemName(ITEM_BELL));
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
        strcat(strcat(buffer, first ? "" : ", "), getItemName(ITEM_HORN));
        first = 0;
    }
    if (items & ITEM_WHEEL) {
        strcat(strcat(buffer, first ? "" : ", "), getItemName(ITEM_WHEEL));
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
