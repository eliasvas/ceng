layout(local_size_x = 1, local_size_y = 1) in;
layout (binding = 0, r32ui) uniform uimage2D bounds;
layout (binding = 1, r32ui) uniform uimage2D global_bounds;
layout (binding = 0) uniform sampler2D depth_buffer;

layout(std430, binding = 0) buffer buf {
  uint zmin[MAX_CASCADES];
  uint zmax[MAX_CASCADES];
  uint xmin[MAX_CASCADES];
  uint xmax[MAX_CASCADES];
  uint ymin[MAX_CASCADES];
  uint ymax[MAX_CASCADES];

  uint minmax[2];
};
 

// Are these really correct, I guess because in screen space the tc's are in [0,1]?

const ivec2 bounds_min_x_tc = ivec2(0,0);
const ivec2 bounds_max_x_tc = ivec2(1,0);

const ivec2 bounds_min_y_tc = ivec2(2,0);
const ivec2 bounds_max_y_tc = ivec2(3,0);

const ivec2 bounds_min_z_tc = ivec2(4,0);
const ivec2 bounds_max_z_tc = ivec2(5,0);

const ivec2 bounds_min_cascade_z_tc = ivec2(6,0);
const ivec2 bounds_max_cascade_z_tc = ivec2(7,0);

void main() {
  ivec2 pixel_coords = ivec2(gl_WorkGroupID.xy);
  float depth = texelFetch(depth_buffer, pixel_coords, 0).x;
  uint d = floatBitsToUint(depth);
  vec2 tc = vec2(pixel_coords) / vec2(textureSize(depth_buffer, 0));
  
  if (depth >= 1.0) return;

  float min_depth = uintBitsToFloat(imageLoad(bounds, bounds_min_cascade_z_tc).x);
  float max_depth = uintBitsToFloat(imageLoad(bounds, bounds_max_cascade_z_tc).x);
  if (depth >= min_depth && depth <= max_depth) {
    uint x = floatBitsToUint(tc.x);
    uint y = floatBitsToUint(tc.y);

    imageAtomicMin(bounds, bounds_min_x_tc, x);
    imageAtomicMax(bounds, bounds_max_x_tc, x);

    imageAtomicMin(bounds, bounds_min_y_tc, y);
    imageAtomicMax(bounds, bounds_max_y_tc, y);

    imageAtomicMin(bounds, bounds_min_z_tc, d);
    imageAtomicMax(bounds, bounds_max_z_tc, d);
  }

  imageAtomicMin(global_bounds, ivec2(0,0), d);
  imageAtomicMax(global_bounds, ivec2(1,0), d);

  minmax[0] = floatBitsToUint(6.0);
  minmax[1] = floatBitsToUint(7.0);

}
