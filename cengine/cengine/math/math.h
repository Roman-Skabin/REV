//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/core.h"

#define MM128f32(mm, i) (mm).m128_f32[i]
#define MM128i32(mm, i) (mm).m128i_i32[i]
#define MM128u32(mm, i) (mm).m128i_u32[i]

#define MM256f32(mm, i) (mm).m256_f32[i]
#define MM256i32(mm, i) (mm).m256i_i32[i]
#define MM256u32(mm, i) (mm).m256i_u32[i]

INLINE f32 MATH_CALL lerp(f32 start, f32 end, f32 percent)
{
    percent = max(0.0f, min(1.0f, percent));
    return (end - start) * percent + start;
}

// value <= 20
u64 __fastcall fact(u8 value);
