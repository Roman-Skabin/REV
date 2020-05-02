//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "math/math.h"

#pragma warning(push)
#pragma warning(disable: 4244)

//
// v2
//

typedef union v2
{
    struct { f32 x, y; };
    struct { f32 w, h; };
} v2;

INLINE v2 MATH_CALL v2_0(f32 val     ) { return (v2){ val, val }; }
INLINE v2 MATH_CALL v2_1(f32 x, f32 y) { return (v2){   x,   y }; }

INLINE f32 MATH_CALL v2_dot(v2 l, v2 r) { return l.x*r.x + l.y*r.y; }

INLINE f32 MATH_CALL v2_length(v2 v)    { return sqrtf(v.x*v.x + v.y*v.y); }
INLINE f32 MATH_CALL v2_length_sq(v2 v) { return v.x*v.x + v.y*v.y; }

INLINE v2 MATH_CALL v2_add(v2 l, v2 r) { return (v2){ l.x + r.x, l.y + r.y }; }
INLINE v2 MATH_CALL v2_sub(v2 l, v2 r) { return (v2){ l.x - r.x, l.y - r.y }; }
INLINE v2 MATH_CALL v2_mul(v2 l, v2 r) { return (v2){ l.x * r.x, l.y * r.y }; }
INLINE v2 MATH_CALL v2_div(v2 l, v2 r) { return (v2){ l.x / r.x, l.y / r.y }; }

INLINE v2 MATH_CALL v2_add_s(v2 l, f32 r) { return (v2){ l.x + r, l.y + r }; }
INLINE v2 MATH_CALL v2_sub_s(v2 l, f32 r) { return (v2){ l.x - r, l.y - r }; }
INLINE v2 MATH_CALL v2_mul_s(v2 l, f32 r) { return (v2){ l.x * r, l.y * r }; }
INLINE v2 MATH_CALL v2_div_s(v2 l, f32 r) { return (v2){ l.x / r, l.y / r }; }

typedef union v2s
{
    struct { s32 x, y; };
    struct { s32 w, h; };
} v2s;

INLINE v2s MATH_CALL v2s_0(s32 val     ) { return (v2s){ val, val }; }
INLINE v2s MATH_CALL v2s_1(s32 x, s32 y) { return (v2s){   x,   y }; }

INLINE s32 MATH_CALL v2s_dot(v2s l, v2s r) { return l.x*r.x + l.y*r.y; }

INLINE f32 MATH_CALL v2s_length(v2s v)    { return sqrtf(cast(f32, v.x*v.x + v.y*v.y)); }
INLINE s32 MATH_CALL v2s_length_sq(v2s v) { return v.x*v.x + v.y*v.y;                   }

INLINE v2s MATH_CALL v2s_add(v2s l, v2s r) { return (v2s){ l.x + r.x, l.y + r.y }; }
INLINE v2s MATH_CALL v2s_sub(v2s l, v2s r) { return (v2s){ l.x - r.x, l.y - r.y }; }
INLINE v2s MATH_CALL v2s_mul(v2s l, v2s r) { return (v2s){ l.x * r.x, l.y * r.y }; }
INLINE v2s MATH_CALL v2s_div(v2s l, v2s r) { return (v2s){ l.x / r.x, l.y / r.y }; }

INLINE v2s MATH_CALL v2s_add_s(v2s l, s32 r) { return (v2s){ l.x + r, l.y + r }; }
INLINE v2s MATH_CALL v2s_sub_s(v2s l, s32 r) { return (v2s){ l.x - r, l.y - r }; }
INLINE v2s MATH_CALL v2s_mul_s(v2s l, s32 r) { return (v2s){ l.x * r, l.y * r }; }
INLINE v2s MATH_CALL v2s_div_s(v2s l, s32 r) { return (v2s){ l.x / r, l.y / r }; }

typedef union v2u
{
    struct { u32 x, y; };
    struct { u32 w, h; };
} v2u;

INLINE v2u MATH_CALL v2u_0(u32 val     ) { return (v2u){ val, val }; }
INLINE v2u MATH_CALL v2u_1(u32 x, u32 y) { return (v2u){ x, y }; }

INLINE u32 MATH_CALL v2u_dot(v2u l, v2u r) { return l.x*r.x + l.y*r.y; }

