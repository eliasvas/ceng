#ifndef MATH_3D_H__
#define MATH_3D_H__
#include <math.h>
#include "helper.h"
#include "base/base_inc.h"

////////////////////////////////////////////////////////////
// References:
// This repo is Linear Algebra mixed with graphics algos.
// I believe the best way to go about this is to get a linalg textbook
// and use it in combination with CG resources, do a soft. rasterizer! 
//
// Immersive Math: https://immersivemath.com/
// Essence of Linear Algebra: https://www.youtube.com/playlist?list=PLZHQObOWTQDPD3MizzM2xVFitgF8hE_ab
// Elementary Linear Algebra, Anton: https://www.studyhalo.com/media/resources/resources/MAT1503/Textbook/MAT1503_-_Prescribed_book.pdf
// songho: https://www.songho.ca/opengl/index.html
// Scratchapixel Rasterization: https://www.scratchapixel.com/lessons/3d-basic-rendering/rasterization-practical-implementation/overview-rasterization-algorithm.html
////////////////////////////////////////////////////////////

// TODO: Should we add integer vector types here? We would also need direction e.g WESN
// TODO: Quaternions :|
// TODO: SIMD

#ifndef BMATH_BAKED_MATH_FUNCTIONS
#define sqrt_f64(n)   (sqrt(n))
#define floor_f64(n)  (floor(n))
#define ceil_f64(n)   (ceil(n))
#define round_f64(n)  (round(n))
#define abs_f64(n)    (fabs(n))
#define mod_f64(a, b) (fmod((a), (b)))
#define pow_f64(b, e) (pow((b), (e)))
#define sin_f64(rad)  (sin(rad))
#define cos_f64(rad)  (cos(rad))
#define tan_f64(rad)  (tan(rad))

#define sqrt_f32(n)   (sqrtf(n))
#define floor_f32(n)  (floorf(n))
#define ceil_f32(n)   (ceilf(n))
#define round_f32(n)  (roundf(n))
#define abs_f32(n)    (fabsf(n))
#define mod_f32(a, b) (fmodf((a), (b)))
#define pow_f32(b, e) (powf((b), (e)))
#define sin_f32(rad)  (sinf(rad))
#define cos_f32(rad)  (cosf(rad))
#define tan_f32(rad)  (tanf(rad))
#else
#error "Unimplemented libmath functions"
#endif

typedef union v2
{
    struct { f32 x,y; };
    struct { f32 u,v; };
    struct { f32 r,g; };
    f32 raw[2];
}v2;

#define v2_zero ((v2){{0, 0}})
#define v2_one ((v2){{1, 1}})
#define v2m(x, y)   ((v2){{x, y}})
INLINE v2  v2_add(v2 a, v2 b)          { return v2m(a.x+b.x,a.y+b.y); }
INLINE v2  v2_sub(v2 a, v2 b)          { return v2m(a.x-b.x,a.y-b.y); }
INLINE v2  v2_mult(v2 a, v2 b)         { return v2m(a.x*b.x,a.y*b.y); }
INLINE v2  v2_multf(v2 a, f32 b)       { return v2m(a.x*b,a.y*b); }
INLINE v2  v2_div(v2 a, v2 b)          { return v2m(a.x/b.x,a.y/b.y); }
INLINE v2  v2_divf(v2 a, f32 b)        { return v2m(a.x/b,a.y/b); }
INLINE v2  v2_lerp(v2 a, v2 b, f32 x)  { return v2m(a.x*(1.0-x) + b.x*x,a.y*(1.0-x) + b.y*x); }
INLINE f32 v2_dot(v2 a, v2 b)          { return (a.x*b.x)+(a.y*b.y); }
INLINE f32 v2_len(v2 a)                { return sqrt_f32(v2_dot(a,a)); }
INLINE v2  v2_norm(v2 a)               { f32 vl=v2_len(a);return v2_divf(a,vl); }
INLINE b32 v2_eq(v2 a, v2 b)           { return (equalf(a.x,b.x,0.001) && equalf(a.y,b.y,0.001)); }
INLINE v2  v2_rot(v2 a, f32 arad)      { return v2m(a.x*cos(arad)-a.y*sin(arad), a.x*sin(arad)+a.y*cos(arad)); }

typedef union v3
{
    struct { f32 x,y,z; };
    struct { f32 r,g,b; };
    f32 raw[3];
}v3;

