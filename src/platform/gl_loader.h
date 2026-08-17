#ifndef GL_LOADER_H
#define GL_LOADER_H

#include <stddef.h>

#ifndef __APPLE__
#include <GL/gl.h>
#else
#include <OpenGL/gl.h>
#endif

#ifndef GL_COMPILE_STATUS
#define GL_BLEND_EQUATION_RGB               0x8009
#define GL_VERTEX_ATTRIB_ARRAY_ENABLED      0x8622
#define GL_VERTEX_ATTRIB_ARRAY_SIZE         0x8623
#define GL_VERTEX_ATTRIB_ARRAY_STRIDE       0x8624
#define GL_VERTEX_ATTRIB_ARRAY_TYPE         0x8625
#define GL_CURRENT_VERTEX_ATTRIB            0x8626
#define GL_VERTEX_PROGRAM_POINT_SIZE        0x8642
#define GL_VERTEX_ATTRIB_ARRAY_POINTER      0x8645
#define GL_STENCIL_BACK_FUNC                0x8800
#define GL_STENCIL_BACK_FAIL                0x8801
#define GL_STENCIL_BACK_PASS_DEPTH_FAIL     0x8802
#define GL_STENCIL_BACK_PASS_DEPTH_PASS     0x8803
#define GL_MAX_DRAW_BUFFERS                 0x8824
#define GL_DRAW_BUFFER0                     0x8825
#define GL_DRAW_BUFFER1                     0x8826
#define GL_DRAW_BUFFER2                     0x8827
#define GL_DRAW_BUFFER3                     0x8828
#define GL_DRAW_BUFFER4                     0x8829
#define GL_DRAW_BUFFER5                     0x882A
#define GL_DRAW_BUFFER6                     0x882B
#define GL_DRAW_BUFFER7                     0x882C
#define GL_DRAW_BUFFER8                     0x882D
#define GL_DRAW_BUFFER9                     0x882E
#define GL_DRAW_BUFFER10                    0x882F
#define GL_DRAW_BUFFER11                    0x8830
#define GL_DRAW_BUFFER12                    0x8831
#define GL_DRAW_BUFFER13                    0x8832
#define GL_DRAW_BUFFER14                    0x8833
#define GL_DRAW_BUFFER15                    0x8834
#define GL_BLEND_EQUATION_ALPHA             0x883D
#define GL_MAX_VERTEX_ATTRIBS               0x8869
#define GL_VERTEX_ATTRIB_ARRAY_NORMALIZED   0x886A
#define GL_MAX_TEXTURE_IMAGE_UNITS          0x8872
#define GL_FRAGMENT_SHADER                  0x8B30
#define GL_VERTEX_SHADER                    0x8B31
#define GL_MAX_FRAGMENT_UNIFORM_COMPONENTS  0x8B49
#define GL_MAX_VERTEX_UNIFORM_COMPONENTS    0x8B4A
#define GL_MAX_VARYING_FLOATS               0x8B4B
#define GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS   0x8B4C
#define GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS 0x8B4D
#define GL_SHADER_TYPE                      0x8B4F
#define GL_FLOAT_VEC2                       0x8B50
#define GL_FLOAT_VEC3                       0x8B51
#define GL_FLOAT_VEC4                       0x8B52
#define GL_INT_VEC2                         0x8B53
#define GL_INT_VEC3                         0x8B54
#define GL_INT_VEC4                         0x8B55
#define GL_BOOL                             0x8B56
#define GL_BOOL_VEC2                        0x8B57
#define GL_BOOL_VEC3                        0x8B58
#define GL_BOOL_VEC4                        0x8B59
#define GL_FLOAT_MAT2                       0x8B5A
#define GL_FLOAT_MAT3                       0x8B5B
#define GL_FLOAT_MAT4                       0x8B5C
#define GL_SAMPLER_1D                       0x8B5D
#define GL_SAMPLER_2D                       0x8B5E
#define GL_SAMPLER_3D                       0x8B5F
#define GL_SAMPLER_CUBE                     0x8B60
#define GL_SAMPLER_1D_SHADOW                0x8B61
#define GL_SAMPLER_2D_SHADOW                0x8B62
#define GL_DELETE_STATUS                    0x8B80
#define GL_COMPILE_STATUS                   0x8B81
#define GL_LINK_STATUS                      0x8B82
#define GL_VALIDATE_STATUS                  0x8B83
#define GL_INFO_LOG_LENGTH                  0x8B84
#define GL_ATTACHED_SHADERS                 0x8B85
#define GL_ACTIVE_UNIFORMS                  0x8B86
#define GL_ACTIVE_UNIFORM_MAX_LENGTH        0x8B87
#define GL_SHADER_SOURCE_LENGTH             0x8B88
#define GL_ACTIVE_ATTRIBUTES                0x8B89
#define GL_ACTIVE_ATTRIBUTE_MAX_LENGTH      0x8B8A
#define GL_FRAGMENT_SHADER_DERIVATIVE_HINT  0x8B8B
#define GL_SHADING_LANGUAGE_VERSION         0x8B8C
#define GL_CURRENT_PROGRAM                  0x8B8D
#define GL_POINT_SPRITE_COORD_ORIGIN        0x8CA0
#define GL_LOWER_LEFT                       0x8CA1
#define GL_UPPER_LEFT                       0x8CA2
#define GL_STENCIL_BACK_REF                 0x8CA3
#define GL_STENCIL_BACK_VALUE_MASK          0x8CA4
#define GL_STENCIL_BACK_WRITEMASK           0x8CA5
#define GL_VERTEX_PROGRAM_TWO_SIDE          0x8643
#define GL_POINT_SPRITE                     0x8861
#define GL_COORD_REPLACE                    0x8862
#define GL_MAX_TEXTURE_COORDS               0x8871

