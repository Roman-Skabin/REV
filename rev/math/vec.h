// Copyright (c) 2020-2021, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#pragma once

#include "math/math.h"
#include "math/simd.h"

#pragma pack(push, 1)
namespace REV::Math
{

//
// v2
//

union REV_INTRIN_TYPE v2 final
{
    struct { f32 x, y; };
    struct { f32 w, h; };

    REV_INLINE v2(f32 val = 0 ) : x(val),                   y(val)                   {}
    REV_INLINE v2(f32 x, f32 y) : x(x),                     y(y)                     {}
    REV_INLINE v2(f32 arr[2]  ) : x(*arr),                  y(arr[1])                {}
    REV_INLINE v2(const v2& v ) : x(v.x),                   y(v.y)                   {}
    REV_INLINE v2(v2&& v      ) : x(v.x),                   y(v.y)                   {}
    REV_INLINE v2(__m128 mm   ) : x(mm_extract_f32<0>(mm)), y(mm_extract_f32<1>(mm)) {}

    REV_INLINE v2& REV_VECTORCALL operator=(f32 val    ) { x = val;  y = val;    return *this; }
    REV_INLINE v2& REV_VECTORCALL operator=(f32 arr[2] ) { x = *arr; y = arr[1]; return *this; }
    REV_INLINE v2& REV_VECTORCALL operator=(const v2& v) { x = v.x;  y = v.y;    return *this; }
    REV_INLINE v2& REV_VECTORCALL operator=(v2&& v     ) { x = v.x;  y = v.y;    return *this; }

    REV_INLINE f32 REV_VECTORCALL dot(v2 r) const { return x*r.x + y*r.y; }

    REV_INLINE f32 REV_VECTORCALL length()    const { return sqrtf(x*x + y*y); }
    REV_INLINE f32 REV_VECTORCALL length_sq() const { return x*x + y*y;        }

    REV_INLINE v2 REV_VECTORCALL normalize() const
    {
        __m128 vec = _mm_setr_ps(x, y, 0.0f, 0.0f);
        __m128 len = _mm_sqrt_ps(_mm_dp_ps(vec, vec, 0x33));
        return v2(_mm_div_ps(vec, len));
    }

    REV_INLINE v2 REV_VECTORCALL project(v2 on_normal) const
    {
        f32 dp = x * on_normal.x + y * on_normal.y;
        return v2(dp * on_normal.x, dp * on_normal.y);
    }

    REV_INLINE v2 REV_VECTORCALL reflect(v2 normal) const
    {
        __m128 vect = _mm_setr_ps(x, y, 0.0f, 0.0f);
        __m128 norm = _mm_setr_ps(normal.x, normal.y, 0.0f, 0.0f);
        __m128 two  = _mm_set1_ps(2.0f);
        return v2(_mm_sub_ps(vect, _mm_mul_ps(_mm_mul_ps(two, _mm_dp_ps(norm, norm, 0x33)), norm)));
    }

    static REV_INLINE v2 REV_VECTORCALL lerp(v2 start, v2 end, v2 percent)
    {
        return v2((end.x - start.x) * percent.x + start.x,
                  (end.y - start.y) * percent.y + start.y);
    }

    static REV_INLINE v2 REV_VECTORCALL invlerp(v2 start, v2 end, v2 value) // ret = [0, 1]
    {
        __m128 mm_val   = _mm_setr_ps(value.x, value.y, 0.0f, 0.0f);
        __m128 mm_start = _mm_setr_ps(start.x, start.y, 0.0f, 0.0f);
        __m128 mm_end   = _mm_setr_ps(end.x,   end.y,   0.0f, 0.0f);
        return v2(_mm_div_ps(_mm_sub_ps(mm_val, mm_start), _mm_sub_ps(mm_end, mm_start)));
    }

    static REV_INLINE v2 REV_VECTORCALL invlerp_n(v2 start, v2 end, v2 value) // ret = [-1, 1]
    {
        __m128 mm_val   = _mm_setr_ps(value.x, value.y, 0.0f, 0.0f);
        __m128 mm_start = _mm_setr_ps(start.x, start.y, 0.0f, 0.0f);
        __m128 mm_end   = _mm_setr_ps(end.x,   end.y,   0.0f, 0.0f);
        __m128 mm_0_5   = _mm_set1_ps(0.5f);
        __m128 mm_2_0   = _mm_set1_ps(2.0f);
        return v2(_mm_mul_ps(_mm_sub_ps(_mm_div_ps(_mm_sub_ps(mm_val, mm_start), _mm_sub_ps(mm_end, mm_start)), mm_0_5), mm_2_0));
    }

    static REV_INLINE v2 REV_VECTORCALL clamp(v2 val, v2 min, v2 max)
    {
        __m128 mm_val = _mm_setr_ps(val.x, val.y, 0.0f, 0.0f);
        __m128 mm_min = _mm_setr_ps(min.x, min.y, 0.0f, 0.0f);
        __m128 mm_max = _mm_setr_ps(max.x, max.y, 0.0f, 0.0f);
        return v2(_mm_max_ps(mm_min, _mm_min_ps(mm_val, mm_max)));
    }

    REV_INLINE v2& REV_VECTORCALL operator+=(v2 r)  { x += r.x; y += r.y; return *this; }
    REV_INLINE v2& REV_VECTORCALL operator-=(v2 r)  { x -= r.x; y -= r.y; return *this; }
    REV_INLINE v2& REV_VECTORCALL operator*=(v2 r)  { x *= r.x; y *= r.y; return *this; }
    REV_INLINE v2& REV_VECTORCALL operator/=(v2 r)  { x /= r.x; y /= r.y; return *this; }

    REV_INLINE v2& REV_VECTORCALL operator+=(f32 r) { x += r;   y += r;   return *this; }
    REV_INLINE v2& REV_VECTORCALL operator-=(f32 r) { x -= r;   y -= r;   return *this; }
    REV_INLINE v2& REV_VECTORCALL operator*=(f32 r) { x *= r;   y *= r;   return *this; }
    REV_INLINE v2& REV_VECTORCALL operator/=(f32 r) { x /= r;   y /= r;   return *this; }
};

REV_INLINE v2 REV_VECTORCALL operator+(v2 l, v2 r) { return v2(l.x + r.x, l.y + r.y); }
REV_INLINE v2 REV_VECTORCALL operator-(v2 l, v2 r) { return v2(l.x - r.x, l.y - r.y); }
REV_INLINE v2 REV_VECTORCALL operator*(v2 l, v2 r) { return v2(l.x * r.x, l.y * r.y); }
REV_INLINE v2 REV_VECTORCALL operator/(v2 l, v2 r) { return v2(l.x / r.x, l.y / r.y); }

REV_INLINE v2 REV_VECTORCALL operator+(v2 l, f32 r) { return v2(l.x + r, l.y + r); }
REV_INLINE v2 REV_VECTORCALL operator-(v2 l, f32 r) { return v2(l.x - r, l.y - r); }
REV_INLINE v2 REV_VECTORCALL operator*(v2 l, f32 r) { return v2(l.x * r, l.y * r); }
REV_INLINE v2 REV_VECTORCALL operator/(v2 l, f32 r) { return v2(l.x / r, l.y / r); }

REV_INLINE v2 REV_VECTORCALL operator+(f32 l, v2 r) { return v2(l + r.x, l + r.y); }
REV_INLINE v2 REV_VECTORCALL operator-(f32 l, v2 r) { return v2(l - r.x, l - r.y); }
REV_INLINE v2 REV_VECTORCALL operator*(f32 l, v2 r) { return v2(l * r.x, l * r.y); }
REV_INLINE v2 REV_VECTORCALL operator/(f32 l, v2 r) { return v2(l / r.x, l / r.y); }

REV_INLINE bool REV_VECTORCALL operator==(v2 l, v2 r) { return l.x == r.x && l.y == r.y; }
REV_INLINE bool REV_VECTORCALL operator!=(v2 l, v2 r) { return l.x != r.x || l.y != r.y; }

// @NOTE(Roman):
// sqrt(x1^2 + y1^2)   <=> sqrt(x2^2 + y2^2)
// sqrt(x1^2 + y1^2)^2 <=> sqrt(x2^2 + y2^2)^2
//      x1^2 + y1^2    <=>      x2^2 + y2^2
REV_INLINE bool REV_VECTORCALL operator<=(v2 l, v2 r) { return l.length_sq() <= r.length_sq(); }
REV_INLINE bool REV_VECTORCALL operator>=(v2 l, v2 r) { return l.length_sq() >= r.length_sq(); }
REV_INLINE bool REV_VECTORCALL operator< (v2 l, v2 r) { return l.length_sq() <  r.length_sq(); }
REV_INLINE bool REV_VECTORCALL operator> (v2 l, v2 r) { return l.length_sq() >  r.length_sq(); }

union REV_INTRIN_TYPE v2s final
{
    struct { s32 x, y; };
    struct { s32 w, h; };

    REV_INLINE v2s(s32 val = 0 ) : x(val),                   y(val)                   {}
    REV_INLINE v2s(s32 x, s32 y) : x(x),                     y(y)                     {}
    REV_INLINE v2s(s32 arr[2]  ) : x(*arr),                  y(arr[1])                {}
    REV_INLINE v2s(const v2s& v) : x(v.x),                   y(v.y)                   {}
    REV_INLINE v2s(v2s&& v     ) : x(v.x),                   y(v.y)                   {}
    REV_INLINE v2s(__m128i mm  ) : x(mm_extract_s32<0>(mm)), y(mm_extract_s32<1>(mm)) {}

    REV_INLINE v2s& REV_VECTORCALL operator=(s32 val     ) { x = val;  y = val;    return *this; }
    REV_INLINE v2s& REV_VECTORCALL operator=(s32 arr[2]  ) { x = *arr; y = arr[1]; return *this; }
    REV_INLINE v2s& REV_VECTORCALL operator=(const v2s& v) { x = v.x;  y = v.y;    return *this; }
    REV_INLINE v2s& REV_VECTORCALL operator=(v2s&& v     ) { x = v.x;  y = v.y;    return *this; }

    REV_INLINE s32 REV_VECTORCALL dot(v2s r) const { return x*r.x + y*r.y; }

    REV_INLINE f32 REV_VECTORCALL length()    const { return sqrtf(cast(f32, x*x + y*y)); }
    REV_INLINE s32 REV_VECTORCALL length_sq() const { return x*x + y*y;                   }

    static REV_INLINE v2s REV_VECTORCALL clamp(v2s val, v2s min, v2s max)
    {
        __m128i mm_val = _mm_setr_epi32(val.x, val.y, 0, 0);
        __m128i mm_min = _mm_setr_epi32(min.x, min.y, 0, 0);
        __m128i mm_max = _mm_setr_epi32(max.x, max.y, 0, 0);
        return v2s(_mm_max_epi32(mm_min, _mm_min_epi32(mm_val, mm_max)));
    }

