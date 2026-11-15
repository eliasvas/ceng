
layout(location=0)  in vec3 pos;
layout(location=1)  in vec3 norm;
layout(location=2)  in vec4 tang;
layout(location=3)  in vec2 tc_0;
layout(location=4)  in vec2 tc_1;
layout(location=5)  in vec2 tc_2;
layout(location=6)  in vec2 tc_3;
layout(location=7)  in vec4 col_0;
layout(location=8)  in vec4 col_1;
layout(location=9)  in vec4 col_2;
layout(location=10) in vec4 col_3;
layout(location=11) in ivec4 joint_0;
//layout(location=12) in vec4 joint_1;
//layout(location=13) in vec4 joint_2;
//layout(location=14) in vec4 joint_3;
layout(location=12) in vec4 weight_0;

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

// FIXME: Why is max number of bones 32???? fix this
layout(std140) uniform JointMatrices {
  mat4 joint_mat[32];
};

out VS_OUT {
  vec4 color;
  vec4 wp;
  vec3 view_dir;
  vec3 norm;
  vec3 tang;
  vec3 binorm;
  vec2 tc[4];
  vec3 frag_pos_ws;
  vec4 frag_pos_ls[MAX_CASCADES];
} vs_out;

void main() { 
  vs_out.color = col_0*base_color_factor;

  mat4 skinMat =
    weight_0.x * joint_mat[joint_0.x] +
    weight_0.y * joint_mat[joint_0.y] +
    weight_0.z * joint_mat[joint_0.z] +
    weight_0.w * joint_mat[joint_0.w];

	gl_Position = proj * view * model_matrix * skinMat * vec4(pos, 1.0);

  vs_out.tc[0] = tc_0;
  vs_out.tc[1] = tc_1;
  vs_out.tc[2] = tc_2;
  vs_out.tc[3] = tc_3;

  // Normal mapping bullshido
  mat3 M = mat3(model_matrix);
  mat3 normal_matrix = transpose(inverse(M));
  vs_out.norm = normalize(normal_matrix * norm);
  vec3 T = normalize(M * tang.xyz);
  vec3 N = normalize(normal_matrix * norm);
  T = normalize(T - N * dot(N, T));
  vec3 B = normalize(cross(N, T)) * tang.w;
  vs_out.tang = T;
  vs_out.binorm = B;

  vs_out.wp = model_matrix * vec4(pos, 1.0f);
  vs_out.view_dir = normalize(cam_pos.xyz - vs_out.wp.xyz);

  vs_out.frag_pos_ws = vec3(model_matrix * vec4(pos, 1.0));
  for (int i = 0; i < MAX_CASCADES; i+=1) {
    vs_out.frag_pos_ls[i] = lsms[i] * vec4(vs_out.frag_pos_ws, 1.0);
  }
}