INLINE f32 MATH_CALL v2u_length(v2u vec) { return sqrtf(cast(f32, vec.x*vec.x + vec.y*vec.y)); }
INLINE u32 MATH_CALL v2u_length_sq(v2u vec) { return vec.x*vec.x + vec.y*vec.y; }

INLINE v2u MATH_CALL v2u_add(v2u l, v2u r) { return (v2u){ l.x + r.x, l.y + r.y }; }
INLINE v2u MATH_CALL v2u_sub(v2u l, v2u r) { return (v2u){ l.x - r.x, l.y - r.y }; }
INLINE v2u MATH_CALL v2u_mul(v2u l, v2u r) { return (v2u){ l.x * r.x, l.y * r.y }; }
INLINE v2u MATH_CALL v2u_div(v2u l, v2u r) { return (v2u){ l.x / r.x, l.y / r.y }; }

INLINE v2u MATH_CALL v2u_add_s(v2u l, u32 r) { return (v2u){ l.x + r, l.y + r }; }
INLINE v2u MATH_CALL v2u_sub_s(v2u l, u32 r) { return (v2u){ l.x - r, l.y - r }; }
INLINE v2u MATH_CALL v2u_mul_s(v2u l, u32 r) { return (v2u){ l.x * r, l.y * r }; }
INLINE v2u MATH_CALL v2u_div_s(v2u l, u32 r) { return (v2u){ l.x / r, l.y / r }; }


INLINE v2s MATH_CALL v2_to_v2s (v2  v) { return (v2s){ cast(s32, roundf(v.x)), cast(s32, roundf(v.y)) }; }
INLINE v2u MATH_CALL v2_to_v2u (v2  v) { return (v2u){ cast(u32, roundf(v.x)), cast(u32, roundf(v.y)) }; }
INLINE v2  MATH_CALL v2s_to_v2 (v2s v) { return (v2 ){ cast(f32,        v.x ), cast(f32,        v.y ) }; }
INLINE v2u MATH_CALL v2s_to_v2u(v2s v) { return (v2u){ cast(u32,        v.x ), cast(u32,        v.y ) }; }
INLINE v2  MATH_CALL v2u_to_v2 (v2u v) { return (v2 ){ cast(f32,        v.x ), cast(f32,        v.y ) }; }
INLINE v2s MATH_CALL v2u_to_v2s(v2u v) { return (v2s){ cast(s32,        v.x ), cast(s32,        v.y ) }; }

//
// v3
//

typedef union v3
{
    struct { f32 x, y, z; };
    v2 xy;
    __m128 mm;
} v3;

INLINE v3 MATH_CALL v3_0(f32 val            ) { v3 v; v.mm = _mm_setr_ps(val, val, val, 0.0f); return v; }
INLINE v3 MATH_CALL v3_1(f32 x, f32 y, f32 z) { v3 v; v.mm = _mm_setr_ps(   x,    y, z, 0.0f); return v; }
INLINE v3 MATH_CALL v3_2(v2 xy,        f32 z) { v3 v; v.mm = _mm_setr_ps(xy.x, xy.y, z, 0.0f); return v; }

INLINE f32 MATH_CALL v3_dot(v3 l, v3 r) { return _mm_cvtss_f32(_mm_dp_ps(l.mm, r.mm, 0x71)); }

INLINE f32 MATH_CALL v3_length(v3 v) { return sqrtf(v3_dot(v, v)); }
INLINE f32 MATH_CALL v3_length_sq(v3 v) { return v3_dot(v, v); }

INLINE v3 MATH_CALL v3_add(v3 l, v3 r) { v3 v; v.mm = _mm_add_ps(l.mm, r.mm); return v; }
INLINE v3 MATH_CALL v3_sub(v3 l, v3 r) { v3 v; v.mm = _mm_sub_ps(l.mm, r.mm); return v; }
INLINE v3 MATH_CALL v3_mul(v3 l, v3 r) { v3 v; v.mm = _mm_mul_ps(l.mm, r.mm); return v; }
INLINE v3 MATH_CALL v3_div(v3 l, v3 r) { v3 v; v.mm = _mm_div_ps(l.mm, r.mm); return v; }

