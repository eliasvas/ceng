layout(local_size_x = 1, local_size_y = 1) in;
layout (binding = 0) uniform sampler2D depth_buffer;

layout(std430, binding = 0) buffer buf {
  uint zmin[MAX_CASCADES];
  uint zmax[MAX_CASCADES];
  uint xmin[MAX_CASCADES];
  uint xmax[MAX_CASCADES];
  uint ymin[MAX_CASCADES];
  uint ymax[MAX_CASCADES];

  uint split_min_z[MAX_CASCADES];
  uint split_max_z[MAX_CASCADES];

  uint minmax[2];
};
 
void main() {
  uint split_idx = gl_WorkGroupID.z;
  ivec2 pixel_coords = ivec2(gl_WorkGroupID.xy);
  float depth = texelFetch(depth_buffer, pixel_coords, 0).x;
  uint d = floatBitsToUint(depth);
  vec2 tc = vec2(pixel_coords) / vec2(textureSize(depth_buffer, 0));
  
  if (depth >= 1.0) return;

  float min_depth = uintBitsToFloat(split_min_z[split_idx]);
  float max_depth = uintBitsToFloat(split_max_z[split_idx]);
  if (depth > min_depth && depth < max_depth) {
    uint x = floatBitsToUint(tc.x);
    uint y = floatBitsToUint(tc.y);

    atomicMin(xmin[split_idx], x);
    atomicMax(xmax[split_idx], x);
    atomicMin(ymin[split_idx], y);
    atomicMax(ymax[split_idx], y);
    atomicMin(zmin[split_idx], d);
    atomicMax(zmax[split_idx], d);
  }

  atomicMin(minmax[0], d);
  atomicMax(minmax[1], d);
}
