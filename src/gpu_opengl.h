#ifdef __APPLE__
#include <OpenGL/gl.h>
#elif defined(__ANDROID__)
#include <GLES3/gl31.h>
#elif defined(_WIN32)
#include "glad.h"
#else
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#endif

#include "anim.h"
#include "tile.h"

enum GLObject {
    GLOB_GUI_LIST,      // GPU_DLIST_GUI (single-buffered)
    GLOB_QUAD,          // No DrawList
    GLOB_HUD_LIST0,     // GPU_DLIST_HUD
    GLOB_HUD_LIST1,
#ifdef GPU_RENDER
    GLOB_DRAW_LIST0,    // GPU_DLIST_VIEW_OBJ
    GLOB_DRAW_LIST1,
    GLOB_FX_LIST0,      // GPU_DLIST_VIEW_FX
    GLOB_FX_LIST1,
    GLOB_MAPFX_LIST0,
    GLOB_MAPFX_LIST1,
    GLOB_MAP_CHUNK0,
    GLOB_MAP_CHUNK1,
    GLOB_MAP_CHUNK2,
    GLOB_MAP_CHUNK3,
#endif
    GLOB_COUNT
};

enum GLTextureUnit {
    GTU_CMAP,
    GTU_MATERIAL,
    GTU_NOISE,
    GTU_SHADOW,
    GTU_SCALER_LUT
};

struct DrawList {
    uint16_t bufI;      // GLObject vbo/vao index toggle.
    uint8_t  dual;      // Double-buffered.
    uint8_t  fpv;       // Floats per vertex.
    int     byteSize;
    GLsizei count;      // Number of floats.
};

#define CHUNK_FX_LIMIT  8

struct MapFx {
    float x, y, w, h;
    float u, v, u2, v2;
    AnimId anim;
};

#define TEXTURE_COUNT  6

struct OpenGLResources {
    GLuint screenTex;
    GLuint whiteTex;
    GLuint fontTex;
    GLuint guiTex;
    GLuint noiseTex;
    GLuint shadowTex;
    GLuint shadowFbo;
    GLuint vbo[ GLOB_COUNT ];
    GLuint vao[ GLOB_COUNT ];

    float guiTexSize[2];

    GLuint scalerLut;
    GLuint scaler;
    GLint  slocScMat;
    GLint  slocScDim;

    GLuint shadeColor;
    GLint  slocTrans;
    GLint  slocTint;

    GLuint shadeSolid;
    GLint  solidTrans;
    GLint  solidColor;

    GLuint shadeGlyph;
    GLint  glyphTrans;
    GLint  glyphOrigin;
    GLint  glyphWidget;
    GLint  glyphFg;

#ifdef GPU_RENDER
    GLuint shadow;
    GLint  shadowTrans;
    GLint  shadowVport;
    GLint  shadowViewer;
    GLint  shadowCounts;
    GLint  shadowShapes;

    GLuint shadeWorld;
    GLint  worldTrans;
    GLint  worldShadowMap;
    GLint  worldScroll;

    GLuint tilesTex;            // Managed by user.
    GLuint tilesMat;            // Managed by user.
    float  tilesVDim;
    float  time;
    DrawList dl[5];
    float* dptr;
    const TileId* mapData;
    const TileRenderData* renderData;
    int    blockCount;
    GLsizei mapChunkVertCount;
    uint16_t mapW;
    uint16_t mapH;
    uint16_t mapChunkDim;       // Size in tiles (width & height are the same).
    uint16_t mapChunkId[4];     // Chunk X,Y of associated GLOB_MAP_CHUNK.
    uint16_t mapChunkFxUsed[4];
    MapFx mapChunkFx[4*CHUNK_FX_LIMIT];
#else
    DrawList dl[2];
    float* dptr;
#endif
};
