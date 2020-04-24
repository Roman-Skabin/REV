//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "math/vec.h"

#ifndef __AVX__ 
#error Compile with -arch:AVX or higher
#endif

//
// m2
//

typedef union __intrin_type __align(16) m2
{
    struct
    {
        f32 e00, e01;
        f32 e10, e11;
    };
    struct
    {
        v2 r0;
        v2 r1;
    };
    __m128 mm;
} m2;

INLINE m2 MATH_CALL m2_zero()
{
    m2 res;
    res.mm = _mm_setzero_ps();
    return res;
}

INLINE m2 MATH_CALL m2_0(f32 val)
{
    m2 res;
    res.mm = _mm_set_ps1(val);
    return res;
}

INLINE m2 MATH_CALL m2_1(f32 e00, f32 e01, f32 e10, f32 e11)
{
    m2 res;
    res.mm = _mm_setr_ps(e00, e01,
                         e10, e11);
    return res;
}

INLINE m2 MATH_CALL m2_2(v2 c1, v2 c2)
{
    m2 res;
    res.mm = _mm_setr_ps(c1.x, c2.x,
                         c1.y, c2.y);
    return res;
}

INLINE m2 MATH_CALL m2_3(f32 arr[4])
{
    m2 res;
    res.mm = _mm_loadu_ps(arr);
    return res;
}

INLINE m2 MATH_CALL m2_add(m2 l, m2 r)
{
    m2 res;
    res.mm = _mm_add_ps(l.mm, r.mm);
    return res;
}

INLINE m2 MATH_CALL m2_sub(m2 l, m2 r)
{
    m2 res;
    res.mm = _mm_sub_ps(l.mm, r.mm);
    return res;
}

INLINE m2 MATH_CALL m2_mul(m2 l, m2 r)
{
    // l.e00 * r.e00 + l.e01 * r.e10     l.e00 * r.e01 + l.e01 * r.e11
    // l.e10 * r.e00 + l.e11 * r.e10     l.e10 * r.e01 + l.e11 * r.e11
    //
    // c1[0] + c1[1]     c2[0] + c2[1]
    // c1[2] + c1[3]     c2[2] + c2[3]

#if 1
    __m128 r1 = _mm_shuffle_ps(r.mm, r.mm, 0x88);
    __m128 r2 = _mm_shuffle_ps(r.mm, r.mm, 0xDD);
#else
    __m128 r1 = _mm_setr_ps(r.e00, r.e10, r.e00, r.e10);
    __m128 r2 = _mm_setr_ps(r.e01, r.e11, r.e01, r.e11);
#endif
    __m128 c1 = _mm_mul_ps(l.mm, r1);
    __m128 c2 = _mm_mul_ps(l.mm, r2);

    m2 res;
    res.mm = _mm_hadd_ps(c1, c2);
    return res;
}

INLINE m2 MATH_CALL m2_add_s(f32 l, m2 r)
{
    m2 res;
    res.mm = _mm_add_ps(_mm_set_ps1(l), r.mm);
    return res;
}

INLINE m2 MATH_CALL m2_sub_s(m2 l, f32 r)
{
    m2 res;
    res.mm = _mm_sub_ps(l.mm, _mm_set_ps1(r));
    return res;
}

INLINE m2 MATH_CALL m2_mul_s(f32 l, m2 r)
{
    m2 res;
    res.mm = _mm_mul_ps(_mm_set_ps1(l), r.mm);
    return res;
}

INLINE v2 MATH_CALL m2_mul_v(m2 l, v2 r)
{
    __m128 mm = _mm_mul_ps(l.mm, _mm_setr_ps(r.x, r.y, r.x, r.y));
    mm = _mm_hadd_ps(mm, mm);

    v2 res;
    res.x = mm.m128_f32[0];
    res.y = mm.m128_f32[1];
    return res;
}

INLINE f32 MATH_CALL m2_det(m2 m)
{
    return m.e00 * m.e11 - m.e01 * m.e10;
}

INLINE m2 MATH_CALL m2_inverse(m2 m)
{
#if 1
    __m128 det = _mm_set_ps1(m.e00 * m.e11 - m.e01 * m.e10);
#else
    __m128 _1 = _mm_set_ps1(m.e00);
    __m128 _2 = _mm_set_ps1(m.e11);
    __m128 _3 = _mm_set_ps1(m.e01);
    __m128 _4 = _mm_set_ps1(m.e10);

    __m128 det = _mm_sub_ps(_mm_mul_ps(_1, _2),
                            _mm_mul_ps(_3, _4))
#endif

    m2 res;
    res.mm = _mm_div_ps(_mm_shuffle_ps(m.mm, m.mm, 0x27), det);
    res.e01 *= -1;
    res.e10 *= -1;
    return res;
}

INLINE m2 MATH_CALL m2_transpose(m2 m)
{
    m2 res;
    res.mm = _mm_shuffle_ps(m.mm, m.mm, 0xD8);
    return res;
}

INLINE m2 MATH_CALL m2_identity()
{
    m2 res;
    res.mm = _mm_setr_ps(1.0f, 0.0f,
                         0.0f, 1.0f);
    return res;
}

INLINE m2 MATH_CALL m2_scaling(f32 nx, f32 ny)
{
    m2 res;
    res.mm = _mm_setr_ps(  nx, 0.0f,
                         0.0f,   ny);
    return res;
}

INLINE m2 MATH_CALL m2_rotation_z(rad angle)
{
    f32 s = sinf(angle);
    f32 c = cosf(angle);

    m2 res;
    res.mm = _mm_setr_ps(c, -s,
                         s, c);
    return res;
}

INLINE m2 MATH_CALL m2_reflection_x()
{
    m2 res;
    res.mm = _mm_setr_ps(-1.0f, 0.0f,
                          0.0f, 1.0f);
    return res;
}

INLINE m2 MATH_CALL m2_reflection_y()
{
    m2 res;
    res.mm = _mm_setr_ps(1.0f,  0.0f,
                         0.0f, -1.0f);
    return res;
}

