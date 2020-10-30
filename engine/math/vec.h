//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "math/math.h"
#include "math/simd.h"

#pragma pack(push, 1)

//
// v2
//

union ENGINE_INTRIN_TYPE ENGINE_ALIGN(8) v2 final
{
    struct { f32 x, y; };
    struct { f32 w, h; };

    v2(f32 val = 0 ) : x(val),  y(val)    {}
    v2(f32 x, f32 y) : x(x),    y(y)      {}
    v2(f32 arr[2]  ) : x(*arr), y(arr[1]) {}
    v2(const v2& v ) : x(v.x),  y(v.y)    {}
    v2(v2&& v      ) : x(v.x),  y(v.y)    {}

    v2& __vectorcall operator=(f32 val    ) { x = val;  y = val;    return *this; }
    v2& __vectorcall operator=(f32 arr[2] ) { x = *arr; y = arr[1]; return *this; }
    v2& __vectorcall operator=(const v2& v) { x = v.x;  y = v.y;    return *this; }
    v2& __vectorcall operator=(v2&& v     ) { x = v.x;  y = v.y;    return *this; }

    f32 __vectorcall dot(v2 r) const { return x*r.x + y*r.y; }

    f32 __vectorcall length()    const { return sqrtf(x*x + y*y); }
    f32 __vectorcall length_sq() const { return x*x + y*y; }

    v2 __vectorcall normalize() const
    {
        f32 len = sqrtf(x*x + y*y);
        return v2(x / len, y / len);
    }

    static v2 __vectorcall lerp(v2 start, v2 end, v2 percent)
    {
        percent.x = __max(0.0f, __min(1.0f, percent.x));
        percent.y = __max(0.0f, __min(1.0f, percent.y));
        return v2((end.x - start.x) * percent.x + start.x,
                  (end.y - start.y) * percent.y + start.y);
    }

    static v2 __vectorcall clamp(v2 val, v2 min, v2 max)
    {
        return v2(__max(min.x, __min(max.x, val.x)),
                  __max(min.y, __min(max.y, val.y)));
    }

    v2& __vectorcall operator+=(v2 r)  { x += r.x; y += r.y; return *this; }
    v2& __vectorcall operator-=(v2 r)  { x -= r.x; y -= r.y; return *this; }
    v2& __vectorcall operator*=(v2 r)  { x *= r.x; y *= r.y; return *this; }
    v2& __vectorcall operator/=(v2 r)  { x /= r.x; y /= r.y; return *this; }

    v2& __vectorcall operator+=(f32 r) { x += r;   y += r;   return *this; }
    v2& __vectorcall operator-=(f32 r) { x -= r;   y -= r;   return *this; }
    v2& __vectorcall operator*=(f32 r) { x *= r;   y *= r;   return *this; }
    v2& __vectorcall operator/=(f32 r) { x /= r;   y /= r;   return *this; }
};

INLINE v2 __vectorcall operator+(v2 l, v2 r) { return v2(l.x + r.x, l.y + r.y); }
INLINE v2 __vectorcall operator-(v2 l, v2 r) { return v2(l.x - r.x, l.y - r.y); }
INLINE v2 __vectorcall operator*(v2 l, v2 r) { return v2(l.x * r.x, l.y * r.y); }
INLINE v2 __vectorcall operator/(v2 l, v2 r) { return v2(l.x / r.x, l.y / r.y); }

INLINE v2 __vectorcall operator+(v2 l, f32 r) { return v2(l.x + r, l.y + r); }
INLINE v2 __vectorcall operator-(v2 l, f32 r) { return v2(l.x - r, l.y - r); }
INLINE v2 __vectorcall operator*(v2 l, f32 r) { return v2(l.x * r, l.y * r); }
INLINE v2 __vectorcall operator/(v2 l, f32 r) { return v2(l.x / r, l.y / r); }

INLINE v2 __vectorcall operator+(f32 l, v2 r) { return v2(l + r.x, l + r.y); }
INLINE v2 __vectorcall operator-(f32 l, v2 r) { return v2(l - r.x, l - r.y); }
INLINE v2 __vectorcall operator*(f32 l, v2 r) { return v2(l * r.x, l * r.y); }
INLINE v2 __vectorcall operator/(f32 l, v2 r) { return v2(l / r.x, l / r.y); }

INLINE bool __vectorcall operator==(v2 l, v2 r) { return l.x == r.x && l.y == r.y; }
INLINE bool __vectorcall operator!=(v2 l, v2 r) { return l.x != r.x || l.y != r.y; }

// @NOTE(Roman):
// sqrt(x1^2 + y1^2)   <=> sqrt(x2^2 + y2^2)
// sqrt(x1^2 + y1^2)^2 <=> sqrt(x2^2 + y2^2)^2
//      x1^2 + y1^2    <=>      x2^2 + y2^2
INLINE bool __vectorcall operator<=(v2 l, v2 r) { return l.length_sq() <= r.length_sq(); }
INLINE bool __vectorcall operator>=(v2 l, v2 r) { return l.length_sq() >= r.length_sq(); }
INLINE bool __vectorcall operator< (v2 l, v2 r) { return l.length_sq() <  r.length_sq(); }
INLINE bool __vectorcall operator> (v2 l, v2 r) { return l.length_sq() >  r.length_sq(); }

union ENGINE_INTRIN_TYPE ENGINE_ALIGN(8) v2s final
{
    struct { s32 x, y; };
    struct { s32 w, h; };

    v2s(s32 val = 0 ) : x(val),  y(val)    {}
    v2s(s32 x, s32 y) : x(x),    y(y)      {}
    v2s(s32 arr[2]  ) : x(*arr), y(arr[1]) {}
    v2s(const v2s& v) : x(v.x),  y(v.y)    {}
    v2s(v2s&& v     ) : x(v.x),  y(v.y)    {}

    v2s& __vectorcall operator=(s32 val     ) { x = val;  y = val;    return *this; }
    v2s& __vectorcall operator=(s32 arr[2]  ) { x = *arr; y = arr[1]; return *this; }
    v2s& __vectorcall operator=(const v2s& v) { x = v.x;  y = v.y;    return *this; }
    v2s& __vectorcall operator=(v2s&& v     ) { x = v.x;  y = v.y;    return *this; }

    s32 __vectorcall dot(v2s r) const { return x*r.x + y*r.y; }

    f32 __vectorcall length()    const { return sqrtf(cast<f32>(x*x + y*y)); }
    s32 __vectorcall length_sq() const { return x*x + y*y;                   }

    static v2s __vectorcall clamp(v2s val, v2s min, v2s max)
    {
        return v2s(__max(min.x, __min(max.x, val.x)),
                   __max(min.y, __min(max.y, val.y)));
    }

    v2s& __vectorcall operator+=(v2s r) { x += r.x; y += r.y; return *this; }
    v2s& __vectorcall operator-=(v2s r) { x -= r.x; y -= r.y; return *this; }
    v2s& __vectorcall operator*=(v2s r) { x *= r.x; y *= r.y; return *this; }
    v2s& __vectorcall operator/=(v2s r) { x /= r.x; y /= r.y; return *this; }

    v2s& __vectorcall operator+=(s32 r) { x += r;   y += r;   return *this; }
    v2s& __vectorcall operator-=(s32 r) { x -= r;   y -= r;   return *this; }
    v2s& __vectorcall operator*=(s32 r) { x *= r;   y *= r;   return *this; }
    v2s& __vectorcall operator/=(s32 r) { x /= r;   y /= r;   return *this; }
};

INLINE v2s __vectorcall operator+(v2s l, v2s r) { return v2s(l.x + r.x, l.y + r.y); }
INLINE v2s __vectorcall operator-(v2s l, v2s r) { return v2s(l.x - r.x, l.y - r.y); }
INLINE v2s __vectorcall operator*(v2s l, v2s r) { return v2s(l.x * r.x, l.y * r.y); }
INLINE v2s __vectorcall operator/(v2s l, v2s r) { return v2s(l.x / r.x, l.y / r.y); }

INLINE v2s __vectorcall operator+(v2s l, s32 r) { return v2s(l.x + r, l.y + r); }
INLINE v2s __vectorcall operator-(v2s l, s32 r) { return v2s(l.x - r, l.y - r); }
INLINE v2s __vectorcall operator*(v2s l, s32 r) { return v2s(l.x * r, l.y * r); }
INLINE v2s __vectorcall operator/(v2s l, s32 r) { return v2s(l.x / r, l.y / r); }

INLINE v2s __vectorcall operator+(s32 l, v2s r) { return v2s(l + r.x, l + r.y); }
INLINE v2s __vectorcall operator-(s32 l, v2s r) { return v2s(l - r.x, l - r.y); }
INLINE v2s __vectorcall operator*(s32 l, v2s r) { return v2s(l * r.x, l * r.y); }
INLINE v2s __vectorcall operator/(s32 l, v2s r) { return v2s(l / r.x, l / r.y); }

INLINE bool __vectorcall operator==(v2s l, v2s r) { return l.x == r.x && l.y == r.y; }
INLINE bool __vectorcall operator!=(v2s l, v2s r) { return l.x != r.x || l.y != r.y; }

// @NOTE(Roman):
// sqrt(x1^2 + y1^2)   <=> sqrt(x2^2 + y2^2)
// sqrt(x1^2 + y1^2)^2 <=> sqrt(x2^2 + y2^2)^2
//      x1^2 + y1^2    <=>      x2^2 + y2^2
INLINE bool __vectorcall operator<=(v2s l, v2s r) { return l.length_sq() <= r.length_sq(); }
INLINE bool __vectorcall operator>=(v2s l, v2s r) { return l.length_sq() >= r.length_sq(); }
INLINE bool __vectorcall operator< (v2s l, v2s r) { return l.length_sq() <  r.length_sq(); }
INLINE bool __vectorcall operator> (v2s l, v2s r) { return l.length_sq() >  r.length_sq(); }

union ENGINE_INTRIN_TYPE ENGINE_ALIGN(8) v2u final
{
    struct { u32 x, y; };
    struct { u32 w, h; };
    struct { u32 r, c; };

    v2u(u32 val = 0 ) : x(val),  y(val)    {}
    v2u(u32 x, u32 y) : x(x),    y(y)      {}
    v2u(u32 arr[2]  ) : x(*arr), y(arr[1]) {}
    v2u(const v2u& v) : x(v.x),  y(v.y)    {}
    v2u(v2u&& v     ) : x(v.x),  y(v.y)    {}

