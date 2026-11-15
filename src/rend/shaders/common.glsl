#version 460 core

#define MAX_CASCADES 4
layout (std140) uniform PerFrameData { 
  mat4 lsms[MAX_CASCADES];
  mat4 view;
  mat4 proj;
  vec3 cam_pos;
  vec3 light_dir;
  vec4 cascade_bounds;
  int cascade_idx;
};