INLINE m2 MATH_CALL m2_reflection_xy()
{
    m2 res;
    res.mm = _mm_setr_ps(-1.0f,  0.0f,
                          0.0f, -1.0f);
    return res;
}

INLINE m2 MATH_CALL m2_shearing(f32 shx, f32 shy)
{
    m2 res;
    res.mm = _mm_setr_ps(1.0f,  shx,
                          shy, 1.0f);
    return res;
}

//
// m3
//

typedef union __intrin_type m3
{
    struct
    {
        f32 e00, e01, e02, __alignment_0;
        f32 e10, e11, e12, __alignment_1;
        f32 e20, e21, e22, __alignment_2;
    };
    struct
    {
        v3 r0;
        v3 r1;
        v3 r2;
    };
    struct
    {
        __m256 mm0;
        __m128 mm1;
    };
} m3;

INLINE m3 MATH_CALL m3_zero()
{
    m3 res;
    res.mm0 = _mm256_setzero_ps();
    res.mm1 = _mm_setzero_ps();
    return res;
}

INLINE m3 MATH_CALL m3_0(f32 val)
{
    m3 res;
    res.mm0 = _mm256_setr_ps(
        val, val, val, 0.0f,
        val, val, val, 0.0f);
    res.mm1 = _mm_setr_ps(
        val, val, val, 0.0f);
    return res;
}

INLINE m3 MATH_CALL m3_1(
    f32 e00, f32 e01, f32 e02,
    f32 e10, f32 e11, f32 e12,
    f32 e20, f32 e21, f32 e22)
{
    m3 res;
    res.mm0 = _mm256_setr_ps(
        e00, e01, e02, 0.0f,
        e10, e11, e12, 0.0f);
    res.mm1 = _mm_setr_ps(
        e20, e21, e22, 0.0f);
    return res;
}

INLINE m3 MATH_CALL m3_2(v3 c1, v3 c2, v3 c3)
{
    m3 res;
    res.mm0 = _mm256_setr_ps(
        c1.x, c2.x, c3.x, 0.0f,
        c1.y, c2.y, c3.y, 0.0f);
    res.mm1 = _mm_setr_ps(
        c1.z, c2.z, c3.z, 0.0f);
    return res;
}

INLINE m3 MATH_CALL m3_3(f32 arr[9])
{
    m3 res;
    res.mm0 = _mm256_setr_ps(
        arr[0], arr[1], arr[2], 0.0f,
        arr[3], arr[4], arr[5], 0.0f);
    res.mm1 = _mm_setr_ps(
        arr[6], arr[7], arr[8], 0.0f);
    return res;
}

INLINE m3 MATH_CALL m3_add(m3 l, m3 r)
{
    m3 res;
    res.mm0 = _mm256_add_ps(l.mm0, r.mm0);
    res.mm1 = _mm_add_ps(l.mm1, r.mm1);
    return res;
}

INLINE m3 MATH_CALL m3_sub(m3 l, m3 r)
{
    m3 res;
    res.mm0 = _mm256_sub_ps(l.mm0, r.mm0);
    res.mm1 = _mm_sub_ps(l.mm1, r.mm1);
    return res;
}

INLINE m3 MATH_CALL m3_transpose(m3 m)
{
    m3 res;
    res.mm0 = _mm256_setr_ps(
        m.e00, m.e10, m.e20, 0.0f,
        m.e01, m.e11, m.e21, 0.0f);
    res.mm1 = _mm_setr_ps(
        m.e02, m.e12, m.e22, 0.0f);
    return res;
}

INLINE m3 MATH_CALL m3_mul(m3 l, m3 r)
{
    __m128 r_c0 = _mm_setr_ps(r.e00, r.e10, r.e20, 0.0f);
    __m128 r_c1 = _mm_setr_ps(r.e01, r.e11, r.e21, 0.0f);
    __m128 r_c2 = _mm_setr_ps(r.e02, r.e12, r.e22, 0.0f);

    m3 res;
    res.mm0 = _mm256_setr_ps(
        _mm_cvtss_f32(_mm_dp_ps(l.r0.mm, r_c0, 0x71)), _mm_cvtss_f32(_mm_dp_ps(l.r0.mm, r_c1, 0x71)), _mm_cvtss_f32(_mm_dp_ps(l.r0.mm, r_c2, 0x71)), 0.0f,
        _mm_cvtss_f32(_mm_dp_ps(l.r1.mm, r_c0, 0x71)), _mm_cvtss_f32(_mm_dp_ps(l.r1.mm, r_c1, 0x71)), _mm_cvtss_f32(_mm_dp_ps(l.r1.mm, r_c2, 0x71)), 0.0f);
    res.mm1 = _mm_setr_ps(
        _mm_cvtss_f32(_mm_dp_ps(l.r2.mm, r_c0, 0x71)), _mm_cvtss_f32(_mm_dp_ps(l.r2.mm, r_c1, 0x71)), _mm_cvtss_f32(_mm_dp_ps(l.r2.mm, r_c2, 0x71)), 0.0f);
    return res;
}

INLINE m3 MATH_CALL m3_add_s(f32 l, m3 r)
{
    __m256 l_mm0 = _mm256_set1_ps(l);
    __m128 l_mm1 = _mm_set_ps1(l);

    m3 res;
    res.mm0 = _mm256_add_ps(l_mm0, r.mm0);
    res.mm1 = _mm_add_ps(l_mm1, r.mm1);
    return res;
}

INLINE m3 MATH_CALL m3_sub_s(m3 l, f32 r)
{
    __m256 r_mm0 = _mm256_set1_ps(r);
    __m128 r_mm1 = _mm_set_ps1(r);

    m3 res;
    res.mm0 = _mm256_sub_ps(l.mm0, r_mm0);
    res.mm1 = _mm_sub_ps(l.mm1, r_mm1);
    return res;
}

INLINE m3 MATH_CALL m3_mul_s(f32 l, m3 r)
{
    __m256 l_mm0 = _mm256_set1_ps(l);
    __m128 l_mm1 = _mm_set_ps1(l);

    m3 res;
    res.mm0 = _mm256_mul_ps(l_mm0, r.mm0);
    res.mm1 = _mm_mul_ps(l_mm1, r.mm1);
    return res;
}

