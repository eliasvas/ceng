/* 
ogl - a minimal OpenGL abstraction layer - eliasvas

This is a single-file library that provides a more sane
API (no state cache) than OpenGL but maintains OpenGL compatibility.

Make sure OGL_IMPLEMENTATION is on exactly one .c file where you do:
  #define OGL_IMPLEMENTATION
  #include "ogl.h"

DESCRIPTION
The whole idea behind this utility is that we bind everything up-front
just before rendering. This means that we don't need to use the OpenGL 
state machine. This makes the code slower theoretically, but GL drivers
have gotten crazy good at this sort of thing, so it doesn't really matter.
If you want performance OpenGL isn't the correct API either way.

GL-VERSION-COMPATIBILITY
As can be seen on the sample, all shaders should start with #version 300 es.
This is intentional, as OpenGLES shaders are the most compatible (Desktop + Web).
Generally you should make GL4_3COMPAT context for Desktop and ES3_0 for Emscripten.

HOW-TO
The API resolves around a structure called Ogl_Render_Bundle.
This is a big bundle of state, holding the shader program, 
vbos, ubos, viewport, scissor, depth state, EVERYTHING.
Only thing the user needs is to provide the correct attachments to this
bundle before rendering, typically via ogl_render_bundle_draw(..) call.
Also don't forget to call ogl_init() before using the API.

SAMPLE (w/ freeglut on Linux)
  -Dependencies: sudo dnf install freeglut freeglut-devel -y
  -CompileRun: gcc -std=gnu23 main.c -lglut -lGL -lGLU -o main && ./main
#if 0
#define GL_GLEXT_PROTOTYPES
#include <GL/freeglut.h>
#define OGL_IMPLEMENTATION
#include "ogl.h"
void display() {
  ogl_clear();
  Ogl_Render_Bundle rbundle = (Ogl_Render_Bundle){
    .sp = ogl_shader_make( R"(#version 300 es
      precision highp float;
      layout (location = 0) in vec3 v_pos;
      layout (location = 1) in vec3 v_col;
      layout (location = 2) in vec2 v_tc;
      out vec3 f_color;
      out vec2 f_tc;
      layout (std140) uniform UboExample { vec4 ubo_color_mult; };
      void main() { gl_Position = vec4(v_pos, 1.0f); f_color = v_col; f_tc = v_tc; }
      )", R"(#version 300 es
      precision highp float;
      layout(location = 0) out vec4 out_color;
      in vec3 f_color;
      in vec2 f_tc;
      uniform sampler2D tex;
      layout (std140) uniform UboExample { vec4 ubo_color_mult; };
      void main() { out_color = vec4(f_color, 1.0) * ubo_color_mult * texture2D(tex, f_tc); }
      )"),
    .vbos = {
      [0] = {
        .buffer = ogl_buf_make(OGL_BUF_KIND_VERTEX, OGL_BUF_HINT_STATIC, (float[]) {
              -0.5f,  0.5f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f,   // top left
              -0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,   // bottom left
               0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,   // bottom right
               0.5f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f,   // top right
            }, 4, 8*sizeof(float)),
        .vattribs = {
          [0] = { .location = 0, .type = OGL_DATA_TYPE_VEC3, .offset = 0*sizeof(float) },
          [1] = { .location = 1, .type = OGL_DATA_TYPE_VEC3, .offset = 3*sizeof(float) },
          [2] = { .location = 2, .type = OGL_DATA_TYPE_VEC2, .offset = 6*sizeof(float) },
        },
      },
    },
    .ubos = {
      [0] = { .name = "UboExample", .buffer = ogl_buf_make(OGL_BUF_KIND_UNIFORM, OGL_BUF_HINT_DYNAMIC, (float[]) { 1,1,1,1 }, 1, sizeof(float)*4), .start_offset = 0, .size = sizeof(float)*4 },
    },
    .textures = {
      [0] = { .name = "tex", .tex = ogl_tex_make((uint8_t[]){200,40,40,255}, 1,1, OGL_TEX_FORMAT_R8U, (Ogl_Tex_Params){.wrap_s = OGL_TEX_WRAP_MODE_REPEAT}),},
    },
    .depth_state = (Ogl_Depth_State) {
      .dwrite = OGL_DEPTH_WRITE_ENABLED,
      .dfunc  = OGL_DFUNC_GEQUAL,
    },
    .dyn_state = (Ogl_Dyn_State){
      .viewport = {0,0,800,600},
    }
  };
  // You can set whatever state you want dynamically e.g viewport
  rbundle.dyn_state.viewport = (Ogl_rect){0,0,800,600};
  ogl_render_bundle_draw(&rbundle, OGL_PRIM_TYPE_TRIANGLE_FAN, 4, 1);
  glutSwapBuffers();
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitContextVersion(4, 3);
    glutInitContextProfile(GLUT_COMPATIBILITY_PROFILE);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(800, 600);
    glutCreateWindow("ogl window");

    ogl_init();
    
    glutDisplayFunc(display);
    glutMainLoop();
    return 0;
}
#endif
*/

