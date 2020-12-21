//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/common.h"

namespace REV::Math
{
    inline constexpr const f64 g_f64_E        = 2.71828182845904523536;  // e
    inline constexpr const f64 g_f64_LOG2E    = 1.44269504088896340736;  // log2(e)
    inline constexpr const f64 g_f64_LOG10E   = 0.434294481903251827651; // log10(e)
    inline constexpr const f64 g_f64_LN2      = 0.693147180559945309417; // ln(2)
    inline constexpr const f64 g_f64_LN10     = 2.30258509299404568402;  // ln(10)
    inline constexpr const f64 g_f64_PI       = 3.14159265358979323846;  // pi
    inline constexpr const f64 g_f64_PI_2     = 1.57079632679489661923;  // pi/2
    inline constexpr const f64 g_f64_PI_4     = 0.785398163397448309616; // pi/4
    inline constexpr const f64 g_f64_1_PI     = 0.318309886183790671538; // 1/pi
    inline constexpr const f64 g_f64_2_PI     = 0.636619772367581343076; // 2/pi
    inline constexpr const f64 g_f64_2_SQRTPI = 1.12837916709551257390;  // 2/sqrt(pi)
    inline constexpr const f64 g_f64_SQRT2    = 1.41421356237309504880;  // sqrt(2)
    inline constexpr const f64 g_f64_1_SQRT_2 = 0.707106781186547524401; // 1/sqrt(2)

    inline constexpr const f32 g_f32_E        = 2.71828182845904523536f;  // e
    inline constexpr const f32 g_f32_LOG2E    = 1.44269504088896340736f;  // log2(e)
    inline constexpr const f32 g_f32_LOG10E   = 0.434294481903251827651f; // log10(e)
    inline constexpr const f32 g_f32_LN2      = 0.693147180559945309417f; // ln(2)
    inline constexpr const f32 g_f32_LN10     = 2.30258509299404568402f;  // ln(10)
    inline constexpr const f32 g_f32_PI       = 3.14159265358979323846f;  // pi
    inline constexpr const f32 g_f32_PI_2     = 1.57079632679489661923f;  // pi/2
    inline constexpr const f32 g_f32_PI_4     = 0.785398163397448309616f; // pi/4
    inline constexpr const f32 g_f32_1_PI     = 0.318309886183790671538f; // 1/pi
    inline constexpr const f32 g_f32_2_PI     = 0.636619772367581343076f; // 2/pi
    inline constexpr const f32 g_f32_2_SQRTPI = 1.12837916709551257390f;  // 2/sqrt(pi)
    inline constexpr const f32 g_f32_SQRT2    = 1.41421356237309504880f;  // sqrt(2)
    inline constexpr const f32 g_f32_1_SQRT_2 = 0.707106781186547524401f; // 1/sqrt(2)

    union REV_INTRIN_TYPE reg32 final
    {
        f32 f;
        s32 s;
        u32 u;
        int i;

        REV_INLINE reg32(              ) : f(0.0f) {}
        REV_INLINE reg32(f32          v) : f(v)    {}
        REV_INLINE reg32(s32          v) : s(v)    {}
        REV_INLINE reg32(u32          v) : u(v)    {}
        REV_INLINE reg32(int          v) : i(v)    {}
        REV_INLINE reg32(const reg32& v) : f(v.f)  {}
        REV_INLINE reg32(reg32&&      v) : f(v.f)  {}

        REV_INLINE REV_VECTORCALL operator f32() const { return f; }
        REV_INLINE REV_VECTORCALL operator s32() const { return s; }
        REV_INLINE REV_VECTORCALL operator u32() const { return u; }
        REV_INLINE REV_VECTORCALL operator int() const { return i; }

        REV_INLINE reg32& REV_VECTORCALL operator=(f32          v) { f = v;   return *this; }
        REV_INLINE reg32& REV_VECTORCALL operator=(s32          v) { s = v;   return *this; }
        REV_INLINE reg32& REV_VECTORCALL operator=(u32          v) { u = v;   return *this; }
        REV_INLINE reg32& REV_VECTORCALL operator=(int          v) { i = v;   return *this; }
        REV_INLINE reg32& REV_VECTORCALL operator=(const reg32& v) { f = v.f; return *this; }
        REV_INLINE reg32& REV_VECTORCALL operator=(reg32&&      v) { f = v.f; return *this; }
    };

    union REV_INTRIN_TYPE reg64 final
    {
        f64 f;
        s64 s;
        u64 u;

        REV_INLINE reg64(              ) : f(0.0) {}
        REV_INLINE reg64(f64          v) : f(v)   {}
        REV_INLINE reg64(s64          v) : s(v)   {}
        REV_INLINE reg64(u64          v) : u(v)   {}
        REV_INLINE reg64(const reg64& v) : f(v.f) {}
        REV_INLINE reg64(reg64&&      v) : f(v.f) {}

        REV_INLINE REV_VECTORCALL operator f64() const { return f; }
        REV_INLINE REV_VECTORCALL operator s64() const { return s; }
        REV_INLINE REV_VECTORCALL operator u64() const { return u; }

        REV_INLINE reg64& REV_VECTORCALL operator=(f64          v) { f = v;   return *this; }
        REV_INLINE reg64& REV_VECTORCALL operator=(s64          v) { s = v;   return *this; }
        REV_INLINE reg64& REV_VECTORCALL operator=(u64          v) { u = v;   return *this; }
        REV_INLINE reg64& REV_VECTORCALL operator=(const reg64& v) { f = v.f; return *this; }
        REV_INLINE reg64& REV_VECTORCALL operator=(reg64&&      v) { f = v.f; return *this; }
    };

    template<typename T, typename = RTTI::enable_if_t<RTTI::is_floating_point_v<T>>>
    REV_INLINE T REV_VECTORCALL lerp(T start, T end, T percent)
    {
        return (end - start) * percent + start;
    }

    template<typename T, typename = RTTI::enable_if_t<RTTI::is_floating_point_v<T>>>
    REV_INLINE T REV_VECTORCALL invlerp(T start, T end, T value) // ret = [0, 1]
    {
        return (value - start) / (end - start);
    }

    template<typename T, typename = RTTI::enable_if_t<RTTI::is_floating_point_v<T>>>
    REV_INLINE T REV_VECTORCALL invlerp_n(T start, T end, T value) // ret = [-1, 1]
    {
        return ((value - start) / (end - start) - 0.5f) * 2.0f;
    }

    template<typename T, typename = RTTI::enable_if_t<RTTI::is_arithmetic_v<T>>>
    REV_INLINE T REV_VECTORCALL clamp(T val, T min_val, T max_val)
    {
        return RTTI::max(min_val, RTTI::min(max_val, val));
    }

    // value <= 20
    REV_API u64 fact(u8 value);

    REV_API void REV_VECTORCALL sincos(rad32 a, f32 *s, f32 *c);
    REV_API void REV_VECTORCALL sincos(rad64 a, f64 *s, f64 *c);
}