    REV_INLINE v2s& REV_VECTORCALL operator+=(v2s r) { x += r.x; y += r.y; return *this; }
    REV_INLINE v2s& REV_VECTORCALL operator-=(v2s r) { x -= r.x; y -= r.y; return *this; }
    REV_INLINE v2s& REV_VECTORCALL operator*=(v2s r) { x *= r.x; y *= r.y; return *this; }
    REV_INLINE v2s& REV_VECTORCALL operator/=(v2s r) { x /= r.x; y /= r.y; return *this; }

    REV_INLINE v2s& REV_VECTORCALL operator+=(s32 r) { x += r;   y += r;   return *this; }
    REV_INLINE v2s& REV_VECTORCALL operator-=(s32 r) { x -= r;   y -= r;   return *this; }
    REV_INLINE v2s& REV_VECTORCALL operator*=(s32 r) { x *= r;   y *= r;   return *this; }
    REV_INLINE v2s& REV_VECTORCALL operator/=(s32 r) { x /= r;   y /= r;   return *this; }
};

REV_INLINE v2s REV_VECTORCALL operator+(v2s l, v2s r) { return v2s(l.x + r.x, l.y + r.y); }
REV_INLINE v2s REV_VECTORCALL operator-(v2s l, v2s r) { return v2s(l.x - r.x, l.y - r.y); }
REV_INLINE v2s REV_VECTORCALL operator*(v2s l, v2s r) { return v2s(l.x * r.x, l.y * r.y); }
REV_INLINE v2s REV_VECTORCALL operator/(v2s l, v2s r) { return v2s(l.x / r.x, l.y / r.y); }

REV_INLINE v2s REV_VECTORCALL operator+(v2s l, s32 r) { return v2s(l.x + r, l.y + r); }
REV_INLINE v2s REV_VECTORCALL operator-(v2s l, s32 r) { return v2s(l.x - r, l.y - r); }
REV_INLINE v2s REV_VECTORCALL operator*(v2s l, s32 r) { return v2s(l.x * r, l.y * r); }
REV_INLINE v2s REV_VECTORCALL operator/(v2s l, s32 r) { return v2s(l.x / r, l.y / r); }

REV_INLINE v2s REV_VECTORCALL operator+(s32 l, v2s r) { return v2s(l + r.x, l + r.y); }
REV_INLINE v2s REV_VECTORCALL operator-(s32 l, v2s r) { return v2s(l - r.x, l - r.y); }
REV_INLINE v2s REV_VECTORCALL operator*(s32 l, v2s r) { return v2s(l * r.x, l * r.y); }
REV_INLINE v2s REV_VECTORCALL operator/(s32 l, v2s r) { return v2s(l / r.x, l / r.y); }

REV_INLINE bool REV_VECTORCALL operator==(v2s l, v2s r) { return l.x == r.x && l.y == r.y; }
REV_INLINE bool REV_VECTORCALL operator!=(v2s l, v2s r) { return l.x != r.x || l.y != r.y; }

// @NOTE(Roman):
// sqrt(x1^2 + y1^2)   <=> sqrt(x2^2 + y2^2)
// sqrt(x1^2 + y1^2)^2 <=> sqrt(x2^2 + y2^2)^2
//      x1^2 + y1^2    <=>      x2^2 + y2^2
REV_INLINE bool REV_VECTORCALL operator<=(v2s l, v2s r) { return l.length_sq() <= r.length_sq(); }
REV_INLINE bool REV_VECTORCALL operator>=(v2s l, v2s r) { return l.length_sq() >= r.length_sq(); }
REV_INLINE bool REV_VECTORCALL operator< (v2s l, v2s r) { return l.length_sq() <  r.length_sq(); }
REV_INLINE bool REV_VECTORCALL operator> (v2s l, v2s r) { return l.length_sq() >  r.length_sq(); }

union REV_INTRIN_TYPE v2u final
{
    struct { u32 x, y; };
    struct { u32 w, h; };
    struct { u32 r, c; }; // row, col

    REV_INLINE v2u(u32 val = 0 ) : x(val),                   y(val)                   {}
    REV_INLINE v2u(u32 x, u32 y) : x(x),                     y(y)                     {}
    REV_INLINE v2u(u32 arr[2]  ) : x(*arr),                  y(arr[1])                {}
    REV_INLINE v2u(const v2u& v) : x(v.x),                   y(v.y)                   {}
    REV_INLINE v2u(v2u&& v     ) : x(v.x),                   y(v.y)                   {}
    REV_INLINE v2u(__m128i mm  ) : x(mm_extract_u32<0>(mm)), y(mm_extract_u32<1>(mm)) {}

    REV_INLINE v2u& REV_VECTORCALL operator=(u32 val     ) { x = val;  y = val;    return *this; }
    REV_INLINE v2u& REV_VECTORCALL operator=(u32 arr[2]  ) { x = *arr; y = arr[1]; return *this; }
    REV_INLINE v2u& REV_VECTORCALL operator=(const v2u& v) { x = v.x;  y = v.y;    return *this; }
    REV_INLINE v2u& REV_VECTORCALL operator=(v2u&& v     ) { x = v.x;  y = v.y;    return *this; }
    
    REV_INLINE u32 REV_VECTORCALL dot(v2u r) const { return x*r.x + y*r.y; }

    REV_INLINE f32 REV_VECTORCALL length()    const { return sqrtf(cast(f32, x*x + y*y)); }
    REV_INLINE u32 REV_VECTORCALL length_sq() const { return x*x + y*y; }

    static REV_INLINE v2u REV_VECTORCALL clamp(v2u val, v2u min, v2u max)
    {
        __m128i mm_val = _mm_setr_epi32(val.x, val.y, 0, 0);
        __m128i mm_min = _mm_setr_epi32(min.x, min.y, 0, 0);
        __m128i mm_max = _mm_setr_epi32(max.x, max.y, 0, 0);
        return v2u(_mm_max_epu32(mm_min, _mm_min_epu32(mm_val, mm_max)));
    }

    REV_INLINE v2u& REV_VECTORCALL operator+=(v2u r) { x += r.x; y += r.y; return *this; }
    REV_INLINE v2u& REV_VECTORCALL operator-=(v2u r) { x -= r.x; y -= r.y; return *this; }
    REV_INLINE v2u& REV_VECTORCALL operator*=(v2u r) { x *= r.x; y *= r.y; return *this; }
    REV_INLINE v2u& REV_VECTORCALL operator/=(v2u r) { x /= r.x; y /= r.y; return *this; }

    REV_INLINE v2u& REV_VECTORCALL operator+=(u32 r) { x += r;   y += r;   return *this; }
    REV_INLINE v2u& REV_VECTORCALL operator-=(u32 r) { x -= r;   y -= r;   return *this; }
    REV_INLINE v2u& REV_VECTORCALL operator*=(u32 r) { x *= r;   y *= r;   return *this; }
    REV_INLINE v2u& REV_VECTORCALL operator/=(u32 r) { x /= r;   y /= r;   return *this; }
};

REV_INLINE v2u REV_VECTORCALL operator+(v2u l, v2u r) { return v2u(l.x + r.x, l.y + r.y); }
REV_INLINE v2u REV_VECTORCALL operator-(v2u l, v2u r) { return v2u(l.x - r.x, l.y - r.y); }
REV_INLINE v2u REV_VECTORCALL operator*(v2u l, v2u r) { return v2u(l.x * r.x, l.y * r.y); }
REV_INLINE v2u REV_VECTORCALL operator/(v2u l, v2u r) { return v2u(l.x / r.x, l.y / r.y); }

REV_INLINE v2u REV_VECTORCALL operator+(v2u l, u32 r) { return v2u(l.x + r, l.y + r); }
REV_INLINE v2u REV_VECTORCALL operator-(v2u l, u32 r) { return v2u(l.x - r, l.y - r); }
REV_INLINE v2u REV_VECTORCALL operator*(v2u l, u32 r) { return v2u(l.x * r, l.y * r); }
REV_INLINE v2u REV_VECTORCALL operator/(v2u l, u32 r) { return v2u(l.x / r, l.y / r); }

REV_INLINE v2u REV_VECTORCALL operator+(u32 l, v2u r) { return v2u(l + r.x, l + r.y); }
REV_INLINE v2u REV_VECTORCALL operator-(u32 l, v2u r) { return v2u(l - r.x, l - r.y); }
REV_INLINE v2u REV_VECTORCALL operator*(u32 l, v2u r) { return v2u(l * r.x, l * r.y); }
REV_INLINE v2u REV_VECTORCALL operator/(u32 l, v2u r) { return v2u(l / r.x, l / r.y); }

REV_INLINE bool REV_VECTORCALL operator==(v2u l, v2u r) { return l.x == r.x && l.y == r.y; }
REV_INLINE bool REV_VECTORCALL operator!=(v2u l, v2u r) { return l.x != r.x || l.y != r.y; }

// @NOTE(Roman):
// sqrt(x1^2 + y1^2)   <=> sqrt(x2^2 + y2^2)
// sqrt(x1^2 + y1^2)^2 <=> sqrt(x2^2 + y2^2)^2
//      x1^2 + y1^2    <=>      x2^2 + y2^2
REV_INLINE bool REV_VECTORCALL operator<=(v2u l, v2u r) { return l.length_sq() <= r.length_sq(); }
REV_INLINE bool REV_VECTORCALL operator>=(v2u l, v2u r) { return l.length_sq() >= r.length_sq(); }
REV_INLINE bool REV_VECTORCALL operator< (v2u l, v2u r) { return l.length_sq() <  r.length_sq(); }
REV_INLINE bool REV_VECTORCALL operator> (v2u l, v2u r) { return l.length_sq() >  r.length_sq(); }


REV_INLINE v2s REV_VECTORCALL v2_to_v2s (v2  v) { return v2s(cast(s32, roundf(v.x)), cast(s32, roundf(v.y))); }
REV_INLINE v2u REV_VECTORCALL v2_to_v2u (v2  v) { return v2u(cast(u32, roundf(v.x)), cast(u32, roundf(v.y))); }
REV_INLINE v2  REV_VECTORCALL v2s_to_v2 (v2s v) { return v2 (cast(f32,        v.x ), cast(f32,        v.y )); }
REV_INLINE v2u REV_VECTORCALL v2s_to_v2u(v2s v) { return v2u(cast(u32,        v.x ), cast(u32,        v.y )); }
REV_INLINE v2  REV_VECTORCALL v2u_to_v2 (v2u v) { return v2 (cast(f32,        v.x ), cast(f32,        v.y )); }
REV_INLINE v2s REV_VECTORCALL v2u_to_v2s(v2u v) { return v2s(cast(s32,        v.x ), cast(s32,        v.y )); }

//
// v3
//

union REV_INTRIN_TYPE v3 final
{
    struct { f32 x, y, z; };
    struct { f32 r, g, b; };
    v2   xy;
    v2   rg;
    f32  e[3];