INLINE v3 MATH_CALL v3_add_s(v3 l, f32 r) { v3 v; v.mm = _mm_add_ps(l.mm, _mm_setr_ps(r, r, r, 0.0f)); return v; }
INLINE v3 MATH_CALL v3_sub_s(v3 l, f32 r) { v3 v; v.mm = _mm_sub_ps(l.mm, _mm_setr_ps(r, r, r, 0.0f)); return v; }
INLINE v3 MATH_CALL v3_mul_s(v3 l, f32 r) { v3 v; v.mm = _mm_mul_ps(l.mm, _mm_setr_ps(r, r, r, 0.0f)); return v; }
INLINE v3 MATH_CALL v3_div_s(v3 l, f32 r) { v3 v; v.mm = _mm_div_ps(l.mm, _mm_setr_ps(r, r, r, 0.0f)); return v; }

INLINE v3 MATH_CALL v3_normalize_w(v3 v)
{
    return v.z && v.z != 1.0f ? v3_div_s(v, v.z) : v;
}

INLINE v3 MATH_CALL v3_normalize(v3 v)
{
    v = v3_div_s(v, v3_length(v));
    v.z = 1.0f;
    return v;
}

typedef union v3s
{
    struct { s32 x, y, z; };
    v2s xy;
    __m128i mm;
} v3s;

INLINE v3s MATH_CALL v3s_0(s32 val             ) { v3s v; v.mm = _mm_setr_epi32(val, val, val, 0); return v; }
INLINE v3s MATH_CALL v3s_1(s32  x, s32 y, s32 z) { v3s v; v.mm = _mm_setr_epi32(   x,    y, z, 0); return v; }
INLINE v3s MATH_CALL v3s_2(v2s xy,        s32 z) { v3s v; v.mm = _mm_setr_epi32(xy.x, xy.y, z, 0); return v; }

INLINE s32 MATH_CALL v3s_dot(v3s l, v3s r)
{
    __m128i m = _mm_mul_epi32(l.mm, r.mm);
    __m128i h = _mm_hadd_epi32(m, m);
    __m128i d = _mm_hadd_epi32(h, h);
#if 0
    return d.m128i_i32[0];
#else
    return _mm_cvtsi128_si32(d);
#endif
}

INLINE v3s MATH_CALL v3s_round(v3 v) { v3s r; r.mm = _mm_cvtps_epi32(v.mm); return r; }

INLINE f32 MATH_CALL v3s_length(v3s v) { return sqrtf(cast(f32, v3s_dot(v, v))); }
INLINE s32 MATH_CALL v3s_length_sq(v3s v) { return v3s_dot(v, v); }

INLINE v3s MATH_CALL v3s_add(v3s l, v3s r) { v3s v; v.mm = _mm_add_epi32(l.mm, r.mm); return v; }
INLINE v3s MATH_CALL v3s_sub(v3s l, v3s r) { v3s v; v.mm = _mm_sub_epi32(l.mm, r.mm); return v; }
INLINE v3s MATH_CALL v3s_mul(v3s l, v3s r) { v3s v; v.mm = _mm_mul_epi32(l.mm, r.mm); return v; }
INLINE v3s MATH_CALL v3s_div(v3s l, v3s r) { return v3s_round(v3_div(v3_1(l.x, l.y, l.z), v3_1(r.x, r.y, r.z))); }

INLINE v3s MATH_CALL v3s_add_s(v3s l, s32 r) { v3s v; v.mm = _mm_add_epi32(l.mm, _mm_setr_epi32(r, r, r, 0)); return v; }
INLINE v3s MATH_CALL v3s_sub_s(v3s l, s32 r) { v3s v; v.mm = _mm_sub_epi32(l.mm, _mm_setr_epi32(r, r, r, 0)); return v; }
INLINE v3s MATH_CALL v3s_mul_s(v3s l, s32 r) { v3s v; v.mm = _mm_mul_epi32(l.mm, _mm_setr_epi32(r, r, r, 0)); return v; }
INLINE v3s MATH_CALL v3s_div_s(v3s l, s32 r) { return v3s_round(v3_div_s(v3_1(l.x, l.y, l.z), cast(f32, r))); }

typedef union v3u
{
    struct { u32 x, y, z; };
    v2u xy;
    __m128i mm;
} v3u;