#define v3_zero ((v3){{0, 0, 0}})
#define v3_one ((v3){{1, 1, 1}})
#define v3m(x, y, z)   ((v3){{x, y, z}})
INLINE v3  v3_add(v3 a, v3 b)         { return v3m(a.x+b.x,a.y+b.y,a.z+b.z); }
INLINE v3  v3_sub(v3 a, v3 b)         { return v3m(a.x-b.x,a.y-b.y,a.z-b.z); }
INLINE v3  v3_mult(v3 a, v3 b)        { return v3m(a.x*b.x,a.y*b.y,a.z*b.z); }
INLINE v3  v3_multf(v3 a, f32 b)      { return v3m(a.x*b,a.y*b,a.z*b); }
INLINE v3  v3_div(v3 a, v3 b)         { return v3m(a.x/b.x,a.y/b.y,a.z/b.z); }
INLINE v3  v3_divf(v3 a, f32 b)       { return v3m(a.x/b,a.y/b,a.z/b); }
INLINE v3  v3_lerp(v3 a, v3 b, f32 x) { return v3m(a.x*(1.0-x) + b.x*x,a.y*(1.0-x) + b.y*x,a.z*(1.0-x)+b.z*x); }
INLINE f32 v3_dot(v3 a, v3 b)         { return (a.x*b.x)+(a.y*b.y)+(a.z*b.z); }
INLINE f32 v3_len(v3 a)               { return sqrt_f32(v3_dot(a,a)); }
INLINE v3  v3_norm(v3 a)              { f32 vl=v3_len(a);if (equalf(vl,0.0,0.000001))return a; else return v3_divf(a,vl); }
INLINE b32 v3_eq(v3 a, v3 b)          { return (equalf(a.x,b.x,0.001) && equalf(a.y,b.y,0.001) && equalf(a.z,b.z,0.001)); }
INLINE v3  v3_cross(v3 a,v3 b)        { v3 res; res.x=(a.y*b.z)-(a.z*b.y); res.y=(a.z*b.x)-(a.x*b.z); res.z=(a.x*b.y)-(a.y*b.x); return (res); }
INLINE v3  v3_rot_x(v3 a, f32 arad)   { return v3m(a.x,a.y*cos_f32(arad)-a.z*sin_f32(arad),a.y*sin_f32(arad)+a.z*cos_f32(arad)); }
INLINE v3  v3_rot_y(v3 a, f32 arad)   { return v3m(a.x*cos_f32(arad)+a.z*sin_f32(arad),a.y,-a.x*sin_f32(arad)+a.z*cos_f32(arad)); }
INLINE v3  v3_rot_z(v3 a, f32 arad)   { return v3m(a.x*cos_f32(arad)-a.y*sin_f32(arad),a.x*sin_f32(arad)+a.y*cos_f32(arad),a.z); }

typedef union v4
{
    struct { f32 x,y,z,w; };
    struct { f32 r,g,b,a; };
    f32 raw[4];
}v4;

#define v4_zero ((v4){{0, 0, 0, 0}})
#define v4_one ((v4){{1, 1, 1, 1}})
#define v4m(x, y, z, w)   ((v4){{x, y, z, w}})
INLINE v4  v4m_3(v3 a)                { return v4m(a.x,a.y,a.z,1); } // THIS SUCKS
INLINE v3  v3m_4(v4 a)                { return v3m(a.x,a.y,a.z); } // should be up SUCKS
INLINE v4  v4_add(v4 a, v4 b)         { return v4m(a.x+b.x,a.y+b.y,a.z+b.z,a.w+b.w); }
INLINE v4  v4_sub(v4 a, v4 b)         { return v4m(a.x-b.x,a.y-b.y,a.z-b.z,a.w-b.w); }
INLINE v4  v4_mult(v4 a, v4 b)        { return v4m(a.x*b.x,a.y*b.y,a.z*b.z,a.w*b.w); }
INLINE v4  v4_multf(v4 a, f32 b)      { return v4m(a.x*b,a.y*b,a.z*b,a.w*b); }
INLINE v4  v4_div(v4 a, v4 b)         { return v4m(a.x/b.x,a.y/b.y,a.z/b.z,a.w/b.w); }
INLINE v4  v4_divf(v4 a, f32 b)       { return v4m(a.x/b,a.y/b,a.z/b,a.w/b); }
INLINE v4  v4_lerp(v4 a, v4 b, f32 x) { return v4m(a.x*(1.0-x) + b.x*x,a.y*(1.0-x) + b.y*x,a.z*(1.0-x)+b.z*x,a.w*(1.0-x)+b.w*x); }
INLINE f32 v4_dot(v4 a, v4 b)         { return (a.x*b.x)+(a.y*b.y)+(a.z*b.z)+(a.w*b.w); }
INLINE f32 v4_len(v4 a)               { return sqrt_f32(v4_dot(a,a)); }
INLINE v4  v4_norm(v4 a)              { f32 vl=v4_len(a);assert(!equalf(vl,0.0,0.01));return v4_divf(a,vl); }
INLINE b32 v4_eq(v4 a, v4 b)          { return (equalf(a.x,b.x,0.001) && equalf(a.y,b.y,0.001) && equalf(a.z,b.z,0.001) && equalf(a.w,b.w,0.001)); }