    REV_INLINE v3(                   ) : x(0),      y(0),      z(0)      {}
    REV_INLINE v3(f32 val            ) : x(val),    y(val),    z(val)    {}
    REV_INLINE v3(f32 x, f32 y, f32 z) : x(x),      y(y),      z(z)      {}
    REV_INLINE v3(f32 arr[3]         ) : x(arr[0]), y(arr[1]), z(arr[2]) {}
    REV_INLINE v3(v2 xy, f32 z       ) : x(xy.x),   y(xy.y),   z(z)      {}
    REV_INLINE v3(const v3& v        ) : x(v.x),    y(v.y),    z(v.z)    {}
    REV_INLINE v3(v3&& v             ) : x(v.x),    y(v.y),    z(v.z)    {}
    REV_INLINE v3(__m128 xmm         ) { store(xmm); }

    REV_INLINE v3& REV_VECTORCALL operator=(f32 val    ) { x = val;    y = val;    z = val;    return *this; }
    REV_INLINE v3& REV_VECTORCALL operator=(f32 arr[3] ) { x = arr[0]; y = arr[1]; z = arr[2]; return *this; }
    REV_INLINE v3& REV_VECTORCALL operator=(const v3& v) { x = v.x;    y = v.y;    z = v.z;    return *this; }
    REV_INLINE v3& REV_VECTORCALL operator=(v3&& v     ) { x = v.x;    y = v.y;    z = v.z;    return *this; }
    REV_INLINE v3& REV_VECTORCALL operator=(__m128 xmm ) { store(xmm);                         return *this; }

    REV_INLINE f32 REV_VECTORCALL dot(v3 r) const
    {
        return _mm_cvtss_f32(_mm_dp_ps(load(), r.load(), 0x71));
    }

    REV_INLINE v3 REV_VECTORCALL cross(v3 r) const
    {
        //  [x]   [r.x]   [ (y * r.z - z * r.y)]   [y * r.z - z * r.y]
        //  [y] x [r.y] = [-(x * r.z - z * r.x)] = [z * r.x - x * r.z];
        //  [z]   [r.z]   [ (x * r.y - y * r.x)]   [x * r.y - y * r.x]
        
        __m128 l_mm = load();
        __m128 r_mm = r.load();

        return v3(_mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(l_mm, l_mm, MM_SHUFFLE_YZXW),
                                        _mm_shuffle_ps(r_mm, r_mm, MM_SHUFFLE_ZXYW)),
                             _mm_mul_ps(_mm_shuffle_ps(l_mm, l_mm, MM_SHUFFLE_ZXYW),
                                        _mm_shuffle_ps(r_mm, r_mm, MM_SHUFFLE_YZXW))));
    }

    REV_INLINE f32 REV_VECTORCALL length() const
    {
        __m128 mm = load();
        return _mm_cvtss_f32(_mm_sqrt_ss(_mm_dp_ps(mm, mm, 0x71)));
    }

    REV_INLINE f32 REV_VECTORCALL length_sq() const
    {
        __m128 mm = load();
        return _mm_cvtss_f32(_mm_dp_ps(mm, mm, 0x71));
    }

    REV_INLINE v3 REV_VECTORCALL normalize() const
    {
        __m128 mm = load();
        return v3(_mm_div_ps(mm, _mm_sqrt_ps(_mm_dp_ps(mm, mm, 0x7F))));
    }

    REV_INLINE v3 REV_VECTORCALL project(v3 on_normal) const
    {
        __m128 on_mm = on_normal.load();
        return v3(_mm_mul_ps(_mm_dp_ps(load(), on_mm, 0x7F), on_mm));
    }

    REV_INLINE v3 REV_VECTORCALL reflect(v3 normal) const
    {
        __m128 dp  = _mm_dp_ps(load(), normal.load(), 0x7F);
        __m128 _2p = _mm_mul_ps(_mm_set_ps1(2.0f), _mm_mul_ps(dp, normal.load()));
        return v3(_mm_sub_ps(load(), _2p));
    }

    static REV_INLINE v3 REV_VECTORCALL lerp(v3 start, v3 end, v3 percent)
    {
        __m128 s_mm = start.load();
        return v3(_mm_add_ps(_mm_mul_ps(_mm_sub_ps(end.load(), s_mm), percent.load()), s_mm));
    }

    static REV_INLINE v3 REV_VECTORCALL invlerp(v3 start, v3 end, v3 value) // ret = [0, 1]
    {
        __m128 mm_start = start.load();
        __m128 mm_end   = end.load();
        __m128 mm_value = value.load();
        return v3(_mm_div_ps(_mm_sub_ps(mm_value, mm_start), _mm_sub_ps(mm_end, mm_start)));
    }

    static REV_INLINE v3 REV_VECTORCALL invlerp_n(v3 start, v3 end, v3 value) // ret = [-1, 1]
    {
        __m128 mm_start = start.load();
        __m128 mm_end   = end.load();
        __m128 mm_value = value.load();
        return v3(_mm_mul_ps(_mm_sub_ps(_mm_div_ps(_mm_sub_ps(mm_value, mm_start), _mm_sub_ps(mm_end, mm_start)), _mm_set_ps1(0.5f)), _mm_set_ps1(2.0f)));
    }

    static REV_INLINE v3 REV_VECTORCALL clamp(v3 val, v3 min, v3 max)
    {
        return v3(_mm_max_ps(min.load(), _mm_min_ps(max.load(), val.load())));
    }

    REV_INLINE __m128 REV_VECTORCALL load() const
    {
    #if REV_ISA >= REV_ISA_AVX512
        return _mm_mask_load_ps(e, 0b0111);
    #elif REV_ISA >= REV_ISA_AVX2
        return _mm_maskload_ps(e, _mm_setr_epi32(REV_U32_MAX, REV_U32_MAX, REV_U32_MAX, 0));
    #else
        return _mm_setr_ps(x, y, z, 0.0f);
    #endif
    }

    REV_INLINE void REV_VECTORCALL store(__m128 xmm)
    {
    #if REV_ISA >= REV_ISA_AVX512
        _mm_mask_store_ps(e, 0b0111, xmm);
    #elif REV_ISA >= REV_ISA_AVX2
        _mm_maskstore_ps(e, _mm_setr_epi32(REV_U32_MAX, REV_U32_MAX, REV_U32_MAX, 0), xmm);
    #else
        _mm_maskmoveu_si128(_mm_castps_si128(xmm), _mm_setr_epi32(REV_U32_MAX, REV_U32_MAX, REV_U32_MAX, 0), cast(char *, e));
    #endif
    }

    REV_INLINE v3& REV_VECTORCALL operator+=(v3 r) { store(_mm_add_ps(load(), r.load())); return *this; }
    REV_INLINE v3& REV_VECTORCALL operator-=(v3 r) { store(_mm_sub_ps(load(), r.load())); return *this; }
    REV_INLINE v3& REV_VECTORCALL operator*=(v3 r) { store(_mm_mul_ps(load(), r.load())); return *this; }
    REV_INLINE v3& REV_VECTORCALL operator/=(v3 r) { store(_mm_div_ps(load(), r.load())); return *this; }

    REV_INLINE v3& REV_VECTORCALL operator+=(f32 r) { store(_mm_add_ps(load(), _mm_set_ps1(r))); return *this; }
    REV_INLINE v3& REV_VECTORCALL operator-=(f32 r) { store(_mm_sub_ps(load(), _mm_set_ps1(r))); return *this; }
    REV_INLINE v3& REV_VECTORCALL operator*=(f32 r) { store(_mm_mul_ps(load(), _mm_set_ps1(r))); return *this; }
    REV_INLINE v3& REV_VECTORCALL operator/=(f32 r) { store(_mm_div_ps(load(), _mm_set_ps1(r))); return *this; }

    REV_INLINE f32  REV_VECTORCALL operator[](u8 i) const { REV_CHECK(i < 3); return e[i]; }
    REV_INLINE f32& REV_VECTORCALL operator[](u8 i)       { REV_CHECK(i < 3); return e[i]; }
};

REV_INLINE v3 REV_VECTORCALL operator+(v3 l, v3 r) { return v3(_mm_add_ps(l.load(), r.load())); }
REV_INLINE v3 REV_VECTORCALL operator-(v3 l, v3 r) { return v3(_mm_sub_ps(l.load(), r.load())); }
REV_INLINE v3 REV_VECTORCALL operator*(v3 l, v3 r) { return v3(_mm_mul_ps(l.load(), r.load())); }
REV_INLINE v3 REV_VECTORCALL operator/(v3 l, v3 r) { return v3(_mm_div_ps(l.load(), r.load())); }

REV_INLINE v3 REV_VECTORCALL operator+(v3 l, f32 r) { return v3(_mm_add_ps(l.load(), _mm_set_ps1(r))); }
REV_INLINE v3 REV_VECTORCALL operator-(v3 l, f32 r) { return v3(_mm_sub_ps(l.load(), _mm_set_ps1(r))); }
REV_INLINE v3 REV_VECTORCALL operator*(v3 l, f32 r) { return v3(_mm_mul_ps(l.load(), _mm_set_ps1(r))); }
REV_INLINE v3 REV_VECTORCALL operator/(v3 l, f32 r) { return v3(_mm_div_ps(l.load(), _mm_set_ps1(r))); }

REV_INLINE v3 REV_VECTORCALL operator+(f32 l, v3 r) { return v3(_mm_add_ps(_mm_set_ps1(l), r.load())); }
REV_INLINE v3 REV_VECTORCALL operator-(f32 l, v3 r) { return v3(_mm_sub_ps(_mm_set_ps1(l), r.load())); }
REV_INLINE v3 REV_VECTORCALL operator*(f32 l, v3 r) { return v3(_mm_mul_ps(_mm_set_ps1(l), r.load())); }
REV_INLINE v3 REV_VECTORCALL operator/(f32 l, v3 r) { return v3(_mm_div_ps(_mm_set_ps1(l), r.load())); }

REV_INLINE bool REV_VECTORCALL operator==(v3 l, v3 r)
{
#if REV_ISA >= REV_ISA_AVX512
    return 0b1111 == _mm_cmpeq_epi32_mask(_mm_castps_si128(l.load()),
                                          _mm_castps_si128(r.load()));
#else
    return _mm_testc_si128(_mm_castps_si128(_mm_cmpeq_ps(l.load(), r.load())),
                           _mm_set1_epi64x(REV_U64_MAX));
#endif
}

REV_INLINE bool REV_VECTORCALL operator!=(v3 l, v3 r)
{
#if REV_ISA >= REV_ISA_AVX512
    return _mm_cmpneq_epi32_mask(_mm_castps_si128(l.load()),
                                 _mm_castps_si128(r.load()));
#else
    return _mm_testc_si128(_mm_castps_si128(_mm_cmpneq_ps(l.load(),
                                                          r.load())),
                           _mm_set1_epi64x(REV_U64_MAX));
#endif
}

