//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "math/vec.h"

inline v4 gBlackA0   (0.0f, 0.0f, 0.0f, 0.0f);
inline v4 gRedA0     (1.0f, 0.0f, 0.0f, 0.0f);
inline v4 gGreenA0   (0.0f, 1.0f, 0.0f, 0.0f);
inline v4 gBlueA0    (0.0f, 0.0f, 1.0f, 0.0f);
inline v4 gYellowA0  (1.0f, 1.0f, 0.0f, 0.0f);
inline v4 gMagentaA0 (1.0f, 0.0f, 1.0f, 0.0f);
inline v4 gCyanA0    (0.0f, 1.0f, 1.0f, 0.0f);
inline v4 gWhiteA0   (1.0f, 1.0f, 1.0f, 0.0f);

inline v4 gBlackA1   (0.0f, 0.0f, 0.0f, 1.0f);
inline v4 gRedA1     (1.0f, 0.0f, 0.0f, 1.0f);
inline v4 gGreenA1   (0.0f, 1.0f, 0.0f, 1.0f);
inline v4 gBlueA1    (0.0f, 0.0f, 1.0f, 1.0f);
inline v4 gYellowA1  (1.0f, 1.0f, 0.0f, 1.0f);
inline v4 gMagentaA1 (1.0f, 0.0f, 1.0f, 1.0f);
inline v4 gCyanA1    (0.0f, 1.0f, 1.0f, 1.0f);
inline v4 gWhiteA1   (1.0f, 1.0f, 1.0f, 1.0f);

// @NOTE(Roamn): Everything below is a legacy from the software rendering times.
//               Btw it's not deprecated so you can use it as well.

//
// norm_to_*
//

INLINE u32 __vectorcall norm_to_hex(v4 color)
{
    __m128i mm = _mm_cvtps_epi32(
                     _mm_mul_ps(
                         _mm_set_ps1(255.0f),
                         _mm_max_ps(
                             _mm_setzero_ps(),
                             _mm_min_ps(
                                 _mm_set_ps1(1.0f),
                                 color.mm))));
    return (mm_extract_u32<3>(mm) << 24)
         | (mm_extract_u32<0>(mm) << 16)
         | (mm_extract_u32<1>(mm) <<  8)
         | (mm_extract_u32<2>(mm)      );
}

INLINE v4 __vectorcall norm_to_v4(v4 color)
{
    v4 res;
    res.mm = _mm_mul_ps(
                 _mm_set_ps1(255.0f),
                 _mm_max_ps(
                     _mm_setzero_ps(),
                     _mm_min_ps(
                         _mm_set_ps1(1.0f),
                         color.mm)));
    return res;
}

INLINE v4u __vectorcall norm_to_v4u(v4 color)
{
    v4u res;
    res.mm = _mm_cvtps_epi32(
                 _mm_mul_ps(
                     _mm_set_ps1(255.0f),
                     _mm_max_ps(
                         _mm_setzero_ps(),
                         _mm_min_ps(
                             _mm_set_ps1(1.0f),
                             color.mm))));
    return res;
}

//
// v4_to_*
//

INLINE v4 __vectorcall v4_to_norm(v4 color)
{
    v4 res;
    res.mm = _mm_div_ps(
                 _mm_cvtepi32_ps(
                     _mm_and_si128(
                         _mm_cvtps_epi32(color.mm),
                         _mm_set1_epi32(0xFF))),
                 _mm_set_ps1(255.0f));
    return res;
}

INLINE u32 __vectorcall v4_to_hex(v4 color)
{
    __m128i mm = _mm_cvtps_epi32(color.mm);
    return (mm_extract_u32<3>(mm) << 24)
         | (mm_extract_u32<0>(mm) << 16)
         | (mm_extract_u32<1>(mm) <<  8)
         | (mm_extract_u32<2>(mm)      );
}

//
// hex_to_*
//

INLINE v4 __vectorcall hex_to_norm(u32 color)
{
    v4 res;
    res.mm = _mm_div_ps(
                 _mm_cvtepi32_ps(
                     _mm_and_si128(
                         _mm_setr_epi32(color >> 16, color >> 8, color, color >> 24),
                         _mm_set1_epi32(0xFF))),
                 _mm_set_ps1(255.0f));
    return res;
}

INLINE v4 __vectorcall hex_to_v4(u32 color)
{
    v4 res;
    res.mm = _mm_cvtepi32_ps(
                 _mm_and_si128(
                     _mm_setr_epi32(color >> 16, color >> 8, color, color >> 24),
                     _mm_set1_epi32(0xFF)));
    return res;
}

INLINE v4u __vectorcall hex_to_v4u(u32 color)
{
    v4u res;
    res.mm = _mm_and_si128(
                 _mm_setr_epi32(color >> 16, color >> 8, color, color >> 24),
                 _mm_set1_epi32(0xFF));
    return res;
}

//
// v4u_to_*
//

INLINE v4 __vectorcall v4u_to_norm(v4u color)
{
    v4 res;
    res.mm = _mm_div_ps(
                 _mm_cvtepi32_ps(
                     _mm_and_si128(
                         color.mm,
                         _mm_set1_epi32(0xFF))),
                 _mm_set_ps1(255.0f));
    return res;
}

INLINE u32 __vectorcall v4u_to_hex(v4u color)
{
    return (mm_extract_u32<3>(color.mm) << 24)
         | (mm_extract_u32<0>(color.mm) << 16)
         | (mm_extract_u32<1>(color.mm) <<  8)
         | (mm_extract_u32<2>(color.mm)      );
}