    v2u& __vectorcall operator=(u32 val     ) { x = val;  y = val;    return *this; }
    v2u& __vectorcall operator=(u32 arr[2]  ) { x = *arr; y = arr[1]; return *this; }
    v2u& __vectorcall operator=(const v2u& v) { x = v.x;  y = v.y;    return *this; }
    v2u& __vectorcall operator=(v2u&& v     ) { x = v.x;  y = v.y;    return *this; }
    
    u32 __vectorcall dot(v2u r) const { return x*r.x + y*r.y; }

    f32 __vectorcall length()    const { return sqrtf(cast<f32>(x*x + y*y)); }
    u32 __vectorcall length_sq() const { return x*x + y*y; }

    static v2u __vectorcall clamp(v2u val, v2u min, v2u max)
    {
        return v2u(__max(min.x, __min(max.x, val.x)),
                   __max(min.y, __min(max.y, val.y)));
    }

    v2u& __vectorcall operator+=(v2u r) { x += r.x; y += r.y; return *this; }
    v2u& __vectorcall operator-=(v2u r) { x -= r.x; y -= r.y; return *this; }
    v2u& __vectorcall operator*=(v2u r) { x *= r.x; y *= r.y; return *this; }
    v2u& __vectorcall operator/=(v2u r) { x /= r.x; y /= r.y; return *this; }

    v2u& __vectorcall operator+=(u32 r) { x += r;   y += r;   return *this; }
    v2u& __vectorcall operator-=(u32 r) { x -= r;   y -= r;   return *this; }
    v2u& __vectorcall operator*=(u32 r) { x *= r;   y *= r;   return *this; }
    v2u& __vectorcall operator/=(u32 r) { x /= r;   y /= r;   return *this; }
};

INLINE v2u __vectorcall operator+(v2u l, v2u r) { return v2u(l.x + r.x, l.y + r.y); }
INLINE v2u __vectorcall operator-(v2u l, v2u r) { return v2u(l.x - r.x, l.y - r.y); }
INLINE v2u __vectorcall operator*(v2u l, v2u r) { return v2u(l.x * r.x, l.y * r.y); }
INLINE v2u __vectorcall operator/(v2u l, v2u r) { return v2u(l.x / r.x, l.y / r.y); }

INLINE v2u __vectorcall operator+(v2u l, u32 r) { return v2u(l.x + r, l.y + r); }
INLINE v2u __vectorcall operator-(v2u l, u32 r) { return v2u(l.x - r, l.y - r); }
INLINE v2u __vectorcall operator*(v2u l, u32 r) { return v2u(l.x * r, l.y * r); }
INLINE v2u __vectorcall operator/(v2u l, u32 r) { return v2u(l.x / r, l.y / r); }

INLINE v2u __vectorcall operator+(u32 l, v2u r) { return v2u(l + r.x, l + r.y); }
INLINE v2u __vectorcall operator-(u32 l, v2u r) { return v2u(l - r.x, l - r.y); }
INLINE v2u __vectorcall operator*(u32 l, v2u r) { return v2u(l * r.x, l * r.y); }
INLINE v2u __vectorcall operator/(u32 l, v2u r) { return v2u(l / r.x, l / r.y); }

INLINE bool __vectorcall operator==(v2u l, v2u r) { return l.x == r.x && l.y == r.y; }
INLINE bool __vectorcall operator!=(v2u l, v2u r) { return l.x != r.x || l.y != r.y; }

// @NOTE(Roman):
// sqrt(x1^2 + y1^2)   <=> sqrt(x2^2 + y2^2)
// sqrt(x1^2 + y1^2)^2 <=> sqrt(x2^2 + y2^2)^2
//      x1^2 + y1^2    <=>      x2^2 + y2^2
INLINE bool __vectorcall operator<=(v2u l, v2u r) { return l.length_sq() <= r.length_sq(); }
INLINE bool __vectorcall operator>=(v2u l, v2u r) { return l.length_sq() >= r.length_sq(); }
INLINE bool __vectorcall operator< (v2u l, v2u r) { return l.length_sq() <  r.length_sq(); }
INLINE bool __vectorcall operator> (v2u l, v2u r) { return l.length_sq() >  r.length_sq(); }


INLINE v2s __vectorcall v2_to_v2s (v2  v) { return v2s(cast<s32>(roundf(v.x)), cast<s32>(roundf(v.y))); }
INLINE v2u __vectorcall v2_to_v2u (v2  v) { return v2u(cast<u32>(roundf(v.x)), cast<u32>(roundf(v.y))); }
INLINE v2  __vectorcall v2s_to_v2 (v2s v) { return v2 (cast<f32>(       v.x ), cast<f32>(       v.y )); }
INLINE v2u __vectorcall v2s_to_v2u(v2s v) { return v2u(cast<u32>(       v.x ), cast<u32>(       v.y )); }
INLINE v2  __vectorcall v2u_to_v2 (v2u v) { return v2 (cast<f32>(       v.x ), cast<f32>(       v.y )); }
INLINE v2s __vectorcall v2u_to_v2s(v2u v) { return v2s(cast<s32>(       v.x ), cast<s32>(       v.y )); }

//
// v3
//

union ENGINE_INTRIN_TYPE v3 final
{
    struct { f32 x, y, z; };
    struct { f32 r, g, b; };
    v2 xy;
    v2 rg;
    f32 e[3];

    v3(                   ) : x(0),                      y(0),                      z(0)                      {}
    v3(f32 val            ) : x(val),                    y(val),                    z(val)                    {}
    v3(f32 x, f32 y, f32 z) : x(x),                      y(y),                      z(z)                      {}
    v3(f32 arr[3]         ) : x(arr[0]),                 y(arr[1]),                 z(arr[2])                 {}
    v3(v2 xy, f32 z       ) : x(xy.x),                   y(xy.y),                   z(z)                      {}
    v3(__m128 _mm         ) : x(mm_extract_f32<0>(_mm)), y(mm_extract_f32<1>(_mm)), z(mm_extract_f32<2>(_mm)) {}
    v3(const v3& v        ) : x(v.x),                    y(v.y),                    z(v.z)                    {}
    v3(v3&& v             ) : x(v.x),                    y(v.y),                    z(v.z)                    {}

    v3& __vectorcall operator=(f32 val    ) { x = val;                    y = val;                    z = val;                    return *this; }
    v3& __vectorcall operator=(f32 arr[3] ) { x = arr[0];                 y = arr[1];                 z = arr[2];                 return *this; }
    v3& __vectorcall operator=(__m128 _mm ) { x = mm_extract_f32<0>(_mm); y = mm_extract_f32<1>(_mm); z = mm_extract_f32<2>(_mm); return *this; }
    v3& __vectorcall operator=(const v3& v) { x = v.x;                    y = v.y;                    z = v.z;                    return *this; }
    v3& __vectorcall operator=(v3&& v     ) { x = v.x;                    y = v.y;                    z = v.z;                    return *this; }

    f32 __vectorcall dot(v3 r) const
    {
        __m128 l_mm = _mm_setr_ps(  x,   y,   z, 0.0f);
        __m128 r_mm = _mm_setr_ps(r.x, r.y, r.z, 0.0f);
        return _mm_cvtss_f32(_mm_dp_ps(l_mm, r_mm, 0x71));
    }

