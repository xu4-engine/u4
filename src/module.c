/*
 * A Module consists of a number of CDI Packages called "layers".
 */

#include <stdlib.h>
#include <string.h>
#include "module.h"

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
 * Return error message or NULL if successful.
 */
const char* mod_addLayer(Module* mod, const char* filename, FILE** pf)
{
    CDIEntry header;
    CDIEntry* toc;
    CDIStringTable fnam;
    const CDIEntry* ent;
    const char* error = NULL;
    uint8_t* fnamBuf;
    FILE* fp;
    int start = mod->entries.used;
    int tocLen;

    fp = cdi_openPak(filename, &header);
    if (! fp)
       return "Cannot open module";

    if (header.appId != CDI32('x','u','4', 1)) {
        fclose(fp);
        return "Invalid module id";
    }

#define NO_PTR(ptr, msg)    if (! ptr) { error = msg; goto fail; }

    toc = cdi_loadPakTOC(fp, &header);
    NO_PTR(toc, "No module TOC");
    tocLen = header.bytes / sizeof(CDIEntry);

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
    NO_PTR(ent, "Module FNAM not found");
    fnamBuf = cdi_loadPakChunk(fp, ent);
    NO_PTR(fnamBuf, "Read FNAM failed");
    cdi_initStringTable(&fnam, fnamBuf);

    if (fnam.form == 1) {
        const char* str;
        uint16_t* it = fnam.index.f1;
        uint32_t appId;
        uint32_t hash;
        size_t len;
        int a, b;
        uint32_t i;

        // Map source filenames to CDIEntry.
        for (i = 0; i < fnam.count; ++it, ++i) {
            str = fnam.strings + *it;
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

#undef NO_PTR

    free(fnamBuf);
    free(toc);
    if (pf)
        *pf = fp;
    else
        fclose(fp);
    return NULL;

fail:
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
