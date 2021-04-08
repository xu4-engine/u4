#ifdef __APPLE__
#include <OpenGL/gl.h>
#elif defined(__ANDROID__)
#include <GLES3/gl31.h>
#else
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#endif

struct OpenGLResources {
    GLuint screenTex;
    GLuint quadVBO;
    GLuint shader;
    GLuint vao;
    GLint  slocTrans;
    GLint  slocCmap;
    GLint  slocTint;
};