#ifndef OGL_H__
#define OGL_H__

#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif
  

typedef enum {
  OGL_PRIM_TYPE_POINT,
  OGL_PRIM_TYPE_LINE,
  OGL_PRIM_TYPE_LINE_STRIP,
  OGL_PRIM_TYPE_LINE_LOOP,
  OGL_PRIM_TYPE_TRIANGLE,
  OGL_PRIM_TYPE_TRIANGLE_STRIP,
  OGL_PRIM_TYPE_TRIANGLE_FAN,
}Ogl_Prim_Type;

typedef enum {
  OGL_BUF_KIND_VERTEX,
  OGL_BUF_KIND_INDEX,
  OGL_BUF_KIND_UNIFORM,
}Ogl_Buf_Kind;

#define OGL_MAX_VERTEX_BUFFERS 8
#define OGL_MAX_ATTRIBS 16
#define OGL_MAX_UNIFORM_BUFFERS 4
#define OGL_MAX_ACTIVE_TEXTURES 4
#define OGL_MAX_RENDER_TARGET_ATTACHMENTS 4

typedef enum {
  OGL_BUF_HINT_STATIC, // Data set once
  OGL_BUF_HINT_DYNAMIC, // Data updated occasionally
  OGL_BUF_HINT_STREAM, // Data updated after every use
} Ogl_Buf_Hint;

typedef struct {
  int64_t bytes_per_elem;
  int64_t count;
  Ogl_Buf_Kind kind;

  uint32_t impl_state;
} Ogl_Buf;

typedef enum {
  OGL_DATA_TYPE_UNKNOWN,
  OGL_DATA_TYPE_FLOAT,
  OGL_DATA_TYPE_INT,
  OGL_DATA_TYPE_IVEC2,
  OGL_DATA_TYPE_IVEC3,
  OGL_DATA_TYPE_IVEC4,
  OGL_DATA_TYPE_VEC2,
  OGL_DATA_TYPE_VEC3,
  OGL_DATA_TYPE_VEC4,
  OGL_DATA_TYPE_MAT4,
} Ogl_Data_Type;

typedef struct {
  // Description
  uint32_t location;
  Ogl_Data_Type type;
  // Extra
  uint64_t offset;
  uint32_t stride;
  bool instanced;
} Ogl_Vert_Attrib;

typedef enum {
  OGL_TEX_FILTER_NEAREST,
  OGL_TEX_FILTER_LINEAR,
} Ogl_Tex_Filter;

// TODO: clamp to border would be GREAT but webgl2 doesnt support it
typedef enum {
  OGL_TEX_WRAP_MODE_CLAMP_TO_EDGE,
  OGL_TEX_WRAP_MODE_REPEAT,
  OGL_TEX_WRAP_MODE_MIRRORED_REPEAT,
} Ogl_Tex_Wrap_Mode;

typedef struct {
  Ogl_Tex_Filter min_filter;
  Ogl_Tex_Filter mag_filter;

  Ogl_Tex_Wrap_Mode wrap_s;
  Ogl_Tex_Wrap_Mode wrap_t;
  Ogl_Tex_Wrap_Mode wrap_r;

  bool is_depth;
  bool generate_mips;
} Ogl_Tex_Params;

typedef enum {
  OGL_TEX_FORMAT_R32F,
  OGL_TEX_FORMAT_R8U,
  OGL_TEX_FORMAT_RGBA8U,
  OGL_TEX_FORMAT_RGBA32F,
} Ogl_Tex_Format;

typedef struct {
  uint32_t width;
  uint32_t height;
  Ogl_Tex_Format format;
  Ogl_Tex_Params params;
  uint64_t impl_state;
} Ogl_Tex;

typedef enum {
  OGL_DFUNC_ALWAYS,
  OGL_DFUNC_NEVER,
  OGL_DFUNC_LESS,
  OGL_DFUNC_EQUAL,
  OGL_DFUNC_LEQUAL,
  OGL_DFUNC_GREATER,
  OGL_DFUNC_NOTEQUAL,
  OGL_DFUNC_GEQUAL,
} Ogl_Depth_Func;

