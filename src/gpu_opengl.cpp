/*
  XU4 OpenGL Renderer
  Copyright 2021 Karl Robillard

  This file is part of XU4.

  XU4 is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  XU4 is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with XU4.  If not, see <http://www.gnu.org/licenses/>.
*/

//#include "gpu_opengl.h"

#define LOC_POS     0
#define LOC_UV      1

const char* cmap_vertShader =
    "#version 330\n"
    "uniform mat4 transform;\n"
    "layout(location = 0) in vec3 position;\n"
    "layout(location = 1) in vec2 uv;\n"
    "out vec2 texCoord;\n"
    "void main() {\n"
    "  texCoord = uv;\n"
    "  gl_Position = transform * vec4(position, 1.0);\n"
    "}\n";

const char* cmap_fragShader =
    "#version 330\n"
    "uniform sampler2D cmap;\n"
    "uniform vec4 tint;\n"
    "in vec2 texCoord;\n"
    "out vec4 fragColor;\n"
    "void main() {\n"
    "  vec4 texel = texture(cmap, texCoord);\n"
    "  fragColor = tint * texel;\n"
    "}\n";

static const float unitMatrix[16] = {
    1.0, 0.0, 0.0, 0.0,
    0.0, 1.0, 0.0, 0.0,
    0.0, 0.0, 1.0, 0.0,
    0.0, 0.0, 0.0, 1.0
};

#define ATTR_STRIDE     (sizeof(float)*5)
static const float quadAttr[] = {
    // X   Y   Z       U  V
   -1.0,-1.0, 0.0,   0.0, 1.0,
    1.0,-1.0, 0.0,   1.0, 1.0,
    1.0, 1.0, 0.0,   1.0, 0.0,
    1.0, 1.0, 0.0,   1.0, 0.0,
   -1.0, 1.0, 0.0,   0.0, 0.0,
   -1.0,-1.0, 0.0,   0.0, 1.0
};


static void printInfoLog(GLuint obj, int prog)
{
    GLint infologLength;
    GLint charsWritten;
    char* infoLog;

    if( prog )
        glGetProgramiv( obj, GL_INFO_LOG_LENGTH, &infologLength );
    else
        glGetShaderiv( obj, GL_INFO_LOG_LENGTH, &infologLength );

    if( infologLength > 0 ) {
        infoLog = (char*) malloc( infologLength );

        if( prog )
            glGetProgramInfoLog( obj, infologLength, &charsWritten, infoLog );
        else
            glGetShaderInfoLog( obj, infologLength, &charsWritten, infoLog );

        fprintf(stderr, "%s\n", infoLog);
        free( infoLog );
    } else {
        fprintf(stderr, "%s failed\n", prog ? "glLinkProgram"
                                            : "glCompileShader");
    }
}

/*
 * Returns zero on success or 1-3 to indicate compile/link error.
 */
static int compileShaders(GLuint program, const char* vert, const char* frag)
{
    GLint ok;
    GLuint vobj = glCreateShader(GL_VERTEX_SHADER);
    GLuint fobj = glCreateShader(GL_FRAGMENT_SHADER);

    glShaderSource(vobj, 1, (const GLchar**) &vert, 0);
    glCompileShader(vobj);
    glGetShaderiv(vobj, GL_COMPILE_STATUS, &ok);
    if (! ok) {
        printInfoLog(vobj, 0);
        return 1;
    }

    glShaderSource(fobj, 1, (const GLchar**) &frag, 0);
    glCompileShader(fobj);
    glGetShaderiv(fobj, GL_COMPILE_STATUS, &ok);
    if (! ok) {
        printInfoLog(fobj, 0);
        return 2;
    }

    glAttachShader(program, vobj);
    glAttachShader(program, fobj);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (! ok)
        printInfoLog(program, 1);

    // These will actually go away when the program is deleted.
    glDeleteShader(vobj);
    glDeleteShader(fobj);

    return ok ? 0 : 3;
}

bool gpu_init(void* res, int w, int h)
{
    OpenGLResources* gr = (OpenGLResources*) res;

    // Create screen texture.
    glGenTextures(1, &gr->screenTex);
    glBindTexture(GL_TEXTURE_2D, gr->screenTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);


    // Set default state.
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_BLEND);
    glViewport(0, 0, w, h);


    // Create colormap shader.
    gr->shader = glCreateProgram();
    if (compileShaders(gr->shader, cmap_vertShader, cmap_fragShader))
        return false;

    gr->slocTrans = glGetUniformLocation(gr->shader, "transform");
    gr->slocCmap  = glGetUniformLocation(gr->shader, "cmap");
    gr->slocTint  = glGetUniformLocation(gr->shader, "tint");

    glUseProgram(gr->shader);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gr->screenTex);

    // Set default uniform values.
    glUniformMatrix4fv(gr->slocTrans, 1, GL_FALSE, unitMatrix);
    glUniform1i(gr->slocCmap, 0);
    glUniform4f(gr->slocTint, 1.0, 1.0, 1.0, 1.0);


    // Create quad geometry.
    glGenBuffers(1, &gr->quadVBO);
    glBindBuffer(GL_ARRAY_BUFFER, gr->quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadAttr), quadAttr, GL_STATIC_DRAW);


    // Define vertex attribute layout.
    glGenVertexArrays(1, &gr->vao);
    glBindVertexArray(gr->vao);
    glBindBuffer(GL_ARRAY_BUFFER, gr->quadVBO);
    glEnableVertexAttribArray(LOC_POS);
    glVertexAttribPointer(LOC_POS, 3, GL_FLOAT, GL_FALSE, ATTR_STRIDE, 0);
    glEnableVertexAttribArray(LOC_UV);
    glVertexAttribPointer(LOC_UV,  2, GL_FLOAT, GL_FALSE, ATTR_STRIDE,
                          (const GLvoid*) 12);

    return true;
}

void gpu_free(void* res)
{
    OpenGLResources* gr = (OpenGLResources*) res;

    glDeleteVertexArrays(1, &gr->vao);
    glDeleteBuffers(1, &gr->quadVBO);
    glDeleteProgram(gr->shader);
    glDeleteTextures(1, &gr->screenTex);
}

uint32_t gpu_makeTexture(Image* img)
{
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img->width(), img->height(),
                 0, GL_RGBA, GL_UNSIGNED_BYTE, img->pixelData());
    return tex;
}
