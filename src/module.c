/*
 * A Module consists of a number of CDI Packages called "layers".
 */

#include <stdlib.h>
#include <string.h>
#include "module.h"

extern int u4find_pathc(const char*, const char*, char*, size_t);

#include "murmurHash3.c"
#define hashFunc(str,len)   murmurHash3_32((const uint8_t*)(str), len, 0x554956)

// The high 0xDA byte of the CDIEntry::cdi is replaced with the layer number.
#define CDI_MASK_DA     CDI32(0xFF,0x00,0x00,0x00)

#define APPID_CONF      CDI32('C','O','N','F')
#define APPID_MODI      CDI32('M','O','D','I')

#define ENTRIES(mod)    (const CDIEntry*) mod->entries.ptr.v
#define FILE_INDEX(mod) (HashEntry*) mod->fileIndex.ptr.v

typedef struct
{
    uint32_t hash;
    uint32_t entry;
}
HashEntry;

void mod_init(Module* mod, int layers)
{
    ur_arrInit(&mod->entries, sizeof(CDIEntry), 128);
    ur_arrInit(&mod->fileIndex, sizeof(HashEntry), 64);
    sst_init(&mod->modulePaths, layers, 128);
}

void mod_free(Module* mod)
{
    ur_arrFree(&mod->entries);
    ur_arrFree(&mod->fileIndex);
    sst_free(&mod->modulePaths);
}

static void mod_registerFile(Module* mod, uint32_t hash, int entryIndex)
{
    HashEntry* fi  = FILE_INDEX(mod);
    HashEntry* end = fi + mod->fileIndex.used;
    while (fi != end) {
        if (fi->hash == hash) {
            // Overwrite existing HashEntry.
            fi->entry = entryIndex;
            return;
        }
        ++fi;
    }

    // Append new HashEntry.
    UBuffer* buf = &mod->fileIndex;
    ur_arrExpand1(HashEntry, buf, fi);
    fi->hash  = hash;
    fi->entry = entryIndex;
}

typedef struct
{
    CDIEntry header;
    CDIEntry* toc;
    uint8_t* modiBuf;
    FILE* fp;
    int tocLen;
}
ModuleLoader;

static void mod_closeModule(ModuleLoader* ml)
{
    free(ml->toc);
    free(ml->modiBuf);
    fclose(ml->fp);
}

/*
 * Open module file and read the Table of Contents and MODI chunk.
 *
 * \param version   Version of required base module or NULL.
 */
static const char* mod_openModule(ModuleLoader* ml, const char* filename,
                                  const char* version,
                                  CDIStringTable* modi)
{
    const CDIEntry* ent;
    const char* error;

    modi->count = 0;

    ml->toc = NULL;
    ml->modiBuf = NULL;

    ml->fp = cdi_openPak(filename, &ml->header);
    if (! ml->fp)
       return "Cannot open module";

    if (ml->header.appId != CDI32('x','u','B', 2)) {
        fclose(ml->fp);
        return "Invalid module id";
    }

    ml->toc = cdi_loadPakTOC(ml->fp, &ml->header);
    if (! ml->toc) {
        fclose(ml->fp);
        return "No module TOC";
    }
    ml->tocLen = ml->header.bytes / sizeof(CDIEntry);

    ent = cdi_findAppId(ml->toc, ml->tocLen, APPID_MODI);
    if (ent) {
        ml->modiBuf = cdi_loadPakChunk(ml->fp, ent);
        if (! ml->modiBuf) {
            error = "Read MODI failed";
            goto fail_toc;
        }
        cdi_initStringTable(modi, ml->modiBuf);

        if (modi->form != 1 || modi->count < MI_COUNT) {
            error = "Invalid MODI";
            goto fail_toc;
        }

        if (version) {
            const char* str = modi->strings + modi->index.f1[MI_VERSION];
            if (strcmp(version, str)) {
                error = "Base module version mismatch";
                goto fail_toc;
            }
        }
    } else if (version) {
        error = "Missing MODI";
        goto fail_toc;
    }
    return NULL;

fail_toc:
    mod_closeModule(ml);
    return error;
}