typedef enum {
  OGL_DEPTH_WRITE_ENABLED,
  OGL_DEPTH_WRITE_DISABLED,
} Ogl_Depth_Write_State;

typedef struct {
  Ogl_Depth_Func dfunc;
  Ogl_Depth_Write_State dwrite;
} Ogl_Depth_State;

typedef struct {
  // This is kind-of a hack, we use Ogl_Vert_Attrib's and don't fill most fields
  Ogl_Vert_Attrib vattribs[OGL_MAX_ATTRIBS];

  uint64_t impl_state;
} Ogl_Shader;

typedef enum { 
  OGL_DYN_STATE_FLAG_SCISSOR    = (0x1 << 0),
  OGL_DYN_STATE_FLAG_BLEND      = (0x1 << 2),
} Ogl_Dyn_State_Flags;

typedef union {
  struct { float x,y,w,h; };
  float raw[4];
} Ogl_rect;

typedef struct {
  Ogl_rect viewport;
  Ogl_rect scissor;
  uint64_t flags;
} Ogl_Dyn_State;

typedef struct {
  const char *name;
  Ogl_Buf buffer;
  uint64_t start_offset;
  uint64_t size;
}Ogl_Uniform_Buffer_Slot;

typedef struct {
  Ogl_Buf buffer;
  Ogl_Vert_Attrib vattribs[OGL_MAX_ATTRIBS];
} Ogl_Vertex_Buffer_Slot;

typedef struct {
  const char *name;
  Ogl_Tex tex;
} Ogl_Tex_Slot;

typedef struct {
  Ogl_Tex attachments[OGL_MAX_RENDER_TARGET_ATTACHMENTS];
  Ogl_Tex depth_attachment;

  uint32_t width; 
  uint32_t height;
  uint64_t impl_state;
} Ogl_Render_Target;



// This is a dirty cache pretty much..
typedef struct {
  Ogl_Shader sp;

  Ogl_Buf index_buffer;

  Ogl_Vertex_Buffer_Slot  vbos[OGL_MAX_VERTEX_BUFFERS];
  Ogl_Uniform_Buffer_Slot ubos[OGL_MAX_UNIFORM_BUFFERS];
  Ogl_Tex_Slot            textures[OGL_MAX_ACTIVE_TEXTURES];

  Ogl_Render_Target rt;

  Ogl_Depth_State depth_state;
  Ogl_Dyn_State dyn_state;
} Ogl_Render_Bundle;

#ifndef OGL_IMPLEMENTATION

  void ogl_init();
  void ogl_clear();

  bool ogl_buf_update(Ogl_Buf *buf, uint64_t offset, void *data, uint32_t count, uint32_t bytes_per_elem);
  bool ogl_buf_init(Ogl_Buf *buf, Ogl_Buf_Kind kind, Ogl_Buf_Hint hint, void *data, uint32_t count, uint32_t bytes_per_elem);
  Ogl_Buf ogl_buf_make(Ogl_Buf_Kind kind, Ogl_Buf_Hint hint, void *data, uint32_t count, uint32_t bytes_per_elem);
  void ogl_buf_deinit(Ogl_Buf *buf);

  bool ogl_shader_init(Ogl_Shader *shader, const char* vertex_source, const char* fragment_source);
  Ogl_Shader ogl_shader_make(const char* vertex_source, const char* fragment_source);
  void ogl_shader_deinit(Ogl_Shader *shader);

  bool ogl_tex_init(Ogl_Tex *tex, uint8_t *data, uint32_t w, uint32_t h, Ogl_Tex_Format format, Ogl_Tex_Params params);
  void ogl_tex_update(Ogl_Tex *tex, uint8_t *data, uint32_t w, uint32_t h, Ogl_Tex_Format format, Ogl_Tex_Params params);
  Ogl_Tex ogl_tex_make(uint8_t *data, uint32_t w, uint32_t h, Ogl_Tex_Format format, Ogl_Tex_Params params);
  void ogl_tex_deinit(Ogl_Tex *tex);

  void ogl_render_bundle_draw(Ogl_Render_Bundle *bundle, Ogl_Prim_Type prim, uint32_t vertex_count, uint32_t instance_count);
  void ogl_render_bundle_destroy(Ogl_Render_Bundle *bundle);

  void ogl_render_target_init(Ogl_Render_Target *rt, uint32_t w, uint32_t h, uint32_t attachment_count, Ogl_Tex_Format format, bool add_depth);
  Ogl_Render_Target ogl_render_target_make(uint32_t w, uint32_t h, uint32_t attachment_count, Ogl_Tex_Format format, bool add_depth);
  void ogl_render_target_deinit(Ogl_Render_Target *rt);

