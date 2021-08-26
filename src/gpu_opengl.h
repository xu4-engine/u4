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

enum GLObject {
    GLOB_DRAW_LIST0,
    GLOB_DRAW_LIST1,
    GLOB_QUAD,
    GLOB_MAP_CHUNK0,
    GLOB_MAP_CHUNK1,
    GLOB_MAP_CHUNK2,
    GLOB_MAP_CHUNK3,
    GLOB_COUNT
};

enum GLTextureUnit {
    GTU_CMAP,
    GTU_SHADOW,
    GTU_SCALER_LUT
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

    GLuint shadeColor;
    GLint  slocTrans;
    GLint  slocCmap;
    GLint  slocTint;
    GLint  slocScroll;

    GLuint shadeWorld;
    GLint  worldTrans;
    GLint  worldCmap;
    GLint  worldShadowMap;
    GLint  worldScroll;

    GLuint tilesTex;            // Managed by user.
    float  tilesVDim;
    float  time;
    int    dbuf;
    float* dptr;
    int    blockCount;
    GLsizei mapChunkVertCount;
    uint16_t mapChunkDim;       // Size in tiles (width & height are the same).
    uint16_t mapChunkLoc[4];    // Chunk X,Y of associated GLOB_MAP_CHUNK.
};
