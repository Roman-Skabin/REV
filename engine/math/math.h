//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/core.h"
#include <corecrt_math.h>

#define f64_E         2.71828182845904523536   // e
#define f64_LOG2E     1.44269504088896340736   // log2(e)
#define f64_LOG10E    0.434294481903251827651  // log10(e)
#define f64_LN2       0.693147180559945309417  // ln(2)
#define f64_LN10      2.30258509299404568402   // ln(10)
#define f64_PI        3.14159265358979323846   // pi
#define f64_PI_2      1.57079632679489661923   // pi/2
#define f64_PI_4      0.785398163397448309616  // pi/4
#define f64_1_PI      0.318309886183790671538  // 1/pi
#define f64_2_PI      0.636619772367581343076  // 2/pi
#define f64_2_SQRTPI  1.12837916709551257390   // 2/sqrt(pi)
#define f64_SQRT2     1.41421356237309504880   // sqrt(2)
#define f64_1_SQRT_2  0.707106781186547524401  // 1/sqrt(2)

#define f32_E        CSTRCAT(f64_E, f)
#define f32_LOG2E    CSTRCAT(f64_LOG2E, f)
#define f32_LOG10E   CSTRCAT(f64_LOG10E, f)
#define f32_LN2      CSTRCAT(f64_LN2, f)
#define f32_LN10     CSTRCAT(f64_LN10, f)
#define f32_PI       CSTRCAT(f64_PI, f)
#define f32_PI_2     CSTRCAT(f64_PI_2, f)
#define f32_PI_4     CSTRCAT(f64_PI_4, f)
#define f32_1_PI     CSTRCAT(f64_1_PI, f)
#define f32_2_PI     CSTRCAT(f64_2_PI, f)
#define f32_2_SQRTPI CSTRCAT(f64_2_SQRTPI, f)
#define f32_SQRT2    CSTRCAT(f64_SQRT2, f)
#define f32_1_SQRT_2 CSTRCAT(f64_1_SQRT_2, f)

union ENGINE_INTRIN_TYPE ENGINE_ALIGN(4) reg32
{
    f32 f;
    s32 s;
    u32 u;
    int i;

    reg32(              ) : f(0.0f) {}
    reg32(f32          v) : f(v)    {}
    reg32(s32          v) : s(v)    {}
    reg32(u32          v) : u(v)    {}
    reg32(int          v) : i(v)    {}
    reg32(const reg32& v) : f(v.f)  {}
    reg32(reg32&&      v) : f(v.f)  {}

    MATH_CALL operator f32() const { return f; }
    MATH_CALL operator s32() const { return s; }
    MATH_CALL operator u32() const { return u; }
    MATH_CALL operator int() const { return i; }

    reg32& MATH_CALL operator=(f32          v) { f = v;   return *this; }
    reg32& MATH_CALL operator=(s32          v) { s = v;   return *this; }
    reg32& MATH_CALL operator=(u32          v) { u = v;   return *this; }
    reg32& MATH_CALL operator=(int          v) { i = v;   return *this; }
    reg32& MATH_CALL operator=(const reg32& v) { f = v.f; return *this; }
    reg32& MATH_CALL operator=(reg32&&      v) { f = v.f; return *this; }
};

union ENGINE_INTRIN_TYPE ENGINE_ALIGN(8) reg64
{
    f64   f;
    s64   s;
    u64   u;

    reg64(              ) : f(0.0) {}
    reg64(f64          v) : f(v)   {}
    reg64(s64          v) : s(v)   {}
    reg64(u64          v) : u(v)   {}
    reg64(const reg64& v) : f(v.f) {}
    reg64(reg64&&      v) : f(v.f) {}

    MATH_CALL operator   f64() const { return f; }
    MATH_CALL operator   s64() const { return s; }
    MATH_CALL operator   u64() const { return u; }

    reg64& MATH_CALL operator=(f64          v) { f = v;   return *this; }
    reg64& MATH_CALL operator=(s64          v) { s = v;   return *this; }
    reg64& MATH_CALL operator=(u64          v) { u = v;   return *this; }
    reg64& MATH_CALL operator=(const reg64& v) { f = v.f; return *this; }
    reg64& MATH_CALL operator=(reg64&&      v) { f = v.f; return *this; }
};

template<typename T, typename = RTTI::enable_if_t<RTTI::is_floating_point_v<T>>>
INLINE T MATH_CALL lerp(T start, T end, T percent)
{
    percent = __max(0, __min(1, percent));
    return (end - start) * percent + start;
}

template<typename T, typename = RTTI::enable_if_t<RTTI::is_arithmetic_v<T>>>
INLINE T MATH_CALL clamp(T val, T min, T max)
{
    return __max(min, __min(max, val));
}

// value <= 20
ENGINE_FUN u64 MATH_CALL fact(u8 value);