INLINE v3u MATH_CALL v3u_0(u32 val             ) { v3u v; v.mm = _mm_setr_epi32(val, val, val, 0); return v; }
INLINE v3u MATH_CALL v3u_1(u32  x, u32 y, u32 z) { v3u v; v.mm = _mm_setr_epi32(   x,    y, z, 0); return v; }
INLINE v3u MATH_CALL v3u_2(v2u xy,        u32 z) { v3u v; v.mm = _mm_setr_epi32(xy.x, xy.y, z, 0); return v; }

INLINE u32 MATH_CALL v3u_dot(v3u l, v3u r)
{
    __m128i m = _mm_mul_epi32(l.mm, r.mm);
    __m128i h = _mm_hadd_epi32(m, m);
    __m128i d = _mm_hadd_epi32(h, h);
#if 0
    return d.m128i_i32[0];
#else
    return _mm_cvtsi128_si32(d);
#endif
}

INLINE v3u MATH_CALL v3u_round(v3 v) { v3u r; r.mm = _mm_cvtps_epi32(v.mm); return r; }

INLINE f32 MATH_CALL v3u_length(v3u v) { return sqrtf(cast(f32, v3u_dot(v, v))); }
INLINE u32 MATH_CALL v3u_length_sq(v3u v) { return v3u_dot(v, v); }

INLINE v3u MATH_CALL v3u_add(v3u l, v3u r) { v3u v; v.mm = _mm_add_epi32(l.mm, r.mm); return v; }
INLINE v3u MATH_CALL v3u_sub(v3u l, v3u r) { v3u v; v.mm = _mm_sub_epi32(l.mm, r.mm); return v; }
INLINE v3u MATH_CALL v3u_mul(v3u l, v3u r) { v3u v; v.mm = _mm_mul_epi32(l.mm, r.mm); return v; }
INLINE v3u MATH_CALL v3u_div(v3u l, v3u r) { return v3u_round(v3_div(v3_1(l.x, l.y, l.z), v3_1(r.x, r.y, r.z))); }

INLINE v3u MATH_CALL v3u_add_s(v3u l, u32 r) { v3u v; v.mm = _mm_add_epi32(l.mm, _mm_setr_epi32(r, r, r, 0)); return v; }
INLINE v3u MATH_CALL v3u_sub_s(v3u l, u32 r) { v3u v; v.mm = _mm_sub_epi32(l.mm, _mm_setr_epi32(r, r, r, 0)); return v; }
INLINE v3u MATH_CALL v3u_mul_s(v3u l, u32 r) { v3u v; v.mm = _mm_mul_epi32(l.mm, _mm_setr_epi32(r, r, r, 0)); return v; }
INLINE v3u MATH_CALL v3u_div_s(v3u l, u32 r) { return v3u_round(v3_div_s(v3_1(l.x, l.y, l.z), cast(f32, r))); }


INLINE v3s MATH_CALL v3_to_v3s (v3  v) { v3s r; r.mm = _mm_cvtps_epi32(v.mm); return r; }
INLINE v3u MATH_CALL v3_to_v3u (v3  v) { v3u r; r.mm = _mm_cvtps_epi32(v.mm); return r; }
INLINE v3  MATH_CALL v3s_to_v3 (v3s v) { v3  r; r.mm = _mm_cvtepi32_ps(v.mm); return r; }
INLINE v3u MATH_CALL v3s_to_v3u(v3s v) { v3u r; r.mm =                 v.mm ; return r; }
INLINE v3  MATH_CALL v3u_to_v3 (v3u v) { v3  r; r.mm = _mm_cvtepi32_ps(v.mm); return r; }
INLINE v3s MATH_CALL v3u_to_v3s(v3u v) { v3s r; r.mm =                 v.mm ; return r; }

//
// v4
//

typedef union v4
{
    struct { f32 x, y, z, w; };
    struct { f32 r, g, b, a; };
    v2 xy;
    v3 xyz;
    __m128 mm;
} v4;

INLINE v4 MATH_CALL v4_0(f32 val                   ) { v4 v; v.mm = _mm_set_ps1(val); return v; }
INLINE v4 MATH_CALL v4_1(f32 x, f32 y, f32 z, f32 w) { v4 v; v.mm = _mm_setr_ps(   x,    y, z, w); return v; }
INLINE v4 MATH_CALL v4_2(v2 xy,        f32 z, f32 w) { v4 v; v.mm = _mm_setr_ps(xy.x, xy.y, z, w); return v; }
INLINE v4 MATH_CALL v4_3(v3 xyz,              f32 w) { v4 v; v.mm = _mm_move_ss(xyz.mm, _mm_load_ss(&w)); return v; }