    v3 __vectorcall cross(v3 r) const
    {
        //  [x]   [r.x]   [ (y * r.z - z * r.y)]   [y * r.z - z * r.y]
        //  [y] x [r.y] = [-(x * r.z - z * r.x)] = [z * r.x - x * r.z];
        //  [z]   [r.z]   [ (x * r.y - y * r.x)]   [x * r.y - y * r.x]
        
        __m128 l_mm = _mm_setr_ps(  x,   y,   z, 0.0f);
        __m128 r_mm = _mm_setr_ps(r.x, r.y, r.z, 0.0f);

        return v3(_mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(l_mm, l_mm, cast<s32>(MM_SHUFFLE::YZXW)),
                                        _mm_shuffle_ps(r_mm, r_mm, cast<s32>(MM_SHUFFLE::ZXYW))),
                             _mm_mul_ps(_mm_shuffle_ps(l_mm, l_mm, cast<s32>(MM_SHUFFLE::ZXYW)),
                                        _mm_shuffle_ps(r_mm, r_mm, cast<s32>(MM_SHUFFLE::YZXW)))));
    }

    f32 __vectorcall length() const
    {
        __m128 mm = _mm_setr_ps(x, y, z, 0.0f);
        return _mm_cvtss_f32(_mm_sqrt_ss(_mm_dp_ps(mm, mm, 0x71)));
    }

    f32 __vectorcall length_sq() const
    {
        __m128 mm = _mm_setr_ps(x, y, z, 0.0f);
        return _mm_cvtss_f32(_mm_dp_ps(mm, mm, 0x71));
    }

    v3 __vectorcall normalize() const
    {
        __m128 mm = _mm_setr_ps(x, y, z, 0.0f);
        return v3(_mm_div_ps(mm, _mm_sqrt_ps(_mm_dp_ps(mm, mm, 0x7F))));
    }

    static v3 __vectorcall lerp(v3 start, v3 end, v3 percent)
    {
        __m128 s_mm = _mm_setr_ps(start.x, start.y, start.z, 0.0f);
        __m128 e_mm = _mm_setr_ps(end.x, end.y, end.z, 0.0f);
        __m128 p_mm = _mm_setr_ps(percent.x, percent.y, percent.z, 0.0f);

        __m128 per = _mm_max_ps(_mm_setzero_ps(), _mm_min_ps(_mm_set_ps1(1.0f), p_mm));
        return v3(_mm_add_ps(_mm_mul_ps(_mm_sub_ps(e_mm, s_mm), per), s_mm));
    }

    static v3 __vectorcall clamp(v3 val, v3 min, v3 max)
    {
        __m128 val_mm = _mm_setr_ps(val.x, val.y, val.z, 0.0f);
        __m128 min_mm = _mm_setr_ps(min.x, min.y, min.z, 0.0f);
        __m128 max_mm = _mm_setr_ps(max.x, max.y, max.z, 0.0f);

        return v3(_mm_max_ps(min_mm, _mm_min_ps(max_mm, val_mm)));
    }

    v3& __vectorcall operator+=(v3 r) { __m128 mm = _mm_add_ps(_mm_setr_ps(x, y, z, 0.0f), _mm_setr_ps(r.x, r.y, r.z, 0.0f)); x = mm_extract_f32<0>(mm); y = mm_extract_f32<1>(mm); z = mm_extract_f32<2>(mm); return *this; }
    v3& __vectorcall operator-=(v3 r) { __m128 mm = _mm_sub_ps(_mm_setr_ps(x, y, z, 0.0f), _mm_setr_ps(r.x, r.y, r.z, 0.0f)); x = mm_extract_f32<0>(mm); y = mm_extract_f32<1>(mm); z = mm_extract_f32<2>(mm); return *this; }
    v3& __vectorcall operator*=(v3 r) { __m128 mm = _mm_mul_ps(_mm_setr_ps(x, y, z, 0.0f), _mm_setr_ps(r.x, r.y, r.z, 0.0f)); x = mm_extract_f32<0>(mm); y = mm_extract_f32<1>(mm); z = mm_extract_f32<2>(mm); return *this; }
    v3& __vectorcall operator/=(v3 r) { __m128 mm = _mm_div_ps(_mm_setr_ps(x, y, z, 0.0f), _mm_setr_ps(r.x, r.y, r.z, 0.0f)); x = mm_extract_f32<0>(mm); y = mm_extract_f32<1>(mm); z = mm_extract_f32<2>(mm); return *this; }

    v3& __vectorcall operator+=(f32 r) { __m128 mm = _mm_add_ps(_mm_setr_ps(x, y, z, 0.0f), _mm_setr_ps(r, r, r, 0.0f)); x = mm_extract_f32<0>(mm); y = mm_extract_f32<1>(mm); z = mm_extract_f32<2>(mm); return *this; }
    v3& __vectorcall operator-=(f32 r) { __m128 mm = _mm_sub_ps(_mm_setr_ps(x, y, z, 0.0f), _mm_setr_ps(r, r, r, 0.0f)); x = mm_extract_f32<0>(mm); y = mm_extract_f32<1>(mm); z = mm_extract_f32<2>(mm); return *this; }
    v3& __vectorcall operator*=(f32 r) { __m128 mm = _mm_mul_ps(_mm_setr_ps(x, y, z, 0.0f), _mm_setr_ps(r, r, r, 0.0f)); x = mm_extract_f32<0>(mm); y = mm_extract_f32<1>(mm); z = mm_extract_f32<2>(mm); return *this; }
    v3& __vectorcall operator/=(f32 r) { __m128 mm = _mm_div_ps(_mm_setr_ps(x, y, z, 0.0f), _mm_setr_ps(r, r, r, 0.0f)); x = mm_extract_f32<0>(mm); y = mm_extract_f32<1>(mm); z = mm_extract_f32<2>(mm); return *this; }

    f32  __vectorcall operator[](u8 i) const { Check(i < 3); return e[i]; }
    f32& __vectorcall operator[](u8 i)       { Check(i < 3); return e[i]; }
};

INLINE v3 __vectorcall operator+(v3 l, v3 r) { return v3(_mm_add_ps(_mm_setr_ps(l.x, l.y, l.z, 0.0f), _mm_setr_ps(r.x, r.y, r.z, 0.0f))); }
INLINE v3 __vectorcall operator-(v3 l, v3 r) { return v3(_mm_sub_ps(_mm_setr_ps(l.x, l.y, l.z, 0.0f), _mm_setr_ps(r.x, r.y, r.z, 0.0f))); }
INLINE v3 __vectorcall operator*(v3 l, v3 r) { return v3(_mm_mul_ps(_mm_setr_ps(l.x, l.y, l.z, 0.0f), _mm_setr_ps(r.x, r.y, r.z, 0.0f))); }
INLINE v3 __vectorcall operator/(v3 l, v3 r) { return v3(_mm_div_ps(_mm_setr_ps(l.x, l.y, l.z, 0.0f), _mm_setr_ps(r.x, r.y, r.z, 0.0f))); }

INLINE v3 __vectorcall operator+(v3 l, f32 r) { return v3(_mm_add_ps(_mm_setr_ps(l.x, l.y, l.z, 0.0f), _mm_setr_ps(r, r, r, 0.0f))); }
INLINE v3 __vectorcall operator-(v3 l, f32 r) { return v3(_mm_sub_ps(_mm_setr_ps(l.x, l.y, l.z, 0.0f), _mm_setr_ps(r, r, r, 0.0f))); }
INLINE v3 __vectorcall operator*(v3 l, f32 r) { return v3(_mm_mul_ps(_mm_setr_ps(l.x, l.y, l.z, 0.0f), _mm_setr_ps(r, r, r, 0.0f))); }
INLINE v3 __vectorcall operator/(v3 l, f32 r) { return v3(_mm_div_ps(_mm_setr_ps(l.x, l.y, l.z, 0.0f), _mm_setr_ps(r, r, r, 0.0f))); }

INLINE v3 __vectorcall operator+(f32 l, v3 r) { return v3(_mm_add_ps(_mm_setr_ps(l, l, l, 0.0f), _mm_setr_ps(r.x, r.y, r.z, 0.0f))); }
INLINE v3 __vectorcall operator-(f32 l, v3 r) { return v3(_mm_sub_ps(_mm_setr_ps(l, l, l, 0.0f), _mm_setr_ps(r.x, r.y, r.z, 0.0f))); }
INLINE v3 __vectorcall operator*(f32 l, v3 r) { return v3(_mm_mul_ps(_mm_setr_ps(l, l, l, 0.0f), _mm_setr_ps(r.x, r.y, r.z, 0.0f))); }
INLINE v3 __vectorcall operator/(f32 l, v3 r) { return v3(_mm_div_ps(_mm_setr_ps(l, l, l, 0.0f), _mm_setr_ps(r.x, r.y, r.z, 0.0f))); }

INLINE bool __vectorcall operator==(v3 l, v3 r)
{
    __m128 l_mm = _mm_setr_ps(l.x, l.y, l.z, 0.0f);
    __m128 r_mm = _mm_setr_ps(r.x, r.y, r.z, 0.0f);
#if ENGINE_ISA >= ENGINE_ISA_AVX512
    return 0b11 == _mm_cmpeq_epi64_mask(*cast<__m128i *>(&l_mm),
                                        *cast<__m128i *>(&r_mm));
#else
    __m128 eq_mask = _mm_cmpeq_ps(l_mm, r_mm);
    __m128 ones    = _mm_cmpeq_ps(eq_mask, eq_mask);
    return _mm_testc_ps(eq_mask, ones);
#endif
}

INLINE bool __vectorcall operator!=(v3 l, v3 r)
{
    __m128 l_mm = _mm_setr_ps(l.x, l.y, l.z, 0.0f);
    __m128 r_mm = _mm_setr_ps(r.x, r.y, r.z, 0.0f);
#if ENGINE_ISA >= ENGINE_ISA_AVX512
    return 0b11 != _mm_cmpeq_epi64_mask(*cast<__m128i *>(&l_mm),
                                        *cast<__m128i *>(&r_mm));
#else
    __m128 eq_mask = _mm_cmpeq_ps(l_mm, r_mm);
    __m128 ones    = _mm_cmpeq_ps(eq_mask, eq_mask);
    return !_mm_testc_ps(eq_mask, ones);
#endif
}

// @NOTE(Roman):
// sqrt(x1^2 + y1^2 + z1^2)   <=> sqrt(x2^2 + y2^2 + z2^2)
// sqrt(x1^2 + y1^2 + z1^2)^2 <=> sqrt(x2^2 + y2^2 + z2^2)^2
//      x1^2 + y1^2 + z1^2    <=>      x2^2 + y2^2 + z2^2
INLINE bool __vectorcall operator<=(v3 l, v3 r) { return l.length_sq() <= r.length_sq(); }
INLINE bool __vectorcall operator>=(v3 l, v3 r) { return l.length_sq() >= r.length_sq(); }
INLINE bool __vectorcall operator< (v3 l, v3 r) { return l.length_sq() <  r.length_sq(); }
INLINE bool __vectorcall operator> (v3 l, v3 r) { return l.length_sq() >  r.length_sq(); }

union ENGINE_INTRIN_TYPE v3s final
{
    struct { s32 x, y, z; };
    struct { s32 r, g, b; };
    v2s xy;
    v2s rg;
    s32 e[3];
    
    v3s(                   ) : x(0),                      y(0),                      z(0)                      {}
    v3s(s32 val            ) : x(val),                    y(val),                    z(val)                    {}
    v3s(s32 x, s32 y, s32 z) : x(x),                      y(y),                      z(z)                      {}
    v3s(s32 arr[3]         ) : x(arr[0]),                 y(arr[1]),                 z(arr[2])                 {}
    v3s(v2s xy, s32 z      ) : x(xy.x),                   y(xy.y),                   z(z)                      {}
    v3s(__m128i _mm        ) : x(mm_extract_s32<0>(_mm)), y(mm_extract_s32<1>(_mm)), z(mm_extract_s32<2>(_mm)) {}
    v3s(const v3s& v       ) : x(v.x),                    y(v.y),                    z(v.z)                    {}
    v3s(v3s&& v            ) : x(v.x),                    y(v.y),                    z(v.z)                    {}

    v3s& __vectorcall operator=(s32 val     ) { x = val;                    y = val;                    z = val;                    return *this; }
    v3s& __vectorcall operator=(s32 arr[3]  ) { x = arr[0];                 y = arr[1];                 z = arr[2];                 return *this; }
    v3s& __vectorcall operator=(__m128i _mm ) { x = mm_extract_s32<0>(_mm); y = mm_extract_s32<1>(_mm); z = mm_extract_s32<2>(_mm); return *this; }
    v3s& __vectorcall operator=(const v3s& v) { x = v.x;                    y = v.y;                    z = v.z;                    return *this; }
    v3s& __vectorcall operator=(v3s&& v     ) { x = v.x;                    y = v.y;                    z = v.z;                    return *this; }

    s32 __vectorcall dot(v3s r) const
    {
        __m128i m = _mm_mul_epi32(_mm_setr_epi32(x, y, z, 0), _mm_setr_epi32(r.x, r.y, r.z, 0));
        __m128i h = _mm_hadd_epi32(m, m);
        __m128i d = _mm_hadd_epi32(h, h);
        return _mm_cvtsi128_si32(d);
    }