INLINE v3 MATH_CALL m3_mul_v(m3 l, v3 r)
{
    v3 res;
    res.mm = _mm_setr_ps(
        _mm_cvtss_f32(_mm_dp_ps(l.r0.mm, r.mm, 0x71)),
        _mm_cvtss_f32(_mm_dp_ps(l.r1.mm, r.mm, 0x71)),
        _mm_cvtss_f32(_mm_dp_ps(l.r2.mm, r.mm, 0x71)),
        0.0f);
    return res;
}

INLINE f32 MATH_CALL m3_det(m3 m)
{
    // return m.e00 * (m.e11 * m.e22 - m.e12 * m.21)
    //      - m.e01 * (m.e10 * m.e22 - m.e12 * m.20)
    //      + m.e02 * (m.e10 * m.e21 - m.e11 * m.20);

    __m128 a = _mm_mul_ps(
                   _mm_setr_ps(m.e11, m.e10, m.e10, 0.0f),
                   _mm_setr_ps(m.e22, m.e22, m.e21, 0.0f));

    __m128 b = _mm_mul_ps(
                   _mm_setr_ps(m.e12, m.e12, m.e11, 0.0f),
                   _mm_setr_ps(m.e21, m.e20, m.e20, 0.0f));

    __m128 c = _mm_mul_ps(
                   _mm_setr_ps(m.e00, m.e01, m.e02, 0.0f),
                   _mm_sub_ps(a, b));

    __m128 lowsub = _mm_hsub_ps(c, c);
    __m128 res    = _mm_add_ps(lowsub, c);

    return res.m128_f32[2];
}

INLINE m3 MATH_CALL m3_inverse(m3 m)
{
    __m128 col_1 = _mm_sub_ps(
                       _mm_mul_ps(
                           _mm_setr_ps(m.e11, m.e12, m.e10, 0.0f),
                           _mm_setr_ps(m.e22, m.e20, m.e21, 0.0f)),
                       _mm_mul_ps(
                           _mm_setr_ps(m.e12, m.e10, m.e11, 0.0f),
                           _mm_setr_ps(m.e21, m.e22, m.e20, 0.0f)));

    __m128 col_2 = _mm_sub_ps(
                       _mm_mul_ps(
                           _mm_setr_ps(m.e02, m.e00, m.e01, 0.0f),
                           _mm_setr_ps(m.e21, m.e22, m.e20, 0.0f)),
                       _mm_mul_ps(
                           _mm_setr_ps(m.e01, m.e02, m.e00, 0.0f),
                           _mm_setr_ps(m.e22, m.e20, m.e21, 0.0f)));

    __m128 col_3 = _mm_sub_ps(
                       _mm_mul_ps(
                           _mm_setr_ps(m.e01, m.e02, m.e00, 0.0f),
                           _mm_setr_ps(m.e12, m.e10, m.e01, 0.0f)),
                       _mm_mul_ps(
                           _mm_setr_ps(m.e02, m.e00, m.e10, 0.0f),
                           _mm_setr_ps(m.e11, m.e12, m.e11, 0.0f)));

    __m128 det = _mm_set_ps1(m3_det(m));

    col_1 = _mm_div_ps(col_1, det);
    col_2 = _mm_div_ps(col_2, det);
    col_3 = _mm_div_ps(col_3, det);

    m3 res;
    res.mm0 = _mm256_setr_ps(
        col_1.m128_f32[0], col_2.m128_f32[0], col_3.m128_f32[0], 0.0f,
        col_1.m128_f32[1], col_2.m128_f32[1], col_3.m128_f32[1], 0.0f);
    res.mm1 = _mm_setr_ps(
        col_1.m128_f32[2], col_2.m128_f32[2], col_3.m128_f32[2], 0.0f);
    return res;
}

INLINE m3 MATH_CALL m3_identity()
{
    m3 res;
    res.mm0 = _mm256_setr_ps(
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f);
    res.mm1 = _mm_setr_ps(
        0.0f, 0.0f, 1.0f, 0.0f);
    return res;
}

INLINE m3 MATH_CALL m3_scaling(f32 nx, f32 ny)
{
    m3 res;
    res.mm0 = _mm256_setr_ps(
          nx, 0.0f, 0.0f, 0.0f,
        0.0f,   ny, 0.0f, 0.0f);
    res.mm1 = _mm_setr_ps(
        0.0f, 0.0f, 1.0f, 0.0f);
    return res;
}

INLINE m3 MATH_CALL m3_rotation_z(rad angle)
{
    f32 s = sinf(angle);
    f32 c = cosf(angle);

    m3 res;
    res.mm0 = _mm256_setr_ps(
           c,   -s, 0.0f, 0.0f,
           s,    c, 0.0f, 0.0f);
    res.mm1 = _mm_setr_ps(
        0.0f, 0.0f, 1.0f, 0.0f);
    return res;
}

INLINE m3 MATH_CALL m3_reflection_x()
{
    m3 res;
    res.mm0 = _mm256_setr_ps(
        -1.0f, 0.0f, 0.0f, 0.0f,
         0.0f, 1.0f, 0.0f, 0.0f);
    res.mm1 = _mm_setr_ps(
         0.0f, 0.0f, 1.0f, 0.0f);
    return res;
}

INLINE m3 MATH_CALL m3_reflection_y()
{
    m3 res;
    res.mm0 = _mm256_setr_ps(
        1.0f,  0.0f, 0.0f, 0.0f,
        0.0f, -1.0f, 0.0f, 0.0f);
    res.mm1 = _mm_setr_ps(
        0.0f,  0.0f, 1.0f, 0.0f);
    return res;
}

INLINE m3 MATH_CALL m3_reflection_xy()
{
    m3 res;
    res.mm0 = _mm256_setr_ps(
        -1.0f,  0.0f, 0.0f, 0.0f,
         0.0f, -1.0f, 0.0f, 0.0f);
    res.mm1 = _mm_setr_ps(
         0.0f,  0.0f, 1.0f, 0.0f);
    return res;
}

