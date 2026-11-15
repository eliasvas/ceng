layout(location = 0) out vec4 out_color;

#define MAX_CASCADES 4

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

void main() {
  float depth = gl_FragCoord.z;
  out_color[cascade_idx] = depth;
}