typedef char GLchar;
typedef int	 GLsizei;
typedef ptrdiff_t GLintptr;
typedef ptrdiff_t GLsizeiptr;

#define GL_VERTEX_SHADER        0x8B31
#define GL_FRAGMENT_SHADER      0x8B30
#define GL_ARRAY_BUFFER         0x8892
#define GL_ELEMENT_ARRAY_BUFFER 0x8893
#define GL_STATIC_DRAW          0x88E4
#define GL_DYNAMIC_DRAW         0x88E8
#define GL_TEXTURE0             0x84C0
#endif


#define GL_PROC_DEF(proc, name) name##SRC = (name##PROC)proc(#name)

typedef void (*GLapiproc)(void);
typedef GLapiproc (*GLloadfunc)(const char *name);
typedef void (*glShaderSourcePROC) (GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length);
typedef GLuint (*glCreateShaderPROC) (GLenum type);
typedef void (*glCompileShaderPROC) (GLuint shader);
typedef GLuint (*glCreateProgramPROC) (void);
typedef void (*glAttachShaderPROC) (GLuint program, GLuint shader);
typedef void (*glBindAttribLocationPROC) (GLuint program, GLuint index, const GLchar *name);
typedef void (*glLinkProgramPROC) (GLuint program);
typedef void (*glBindBufferPROC) (GLenum target, GLuint buffer);
typedef void (*glBufferDataPROC) (GLenum target, GLsizeiptr size, const void *data, GLenum usage);
typedef void (*glEnableVertexAttribArrayPROC) (GLuint index);
typedef void (*glVertexAttribPointerPROC) (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer);
typedef void (*glVertexAttribIPointerPROC) (GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer);
typedef void (*glDisableVertexAttribArrayPROC) (GLuint index);
typedef void (*glDeleteBuffersPROC) (GLsizei n, const GLuint *buffers);
typedef void (*glDeleteVertexArraysPROC) (GLsizei n, const GLuint *arrays);
typedef void (*glUseProgramPROC) (GLuint program);
typedef void (*glDetachShaderPROC) (GLuint program, GLuint shader);
typedef void (*glDeleteShaderPROC) (GLuint shader);
typedef void (*glDeleteProgramPROC) (GLuint program);
typedef void (*glBufferSubDataPROC) (GLenum target, GLintptr offset, GLsizeiptr size, const void *data);
typedef void (*glGetShaderivPROC)(GLuint shader, GLenum pname, GLint *params);
typedef void (*glGetShaderInfoLogPROC)(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef void (*glGetProgramivPROC)(GLuint program, GLenum pname, GLint *params);
typedef void (*glGetProgramInfoLogPROC)(GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef void (*glGenVertexArraysPROC)(GLsizei n, GLuint *arrays);
typedef void (*glGenBuffersPROC)(GLsizei n, GLuint *buffers);
typedef void (*glBindVertexArrayPROC)(GLuint array);
typedef GLint (*glGetUniformLocationPROC)(GLuint program, const GLchar *name);
typedef void (*glUniformMatrix4fvPROC)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void (*glTexImage2DPROC)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels);
typedef void (*glActiveTexturePROC) (GLenum texture);
typedef void (*glDebugMessageCallbackPROC)(void* callback, const void*);
typedef void (*glDrawElementsPROC)(GLenum mode, GLsizei count, GLenum type, const void * indices);
typedef void (*glDrawBuffersPROC)(GLsizei n, const GLenum *bufs);
typedef void (*glClearPROC)(GLbitfield mask);
typedef void (*glClearColorPROC)(GLfloat, GLfloat, GLfloat, GLfloat);
typedef void (*glViewportPROC)(GLint, GLint, GLsizei, GLsizei);
typedef void (*glGetActiveAttribPROC)(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name);
typedef GLint (*glGetAttribLocationPROC)(GLuint program, const GLchar *name);
typedef void (*glGenerateMipmapPROC)(GLenum target);
typedef void (*glGenFramebuffersPROC)(GLsizei n, GLuint *ids);
typedef void (*glBindFramebufferPROC)(GLenum target, GLuint framebuffer);
typedef void (*glFramebufferTexture2DPROC)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
typedef GLenum (*glCheckFramebufferStatusPROC)(GLenum target);
typedef void (*glDeleteFramebuffersPROC)(GLsizei n, const GLuint *framebuffers);
typedef void (*glVertexAttribDivisorPROC)(GLuint index, GLuint divisor);
typedef GLuint (*glGetUniformBlockIndexPROC)(GLuint program, const GLchar *uniformBlockName);
typedef void (*glUniformBlockBindingPROC)(GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding);
typedef void (*glBindBufferRangePROC)(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size);
typedef void (*glUniform1ivPROC)(GLint location, GLsizei count, const GLint *value);
typedef void (*glBlendFuncSeparatePROC)(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha);
typedef void (*glBlendEquationSeparatePROC)(GLenum modeRGB, GLenum modeAlpha);
typedef void (*glDrawArraysInstancedPROC)(GLenum mode, GLint first, GLsizei count, GLsizei instancecount);

glShaderSourcePROC glShaderSourceSRC = NULL;
glCreateShaderPROC glCreateShaderSRC = NULL;
glCompileShaderPROC glCompileShaderSRC = NULL;
glCreateProgramPROC glCreateProgramSRC = NULL;
glAttachShaderPROC glAttachShaderSRC = NULL;
glBindAttribLocationPROC glBindAttribLocationSRC = NULL;
glLinkProgramPROC glLinkProgramSRC = NULL;
glBindBufferPROC glBindBufferSRC = NULL;
glBufferDataPROC glBufferDataSRC = NULL;
glEnableVertexAttribArrayPROC glEnableVertexAttribArraySRC = NULL;
glVertexAttribPointerPROC glVertexAttribPointerSRC = NULL;
glVertexAttribIPointerPROC glVertexAttribIPointerSRC = NULL;
glDisableVertexAttribArrayPROC glDisableVertexAttribArraySRC = NULL;
glDeleteBuffersPROC glDeleteBuffersSRC = NULL;
glDeleteVertexArraysPROC glDeleteVertexArraysSRC = NULL;
glUseProgramPROC glUseProgramSRC = NULL;
glDetachShaderPROC glDetachShaderSRC = NULL;
glDeleteShaderPROC glDeleteShaderSRC = NULL;
glDeleteProgramPROC glDeleteProgramSRC = NULL;
glBufferSubDataPROC glBufferSubDataSRC = NULL;
glGetShaderivPROC glGetShaderivSRC = NULL;
glGetShaderInfoLogPROC glGetShaderInfoLogSRC = NULL;
glGetProgramivPROC glGetProgramivSRC = NULL;
glGetProgramInfoLogPROC glGetProgramInfoLogSRC = NULL;
glGenVertexArraysPROC glGenVertexArraysSRC = NULL;
glGenBuffersPROC glGenBuffersSRC = NULL;
glBindVertexArrayPROC glBindVertexArraySRC = NULL;
glGetUniformLocationPROC glGetUniformLocationSRC = NULL;
glUniformMatrix4fvPROC glUniformMatrix4fvSRC = NULL;
glActiveTexturePROC glActiveTextureSRC = NULL;
glDebugMessageCallbackPROC glDebugMessageCallbackSRC = NULL;
glDrawElementsPROC glDrawElementsSRC = NULL;
glDrawBuffersPROC glDrawBuffersSRC = NULL;
glClearPROC glClearSRC = NULL;
glClearColorPROC glClearColorSRC = NULL;
glViewportPROC glViewportSRC = NULL;
glGetActiveAttribPROC glGetActiveAttribSRC = NULL;
glGetAttribLocationPROC glGetAttribLocationSRC = NULL;
glGenerateMipmapPROC glGenerateMipmapSRC = NULL;
glGenFramebuffersPROC glGenFramebuffersSRC = NULL;
glBindFramebufferPROC glBindFramebufferSRC = NULL;
glFramebufferTexture2DPROC glFramebufferTexture2DSRC = NULL;
glCheckFramebufferStatusPROC glCheckFramebufferStatusSRC = NULL;
glDeleteFramebuffersPROC glDeleteFramebuffersSRC = NULL;
glVertexAttribDivisorPROC glVertexAttribDivisorSRC = NULL;
glGetUniformBlockIndexPROC glGetUniformBlockIndexSRC = NULL;
glUniformBlockBindingPROC glUniformBlockBindingSRC = NULL;
glBindBufferRangePROC glBindBufferRangeSRC = NULL;
glUniform1ivPROC glUniform1ivSRC = NULL;
glBlendFuncSeparatePROC glBlendFuncSeparateSRC = NULL;
glBlendEquationSeparatePROC glBlendEquationSeparateSRC = NULL;
glDrawArraysInstancedPROC glDrawArraysInstancedSRC = NULL;

#define glActiveTexture glActiveTextureSRC
#define glShaderSource glShaderSourceSRC
#define glCreateShader glCreateShaderSRC
#define glCompileShader glCompileShaderSRC
#define glCreateProgram glCreateProgramSRC
#define glAttachShader glAttachShaderSRC
#define glBindAttribLocation glBindAttribLocationSRC
#define glLinkProgram glLinkProgramSRC
#define glBindBuffer glBindBufferSRC
#define glBufferData glBufferDataSRC
#define glEnableVertexAttribArray glEnableVertexAttribArraySRC
#define glVertexAttribPointer glVertexAttribPointerSRC
#define glVertexAttribIPointer glVertexAttribIPointerSRC
#define glDisableVertexAttribArray glDisableVertexAttribArraySRC
#define glDeleteBuffers glDeleteBuffersSRC
#define glDeleteVertexArrays glDeleteVertexArraysSRC
#define glUseProgram glUseProgramSRC
#define glDetachShader glDetachShaderSRC
#define glDeleteShader glDeleteShaderSRC
#define glDeleteProgram glDeleteProgramSRC
#define glBufferSubData glBufferSubDataSRC
#define glGetShaderiv glGetShaderivSRC
#define glGetShaderInfoLog glGetShaderInfoLogSRC
#define glGetProgramiv glGetProgramivSRC
#define glGetProgramInfoLog glGetProgramInfoLogSRC
#define glGenVertexArrays glGenVertexArraysSRC
#define glGenBuffers glGenBuffersSRC
#define glBindVertexArray glBindVertexArraySRC
#define glGetUniformLocation glGetUniformLocationSRC
#define glUniformMatrix4fv glUniformMatrix4fvSRC
#define glDebugMessageCallback glDebugMessageCallbackSRC
#define glDrawElements glDrawElementsSRC
#define glDrawBuffers glDrawBuffersSRC
#define glClear glClearSRC
#define glClearColor glClearColorSRC
#define glViewport glViewportSRC
#define glGetActiveAttrib glGetActiveAttribSRC
#define glGetAttribLocation glGetAttribLocationSRC
#define glGenerateMipmap glGenerateMipmapSRC
#define glGenFramebuffers glGenFramebuffersSRC
#define glBindFramebuffer glBindFramebufferSRC
#define glFramebufferTexture2D glFramebufferTexture2DSRC
#define glCheckFramebufferStatus glCheckFramebufferStatusSRC
#define glDeleteFramebuffers glDeleteFramebuffersSRC
#define glVertexAttribDivisor glVertexAttribDivisorSRC
#define glGetUniformBlockIndex glGetUniformBlockIndexSRC
#define glUniformBlockBinding glUniformBlockBindingSRC
#define glBindBufferRange glBindBufferRangeSRC
#define glBlendFuncSeparate glBlendFuncSeparateSRC
#define glBlendEquationSeparate glBlendEquationSeparateSRC
#define glDrawArraysInstanced glDrawArraysInstancedSRC

#define glUniform1iv glUniform1ivSRC

extern int GL_loadGL(GLloadfunc proc);

#include <stdio.h>

const GLubyte *gluErrorString(GLenum error);

#if 1
int GL_loadGL(GLloadfunc proc) {
    GL_PROC_DEF(proc, glShaderSource);
    GL_PROC_DEF(proc, glCreateShader);
    GL_PROC_DEF(proc, glCompileShader);
    GL_PROC_DEF(proc, glCreateProgram);
    GL_PROC_DEF(proc, glAttachShader);
    GL_PROC_DEF(proc, glBindAttribLocation);
    GL_PROC_DEF(proc, glLinkProgram);
    GL_PROC_DEF(proc, glBindBuffer);
    GL_PROC_DEF(proc, glBufferData);
    GL_PROC_DEF(proc, glEnableVertexAttribArray);
    GL_PROC_DEF(proc, glVertexAttribPointer);
    GL_PROC_DEF(proc, glVertexAttribIPointer);
    GL_PROC_DEF(proc, glDisableVertexAttribArray);
    GL_PROC_DEF(proc, glDeleteBuffers);
    GL_PROC_DEF(proc, glDeleteVertexArrays);
    GL_PROC_DEF(proc, glUseProgram);
    GL_PROC_DEF(proc, glDetachShader);
    GL_PROC_DEF(proc, glDeleteShader);
    GL_PROC_DEF(proc, glDeleteProgram);
    GL_PROC_DEF(proc, glBufferSubData);
    GL_PROC_DEF(proc, glGetShaderiv);
    GL_PROC_DEF(proc, glGetShaderInfoLog);
    GL_PROC_DEF(proc, glGetProgramiv);
    GL_PROC_DEF(proc, glGetProgramInfoLog);
    GL_PROC_DEF(proc, glGenVertexArrays);
    GL_PROC_DEF(proc, glGenBuffers);
    GL_PROC_DEF(proc, glBindVertexArray);
    GL_PROC_DEF(proc, glGetUniformLocation);
    GL_PROC_DEF(proc, glUniformMatrix4fv);
    GL_PROC_DEF(proc, glActiveTexture);
    GL_PROC_DEF(proc, glDebugMessageCallback);
    GL_PROC_DEF(proc, glDrawElements);
    GL_PROC_DEF(proc, glDrawBuffers);
    GL_PROC_DEF(proc, glClear);
    GL_PROC_DEF(proc, glClearColor);
    GL_PROC_DEF(proc, glGetActiveAttrib);
    GL_PROC_DEF(proc, glGetAttribLocation);
    GL_PROC_DEF(proc, glGenerateMipmap);

    GL_PROC_DEF(proc, glGenFramebuffers);
    GL_PROC_DEF(proc, glBindFramebuffer);
    GL_PROC_DEF(proc, glFramebufferTexture2D);
    GL_PROC_DEF(proc, glCheckFramebufferStatus);
    GL_PROC_DEF(proc, glDeleteFramebuffers);

    GL_PROC_DEF(proc, glVertexAttribDivisor);

    GL_PROC_DEF(proc, glGetUniformBlockIndex);
    GL_PROC_DEF(proc, glUniformBlockBinding);
    GL_PROC_DEF(proc, glBindBufferRange);

    GL_PROC_DEF(proc, glUniform1iv);
    GL_PROC_DEF(proc, glViewport);
    GL_PROC_DEF(proc, glBlendFuncSeparate);
    GL_PROC_DEF(proc, glBlendEquationSeparate);
    GL_PROC_DEF(proc, glDrawArraysInstanced);
    if (
        // TODO make this with multiple cursors out of all the defines
        glShaderSourceSRC == NULL ||
        glCreateShaderSRC == NULL ||
        glCompileShaderSRC == NULL ||
        glCreateProgramSRC == NULL ||
        glAttachShaderSRC == NULL ||
        glBindAttribLocationSRC == NULL ||
        glLinkProgramSRC == NULL ||
        glBindBufferSRC == NULL ||
        glBufferDataSRC == NULL ||
        glEnableVertexAttribArraySRC == NULL ||
        glVertexAttribPointerSRC == NULL ||
        glVertexAttribIPointerSRC == NULL ||
        glDisableVertexAttribArraySRC == NULL ||
        glDeleteBuffersSRC == NULL ||
        glDeleteVertexArraysSRC == NULL ||
        glUseProgramSRC == NULL ||
        glDetachShaderSRC == NULL ||
        glDeleteShaderSRC == NULL ||
        glDeleteProgramSRC == NULL ||
        glBufferSubDataSRC == NULL ||
        glGetShaderivSRC == NULL ||
        glGetShaderInfoLogSRC == NULL ||
        glGetProgramivSRC == NULL ||
        glGetProgramInfoLogSRC == NULL ||
        glGenVertexArraysSRC == NULL ||
        glGenBuffersSRC == NULL ||
        glBindVertexArraySRC == NULL ||
        glGetUniformLocationSRC == NULL ||
        glUniformMatrix4fvSRC == NULL ||
        glBlendFuncSeparateSRC == NULL ||
        glBlendEquationSeparateSRC == NULL ||
        glDrawArraysInstancedSRC == NULL ||
        glClearSRC == NULL ||
        glClearColorSRC == NULL ||
        glViewportSRC == NULL
	)
        return 1;

    GLuint vao;
    glGenVertexArraysSRC(1, &vao);

    if (vao == 0)
        return 1;

    glDeleteVertexArraysSRC(1, &vao);
    return 0;
}

#endif
#endif