INLINE m3 MATH_CALL m3_shearing(f32 shx, f32 shy)
{
    m3 res;
    res.mm0 = _mm256_setr_ps(
        1.0f,  shx, 0.0f, 0.0f,
         shy, 1.0f, 0.0f, 0.0f);
    res.mm1 = _mm_setr_ps(
        0.0f, 0.0f, 1.0f, 0.0f);
    return res;
}

INLINE m3 MATH_CALL m3_translation(f32 dx, f32 dy)
{
    m3 res;
    res.mm0 = _mm256_setr_ps(
        1.0f, 0.0f,   dx, 0.0f,
        0.0f, 1.0f,   dy, 0.0f);
    res.mm1 = _mm_setr_ps(
        0.0f, 0.0f, 1.0f, 0.0f);
    return res;
}

INLINE m3 MATH_CALL m3_translation_v(v3 v)
{
    m3 res;
    res.mm0 = _mm256_setr_ps(
        1.0f, 0.0f,  v.x, 0.0f,
        0.0f, 1.0f,  v.y, 0.0f);
    res.mm1 = _mm_setr_ps(
        0.0f, 0.0f, 1.0f, 0.0f);
    return res;
}

INLINE m3 MATH_CALL m3_ortho(f32 left, f32 right, f32 bottom, f32 top)
{
    m3 res;
    res.mm0 = _mm256_setr_ps(
        2.0f/(right-left),              0.0f, -(right+left)/(right-left), 0.0f,
                     0.0f, 2.0f/(top-bottom), -(top+bottom)/(top-bottom), 0.0f);
    res.mm1 = _mm_setr_ps(
                     0.0f,              0.0f,                       1.0f, 0.0f);
    return res;
}

//
// m4
//

typedef union __intrin_type __align(32) m4
{
    struct
    {
        f32 e00, e01, e02, e03;
        f32 e10, e11, e12, e13;
        f32 e20, e21, e22, e23;
        f32 e30, e31, e32, e33;
    };
    struct
    {
        v4 r0;
        v4 r1;
        v4 r2;
        v4 r3;
    };
    struct
    {
        __m256 mm0;
        __m256 mm1;
    };
} m4;

INLINE m4 MATH_CALL m4_zero()
{
    m4 res;
    res.mm0 = _mm256_setzero_ps();
    res.mm1 = _mm256_setzero_ps();
    return res;
}

INLINE m4 MATH_CALL m4_0(f32 val)
{
    m4 res;
    res.mm0 = _mm256_set1_ps(val);
    res.mm1 = _mm256_set1_ps(val);
    return res;
}

INLINE m4 MATH_CALL m4_1(
    f32 e00, f32 e01, f32 e02, f32 e03,
    f32 e10, f32 e11, f32 e12, f32 e13,
    f32 e20, f32 e21, f32 e22, f32 e23,
    f32 e30, f32 e31, f32 e32, f32 e33)
{
    m4 res;
    res.mm0 = _mm256_setr_ps(e00, e01, e02, e03,
                             e10, e11, e12, e13);
    res.mm1 = _mm256_setr_ps(e20, e21, e22, e23,
                             e30, e31, e32, e33);
    return res;
}

INLINE m4 MATH_CALL m4_2(v4 c0, v4 c1, v4 c2, v4 c3)
{
    m4 res;
    res.mm0 = _mm256_setr_ps(c0.x, c1.x, c2.x, c3.x,
                             c0.y, c1.y, c2.y, c3.y);
    res.mm1 = _mm256_setr_ps(c0.z, c1.z, c2.z, c3.z,
                             c0.w, c1.w, c2.w, c3.w);
    return res;
}

INLINE m4 MATH_CALL m4_3(f32 arr[16])
{
    m4 res;
    res.mm0 = _mm256_loadu_ps(arr    );
    res.mm1 = _mm256_loadu_ps(arr + 8);
    return res;
}

INLINE m4 MATH_CALL m4_add(m4 l, m4 r)
{
    m4 res;
    res.mm0 = _mm256_add_ps(l.mm0, r.mm0);
    res.mm1 = _mm256_add_ps(l.mm1, r.mm1);
    return res;
}

INLINE m4 MATH_CALL m4_sub(m4 l, m4 r)
{
    m4 res;
    res.mm0 = _mm256_sub_ps(l.mm0, r.mm0);
    res.mm1 = _mm256_sub_ps(l.mm1, r.mm1);
    return res;
}

INLINE m4 MATH_CALL m4_transpose(m4 m)
{
    m4 res;
    res.mm0 = _mm256_setr_ps(m.e00, m.e10, m.e20, m.e30,
                             m.e01, m.e11, m.e21, m.e31);
    res.mm1 = _mm256_setr_ps(m.e02, m.e12, m.e22, m.e32,
                             m.e03, m.e13, m.e23, m.e33);
    return res;
}

INLINE m4 MATH_CALL m4_mul(m4 l, m4 r)
{
    __m128 r_c0 = _mm_setr_ps(r.e00, r.e10, r.e20, r.e30);
    __m128 r_c1 = _mm_setr_ps(r.e01, r.e11, r.e21, r.e31);
    __m128 r_c2 = _mm_setr_ps(r.e02, r.e12, r.e22, r.e32);
    __m128 r_c3 = _mm_setr_ps(r.e03, r.e13, r.e23, r.e33);

    m4 res;
    res.mm0 = _mm256_setr_ps(
        _mm_cvtss_f32(_mm_dp_ps(l.r0.mm, r_c0, 0xF1)), _mm_cvtss_f32(_mm_dp_ps(l.r0.mm, r_c1, 0xF1)), _mm_cvtss_f32(_mm_dp_ps(l.r0.mm, r_c2, 0xF1)), _mm_cvtss_f32(_mm_dp_ps(l.r0.mm, r_c3, 0xF1)),
        _mm_cvtss_f32(_mm_dp_ps(l.r1.mm, r_c0, 0xF1)), _mm_cvtss_f32(_mm_dp_ps(l.r1.mm, r_c1, 0xF1)), _mm_cvtss_f32(_mm_dp_ps(l.r1.mm, r_c2, 0xF1)), _mm_cvtss_f32(_mm_dp_ps(l.r1.mm, r_c3, 0xF1)));
    res.mm1 = _mm256_setr_ps(
        _mm_cvtss_f32(_mm_dp_ps(l.r2.mm, r_c0, 0xF1)), _mm_cvtss_f32(_mm_dp_ps(l.r2.mm, r_c1, 0xF1)), _mm_cvtss_f32(_mm_dp_ps(l.r2.mm, r_c2, 0xF1)), _mm_cvtss_f32(_mm_dp_ps(l.r2.mm, r_c3, 0xF1)),
        _mm_cvtss_f32(_mm_dp_ps(l.r3.mm, r_c0, 0xF1)), _mm_cvtss_f32(_mm_dp_ps(l.r3.mm, r_c1, 0xF1)), _mm_cvtss_f32(_mm_dp_ps(l.r3.mm, r_c2, 0xF1)), _mm_cvtss_f32(_mm_dp_ps(l.r3.mm, r_c3, 0xF1)));
    return res;
}

