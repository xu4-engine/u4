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

enum GLObject {
    GLOB_DRAW_LIST0,
    GLOB_DRAW_LIST1,
    GLOB_FX_LIST0,
    GLOB_FX_LIST1,
    GLOB_MAPFX_LIST0,
    GLOB_MAPFX_LIST1,
    GLOB_QUAD,
    GLOB_MAP_CHUNK0,
    GLOB_MAP_CHUNK1,
    GLOB_MAP_CHUNK2,
    GLOB_MAP_CHUNK3,
    GLOB_COUNT
};

enum GLTextureUnit {
    GTU_CMAP,
    GTU_MATERIAL,
    GTU_SHADOW,
    GTU_SCALER_LUT
};

struct DrawList {
    int     buf;        // GLObject vbo index toggle.
    int     byteSize;
    GLsizei count;      // Number of floats.
};

#define CHUNK_FX_LIMIT  8

struct MapFx {
    float x, y, w, h;
    float u, v, u2, v2;
    AnimId anim;
};

struct OpenGLResources {
    GLuint screenTex;
    GLuint shadowTex;
    GLuint shadowFbo;
    GLuint vbo[ GLOB_COUNT ];
    GLuint vao[ GLOB_COUNT ];

    GLuint scalerLut;
    GLuint scaler;
    GLint  slocScMat;
    GLint  slocScDim;
    GLint  slocScTex;
    GLint  slocScLut;

    GLuint shadow;
    GLint  shadowTrans;
    GLint  shadowVport;
    GLint  shadowViewer;
    GLint  shadowCounts;
    GLint  shadowShapes;

    GLuint shadeSolid;
    GLint  solidTrans;
    GLint  solidColor;

    GLuint shadeColor;
    GLint  slocTrans;
    GLint  slocTint;
    GLint  slocScroll;

    GLuint shadeWorld;
    GLint  worldTrans;
    GLint  worldShadowMap;
    GLint  worldScroll;

    GLuint tilesTex;            // Managed by user.
    GLuint tilesMat;            // Managed by user.
    float  tilesVDim;
    float  time;
    DrawList dl[3];
    float* dptr;
    int    blockCount;
    GLsizei mapChunkVertCount;
    uint16_t mapChunkDim;       // Size in tiles (width & height are the same).
    uint16_t mapChunkId[4];     // Chunk X,Y of associated GLOB_MAP_CHUNK.
    uint16_t mapChunkFxUsed[4];
    MapFx mapChunkFx[4*CHUNK_FX_LIMIT];
};