#else

// This is complete bullshit, WHY do I need to make a vao at startup on MODERN opengl??????
void ogl_init() {
  GLuint vao = 0;
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);
}

void ogl_clear() {
  glDisable(GL_SCISSOR_TEST);
  glClearColor(0,0,0,0);
  glClear(GL_COLOR_BUFFER_BIT);
  glClear(GL_DEPTH_BUFFER_BIT);
}
 
static int64_t ogl_buf_count_bytes(Ogl_Buf *buf) {
  return buf->count * buf->bytes_per_elem;
}

static GLuint ogl_to_gl_buf_hint(Ogl_Buf_Hint hint) {
  switch (hint) {
    case OGL_BUF_HINT_STATIC:  return GL_STATIC_DRAW;
    case OGL_BUF_HINT_DYNAMIC: return GL_DYNAMIC_DRAW;
    case OGL_BUF_HINT_STREAM:  return GL_STREAM_DRAW;
    default: break;
  }
  return 0; // I dont like this
}

static GLuint ogl_to_gl_buf_kind(Ogl_Buf_Kind kind) {
  switch (kind) {
    case OGL_BUF_KIND_VERTEX:  return GL_ARRAY_BUFFER;
    case OGL_BUF_KIND_INDEX:   return GL_ELEMENT_ARRAY_BUFFER;
    case OGL_BUF_KIND_UNIFORM: return GL_UNIFORM_BUFFER;
    default: break;
  }
  return 0; // I dont like this
}

bool ogl_buf_update(Ogl_Buf *buf, uint64_t offset, void *data, uint32_t count, uint32_t bytes_per_elem) {
  if (buf) {
    GLuint gl_buf_kind = ogl_to_gl_buf_kind(buf->kind);
    glBindBuffer(gl_buf_kind, buf->impl_state);
    if (data) {
      glBufferSubData(gl_buf_kind, 0, count*bytes_per_elem, data);
    }
    glBindBuffer(gl_buf_kind, 0);
  }
  return (buf != NULL);
}

bool ogl_buf_init(Ogl_Buf *buf, Ogl_Buf_Kind kind, Ogl_Buf_Hint hint, void *data, uint32_t count, uint32_t bytes_per_elem) {
  if (buf) {
    buf->kind = kind;
    buf->count = count;
    buf->bytes_per_elem = bytes_per_elem;

    GLuint gl_buf_kind = ogl_to_gl_buf_kind(kind);
    glGenBuffers(1, &buf->impl_state);
    glBindBuffer(gl_buf_kind, buf->impl_state);
    glBufferData(gl_buf_kind, count*bytes_per_elem, data, ogl_to_gl_buf_hint(hint)); // if data == nil, buf will be zeroed
    glBindBuffer(gl_buf_kind, 0);
  }
  return (buf != NULL);
}

Ogl_Buf ogl_buf_make(Ogl_Buf_Kind kind, Ogl_Buf_Hint hint, void *data, uint32_t count, uint32_t bytes_per_elem) {
  Ogl_Buf buf;
  assert(ogl_buf_init(&buf, kind, hint, data, count, bytes_per_elem));
  return buf;
}

void ogl_buf_deinit(Ogl_Buf *buf) {
  if (buf && buf->impl_state) {
    glDeleteBuffers(1, &buf->impl_state);
  }
}
static uint32_t ogl_data_get_count(Ogl_Data_Type type) {
  switch (type) {
    case OGL_DATA_TYPE_UNKNOWN: return 0;
    case OGL_DATA_TYPE_FLOAT:   return 1;
    case OGL_DATA_TYPE_INT:     return 1;
    case OGL_DATA_TYPE_IVEC2:   return 2;
    case OGL_DATA_TYPE_IVEC3:   return 3;
    case OGL_DATA_TYPE_IVEC4:   return 4;
    case OGL_DATA_TYPE_VEC2:    return 2;
    case OGL_DATA_TYPE_VEC3:    return 3;
    case OGL_DATA_TYPE_VEC4:    return 4;
    case OGL_DATA_TYPE_MAT4:    return 16;
  }
}

static Ogl_Data_Type ogl_data_type_from_gl(GLenum type) {
  switch (type) {
    case GL_FLOAT:      return OGL_DATA_TYPE_FLOAT;
    case GL_INT:        return OGL_DATA_TYPE_INT;
    case GL_INT_VEC2:   return OGL_DATA_TYPE_IVEC2;
    case GL_INT_VEC3:   return OGL_DATA_TYPE_IVEC3;
    case GL_INT_VEC4:   return OGL_DATA_TYPE_IVEC4;
    case GL_FLOAT_VEC2: return OGL_DATA_TYPE_VEC2;
    case GL_FLOAT_VEC3: return OGL_DATA_TYPE_VEC3;
    case GL_FLOAT_VEC4: return OGL_DATA_TYPE_VEC4;
    case GL_FLOAT_MAT4: return OGL_DATA_TYPE_MAT4;
  }
  return OGL_DATA_TYPE_FLOAT;
}