// @NOTE(Roman):
// sqrt(x1^2 + y1^2 + z1^2)   <=> sqrt(x2^2 + y2^2 + z2^2)
// sqrt(x1^2 + y1^2 + z1^2)^2 <=> sqrt(x2^2 + y2^2 + z2^2)^2
//      x1^2 + y1^2 + z1^2    <=>      x2^2 + y2^2 + z2^2
REV_INLINE bool REV_VECTORCALL operator<=(v3 l, v3 r) { return l.length_sq() <= r.length_sq(); }
REV_INLINE bool REV_VECTORCALL operator>=(v3 l, v3 r) { return l.length_sq() >= r.length_sq(); }
REV_INLINE bool REV_VECTORCALL operator< (v3 l, v3 r) { return l.length_sq() <  r.length_sq(); }
REV_INLINE bool REV_VECTORCALL operator> (v3 l, v3 r) { return l.length_sq() >  r.length_sq(); }

union REV_INTRIN_TYPE v3s final
{
    struct { s32 x, y, z; };
    struct { s32 r, g, b; };
    v2s xy;
    v2s rg;
    s32 e[3];
    
    REV_INLINE v3s(                   ) : x(0),      y(0),      z(0)      {}
    REV_INLINE v3s(s32 val            ) : x(val),    y(val),    z(val)    {}
    REV_INLINE v3s(s32 x, s32 y, s32 z) : x(x),      y(y),      z(z)      {}
    REV_INLINE v3s(s32 arr[3]         ) : x(arr[0]), y(arr[1]), z(arr[2]) {}
    REV_INLINE v3s(v2s xy, s32 z      ) : x(xy.x),   y(xy.y),   z(z)      {}
    REV_INLINE v3s(const v3s& v       ) : x(v.x),    y(v.y),    z(v.z)    {}
    REV_INLINE v3s(v3s&& v            ) : x(v.x),    y(v.y),    z(v.z)    {}
    REV_INLINE v3s(__m128i xmm        ) { store(xmm); }

    REV_INLINE v3s& REV_VECTORCALL operator=(s32 val     ) { x = val;    y = val;    z = val;    return *this; }
    REV_INLINE v3s& REV_VECTORCALL operator=(s32 arr[3]  ) { x = arr[0]; y = arr[1]; z = arr[2]; return *this; }
    REV_INLINE v3s& REV_VECTORCALL operator=(const v3s& v) { x = v.x;    y = v.y;    z = v.z;    return *this; }
    REV_INLINE v3s& REV_VECTORCALL operator=(v3s&& v     ) { x = v.x;    y = v.y;    z = v.z;    return *this; }
    REV_INLINE v3s& REV_VECTORCALL operator=(__m128i xmm ) { store(xmm);                         return *this; }

    REV_INLINE s32 REV_VECTORCALL dot(v3s r) const
    {
        __m128i m = _mm_mullo_epi32(load(), r.load());
        __m128i h = _mm_hadd_epi32(m, m);
        return _mm_cvtsi128_si32(_mm_hadd_epi32(h, h));
    }

    REV_INLINE v3s REV_VECTORCALL cross(v3s r) const
    {
        //  [x]   [r.x]   [ (y * r.z - z * r.y)]   [y * r.z - z * r.y]
        //  [y] x [r.y] = [-(x * r.z - z * r.x)] = [z * r.x - x * r.z];
        //  [z]   [r.z]   [ (x * r.y - y * r.x)]   [x * r.y - y * r.x]
        
        __m128i l_mm = load();
        __m128i r_mm = r.load();

        return v3s(_mm_sub_epi32(_mm_mullo_epi32(_mm_shuffle_epi32(l_mm, MM_SHUFFLE_YZXW),
                                                 _mm_shuffle_epi32(r_mm, MM_SHUFFLE_ZXYW)),
                                 _mm_mullo_epi32(_mm_shuffle_epi32(l_mm, MM_SHUFFLE_ZXYW),
                                                 _mm_shuffle_epi32(r_mm, MM_SHUFFLE_YZXW))));
    }

    REV_INLINE f32 REV_VECTORCALL length() const
    {
        __m128 imm = _mm_cvtepi32_ps(load());
        return _mm_cvtss_f32(_mm_sqrt_ss(_mm_dp_ps(imm, imm, 0x71)));
    }

    REV_INLINE s32 REV_VECTORCALL length_sq() const
    {
        __m128i mm = load();
        __m128i m  = _mm_mullo_epi32(mm, mm);
        __m128i h  = _mm_hadd_epi32(m, m);
        return _mm_cvtsi128_si32(_mm_hadd_epi32(h, h));
    }

    static REV_INLINE v3s REV_VECTORCALL clamp(v3s val, v3s min, v3s max)
    {
        return v3s(_mm_max_epi32(min.load(), _mm_min_epi32(max.load(), val.load())));
    }

    REV_INLINE __m128i REV_VECTORCALL load() const
    {
    #if REV_ISA >= REV_ISA_AVX512
        return _mm_mask_load_epi32(e, 0b0111);
    #elif REV_ISA >= REV_ISA_AVX2
        return _mm_maskload_epi32(cast(const int *, e), _mm_setr_epi32(REV_U32_MAX, REV_U32_MAX, REV_U32_MAX, 0));
    #else
        return _mm_setr_epi32(x, y, z, 0.0f);
    #endif
    }

    REV_INLINE void REV_VECTORCALL store(__m128i xmm)
    {
    #if REV_ISA >= REV_ISA_AVX512
        _mm_mask_store_epi32(e, 0b0111, xmm);
    #elif REV_ISA >= REV_ISA_AVX2
        _mm_maskstore_epi32(cast(int *, e), _mm_setr_epi32(REV_U32_MAX, REV_U32_MAX, REV_U32_MAX, 0), xmm);
    #else
        _mm_maskmoveu_si128(xmm, _mm_setr_epi32(REV_U32_MAX, REV_U32_MAX, REV_U32_MAX, 0), cast(char *, e));
    #endif
    }

    REV_INLINE v3s& REV_VECTORCALL operator+=(v3s r) { store(_mm_add_epi32(load(),   r.load())); return *this; }
    REV_INLINE v3s& REV_VECTORCALL operator-=(v3s r) { store(_mm_sub_epi32(load(),   r.load())); return *this; }
    REV_INLINE v3s& REV_VECTORCALL operator*=(v3s r) { store(_mm_mullo_epi32(load(), r.load())); return *this; }
    REV_INLINE v3s& REV_VECTORCALL operator/=(v3s r) { store(_mm_div_epi32(load(),   r.load())); return *this; }

    REV_INLINE v3s& REV_VECTORCALL operator+=(s32 r) { store(_mm_add_epi32(load(),   _mm_set1_epi32(r))); return *this; }
    REV_INLINE v3s& REV_VECTORCALL operator-=(s32 r) { store(_mm_sub_epi32(load(),   _mm_set1_epi32(r))); return *this; }
    REV_INLINE v3s& REV_VECTORCALL operator*=(s32 r) { store(_mm_mullo_epi32(load(), _mm_set1_epi32(r))); return *this; }
    REV_INLINE v3s& REV_VECTORCALL operator/=(s32 r) { store(_mm_div_epi32(load(),   _mm_set1_epi32(r))); return *this; }

    REV_INLINE s32  REV_VECTORCALL operator[](u8 i) const { REV_CHECK(i < 3); return e[i]; }
    REV_INLINE s32& REV_VECTORCALL operator[](u8 i)       { REV_CHECK(i < 3); return e[i]; }
};

REV_INLINE v3s REV_VECTORCALL operator+(v3s l, v3s r) { return v3s(_mm_add_epi32(l.load(),   r.load())); }
REV_INLINE v3s REV_VECTORCALL operator-(v3s l, v3s r) { return v3s(_mm_sub_epi32(l.load(),   r.load())); }
REV_INLINE v3s REV_VECTORCALL operator*(v3s l, v3s r) { return v3s(_mm_mullo_epi32(l.load(), r.load())); }
REV_INLINE v3s REV_VECTORCALL operator/(v3s l, v3s r) { return v3s(_mm_div_epi32(l.load(),   r.load())); }

REV_INLINE v3s REV_VECTORCALL operator+(v3s l, s32 r) { return v3s(_mm_add_epi32(l.load(),   _mm_set1_epi32(r))); }
REV_INLINE v3s REV_VECTORCALL operator-(v3s l, s32 r) { return v3s(_mm_sub_epi32(l.load(),   _mm_set1_epi32(r))); }
REV_INLINE v3s REV_VECTORCALL operator*(v3s l, s32 r) { return v3s(_mm_mullo_epi32(l.load(), _mm_set1_epi32(r))); }
REV_INLINE v3s REV_VECTORCALL operator/(v3s l, s32 r) { return v3s(_mm_div_epi32(l.load(),   _mm_set1_epi32(r))); }

REV_INLINE v3s REV_VECTORCALL operator+(s32 l, v3s r) { return v3s(_mm_add_epi32(_mm_set1_epi32(l),   r.load())); }
REV_INLINE v3s REV_VECTORCALL operator-(s32 l, v3s r) { return v3s(_mm_sub_epi32(_mm_set1_epi32(l),   r.load())); }
REV_INLINE v3s REV_VECTORCALL operator*(s32 l, v3s r) { return v3s(_mm_mullo_epi32(_mm_set1_epi32(l), r.load())); }
REV_INLINE v3s REV_VECTORCALL operator/(s32 l, v3s r) { return v3s(_mm_div_epi32(_mm_set1_epi32(l),   r.load())); }

REV_INLINE bool REV_VECTORCALL operator==(v3s l, v3s r)
{
#if REV_ISA >= REV_ISA_AVX512
    return 0b1111 == _mm_cmpeq_epi32_mask(l.load(), r.load());
#else
    return _mm_testc_si128(_mm_cmpeq_epi32(l.load(), r.load()),
                           _mm_set1_epi64x(REV_U64_MAX));
#endif
}

REV_INLINE bool REV_VECTORCALL operator!=(v3s l, v3s r)
{
#if REV_ISA >= REV_ISA_AVX512
    return _mm_cmpneq_epi32_mask(l.load(), r.load());
#else
    __m128i cmp  = _mm_cmpeq_epi32(l.load(), r.load());
    __m128i mask = _mm_set1_epi64x(REV_U64_MAX);
    return _mm_testc_si128(_mm_andnot_si128(cmp, mask), mask);
#endif
}