// Vector convertion functions
INLINE v2  v2_from_v3(v3 a)        { return v2m(a.x, a.y); }
INLINE v2  v2_from_v4(v4 a)        { return v2m(a.x, a.y); }
INLINE v3  v3_from_v2(v2 a, f32 z) { return v3m(a.x, a.y, z); }
INLINE v3  v3_from_v4(v4 a)        { return v3m(a.x, a.y, a.z); }
INLINE v4  v4_from_v3(v3 a, f32 w) { return v4m(a.x, a.y, a.z, w); }

typedef union {
    f32 col[3][3];//{x.x,x.y,x.z,0,y.x,y.y,y.z,0,z.x,z.y,z.z,0,p.x,p.y,p.z,1}
    f32 raw[9]; //{x.x,x.y,x.z,0,y.x,y.y,y.z,0,z.x,z.y,z.z,0,p.x,p.y,p.z,1}
} m3;

typedef union {
    f32 col[4][4];//{x.x,x.y,x.z,0,y.x,y.y,y.z,0,z.x,z.y,z.z,0,p.x,p.y,p.z,1}
    f32 raw[16]; //{x.x,x.y,x.z,0,y.x,y.y,y.z,0,z.x,z.y,z.z,0,p.x,p.y,p.z,1}
} m4;

INLINE m4 m4d(f32 d) {
    m4 res = {};
    res.col[0][0] = d;
    res.col[1][1] = d;
    res.col[2][2] = d;
    res.col[3][3] = d;
    return res;
}

INLINE m4 m4_ortho(f32 l, f32 r, f32 b, f32 t, f32 n, f32 f) {
    m4 res = {};
    res.col[0][0] = 2.0f / (r - l);
    res.col[1][1] = 2.0f / (t - b);
    res.col[2][2] = 2.0f / (n - f);
    res.col[3][0] = (l + r) / (l - r);
    res.col[3][1] = (b + t) / (b - t);
    res.col[3][2] = (f + n) / (n - f);
    res.col[3][3] = 1.0f;
    return res;
}

// Investigate this, not sure its correct for our coordinate system, the sin is suspect
INLINE m4 m4_rotate(f32 angle, v3 axis) {
  m4 res = m4d(1.0f);

  axis = v3_norm(axis);

  f32 radians = (angle);
  f32 sinA = -sin_f32(radians);
  f32 cosA = cos_f32(radians);
  f32 t = 1.0f - cosA;

  res.col[0][0] = t * axis.x * axis.x + cosA;
  res.col[0][1] = t * axis.x * axis.y - axis.z * sinA;
  res.col[0][2] = t * axis.x * axis.z + axis.y * sinA;

  res.col[1][0] = t * axis.y * axis.x + axis.z * sinA;
  res.col[1][1] = t * axis.y * axis.y + cosA;
  res.col[1][2] = t * axis.y * axis.z - axis.x * sinA;

  res.col[2][0] = t * axis.z * axis.x - axis.y * sinA;
  res.col[2][1] = t * axis.z * axis.y + axis.x * sinA;
  res.col[2][2] = t * axis.z * axis.z + cosA;

  return res;
}

// World-Space -> View-Space
INLINE m4 _m4_look_at(v3 F, v3 S, v3 U, v3 Eye) {
  m4 m = {};

  m.col[0][0] = S.x;
  m.col[0][1] = U.x;
  m.col[0][2] = -F.x;
  m.col[0][3] = 0.0f;

  m.col[1][0] = S.y;
  m.col[1][1] = U.y;
  m.col[1][2] = -F.y;
  m.col[1][3] = 0.0f;

  m.col[2][0] = S.z;
  m.col[2][1] = U.z;
  m.col[2][2] = -F.z;
  m.col[2][3] = 0.0f;

  m.col[3][0] = -v3_dot(S, Eye);
  m.col[3][1] = -v3_dot(U, Eye);
  m.col[3][2] = v3_dot(F, Eye);
  m.col[3][3] = 1.0f;

  return m;
}
INLINE m4 m4_look_at(v3 eye, v3 center, v3 up) {
  v3 F = v3_norm(v3_sub(center, eye));
  v3 S = v3_norm(v3_cross(F, up));
  v3 U = v3_cross(S, F);

  return _m4_look_at(F, S, U, eye);
}


