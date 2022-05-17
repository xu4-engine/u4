#ifndef STRINGTABLE_H
#define STRINGTABLE_H
/*
 * Small Static String Table
 * Written and dedicated to the public domain in 2022 by Karl Robillard.
 *
 * An expandable array of nul terminated static strings.
 * A single block of memory is allocated for both the index and all strings.
 */

#include <stdint.h>

typedef struct {
    uint16_t start;     // LIMIT: 65k characters total.
    uint16_t len;
}
StringEntry;

typedef struct StringTable StringTable;

struct StringTable {
    StringEntry* table;
    uint32_t avail;
    uint32_t used;
    uint32_t storeUsed;
    uint16_t allocLen;
    uint8_t  encoding;      // User defined. Set to zero by sst_init().
    uint8_t  _pad;
};

#define sst_strings(ST)     ((const char*) ((ST)->table + (ST)->avail))
#define sst_start(ST,N)     (ST)->table[N].start
#define sst_len(ST,N)       (ST)->table[N].len

#ifdef __cplusplus
extern "C" {
#endif

void sst_init(StringTable*, int reserve, int averageLen);
void sst_free(StringTable*);
void sst_append(StringTable*, const char* str, int len);
void sst_appendCon(StringTable*, const char* strA, const char* strB);
int  sst_find(const StringTable*, const char* pattern, int len);
const char* sst_stringL(const StringTable*, int n, int* plen);

#ifdef __cplusplus
}
#endif

#endif  // STRINGTABLE_H