// @NOTE(Roman):
// sqrt(x1^2 + y1^2 + z1^2)   <=> sqrt(x2^2 + y2^2 + z2^2)
// sqrt(x1^2 + y1^2 + z1^2)^2 <=> sqrt(x2^2 + y2^2 + z2^2)^2
//      x1^2 + y1^2 + z1^2    <=>      x2^2 + y2^2 + z2^2
REV_INLINE bool REV_VECTORCALL operator<=(v3s l, v3s r) { return l.length_sq() <= r.length_sq(); }
REV_INLINE bool REV_VECTORCALL operator>=(v3s l, v3s r) { return l.length_sq() >= r.length_sq(); }
REV_INLINE bool REV_VECTORCALL operator< (v3s l, v3s r) { return l.length_sq() <  r.length_sq(); }
REV_INLINE bool REV_VECTORCALL operator> (v3s l, v3s r) { return l.length_sq() >  r.length_sq(); }

union REV_INTRIN_TYPE v3u final
{
    struct { u32 x, y, z; };
    struct { s32 x, y, z; } s; // for not casting u32 to s32 for mm intrinsics
    struct { u32 r, g, b; };
    v2u xy;
    v2u rg;
    u32 e[3];

    REV_INLINE v3u(                   ) : x(0),      y(0),      z(0)      {}
    REV_INLINE v3u(u32 val            ) : x(val),    y(val),    z(val)    {}
    REV_INLINE v3u(u32 x, u32 y, u32 z) : x(x),      y(y),      z(z)      {}
    REV_INLINE v3u(u32 arr[3]         ) : x(arr[0]), y(arr[1]), z(arr[2]) {}
    REV_INLINE v3u(v2u xy, u32 z      ) : x(xy.x),   y(xy.y),   z(z)      {}
    REV_INLINE v3u(const v3u& v       ) : x(v.x),    y(v.y),    z(v.z)    {}
    REV_INLINE v3u(v3u&& v            ) : x(v.x),    y(v.y),    z(v.z)    {}
    REV_INLINE v3u(__m128i xmm        ) { store(xmm); }

    REV_INLINE v3u& REV_VECTORCALL operator=(u32 val     ) { x = val;    y = val;    z = val;    return *this; }
    REV_INLINE v3u& REV_VECTORCALL operator=(u32 arr[3]  ) { x = arr[0]; y = arr[1]; z = arr[2]; return *this; }
    REV_INLINE v3u& REV_VECTORCALL operator=(const v3u& v) { x = v.x;    y = v.y;    z = v.z;    return *this; }
    REV_INLINE v3u& REV_VECTORCALL operator=(v3u&& v     ) { x = v.x;    y = v.y;    z = v.z;    return *this; }
    REV_INLINE v3u& REV_VECTORCALL operator=(__m128i xmm ) { store(xmm);                         return *this; }

    REV_INLINE u32 REV_VECTORCALL dot(v3u r) const
    {
        __m128i m = _mm_mullo_epi32(load(), r.load());
        __m128i h = _mm_hadd_epi32(m, m);
        return cast(u32, _mm_cvtsi128_si32(_mm_hadd_epi32(h, h)));
    }

    REV_INLINE v3u REV_VECTORCALL cross(v3u r) const
    {
        //  [x]   [r.x]   [ (y * r.z - z * r.y)]   [y * r.z - z * r.y]
        //  [y] x [r.y] = [-(x * r.z - z * r.x)] = [z * r.x - x * r.z];
        //  [z]   [r.z]   [ (x * r.y - y * r.x)]   [x * r.y - y * r.x]
        
        __m128i l_mm = load();
        __m128i r_mm = r.load();

        return v3u(_mm_sub_epi32(_mm_mullo_epi32(_mm_shuffle_epi32(l_mm, MM_SHUFFLE_YZXW),
                                                 _mm_shuffle_epi32(r_mm, MM_SHUFFLE_ZXYW)),
                                 _mm_mullo_epi32(_mm_shuffle_epi32(l_mm, MM_SHUFFLE_ZXYW),
                                                 _mm_shuffle_epi32(r_mm, MM_SHUFFLE_YZXW))));
    }

    REV_INLINE f32 REV_VECTORCALL length() const
    {
        __m128 imm = mm_cvtepu32_ps(load());
        return _mm_cvtss_f32(_mm_sqrt_ss(_mm_dp_ps(imm, imm, 0x71)));
    }

    REV_INLINE u32 REV_VECTORCALL length_sq() const
    {
        __m128i mm = load();
        __m128i m  = _mm_mullo_epi32(mm, mm);
        __m128i h  = _mm_hadd_epi32(m, m);
        return _mm_cvtsi128_si32(_mm_hadd_epi32(h, h));
    }

    static REV_INLINE v3u REV_VECTORCALL clamp(v3u val, v3u min, v3u max)
    {
        return v3u(_mm_max_epu32(min.load(), _mm_min_epu32(max.load(), val.load())));
    }

    REV_INLINE __m128i REV_VECTORCALL load() const
    {
    #if REV_ISA >= REV_ISA_AVX512
        return _mm_mask_load_epi32(e, 0b0111);
    #elif REV_ISA >= REV_ISA_AVX2
        return _mm_maskload_epi32(cast(const int *, e), _mm_setr_epi32(REV_U32_MAX, REV_U32_MAX, REV_U32_MAX, 0));
    #else
        return _mm_setr_epi32(s.x, s.y, s.z, 0);
    #endif
    }

    REV_INLINE void REV_VECTORCALL store(__m128i xmm)
    {
    #if REV_ISA >= REV_ISA_AVX512
        _mm_mask_store_epi32(e, 0b0111, xmm);
    #elif REV_ISA >= REV_ISA_AVX2
        _mm_maskstore_epi32(cast(int *, e), _mm_setr_epi32(REV_U32_MAX, REV_U32_MAX, REV_U32_MAX, 0), xmm);
    #else
        _mm_maskmoveu_si128(xmm, _mm_setr_epi32(REV_U32_MAX, REV_U32_MAX, REV_U32_MAX, 0), cast(char *, e));
    #endif
    }

    REV_INLINE v3u& REV_VECTORCALL operator+=(v3u r) { store(_mm_add_epi32(load(),   r.load())); return *this; }
    REV_INLINE v3u& REV_VECTORCALL operator-=(v3u r) { store(_mm_sub_epi32(load(),   r.load())); return *this; }
    REV_INLINE v3u& REV_VECTORCALL operator*=(v3u r) { store(_mm_mullo_epi32(load(), r.load())); return *this; }
    REV_INLINE v3u& REV_VECTORCALL operator/=(v3u r) { store(_mm_div_epu32(load(),   r.load())); return *this; }

    REV_INLINE v3u& REV_VECTORCALL operator+=(u32 r) { store(_mm_add_epi32(load(),   _mm_set1_epi32(r))); return *this; }
    REV_INLINE v3u& REV_VECTORCALL operator-=(u32 r) { store(_mm_sub_epi32(load(),   _mm_set1_epi32(r))); return *this; }
    REV_INLINE v3u& REV_VECTORCALL operator*=(u32 r) { store(_mm_mullo_epi32(load(), _mm_set1_epi32(r))); return *this; }
    REV_INLINE v3u& REV_VECTORCALL operator/=(u32 r) { store(_mm_div_epu32(load(),   _mm_set1_epi32(r))); return *this; }

    REV_INLINE u32  REV_VECTORCALL operator[](u8 i) const { REV_CHECK(i < 3); return e[i]; }
    REV_INLINE u32& REV_VECTORCALL operator[](u8 i)       { REV_CHECK(i < 3); return e[i]; }
};

REV_INLINE v3u REV_VECTORCALL operator+(v3u l, v3u r) { return v3u(_mm_add_epi32(l.load(),   r.load())); }
REV_INLINE v3u REV_VECTORCALL operator-(v3u l, v3u r) { return v3u(_mm_sub_epi32(l.load(),   r.load())); }
REV_INLINE v3u REV_VECTORCALL operator*(v3u l, v3u r) { return v3u(_mm_mullo_epi32(l.load(), r.load())); }
REV_INLINE v3u REV_VECTORCALL operator/(v3u l, v3u r) { return v3u(_mm_div_epu32(l.load(),   r.load())); }

REV_INLINE v3u REV_VECTORCALL operator+(v3u l, u32 r) { return v3u(_mm_add_epi32(l.load(),   _mm_set1_epi32(r))); }
REV_INLINE v3u REV_VECTORCALL operator-(v3u l, u32 r) { return v3u(_mm_sub_epi32(l.load(),   _mm_set1_epi32(r))); }
REV_INLINE v3u REV_VECTORCALL operator*(v3u l, u32 r) { return v3u(_mm_mullo_epi32(l.load(), _mm_set1_epi32(r))); }
REV_INLINE v3u REV_VECTORCALL operator/(v3u l, u32 r) { return v3u(_mm_div_epu32(l.load(),   _mm_set1_epi32(r))); }

REV_INLINE v3u REV_VECTORCALL operator+(u32 l, v3u r) { return v3u(_mm_add_epi32(_mm_set1_epi32(l),   r.load())); }
REV_INLINE v3u REV_VECTORCALL operator-(u32 l, v3u r) { return v3u(_mm_sub_epi32(_mm_set1_epi32(l),   r.load())); }
REV_INLINE v3u REV_VECTORCALL operator*(u32 l, v3u r) { return v3u(_mm_mullo_epi32(_mm_set1_epi32(l), r.load())); }
REV_INLINE v3u REV_VECTORCALL operator/(u32 l, v3u r) { return v3u(_mm_div_epu32(_mm_set1_epi32(l),   r.load())); }

REV_INLINE bool REV_VECTORCALL operator==(v3u l, v3u r)
{
#if REV_ISA >= REV_ISA_AVX512
    return 0b1111 == _mm_cmpeq_epi32_mask(l.load(), r.load());
#else
    return _mm_testc_si128(_mm_cmpeq_epi32(l.load(), r.load()),
                           _mm_set1_epi64x(REV_U64_MAX));
#endif
}

REV_INLINE bool REV_VECTORCALL operator!=(v3u l, v3u r)
{
#if REV_ISA >= REV_ISA_AVX512
    return _mm_cmpneq_epi32_mask(l.load(), r.load());
#else
    __m128i cmp  = _mm_cmpeq_epi32(l.load(), r.load());
    __m128i mask = _mm_set1_epi64x(REV_U64_MAX);
    return _mm_testc_si128(_mm_andnot_si128(cmp, mask), mask);
#endif
}

// @NOTE(Roman):
// sqrt(x1^2 + y1^2 + z1^2)   <=> sqrt(x2^2 + y2^2 + z2^2)
// sqrt(x1^2 + y1^2 + z1^2)^2 <=> sqrt(x2^2 + y2^2 + z2^2)^2
//      x1^2 + y1^2 + z1^2    <=>      x2^2 + y2^2 + z2^2
REV_INLINE bool REV_VECTORCALL operator<=(v3u l, v3u r) { return l.length_sq() <= r.length_sq(); }
REV_INLINE bool REV_VECTORCALL operator>=(v3u l, v3u r) { return l.length_sq() >= r.length_sq(); }
REV_INLINE bool REV_VECTORCALL operator< (v3u l, v3u r) { return l.length_sq() <  r.length_sq(); }
REV_INLINE bool REV_VECTORCALL operator> (v3u l, v3u r) { return l.length_sq() >  r.length_sq(); }