// View-Space -> Clip-Space
// fovx in degrees, aspect is w/h
// See https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/gluPerspective.xml
INLINE m4 m4_persp(f32 fovx, f32 aspect, f32 near, f32 far) {
  m4 m = {};

  float cot = 1.0f / tan(fovx / 2.0f);
  m.col[0][0] = cot / aspect;
  m.col[1][1] = cot;
  m.col[2][3] = -1.0f;

  m.col[2][2] = (near + far) / (near - far);
  m.col[3][2] = (2.0f * near * far) / (near - far);

  return m;
}

INLINE m4 m4_scale(v3 s) {
  m4 res = m4d(1.0f);
  res.col[0][0] = s.x;
  res.col[1][1] = s.y;
  res.col[2][2] = s.z;
  return res;
}

INLINE m4 m4_translate(v3 t) {
  m4 res = m4d(1.0f);
  res.col[3][0] = t.x;
  res.col[3][1] = t.y;
  res.col[3][2] = t.z;
  return res;
}

INLINE m4 m4_mult(m4 l, m4 r) {
  m4 res = m4d(1.0f);
  for (u32 col = 0; col < 4; col+=1) {
    for (u32 row = 0; row < 4; row+=1) {
      f32 sum = 0;
      for (u32 midx = 0; midx < 4; ++midx) {
        sum += (f32)l.col[midx][row] * (f32)r.col[col][midx];
      }
      res.col[col][row] = sum;
    }
  }
  return res;
}
INLINE v4 m4_multv(m4 m, v4 vec) {
  v4 res;
  for(s32 rows = 0; rows < 4; ++rows) {
    f32 s = 0;
    for(s32 cols = 0; cols < 4; ++cols) {
      s += m.col[cols][rows] * vec.raw[cols];
    }
    res.raw[rows] = s;
  }
  return (res);
}

INLINE m4 m4_transpose(m4 m) {
  m4 res = {};
  for (s32 col_idx = 0; col_idx < 4; ++col_idx) {
    for (s32 row_idx = 0; row_idx < 4; ++row_idx) {
      res.col[col_idx][row_idx] = m.col[row_idx][col_idx];
    }
  }
  return res;
}