    v3s __vectorcall cross(v3s r) const
    {
        //  [x]   [r.x]   [ (y * r.z - z * r.y)]   [y * r.z - z * r.y]
        //  [y] x [r.y] = [-(x * r.z - z * r.x)] = [z * r.x - x * r.z];
        //  [z]   [r.z]   [ (x * r.y - y * r.x)]   [x * r.y - y * r.x]
        
        __m128i l_mm = _mm_setr_epi32(  x,   y,   z, 0);
        __m128i r_mm = _mm_setr_epi32(r.x, r.y, r.z, 0);

        return v3s(_mm_sub_epi32(_mm_mul_epi32(_mm_shuffle_epi32(l_mm, cast<s32>(MM_SHUFFLE::YZXW)),
                                               _mm_shuffle_epi32(r_mm, cast<s32>(MM_SHUFFLE::ZXYW))),
                                 _mm_mul_epi32(_mm_shuffle_epi32(l_mm, cast<s32>(MM_SHUFFLE::ZXYW)),
                                               _mm_shuffle_epi32(r_mm, cast<s32>(MM_SHUFFLE::YZXW)))));
    }

    f32 __vectorcall length() const
    {
        __m128 m = _mm_cvtepi32_ps(_mm_setr_epi32(x, y, z, 0));
        return _mm_cvtss_f32(_mm_sqrt_ss(_mm_dp_ps(m, m, 0x71)));
    }

    s32 __vectorcall length_sq() const
    {
        __m128i mm = _mm_setr_epi32(x, y, z, 0);
        __m128i m  = _mm_mul_epi32(mm, mm);
        __m128i h  = _mm_hadd_epi32(m, m);
        __m128i d  = _mm_hadd_epi32(h, h);
        return _mm_cvtsi128_si32(d);
    }

    static v3s __vectorcall clamp(v3s val, v3s min, v3s max)
    {
        __m128i val_mm = _mm_setr_epi32(val.x, val.y, val.z, 0);
        __m128i min_mm = _mm_setr_epi32(min.x, min.y, min.z, 0);
        __m128i max_mm = _mm_setr_epi32(max.x, max.y, max.z, 0);
        return v3s(_mm_max_epi32(min_mm, _mm_min_epi32(max_mm, val_mm)));
    }

    v3s& __vectorcall operator+=(v3s r) { __m128i mm = _mm_add_epi32(_mm_setr_epi32(x, y, z, 0), _mm_setr_epi32(r.x, r.y, r.z, 0)); x = mm_extract_s32<0>(mm); y = mm_extract_s32<1>(mm); z = mm_extract_s32<2>(mm); return *this; }
    v3s& __vectorcall operator-=(v3s r) { __m128i mm = _mm_sub_epi32(_mm_setr_epi32(x, y, z, 0), _mm_setr_epi32(r.x, r.y, r.z, 0)); x = mm_extract_s32<0>(mm); y = mm_extract_s32<1>(mm); z = mm_extract_s32<2>(mm); return *this; }
    v3s& __vectorcall operator*=(v3s r) { __m128i mm = _mm_mul_epi32(_mm_setr_epi32(x, y, z, 0), _mm_setr_epi32(r.x, r.y, r.z, 0)); x = mm_extract_s32<0>(mm); y = mm_extract_s32<1>(mm); z = mm_extract_s32<2>(mm); return *this; }
    v3s& __vectorcall operator/=(v3s r) { __m128i mm = _mm_div_epi32(_mm_setr_epi32(x, y, z, 0), _mm_setr_epi32(r.x, r.y, r.z, 0)); x = mm_extract_s32<0>(mm); y = mm_extract_s32<1>(mm); z = mm_extract_s32<2>(mm); return *this; }

    v3s& __vectorcall operator+=(s32 r) { __m128i mm = _mm_add_epi32(_mm_setr_epi32(x, y, z, 0), _mm_setr_epi32(r, r, r, 0)); x = mm_extract_s32<0>(mm); y = mm_extract_s32<1>(mm); z = mm_extract_s32<2>(mm); return *this; }
    v3s& __vectorcall operator-=(s32 r) { __m128i mm = _mm_sub_epi32(_mm_setr_epi32(x, y, z, 0), _mm_setr_epi32(r, r, r, 0)); x = mm_extract_s32<0>(mm); y = mm_extract_s32<1>(mm); z = mm_extract_s32<2>(mm); return *this; }
    v3s& __vectorcall operator*=(s32 r) { __m128i mm = _mm_mul_epi32(_mm_setr_epi32(x, y, z, 0), _mm_setr_epi32(r, r, r, 0)); x = mm_extract_s32<0>(mm); y = mm_extract_s32<1>(mm); z = mm_extract_s32<2>(mm); return *this; }
    v3s& __vectorcall operator/=(s32 r) { __m128i mm = _mm_div_epi32(_mm_setr_epi32(x, y, z, 0), _mm_setr_epi32(r, r, r, 0)); x = mm_extract_s32<0>(mm); y = mm_extract_s32<1>(mm); z = mm_extract_s32<2>(mm); return *this; }

    s32  __vectorcall operator[](u8 i) const { Check(i < 3); return e[i]; }
    s32& __vectorcall operator[](u8 i)       { Check(i < 3); return e[i]; }
};

INLINE v3s __vectorcall operator+(v3s l, v3s r) { return v3s(_mm_add_epi32(_mm_setr_epi32(l.x, l.y, l.z, 0), _mm_setr_epi32(r.x, r.y, r.z, 0))); }
INLINE v3s __vectorcall operator-(v3s l, v3s r) { return v3s(_mm_sub_epi32(_mm_setr_epi32(l.x, l.y, l.z, 0), _mm_setr_epi32(r.x, r.y, r.z, 0))); }
INLINE v3s __vectorcall operator*(v3s l, v3s r) { return v3s(_mm_mul_epi32(_mm_setr_epi32(l.x, l.y, l.z, 0), _mm_setr_epi32(r.x, r.y, r.z, 0))); }
INLINE v3s __vectorcall operator/(v3s l, v3s r) { return v3s(_mm_div_epi32(_mm_setr_epi32(l.x, l.y, l.z, 0), _mm_setr_epi32(r.x, r.y, r.z, 0))); }

INLINE v3s __vectorcall operator+(v3s l, s32 r) { return v3s(_mm_add_epi32(_mm_setr_epi32(l.x, l.y, l.z, 0), _mm_setr_epi32(r, r, r, 0))); }
INLINE v3s __vectorcall operator-(v3s l, s32 r) { return v3s(_mm_sub_epi32(_mm_setr_epi32(l.x, l.y, l.z, 0), _mm_setr_epi32(r, r, r, 0))); }
INLINE v3s __vectorcall operator*(v3s l, s32 r) { return v3s(_mm_mul_epi32(_mm_setr_epi32(l.x, l.y, l.z, 0), _mm_setr_epi32(r, r, r, 0))); }
INLINE v3s __vectorcall operator/(v3s l, s32 r) { return v3s(_mm_div_epi32(_mm_setr_epi32(l.x, l.y, l.z, 0), _mm_setr_epi32(r, r, r, 0))); }

INLINE v3s __vectorcall operator+(s32 l, v3s r) { return v3s(_mm_add_epi32(_mm_setr_epi32(l, l, l, 0), _mm_setr_epi32(r.x, r.y, r.z, 0))); }
INLINE v3s __vectorcall operator-(s32 l, v3s r) { return v3s(_mm_sub_epi32(_mm_setr_epi32(l, l, l, 0), _mm_setr_epi32(r.x, r.y, r.z, 0))); }
INLINE v3s __vectorcall operator*(s32 l, v3s r) { return v3s(_mm_mul_epi32(_mm_setr_epi32(l, l, l, 0), _mm_setr_epi32(r.x, r.y, r.z, 0))); }
INLINE v3s __vectorcall operator/(s32 l, v3s r) { return v3s(_mm_div_epi32(_mm_setr_epi32(l, l, l, 0), _mm_setr_epi32(r.x, r.y, r.z, 0))); }

INLINE bool __vectorcall operator==(v3s l, v3s r)
{
    __m128i l_mm = _mm_setr_epi32(l.x, l.y, l.z, 0);
    __m128i r_mm = _mm_setr_epi32(r.x, r.y, r.z, 0);
#if ENGINE_ISA >= ENGINE_ISA_AVX512
    return 0b11 == _mm_cmpeq_epi64_mask(l_mm, r_mm);
#else
    __m128i eq_mask = _mm_cmpeq_epi64(l_mm, r_mm);
    __m128i ones    = _mm_cmpeq_epi64(eq_mask, eq_mask);
    return _mm_testc_si128(eq_mask, ones);
#endif
}

INLINE bool __vectorcall operator!=(v3s l, v3s r)
{
    __m128i l_mm = _mm_setr_epi32(l.x, l.y, l.z, 0);
    __m128i r_mm = _mm_setr_epi32(r.x, r.y, r.z, 0);
#if ENGINE_ISA >= ENGINE_ISA_AVX512
    return 0b11 != _mm_cmpeq_epi64_mask(l_mm, r_mm);
#else
    __m128i eq_mask = _mm_cmpeq_epi64(l_mm, r_mm);
    __m128i ones    = _mm_cmpeq_epi64(eq_mask, eq_mask);
    return !_mm_testc_si128(eq_mask, ones);
#endif
}

// @NOTE(Roman):
// sqrt(x1^2 + y1^2 + z1^2)   <=> sqrt(x2^2 + y2^2 + z2^2)
// sqrt(x1^2 + y1^2 + z1^2)^2 <=> sqrt(x2^2 + y2^2 + z2^2)^2
//      x1^2 + y1^2 + z1^2    <=>      x2^2 + y2^2 + z2^2
INLINE bool __vectorcall operator<=(v3s l, v3s r) { return l.length_sq() <= r.length_sq(); }
INLINE bool __vectorcall operator>=(v3s l, v3s r) { return l.length_sq() >= r.length_sq(); }
INLINE bool __vectorcall operator< (v3s l, v3s r) { return l.length_sq() <  r.length_sq(); }
INLINE bool __vectorcall operator> (v3s l, v3s r) { return l.length_sq() >  r.length_sq(); }

