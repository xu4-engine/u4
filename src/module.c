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

#define ENTRIES(mod)    (const CDIEntry*) mod->entries.ptr.v
#define FILE_INDEX(mod) (HashEntry*) mod->fileIndex.ptr.v

typedef struct
{
    uint32_t hash;
    uint32_t entry;
}
HashEntry;

// Order matches modi context in pack-xu4.b.
enum ModInfoValues
{
    MI_ABOUT,
    MI_AUTHOR,
    MI_RULES,
    MI_VERSION,

    MI_COUNT
};

void mod_init(Module* mod, int layers)
{
    ur_arrInit(&mod->entries, sizeof(CDIEntry), 128);
    ur_arrInit(&mod->fileIndex, sizeof(HashEntry), 64);
    ur_strInit(&mod->modulePaths, UR_ENC_UTF8, layers * 128);
    mod->pathCount = 0;
}

void mod_free(Module* mod)
{
    ur_arrFree(&mod->entries);
    ur_arrFree(&mod->fileIndex);
    ur_strFree(&mod->modulePaths);
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
    CDIEntry header;
    CDIEntry* toc;
    CDIStringTable stab;
    const CDIEntry* ent;
    const char* error = NULL;
    FILE* fp;
    int start;
    int tocLen;

    fp = cdi_openPak(filename, &header);
    if (! fp)
       return "Cannot open module";

    if (header.appId != CDI32('x','u','4', 1)) {
        fclose(fp);
        return "Invalid module id";
    }

    toc = cdi_loadPakTOC(fp, &header);
    if (! toc) {
        fclose(fp);
        return "No module TOC";
    }
    tocLen = header.bytes / sizeof(CDIEntry);

#define NO_PTR(ptr, msg)    if (! ptr) { error = msg; goto fail_toc; }

    ent = cdi_findAppId(toc, tocLen, CDI32('M','O','D','I'));
    if (ent) {
        char* vers;
        const char* str;
        uint8_t* modiBuf = cdi_loadPakChunk(fp, ent);
        NO_PTR(modiBuf, "Read MODI failed");
        cdi_initStringTable(&stab, modiBuf);

        if (stab.form != 1 || stab.count < MI_COUNT) {
            error = "Invalid MODI";
            goto fail_toc;
        }

        if (version) {
            str = stab.strings + stab.index.f1[MI_VERSION];
            if (strcmp(version, str)) {
                error = "Base module version mismatch";
                goto fail_toc;
            }
        }

        // Check if package is a game extension.
        str = stab.strings + stab.index.f1[MI_RULES];
        vers = strchr(str, '/');
        if (vers) {
            char bpath[512];
            *vers = '\0';

            if (! u4find_pathc(str, ".mod", bpath, sizeof(bpath))) {
                error = "Base module not found";
                goto fail_toc;
            }

            error = mod_addLayer(mod, bpath, vers+1, config, user);
            if (error)
                goto fail_toc;
        }
        free(modiBuf);
    } else if (version) {
        error = "Missing MODI";
        goto fail_toc;
    }

    start = mod->entries.used;

    // Append TOC to entries.
    {
    CDIEntry* it;
    uint8_t* layer;
    int n;
    int layerNum = mod->pathCount;

    ur_arrExpand(&mod->entries, start, tocLen);
    it = (CDIEntry*) mod->entries.ptr.v + start;
    memcpy(it, toc, header.bytes);

    layer = (uint8_t*) &it->cdi;
    for (n = 0; n < tocLen; ++n) {
        *layer = layerNum;
        layer += sizeof(CDIEntry);
    }
    }

    // Append module path.
    {
    UBuffer* paths = &mod->modulePaths;
    mod->pathIndex[ mod->pathCount++ ] = paths->used;
    ur_strAppendCStr(paths, filename);
    ur_strTermNull(paths);
    ++paths->used;
    }

    // Add fileIndex entries for FNAM strings.
    ent = cdi_findAppId(toc, tocLen, CDI32('F','N','A','M'));
    if (ent) {
        uint8_t* fnamBuf = cdi_loadPakChunk(fp, ent);
        NO_PTR(fnamBuf, "Read FNAM failed");
        cdi_initStringTable(&stab, fnamBuf);

        if (stab.form == 1) {
            const char* str;
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
                } else {
                    a = 'I';    // .png
                    b = 'M';
                }
                appId = CDI32(a, b, (i >> 8), (i & 0xff));

                ent = cdi_findAppId(toc, tocLen, appId);
                if (ent)
                    mod_registerFile(mod, hash, start + (ent - toc));
            }
        }
        free(fnamBuf);
    }

    // Process CONF chunk.
    if (config) {
        ent = cdi_findAppId(toc, tocLen, CDI32('C','O','N','F'));
        NO_PTR(ent, "Module CONF not found");
        error = config(fp, ent, user);
        //if (error) goto fail_toc;
    }

fail_toc:
    free(toc);
    fclose(fp);
    return error;
}

const char* mod_path(const Module* mod, const CDIEntry* ent)
{
    int i = ent->cdi & CDI_MASK_DA;
#ifdef __BIG_ENDIAN__
    i >>= 24;
#endif
    return mod->modulePaths.ptr.c + mod->pathIndex[i];
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