INLINE m4 m4_inv(m4 m) {
  f32 det;
  m4 inv, inv_out;
  s32 i;

  inv.raw[0] = m.raw[5] * m.raw[10] * m.raw[15] - m.raw[5]  * m.raw[11] * m.raw[14] - m.raw[9]  * m.raw[6]  * m.raw[15] + m.raw[9]  * m.raw[7]  * m.raw[14] + m.raw[13] * m.raw[6]  * m.raw[11] - m.raw[13] * m.raw[7]  * m.raw[10];
  inv.raw[4] = -m.raw[4] * m.raw[10] * m.raw[15] + m.raw[4]  * m.raw[11] * m.raw[14] + m.raw[8]  * m.raw[6]  * m.raw[15] - m.raw[8]  * m.raw[7]  * m.raw[14] - m.raw[12] * m.raw[6]  * m.raw[11] + m.raw[12] * m.raw[7]  * m.raw[10];
  inv.raw[8] = m.raw[4] * m.raw[9] * m.raw[15] - m.raw[4]  * m.raw[11] * m.raw[13] - m.raw[8]  * m.raw[5] * m.raw[15] + m.raw[8]  * m.raw[7] * m.raw[13] + m.raw[12] * m.raw[5] * m.raw[11] - m.raw[12] * m.raw[7] * m.raw[9];
  inv.raw[12] = -m.raw[4] * m.raw[9] * m.raw[14] + m.raw[4]  * m.raw[10] * m.raw[13] + m.raw[8]  * m.raw[5] * m.raw[14] - m.raw[8]  * m.raw[6] * m.raw[13] - m.raw[12] * m.raw[5] * m.raw[10] + m.raw[12] * m.raw[6] * m.raw[9];
  inv.raw[1] = -m.raw[1] * m.raw[10] * m.raw[15] + m.raw[1]  * m.raw[11] * m.raw[14] + m.raw[9]  * m.raw[2] * m.raw[15] - m.raw[9]  * m.raw[3] * m.raw[14] - m.raw[13] * m.raw[2] * m.raw[11] + m.raw[13] * m.raw[3] * m.raw[10];
  inv.raw[5] = m.raw[0] * m.raw[10] * m.raw[15] - m.raw[0]  * m.raw[11] * m.raw[14] - m.raw[8]  * m.raw[2] * m.raw[15] + m.raw[8]  * m.raw[3] * m.raw[14] + m.raw[12] * m.raw[2] * m.raw[11] - m.raw[12] * m.raw[3] * m.raw[10];
  inv.raw[9] = -m.raw[0] * m.raw[9] * m.raw[15] + m.raw[0]  * m.raw[11] * m.raw[13] + m.raw[8]  * m.raw[1] * m.raw[15] - m.raw[8]  * m.raw[3] * m.raw[13] - m.raw[12] * m.raw[1] * m.raw[11] + m.raw[12] * m.raw[3] * m.raw[9];
  inv.raw[13] = m.raw[0] * m.raw[9] * m.raw[14] - m.raw[0]  * m.raw[10] * m.raw[13] - m.raw[8]  * m.raw[1] * m.raw[14] + m.raw[8]  * m.raw[2] * m.raw[13] + m.raw[12] * m.raw[1] * m.raw[10] - m.raw[12] * m.raw[2] * m.raw[9];
  inv.raw[2] = m.raw[1] * m.raw[6] * m.raw[15] - m.raw[1]  * m.raw[7] * m.raw[14] - m.raw[5]  * m.raw[2] * m.raw[15] + m.raw[5]  * m.raw[3] * m.raw[14] + m.raw[13] * m.raw[2] * m.raw[7] - m.raw[13] * m.raw[3] * m.raw[6];
  inv.raw[6] = -m.raw[0] * m.raw[6] * m.raw[15] + m.raw[0]  * m.raw[7] * m.raw[14] + m.raw[4]  * m.raw[2] * m.raw[15] - m.raw[4]  * m.raw[3] * m.raw[14] - m.raw[12] * m.raw[2] * m.raw[7] + m.raw[12] * m.raw[3] * m.raw[6];
  inv.raw[10] = m.raw[0] * m.raw[5] * m.raw[15] - m.raw[0]  * m.raw[7] * m.raw[13] - m.raw[4]  * m.raw[1] * m.raw[15] + m.raw[4]  * m.raw[3] * m.raw[13] + m.raw[12] * m.raw[1] * m.raw[7] - m.raw[12] * m.raw[3] * m.raw[5];
  inv.raw[14] = -m.raw[0] * m.raw[5] * m.raw[14] + m.raw[0]  * m.raw[6] * m.raw[13] + m.raw[4]  * m.raw[1] * m.raw[14] - m.raw[4]  * m.raw[2] * m.raw[13] - m.raw[12] * m.raw[1] * m.raw[6] + m.raw[12] * m.raw[2] * m.raw[5];
  inv.raw[3] = -m.raw[1] * m.raw[6] * m.raw[11] + m.raw[1] * m.raw[7] * m.raw[10] + m.raw[5] * m.raw[2] * m.raw[11] - m.raw[5] * m.raw[3] * m.raw[10] - m.raw[9] * m.raw[2] * m.raw[7] + m.raw[9] * m.raw[3] * m.raw[6];
  inv.raw[7] = m.raw[0] * m.raw[6] * m.raw[11] - m.raw[0] * m.raw[7] * m.raw[10] - m.raw[4] * m.raw[2] * m.raw[11] + m.raw[4] * m.raw[3] * m.raw[10] + m.raw[8] * m.raw[2] * m.raw[7] - m.raw[8] * m.raw[3] * m.raw[6];
  inv.raw[11] = -m.raw[0] * m.raw[5] * m.raw[11] + m.raw[0] * m.raw[7] * m.raw[9] + m.raw[4] * m.raw[1] * m.raw[11] - m.raw[4] * m.raw[3] * m.raw[9] - m.raw[8] * m.raw[1] * m.raw[7] + m.raw[8] * m.raw[3] * m.raw[5];
  inv.raw[15] = m.raw[0] * m.raw[5] * m.raw[10] - m.raw[0] * m.raw[6] * m.raw[9] - m.raw[4] * m.raw[1] * m.raw[10] + m.raw[4] * m.raw[2] * m.raw[9] + m.raw[8] * m.raw[1] * m.raw[6] - m.raw[8] * m.raw[2] * m.raw[5];

  det = m.raw[0] * inv.raw[0] + m.raw[1] * inv.raw[4] + m.raw[2] * inv.raw[8] + m.raw[3] * inv.raw[12];

  //in case the matrix is non-invertible
  if (det == 0) return m4d(0.f);

  det = 1.f / det;

  for (i = 0; i < 16; ++i) inv_out.raw[i] = inv.raw[i] * det;

  return inv_out;
}

