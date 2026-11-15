vec2 vertices[4] = vec2[](
  vec2(-1,+1),
  vec2(-1,-1),
  vec2(+1,-1),
  vec2(+1,+1)
);

vec2 tex_coords[4] = vec2[](
  vec2(0.0,1.0),
  vec2(0.0,0.0),
  vec2(1.0,0.0),
  vec2(1.0,1.0)
);

out VS_OUT {
  vec2 tc;
} vs_out;

void main() { 
  vs_out.tc = tex_coords[gl_VertexID];

  vec2 pos = vertices[gl_VertexID];
	gl_Position = vec4(pos, 0.0, 1.0);
}