INLINE m4 MATH_CALL m4_add_s(f32 l, m4 r)
{
    __m256 mm = _mm256_set1_ps(l);

    m4 res;
    res.mm0 = _mm256_add_ps(mm, r.mm0);
    res.mm1 = _mm256_add_ps(mm, r.mm1);
    return res;
}

INLINE m4 MATH_CALL m4_sub_s(m4 l, f32 r)
{
    __m256 mm = _mm256_set1_ps(r);

    m4 res;
    res.mm0 = _mm256_sub_ps(l.mm0, mm);
    res.mm1 = _mm256_sub_ps(l.mm1, mm);
    return res;
}

INLINE m4 MATH_CALL m4_mul_s(f32 l, m4 r)
{
    __m256 mm = _mm256_set1_ps(l);

    m4 res;
    res.mm0 = _mm256_mul_ps(mm, r.mm0);
    res.mm1 = _mm256_mul_ps(mm, r.mm1);
    return res;
}

INLINE v4 MATH_CALL m4_mul_v(m4 l, v4 r)
{
    v4 res;
    res.mm = _mm_setr_ps(
        _mm_cvtss_f32(_mm_dp_ps(l.r0.mm, r.mm, 0xF1)),
        _mm_cvtss_f32(_mm_dp_ps(l.r1.mm, r.mm, 0xF1)),
        _mm_cvtss_f32(_mm_dp_ps(l.r2.mm, r.mm, 0xF1)),
        _mm_cvtss_f32(_mm_dp_ps(l.r3.mm, r.mm, 0xF1)));
    return res;
}

INLINE f32 MATH_CALL m4_det(m4 m)
{
    // return m.e00 * (  m.e11 * (m.e22 * m.e33 - m.e23 * m.e32)
    //                 - m.e12 * (m.e21 * m.e33 - m.e23 * m.e31)
    //                 + m.e13 * (m.e21 * m.e32 - m.e22 * m.e31))
    //      - m.e01 * (  m.e10 * (m.e22 * m.e33 - m.e23 * m.e32)
    //                 - m.e12 * (m.e20 * m.e33 - m.e23 * m.e30)
    //                 + m.e13 * (m.e20 * m.e32 - m.e22 * m.e30))
    //      + m.e02 * (  m.e10 * (m.e21 * m.e33 - m.e23 * m.e31)
    //                 - m.e11 * (m.e20 * m.e33 - m.e23 * m.e30)
    //                 + m.e13 * (m.e20 * m.e31 - m.e21 * m.e30))
    //      - m.e03 * (  m.e10 * (m.e21 * m.e32 - m.e22 * m.e31)
    //                 - m.e11 * (m.e20 * m.e32 - m.e22 * m.e30)
    //                 + m.e12 * (m.e20 * m.e31 - m.e21 * m.e30));

    __m256 first_1 = _mm256_setr_ps(m.e22, m.e21, m.e21, m.e22, m.e20, m.e20, m.e21, m.e20);
    __m128 first_2 = _mm_setr_ps(m.e20, m.e21, m.e20, m.e20);

    __m256 second_1 = _mm256_setr_ps(m.e33, m.e33, m.e32, m.e33, m.e33, m.e32, m.e33, m.e33);
    __m128 second_2 = _mm_setr_ps(m.e31, m.e32, m.e32, m.e31);

    __m256 third_1 = _mm256_setr_ps(m.e23, m.e23, m.e22, m.e23, m.e23, m.e22, m.e23, m.e23);
    __m128 third_2 = _mm_setr_ps(m.e21, m.e22, m.e22, m.e21);

    __m256 forth_1 = _mm256_setr_ps(m.e32, m.e31, m.e31, m.e32, m.e30, m.e30, m.e31, m.e30);
    __m128 forth_2 = _mm_setr_ps(m.e30, m.e31, m.e30, m.e30);

    __m256 fifth_1 = _mm256_setr_ps(m.e11, m.e12, m.e13, m.e10, m.e12, m.e13, m.e10, m.e11);
    __m128 fifth_2 = _mm_setr_ps(m.e13, m.e10, m.e11, m.e12);

    __m256 subres_1 = _mm256_mul_ps(fifth_1, _mm256_sub_ps(_mm256_mul_ps(first_1, second_1), _mm256_mul_ps(third_1, forth_1)));
    __m128 subres_2 = _mm_mul_ps(fifth_2, _mm_sub_ps(_mm_mul_ps(first_2, second_2), _mm_mul_ps(third_2, forth_2)));

    __m128 right_1 = _mm_setr_ps(subres_1.m256_f32[0], subres_1.m256_f32[3], subres_1.m256_f32[6], subres_2.m128_f32[1]);
    __m128 right_2 = _mm_setr_ps(subres_1.m256_f32[1], subres_1.m256_f32[4], subres_1.m256_f32[7], subres_2.m128_f32[2]);
    __m128 right_3 = _mm_setr_ps(subres_1.m256_f32[2], subres_1.m256_f32[5], subres_2.m128_f32[0], subres_2.m128_f32[3]);

    __m128 left  = _mm_setr_ps(m.e00, m.e01, m.e02, m.e03);
    __m128 right = _mm_add_ps(_mm_sub_ps(right_1, right_2), right_3);

    __m128 res = _mm_mul_ps(left, right);
    res = _mm_hsub_ps(res, res);
    res = _mm_add_ps(_mm_moveldup_ps(res), res);

    return res.m128_f32[1];
}

