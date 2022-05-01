#include <stdint.h>

typedef struct TxfHeader TxfHeader;

struct TxfHeader {
    uint16_t texW;          // Atlas texture width
    uint16_t texH;          // Atlas texture height
    uint16_t glyphCount;    // Number of TxfGlyph structs following header
    uint16_t kernOffset;
    float    fontSize;      // Glyph pixels per EM
    float    pixelRange;    // SDF distance range in output pixels
    float    lineHeight;
    float    ascender;
    float    descender;
};

typedef struct {
    uint16_t code;
    uint16_t kernIndex;
    float advance;
    float emRect[4];
    float tcRect[4];
}
TxfGlyph;

typedef struct TxfDrawState TxfDrawState;

struct TxfDrawState {
    const TxfHeader* tf;
    const uint8_t* (*lowChar)(TxfDrawState*, const uint8_t*, const uint8_t*);
    TxfGlyph* prev;
    float prScale;
    float x;
    float y;
    float psize;
    float lineSpacing;
    float marginL;
    float marginR;
    int emitTris;
};

#ifdef __cplusplus
extern "C" {
#endif

void txf_begin(TxfDrawState* ds, const TxfHeader* tf, float pointSize,
               float x, float y);
void txf_setFontSize(TxfDrawState* ds, float pointSize);
int  txf_genText(TxfDrawState* ds, float* uvs, float* vertex, int stride,
                 const uint8_t* it, unsigned int len);
float txf_emWidth(const TxfHeader*, const uint8_t* it, unsigned int len);
void  txf_emSize(const TxfHeader*, const uint8_t* it, unsigned int len,
                 float* size);

#ifdef __cplusplus
}
#endif
