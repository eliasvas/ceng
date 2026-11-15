layout(local_size_x = 1, local_size_y = 1) in;
layout (binding = 0, r32ui) uniform uimage2D drange;
layout (binding = 0)uniform sampler2D depth_buffer;

const ivec2 drange_min_tc = ivec2(0,0);
const ivec2 drange_max_tc = ivec2(1,0);

void main() {
    ivec2 pixel_coords = ivec2(gl_WorkGroupID.xy);
    float depth = texelFetch(depth_buffer, pixel_coords, 0).x;
    if (depth >= 1.0) return; // We don't handle the glCleared values
    uint k = floatBitsToUint(depth);
    imageAtomicMin(drange, drange_min_tc, k);
    imageAtomicMax(drange, drange_max_tc, k);
}