INLINE m4 MATH_CALL m4_inverse(m4 m)
{
    f32 c00 = m3_det(m3_1(m.e11, m.e12, m.e13,
                          m.e21, m.e22, m.e23,
                          m.e31, m.e32, m.e33));
    f32 c01 = m3_det(m3_1(m.e10, m.e12, m.e13,
                          m.e20, m.e22, m.e23,
                          m.e30, m.e32, m.e33));
    f32 c02 = m3_det(m3_1(m.e10, m.e11, m.e13,
                          m.e20, m.e21, m.e23,
                          m.e30, m.e31, m.e33));
    f32 c03 = m3_det(m3_1(m.e10, m.e11, m.e12,
                          m.e20, m.e21, m.e22,
                          m.e30, m.e31, m.e32));
    f32 c10 = m3_det(m3_1(m.e01, m.e02, m.e03,
                          m.e21, m.e22, m.e23,
                          m.e31, m.e32, m.e33));
    f32 c11 = m3_det(m3_1(m.e00, m.e02, m.e03,
                          m.e20, m.e22, m.e23,
                          m.e30, m.e32, m.e33));
    f32 c12 = m3_det(m3_1(m.e00, m.e01, m.e03,
                          m.e20, m.e21, m.e23,
                          m.e30, m.e31, m.e33));
    f32 c13 = m3_det(m3_1(m.e00, m.e01, m.e02,
                          m.e20, m.e21, m.e22,
                          m.e30, m.e31, m.e32));
    f32 c20 = m3_det(m3_1(m.e01, m.e02, m.e03,
                          m.e11, m.e12, m.e13,
                          m.e31, m.e32, m.e33));
    f32 c21 = m3_det(m3_1(m.e00, m.e02, m.e03,
                          m.e10, m.e12, m.e13,
                          m.e30, m.e32, m.e33));
    f32 c22 = m3_det(m3_1(m.e00, m.e01, m.e03,
                          m.e10, m.e11, m.e13,
                          m.e30, m.e31, m.e33));
    f32 c23 = m3_det(m3_1(m.e00, m.e01, m.e02,
                          m.e10, m.e11, m.e12,
                          m.e30, m.e31, m.e32));
    f32 c30 = m3_det(m3_1(m.e01, m.e02, m.e03,
                          m.e11, m.e12, m.e13,
                          m.e21, m.e22, m.e23));
    f32 c31 = m3_det(m3_1(m.e00, m.e02, m.e03,
                          m.e10, m.e12, m.e13,
                          m.e20, m.e22, m.e23));
    f32 c32 = m3_det(m3_1(m.e00, m.e01, m.e03,
                          m.e10, m.e11, m.e13,
                          m.e20, m.e21, m.e23));
    f32 c33 = m3_det(m3_1(m.e00, m.e01, m.e02,
                          m.e10, m.e11, m.e12,
                          m.e20, m.e21, m.e22));

    m4 res;
    res.mm0 = _mm256_setr_ps(c00, c01, c02, c03,
                             c10, c11, c12, c13);
    res.mm1 = _mm256_setr_ps(c20, c21, c22, c23,
                             c30, c31, c32, c33);

    __m256 det = _mm256_set1_ps(m4_det(m));
    res.mm0 = _mm256_div_ps(res.mm0, det);
    res.mm1 = _mm256_div_ps(res.mm1, det);

    return res;
}

INLINE m4 MATH_CALL m4_identity()
{
    m4 res;
    res.mm0 = _mm256_setr_ps(1.0f, 0.0f, 0.0f, 0.0f,
                             0.0f, 1.0f, 0.0f, 0.0f);
    res.mm1 = _mm256_setr_ps(0.0f, 0.0f, 1.0f, 0.0f,
                             0.0f, 0.0f, 0.0f, 1.0f);
    return res;
}

INLINE m4 MATH_CALL m4_scaling(f32 nx, f32 ny, f32 nz)
{
    m4 res;
    res.mm0 = _mm256_setr_ps(  nx, 0.0f, 0.0f, 0.0f,
                             0.0f,   ny, 0.0f, 0.0f);
    res.mm1 = _mm256_setr_ps(0.0f, 0.0f,   nz, 0.0f,
                             0.0f, 0.0f, 0.0f, 1.0f);
    return res;
}

INLINE m4 MATH_CALL m4_rotation_x(rad angle)
{
    f32 s = sinf(angle);
    f32 c = cosf(angle);

    m4 res;
    res.mm0 = _mm256_setr_ps(1.0f, 0.0f, 0.0f, 0.0f,
                             0.0f,    c,   -s, 0.0f);
    res.mm1 = _mm256_setr_ps(0.0f,    s,    c, 0.0f,
                             0.0f, 0.0f, 0.0f, 1.0f);
    return res;
}

INLINE m4 MATH_CALL m4_rotation_y(rad angle)
{
    f32 s = sinf(angle);
    f32 c = cosf(angle);

    m4 res;
    res.mm0 = _mm256_setr_ps(   c, 0.0f,    s, 0.0f,
                             0.0f, 1.0f, 0.0f, 0.0f);
    res.mm1 = _mm256_setr_ps(  -s, 0.0f,    c, 0.0f,
                             0.0f, 0.0f, 0.0f, 1.0f);
    return res;
}

INLINE m4 MATH_CALL m4_rotation_z(rad angle)
{
    f32 s = sinf(angle);
    f32 c = cosf(angle);

    m4 res;
    res.mm0 = _mm256_setr_ps(   c,   -s, 0.0f, 0.0f,
                                s,    c, 0.0f, 0.0f);
    res.mm1 = _mm256_setr_ps(0.0f, 0.0f, 1.0f, 0.0f,
                             0.0f, 0.0f, 0.0f, 1.0f);
    return res;
}

