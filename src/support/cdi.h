/*
  Concise Data Identifer
  Version 0.2
*/

#include <stdint.h>
#include <stdio.h>

#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define __BIG_ENDIAN__  1
#endif

#ifdef __BIG_ENDIAN__
#define CDI32(a,b,c,d)   (((uint32_t) d) | c<<8 | b<<16 | a<<24)
#else
#define CDI32(a,b,c,d)   (((uint32_t) a) | b<<8 | c<<16 | d<<24)
#endif

#define CDI_MASK_CATEGORY           CDI32(0x00,0x00,0xF0,0x00)
#define CDI_MASK_SELECTOR           CDI32(0x00,0x00,0x0F,0xFF)
#define CDI_MASK_FORMAT             CDI32(0x00,0x00,0xFF,0xFF)

#define CDI_TEXT                    CDI32(0x00,0x00,0x00,0x00)
#define CDI_IMAGE                   CDI32(0x00,0x00,0x10,0x00)
#define CDI_AUDIO                   CDI32(0x00,0x00,0x20,0x00)
#define CDI_VIDEO                   CDI32(0x00,0x00,0x30,0x00)
#define CDI_BYTECODE                CDI32(0x00,0x00,0x40,0x00)
#define CDI_GEOMETRY                CDI32(0x00,0x00,0x50,0x00)
#define CDI_ANIMATION               CDI32(0x00,0x00,0x60,0x00)
#define CDI_CONTAINER               CDI32(0x00,0x00,0x70,0x00)
#define CDI_ARCHIVE                 CDI32(0x00,0x00,0x80,0x00)
#define CDI_PRIVATE                 CDI32(0x00,0x00,0xF0,0x00)

#define DA7A_TEXT_ASCII             CDI32(0xDA,0x7A,0x00,0x00)
#define DA7A_TEXT_UTF8              CDI32(0xDA,0x7A,0x00,0x01)
#define DA7A_TEXT_HTML              CDI32(0xDA,0x7A,0x00,0x02)
#define DA7A_TEXT_MARKDOWN          CDI32(0xDA,0x7A,0x00,0x03)
#define DA7A_TEXT_XML               CDI32(0xDA,0x7A,0x00,0x04)
#define DA7A_TEXT_JSON              CDI32(0xDA,0x7A,0x00,0x05)
#define DA7A_TEXT_STRING_TABLE      CDI32(0xDA,0x7A,0x00,0x06)
#define DA7A_TEXT_BORON             CDI32(0xDA,0x7A,0x00,0x07)

#define DA7A_IMAGE_PPM              CDI32(0xDA,0x7A,0x10,0x00)
#define DA7A_IMAGE_PBM              CDI32(0xDA,0x7A,0x10,0x01)
#define DA7A_IMAGE_PNG              CDI32(0xDA,0x7A,0x10,0x02)
#define DA7A_IMAGE_JPEG             CDI32(0xDA,0x7A,0x10,0x03)
#define DA7A_IMAGE_SVG              CDI32(0xDA,0x7A,0x10,0x04)
#define DA7A_IMAGE_GIF              CDI32(0xDA,0x7A,0x10,0x05)
#define DA7A_IMAGE_RGBA8            CDI32(0xDA,0x7A,0x10,0x06)

#define DA7A_AUDIO_RAW_S8           CDI32(0xDA,0x7A,0x20,0x00)
#define DA7A_AUDIO_RAW_S16          CDI32(0xDA,0x7A,0x20,0x01)
#define DA7A_AUDIO_RAW_F32          CDI32(0xDA,0x7A,0x20,0x02)
#define DA7A_AUDIO_WAVE             CDI32(0xDA,0x7A,0x20,0x06)
#define DA7A_AUDIO_MP3              CDI32(0xDA,0x7A,0x20,0x07)
#define DA7A_AUDIO_OGG_VORBIS       CDI32(0xDA,0x7A,0x20,0x08)
#define DA7A_AUDIO_OGG_OPUS         CDI32(0xDA,0x7A,0x20,0x09)
#define DA7A_AUDIO_AAC              CDI32(0xDA,0x7A,0x20,0x10)
#define DA7A_AUDIO_FLAC             CDI32(0xDA,0x7A,0x20,0x11)
#define DA7A_AUDIO_IMPULSE_TRACKER  CDI32(0xDA,0x7A,0x20,0x20)
#define DA7A_AUDIO_RFX              CDI32(0xDA,0x7A,0x20,0x30)

#define DA7A_BYTECODE_BORON         CDI32(0xDA,0x7A,0x40,0x07)

#define DA7A_GEOMETRY_OBJ           CDI32(0xDA,0x7A,0x50,0x00)
#define DA7A_GEOMETRY_GLTF          CDI32(0xDA,0x7A,0x50,0x01)
#define DA7A_GEOMETRY_STL           CDI32(0xDA,0x7A,0x50,0x02)

#define DA7A_ANIMATION_IQM          CDI32(0xDA,0x7A,0x60,0x00)

#define DA7A_CONTAINER_CDI_PAK      CDI32(0xDA,0x7A,0x70,0x00)
#define DA7A_CONTAINER_CDI_CHUNK    CDI32(0xDA,0x7A,0x70,0x01)
#define DA7A_CONTAINER_IFF          CDI32(0xDA,0x7A,0x70,0x02)
#define DA7A_CONTAINER_RIFF         CDI32(0xDA,0x7A,0x70,0x03)
#define DA7A_CONTAINER_MP4          CDI32(0xDA,0x7A,0x70,0x04)

#define DA7A_ARCHIVE_ZIP            CDI32(0xDA,0x7A,0x80,0x00)
#define DA7A_ARCHIVE_LHA            CDI32(0xDA,0x7A,0x80,0x01)
#define DA7A_ARCHIVE_BZ2            CDI32(0xDA,0x7A,0x80,0x02)


#define CDI_COUNT24(bp)     (((int)bp[1])<<16 | ((int)bp[2])<<8 | bp[3])
#define CDI_TOC_SIZE(head)  (head->bytes / sizeof(CDIEntry))

/* CDI package header and table of contents entry */
typedef struct {
    uint32_t cdi;
    uint32_t appId;
    uint32_t offset;
    uint32_t bytes;
} CDIEntry;

typedef struct {
    uint32_t form;
    uint32_t count;
    union {
        uint16_t* f1;
        uint32_t* f2;
    } index;
    const char* strings;
} CDIStringTable;

#ifdef __cplusplus
extern "C" {
#endif

FILE*           cdi_openPak(const char* filename, CDIEntry* header);
uint8_t*        cdi_loadPakChunk(FILE* fp, const CDIEntry* ent);
CDIEntry*       cdi_loadPakTOC(FILE* fp, const CDIEntry* header);
const CDIEntry* cdi_findAppId(const CDIEntry* toc, size_t count, uint32_t id);
const CDIEntry* cdi_findFormat(const CDIEntry* toc, size_t count, uint32_t cdi);
CDIStringTable* cdi_initStringTable(CDIStringTable* table, const uint8_t* buf);

void cdi_swap16(uint16_t* vars, size_t count);
void cdi_swap32(uint32_t* vars, size_t count);

#ifdef __cplusplus
}
#endif
