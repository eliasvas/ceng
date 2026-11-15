layout(location = 0) out vec4 out_color;

in VS_OUT {
  vec2 tc;
} fs_in;

uniform sampler2D u_tex;

vec3 sample_tex(vec2 p) { return texture(u_tex, p).xyz; }

void main() {
  out_color = vec4(sample_tex(fs_in.tc), 1.0);
}
