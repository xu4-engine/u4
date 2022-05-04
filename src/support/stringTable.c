/*
 * Small Static String Table
 * Written and dedicated to the public domain in 2022 by Karl Robillard.
 *
 * An expandable array of nul terminated static strings.
 * A single block of memory is allocated for both the index and all strings.
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "stringTable.h"

#define DEFAULT_AVAIL   8
#define MIN_STR_LEN     8

static void sst_resize(StringTable* st, int count)
{
    StringEntry* newTable =
        (StringEntry*) malloc(count * (sizeof(StringEntry) + st->allocLen));
    assert(newTable);

    if (st->used) {
        memcpy(newTable, st->table, st->used * sizeof(StringEntry));
        memcpy(newTable + count, sst_strings(st), st->storeUsed);
    }

    free(st->table);
    st->table = newTable;
    st->avail = count;
}

void sst_init(StringTable* st, int reserve, int averageLen)
{
    memset(st, 0, sizeof(StringTable));
    st->allocLen = (averageLen < MIN_STR_LEN) ? MIN_STR_LEN : averageLen;
    if (reserve > 0)
        sst_resize(st, reserve);
}

void sst_free(StringTable* st)
{
    free(st->table);
    st->table = NULL;
    st->avail = st->used = 0;
}

static char* sst_make(StringTable* st, int len)
{
    StringEntry* ent;
    char* store;
    int storeAvail = st->avail * st->allocLen;
    int storeUsedNew;

    storeUsedNew = st->storeUsed + len + 1;

    if (st->avail == 0)
        sst_resize(st, DEFAULT_AVAIL);
    else if (st->used == st->avail || storeUsedNew > storeAvail)
        sst_resize(st, st->avail * 2);
    assert(storeUsedNew <= storeAvail);

    ent = st->table + st->used;
    ent->start = st->storeUsed;
    ent->len   = len;
    st->used++;

    store = ((char*) (st->table + st->avail)) + st->storeUsed;
    store[len] = '\0';
    st->storeUsed = storeUsedNew;
    return store;
}

void sst_append(StringTable* st, const char* str, int len)
{
    if (len < 0)
        len = strlen(str);
    char* store = sst_make(st, len);
    if (len)
        memcpy(store, str, len);
}

/*
 * Concatenate two strings and append the new string to the table.
 */
void sst_appendCon(StringTable* st, const char* strA, const char* strB)
{
    size_t lenA = strlen(strA);
    size_t lenB = strlen(strB);
    char* store = sst_make(st, lenA + lenB);
    memcpy(store, strA, lenA);
    memcpy(store + lenA, strB, lenB);
}

/*
 * Return index of string or -1 if not found.
 */
int sst_find(const StringTable* st, const char* pattern, int len)
{
    const char* store = sst_strings(st);
    uint32_t i;

    if (len < 0)
        len = strlen(pattern);

    for (i = 0; i < st->used; ++i) {
        if (strncmp(pattern, store + sst_start(st, i), len) == 0)
            return i;
    }
    return -1;
}

const char* sst_stringL(const StringTable* st, int n, int* plen)
{
    const StringEntry* ent = st->table + n;
    *plen = ent->len;
    return sst_strings(st) + ent->start;
}
