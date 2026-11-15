precision highp float;
layout(location = 0) out vec4 out_color;

// ref: https://www.rastertek.com/gl4linuxtut52.html
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
uniform sampler2D normal_tex;
uniform sampler2D metallic_roughness_tex;
uniform sampler2D emissive_tex;
uniform sampler2D occlusion_tex;

uniform sampler2D shadow_map;

int get_cascade_layer(vec3 frag_pos_ws) {
  // Find the depth of the fragment from camera POV
  vec4 frag_pos_vs = view * vec4(frag_pos_ws, 1);
  float depth_value = abs(frag_pos_vs.z);
  // Find the actual depth value
  int layer = -1;
  for (int i = 0; i < MAX_CASCADES; i+=1) {
    if (depth_value < cascade_bounds[i]) {
      layer = i;
      break;
    }
  }
  if (layer == -1) {
    layer = 3;
  }

  return layer;
}

float shadow_calc(vec3 frag_pos_ws) {
  int layer = get_cascade_layer(frag_pos_ws);

  vec4 frag_pos_lightspace = fs_in.frag_pos_ls[layer];

  // project to get actual NDC (not needed for ortho but still)
  vec3 proj_coords = frag_pos_lightspace.xyz / frag_pos_lightspace.w;
  // go to [0,1] range (to sample the tex)
  proj_coords = proj_coords*0.5+0.5;

  // sample the tex
  float closest_depth = texture(shadow_map, proj_coords.xy)[layer];
  float current_depth = proj_coords.z;

  // This is wrong!
  float bias = 0.005;

  // calculate bias
  //vec3 normal = normalize(fs_in.norm);
  //float bias = max(0.05 * (1.0 - dot(normal, light_dir)), 0.005);
  //bias *= 1 / (cascade_bounds[layer] * 0.5f);

  // calculate PCF
  float shadow = 0.0;
  vec2 texel_size = 1.0 / vec2(textureSize(shadow_map, 0));
  for(int x = -1; x <= 1; ++x) {
    for(int y = -1; y <= 1; ++y) {
      float pcf_depth = texture(shadow_map, vec2(proj_coords.xy + vec2(x, y) * texel_size))[layer];
      shadow += (current_depth- bias) > pcf_depth ? 1.0 : 0.0;
    }
  }
  shadow /= 9.0*1.5; // should be 9 i just want soft shadows

  if (proj_coords.z > 1.0) {
    shadow = 0.0;
  }

  return shadow;
}

bool texture_present(sampler2D tex) {
  return (textureSize(tex, 0).x > 1);
}

vec3 get_normal() {
  vec3 norm = fs_in.norm;
  if (texture_present(normal_tex)) {
    vec3 bump_map = texture(normal_tex, fs_in.tc[normal_tc_idx]).rgb;
    bump_map = (bump_map * 2.0f) - 1.0f;
    norm = (bump_map.x * fs_in.tang) + (bump_map.y * fs_in.binorm) + (bump_map.z * fs_in.norm);
    norm = normalize(norm);
  }
  return norm;
}

void main() {
  //vec3 lD = -light_dir;
  vec3 lD = light_dir;

  // Sample the textures needed
  vec3 albedo = texture(base_color_tex, fs_in.tc[base_tc_idx]).rgb * fs_in.color.rgb;
  vec3 metallic_rough = texture(metallic_roughness_tex, fs_in.tc[metallic_roughness_tc_idx]).rgb*vec3(metallic_factor, roughness_factor, 1);
  vec3 norm = get_normal();

  // Retrieve metallic/roughness
  float roughness = metallic_rough.r;
  float metallic = metallic_rough.b;

  // Calculate fresnel factor
  vec3 F0 = vec3(0.04f, 0.04f, 0.04f);
  F0 = mix(F0, albedo, metallic);

  // Setup all needed vectors for lighting
  vec3 halfdir = normalize(fs_in.view_dir + lD);
  float NdotH = max(0.0f, dot(norm, halfdir));
  float NdotV = max(0.0f, dot(norm, fs_in.view_dir));
  float NdotL = max(0.0f, dot(norm, lD));
  float HdotV = max(0.0f, dot(halfdir, fs_in.view_dir));

  // GGX for normal distribution
  float roughness_sqr = roughness * roughness;
  float rough_sqr2 = roughness_sqr * roughness_sqr;
  float NdotHSqr = NdotH * NdotH;
  float denominator = (NdotHSqr * (rough_sqr2 - 1.0f) + 1.0f);
  denominator = 3.14159265359f * (denominator * denominator);
  float normal_distribution = rough_sqr2 / denominator;

  // Schlick GGX for the geometric shadow
  float smithL = NdotL / (NdotL * (1.0f - roughness_sqr) + roughness_sqr);
  float smithV = NdotV / (NdotV * (1.0f - roughness_sqr) + roughness_sqr);
  float geom_shadow = smithL * smithV;

  // Fresnel SChlick for fresnel calculation
  vec3 fresnel = F0 + (1.0f - F0) * pow(1.0f - HdotV, 5.0f);

  // Cook-Torrance specular BRDF
  vec3 specularity = (normal_distribution * fresnel * geom_shadow) / ((4.0f * (NdotL * NdotV)) + 0.00001f);

  // Emissive color
  vec3 emissive = texture(emissive_tex, fs_in.tc[emissive_tc_idx]).rgb * emissive_factor.rgb;

  vec4 color = vec4(0.0,0.0,0.0,1.0);
  color.rgb = (albedo + specularity + emissive) * NdotL;

  if (is_unlit == 1) {
    color.rgb = albedo;
  }


  // For shadow map visualization
  vec3 layer_color = vec3(0.0);
#if 1
  int layer = get_cascade_layer(fs_in.frag_pos_ws);
  if (layer == 0) { layer_color = 0.3*vec3(1.0,0.0,0.0); }
  if (layer == 1) { layer_color = 0.3*vec3(0.0,1.0,0.0); }
  if (layer == 2) { layer_color = 0.3*vec3(0.0,0.0,1.0); }
  if (layer == 3) { layer_color = 0.3*vec3(1.0,1.0,1.0); }
#endif

  float shadow = shadow_calc(fs_in.frag_pos_ws);
  out_color.rgb = color.rgb * (1.0 - shadow) + (1.0 - shadow) * layer_color;
  out_color.a = color.a;
}