static GLuint ogl_prim_type_to_gl_type(Ogl_Prim_Type prim) {
  switch (prim) {
    case OGL_PRIM_TYPE_POINT:          return GL_POINTS;
    case OGL_PRIM_TYPE_LINE:           return GL_LINES;
    case OGL_PRIM_TYPE_LINE_STRIP:     return GL_LINE_STRIP;
    case OGL_PRIM_TYPE_LINE_LOOP:      return GL_LINE_LOOP;
    case OGL_PRIM_TYPE_TRIANGLE:       return GL_TRIANGLES;
    case OGL_PRIM_TYPE_TRIANGLE_STRIP: return GL_TRIANGLE_STRIP;
    case OGL_PRIM_TYPE_TRIANGLE_FAN:   return GL_TRIANGLE_FAN;
  }
}

static GLuint ogl_create_gl_shader(const char* vertex_source, const char* fragment_source) {
  GLuint vs = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vs, 1, &vertex_source, NULL);
  glCompileShader(vs);
  int success;
  char info_log[512];
  glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(vs, 512, NULL, info_log);
    fprintf(stderr, "Vertex shader compilation failed:\n%s\n", info_log);
    return 0;
  }

  GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fs, 1, &fragment_source, NULL);
  glCompileShader(fs);
  glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(fs, 512, NULL, info_log);
    fprintf(stderr, "Fragment shader compilation failed:\n%s\n", info_log);
    return 0;
  }

  GLuint sp = glCreateProgram();
  glAttachShader(sp, vs);
  glAttachShader(sp, fs);
  glLinkProgram(sp);
  glGetProgramiv(sp, GL_LINK_STATUS, &success);
  if (!success) {
    glGetProgramInfoLog(sp, 512, NULL, info_log);
    fprintf(stderr, "Shader program linking failed:\n%s\n", info_log);
    return 0;
  }

  glDeleteShader(vs);
  glDeleteShader(fs);

  return sp;
}

static bool ogl_vert_attrib_is_integral(Ogl_Vert_Attrib *attrib) {
  Ogl_Data_Type type = attrib->type;
  return (type == OGL_DATA_TYPE_INT || type == OGL_DATA_TYPE_IVEC2 || type == OGL_DATA_TYPE_IVEC3 || type == OGL_DATA_TYPE_IVEC4);
}

static void ogl_shader_detect_vert_attribs(Ogl_Shader *shader) {
  char name_buf[256];
  GLint attrib_count;
  glGetProgramiv(shader->impl_state, GL_ACTIVE_ATTRIBUTES, &attrib_count);
  for (GLint i = 0; i < attrib_count; ++i) {
    GLenum type;
    GLint size;
    GLsizei length;
    glGetActiveAttrib(shader->impl_state, i, 256, &length, &size, &type, name_buf);
    GLint location = glGetAttribLocation(shader->impl_state, name_buf);
    if (location == -1) continue;

    // Fill the attrib slot, so we can know if the attrib is there at runtime!
    shader->vattribs[location] = (Ogl_Vert_Attrib){
      .location = location,
      .type = ogl_data_type_from_gl(type),
    };
  }
}


bool ogl_shader_init(Ogl_Shader *shader, const char* vertex_source, const char* fragment_source) {
  shader->impl_state = ogl_create_gl_shader(vertex_source, fragment_source);
  if (shader->impl_state){
    ogl_shader_detect_vert_attribs(shader);
  }
  return (shader->impl_state != 0);
}

Ogl_Shader ogl_shader_make(const char* vertex_source, const char* fragment_source) {
  Ogl_Shader shader = {0};
  assert(ogl_shader_init(&shader, vertex_source, fragment_source)); 
  return shader;
}

void ogl_shader_deinit(Ogl_Shader *shader) {
  if (shader) {
    glDeleteProgram(shader->impl_state);
    shader->impl_state = 0;
  }
}

static GLuint ogl_to_gl_wrap_mode(Ogl_Tex_Wrap_Mode mode) {
  switch (mode) {
    case OGL_TEX_WRAP_MODE_CLAMP_TO_EDGE:   return GL_CLAMP_TO_EDGE;
    case OGL_TEX_WRAP_MODE_REPEAT:          return GL_REPEAT;
    case OGL_TEX_WRAP_MODE_MIRRORED_REPEAT: return GL_MIRRORED_REPEAT;
  }
}