union ENGINE_INTRIN_TYPE v3u final
{
    struct { u32 x, y, z; };
    struct { u32 r, g, b; };
    v2u xy;
    v2u rg;
    u32 e[3];

    v3u(                   ) : x(0),                      y(0),                      z(0)                      {}
    v3u(u32 val            ) : x(val),                    y(val),                    z(val)                    {}
    v3u(u32 x, u32 y, u32 z) : x(x),                      y(y),                      z(z)                      {}
    v3u(u32 arr[3]         ) : x(arr[0]),                 y(arr[1]),                 z(arr[2])                 {}
    v3u(v2u xy, u32 z      ) : x(xy.x),                   y(xy.y),                   z(z)                      {}
    v3u(__m128i _mm        ) : x(mm_extract_u32<0>(_mm)), y(mm_extract_u32<1>(_mm)), z(mm_extract_u32<2>(_mm)) {}
    v3u(const v3u& v       ) : x(v.x),                    y(v.y),                    z(v.z)                    {}
    v3u(v3u&& v            ) : x(v.x),                    y(v.y),                    z(v.z)                    {}

    v3u& __vectorcall operator=(u32 val     ) { x = val;                    y = val;                    z = val;                    return *this; }
    v3u& __vectorcall operator=(u32 arr[3]  ) { x = arr[0];                 y = arr[1];                 z = arr[2];                 return *this; }
    v3u& __vectorcall operator=(__m128i _mm ) { x = mm_extract_u32<0>(_mm); y = mm_extract_u32<1>(_mm); z = mm_extract_u32<2>(_mm); return *this; }
    v3u& __vectorcall operator=(const v3u& v) { x = v.x;                    y = v.y;                    z = v.z;                    return *this; }
    v3u& __vectorcall operator=(v3u&& v     ) { x = v.x;                    y = v.y;                    z = v.z;                    return *this; }

    u32 __vectorcall dot(v3u r) const
    {
        __m128i m = _mm_mul_epi32(_mm_setr_epi32(x, y, z, 0), _mm_setr_epi32(r.x, r.y, r.z, 0));
        __m128i h = _mm_hadd_epi32(m, m);
        __m128i d = _mm_hadd_epi32(h, h);
        return _mm_cvtsi128_si32(d);
    }

    v3u __vectorcall cross(v3u r) const
    {
        //  [x]   [r.x]   [ (y * r.z - z * r.y)]   [y * r.z - z * r.y]
        //  [y] x [r.y] = [-(x * r.z - z * r.x)] = [z * r.x - x * r.z];
        //  [z]   [r.z]   [ (x * r.y - y * r.x)]   [x * r.y - y * r.x]
        
        __m128i l_mm = _mm_setr_epi32(  x,   y,   z, 0);
        __m128i r_mm = _mm_setr_epi32(r.x, r.y, r.z, 0);

        return v3u(_mm_sub_epi32(_mm_mul_epi32(_mm_shuffle_epi32(l_mm, cast<s32>(MM_SHUFFLE::YZXW)),
                                               _mm_shuffle_epi32(r_mm, cast<s32>(MM_SHUFFLE::ZXYW))),
                                 _mm_mul_epi32(_mm_shuffle_epi32(l_mm, cast<s32>(MM_SHUFFLE::ZXYW)),
                                               _mm_shuffle_epi32(r_mm, cast<s32>(MM_SHUFFLE::YZXW)))));
    }

    f32 __vectorcall length() const
    {
        __m128 m = _mm_cvtepi32_ps(_mm_setr_epi32(x, y, z, 0));
        return _mm_cvtss_f32(_mm_sqrt_ss(_mm_dp_ps(m, m, 0x71)));
    }

    u32 __vectorcall length_sq() const
    {
        __m128i mm = _mm_setr_epi32(x, y, z, 0);
        __m128i m = _mm_mul_epi32(mm, mm);
        __m128i h = _mm_hadd_epi32(m, m);
        __m128i d = _mm_hadd_epi32(h, h);
        return _mm_cvtsi128_si32(d);
    }

    static v3u __vectorcall clamp(v3u val, v3u min, v3u max)
    {
        __m128i val_mm = _mm_setr_epi32(val.x, val.y, val.z, 0);
        __m128i min_mm = _mm_setr_epi32(min.x, min.y, min.z, 0);
        __m128i max_mm = _mm_setr_epi32(max.x, max.y, max.z, 0);
        return v3u(_mm_max_epi32(min_mm, _mm_min_epi32(max_mm, val_mm)));
    }

    v3u& __vectorcall operator+=(v3u r) { __m128i mm = _mm_add_epi32(_mm_setr_epi32(x, y, z, 0), _mm_setr_epi32(r.x, r.y, r.z, 0)); x = mm_extract_u32<0>(mm); y = mm_extract_u32<1>(mm); z = mm_extract_u32<2>(mm); return *this; }
    v3u& __vectorcall operator-=(v3u r) { __m128i mm = _mm_sub_epi32(_mm_setr_epi32(x, y, z, 0), _mm_setr_epi32(r.x, r.y, r.z, 0)); x = mm_extract_u32<0>(mm); y = mm_extract_u32<1>(mm); z = mm_extract_u32<2>(mm); return *this; }
    v3u& __vectorcall operator*=(v3u r) { __m128i mm = _mm_mul_epi32(_mm_setr_epi32(x, y, z, 0), _mm_setr_epi32(r.x, r.y, r.z, 0)); x = mm_extract_u32<0>(mm); y = mm_extract_u32<1>(mm); z = mm_extract_u32<2>(mm); return *this; }
    v3u& __vectorcall operator/=(v3u r) { __m128i mm = _mm_div_epi32(_mm_setr_epi32(x, y, z, 0), _mm_setr_epi32(r.x, r.y, r.z, 0)); x = mm_extract_u32<0>(mm); y = mm_extract_u32<1>(mm); z = mm_extract_u32<2>(mm); return *this; }

    v3u& __vectorcall operator+=(u32 r) { __m128i mm = _mm_add_epi32(_mm_setr_epi32(x, y, z, 0), _mm_setr_epi32(r, r, r, 0)); x = mm_extract_u32<0>(mm); y = mm_extract_u32<1>(mm); z = mm_extract_u32<2>(mm); return *this; }
    v3u& __vectorcall operator-=(u32 r) { __m128i mm = _mm_sub_epi32(_mm_setr_epi32(x, y, z, 0), _mm_setr_epi32(r, r, r, 0)); x = mm_extract_u32<0>(mm); y = mm_extract_u32<1>(mm); z = mm_extract_u32<2>(mm); return *this; }
    v3u& __vectorcall operator*=(u32 r) { __m128i mm = _mm_mul_epi32(_mm_setr_epi32(x, y, z, 0), _mm_setr_epi32(r, r, r, 0)); x = mm_extract_u32<0>(mm); y = mm_extract_u32<1>(mm); z = mm_extract_u32<2>(mm); return *this; }
    v3u& __vectorcall operator/=(u32 r) { __m128i mm = _mm_div_epi32(_mm_setr_epi32(x, y, z, 0), _mm_setr_epi32(r, r, r, 0)); x = mm_extract_u32<0>(mm); y = mm_extract_u32<1>(mm); z = mm_extract_u32<2>(mm); return *this; }

    u32  __vectorcall operator[](u8 i) const { Check(i < 3); return e[i]; }
    u32& __vectorcall operator[](u8 i)       { Check(i < 3); return e[i]; }
};

INLINE v3u __vectorcall operator+(v3u l, v3u r) { return v3u(_mm_add_epi32(_mm_setr_epi32(l.x, l.y, l.z, 0), _mm_setr_epi32(r.x, r.y, r.z, 0))); }
INLINE v3u __vectorcall operator-(v3u l, v3u r) { return v3u(_mm_sub_epi32(_mm_setr_epi32(l.x, l.y, l.z, 0), _mm_setr_epi32(r.x, r.y, r.z, 0))); }
INLINE v3u __vectorcall operator*(v3u l, v3u r) { return v3u(_mm_mul_epi32(_mm_setr_epi32(l.x, l.y, l.z, 0), _mm_setr_epi32(r.x, r.y, r.z, 0))); }
INLINE v3u __vectorcall operator/(v3u l, v3u r) { return v3u(_mm_div_epi32(_mm_setr_epi32(l.x, l.y, l.z, 0), _mm_setr_epi32(r.x, r.y, r.z, 0))); }

INLINE v3u __vectorcall operator+(v3u l, u32 r) { return v3u(_mm_add_epi32(_mm_setr_epi32(l.x, l.y, l.z, 0), _mm_setr_epi32(r, r, r, 0))); }
INLINE v3u __vectorcall operator-(v3u l, u32 r) { return v3u(_mm_sub_epi32(_mm_setr_epi32(l.x, l.y, l.z, 0), _mm_setr_epi32(r, r, r, 0))); }
INLINE v3u __vectorcall operator*(v3u l, u32 r) { return v3u(_mm_mul_epi32(_mm_setr_epi32(l.x, l.y, l.z, 0), _mm_setr_epi32(r, r, r, 0))); }
INLINE v3u __vectorcall operator/(v3u l, u32 r) { return v3u(_mm_div_epi32(_mm_setr_epi32(l.x, l.y, l.z, 0), _mm_setr_epi32(r, r, r, 0))); }

INLINE v3u __vectorcall operator+(u32 l, v3u r) { return v3u(_mm_add_epi32(_mm_setr_epi32(l, l, l, 0), _mm_setr_epi32(r.x, r.y, r.z, 0))); }
INLINE v3u __vectorcall operator-(u32 l, v3u r) { return v3u(_mm_sub_epi32(_mm_setr_epi32(l, l, l, 0), _mm_setr_epi32(r.x, r.y, r.z, 0))); }
INLINE v3u __vectorcall operator*(u32 l, v3u r) { return v3u(_mm_mul_epi32(_mm_setr_epi32(l, l, l, 0), _mm_setr_epi32(r.x, r.y, r.z, 0))); }
INLINE v3u __vectorcall operator/(u32 l, v3u r) { return v3u(_mm_div_epi32(_mm_setr_epi32(l, l, l, 0), _mm_setr_epi32(r.x, r.y, r.z, 0))); }