INLINE m4 MATH_CALL m4_rotation(v4 v, rad angle)
{
    // x*x*(1.0f-c)+  c     x*y*(1.0f-c)-z*s     x*z*(1.0-c)+y*s     0.0f
    // x*y*(1.0f-c)+z*s     y*y*(1.0f-c)+  c     y*z*(1.0-c)-x*s     0.0f
    // x*z*(1.0f-c)-y*s     z*y*(1.0f-c)+x*s     z*z*(1.0-c)+  c     0.0f
    //             0.0f                 0.0f                0.0f     1.0f

    f32 s = sinf(angle);
    f32 c = cosf(angle);

    __m128 col1 = _mm_setr_ps(v.x, v.x, v.x, 0.0f);
    __m128 col2 = _mm_setr_ps(v.x, v.y, v.y, 0.0f);
    __m128 col3 = _mm_setr_ps(v.x, v.y, v.z, 0.0f);

    col1 = _mm_mul_ps(col1, _mm_set_ps(v.x, v.y, v.z, 0.0f));
    col2 = _mm_mul_ps(col2, _mm_set_ps(v.y, v.y, v.z, 0.0f));
    col3 = _mm_mul_ps(col3, _mm_set_ps(v.z, v.z, v.z, 0.0f));

    __m128 invc = _mm_set_ps1(1.0f - c);
    invc.m128_f32[3] = 0.0f;

    col1 = _mm_mul_ps(col1, invc);
    col2 = _mm_mul_ps(col2, invc);
    col3 = _mm_mul_ps(col3, invc);

    __m128 m1 = _mm_setr_ps(1.0f,  v.z,  v.y, 0.0f);
    __m128 m2 = _mm_setr_ps(-v.z, 1.0f,  v.x, 0.0f);
    __m128 m3 = _mm_setr_ps( v.y, -v.x, 1.0f, 0.0f);

    m1 = _mm_mul_ps(m1, _mm_setr_ps(c, s, s, 0.0f));
    m2 = _mm_mul_ps(m2, _mm_setr_ps(s, c, s, 0.0f));
    m3 = _mm_mul_ps(m3, _mm_setr_ps(s, s, c, 0.0f));

    col1 = _mm_add_ps(col1, m1);
    col2 = _mm_add_ps(col2, m2);
    col3 = _mm_add_ps(col3, m3);

    m4 res;
    res.mm0 = _mm256_setr_ps(col1.m128_f32[0], col2.m128_f32[0], col3.m128_f32[0], 0.0f,
                             col1.m128_f32[1], col2.m128_f32[1], col3.m128_f32[1], 0.0f);
    res.mm1 = _mm256_setr_ps(col1.m128_f32[2], col2.m128_f32[2], col3.m128_f32[2], 0.0f,
                             col1.m128_f32[3], col2.m128_f32[3], col3.m128_f32[3], 1.0f);
    return res;
}

INLINE m4 MATH_CALL m4_reflection_x()
{
    m4 res;
    res.mm0 = _mm256_setr_ps(-1.0f, 0.0f, 0.0f, 0.0f,
                              0.0f, 1.0f, 0.0f, 0.0f);
    res.mm1 = _mm256_setr_ps( 0.0f, 0.0f, 1.0f, 0.0f,
                              0.0f, 0.0f, 0.0f, 1.0f);
    return res;
}

INLINE m4 MATH_CALL m4_reflection_y()
{
    m4 res;
    res.mm0 = _mm256_setr_ps(1.0f,  0.0f, 0.0f, 0.0f,
                             0.0f, -1.0f, 0.0f, 0.0f);
    res.mm1 = _mm256_setr_ps(0.0f,  0.0f, 1.0f, 0.0f,
                             0.0f,  0.0f, 0.0f, 1.0f);
    return res;
}

INLINE m4 MATH_CALL m4_reflection_z()
{
    m4 res;
    res.mm0 = _mm256_setr_ps(1.0f, 0.0f,  0.0f, 0.0f,
                             0.0f, 1.0f,  0.0f, 0.0f);
    res.mm1 = _mm256_setr_ps(0.0f, 0.0f, -1.0f, 0.0f,
                             0.0f, 0.0f,  0.0f, 1.0f);
    return res;
}

INLINE m4 MATH_CALL m4_reflection_xy()
{
    m4 res;
    res.mm0 = _mm256_setr_ps(-1.0f,  0.0f, 0.0f, 0.0f,
                              0.0f, -1.0f, 0.0f, 0.0f);
    res.mm1 = _mm256_setr_ps( 0.0f,  0.0f, 1.0f, 0.0f,
                              0.0f,  0.0f, 0.0f, 1.0f);
    return res;
}

INLINE m4 MATH_CALL m4_reflection_xz()
{
    m4 res;
    res.mm0 = _mm256_setr_ps(-1.0f, 0.0f,  0.0f, 0.0f,
                              0.0f, 1.0f,  0.0f, 0.0f);
    res.mm1 = _mm256_setr_ps( 0.0f, 0.0f, -1.0f, 0.0f,
                              0.0f, 0.0f,  0.0f, 1.0f);
    return res;
}

INLINE m4 MATH_CALL m4_reflection_yz()
{
    m4 res;
    res.mm0 = _mm256_setr_ps(1.0f,  0.0f,  0.0f, 0.0f,
                             0.0f, -1.0f,  0.0f, 0.0f);
    res.mm1 = _mm256_setr_ps(0.0f,  0.0f, -1.0f, 0.0f,
                             0.0f,  0.0f,  0.0f, 1.0f);
    return res;
}

INLINE m4 MATH_CALL m4_reflection_xyz()
{
    m4 res;
    res.mm0 = _mm256_setr_ps(-1.0f,  0.0f,  0.0f, 0.0f,
                              0.0f, -1.0f,  0.0f, 0.0f);
    res.mm1 = _mm256_setr_ps( 0.0f,  0.0f, -1.0f, 0.0f,
                              0.0f,  0.0f,  0.0f, 1.0f);
    return res;
}

