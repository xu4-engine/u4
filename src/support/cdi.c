/*
  CDI Package Functions
*/

#include <stdlib.h>
#include "cdi.h"

#if defined(__APPLE__)
#include <libkern/OSByteOrder.h>
#define bswap_16(x) OSSwapInt16(x)
#define bswap_32(x) OSSwapInt32(x)
//#define bswap_64(x) OSSwapInt64(x)

#elif defined(_MSC_VER)
#define bswap_16(x) _byteswap_ushort(x)
#define bswap_32(x) _byteswap_ulong(x)
//#define bswap_64(x) _byteswap_uint64(x)

#elif defined(__BYTE_ORDER__)
#ifdef __MINGW32__
#define bswap_16(x) __builtin_bswap16(x)
#define bswap_32(x) __builtin_bswap32(x)
#else
#include <byteswap.h>
#endif
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define __BIG_ENDIAN__  1
#endif

#else
#error __BYTE_ORDER__ not defined
#endif


void cdi_swap16(uint16_t* vars, size_t count)
{
    uint16_t* end = vars + count;
    for (; vars != end; ++vars)
        *vars = bswap_16(*vars);
}

void cdi_swap32(uint32_t* vars, size_t count)
{
    uint32_t* end = vars + count;
    for (; vars != end; ++vars)
        *vars = bswap_32(*vars);
}

/*
  Open a CDI package and read the header.

  \param filename   Path to CDI package file.
  \param phead      Return struct for package header.

  \return A standard FILE pointer which the caller must fclose(). NULL is
          returned if the header could not be read or it is invalid.
*/
FILE* cdi_openPak(const char* filename, CDIEntry* phead)
{
    FILE* fp = fopen(filename, "rb");
    if (! fp)
        return NULL;
    if (fread(phead, sizeof(uint32_t), 4, fp) != 4)
        goto fatal;
    if (phead->cdi != DA7A_CONTAINER_CDI_PAK)
        goto fatal;
#ifdef __BIG_ENDIAN__
    phead->offset = bswap_32(phead->offset);
    phead->bytes  = bswap_32(phead->bytes);
#endif
    return fp;

fatal:
    fclose(fp);
    return NULL;
}

/*
  \return Pointer to buffer which the caller must free(), or NULL if either
          malloc or fread fails.
*/
uint8_t* cdi_loadPakChunk(FILE* fp, const CDIEntry* ent)
{
    uint8_t* buf = (uint8_t*) malloc(ent->bytes);
    if (! buf)
        return NULL;
    if (fseek(fp, ent->offset, SEEK_SET) == 0) {
        size_t n = fread(buf, 1, ent->bytes, fp);
        if (n == ent->bytes)
            return buf;
    }
    free(buf);
    return NULL;
}

/*
  Initialize a CDIStringTable struct to point to the data in string table buf.

  \return Pointer to table or NULL if buf is not a valid string table.
*/
CDIStringTable* cdi_initStringTable(CDIStringTable* table, const uint8_t* buf)
{
    int form = buf[0];
    if (form > 2)
        return NULL;

    table->form = form;
    table->count = buf[1] << 16 | buf[2] << 8 | buf[3];
    table->strings = (char*) (buf+4);
    if (form) {
        table->strings += table->count * 2 * form;
        table->index.f1 = (uint16_t*) (buf+4);
#ifdef __BIG_ENDIAN__
        if (form == 1)
            cdi_swap16(table->index.f1, table->count);
        else
            cdi_swap32(table->index.f2, table->count);
#endif
    } else {
        table->index.f1 = NULL;
    }
    return table;
}

/*
  Read a CDI package Table of Contents.

  To obtain the number of entries in the table of contents use the
  CDI_TOC_SIZE macro with the package header.

  On big endian archetectures the CDIEntry offset and bytes values read from
  the file are swapped.

  \return Pointer to CDIEntry array which the caller must free(), or NULL if
          seek, malloc, or fread fails.
*/
CDIEntry* cdi_loadPakTOC(FILE* fp, const CDIEntry* header)
{
    CDIEntry* toc = (CDIEntry*) cdi_loadPakChunk(fp, header);
#ifdef __BIG_ENDIAN__
    if (toc) {
        CDIEntry* it  = toc;
        CDIEntry* end = toc + CDI_TOC_SIZE(header);
        while (it != end) {
            it->offset = bswap_32(it->offset);
            it->bytes  = bswap_32(it->bytes);
        }
    }
#endif
    return toc;
}

const CDIEntry* cdi_findAppId(const CDIEntry* toc, size_t count, uint32_t id)
{
    const CDIEntry* end = toc + count;
    for (; toc != end; ++toc) {
        if (toc->appId == id)
            return toc;
    }
    return NULL;
}

const CDIEntry* cdi_findFormat(const CDIEntry* toc, size_t count, uint32_t cdi)
{
    const CDIEntry* end = toc + count;
    for (; toc != end; ++toc) {
        if (toc->cdi == cdi)
            return toc;
    }
    return NULL;
}