static GLuint ogl_to_gl_tex_filter(Ogl_Tex_Filter filter) {
  switch(filter) {
    case OGL_TEX_FILTER_NEAREST: return GL_NEAREST;
    case OGL_TEX_FILTER_LINEAR: return GL_LINEAR;
  }
}

static GLuint ogl_to_gl_depth_func(Ogl_Depth_Func dfunc) {
  switch (dfunc) {
    case OGL_DFUNC_ALWAYS:   return GL_ALWAYS; 
    case OGL_DFUNC_NEVER:    return GL_NEVER;
    case OGL_DFUNC_LESS:     return GL_LESS;
    case OGL_DFUNC_EQUAL:    return GL_EQUAL;
    case OGL_DFUNC_LEQUAL:   return GL_LEQUAL;
    case OGL_DFUNC_GREATER:  return GL_GREATER;
    case OGL_DFUNC_NOTEQUAL: return GL_NOTEQUAL;
    case OGL_DFUNC_GEQUAL:   return GL_GEQUAL;
  }
}

static bool ogl_tex_format_is_floating_point(Ogl_Tex_Format format) {
  switch (format) {
    case OGL_TEX_FORMAT_R32F:    return true;
    case OGL_TEX_FORMAT_R8U:     return false;
    case OGL_TEX_FORMAT_RGBA8U:  return false;
    case OGL_TEX_FORMAT_RGBA32F: return true;
  }
}

static GLint ogl_tex_format_to_gl_internal_format(Ogl_Tex_Format format) {
  switch (format) {
    // TODO: Why do we have to do this? Isn't there a common way to express one component textures? read spec
#if (ARCH_WASM64 || ARCH_WASM32)
    case OGL_TEX_FORMAT_R8U:     return GL_LUMINANCE;
    case OGL_TEX_FORMAT_R32F:    return GL_LUMINANCE;
#else
    case OGL_TEX_FORMAT_R8U:     return GL_RED;
    case OGL_TEX_FORMAT_R32F:    return GL_RED;
#endif
    case OGL_TEX_FORMAT_RGBA8U:  return GL_RGBA;
    case OGL_TEX_FORMAT_RGBA32F: return GL_RGBA;
  }
}
static GLint ogl_tex_format_component_num(Ogl_Tex_Format format) {
  switch (format) {
    case OGL_TEX_FORMAT_R32F:    return 1;
    case OGL_TEX_FORMAT_R8U:     return 1;
    case OGL_TEX_FORMAT_RGBA8U:  return 4;
    case OGL_TEX_FORMAT_RGBA32F: return 4;
  }
}


void ogl_tex_update(Ogl_Tex *tex, uint8_t *data, uint32_t w, uint32_t h, Ogl_Tex_Format format, Ogl_Tex_Params params) {
  tex->format = format;
  tex->width = w;
  tex->height= w;

  GLenum type = (ogl_tex_format_is_floating_point(tex->format)) ? GL_FLOAT : GL_UNSIGNED_BYTE;
  GLint internal_format = ogl_tex_format_to_gl_internal_format(tex->format);
  GLenum image_format = internal_format;

  // This is pretty much a hack to make depth buffers easily w/ this
  if (params.is_depth) {
    type = GL_UNSIGNED_INT_24_8;
    internal_format = GL_DEPTH24_STENCIL8;
    image_format = GL_DEPTH_STENCIL;
  }

  //----------------------------------------------------------------
  // WebGL2 doesn't support texture swizzles.. so.. we do nothing here !!!! ???? @@@@
  // https://registry.khronos.org/webgl/specs/latest/2.0/#5.18
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  //glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#if 0 
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_RED);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_GREEN);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_BLUE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_ALPHA);
#endif
  //----------------------------------------------------------------


  glBindTexture(GL_TEXTURE_2D, tex->impl_state);
  glTexImage2D(GL_TEXTURE_2D, 0, internal_format, w, h, 0, image_format, type, data);
  if (params.generate_mips) {
    glGenerateMipmap(GL_TEXTURE_2D);
  }
}