typedef union {
#if 0
  struct {
    union {
      v3 xyz;
      struct {
        f32 x,y,z;
      };
    };
    f32 w;
  };
#endif
  struct {
    f32 x,y,z,w;
  };
  f32 raw[4];
} quat;

#define qu(x, y, z, w) ((quat){{x, y, z, w}})

static quat quat_norm(quat q) {
  v4 v = v4m(q.x, q.y, q.z, q.w);
  v = v4_norm(v);
  return qu(v.x, v.y, v.z, v.w);
}

static quat quat_inv(quat q) {
  return qu(-q.x, -q.y, -q.z, q.w);
}

static quat quat_sub(quat left, quat right) {
  quat q;
  q.x = left.x - right.x;
  q.y = left.y - right.y;
  q.z = left.z - right.z;
  q.w = left.w - right.w;
  return q;
}

static quat quat_add(quat left, quat right) {
  quat q;
  q.x = left.x + right.x;
  q.y = left.y + right.y;
  q.z = left.z + right.z;
  q.w = left.w + right.w;
  return q;
}

static quat quat_mult(quat left, quat right) {
  quat q;

  q.x =  right.raw[3] * +left.raw[0];
  q.y =  right.raw[2] * -left.raw[0];
  q.z =  right.raw[1] * +left.raw[0];
  q.w =  right.raw[0] * -left.raw[0];

  q.x += right.raw[2] * +left.raw[1];
  q.y += right.raw[3] * +left.raw[1];
  q.z += right.raw[0] * -left.raw[1];
  q.w += right.raw[1] * -left.raw[1];

  q.x += right.raw[1] * -left.raw[2];
  q.y += right.raw[0] * +left.raw[2];
  q.z += right.raw[3] * +left.raw[2];
  q.w += right.raw[2] * -left.raw[2];

  q.x += right.raw[0] * +left.raw[3];
  q.y += right.raw[1] * +left.raw[3];
  q.z += right.raw[2] * +left.raw[3];
  q.w += right.raw[3] * +left.raw[3];

  return q;
}

static quat quat_multf(quat left, f32 d) {
    quat q;
    q.x = left.x * d;
    q.y = left.y * d;
    q.z = left.z * d;
    q.w = left.w * d;
    return q;
}


static f32 quat_dot(quat a, quat b) {
  return (a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w);
}

static quat quat_scale(quat a, f32 d) {
  return qu(a.x*d, a.y*d, a.z*d, a.w*d);
}

static quat quat_from_axis_angle(v3 axis, f32 angle) {
  v3 axis_norm = v3_norm(axis);
  f32 sine_of_rot = sin_f32(angle/2.0f);
  v3 r = v3_multf(axis_norm, sine_of_rot);
  return qu(r.x, r.y, r.z, cos_f32(angle/2.0f));
}

static m4 m4_from_quat(quat q) {
  m4 m;
  quat norm_q = quat_norm(q);

  f32 xx, yy, zz, xy, xz, yz, wx, wy, wz;
  xx = norm_q.x * norm_q.x;
  yy = norm_q.y * norm_q.y;
  zz = norm_q.z * norm_q.z;
  xy = norm_q.x * norm_q.y;
  xz = norm_q.x * norm_q.z;
  yz = norm_q.y * norm_q.z;
  wx = norm_q.w * norm_q.x;
  wy = norm_q.w * norm_q.y;
  wz = norm_q.w * norm_q.z;

  m.col[0][0] = 1.0f - 2.0f * (yy + zz);
  m.col[0][1] = 2.0f * (xy + wz);
  m.col[0][2] = 2.0f * (xz - wy);
  m.col[0][3] = 0.0f;

  m.col[1][0] = 2.0f * (xy - wz);
  m.col[1][1] = 1.0f - 2.0f * (xx + zz);
  m.col[1][2] = 2.0f * (yz + wx);
  m.col[1][3] = 0.0f;

  m.col[2][0] = 2.0f * (xz + wy);
  m.col[2][1] = 2.0f * (yz - wx);
  m.col[2][2] = 1.0f - 2.0f * (xx + yy);
  m.col[2][3] = 0.0f;

  m.col[3][0] = 0.0f;
  m.col[3][1] = 0.0f;
  m.col[3][2] = 0.0f;
  m.col[3][3] = 1.0f;

  return m;
}