REV_INLINE v3s REV_VECTORCALL v3_to_v3s (v3  v) { return v3s(_mm_cvtps_epi32(v.load())); }
REV_INLINE v3u REV_VECTORCALL v3_to_v3u (v3  v) { return v3u(_mm_cvtps_epi32(v.load())); }
REV_INLINE v3  REV_VECTORCALL v3s_to_v3 (v3s v) { return v3 (_mm_cvtepi32_ps(v.load())); }
REV_INLINE v3u REV_VECTORCALL v3s_to_v3u(v3s v) { return v3u(                v.load() ); }
REV_INLINE v3  REV_VECTORCALL v3u_to_v3 (v3u v) { return v3 (mm_cvtepu32_ps( v.load())); }
REV_INLINE v3s REV_VECTORCALL v3u_to_v3s(v3u v) { return v3s(                v.load() ); }

//
// v4
//

union REV_INTRIN_TYPE v4 final
{
    struct { f32 x,  y,  z, w; };
    struct { f32 r,  g,  b, a; };
    struct { v2  xy, wh; };
    v3    xyz;
    v3    rgb;
    f32   e[4]; 
    xmm_u mm;

    REV_INLINE v4(                          ) : mm(_mm_setzero_ps())                    {}
    REV_INLINE v4(f32 val                   ) : mm(_mm_set_ps1(val))                    {}
    REV_INLINE v4(f32 x, f32 y, f32 z, f32 w) : mm(_mm_setr_ps(x, y, z, w))             {}
    REV_INLINE v4(f32 arr[4]                ) : mm(_mm_load_ps(arr))                    {}
    REV_INLINE v4(v2 xy, f32 z, f32 w       ) : mm(_mm_setr_ps(xy.x, xy.y, z, w))       {}
    REV_INLINE v4(v2 xy, v2 wh              ) : mm(_mm_setr_ps(xy.x, xy.y, wh.w, wh.h)) {}
    REV_INLINE v4(v3 xyz, f32 w             ) : mm(_mm_setr_ps(xyz.x, xyz.y, xyz.z, w)) {}
    REV_INLINE v4(__m128 xmm                ) : mm(xmm)                                 {}
    REV_INLINE v4(const v4& v               ) : mm(v.mm)                                {}
    REV_INLINE v4(v4&& v                    ) : mm(v.mm)                                {}

    REV_INLINE v4& REV_VECTORCALL operator=(f32 val    ) { mm = _mm_set_ps1(val); return *this; }
    REV_INLINE v4& REV_VECTORCALL operator=(f32 arr[4] ) { mm = _mm_load_ps(arr); return *this; }
    REV_INLINE v4& REV_VECTORCALL operator=(__m128 _mm ) { mm = _mm;              return *this; }
    REV_INLINE v4& REV_VECTORCALL operator=(const v4& v) { mm = v.mm;             return *this; }
    REV_INLINE v4& REV_VECTORCALL operator=(v4&& v     ) { mm = v.mm;             return *this; }

    REV_INLINE f32 REV_VECTORCALL dot(v4 r) const
    {
        return _mm_cvtss_f32(_mm_dp_ps(mm, r.mm, 0x71));
    }

    REV_INLINE v4 REV_VECTORCALL cross(v4 r) const
    {
        //  [x]   [r.x]   [ (y * r.z - z * r.y)]   [y * r.z - z * r.y]
        //  [y] x [r.y] = [-(x * r.z - z * r.x)] = [z * r.x - x * r.z];
        //  [z]   [r.z]   [ (x * r.y - y * r.x)]   [x * r.y - y * r.x]
        //  [w]   [r.w]   [ (w * r.w - w * r.w)]   [        0        ]

        return v4(_mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(  mm,   mm, MM_SHUFFLE_YZXW),
                                        _mm_shuffle_ps(r.mm, r.mm, MM_SHUFFLE_ZXYW)),
                             _mm_mul_ps(_mm_shuffle_ps(  mm,   mm, MM_SHUFFLE_ZXYW),
                                        _mm_shuffle_ps(r.mm, r.mm, MM_SHUFFLE_YZXW))));
    }

    REV_INLINE f32 REV_VECTORCALL length() const
    {
        __m128 _mm = mm;
        return _mm_cvtss_f32(_mm_sqrt_ss(_mm_dp_ps(_mm, _mm, 0x71)));
    }

    REV_INLINE f32 REV_VECTORCALL length_sq() const
    {
        __m128 _mm = mm;
        return _mm_cvtss_f32(_mm_dp_ps(_mm, _mm, 0x71));
    }

    REV_INLINE v4 REV_VECTORCALL normalize() const
    {
        __m128 _mm = mm;
        return v4(mm_insert_f32<3>(_mm_div_ps(_mm, _mm_sqrt_ps(_mm_dp_ps(_mm, _mm, 0x7F))), w)); // save original w value
    }

    REV_INLINE v4 REV_VECTORCALL normalize_w() const
    {
        if (w && w != 1.0f)
        {
            return v4(_mm_div_ps(mm, _mm_set_ps1(w)));
        }
        return *this;
    }

    REV_INLINE v4 REV_VECTORCALL project(v4 on_normal) const
    {
        return v4(_mm_mul_ps(_mm_dp_ps(mm, on_normal.mm, 0x7F), on_normal.mm));
    }

    REV_INLINE v4 REV_VECTORCALL reflect(v4 normal) const
    {
        __m128 dp  = _mm_dp_ps(mm, normal.mm, 0x7F);
        __m128 _2p = _mm_mul_ps(_mm_set_ps1(2.0f), _mm_mul_ps(dp, normal.mm));
        return v4(_mm_sub_ps(mm, _2p));
    }

    static REV_INLINE v4 REV_VECTORCALL lerp(v4 start, v4 end, v4 percent)
    {
        return v4(_mm_add_ps(_mm_mul_ps(_mm_sub_ps(end.mm, start.mm), percent.mm), start.mm));
    }

    static REV_INLINE v4 REV_VECTORCALL invlerp(v4 start, v4 end, v4 value) // ret = [0, 1]
    {
        __m128 mm_start = start.mm;
        return v4(_mm_div_ps(_mm_sub_ps(value.mm, mm_start), _mm_sub_ps(end.mm, mm_start)));
    }

    static REV_INLINE v4 REV_VECTORCALL invlerp_n(v4 start, v4 end, v4 value) // ret = [-1, 1]
    {
        __m128 mm_start = start.mm;
        return v4(_mm_mul_ps(_mm_sub_ps(_mm_div_ps(_mm_sub_ps(value.mm, mm_start), _mm_sub_ps(end.mm, mm_start)), _mm_set_ps1(0.5f)), _mm_set_ps1(2.0f)));
    }

    static REV_INLINE v4 REV_VECTORCALL clamp(v4 val, v4 min, v4 max)
    {
        return v4(_mm_max_ps(min.mm, _mm_min_ps(max.mm, val.mm)));
    }

    REV_INLINE v4& REV_VECTORCALL operator+=(v4 r) { mm = _mm_add_ps(mm, r.mm); return *this; }
    REV_INLINE v4& REV_VECTORCALL operator-=(v4 r) { mm = _mm_sub_ps(mm, r.mm); return *this; }
    REV_INLINE v4& REV_VECTORCALL operator*=(v4 r) { mm = _mm_mul_ps(mm, r.mm); return *this; }
    REV_INLINE v4& REV_VECTORCALL operator/=(v4 r) { mm = _mm_div_ps(mm, r.mm); return *this; }

    REV_INLINE v4& REV_VECTORCALL operator+=(f32 r) { mm = _mm_add_ps(mm, _mm_set_ps1(r)); return *this; }
    REV_INLINE v4& REV_VECTORCALL operator-=(f32 r) { mm = _mm_sub_ps(mm, _mm_set_ps1(r)); return *this; }
    REV_INLINE v4& REV_VECTORCALL operator*=(f32 r) { mm = _mm_mul_ps(mm, _mm_set_ps1(r)); return *this; }
    REV_INLINE v4& REV_VECTORCALL operator/=(f32 r) { mm = _mm_div_ps(mm, _mm_set_ps1(r)); return *this; }

    REV_INLINE f32  REV_VECTORCALL operator[](u8 i) const { REV_CHECK(i < 4); return e[i]; }
    REV_INLINE f32& REV_VECTORCALL operator[](u8 i)       { REV_CHECK(i < 4); return e[i]; }
};

REV_INLINE v4 REV_VECTORCALL operator+(v4 l, v4 r) { return v4(_mm_add_ps(l.mm, r.mm)); }
REV_INLINE v4 REV_VECTORCALL operator-(v4 l, v4 r) { return v4(_mm_sub_ps(l.mm, r.mm)); }
REV_INLINE v4 REV_VECTORCALL operator*(v4 l, v4 r) { return v4(_mm_mul_ps(l.mm, r.mm)); }
REV_INLINE v4 REV_VECTORCALL operator/(v4 l, v4 r) { return v4(_mm_div_ps(l.mm, r.mm)); }

REV_INLINE v4 REV_VECTORCALL operator+(v4 l, f32 r) { return v4(_mm_add_ps(l.mm, _mm_set_ps1(r))); }
REV_INLINE v4 REV_VECTORCALL operator-(v4 l, f32 r) { return v4(_mm_sub_ps(l.mm, _mm_set_ps1(r))); }
REV_INLINE v4 REV_VECTORCALL operator*(v4 l, f32 r) { return v4(_mm_mul_ps(l.mm, _mm_set_ps1(r))); }
REV_INLINE v4 REV_VECTORCALL operator/(v4 l, f32 r) { return v4(_mm_div_ps(l.mm, _mm_set_ps1(r))); }

REV_INLINE v4 REV_VECTORCALL operator+(f32 l, v4 r) { return v4(_mm_add_ps(_mm_set_ps1(l), r.mm)); }
REV_INLINE v4 REV_VECTORCALL operator-(f32 l, v4 r) { return v4(_mm_sub_ps(_mm_set_ps1(l), r.mm)); }
REV_INLINE v4 REV_VECTORCALL operator*(f32 l, v4 r) { return v4(_mm_mul_ps(_mm_set_ps1(l), r.mm)); }
REV_INLINE v4 REV_VECTORCALL operator/(f32 l, v4 r) { return v4(_mm_div_ps(_mm_set_ps1(l), r.mm)); }

REV_INLINE bool REV_VECTORCALL operator==(v4 l, v4 r)
{
#if REV_ISA >= REV_ISA_AVX512
    return 0b1111 == _mm_cmpeq_epi32_mask(_mm_castps_si128(l.mm),
                                          _mm_castps_si128(r.mm));
#else
    return _mm_testc_si128(_mm_castps_si128(_mm_cmpeq_ps(l.mm, r.mm)),
                           _mm_set1_epi64x(REV_U64_MAX));
#endif
}