bool ogl_tex_init(Ogl_Tex *tex, uint8_t *data, uint32_t w, uint32_t h, Ogl_Tex_Format format, Ogl_Tex_Params params) {
  tex->params = params;
  tex->width = w;
  tex->height = h;
  tex->format = format;

  GLuint texture_id;
  glGenTextures(1, &texture_id);
  glBindTexture(GL_TEXTURE_2D, texture_id);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, ogl_to_gl_wrap_mode(tex->params.wrap_s));
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, ogl_to_gl_wrap_mode(tex->params.wrap_t));
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, ogl_to_gl_wrap_mode(tex->params.wrap_r));
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, ogl_to_gl_tex_filter(tex->params.min_filter));
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, ogl_to_gl_tex_filter(tex->params.mag_filter));

  tex->impl_state = texture_id;
  ogl_tex_update(tex, data, w, h, format, params);

  assert(tex->impl_state);

  return (tex->impl_state != 0);
}

// Is this really needed? I don't really need it :|
Ogl_Tex ogl_tex_make(uint8_t *data, uint32_t w, uint32_t h, Ogl_Tex_Format format, Ogl_Tex_Params params) {
  Ogl_Tex tex = {};
  ogl_tex_init(&tex, data, w, h, format, params);
  return tex;
}

void ogl_tex_deinit(Ogl_Tex *tex) {
  glDeleteTextures(1, (GLuint*)&tex->impl_state);
  tex->impl_state = 0;
}

void ogl_render_target_init(Ogl_Render_Target *rt, uint32_t w, uint32_t h, uint32_t attachment_count, Ogl_Tex_Format format, bool add_depth) {
  assert(attachment_count <= OGL_MAX_RENDER_TARGET_ATTACHMENTS);

  rt->width = w;
  rt->height = h;

  GLuint fbo = 0;
  glGenFramebuffers(1, &fbo);
  rt->impl_state = fbo;
  assert(rt->impl_state);
  glBindFramebuffer(GL_FRAMEBUFFER, rt->impl_state);

  for (uint32_t attachment_idx = 0; attachment_idx < attachment_count; ++attachment_idx) {
    rt->attachments[attachment_idx] = ogl_tex_make(NULL, rt->width, rt->height, format, (Ogl_Tex_Params){});
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + attachment_idx, GL_TEXTURE_2D, rt->attachments[attachment_idx].impl_state, 0);
  }

  if (add_depth) {
    rt->depth_attachment = ogl_tex_make(NULL, rt->width, rt->height, OGL_TEX_FORMAT_R32F,(Ogl_Tex_Params){.is_depth = true,});
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, rt->depth_attachment.impl_state, 0);
  }

  GLenum draw_buffers[OGL_MAX_RENDER_TARGET_ATTACHMENTS];
  for (uint32_t attachment_idx = 0; attachment_idx < attachment_count; ++attachment_idx) {
    draw_buffers[attachment_idx] = GL_COLOR_ATTACHMENT0 + attachment_idx;
  }
  glDrawBuffers(attachment_count, draw_buffers);

  assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

Ogl_Render_Target ogl_render_target_make(uint32_t w, uint32_t h, uint32_t attachment_count, Ogl_Tex_Format format, bool add_depth) {
  Ogl_Render_Target rt = {};
  ogl_render_target_init(&rt, w, h, attachment_count, format, add_depth);
  return rt;
}

void ogl_render_target_deinit(Ogl_Render_Target *rt) {
  glDeleteFramebuffers(1, (GLuint*)&rt->impl_state);
}

void ogl_render_bundle_destroy(Ogl_Render_Bundle *bundle) {
  ogl_shader_deinit(&bundle->sp);
  ogl_buf_deinit(&bundle->index_buffer);
  for (uint64_t slot_idx = 0; slot_idx < OGL_MAX_VERTEX_BUFFERS; ++slot_idx) {
    Ogl_Vertex_Buffer_Slot *vbo = &bundle->vbos[slot_idx];
    if (ogl_buf_count_bytes(&vbo->buffer) > 0) {
      ogl_buf_deinit(&vbo->buffer);
    }
  }
  for (uint64_t slot_idx = 0; slot_idx < OGL_MAX_UNIFORM_BUFFERS; ++slot_idx) {
    Ogl_Uniform_Buffer_Slot *ubo = &bundle->ubos[slot_idx];
    if (ogl_buf_count_bytes(&ubo->buffer) > 0) {
      ogl_buf_deinit(&ubo->buffer);
    }
  }
}