INLINE f32 MATH_CALL v4_dot(v4 l, v4 r) { return _mm_cvtss_f32(_mm_dp_ps(l.mm, r.mm, 0xF1)); }

INLINE f32 MATH_CALL v4_length(v4 v)    { return sqrtf(v.x*v.x + v.y*v.y + v.z*v.z); }
INLINE f32 MATH_CALL v4_length_sq(v4 v) { return       v.x*v.x + v.y*v.y + v.z*v.z ; }

INLINE v4 MATH_CALL v4_add(v4 l, v4 r) { v4 v; v.mm = _mm_add_ps(l.mm, r.mm); return v; }
INLINE v4 MATH_CALL v4_sub(v4 l, v4 r) { v4 v; v.mm = _mm_sub_ps(l.mm, r.mm); return v; }
INLINE v4 MATH_CALL v4_mul(v4 l, v4 r) { v4 v; v.mm = _mm_mul_ps(l.mm, r.mm); return v; }
INLINE v4 MATH_CALL v4_div(v4 l, v4 r) { v4 v; v.mm = _mm_div_ps(l.mm, r.mm); return v; }

INLINE v4 MATH_CALL v4_add_s(v4 l, f32 r) { v4 v; v.mm = _mm_add_ps(l.mm, _mm_set_ps1(r)); return v; }
INLINE v4 MATH_CALL v4_sub_s(v4 l, f32 r) { v4 v; v.mm = _mm_sub_ps(l.mm, _mm_set_ps1(r)); return v; }
INLINE v4 MATH_CALL v4_mul_s(v4 l, f32 r) { v4 v; v.mm = _mm_mul_ps(l.mm, _mm_set_ps1(r)); return v; }
INLINE v4 MATH_CALL v4_div_s(v4 l, f32 r) { v4 v; v.mm = _mm_div_ps(l.mm, _mm_set_ps1(r)); return v; }

INLINE v4 MATH_CALL v4_normalize_w(v4 v)
{
    if (v.w && v.w != 1.0f)
        v = v4_div_s(v, v.w);
    v.w = 1.0f;
    return v;
}

INLINE v4 MATH_CALL v4_normalize(v4 v)
{
    v = v4_div_s(v, v4_length(v));
    v.w = 1.0f;
    return v;
}

INLINE v4 MATH_CALL v4_cross(v4 l, v4 r)
{
    v4 v;
    v.mm = _mm_sub_ps(
        _mm_mul_ps(
            _mm_shuffle_ps(l.mm, l.mm, _MM_SHUFFLE(3, 0, 2, 1)),
            _mm_shuffle_ps(r.mm, r.mm, _MM_SHUFFLE(3, 1, 0, 2))
        ),
        _mm_mul_ps(
            _mm_shuffle_ps(l.mm, l.mm, _MM_SHUFFLE(3, 1, 0, 2)),
            _mm_shuffle_ps(r.mm, r.mm, _MM_SHUFFLE(3, 0, 2, 1))
        )
    );
    return v;
}

INLINE v4 MATH_CALL v4_lerp(v4 start, v4 end, v4 percent)
{
    v4 out;
    out.mm = _mm_add_ps(
                 _mm_mul_ps(
                     _mm_sub_ps(end.mm, start.mm),
                     _mm_max_ps(
                         _mm_setzero_ps(),
                         _mm_min_ps(_mm_set_ps1(1.0f), percent.mm))),
                 start.mm);
    return out;
}

INLINE v4 MATH_CALL v4_lerp_s(f32 start, f32 end, v4 percent)
{
    __m128 low = _mm_set_ps1(start);
    v4 out;
    out.mm = _mm_add_ps(
                 _mm_mul_ps(
                     _mm_sub_ps(
                         _mm_set_ps1(end), low),
                     _mm_max_ps(
                         _mm_setzero_ps(),
                         _mm_min_ps(_mm_set_ps1(1.0f), percent.mm))),
                 low);
    return out;
}

typedef union v4s
{
    struct { s32 x, y, z, w; };
    struct { s32 r, g, b, a; };
    v2s xy;
    v3s xyz;
    __m128i mm;
} v4s;