INLINE bool __vectorcall operator==(v3u l, v3u r)
{
    __m128i l_mm = _mm_setr_epi32(l.x, l.y, l.z, 0);
    __m128i r_mm = _mm_setr_epi32(r.x, r.y, r.z, 0);
#if ENGINE_ISA >= ENGINE_ISA_AVX512
    return 0b11 == _mm_cmpeq_epi64_mask(l_mm, r_mm);
#else
    __m128i eq_mask = _mm_cmpeq_epi64(l_mm, r_mm);
    __m128i ones    = _mm_cmpeq_epi64(eq_mask, eq_mask);
    return _mm_testc_si128(eq_mask, ones);
#endif
}

INLINE bool __vectorcall operator!=(v3u l, v3u r)
{
    __m128i l_mm = _mm_setr_epi32(l.x, l.y, l.z, 0);
    __m128i r_mm = _mm_setr_epi32(r.x, r.y, r.z, 0);
#if ENGINE_ISA >= ENGINE_ISA_AVX512
    return 0b11 != _mm_cmpeq_epi64_mask(l_mm, r_mm);
#else
    __m128i eq_mask = _mm_cmpeq_epi64(l_mm, r_mm);
    __m128i ones    = _mm_cmpeq_epi64(eq_mask, eq_mask);
    return !_mm_testc_si128(eq_mask, ones);
#endif
}

// @NOTE(Roman):
// sqrt(x1^2 + y1^2 + z1^2)   <=> sqrt(x2^2 + y2^2 + z2^2)
// sqrt(x1^2 + y1^2 + z1^2)^2 <=> sqrt(x2^2 + y2^2 + z2^2)^2
//      x1^2 + y1^2 + z1^2    <=>      x2^2 + y2^2 + z2^2
INLINE bool __vectorcall operator<=(v3u l, v3u r) { return l.length_sq() <= r.length_sq(); }
INLINE bool __vectorcall operator>=(v3u l, v3u r) { return l.length_sq() >= r.length_sq(); }
INLINE bool __vectorcall operator< (v3u l, v3u r) { return l.length_sq() <  r.length_sq(); }
INLINE bool __vectorcall operator> (v3u l, v3u r) { return l.length_sq() >  r.length_sq(); }


INLINE v3s __vectorcall v3_to_v3s (v3  v) { return v3s(_mm_cvtps_epi32(_mm_setr_ps   (v.x, v.y, v.z, 0.0f))); }
INLINE v3u __vectorcall v3_to_v3u (v3  v) { return v3u(_mm_cvtps_epi32(_mm_setr_ps   (v.x, v.y, v.z, 0.0f))); }
INLINE v3  __vectorcall v3s_to_v3 (v3s v) { return v3 (_mm_cvtepi32_ps(_mm_setr_epi32(v.x, v.y, v.z, 0   ))); }
INLINE v3u __vectorcall v3s_to_v3u(v3s v) { return v3u(                _mm_setr_epi32(v.x, v.y, v.z, 0   ) ); }
INLINE v3  __vectorcall v3u_to_v3 (v3u v) { return v3 (_mm_cvtepi32_ps(_mm_setr_epi32(v.x, v.y, v.z, 0   ))); }
INLINE v3s __vectorcall v3u_to_v3s(v3u v) { return v3s(                _mm_setr_epi32(v.x, v.y, v.z, 0   ) ); }

//
// v4
//

union ENGINE_INTRIN_TYPE ENGINE_ALIGN(16) v4 final
{
    struct { f32 x,  y,  z, w; };
    struct { f32 r,  g,  b, a; };
    struct { v2  xy, wh; };
    v3 xyz;
    v3 rgb;
    __m128 mm;

    v4(                          ) : mm(_mm_setzero_ps())                    {}
    v4(f32 val                   ) : mm(_mm_set_ps1(val))                    {}
    v4(f32 x, f32 y, f32 z, f32 w) : mm(_mm_setr_ps(x, y, z, w))             {}
    v4(f32 arr[4]                ) : mm(_mm_load_ps(arr))                    {}
    v4(v2 xy, f32 z, f32 w       ) : mm(_mm_setr_ps(xy.x, xy.y, z, w))       {}
    v4(v2 xy, v2 wh              ) : mm(_mm_setr_ps(xy.x, xy.y, wh.w, wh.h)) {}
    v4(v3 xyz, f32 w             ) : mm(_mm_setr_ps(xyz.x, xyz.y, xyz.z, w)) {}
    v4(__m128 _mm                ) : mm(_mm)                                 {}
    v4(const v4& v               ) : mm(v.mm)                                {}
    v4(v4&& v                    ) : mm(v.mm)                                {}

    v4& __vectorcall operator=(f32 val    ) { mm = _mm_set_ps1(val); return *this; }
    v4& __vectorcall operator=(f32 arr[4] ) { mm = _mm_load_ps(arr); return *this; }
    v4& __vectorcall operator=(__m128 _mm ) { mm = _mm;              return *this; }
    v4& __vectorcall operator=(const v4& v) { mm = v.mm;             return *this; }
    v4& __vectorcall operator=(v4&& v     ) { mm = v.mm;             return *this; }

    f32 __vectorcall dot(v4 r) const
    {
        return _mm_cvtss_f32(_mm_dp_ps(mm, r.mm, 0x71));
    }

    v4 __vectorcall cross(v4 r) const
    {
        //  [x]   [r.x]   [ (y * r.z - z * r.y)]   [y * r.z - z * r.y]
        //  [y] x [r.y] = [-(x * r.z - z * r.x)] = [z * r.x - x * r.z];
        //  [z]   [r.z]   [ (x * r.y - y * r.x)]   [x * r.y - y * r.x]
        //  [w]   [r.w]   [ (w * r.w - w * r.w)]   [        0        ]

        return v4(_mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(  mm,   mm, cast<s32>(MM_SHUFFLE::YZXW)),
                                        _mm_shuffle_ps(r.mm, r.mm, cast<s32>(MM_SHUFFLE::ZXYW))),
                             _mm_mul_ps(_mm_shuffle_ps(  mm,   mm, cast<s32>(MM_SHUFFLE::ZXYW)),
                                        _mm_shuffle_ps(r.mm, r.mm, cast<s32>(MM_SHUFFLE::YZXW)))));
    }

    f32 __vectorcall length() const
    {
        return _mm_cvtss_f32(_mm_sqrt_ss(_mm_dp_ps(mm, mm, 0x71)));
    }

    f32 __vectorcall length_sq() const
    {
        return _mm_cvtss_f32(_mm_dp_ps(mm, mm, 0x71));
    }

    v4 __vectorcall normalize() const
    {
        return v4(mm_insert_f32<3>(_mm_div_ps(mm, _mm_sqrt_ps(_mm_dp_ps(mm, mm, 0x7F))), w)); // save original w value
    }

    v4 __vectorcall normalize_w() const
    {
        if (w && w != 1.0f)
        {
            return v4(_mm_div_ps(mm, _mm_set_ps1(w)));
        }
        return *this;
    }

    static v4 __vectorcall lerp(v4 start, v4 end, v4 percent)
    {
        __m128 per = _mm_max_ps(_mm_setzero_ps(), _mm_min_ps(_mm_set_ps1(1.0f), percent.mm));
        return v4(_mm_add_ps(_mm_mul_ps(_mm_sub_ps(end.mm, start.mm), per), start.mm));
    }

    static v4 __vectorcall clamp(v4 val, v4 min, v4 max)
    {
        return v4(_mm_max_ps(min.mm, _mm_min_ps(max.mm, val.mm)));
    }

    v4& __vectorcall operator+=(v4 r) { mm = _mm_add_ps(mm, r.mm); return *this; }
    v4& __vectorcall operator-=(v4 r) { mm = _mm_sub_ps(mm, r.mm); return *this; }
    v4& __vectorcall operator*=(v4 r) { mm = _mm_mul_ps(mm, r.mm); return *this; }
    v4& __vectorcall operator/=(v4 r) { mm = _mm_div_ps(mm, r.mm); return *this; }

    v4& __vectorcall operator+=(f32 r) { mm = _mm_add_ps(mm, _mm_set_ps1(r)); return *this; }
    v4& __vectorcall operator-=(f32 r) { mm = _mm_sub_ps(mm, _mm_set_ps1(r)); return *this; }
    v4& __vectorcall operator*=(f32 r) { mm = _mm_mul_ps(mm, _mm_set_ps1(r)); return *this; }
    v4& __vectorcall operator/=(f32 r) { mm = _mm_div_ps(mm, _mm_set_ps1(r)); return *this; }

    f32  __vectorcall operator[](u8 i) const { Check(i < 4); return mm.m128_f32[i]; }
    f32& __vectorcall operator[](u8 i)       { Check(i < 4); return mm.m128_f32[i]; }
};

INLINE v4 __vectorcall operator+(v4 l, v4 r) { return v4(_mm_add_ps(l.mm, r.mm)); }
INLINE v4 __vectorcall operator-(v4 l, v4 r) { return v4(_mm_sub_ps(l.mm, r.mm)); }
INLINE v4 __vectorcall operator*(v4 l, v4 r) { return v4(_mm_mul_ps(l.mm, r.mm)); }
INLINE v4 __vectorcall operator/(v4 l, v4 r) { return v4(_mm_div_ps(l.mm, r.mm)); }

INLINE v4 __vectorcall operator+(v4 l, f32 r) { return v4(_mm_add_ps(l.mm, _mm_set_ps1(r))); }
INLINE v4 __vectorcall operator-(v4 l, f32 r) { return v4(_mm_sub_ps(l.mm, _mm_set_ps1(r))); }
INLINE v4 __vectorcall operator*(v4 l, f32 r) { return v4(_mm_mul_ps(l.mm, _mm_set_ps1(r))); }
INLINE v4 __vectorcall operator/(v4 l, f32 r) { return v4(_mm_div_ps(l.mm, _mm_set_ps1(r))); }

INLINE v4 __vectorcall operator+(f32 l, v4 r) { return v4(_mm_add_ps(_mm_set_ps1(l), r.mm)); }
INLINE v4 __vectorcall operator-(f32 l, v4 r) { return v4(_mm_sub_ps(_mm_set_ps1(l), r.mm)); }
INLINE v4 __vectorcall operator*(f32 l, v4 r) { return v4(_mm_mul_ps(_mm_set_ps1(l), r.mm)); }
INLINE v4 __vectorcall operator/(f32 l, v4 r) { return v4(_mm_div_ps(_mm_set_ps1(l), r.mm)); }