static void ogl_render_bundle_bind(Ogl_Render_Bundle *bundle) {
  // Bind the shader
  glUseProgram(bundle->sp.impl_state); 
  // Bind the index buffer
  glBindBuffer(ogl_to_gl_buf_kind(bundle->index_buffer.kind), bundle->index_buffer.impl_state);
  // Bind the vertex buffer(s) + set attributes
  for (uint64_t slot_idx = 0; slot_idx < OGL_MAX_VERTEX_BUFFERS; ++slot_idx) {
    Ogl_Vertex_Buffer_Slot *vbo = &bundle->vbos[slot_idx];
    if (ogl_buf_count_bytes(&vbo->buffer) > 0) {
      glBindBuffer(ogl_to_gl_buf_kind(vbo->buffer.kind), vbo->buffer.impl_state);
      for (uint64_t vattr_idx = 0; vattr_idx < OGL_MAX_ATTRIBS; ++vattr_idx) {
        Ogl_Vert_Attrib *attr = &vbo->vattribs[vattr_idx];
        if (attr->type != OGL_DATA_TYPE_UNKNOWN) {
          glEnableVertexAttribArray(vattr_idx);
          GLsizei stride = (attr->stride == 0) ? (vbo->buffer.bytes_per_elem) : (attr->stride);
          if (ogl_vert_attrib_is_integral(attr)) {
            glVertexAttribIPointer(vattr_idx, ogl_data_get_count(attr->type), GL_INT, stride, (void*)attr->offset);
          } else {
            glVertexAttribPointer(vattr_idx, ogl_data_get_count(attr->type), GL_FLOAT, GL_FALSE, stride, (void*)attr->offset);
          }
          glVertexAttribDivisor(vattr_idx, attr->instanced);
        } else {
          glDisableVertexAttribArray(vattr_idx);
        }
      }
    }
  }
  // Bind the uniform buffer(s)
  for (int64_t slot_idx = 0; slot_idx < OGL_MAX_UNIFORM_BUFFERS; ++slot_idx) {
    Ogl_Uniform_Buffer_Slot *ubo = &bundle->ubos[slot_idx];
    if (ogl_buf_count_bytes(&ubo->buffer) > 0) {
      GLuint ubo_block_idx = glGetUniformBlockIndex(bundle->sp.impl_state, ubo->name);
      glUniformBlockBinding(bundle->sp.impl_state, ubo_block_idx, slot_idx);
      glBindBufferRange(GL_UNIFORM_BUFFER, slot_idx, ubo->buffer.impl_state, ubo->start_offset, ubo->size);
    }
  }
  // Bind the texture(s)
  for (int64_t slot_idx = 0; slot_idx < OGL_MAX_UNIFORM_BUFFERS; ++slot_idx) {
    Ogl_Tex_Slot *tex = &bundle->textures[slot_idx];
    if (tex->tex.impl_state) {
      glActiveTexture(GL_TEXTURE0+slot_idx);
      glBindTexture(GL_TEXTURE_2D, tex->tex.impl_state);
      GLuint tex_loc = glGetUniformLocation(bundle->sp.impl_state, tex->name); 
      // NOTE: If we first bind all the textures to slots we can populate a c-style sampler array via glUniform1iv and passing all slot_idx's
      GLint *samplers = (GLint[]) {(GLint)slot_idx};
      glUniform1iv(tex_loc, 1, samplers);
    }
  }
  // Bind the render target 
  if (bundle->rt.width * bundle->rt.height > 0) {
    glBindFramebuffer(GL_FRAMEBUFFER, bundle->rt.impl_state);
    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
  } else {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
  }

  // Set the dynamic state
  Ogl_rect viewport = bundle->dyn_state.viewport;
  glViewport(viewport.x, viewport.y, viewport.w, viewport.h);
  if (bundle->dyn_state.flags & OGL_DYN_STATE_FLAG_SCISSOR) {
    glEnable(GL_SCISSOR_TEST);
    Ogl_rect scissor = bundle->dyn_state.scissor;
    glScissor(scissor.x, scissor.y, scissor.w, scissor.h);
  } else {
    glDisable(GL_SCISSOR_TEST);
  }
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(ogl_to_gl_depth_func(bundle->depth_state.dfunc));
  glDepthMask((bundle->depth_state.dwrite == OGL_DEPTH_WRITE_ENABLED) ? GL_TRUE : GL_FALSE);
  if (bundle->dyn_state.flags & OGL_DYN_STATE_FLAG_BLEND) {
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  } else {
    glDisable(GL_BLEND);
  }

}

void ogl_render_bundle_draw(Ogl_Render_Bundle *bundle, Ogl_Prim_Type prim, uint32_t vertex_count, uint32_t instance_count) {
  ogl_render_bundle_bind(bundle);
  // FIXME: first == 0? why? we need to enhance the API
  glDrawArraysInstanced(ogl_prim_type_to_gl_type(prim), 0, vertex_count, instance_count);
}

#endif

#ifdef __cplusplus
}
#endif

#endif
