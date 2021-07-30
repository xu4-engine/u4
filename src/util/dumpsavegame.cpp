// Dump Ultima 4 PARTY.SAV

#include <cstdio>
#include <cstdlib>
#include <cstring>

#define SAVE_UTIL
#include "savegame.cpp"

#define EX_USAGE     64  /* command line usage error */
#define EX_NOINPUT   66  /* cannot open input */

void showSaveGamePlayerRecord(SaveGamePlayerRecord *rec) {
    static const char* const weapNames[] = {
        "Hands",       "Staff",       "Dagger",     "Sling",
        "Mace",        "Axe",         "Sword",      "Bow",
        "Crossbow",    "Flaming-Oil", "Halberd",    "Magic-Axe",
        "Magic-Sword", "Magic-Bow",   "Magic-Wand", "Mystic-Sword"
    };
    static const char* const armorNames[] = {
        "none",  "Cloth",   "Leather", "Chain",
        "Plate", "M-Chain", "M-Plate", "M-Robe"
    };
    static const char* const classNames[] = {
        "Mage",   "Bard",    "Fighter", "Druid",
        "Tinker", "Paladin", "Ranger",  "Shepherd"
    };

    printf("%-12s %3d %3d %4d", rec->name, rec->hp, rec->hpMax, rec->xp);
    printf(" %2d  %2d  %2d  %2d %d",
           rec->str, rec->dex, rec->intel, rec->mp, rec->unknown);
    printf(" %-12s %-8s %c  %-9s %c\n",
           weapNames[rec->weapon],
           armorNames[rec->armor],
           rec->sex == 11 ? 'M' : 'F',
           (rec->klass < 8) ? classNames[rec->klass] : "???",
           rec->status);
}

char *itemsString(uint16_t items) {
    static char sbuf[256];
    size_t len;

    sbuf[0] = '\0';

    if (items & 0xff) {
        strcat(sbuf, "\n    ");
        if (items & ITEM_SKULL)
            strcat(sbuf, "skull ");
        if (items & ITEM_SKULL_DESTROYED)
            strcat(sbuf, "skull-destroyed ");
        if (items & ITEM_CANDLE)
            strcat(sbuf, "candle ");
        if (items & ITEM_BOOK)
            strcat(sbuf, "book ");
        if (items & ITEM_BELL)
            strcat(sbuf, "bell ");
        if (items & ITEM_KEY_C)
            strcat(sbuf, "key-courage ");
        if (items & ITEM_KEY_L)
            strcat(sbuf, "key-love ");
        if (items & ITEM_KEY_T)
            strcat(sbuf, "key-truth ");
    }

    if (items & 0xff00) {
        strcat(sbuf, "\n    ");
        if (items & ITEM_HORN)
            strcat(sbuf, "horn ");
        if (items & ITEM_WHEEL)
            strcat(sbuf, "wheel ");
        if (items & ITEM_CANDLE_USED)
            strcat(sbuf, "candle-used ");
        if (items & ITEM_BOOK_USED)
            strcat(sbuf, "book-used ");
        if (items & ITEM_BELL_USED)
            strcat(sbuf, "bell-used ");
        if (items & 0x2000)
            strcat(sbuf, "bit-14 ");
        if (items & 0x4000)
            strcat(sbuf, "bit-15 ");
        if (items & 0x8000)
            strcat(sbuf, "bit-16 ");
    }

    len = strlen(sbuf);
    if (len)
        sbuf[ len ] = '\n';

    return sbuf;
}

void showSaveGame(SaveGame *sg) {
    int i;

    printf("counter: 0x%x moves: %d\n", sg->unknown1, sg->moves);

    printf("players: [\n");
    printf("; Name        HP/max  XP  Str Dex Int MP ? Weapon       Armor   Sex Class   Stat\n");
    for (i = 0; i < 8; i++) {
        showSaveGamePlayerRecord(&(sg->players[i]));
    }
    printf("]\n");

    printf("food: %-5g gold: %d\n", ((double)sg->food) / 100.0, sg->gold);

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

    printf("mixtures: [");
    for (i = 0; i < 26; i++) {
        if ((i % 10) == 0)
            printf("\n    ");
        printf("%d ", sg->mixtures[i]);
    }
    printf("\n]\n");

    printf("items: [%s]\n", itemsString(sg->items));
    printf("x: %d y: %d\n", sg->x, sg->y);
    printf("stones: 0x%02x runes: 0x%02x\n", sg->stones, sg->runes);
    printf("party-members: %d\n", sg->members);
    printf("transport: 0x%x\n", sg->transport);
    printf("torch-duration: 0x%x\t; Or balloon-state\n", sg->balloonstate);
    printf("trammel: %d  felucca: %d\n", sg->trammelphase, sg->feluccaphase);
    printf("shiphull: %d\n", sg->shiphull);
    printf("lbintro: %d\n", sg->lbintro);
    printf("last-camp: %-5d       last-reagent: %d\n",
            sg->lastcamp, sg->lastreagent);
    printf("last-meditation: %-5d last-virtue: %d\n",
            sg->lastmeditation, sg->lastvirtue);
    printf("dngx: %-5d dngy: %-5d orientation: %d dnglevel: %d\n",
           sg->dngx, sg->dngy, sg->orientation, sg->dnglevel);
    printf("location: 0x%x\n", sg->location);
}

int main(int argc, char *argv[]) {
    SaveGame sg;
    FILE *in;

    if (argc != 2) {
        fprintf(stderr, "usage: %s party.sav\n", argv[0]);
        return EX_USAGE;
    }

    in = fopen(argv[1], "rb");
    if (!in) {
        perror(argv[1]);
        return EX_NOINPUT;
    }

    sg.read(in);
    showSaveGame(&sg);
    fclose(in);
    return 0;
}