INLINE bool __vectorcall operator==(v4 l, v4 r)
{
#if ENGINE_ISA >= ENGINE_ISA_AVX512
    return 0b11 == _mm_cmpeq_epi64_mask(*cast<__m128i *>(&l.mm),
                                        *cast<__m128i *>(&r.mm));
#else
    __m128 eq_mask = _mm_cmpeq_ps(l.mm, r.mm);
    __m128 ones    = _mm_cmpeq_ps(eq_mask, eq_mask);
    return _mm_testc_ps(eq_mask, ones);
#endif
}

INLINE bool __vectorcall operator!=(v4 l, v4 r)
{
#if ENGINE_ISA >= ENGINE_ISA_AVX512
    return 0b11 != _mm_cmpeq_epi64_mask(*cast<__m128i *>(&l.mm),
                                        *cast<__m128i *>(&r.mm));
#else
    __m128 eq_mask = _mm_cmpeq_ps(l.mm, r.mm);
    __m128 ones    = _mm_cmpeq_ps(eq_mask, eq_mask);
    return !_mm_testc_ps(eq_mask, ones);
#endif
}

// @NOTE(Roman):
// sqrt(x1^2 + y1^2 + z1^2)   <=> sqrt(x2^2 + y2^2 + z2^2)
// sqrt(x1^2 + y1^2 + z1^2)^2 <=> sqrt(x2^2 + y2^2 + z2^2)^2
//      x1^2 + y1^2 + z1^2    <=>      x2^2 + y2^2 + z2^2
INLINE bool __vectorcall operator<=(v4 l, v4 r) { return l.length_sq() <= r.length_sq(); }
INLINE bool __vectorcall operator>=(v4 l, v4 r) { return l.length_sq() >= r.length_sq(); }
INLINE bool __vectorcall operator< (v4 l, v4 r) { return l.length_sq() <  r.length_sq(); }
INLINE bool __vectorcall operator> (v4 l, v4 r) { return l.length_sq() >  r.length_sq(); }

union ENGINE_INTRIN_TYPE ENGINE_ALIGN(16) v4s final
{
    struct { s32 x,  y,  z, w; };
    struct { s32 r,  g,  b, a; };
    struct { v2s xy, wh; };
    v3s xyz;
    v3s rgb;
    __m128i mm;

    v4s(                          ) : mm(_mm_setzero_si128())                    {}
    v4s(s32 val                   ) : mm(_mm_set1_epi32(val))                    {}
    v4s(s32 x, s32 y, s32 z, s32 w) : mm(_mm_setr_epi32(x, y, z, w))             {}
    v4s(s32 arr[4]                ) : mm(_mm_load_si128(cast<__m128i *>(arr)))   {}
    v4s(v2s xy, s32 z, s32 w      ) : mm(_mm_setr_epi32(xy.x, xy.y, z, w))       {}
    v4s(v2s xy, v2s wh            ) : mm(_mm_setr_epi32(xy.x, xy.y, wh.w, wh.h)) {}
    v4s(v3s xyz, s32 w            ) : mm(_mm_setr_epi32(xyz.x, xyz.y, xyz.z, w)) {}
    v4s(__m128i _mm               ) : mm(_mm)                                    {}
    v4s(const v4s& v              ) : mm(v.mm)                                   {}
    v4s(v4s&& v                   ) : mm(v.mm)                                   {}

    v4s& __vectorcall operator=(s32 val     ) { mm = _mm_set1_epi32(val);                  return *this; }
    v4s& __vectorcall operator=(s32 arr[4]  ) { mm = _mm_load_si128(cast<__m128i *>(arr)); return *this; }
    v4s& __vectorcall operator=(__m128i _mm ) { mm = _mm;                                  return *this; }
    v4s& __vectorcall operator=(const v4s& v) { mm = v.mm;                                 return *this; }
    v4s& __vectorcall operator=(v4s&& v     ) { mm = v.mm;                                 return *this; }

    s32 __vectorcall dot(v4s r) const
    {
        __m128i m = _mm_mul_epi32(mm, r.mm);
        __m128i h = _mm_hadd_epi32(m, m);
        __m128i d = _mm_hadd_epi32(h, h);
        return _mm_cvtsi128_si32(d);
    }

    v4s __vectorcall cross(v4s r) const
    {
        //  [x]   [r.x]   [ (y * r.z - z * r.y)]   [y * r.z - z * r.y]
        //  [y] x [r.y] = [-(x * r.z - z * r.x)] = [z * r.x - x * r.z];
        //  [z]   [r.z]   [ (x * r.y - y * r.x)]   [x * r.y - y * r.x]
        //  [w]   [r.w]   [ (w * r.w - w * r.w)]   [        0        ]

        return v4s(_mm_sub_epi32(_mm_mul_epi32(_mm_shuffle_epi32(  mm, cast<s32>(MM_SHUFFLE::YZXW)),
                                               _mm_shuffle_epi32(r.mm, cast<s32>(MM_SHUFFLE::ZXYW))),
                                 _mm_mul_epi32(_mm_shuffle_epi32(  mm, cast<s32>(MM_SHUFFLE::ZXYW)),
                                               _mm_shuffle_epi32(r.mm, cast<s32>(MM_SHUFFLE::YZXW)))));
    }

    f32 __vectorcall length() const
    {
        __m128 m = _mm_cvtepi32_ps(mm);
        return _mm_cvtss_f32(_mm_sqrt_ss(_mm_dp_ps(m, m, 0x71)));
    }

    s32 __vectorcall length_sq() const
    {
        __m128i m = _mm_mul_epi32(mm, mm);
        __m128i h = _mm_hadd_epi32(m, m);
        __m128i d = _mm_hadd_epi32(h, h);
        return _mm_cvtsi128_si32(d);
    }

    static v4s __vectorcall clamp(v4s val, v4s min, v4s max)
    {
        return v4s(_mm_max_epi32(min.mm, _mm_min_epi32(max.mm, val.mm)));
    }

    v4s& __vectorcall operator+=(v4s r) { mm = _mm_add_epi32(mm, r.mm); return *this; }
    v4s& __vectorcall operator-=(v4s r) { mm = _mm_sub_epi32(mm, r.mm); return *this; }
    v4s& __vectorcall operator*=(v4s r) { mm = _mm_mul_epi32(mm, r.mm); return *this; }
    v4s& __vectorcall operator/=(v4s r) { mm = _mm_div_epi32(mm, r.mm); return *this; }

    v4s& __vectorcall operator+=(s32 r) { mm = _mm_add_epi32(mm, _mm_set1_epi32(r)); return *this; }
    v4s& __vectorcall operator-=(s32 r) { mm = _mm_sub_epi32(mm, _mm_set1_epi32(r)); return *this; }
    v4s& __vectorcall operator*=(s32 r) { mm = _mm_mul_epi32(mm, _mm_set1_epi32(r)); return *this; }
    v4s& __vectorcall operator/=(s32 r) { mm = _mm_div_epi32(mm, _mm_set1_epi32(r)); return *this; }

    s32  __vectorcall operator[](u8 i) const { Check(i < 4); return            mm.m128i_i32[i];  }
    s32& __vectorcall operator[](u8 i)       { Check(i < 4); return cast<s32&>(mm.m128i_i32[i]); }
};

INLINE v4s __vectorcall operator+(v4s l, v4s r) { return v4s(_mm_add_epi32(l.mm, r.mm)); }
INLINE v4s __vectorcall operator-(v4s l, v4s r) { return v4s(_mm_sub_epi32(l.mm, r.mm)); }
INLINE v4s __vectorcall operator*(v4s l, v4s r) { return v4s(_mm_mul_epi32(l.mm, r.mm)); }
INLINE v4s __vectorcall operator/(v4s l, v4s r) { return v4s(_mm_div_epi32(l.mm, r.mm)); }

INLINE v4s __vectorcall operator+(v4s l, s32 r) { return v4s(_mm_add_epi32(l.mm, _mm_set1_epi32(r))); }
INLINE v4s __vectorcall operator-(v4s l, s32 r) { return v4s(_mm_sub_epi32(l.mm, _mm_set1_epi32(r))); }
INLINE v4s __vectorcall operator*(v4s l, s32 r) { return v4s(_mm_mul_epi32(l.mm, _mm_set1_epi32(r))); }
INLINE v4s __vectorcall operator/(v4s l, s32 r) { return v4s(_mm_div_epi32(l.mm, _mm_set1_epi32(r))); }

INLINE v4s __vectorcall operator+(s32 l, v4s r) { return v4s(_mm_add_epi32(_mm_set1_epi32(l), r.mm)); }
INLINE v4s __vectorcall operator-(s32 l, v4s r) { return v4s(_mm_sub_epi32(_mm_set1_epi32(l), r.mm)); }
INLINE v4s __vectorcall operator*(s32 l, v4s r) { return v4s(_mm_mul_epi32(_mm_set1_epi32(l), r.mm)); }
INLINE v4s __vectorcall operator/(s32 l, v4s r) { return v4s(_mm_div_epi32(_mm_set1_epi32(l), r.mm)); }

INLINE bool __vectorcall operator==(v4s l, v4s r)
{
#if ENGINE_ISA >= ENGINE_ISA_AVX512
    return 0b11 == _mm_cmpeq_epi64_mask(l.mm, r.mm);
#else
    __m128i eq_mask = _mm_cmpeq_epi64(l.mm, r.mm);
    __m128i ones    = _mm_cmpeq_epi64(eq_mask, eq_mask);
    return _mm_testc_si128(eq_mask, ones);
#endif
}

INLINE bool __vectorcall operator!=(v4s l, v4s r)
{
#if ENGINE_ISA >= ENGINE_ISA_AVX512
    return 0b11 != _mm_cmpeq_epi64_mask(l.mm, r.mm);
#else
    __m128i eq_mask = _mm_cmpeq_epi64(l.mm, r.mm);
    __m128i ones    = _mm_cmpeq_epi64(eq_mask, eq_mask);
    return !_mm_testc_si128(eq_mask, ones);
#endif
}

