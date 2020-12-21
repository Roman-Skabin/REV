//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "math/vec.h"

namespace REV::Math::Color
{
    inline const v4 g_BlackA0   (0.0f, 0.0f, 0.0f, 0.0f);
    inline const v4 g_RedA0     (1.0f, 0.0f, 0.0f, 0.0f);
    inline const v4 g_GreenA0   (0.0f, 1.0f, 0.0f, 0.0f);
    inline const v4 g_BlueA0    (0.0f, 0.0f, 1.0f, 0.0f);
    inline const v4 g_YellowA0  (1.0f, 1.0f, 0.0f, 0.0f);
    inline const v4 g_MagentaA0 (1.0f, 0.0f, 1.0f, 0.0f);
    inline const v4 g_CyanA0    (0.0f, 1.0f, 1.0f, 0.0f);
    inline const v4 g_WhiteA0   (1.0f, 1.0f, 1.0f, 0.0f);

    inline const v4 g_BlackA1   (0.0f, 0.0f, 0.0f, 1.0f);
    inline const v4 g_RedA1     (1.0f, 0.0f, 0.0f, 1.0f);
    inline const v4 g_GreenA1   (0.0f, 1.0f, 0.0f, 1.0f);
    inline const v4 g_BlueA1    (0.0f, 0.0f, 1.0f, 1.0f);
    inline const v4 g_YellowA1  (1.0f, 1.0f, 0.0f, 1.0f);
    inline const v4 g_MagentaA1 (1.0f, 0.0f, 1.0f, 1.0f);
    inline const v4 g_CyanA1    (0.0f, 1.0f, 1.0f, 1.0f);
    inline const v4 g_WhiteA1   (1.0f, 1.0f, 1.0f, 1.0f);

    // @NOTE(Roamn): Everything below is a legacy from the software rendering times.
    //               Btw it's not deprecated so you can use it as well.

    //
    // norm_to_*
    //

    REV_INLINE u32 REV_VECTORCALL norm_to_hex(v4 color)
    {
        __m128  clamped = _mm_max_ps(_mm_setzero_ps(), _mm_min_ps(_mm_set_ps1(1.0f), color.mm));
        __m128i mm      = _mm_cvtps_epi32(_mm_mul_ps(_mm_set_ps1(255.0f), clamped));
        return (mm_extract_u32<3>(mm) << 24)
             | (mm_extract_u32<0>(mm) << 16)
             | (mm_extract_u32<1>(mm) <<  8)
             | (mm_extract_u32<2>(mm)      );
    }

    REV_INLINE v4 REV_VECTORCALL norm_to_v4(v4 color)
    {
        __m128 clamped = _mm_max_ps(_mm_setzero_ps(), _mm_min_ps(_mm_set_ps1(1.0f), color.mm));
        return v4(_mm_mul_ps(_mm_set_ps1(255.0f), clamped));
    }

    REV_INLINE v4u REV_VECTORCALL norm_to_v4u(v4 color)
    {
        __m128 clamped = _mm_max_ps(_mm_setzero_ps(), _mm_min_ps(_mm_set_ps1(1.0f), color.mm));
        return v4u(_mm_cvtps_epi32(_mm_mul_ps(_mm_set_ps1(255.0f), clamped)));
    }

    //
    // v4_to_*
    //

    REV_INLINE v4 REV_VECTORCALL v4_to_norm(v4 color)
    {
        __m128 mm_255  = _mm_set_ps1(255.0f);
        __m128 clamped = _mm_max_ps(_mm_setzero_ps(), _mm_min_ps(mm_255, color.mm));
        return v4(_mm_div_ps(clamped, mm_255));
    }

    REV_INLINE u32 REV_VECTORCALL v4_to_hex(v4 color)
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

    REV_INLINE v4 REV_VECTORCALL hex_to_norm(u32 color)
    {
        __m128 mm_255  = _mm_set_ps1(255.0f);
        __m128 vcolor  = _mm_cvtepi32_ps(_mm_setr_epi32(color >> 16, color >> 8, color, color >> 24));
        __m128 clamped = _mm_max_ps(_mm_setzero_ps(), _mm_min_ps(mm_255, vcolor));
        return v4(_mm_div_ps(clamped, mm_255));
    }

    REV_INLINE v4 REV_VECTORCALL hex_to_v4(u32 color)
    {
        __m128 vcolor  = _mm_cvtepi32_ps(_mm_setr_epi32(color >> 16, color >> 8, color, color >> 24));
        __m128 clamped = _mm_max_ps(_mm_setzero_ps(), _mm_min_ps(_mm_set_ps1(255.0f), vcolor));
        return v4(clamped);
    }

    REV_INLINE v4u REV_VECTORCALL hex_to_v4u(u32 color)
    {
        __m128i vcolor  = _mm_setr_epi32(color >> 16, color >> 8, color, color >> 24);
        __m128i clamped = _mm_max_epi32(_mm_setzero_si128(), _mm_min_epi32(_mm_set1_epi32(0xFF), vcolor));
        return v4u(clamped);
    }

    //
    // v4u_to_*
    //

    REV_INLINE v4 REV_VECTORCALL v4u_to_norm(v4u color)
    {
        __m128 mm_255 = _mm_set_ps1(255.0f);
        __m128 clamped = _mm_max_ps(_mm_setzero_ps(), _mm_min_ps(mm_255, _mm_cvtepi32_ps(color.mm)));
        return v4(_mm_div_ps(clamped, mm_255));
    }

    REV_INLINE u32 REV_VECTORCALL v4u_to_hex(v4u color)
    {
        return (mm_extract_u32<3>(color.mm) << 24)
             | (mm_extract_u32<0>(color.mm) << 16)
             | (mm_extract_u32<1>(color.mm) <<  8)
             | (mm_extract_u32<2>(color.mm)      );
    }
}
