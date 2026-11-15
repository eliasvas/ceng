layout(location = 0) out vec4 out_color;

const float EPSILON = 0.00001f;
bool eqf(float a, float b) { return abs(a - b) <= (abs(a) < abs(b) ? abs(b) : abs(a)) * EPSILON; }
float max3(vec3 v) { return max(max(v.x, v.y), v.z); }

in VS_OUT {
  vec2 tc;
} fs_in;

uniform sampler2D u_tex;
uniform sampler2D accum;
uniform sampler2D reveal;

// Ref: https://www.4rknova.com/blog/2015/02/15/fxaa
vec3 sample_tex(vec2 p) { return texture(u_tex, p).xyz; }
vec3 fxaa(vec2 p, ivec2 size) {
	float FXAA_SPAN_MAX	= 8.0;
	float FXAA_REDUCE_MUL = 1.0 / 8.0;
	float FXAA_REDUCE_MIN = 1.0 / 128.0;

	// 1st stage - Find edge
	vec3 rgbNW = sample_tex(p + (vec2(-1.,-1.) / size));
	vec3 rgbNE = sample_tex(p + (vec2( 1.,-1.) / size));
	vec3 rgbSW = sample_tex(p + (vec2(-1., 1.) / size));
	vec3 rgbSE = sample_tex(p + (vec2( 1., 1.) / size));
	vec3 rgbM = sample_tex(p);
	vec3 luma = vec3(0.299, 0.587, 0.114);
	float lumaNW = dot(rgbNW, luma);
	float lumaNE = dot(rgbNE, luma);
	float lumaSW = dot(rgbSW, luma);
	float lumaSE = dot(rgbSE, luma);
	float lumaM = dot(rgbM, luma);
	vec2 dir;
	dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
	dir.y = ((lumaNW + lumaSW) - (lumaNE + lumaSE));
	float lumaSum = lumaNW + lumaNE + lumaSW + lumaSE;
	float dirReduce = max(lumaSum * (0.25 * FXAA_REDUCE_MUL) , FXAA_REDUCE_MIN);
	float rcpDirMin = 1. / (min(abs(dir.x), abs(dir.y)) + dirReduce);
	dir = min(vec2(FXAA_SPAN_MAX), max(vec2(-FXAA_SPAN_MAX), dir * rcpDirMin)) / size;

	// 2nd stage - Blur
	vec3 rgbA = .5 * (sample_tex(p + dir * (1./3. - .5)) + sample_tex(p + dir * (2./3. - .5)));
	vec3 rgbB = rgbA * .5 + .25 * ( sample_tex(p + dir * (0./3. - .5)) + sample_tex(p + dir * (3./3. - .5)));
	float lumaB = dot(rgbB, luma);
	float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
	float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));
	return ((lumaB < lumaMin) || (lumaB > lumaMax)) ? rgbA : rgbB;
}

void main() {
  ivec2 size = textureSize(u_tex, 0);
  // Calculate opaque color
  vec3 opaque_color = fxaa(fs_in.tc, size);
  // FIXME: enable fxaa
  //vec3 opaque_color = texture(u_tex, fs_in.tc).rgb;
  // Calculate transparent color
  ivec2 coords = ivec2(gl_FragCoord.xy);
  float revealage = texelFetch(reveal, coords, 0).r;

  vec3 transparent_color = vec3(0);
  if (!eqf(revealage, 1.0f)) {
    vec4 accumulation = texelFetch(accum, coords, 0);
    if (isinf(max3(abs(accumulation.rgb)))) accumulation.rgb = vec3(accumulation.a);
    vec3 average_color = accumulation.rgb / max(accumulation.a, EPSILON);
    transparent_color = average_color.xyz;
  }
  float alpha = 1.0 - revealage;
  out_color = vec4(transparent_color * alpha + opaque_color * revealage, 1.0);
}