// https://d3cw3dd2w32x2b.cloudfront.net/wp-content/uploads/2015/01/matrix-to-quat.pdf
static quat quat_from_m4(m4 m) {
  f32 t;
  quat q;

  if (m.col[2][2] < 0.0f) {
    if (m.col[0][0] > m.col[1][1]) {
      t = 1 + m.col[0][0] - m.col[1][1] - m.col[2][2];
      q = qu(t, m.col[0][1]+m.col[1][0], m.col[2][0]+m.col[0][2], m.col[1][2]-m.col[2][1]);
    } else {
      t = 1 - m.col[0][0] + m.col[1][1] - m.col[2][2];
      q = qu(m.col[0][1] + m.col[1][0], t, m.col[1][2] + m.col[2][1], m.col[2][0] - m.col[0][2]);
    }
  } else {
    if (m.col[0][0] < -m.col[1][1]) {
      t = 1 - m.col[0][0] - m.col[1][1] + m.col[2][2];
      q = qu(m.col[2][0] + m.col[0][2], m.col[1][2] + m.col[2][1], t, m.col[0][1] - m.col[1][0]);
    } else {
      t = 1 + m.col[0][0] + m.col[1][1] + m.col[2][2];
      q = qu(m.col[1][2] - m.col[2][1], m.col[2][0] - m.col[0][2], m.col[0][1] - m.col[1][0], t);
    }
  }
  q = quat_multf(q, 0.5f / sqrtf(t));

  return q;
}

static quat quat_nlerp(quat a, quat b, f32 t) {
  // Make sure we take the shortest path.
  if (quat_dot(a, b) < 0.0f) {
    b = quat_scale(b, -1.0f);
  }

  quat q = quat_add(
    quat_scale(a, 1.0f - t),
    quat_scale(b, t)
  );

  return quat_norm(q);
}

static v3 m4_extract_trans(m4 m) {
  return v3m(m.col[3][0], m.col[3][1], m.col[3][2]);
}

static v3 m4_extract_scale(m4 m) {
  v3 scale;

  scale.x = v3_len(v3m(
    m.col[0][0],
    m.col[0][1],
    m.col[0][2]
  ));

  scale.y = v3_len(v3m(
    m.col[1][0],
    m.col[1][1],
    m.col[1][2]
  ));

  scale.z = v3_len(v3m(
    m.col[2][0],
    m.col[2][1],
    m.col[2][2]
  ));

  return scale;
}
static m4 m4_remove_scale(m4 m, v3 scale) {
  if (scale.x > 0.00001f) {
    for (s32 i = 0; i < 4; i+=1) {
    m.col[0][i] /= scale.x;
    }
  }
  if (scale.y > 0.00001f) {
    for (s32 i = 0; i < 4; i+=1) {
    m.col[1][i] /= scale.y;
    }
  }
  if (scale.y > 0.00001f) {
    for (s32 i = 0; i < 4; i+=1) {
    m.col[2][i] /= scale.z;
    }
  }
  return m;
}


typedef union {
  struct { f32 x,y,w,h; };
  struct { v2 p,dim; };
  f32 raw[4];
}rect;

static rect rec(f32 x, f32 y, f32 w, f32 h) {
  return (rect){{x,y,w,h}};
}

static rect rec_centered(v2 center, v2 hdim) {
  return (rect){{center.x-hdim.x, center.y-hdim.y, hdim.x*2, hdim.y*2}};
}
static b32 rect_isect_point(rect r, v2 p) {
  return ((p.x >= r.x) && (p.x <= r.x+r.w)) && ((p.y >= r.y) && (p.y <= r.y+r.h));
}

static b32 rect_isect_rect(rect a, rect b) {
    return !(a.x + a.w < b.x ||
             b.x + b.w < a.x ||
             a.y + a.h < b.y ||
             b.y + b.h < a.y);
}