REV_INLINE bool REV_VECTORCALL operator!=(v4 l, v4 r)
{
#if REV_ISA >= REV_ISA_AVX512
    return _mm_cmpneq_epi32_mask(_mm_castps_si128(l.mm),
                                 _mm_castps_si128(r.mm));
#else
    return _mm_testc_si128(_mm_castps_si128(_mm_cmpneq_ps(l.mm, r.mm)),
                           _mm_set1_epi64x(REV_U64_MAX));
#endif
}

// @NOTE(Roman):
// sqrt(x1^2 + y1^2 + z1^2)   <=> sqrt(x2^2 + y2^2 + z2^2)
// sqrt(x1^2 + y1^2 + z1^2)^2 <=> sqrt(x2^2 + y2^2 + z2^2)^2
//      x1^2 + y1^2 + z1^2    <=>      x2^2 + y2^2 + z2^2
REV_INLINE bool REV_VECTORCALL operator<=(v4 l, v4 r) { return l.length_sq() <= r.length_sq(); }
REV_INLINE bool REV_VECTORCALL operator>=(v4 l, v4 r) { return l.length_sq() >= r.length_sq(); }
REV_INLINE bool REV_VECTORCALL operator< (v4 l, v4 r) { return l.length_sq() <  r.length_sq(); }
REV_INLINE bool REV_VECTORCALL operator> (v4 l, v4 r) { return l.length_sq() >  r.length_sq(); }

union REV_INTRIN_TYPE v4s final
{
    struct { s32 x,  y,  z, w; };
    struct { s32 r,  g,  b, a; };
    struct { v2s xy, wh; };
    v3s   xyz;
    v3s   rgb;
    s32   e[4];
    xmm_u mm;

    REV_INLINE v4s(                          ) : mm(_mm_setzero_si128())                    {}
    REV_INLINE v4s(s32 val                   ) : mm(_mm_set1_epi32(val))                    {}
    REV_INLINE v4s(s32 x, s32 y, s32 z, s32 w) : mm(_mm_setr_epi32(x, y, z, w))             {}
    REV_INLINE v4s(s32 arr[4]                ) : mm(_mm_lddqu_si128(cast(__m128i *, arr)))  {}
    REV_INLINE v4s(v2s xy, s32 z, s32 w      ) : mm(_mm_setr_epi32(xy.x, xy.y, z, w))       {}
    REV_INLINE v4s(v2s xy, v2s wh            ) : mm(_mm_setr_epi32(xy.x, xy.y, wh.w, wh.h)) {}
    REV_INLINE v4s(v3s xyz, s32 w            ) : mm(_mm_setr_epi32(xyz.x, xyz.y, xyz.z, w)) {}
    REV_INLINE v4s(__m128i _mm               ) : mm(_mm)                                    {}
    REV_INLINE v4s(const v4s& v              ) : mm(v.mm)                                   {}
    REV_INLINE v4s(v4s&& v                   ) : mm(v.mm)                                   {}

    REV_INLINE v4s& REV_VECTORCALL operator=(s32 val     ) { mm = _mm_set1_epi32(val);                   return *this; }
    REV_INLINE v4s& REV_VECTORCALL operator=(s32 arr[4]  ) { mm = _mm_lddqu_si128(cast(__m128i *, arr)); return *this; }
    REV_INLINE v4s& REV_VECTORCALL operator=(__m128i _mm ) { mm = _mm;                                   return *this; }
    REV_INLINE v4s& REV_VECTORCALL operator=(const v4s& v) { mm = v.mm;                                  return *this; }
    REV_INLINE v4s& REV_VECTORCALL operator=(v4s&& v     ) { mm = v.mm;                                  return *this; }

    REV_INLINE s32 REV_VECTORCALL dot(v4s r) const
    {
        __m128i m = _mm_mullo_epi32(mm, r.mm);
        __m128i h = _mm_hadd_epi32(m, m);
        return _mm_cvtsi128_si32(_mm_hadd_epi32(h, h));
    }

    REV_INLINE v4s REV_VECTORCALL cross(v4s r) const
    {
        //  [x]   [r.x]   [ (y * r.z - z * r.y)]   [y * r.z - z * r.y]
        //  [y] x [r.y] = [-(x * r.z - z * r.x)] = [z * r.x - x * r.z];
        //  [z]   [r.z]   [ (x * r.y - y * r.x)]   [x * r.y - y * r.x]
        //  [w]   [r.w]   [ (w * r.w - w * r.w)]   [        0        ]

        return v4s(_mm_sub_epi32(_mm_mullo_epi32(_mm_shuffle_epi32(  mm, MM_SHUFFLE_YZXW),
                                                 _mm_shuffle_epi32(r.mm, MM_SHUFFLE_ZXYW)),
                                 _mm_mullo_epi32(_mm_shuffle_epi32(  mm, MM_SHUFFLE_ZXYW),
                                                 _mm_shuffle_epi32(r.mm, MM_SHUFFLE_YZXW))));
    }

    REV_INLINE f32 REV_VECTORCALL length() const
    {
        __m128 imm = _mm_cvtepi32_ps(mm);
        return _mm_cvtss_f32(_mm_sqrt_ss(_mm_dp_ps(imm, imm, 0x71)));
    }

    REV_INLINE s32 REV_VECTORCALL length_sq() const
    {
        __m128i m = _mm_mullo_epi32(mm, mm);
        __m128i h = _mm_hadd_epi32(m, m);
        return _mm_cvtsi128_si32(_mm_hadd_epi32(h, h));
    }

    static REV_INLINE v4s REV_VECTORCALL clamp(v4s val, v4s min, v4s max)
    {
        return v4s(_mm_max_epi32(min.mm, _mm_min_epi32(max.mm, val.mm)));
    }

    REV_INLINE v4s& REV_VECTORCALL operator+=(v4s r) { mm = _mm_add_epi32(mm,   r.mm); return *this; }
    REV_INLINE v4s& REV_VECTORCALL operator-=(v4s r) { mm = _mm_sub_epi32(mm,   r.mm); return *this; }
    REV_INLINE v4s& REV_VECTORCALL operator*=(v4s r) { mm = _mm_mullo_epi32(mm, r.mm); return *this; }
    REV_INLINE v4s& REV_VECTORCALL operator/=(v4s r) { mm = _mm_div_epi32(mm,   r.mm); return *this; }

    REV_INLINE v4s& REV_VECTORCALL operator+=(s32 r) { mm = _mm_add_epi32(mm,   _mm_set1_epi32(r)); return *this; }
    REV_INLINE v4s& REV_VECTORCALL operator-=(s32 r) { mm = _mm_sub_epi32(mm,   _mm_set1_epi32(r)); return *this; }
    REV_INLINE v4s& REV_VECTORCALL operator*=(s32 r) { mm = _mm_mullo_epi32(mm, _mm_set1_epi32(r)); return *this; }
    REV_INLINE v4s& REV_VECTORCALL operator/=(s32 r) { mm = _mm_div_epi32(mm,   _mm_set1_epi32(r)); return *this; }

    REV_INLINE s32  REV_VECTORCALL operator[](u8 i) const { REV_CHECK(i < 4); return e[i]; }
    REV_INLINE s32& REV_VECTORCALL operator[](u8 i)       { REV_CHECK(i < 4); return e[i]; }
};

REV_INLINE v4s REV_VECTORCALL operator+(v4s l, v4s r) { return v4s(_mm_add_epi32(l.mm,   r.mm)); }
REV_INLINE v4s REV_VECTORCALL operator-(v4s l, v4s r) { return v4s(_mm_sub_epi32(l.mm,   r.mm)); }
REV_INLINE v4s REV_VECTORCALL operator*(v4s l, v4s r) { return v4s(_mm_mullo_epi32(l.mm, r.mm)); }
REV_INLINE v4s REV_VECTORCALL operator/(v4s l, v4s r) { return v4s(_mm_div_epi32(l.mm,   r.mm)); }

REV_INLINE v4s REV_VECTORCALL operator+(v4s l, s32 r) { return v4s(_mm_add_epi32(l.mm,   _mm_set1_epi32(r))); }
REV_INLINE v4s REV_VECTORCALL operator-(v4s l, s32 r) { return v4s(_mm_sub_epi32(l.mm,   _mm_set1_epi32(r))); }
REV_INLINE v4s REV_VECTORCALL operator*(v4s l, s32 r) { return v4s(_mm_mullo_epi32(l.mm, _mm_set1_epi32(r))); }
REV_INLINE v4s REV_VECTORCALL operator/(v4s l, s32 r) { return v4s(_mm_div_epi32(l.mm,   _mm_set1_epi32(r))); }

REV_INLINE v4s REV_VECTORCALL operator+(s32 l, v4s r) { return v4s(_mm_add_epi32(_mm_set1_epi32(l),   r.mm)); }
REV_INLINE v4s REV_VECTORCALL operator-(s32 l, v4s r) { return v4s(_mm_sub_epi32(_mm_set1_epi32(l),   r.mm)); }
REV_INLINE v4s REV_VECTORCALL operator*(s32 l, v4s r) { return v4s(_mm_mullo_epi32(_mm_set1_epi32(l), r.mm)); }
REV_INLINE v4s REV_VECTORCALL operator/(s32 l, v4s r) { return v4s(_mm_div_epi32(_mm_set1_epi32(l),   r.mm)); }

REV_INLINE bool REV_VECTORCALL operator==(v4s l, v4s r)
{
#if REV_ISA >= REV_ISA_AVX512
    return 0b1111 == _mm_cmpeq_epi32_mask(l.mm, r.mm);
#else
    return _mm_testc_si128(_mm_cmpeq_epi32(l.mm, r.mm),
                           _mm_set1_epi64x(REV_U64_MAX));
#endif
}

REV_INLINE bool REV_VECTORCALL operator!=(v4s l, v4s r)
{
#if REV_ISA >= REV_ISA_AVX512
    return _mm_cmpneq_epi32_mask(l.mm, r.mm);
#else
    __m128i mask = _mm_set1_epi64x(REV_U64_MAX);
    return _mm_testc_si128(_mm_andnot_si128(_mm_cmpeq_epi32(l.mm, r.mm), mask), mask);
#endif
}

// @NOTE(Roman):
// sqrt(x1^2 + y1^2 + z1^2)   <=> sqrt(x2^2 + y2^2 + z2^2)
// sqrt(x1^2 + y1^2 + z1^2)^2 <=> sqrt(x2^2 + y2^2 + z2^2)^2
//      x1^2 + y1^2 + z1^2    <=>      x2^2 + y2^2 + z2^2
REV_INLINE bool REV_VECTORCALL operator<=(v4s l, v4s r) { return l.length_sq() <= r.length_sq(); }
REV_INLINE bool REV_VECTORCALL operator>=(v4s l, v4s r) { return l.length_sq() >= r.length_sq(); }
REV_INLINE bool REV_VECTORCALL operator< (v4s l, v4s r) { return l.length_sq() <  r.length_sq(); }
REV_INLINE bool REV_VECTORCALL operator> (v4s l, v4s r) { return l.length_sq() >  r.length_sq(); }