INLINE v4s MATH_CALL v4s_0(s32 val                   ) { v4s v; v.mm = _mm_set1_epi32(val); return v; }
INLINE v4s MATH_CALL v4s_1(s32 x, s32 y, s32 z, s32 w) { v4s v; v.mm = _mm_setr_epi32(   x,    y, z, w); return v; }
INLINE v4s MATH_CALL v4s_2(v2s xy,       s32 z, s32 w) { v4s v; v.mm = _mm_setr_epi32(xy.x, xy.y, z, w); return v; }
INLINE v4s MATH_CALL v4s_3(v3s xyz,             s32 w) { v4s v; v.mm = _mm_load_si128(&xyz.mm), v.w = w; return v; }

INLINE s32 MATH_CALL v4s_dot(v4s l, v4s r)
{
    __m128i m = _mm_mul_epi32(l.mm, r.mm);
    __m128i h = _mm_hadd_epi32(m, m);
    __m128i d = _mm_hadd_epi32(h, h);
#if 0
    return d.m128i_i32[0];
#else
    return _mm_cvtsi128_si32(d);
#endif
}

INLINE v4s MATH_CALL v4s_round(v4 v) { v4s r; r.mm = _mm_cvtps_epi32(v.mm); return r; }

INLINE f32 MATH_CALL v4s_length(v4s v) { return sqrtf(cast(f32, v4s_dot(v, v))); }
INLINE s32 MATH_CALL v4s_length_sq(v4s v) { return v4s_dot(v, v); }

INLINE v4s MATH_CALL v4s_add(v4s l, v4s r) { v4s v; v.mm = _mm_add_epi32(l.mm, r.mm); return v; }
INLINE v4s MATH_CALL v4s_sub(v4s l, v4s r) { v4s v; v.mm = _mm_sub_epi32(l.mm, r.mm); return v; }
INLINE v4s MATH_CALL v4s_mul(v4s l, v4s r) { v4s v; v.mm = _mm_mul_epi32(l.mm, r.mm); return v; }
INLINE v4s MATH_CALL v4s_div(v4s l, v4s r) { return v4s_round(v4_div(v4_1(l.x, l.y, l.z, l.w), v4_1(r.x, r.y, r.z, l.w))); }

INLINE v4s MATH_CALL v4s_add_s(v4s l, s32 r) { v4s v; v.mm = _mm_add_epi32(l.mm, _mm_set1_epi32(r)); return v; }
INLINE v4s MATH_CALL v4s_sub_s(v4s l, s32 r) { v4s v; v.mm = _mm_sub_epi32(l.mm, _mm_set1_epi32(r)); return v; }
INLINE v4s MATH_CALL v4s_mul_s(v4s l, s32 r) { v4s v; v.mm = _mm_mul_epi32(l.mm, _mm_set1_epi32(r)); return v; }
INLINE v4s MATH_CALL v4s_div_s(v4s l, s32 r) { return v4s_round(v4_div_s(v4_1(l.x, l.y, l.z, l.w), cast(f32, r))); }

INLINE v4s MATH_CALL v4s_cross(v4s l, v4s r)
{
    v4s v;
    v.mm = _mm_sub_epi32(
        _mm_mul_epi32(
            _mm_shuffle_epi32(l.mm, _MM_SHUFFLE(3, 0, 2, 1)),
            _mm_shuffle_epi32(r.mm, _MM_SHUFFLE(3, 1, 0, 2))
        ),
        _mm_mul_epi32(
            _mm_shuffle_epi32(l.mm, _MM_SHUFFLE(3, 1, 0, 2)),
            _mm_shuffle_epi32(r.mm, _MM_SHUFFLE(3, 0, 2, 1))
        )
    );
    return v;
}

typedef union v4u
{
    struct { u32 x, y, z, w; };
    struct { u32 r, g, b, a; };
    v2u xy;
    v3u xyz;
    __m128i mm;
} v4u;