static rect rect_clip_against(rect clipey, rect clipper) {
    f32 clipey_min_x = minimum(clipey.x, clipey.x + clipey.w);
    f32 clipey_max_x = maximum(clipey.x, clipey.x + clipey.w);
    f32 clipey_min_y = minimum(clipey.y, clipey.y + clipey.h);
    f32 clipey_max_y = maximum(clipey.y, clipey.y + clipey.h);

    f32 clipper_min_x = minimum(clipper.x, clipper.x + clipper.w);
    f32 clipper_max_x = maximum(clipper.x, clipper.x + clipper.w);
    f32 clipper_min_y = minimum(clipper.y, clipper.y + clipper.h);
    f32 clipper_max_y = maximum(clipper.y, clipper.y + clipper.h);

    f32 isec_min_x = maximum(clipey_min_x, clipper_min_x);
    f32 isec_max_x = minimum(clipey_max_x, clipper_max_x);
    f32 isec_min_y = maximum(clipey_min_y, clipper_min_y);
    f32 isec_max_y = minimum(clipey_max_y, clipper_max_y);

    rect final;
    final.x = isec_min_x;
    final.y = isec_min_y;
    final.w = maximum(0.0, isec_max_x - isec_min_x);
    final.h = maximum(0.0, isec_max_y - isec_min_y);

    return final;
}

static b32 rect_inside_rect(rect b, rect s) {
  return (rect_isect_point(b, v2m(s.x,     s.y)) &&
          rect_isect_point(b, v2m(s.x+s.w, s.y)) &&
          rect_isect_point(b, v2m(s.x,     s.y+s.h)) &&
          rect_isect_point(b, v2m(s.x+s.w, s.y+s.h)));
}

static rect rect_calc_bounding_rect(rect r0, rect r1) {
  v2 p0 = v2m(
    minimum(r0.x, minimum(r0.x+r0.w, minimum(r1.x, r1.x+r1.w))),
    minimum(r0.y, minimum(r0.y+r0.h, minimum(r1.y, r1.y+r1.h)))
  );

  v2 p1 = v2m(
    maximum(r0.x, maximum(r0.x+r0.w, maximum(r1.x, r1.x+r1.w))),
    maximum(r0.y, maximum(r0.y+r0.h, maximum(r1.y, r1.y+r1.h)))
  );

  return (rect) {
    .x = p0.x,
    .y = p0.y,
    .w = p1.x - p0.x,
    .h = p1.y - p0.y,
  };
}
static rect rect_add_radius(rect r, v2 radius) {
  r.x -= radius.x;
  r.y -= radius.y;
  r.w += 2.0*radius.x;
  r.h += 2.0*radius.y;

  return r;
}

typedef enum {
  RECT_FIT_MODE_LEFT,
  RECT_FIT_MODE_RIGHT,
  RECT_FIT_MODE_CENTER,
} Rect_Fit_Mode;

static rect rect_fit_inside(rect src, rect dest, Rect_Fit_Mode mode) {
  v2 p = v2m(0,0);
  switch(mode) {
    case RECT_FIT_MODE_LEFT: {
      v2 lp = v2m(dest.x, dest.y + dest.h/2.0);
      p = v2_sub(lp, v2m(0, src.h/2.0));
    }break;
    case RECT_FIT_MODE_RIGHT: {
      v2 rp = v2m(dest.x + dest.w, dest.y + dest.h/2.0);
      p = v2_sub(rp, v2m(src.w, src.h/2.0));
    }break;
    case RECT_FIT_MODE_CENTER: {
      v2 mp = v2m(dest.x + dest.w/2.0, dest.y + dest.h/2.0);
      p = v2_sub(mp, v2m(src.w/2.0, src.h/2.0));
    }break;
    default: break;
  }
  return (rect) {
    .x = p.x,
    .y = p.y,
    .w = src.w,
    .h = src.h,
  };
}

static b32 rect_equals(rect l, rect r) { return (equalf(l.x,r.x,0.01) && equalf(l.y,r.y,0.01) && equalf(l.w,r.w,0.01) && equalf(l.h,r.h,0.01)); }
static rect rect_bl_to_tl(rect r, f32 screen_height) { return rec(r.x, screen_height - r.y - r.h, r.w, r.h); }


typedef struct {
  v3 t;
  quat r;
  v3 s;
} transform;

static transform transform_from_m4(m4 m) {
  transform xform;
  xform.t = m4_extract_trans(m);
  xform.s = m4_extract_scale(m);

  m4 rot = m4_remove_scale(m, xform.s);
  xform.r = quat_from_m4(rot);

  return xform;
}

static m4 m4_from_transform(transform xform) {
  m4 trs = m4_mult(
      m4_translate(xform.t), 
      m4_mult(m4_from_quat(xform.r), 
        m4_scale(xform.s))
  ); 
  return trs;
}


// TODO: We should improve this ok?
typedef union iv4
{
    struct { s32 x,y,z,w; };
    struct { s32 r,g,b,a; };
    s32 raw[4];
}iv4;

#define iv4m(x, y, z, w)   ((iv4){{x, y, z, w}})

#endif
