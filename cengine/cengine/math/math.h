//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/core.h"
#include <math.h>

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

#define MM(mm, Type, i) (cast(Type *, &(mm))[i])

#define MMf(mm, n)  (mm).m128_f32[n]
#define MMs(mm, n) (mm).m128i_i32[n]
#define MMu(mm, n) (mm).m128i_u32[n]

INLINE f32 MATH_CALL lerp(f32 start, f32 end, f32 percent)
{
    percent = __max(0.0f, __min(1.0f, percent));
    return (end - start) * percent + start;
}

// value <= 20
CEXTERN u64 __fastcall fact(u8 value);