// Return position of ".mod" extension or 0 if none.
int mod_extension(const char* name, int* slen)
{
    int len = strlen(name);
    *slen = len;
    if (len > 4 && strcmp(name + len - 4, ".mod") == 0)
        return len - 4;
    return 0;
}

// Compare names ignoring any ".mod" extension.
int mod_namesEqual(const char* a, const char* b)
{
    int lenA, lenB;
    int modA = mod_extension(a, &lenA);
    int modB = mod_extension(b, &lenB);
    if (modA)
        lenA = modA;
    if (modB)
        lenB = modB;
    return ((lenA == lenB) && strncmp(a, b, lenA) == 0);
}

static int mod_loaded(Module* mod, const char* name, const char* version)
{
    const char* base;
    const char* pstart;
    int len;
    uint32_t i;

    // Search the set paths to see if the module is already loaded.
    for (i = 0; i < mod->modulePaths.used; ++i) {
        pstart = sst_stringL(&mod->modulePaths, i, &len);
        for (base = pstart + len; base != pstart; ) {
            --base;
            if (*base == '/' || *base == '\\') {
                ++base;
                break;
            }
        }

        // TODO: Also check version (needs to be saved).
        if (mod_namesEqual(base, name))
            return 1;
    }
    return 0;
}

/*
 * \param mod       Pointer to initialized module.
 * \param filename  Path to package.
 * \param version   Required MODI version or NULL if not applicable.
 * \param config    Callback function for CONF chunk or NULL to ignore.
 * \param user      Callback user data.
 *
 * Return error message or NULL if successful.
 */
const char* mod_addLayer(Module* mod, const char* filename,
                         const char* version,
                         const char* (*config)(FILE*, const CDIEntry*, void*),
                         void* user)
{
    ModuleLoader ml;
    CDIStringTable stab;
    const CDIEntry* ent;
    const char* error;
    const char* str;
    int start;
    int extIdMask = 0;

    //printf("mod_addLayer %s\n", filename);

    error = mod_openModule(&ml, filename, version, &stab);
    if (error)
        return error;

    // Check if package is a game extension.
    if (stab.count) {
        char* vers;

        str = stab.strings + stab.index.f1[MI_RULES];
        vers = strchr(str, '/');
        if (vers) {
            *vers = '\0';   // Terminate str.

            if (! mod_loaded(mod, str, vers+1)) {
                char bpath[512];

                if (! u4find_pathc(str, ".mod", bpath, sizeof(bpath))) {
                    error = "Base module not found";
                    goto fail_layer;
                }

                error = mod_addLayer(mod, bpath, vers+1, config, user);
                if (error)
                    goto fail_layer;
            }

            extIdMask = 0x20;       // Match module-layer in pack-xu4.b
        }
    }

    start = mod->entries.used;

    // Append TOC to entries.
    {
    CDIEntry* it;
    uint8_t* layer;
    int n;
    int layerNum = mod->modulePaths.used;

    ur_arrExpand(&mod->entries, start, ml.tocLen);
    it = (CDIEntry*) mod->entries.ptr.v + start;
    memcpy(it, ml.toc, ml.header.bytes);

    // Replace high 0xDA byte with layer number in all entries.
    layer = (uint8_t*) &it->cdi;
    for (n = 0; n < ml.tocLen; ++n) {
        *layer = layerNum;
        layer += sizeof(CDIEntry);
    }
    }

    // Append module path.
    sst_append(&mod->modulePaths, filename, -1);

#define NO_PTR(ptr, msg)    if (! ptr) { error = msg; goto fail_layer; }

    // Add fileIndex entries for FNAM strings.
    ent = cdi_findAppId(ml.toc, ml.tocLen, CDI32('F','N','A','M'));
    if (ent) {
        uint8_t* fnamBuf = cdi_loadPakChunk(ml.fp, ent);
        NO_PTR(fnamBuf, "Read FNAM failed");
        cdi_initStringTable(&stab, fnamBuf);

        if (stab.form == 1) {
            uint16_t* it = stab.index.f1;
            uint32_t appId;
            uint32_t hash;
            size_t len;
            int a, b;
            uint32_t i;

            // Map source filenames to CDIEntry.
            for (i = 0; i < stab.count; ++it, ++i) {
                str = stab.strings + *it;
                len = strlen(str);
                if (len < 1)
                    continue;
                hash = hashFunc(str, len);

                if (str[len - 1] == 'l') {
                    a = 'S';    // .glsl
                    b = 'L';
                } else if (str[len - 1] == 'f') {
                    a = 'T';    // .txf
                    b = 'F';
                } else {
                    a = 'I';    // .png
                    b = 'M';
                }
                appId = CDI32(a, b, (extIdMask | (i >> 8)), (i & 0xff));

                ent = cdi_findAppId(ml.toc, ml.tocLen, appId);
                if (ent)
                    mod_registerFile(mod, hash, start + (ent - ml.toc));
            }
        }
        free(fnamBuf);
    }

    // Process CONF chunk.
    if (config) {
        ent = cdi_findAppId(ml.toc, ml.tocLen, APPID_CONF);
        NO_PTR(ent, "Module CONF not found");
        error = config(ml.fp, ent, user);
        //if (error) goto fail_layer;
    }

fail_layer:
    mod_closeModule(&ml);
    return error;
}