// @NOTE(Roman):
// sqrt(x1^2 + y1^2 + z1^2)   <=> sqrt(x2^2 + y2^2 + z2^2)
// sqrt(x1^2 + y1^2 + z1^2)^2 <=> sqrt(x2^2 + y2^2 + z2^2)^2
//      x1^2 + y1^2 + z1^2    <=>      x2^2 + y2^2 + z2^2
INLINE bool __vectorcall operator<=(v4s l, v4s r) { return l.length_sq() <= r.length_sq(); }
INLINE bool __vectorcall operator>=(v4s l, v4s r) { return l.length_sq() >= r.length_sq(); }
INLINE bool __vectorcall operator< (v4s l, v4s r) { return l.length_sq() <  r.length_sq(); }
INLINE bool __vectorcall operator> (v4s l, v4s r) { return l.length_sq() >  r.length_sq(); }

union ENGINE_INTRIN_TYPE ENGINE_ALIGN(16) v4u final
{
    struct { u32 x,  y,  z, w; };
    struct { u32 r,  g,  b, a; };
    struct { v2u xy, wh; };
    v3u xyz;
    v3u rgb;
    __m128i mm;

    v4u(                          ) : mm(_mm_setzero_si128())                    {}
    v4u(u32 val                   ) : mm(_mm_set1_epi32(val))                    {}
    v4u(u32 x, u32 y, u32 z, u32 w) : mm(_mm_setr_epi32(x, y, z, w))             {}
    v4u(u32 arr[4]                ) : mm(_mm_load_si128(cast<__m128i *>(arr)))   {}
    v4u(v2u xy, u32 z, u32 w      ) : mm(_mm_setr_epi32(xy.x, xy.y, z, w))       {}
    v4u(v2u xy, v2u wh            ) : mm(_mm_setr_epi32(xy.x, xy.y, wh.w, wh.h)) {}
    v4u(v3u xyz, u32 w            ) : mm(_mm_setr_epi32(xyz.x, xyz.y, xyz.z, w)) {}
    v4u(__m128i _mm               ) : mm(_mm)                                    {}
    v4u(const v4u& v              ) : mm(v.mm)                                   {}
    v4u(v4u&& v                   ) : mm(v.mm)                                   {}

    v4u& __vectorcall operator=(u32 val     ) { mm = _mm_set1_epi32(val);                  return *this; }
    v4u& __vectorcall operator=(u32 arr[4]  ) { mm = _mm_load_si128(cast<__m128i *>(arr)); return *this; }
    v4u& __vectorcall operator=(__m128i _mm ) { mm = _mm;                                  return *this; }
    v4u& __vectorcall operator=(const v4u& v) { mm = v.mm;                                 return *this; }
    v4u& __vectorcall operator=(v4u&& v     ) { mm = v.mm;                                 return *this; }

    u32 __vectorcall dot(v4u r) const
    {
        __m128i m = _mm_mul_epi32(mm, r.mm);
        __m128i h = _mm_hadd_epi32(m, m);
        __m128i d = _mm_hadd_epi32(h, h);
        return _mm_cvtsi128_si32(d);
    }

    v4u __vectorcall cross(v4u r) const
    {
        //  [x]   [r.x]   [ (y * r.z - z * r.y)]   [y * r.z - z * r.y]
        //  [y] x [r.y] = [-(x * r.z - z * r.x)] = [z * r.x - x * r.z];
        //  [z]   [r.z]   [ (x * r.y - y * r.x)]   [x * r.y - y * r.x]
        //  [w]   [r.w]   [ (w * r.w - w * r.w)]   [        0        ]

        return v4u(_mm_sub_epi32(_mm_mul_epi32(_mm_shuffle_epi32(  mm, cast<s32>(MM_SHUFFLE::YZXW)),
                                               _mm_shuffle_epi32(r.mm, cast<s32>(MM_SHUFFLE::ZXYW))),
                                 _mm_mul_epi32(_mm_shuffle_epi32(  mm, cast<s32>(MM_SHUFFLE::ZXYW)),
                                               _mm_shuffle_epi32(r.mm, cast<s32>(MM_SHUFFLE::YZXW)))));
    }

    f32 __vectorcall length() const
    {
        __m128 m = _mm_cvtepi32_ps(mm);
        return _mm_cvtss_f32(_mm_sqrt_ss(_mm_dp_ps(m, m, 0x71)));
    }

    u32 __vectorcall length_sq() const
    {
        __m128i m = _mm_mul_epi32(mm, mm);
        __m128i h = _mm_hadd_epi32(m, m);
        __m128i d = _mm_hadd_epi32(h, h);
        return _mm_cvtsi128_si32(d);
    }

    static v4u __vectorcall clamp(v4u val, v4u min, v4u max)
    {
        return v4u(_mm_max_epi32(min.mm, _mm_min_epi32(max.mm, val.mm)));
    }

    v4u& __vectorcall operator+=(v4u r) { mm = _mm_add_epi32(mm, r.mm); return *this; }
    v4u& __vectorcall operator-=(v4u r) { mm = _mm_sub_epi32(mm, r.mm); return *this; }
    v4u& __vectorcall operator*=(v4u r) { mm = _mm_mul_epi32(mm, r.mm); return *this; }
    v4u& __vectorcall operator/=(v4u r) { mm = _mm_div_epi32(mm, r.mm); return *this; }

    v4u& __vectorcall operator+=(u32 r) { mm = _mm_add_epi32(mm, _mm_set1_epi32(r)); return *this; }
    v4u& __vectorcall operator-=(u32 r) { mm = _mm_sub_epi32(mm, _mm_set1_epi32(r)); return *this; }
    v4u& __vectorcall operator*=(u32 r) { mm = _mm_mul_epi32(mm, _mm_set1_epi32(r)); return *this; }
    v4u& __vectorcall operator/=(u32 r) { mm = _mm_div_epi32(mm, _mm_set1_epi32(r)); return *this; }

    u32  __vectorcall operator[](u8 i) const { Check(i < 4); return cast<u32 >(mm.m128i_i32[i]); }
    u32& __vectorcall operator[](u8 i)       { Check(i < 4); return cast<u32&>(mm.m128i_i32[i]); }
};

INLINE v4u __vectorcall operator+(v4u l, v4u r) { return v4u(_mm_add_epi32(l.mm, r.mm)); }
INLINE v4u __vectorcall operator-(v4u l, v4u r) { return v4u(_mm_sub_epi32(l.mm, r.mm)); }
INLINE v4u __vectorcall operator*(v4u l, v4u r) { return v4u(_mm_mul_epi32(l.mm, r.mm)); }
INLINE v4u __vectorcall operator/(v4u l, v4u r) { return v4u(_mm_div_epi32(l.mm, r.mm)); }

INLINE v4u __vectorcall operator+(v4u l, u32 r) { return v4u(_mm_add_epi32(l.mm, _mm_set1_epi32(r))); }
INLINE v4u __vectorcall operator-(v4u l, u32 r) { return v4u(_mm_sub_epi32(l.mm, _mm_set1_epi32(r))); }
INLINE v4u __vectorcall operator*(v4u l, u32 r) { return v4u(_mm_mul_epi32(l.mm, _mm_set1_epi32(r))); }
INLINE v4u __vectorcall operator/(v4u l, u32 r) { return v4u(_mm_div_epi32(l.mm, _mm_set1_epi32(r))); }

INLINE v4u __vectorcall operator+(u32 l, v4u r) { return v4u(_mm_add_epi32(_mm_set1_epi32(l), r.mm)); }
INLINE v4u __vectorcall operator-(u32 l, v4u r) { return v4u(_mm_sub_epi32(_mm_set1_epi32(l), r.mm)); }
INLINE v4u __vectorcall operator*(u32 l, v4u r) { return v4u(_mm_mul_epi32(_mm_set1_epi32(l), r.mm)); }
INLINE v4u __vectorcall operator/(u32 l, v4u r) { return v4u(_mm_div_epi32(_mm_set1_epi32(l), r.mm)); }

INLINE bool __vectorcall operator==(v4u l, v4u r)
{
#if ENGINE_ISA >= ENGINE_ISA_AVX512
    return 0b11 == _mm_cmpeq_epi64_mask(l.mm, r.mm);
#else
    __m128i eq_mask = _mm_cmpeq_epi64(l.mm, r.mm);
    __m128i ones    = _mm_cmpeq_epi64(eq_mask, eq_mask);
    return _mm_testc_si128(eq_mask, ones);
#endif
}

INLINE bool __vectorcall operator!=(v4u l, v4u r)
{
#if ENGINE_ISA >= ENGINE_ISA_AVX512
    return 0b11 != _mm_cmpeq_epi64_mask(l.mm, r.mm);
#else
    __m128i eq_mask = _mm_cmpeq_epi64(l.mm, r.mm);
    __m128i ones    = _mm_cmpeq_epi64(eq_mask, eq_mask);
    return !_mm_testc_si128(eq_mask, ones);
#endif
}

// @NOTE(Roman):
// sqrt(x1^2 + y1^2 + z1^2)   <=> sqrt(x2^2 + y2^2 + z2^2)
// sqrt(x1^2 + y1^2 + z1^2)^2 <=> sqrt(x2^2 + y2^2 + z2^2)^2
//      x1^2 + y1^2 + z1^2    <=>      x2^2 + y2^2 + z2^2
INLINE bool __vectorcall operator<=(v4u l, v4u r) { return l.length_sq() <= r.length_sq(); }
INLINE bool __vectorcall operator>=(v4u l, v4u r) { return l.length_sq() >= r.length_sq(); }
INLINE bool __vectorcall operator< (v4u l, v4u r) { return l.length_sq() <  r.length_sq(); }
INLINE bool __vectorcall operator> (v4u l, v4u r) { return l.length_sq() >  r.length_sq(); }


INLINE v4s __vectorcall v4_to_v4s (v4  v) { return v4s(_mm_cvtps_epi32(v.mm)); }
INLINE v4u __vectorcall v4_to_v4u (v4  v) { return v4u(_mm_cvtps_epi32(v.mm)); }
INLINE v4  __vectorcall v4s_to_v4 (v4s v) { return v4 (_mm_cvtepi32_ps(v.mm)); }
INLINE v4u __vectorcall v4s_to_v4u(v4s v) { return v4u(                v.mm ); }
INLINE v4  __vectorcall v4u_to_v4 (v4u v) { return v4 (_mm_cvtepi32_ps(v.mm)); }
INLINE v4s __vectorcall v4u_to_v4s(v4u v) { return v4s(                v.mm ); }

#pragma pack(pop)