INLINE m4 MATH_CALL m4_shearing_x(f32 shy, f32 shz)
{
    m4 res;
    res.mm0 = _mm256_setr_ps(1.0f, 0.0f, 0.0f, 0.0f,
                              shy, 1.0f, 0.0f, 0.0f);
    res.mm1 = _mm256_setr_ps( shz, 0.0f, 1.0f, 0.0f,
                             0.0f, 0.0f, 0.0f, 1.0f);
    return res;
}

INLINE m4 MATH_CALL m4_shearing_y(f32 shx, f32 shz)
{
    m4 res;
    res.mm0 = _mm256_setr_ps(1.0f,  shx, 0.0f, 0.0f,
                             0.0f, 1.0f, 0.0f, 0.0f);
    res.mm1 = _mm256_setr_ps(0.0f,  shz, 1.0f, 0.0f,
                             0.0f, 0.0f, 0.0f, 1.0f);
    return res;
}

INLINE m4 MATH_CALL m4_shearing_z(f32 shx, f32 shy)
{
    m4 res;
    res.mm0 = _mm256_setr_ps(1.0f, 0.0f,  shx, 0.0f,
                             0.0f, 1.0f,  shy, 0.0f);
    res.mm1 = _mm256_setr_ps(0.0f, 0.0f, 1.0f, 0.0f,
                             0.0f, 0.0f, 0.0f, 1.0f);
    return res;
}

INLINE m4 MATH_CALL m4_shearing(f32 shxy, f32 shxz, f32 shyx, f32 shyz, f32 shzx, f32 shzy)
{
    m4 res;
    res.mm0 = _mm256_setr_ps(1.0f, shyx, shzx, 0.0f,
                             shxy, 1.0f, shzy, 0.0f);
    res.mm1 = _mm256_setr_ps(shxz, shyz, 1.0f, 0.0f,
                             0.0f, 0.0f, 0.0f, 1.0f);
    return res;
}

INLINE m4 MATH_CALL m4_translation(f32 dx, f32 dy, f32 dz)
{
    m4 res;
    res.mm0 = _mm256_setr_ps(1.0f, 0.0f, 0.0f,   dx,
                             0.0f, 1.0f, 0.0f,   dy);
    res.mm1 = _mm256_setr_ps(0.0f, 0.0f, 1.0f,   dz,
                             0.0f, 0.0f, 0.0f, 1.0f);
    return res;
}

INLINE m4 MATH_CALL m4_translation_v(v4 dv)
{
    m4 res;
    res.mm0 = _mm256_setr_ps(1.0f, 0.0f, 0.0f, dv.x,
                             0.0f, 1.0f, 0.0f, dv.y);
    res.mm1 = _mm256_setr_ps(0.0f, 0.0f, 1.0f, dv.z,
                             0.0f, 0.0f, 0.0f, 1.0f);
    return res;
}

INLINE m4 MATH_CALL m4_ortho_lh(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far)
{
    m4 res;
    res.mm0 = _mm256_setr_ps(2.0f/(right-left),              0.0f,            0.0f, -(right+left)/(right-left),
                                          0.0f, 2.0f/(top-bottom),            0.0f, -(top+bottom)/(top-bottom));
    res.mm1 = _mm256_setr_ps(             0.0f,              0.0f, 1.0f/(far-near),           -near/(far-near),
                                          0.0f,              0.0f,            0.0f,                       1.0f);
    return res;
}

// @TODO(Roman): m4_ortho_rh
// ...

INLINE m4 MATH_CALL m4_persp_lh(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far)
{
    m4 res;
    res.mm0 = _mm256_setr_ps(2.0f*near/(right-left),                   0.0f,                  0.0f,                        0.0f,
                                               0.0f, 2.0f*near/(top-bottom),                  0.0f,                        0.0f);
    res.mm1 = _mm256_setr_ps(                  0.0f,                   0.0f, (far+near)/(far-near), -(2.0f*near*far)/(far-near),
                                               0.0f,                   0.0f,                  1.0f,                        0.0f);
    return res;
}

INLINE m4 MATH_CALL m4_persp_lh_fov(f32 aspect, deg fov, f32 near, f32 far)
{
    f32 tanfov2 = tanf(fov / 2.0f * f32_PI / 180.0f);

    m4 res;
    res.mm0 = _mm256_setr_ps(1.0f/(aspect*tanfov2),         0.0f,                  0.0f,                        0.0f,
                                              0.0f, 1.0f/tanfov2,                  0.0f,                        0.0f);
    res.mm1 = _mm256_setr_ps(                 0.0f,         0.0f, (far+near)/(far-near), -(2.0f*near*far)/(far-near),
                                              0.0f,         0.0f,                  1.0f,                        0.0f);
    return res;
}

INLINE m4 MATH_CALL m4_persp_rh(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far)
{
    m4 res;
    res.mm0 = _mm256_setr_ps(2.0f*near/(right-left),                   0.0f,                  0.0f,                       0.0f,
                                               0.0f, 2.0f*near/(top-bottom),                  0.0f,                       0.0f);
    res.mm1 = _mm256_setr_ps(                  0.0f,                   0.0f, (far+near)/(far-near), (2.0f*near*far)/(far-near),
                                               0.0f,                   0.0f,                 -1.0f,                       0.0f);
    return res;
}

INLINE m4 MATH_CALL m4_persp_rh_fov(f32 aspect, deg fov, f32 near, f32 far)
{
    f32 tanfov2 = tanf(fov / 2.0f * f32_PI / 180.0f);
    
    m4 res;
    res.mm0 = _mm256_setr_ps(1.0f/(aspect*tanfov2),         0.0f,                  0.0f,                       0.0f,
                                              0.0f, 1.0f/tanfov2,                  0.0f,                       0.0f);
    res.mm1 = _mm256_setr_ps(                 0.0f,         0.0f, (far+near)/(far-near), (2.0f*near*far)/(far-near),
                                              0.0f,         0.0f,                 -1.0f,                       0.0f);
    return res;
}
