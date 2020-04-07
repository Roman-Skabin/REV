//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "math/vec.h"

//
// norm_to_*
//

INLINE u32 MATH_CALL norm_to_hex(v4 color)
{
    __m128i mm = _mm_cvtps_epi32(
                     _mm_mul_ps(
                         _mm_set_ps1(255.0f),
                         _mm_max_ps(
                             _mm_setzero_ps(),
                             _mm_min_ps(
                                 _mm_set_ps1(1.0f),
                                 color.mm))));
    return (MM(mm, u32, 3) << 24)
         | (MM(mm, u32, 0) << 16)
         | (MM(mm, u32, 1) <<  8)
         | (MM(mm, u32, 2)      );
}

INLINE v4 MATH_CALL norm_to_v4(v4 color)
{
    v4 out;
    out.mm = _mm_mul_ps(
                 _mm_set_ps1(255.0f),
                 _mm_max_ps(
                     _mm_setzero_ps(),
                     _mm_min_ps(
                         _mm_set_ps1(1.0f),
                         color.mm)));
    return out;
}

INLINE v4u MATH_CALL norm_to_v4u(v4 color)
{
    v4u out;
    out.mm = _mm_cvtps_epi32(
                 _mm_mul_ps(
                     _mm_set_ps1(255.0f),
                     _mm_max_ps(
                         _mm_setzero_ps(),
                         _mm_min_ps(
                             _mm_set_ps1(1.0f),
                             color.mm))));
    return out;
}

//
// v4_to_*
//

INLINE v4 MATH_CALL v4_to_norm(v4 color)
{
    v4 out;
    out.mm = _mm_div_ps(
                 _mm_cvtepi32_ps(
                     _mm_and_si128(
                         _mm_cvtps_epi32(color.mm),
                         _mm_set1_epi32(0xFF))),
                 _mm_set_ps1(255.0f));
    return out;
}

INLINE u32 MATH_CALL v4_to_hex(v4 color)
{
    __m128i mm = _mm_cvtps_epi32(color.mm);
    return (MM(mm, u32, 3) << 24)
         | (MM(mm, u32, 0) << 16)
         | (MM(mm, u32, 1) <<  8)
         | (MM(mm, u32, 2)      );
}

//
// hex_to_*
//

INLINE v4 MATH_CALL hex_to_norm(u32 color)
{
    v4 out;
    out.mm = _mm_div_ps(
                 _mm_cvtepi32_ps(
                     _mm_and_si128(
                         _mm_setr_epi32(color >> 16, color >> 8, color, color >> 24),
                         _mm_set1_epi32(0xFF))),
                 _mm_set_ps1(255.0f));
    return out;
}

INLINE v4 MATH_CALL hex_to_v4(u32 color)
{
    v4 out;
    out.mm = _mm_cvtepi32_ps(
                 _mm_and_si128(
                     _mm_setr_epi32(color >> 16, color >> 8, color, color >> 24),
                     _mm_set1_epi32(0xFF)));
    return out;
}

INLINE v4u MATH_CALL hex_to_v4u(u32 color)
{
    v4u out;
    out.mm = _mm_and_si128(
                 _mm_setr_epi32(color >> 16, color >> 8, color, color >> 24),
                 _mm_set1_epi32(0xFF));
    return out;
}

//
// v4u_to_*
//

INLINE v4 MATH_CALL v4u_to_norm(v4u color)
{
    v4 out;
    out.mm = _mm_div_ps(
                 _mm_cvtepi32_ps(
                     _mm_and_si128(
                         color.mm,
                         _mm_set1_epi32(0xFF))),
                 _mm_set_ps1(255.0f));
    return out;
}

INLINE u32 MATH_CALL v4u_to_hex(v4u color)
{
    return (MM(color.mm, u32, 3) << 24)
         | (MM(color.mm, u32, 0) << 16)
         | (MM(color.mm, u32, 1) <<  8)
         | (MM(color.mm, u32, 2)      );
}

//
// Blending
//

INLINE void MATH_CALL BlendOnRender(Engine *engine, u32 x, u32 y, f32 z, v4 color)
{
    s32  offset = y * engine->window.size.w + x;
    v4  *sum    = engine->renderer.blending.sum + offset;

    sum->mm = _mm_add_ps(
                  sum->mm,
                  _mm_mul_ps(
                      color.mm,
                      _mm_set_ps1(engine->renderer.zb.far - z)));

    engine->renderer.blending.mul[offset] *= 1.0f - color.a;
}

INLINE void MATH_CALL BlendOnPresent(u32 *pixel, v4 sum, f32 mul)
{
    __m128 mm_mul = _mm_set_ps1(mul);
    __m128 mm_one = _mm_set_ps1(1.0f);
    __m128 mm_255 = _mm_set_ps1(255.0f);

    // hex_to_norm with zero alpha
    __m128 dest_color = _mm_div_ps(
                            _mm_cvtepi32_ps(
                                _mm_and_si128(
                                    _mm_setr_epi32(*pixel >> 16, *pixel >> 8, *pixel, 0),
                                    _mm_set1_epi32(0xFF))),
                            mm_255);

    __m128 fraction = _mm_div_ps(sum.mm, _mm_set_ps(sum.a, sum.a, sum.a, 1.0f));
    __m128 bracket  = _mm_sub_ps(mm_one, mm_mul);
    __m128 left     = _mm_mul_ps(fraction, bracket);
    __m128 right    = _mm_mul_ps(dest_color, mm_mul);
    __m128 norm     = _mm_add_ps(left, right);

    // norm_to_hex
    __m128i hex = _mm_cvtps_epi32(
                      _mm_mul_ps(
                          mm_255,
                          _mm_max_ps(
                              _mm_setzero_ps(),
                              _mm_min_ps(
                                  mm_one,
                                  norm))));

    *pixel = (MM(hex, u32, 3) << 24)
           | (MM(hex, u32, 0) << 16)
           | (MM(hex, u32, 1) <<  8)
           | (MM(hex, u32, 2)      );
}
