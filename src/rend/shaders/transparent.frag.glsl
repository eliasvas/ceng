layout(location = 0) out vec4 accum;
layout(location = 1) out float reveal;

layout(std140) uniform Material {
  vec4 base_color_factor;
  vec4 emissive_factor;
  float metallic_factor;
  float roughness_factor;

  int base_tc_idx;
  int normal_tc_idx;
  int metallic_roughness_tc_idx;
  int emissive_tc_idx;
  int occlusion_tc_idx;
  int is_unlit;

  mat4 model_matrix;
};

in VS_OUT {
  vec4 color;
  vec4 wp;
  vec3 view_dir;
  vec3 norm;
  vec3 tang;
  vec3 binorm;
  vec2 tc[4];
  vec3 frag_pos_ws;
  vec4 frag_pos_ls[MAX_CASCADES];
} fs_in;

uniform sampler2D base_color_tex;

void main() {
  // Calculate the color like in regular VS
  vec4 fs_color = fs_in.color * texture(base_color_tex, fs_in.tc[base_tc_idx]);

  // the weight function
  float weight = clamp(pow(min(1.0, fs_color.a * 10.0) + 0.01, 3.0) * 1e8 * pow(1.0 - gl_FragCoord.z * 0.9, 3.0), 1e-2, 3e3);
  // accum is for color accumulation
  accum = vec4(fs_color.rgb * fs_color.a, fs_color.a) * weight;
  reveal = fs_color.a;
}