INLINE v4u MATH_CALL v4u_0(u32 val                   ) { v4u v; v.mm = _mm_set1_epi32(val); return v; }
INLINE v4u MATH_CALL v4u_1(u32 x, u32 y, u32 z, u32 w) { v4u v; v.mm = _mm_setr_epi32(   x,    y, z, w); return v; }
INLINE v4u MATH_CALL v4u_2(v2u xy,       u32 z, u32 w) { v4u v; v.mm = _mm_setr_epi32(xy.x, xy.y, z, w); return v; }
INLINE v4u MATH_CALL v4u_3(v3u xyz,             u32 w) { v4u v; v.mm = _mm_load_si128(&xyz.mm), v.w = w; return v; }

INLINE u32 MATH_CALL v4u_dot(v4u l, v4u r)
{
    __m128i m = _mm_mul_epi32(l.mm, r.mm);
    __m128i h = _mm_hadd_epi32(m, m);
    __m128i d = _mm_hadd_epi32(h, h);
#if 0
    return d.m128i_i32[0];
#else
    return _mm_cvtsi128_si32(d);
#endif
}

INLINE v4u MATH_CALL v4u_round(v4 v) { v4u r; r.mm = _mm_cvtps_epi32(v.mm); return r; }

INLINE f32 MATH_CALL v4u_length(v4u v) { return sqrtf(cast(f32, v4u_dot(v, v))); }
INLINE u32 MATH_CALL v4u_length_sq(v4u v) { return v4u_dot(v, v); }

INLINE v4u MATH_CALL v4u_add(v4u l, v4u r) { v4u v; v.mm = _mm_add_epi32(l.mm, r.mm); return v; }
INLINE v4u MATH_CALL v4u_sub(v4u l, v4u r) { v4u v; v.mm = _mm_sub_epi32(l.mm, r.mm); return v; }
INLINE v4u MATH_CALL v4u_mul(v4u l, v4u r) { v4u v; v.mm = _mm_mul_epi32(l.mm, r.mm); return v; }
INLINE v4u MATH_CALL v4u_div(v4u l, v4u r) { return v4u_round(v4_div(v4_1(l.x, l.y, l.z, l.w), v4_1(r.x, r.y, r.z, l.w))); }

INLINE v4u MATH_CALL v4u_add_s(v4u l, u32 r) { v4u v; v.mm = _mm_add_epi32(l.mm, _mm_set1_epi32(r)); return v; }
INLINE v4u MATH_CALL v4u_sub_s(v4u l, u32 r) { v4u v; v.mm = _mm_sub_epi32(l.mm, _mm_set1_epi32(r)); return v; }
INLINE v4u MATH_CALL v4u_mul_s(v4u l, u32 r) { v4u v; v.mm = _mm_mul_epi32(l.mm, _mm_set1_epi32(r)); return v; }
INLINE v4u MATH_CALL v4u_div_s(v4u l, u32 r) { return v4u_round(v4_div_s(v4_1(l.x, l.y, l.z, l.w), cast(f32, r))); }

INLINE v4u MATH_CALL v4u_cross(v4u l, v4u r)
{
    v4u v;
    v.mm = _mm_sub_epi32(
        _mm_mul_epi32(
            _mm_shuffle_epi32(l.mm, _MM_SHUFFLE(3, 0, 2, 1)),
            _mm_shuffle_epi32(r.mm, _MM_SHUFFLE(3, 1, 0, 2))
        ),
        _mm_mul_epi32(
            _mm_shuffle_epi32(l.mm, _MM_SHUFFLE(3, 1, 0, 2)),
            _mm_shuffle_epi32(r.mm, _MM_SHUFFLE(3, 0, 2, 1))
        )
    );
    return v;
}


INLINE v4s MATH_CALL v4_to_v4s (v4  v) { v4s r; r.mm = _mm_cvtps_epi32(v.mm); return r; }
INLINE v4u MATH_CALL v4_to_v4u (v4  v) { v4u r; r.mm = _mm_cvtps_epi32(v.mm); return r; }
INLINE v4  MATH_CALL v4s_to_v4 (v4s v) { v4  r; r.mm = _mm_cvtepi32_ps(v.mm); return r; }
INLINE v4u MATH_CALL v4s_to_v4u(v4s v) { v4u r; r.mm =                 v.mm ; return r; }
INLINE v4  MATH_CALL v4u_to_v4 (v4u v) { v4  r; r.mm = _mm_cvtepi32_ps(v.mm); return r; }
INLINE v4s MATH_CALL v4u_to_v4s(v4u v) { v4s r; r.mm =                 v.mm ; return r; }

#pragma warning(pop)