union REV_INTRIN_TYPE v4u final
{
    struct { u32 x, y, z, w; };
    struct { u32 r, g, b, a; };
    struct { v2u xy, wh; };
    v3u   xyz;
    v3u   rgb;
    u32   e[4];
    xmm_u mm;

    REV_INLINE v4u(                          ) : mm(_mm_setzero_si128())                    {}
    REV_INLINE v4u(u32 val                   ) : mm(_mm_set1_epi32(val))                    {}
    REV_INLINE v4u(u32 x, u32 y, u32 z, u32 w) : mm(_mm_setr_epi32(x, y, z, w))             {}
    REV_INLINE v4u(u32 arr[4]                ) : mm(_mm_load_si128(cast(__m128i *, arr)))   {}
    REV_INLINE v4u(v2u xy, u32 z, u32 w      ) : mm(_mm_setr_epi32(xy.x, xy.y, z, w))       {}
    REV_INLINE v4u(v2u xy, v2u wh            ) : mm(_mm_setr_epi32(xy.x, xy.y, wh.w, wh.h)) {}
    REV_INLINE v4u(v3u xyz, u32 w            ) : mm(_mm_setr_epi32(xyz.x, xyz.y, xyz.z, w)) {}
    REV_INLINE v4u(__m128i _mm               ) : mm(_mm)                                    {}
    REV_INLINE v4u(const v4u& v              ) : mm(v.mm)                                   {}
    REV_INLINE v4u(v4u&& v                   ) : mm(v.mm)                                   {}

    REV_INLINE v4u& REV_VECTORCALL operator=(u32 val     ) { mm = _mm_set1_epi32(val);                  return *this; }
    REV_INLINE v4u& REV_VECTORCALL operator=(u32 arr[4]  ) { mm = _mm_load_si128(cast(__m128i *, arr)); return *this; }
    REV_INLINE v4u& REV_VECTORCALL operator=(__m128i _mm ) { mm = _mm;                                  return *this; }
    REV_INLINE v4u& REV_VECTORCALL operator=(const v4u& v) { mm = v.mm;                                 return *this; }
    REV_INLINE v4u& REV_VECTORCALL operator=(v4u&& v     ) { mm = v.mm;                                 return *this; }

    REV_INLINE u32 REV_VECTORCALL dot(v4u r) const
    {
        __m128i m = _mm_mullo_epi32(mm, r.mm);
        __m128i h = _mm_hadd_epi32(m, m);
        return _mm_cvtsi128_si32(_mm_hadd_epi32(h, h));
    }

    REV_INLINE v4u REV_VECTORCALL cross(v4u r) const
    {
        //  [x]   [r.x]   [ (y * r.z - z * r.y)]   [y * r.z - z * r.y]
        //  [y] x [r.y] = [-(x * r.z - z * r.x)] = [z * r.x - x * r.z];
        //  [z]   [r.z]   [ (x * r.y - y * r.x)]   [x * r.y - y * r.x]
        //  [w]   [r.w]   [ (w * r.w - w * r.w)]   [        0        ]

        return v4u(_mm_sub_epi32(_mm_mullo_epi32(_mm_shuffle_epi32(  mm, MM_SHUFFLE_YZXW),
                                                 _mm_shuffle_epi32(r.mm, MM_SHUFFLE_ZXYW)),
                                 _mm_mullo_epi32(_mm_shuffle_epi32(  mm, MM_SHUFFLE_ZXYW),
                                                 _mm_shuffle_epi32(r.mm, MM_SHUFFLE_YZXW))));
    }

    REV_INLINE f32 REV_VECTORCALL length() const
    {
        __m128 m = mm_cvtepu32_ps(mm);
        return _mm_cvtss_f32(_mm_sqrt_ss(_mm_dp_ps(m, m, 0x71)));
    }

    REV_INLINE u32 REV_VECTORCALL length_sq() const
    {
        __m128i m = _mm_mullo_epi32(mm, mm);
        __m128i h = _mm_hadd_epi32(m, m);
        return _mm_cvtsi128_si32(_mm_hadd_epi32(h, h));
    }

    static REV_INLINE v4u REV_VECTORCALL clamp(v4u val, v4u min, v4u max)
    {
        return v4u(_mm_max_epu32(min.mm, _mm_min_epu32(max.mm, val.mm)));
    }

    REV_INLINE v4u& REV_VECTORCALL operator+=(v4u r) { mm = _mm_add_epi32(mm,   r.mm); return *this; }
    REV_INLINE v4u& REV_VECTORCALL operator-=(v4u r) { mm = _mm_sub_epi32(mm,   r.mm); return *this; }
    REV_INLINE v4u& REV_VECTORCALL operator*=(v4u r) { mm = _mm_mullo_epi32(mm, r.mm); return *this; }
    REV_INLINE v4u& REV_VECTORCALL operator/=(v4u r) { mm = _mm_div_epu32(mm,   r.mm); return *this; }

    REV_INLINE v4u& REV_VECTORCALL operator+=(u32 r) { mm = _mm_add_epi32(mm,   _mm_set1_epi32(r)); return *this; }
    REV_INLINE v4u& REV_VECTORCALL operator-=(u32 r) { mm = _mm_sub_epi32(mm,   _mm_set1_epi32(r)); return *this; }
    REV_INLINE v4u& REV_VECTORCALL operator*=(u32 r) { mm = _mm_mullo_epi32(mm, _mm_set1_epi32(r)); return *this; }
    REV_INLINE v4u& REV_VECTORCALL operator/=(u32 r) { mm = _mm_div_epu32(mm,   _mm_set1_epi32(r)); return *this; }

    REV_INLINE u32  REV_VECTORCALL operator[](u8 i) const { REV_CHECK(i < 4); return e[i]; }
    REV_INLINE u32& REV_VECTORCALL operator[](u8 i)       { REV_CHECK(i < 4); return e[i]; }
};

REV_INLINE v4u REV_VECTORCALL operator+(v4u l, v4u r) { return v4u(_mm_add_epi32(l.mm,   r.mm)); }
REV_INLINE v4u REV_VECTORCALL operator-(v4u l, v4u r) { return v4u(_mm_sub_epi32(l.mm,   r.mm)); }
REV_INLINE v4u REV_VECTORCALL operator*(v4u l, v4u r) { return v4u(_mm_mullo_epi32(l.mm, r.mm)); }
REV_INLINE v4u REV_VECTORCALL operator/(v4u l, v4u r) { return v4u(_mm_div_epu32(l.mm,   r.mm)); }

REV_INLINE v4u REV_VECTORCALL operator+(v4u l, u32 r) { return v4u(_mm_add_epi32(l.mm,   _mm_set1_epi32(r))); }
REV_INLINE v4u REV_VECTORCALL operator-(v4u l, u32 r) { return v4u(_mm_sub_epi32(l.mm,   _mm_set1_epi32(r))); }
REV_INLINE v4u REV_VECTORCALL operator*(v4u l, u32 r) { return v4u(_mm_mullo_epi32(l.mm, _mm_set1_epi32(r))); }
REV_INLINE v4u REV_VECTORCALL operator/(v4u l, u32 r) { return v4u(_mm_div_epu32(l.mm,   _mm_set1_epi32(r))); }

REV_INLINE v4u REV_VECTORCALL operator+(u32 l, v4u r) { return v4u(_mm_add_epi32(_mm_set1_epi32(l),   r.mm)); }
REV_INLINE v4u REV_VECTORCALL operator-(u32 l, v4u r) { return v4u(_mm_sub_epi32(_mm_set1_epi32(l),   r.mm)); }
REV_INLINE v4u REV_VECTORCALL operator*(u32 l, v4u r) { return v4u(_mm_mullo_epi32(_mm_set1_epi32(l), r.mm)); }
REV_INLINE v4u REV_VECTORCALL operator/(u32 l, v4u r) { return v4u(_mm_div_epu32(_mm_set1_epi32(l),   r.mm)); }

REV_INLINE bool REV_VECTORCALL operator==(v4u l, v4u r)
{
#if REV_ISA >= REV_ISA_AVX512
    return 0b1111 == _mm_cmpeq_epi32_mask(l.mm, r.mm);
#else
    return _mm_testc_si128(_mm_cmpeq_epi64(l.mm, r.mm),
                           _mm_set1_epi64x(REV_U64_MAX));
#endif
}

REV_INLINE bool REV_VECTORCALL operator!=(v4u l, v4u r)
{
#if REV_ISA >= REV_ISA_AVX512
    return _mm_cmpneq_epi32_mask(l.mm, r.mm);
#else
    __m128i mask = _mm_set1_epi64x(REV_U64_MAX);
    return _mm_testc_si128(_mm_andnot_si128(_mm_cmpeq_epi32(l.mm, r.mm), mask), mask);
#endif
}

// @NOTE(Roman):
// sqrt(x1^2 + y1^2 + z1^2)   <=> sqrt(x2^2 + y2^2 + z2^2)
// sqrt(x1^2 + y1^2 + z1^2)^2 <=> sqrt(x2^2 + y2^2 + z2^2)^2
//      x1^2 + y1^2 + z1^2    <=>      x2^2 + y2^2 + z2^2
REV_INLINE bool REV_VECTORCALL operator<=(v4u l, v4u r) { return l.length_sq() <= r.length_sq(); }
REV_INLINE bool REV_VECTORCALL operator>=(v4u l, v4u r) { return l.length_sq() >= r.length_sq(); }
REV_INLINE bool REV_VECTORCALL operator< (v4u l, v4u r) { return l.length_sq() <  r.length_sq(); }
REV_INLINE bool REV_VECTORCALL operator> (v4u l, v4u r) { return l.length_sq() >  r.length_sq(); }


REV_INLINE v4s REV_VECTORCALL v4_to_v4s (v4  v) { return v4s(_mm_cvtps_epi32(v.mm)); }
REV_INLINE v4u REV_VECTORCALL v4_to_v4u (v4  v) { return v4u(_mm_cvtps_epi32(v.mm)); }
REV_INLINE v4  REV_VECTORCALL v4s_to_v4 (v4s v) { return v4 (_mm_cvtepi32_ps(v.mm)); }
REV_INLINE v4u REV_VECTORCALL v4s_to_v4u(v4s v) { return v4u(                v.mm ); }
REV_INLINE v4  REV_VECTORCALL v4u_to_v4 (v4u v) { return v4 (mm_cvtepu32_ps( v.mm)); }
REV_INLINE v4s REV_VECTORCALL v4u_to_v4s(v4u v) { return v4s(                v.mm ); }

}
#pragma pack(pop)