const char* mod_path(const Module* mod, const CDIEntry* ent)
{
    int i = ent->cdi & CDI_MASK_DA;
    int len;
#ifdef __BIG_ENDIAN__
    i >>= 24;
#endif
    return sst_stringL(&mod->modulePaths, i, &len);
}

const CDIEntry* mod_findAppId(const Module* mod, uint32_t id)
{
    // Search in reverse order.
    const CDIEntry* end = ENTRIES(mod) - 1;
    const CDIEntry* it  = end + mod->entries.used;
    while (it != end) {
        if (it->appId == id)
            return it;
        --it;
    }
    return NULL;
}

const CDIEntry* mod_fileEntry(const Module* mod, const char* filename)
{
    uint32_t hash = hashFunc(filename, strlen(filename));
    const HashEntry* fi  = FILE_INDEX(mod);
    const HashEntry* end = fi + mod->fileIndex.used;
    while (fi != end) {
        if (fi->hash == hash)
            return ENTRIES(mod) + fi->entry;
        ++fi;
    }
    return NULL;
}

/*
 * Return ModuleCategory and append MODI strings to modInfo.
 */
int mod_query(const char* filename, StringTable* modInfo)
{
    ModuleLoader ml;
    CDIStringTable modi;
    const uint32_t muMask = CDI32(0xff,0xff,0,0);
    const uint32_t muId   = CDI32('M','U',0,0);
    int cat;

    if (mod_openModule(&ml, filename, NULL, &modi))
       return MOD_UNKNOWN;

    if (modi.count) {
        const char* rules = modi.strings + modi.index.f1[MI_RULES];
        uint32_t i;

        if (strchr(rules, '/'))
            cat = MOD_EXTENSION;
        else
            cat = MOD_BASE;

        for (i = 0; i < modi.count; ++i)
            sst_append(modInfo, modi.strings + modi.index.f1[i], -1);
    } else
        cat = MOD_FILE_PACKAGE;

    // Check if package only contains music chunks.
    if (cat == MOD_EXTENSION) {
        const CDIEntry* it  = ml.toc;
        const CDIEntry* end = it + ml.tocLen;
        for (; it != end; ++it) {
            if (it->appId == APPID_CONF || it->appId == APPID_MODI)
                continue;
            if ((it->appId & muMask) != muId)
                break;
        }
        if (it == end)
            cat = MOD_SOUNDTRACK;
    }

    mod_closeModule(&ml);
    return cat;
}
