#include <boron/urlan.h>
#include "stringTable.h"
#include "cdi.h"

struct StringTable;

typedef struct
{
    UBuffer entries;            // Master CDIEntry array
    UBuffer fileIndex;          // Master FNAM index into entries
    StringTable modulePaths;    // Layer file names
    uint8_t category[4];        // ModuleCategory for each modulePaths entry.
}
Module;

enum ModuleCategory
{
    MOD_UNKNOWN,
    MOD_FILE_PACKAGE,
    MOD_BASE,
    MOD_EXTENSION,
    MOD_SOUNDTRACK
};

// Order matches modi context in pack-xu4.b.
enum ModInfoValues
{
    MI_ABOUT,
    MI_AUTHOR,
    MI_RULES,
    MI_VERSION,

    MI_COUNT
};

#ifdef __cplusplus
extern "C" {
#endif

void            mod_init(Module*, int layers);
void            mod_free(Module*);
const char*     mod_addLayer(Module*, const char* filename,
                         const char* version,
                         const char* (*config)(FILE*, const CDIEntry*, void*),
                         void* user);
void            mod_removeLayer(Module*);
const char*     mod_path(const Module*, const CDIEntry* ent);
const CDIEntry* mod_findAppId(const Module*, uint32_t id);
const CDIEntry* mod_fileEntry(const Module*, const char* filename);

int             mod_extension(const char* name, int* slen);
int             mod_namesEqual(const char* a, const char* b);
int             mod_query(const char* filename, StringTable* modi);

#ifdef __cplusplus
}
#endif
