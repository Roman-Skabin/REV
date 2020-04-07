//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "math/vec.h"

//
// m2
//

typedef union m2
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

INLINE m2 MATH_CALL m2_0(f32 val)                            { m2 m; m.mm = _mm_set_ps1(val); return m; }
INLINE m2 MATH_CALL m2_1(f32 e00, f32 e01, f32 e10, f32 e11) { m2 m; m.mm = _mm_setr_ps(e00, e01, e10, e11); return m; }
INLINE m2 MATH_CALL m2_2(v2 c1, v2 c2)                       { m2 m; m.mm = _mm_setr_ps(c1.x, c2.x, c1.y, c2.y); return m; }
INLINE m2 MATH_CALL m2_3(f32 arr[4])                         { m2 m; m.mm = _mm_loadu_ps(arr); return m; }

INLINE m2 MATH_CALL m2_add(m2 l, m2 r) { m2 m; m.mm = _mm_add_ps(l.mm, r.mm); return m; }
INLINE m2 MATH_CALL m2_sub(m2 l, m2 r) { m2 m; m.mm = _mm_sub_ps(l.mm, r.mm); return m; }

INLINE m2 MATH_CALL m2_mul(m2 l, m2 r)
{
    v2 r_c0 = v2_1(r.e00, r.e10);
    v2 r_c1 = v2_1(r.e01, r.e11);
    return m2_1(
        v2_dot(l.r0, r_c0), v2_dot(l.r0, r_c1),
        v2_dot(l.r1, r_c0), v2_dot(l.r1, r_c1)
    );
}

INLINE m2 MATH_CALL m2_add_s(f32 l, m2  r) { m2 m; m.mm = _mm_add_ps(_mm_set_ps1(l), r.mm); return m; }
INLINE m2 MATH_CALL m2_sub_s(m2  l, f32 r) { m2 m; m.mm = _mm_sub_ps(l.mm, _mm_set_ps1(r)); return m; }
INLINE m2 MATH_CALL m2_mul_s(f32 l, m2  r) { m2 m; m.mm = _mm_mul_ps(_mm_set_ps1(l), r.mm); return m; }

INLINE v2 MATH_CALL m2_mul_v(m2 l, v2 r) { return v2_1(v2_dot(l.r0, r), v2_dot(l.r1, r)); }

INLINE f32 MATH_CALL m2_det(m2 m) { return m.e00 * m.e11 - m.e01 * m.e10; }

INLINE m2 MATH_CALL m2_inverse(m2 m)
{
    m2 i;
    i.mm = _mm_mul_ps(_mm_set_ps1(1.0f / m2_det(m)), _mm_shuffle_ps(m.mm, m.mm, _MM_SHUFFLE(0, 2, 1, 3)));
    i.e01 *= -1;
    i.e10 *= -1;
    return i;
}

INLINE m2 MATH_CALL m2_transpose(m2 m)
{
    m2 t;
    t.mm = _mm_shuffle_ps(m.mm, m.mm, _MM_SHUFFLE(3, 1, 2, 0));
    return t;
}

INLINE m2 MATH_CALL m2_identity() { return m2_1(1.0f, 0.0f, 0.0f, 1.0f); }

INLINE m2 MATH_CALL m2_scaling(f32 nx, f32 ny) { return m2_1(nx, 0.0f, 0.0f, ny); }

INLINE m2 MATH_CALL m2_rotation_z(rad angle)
{
    f32 s = sinf(angle);
    f32 c = cosf(angle);
    return m2_1(c, -s, s, c);
}

INLINE m2 MATH_CALL m2_rotation(v2 v, rad angle)
{
    f32 s = sinf(angle);
    f32 c = cosf(angle);
    m2 m;
    m.mm = _mm_add_ps(
        _mm_mul_ps(
            _mm_mul_ps(
                _mm_setr_ps(v.x, v.x, v.x, v.y),
                _mm_setr_ps(v.x, v.y, v.y, v.y)
            ),
            _mm_set_ps1(1.0f - c)
        ),
        _mm_setr_ps(c, 0.0f, 0.0f, c)
    );
    return m;
}

INLINE m2 MATH_CALL m2_reflection_x() { return m2_1(-1.0f, 0.0f, 0.0f,  1.0f); }
INLINE m2 MATH_CALL m2_reflection_y() { return m2_1( 1.0f, 0.0f, 0.0f, -1.0f); }

INLINE m2 MATH_CALL m2_shearing(f32 shx, f32 shy) { return m2_1(1.0f, shx, shy, 1.0f); }

//
// m3
//

typedef union m3
{
    struct
    {
        f32 e00, e01, e02, __reserved0;
        f32 e10, e11, e12, __reserved1;
        f32 e20, e21, e22, __reserved2;
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

INLINE m3 MATH_CALL m3_0(f32 val)
{
    m3 m;
    m.mm0 = _mm256_set1_ps(val);
    m.mm1 = _mm_set_ps1(val);
    m.__reserved0 = 0.0f;
    m.__reserved1 = 0.0f;
    m.__reserved2 = 0.0f;
    return m;
}

INLINE m3 MATH_CALL m3_1(
    f32 e00, f32 e01, f32 e02,
    f32 e10, f32 e11, f32 e12,
    f32 e20, f32 e21, f32 e22)
{
    m3 m;
    m.mm0 = _mm256_setr_ps(
        e00, e01, e02, 0.0f,
        e10, e11, e12, 0.0f
    );
    m.mm1 = _mm_setr_ps(
        e20, e21, e22, 0.0f
    );
    return m;
}

INLINE m3 MATH_CALL m3_2(f32 arr[9])
{
    m3 m;
    m.mm0 = _mm256_setr_ps(
        arr[0], arr[1], arr[2], 0.0f,
        arr[3], arr[4], arr[5], 0.0f
    );
    m.mm1 = _mm_setr_ps(
        arr[6], arr[7], arr[8], 0.0f
    );
    return m;
}

INLINE m3 MATH_CALL m3_3(v3 c1, v3 c2, v3 c3)
{
    m3 m;
    m.mm0 = _mm256_setr_ps(
        c1.x, c2.x, c3.x, 0.0f,
        c1.y, c2.y, c3.y, 0.0f
    );
    m.mm1 = _mm_setr_ps(
        c1.z, c2.z, c3.z, 0.0f
    );
    return m;
}

INLINE m3 MATH_CALL m3_add(m3 l, m3 r)
{
    m3 m;
    m.mm0 = _mm256_add_ps(l.mm0, r.mm0);
    m.mm1 = _mm_add_ps(l.mm1, r.mm1);
    return m;
}

INLINE m3 MATH_CALL m3_sub(m3 l, m3 r)
{
    m3 m;
    m.mm0 = _mm256_sub_ps(l.mm0, r.mm0);
    m.mm1 = _mm_sub_ps(l.mm1, r.mm1);
    return m;
}

INLINE m3 MATH_CALL m3_transpose(m3 m)
{
    return m3_1(
        m.e00, m.e10, m.e20,
        m.e01, m.e11, m.e21,
        m.e02, m.e12, m.e22
    );
}

INLINE m3 MATH_CALL m3_mul(m3 l, m3 r)
{
    v3 r_c0 = v3_1(r.e00, r.e10, r.e20);
    v3 r_c1 = v3_1(r.e01, r.e11, r.e21);
    v3 r_c2 = v3_1(r.e02, r.e12, r.e22);
    return m3_1(
        v3_dot(l.r0, r_c0), v3_dot(l.r0, r_c1), v3_dot(l.r0, r_c2),
        v3_dot(l.r1, r_c0), v3_dot(l.r1, r_c1), v3_dot(l.r1, r_c2),
        v3_dot(l.r2, r_c0), v3_dot(l.r2, r_c1), v3_dot(l.r2, r_c2)
    );
}

INLINE m3 MATH_CALL m3_add_s(f32 l, m3  r)
{
    m3 _l = m3_0(l);
    m3 m;
    m.mm0 = _mm256_add_ps(_l.mm0, r.mm0);
    m.mm1 = _mm_add_ps(_l.mm1, r.mm1);
    return m;
}

INLINE m3 MATH_CALL m3_sub_s(m3  l, f32 r)
{
    m3 _r = m3_0(r);
    m3 m;
    m.mm0 = _mm256_sub_ps(l.mm0, _r.mm0);
    m.mm1 = _mm_sub_ps(l.mm1, _r.mm1);
    return m;
}

INLINE m3 MATH_CALL m3_mul_s(f32 l, m3  r)
{
    m3 _l = m3_0(l);
    m3 m;
    m.mm0 = _mm256_mul_ps(_l.mm0, r.mm0);
    m.mm1 = _mm_mul_ps(_l.mm1, r.mm1);
    return m;
}

INLINE v3 MATH_CALL m3_mul_v(m3 l, v3 r)
{
    return v3_1(
        v3_dot(l.r0, r),
        v3_dot(l.r1, r),
        v3_dot(l.r2, r)
    );
}

INLINE f32 MATH_CALL m3_det(m3 m)
{
#if 1
    __m128 a = _mm_mul_ps(
                   _mm_setr_ps(m.e11, m.e10, m.e10, 0.0f),
                   _mm_setr_ps(m.e22, m.e22, m.e21, 0.0f));

    __m128 b = _mm_mul_ps(
                   _mm_setr_ps(m.e12, m.e12, m.e11, 0.0f),
                   _mm_setr_ps(m.e21, m.e20, m.e20, 0.0f));

    __m128 c = _mm_mul_ps(
                   _mm_setr_ps(m.e00, m.e01, m.e02, 0.0f),
                   _mm_sub_ps(a, b));

    return c.m128_f32[0] - c.m128_f32[1] + c.m128_f32[2];
#else
    return m.e00 * (m.e11 * m.e22 - m.e12 * m.e21)
         - m.e01 * (m.e10 * m.e22 - m.e12 * m.e20)
         + m.e02 * (m.e10 * m.e21 - m.e11 * m.e20);
#endif
}

INLINE m3 MATH_CALL m3_inverse(m3 m)
{
    // @Optimize(Roman): faster solution (gaussian eliminations maybe?)
    return m3_mul_s(1.0f / m3_det(m), m3_1(
        m2_det(m2_1(m.e11, m.e12, m.e21, m.e22)), m2_det(m2_1(m.e02, m.e01, m.e22, m.e21)), m2_det(m2_1(m.e01, m.e02, m.e11, m.e12)),
        m2_det(m2_1(m.e12, m.e10, m.e22, m.e20)), m2_det(m2_1(m.e00, m.e02, m.e20, m.e22)), m2_det(m2_1(m.e02, m.e00, m.e12, m.e10)),
        m2_det(m2_1(m.e10, m.e11, m.e20, m.e21)), m2_det(m2_1(m.e01, m.e00, m.e21, m.e20)), m2_det(m2_1(m.e00, m.e01, m.e10, m.e11))
    ));
}

INLINE m3 MATH_CALL m3_identity()
{
    return m3_1(
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 1.0f
    );
}

INLINE m3 MATH_CALL m3_scaling(f32 nx, f32 ny)
{
    return m3_1(
          nx, 0.0f, 0.0f,
        0.0f,   ny, 0.0f,
        0.0f, 0.0f, 1.0f
    );
}

INLINE m3 MATH_CALL m3_rotation_z(rad angle)
{
    f32 s = sinf(angle);
    f32 c = cosf(angle);
    return m3_1(
           c,   -s, 0.0f,
           s,    c, 0.0f,
        0.0f, 0.0f, 1.0f
    );
}

INLINE m3 MATH_CALL m3_rotation(v3 v, rad angle)
{
    f32 s = sinf(angle);
    f32 c = cosf(angle);
    __m128 mm = _mm_add_ps(
        _mm_mul_ps(
            _mm_mul_ps(
                _mm_setr_ps(v.x, v.x, v.x, v.y),
                _mm_setr_ps(v.x, v.y, v.y, v.y)
            ),
            _mm_set_ps1(1.0f - c)
        ),
        _mm_setr_ps(c, 0.0f, 0.0f, c)
    );
    return m3_1(
        mm.m128_f32[0], mm.m128_f32[1], 0.0f,
        mm.m128_f32[2], mm.m128_f32[3], 0.0f,
                  0.0f,           0.0f, 1.0f
    );
}

INLINE m3 MATH_CALL m3_reflection_x() { return m3_1(-1.0f, 0.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f, 0.0f, 1.0f); }
INLINE m3 MATH_CALL m3_reflection_y() { return m3_1( 1.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f); }

INLINE m3 MATH_CALL m3_shearing(f32 shx, f32 shy) { return m3_1(1.0f, shx, 0.0f, shy, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f); }

INLINE m3 MATH_CALL m3_translation(f32 dx, f32 dy) { return m3_1(1.0f, 0.0f,  dx, 0.0f, 1.0f,  dy, 0.0f, 0.0f, 1.0f); }
INLINE m3 MATH_CALL m3_translation_v(v3 v)         { return m3_1(1.0f, 0.0f, v.x, 0.0f, 1.0f, v.y, 0.0f, 0.0f, 1.0f); }

INLINE m3 MATH_CALL m3_ortho(f32 left, f32 right, f32 bottom, f32 top)
{
    return m3_1(
        2.0f/(right-left),              0.0f, -(right+left)/(right-left),
                     0.0f, 2.0f/(top-bottom), -(top+bottom)/(top-bottom),
                     0.0f,              0.0f,                       1.0f
    );
}

//
// m4
//

typedef union m4
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

INLINE m4 MATH_CALL m4_0(f32 val)
{
    m4 m;
    m.mm0 = _mm256_set1_ps(val);
    m.mm1 = _mm256_set1_ps(val);
    return m;
}

INLINE m4 MATH_CALL m4_1(
    f32 e00, f32 e01, f32 e02, f32 e03,
    f32 e10, f32 e11, f32 e12, f32 e13,
    f32 e20, f32 e21, f32 e22, f32 e23,
    f32 e30, f32 e31, f32 e32, f32 e33)
{
    m4 m;
    m.mm0 = _mm256_setr_ps(e00, e01, e02, e03, e10, e11, e12, e13);
    m.mm1 = _mm256_setr_ps(e20, e21, e22, e23, e30, e31, e32, e33);
    return m;
}

INLINE m4 MATH_CALL m4_2(v4 c0, v4 c1, v4 c2, v4 c3)
{
    m4 m;
    m.mm0 = _mm256_setr_ps(
        c0.x, c1.x, c2.x, c3.x,
        c0.y, c1.y, c2.y, c3.y
    );
    m.mm1 = _mm256_setr_ps(
        c0.z, c1.z, c2.z, c3.z,
        c0.w, c1.w, c2.w, c3.w
    );
    return m;
}

INLINE m4 MATH_CALL m4_3(f32 arr[16])
{
    m4 m;
    m.mm0 = _mm256_loadu_ps(arr    );
    m.mm1 = _mm256_loadu_ps(arr + 8);
    return m;
}

INLINE m4 MATH_CALL m4_4(__m128 c0, __m128 c1, __m128 c2, __m128 c3)
{
    m4 m;
    m.mm0 = _mm256_setr_ps(
        c0.m128_f32[0], c1.m128_f32[0], c2.m128_f32[0], c3.m128_f32[0],
        c0.m128_f32[1], c1.m128_f32[1], c2.m128_f32[1], c3.m128_f32[1]
    );
    m.mm1 = _mm256_setr_ps(
        c0.m128_f32[2], c1.m128_f32[2], c2.m128_f32[2], c3.m128_f32[2],
        c0.m128_f32[3], c1.m128_f32[3], c2.m128_f32[3], c3.m128_f32[3]
    );
    return m;
}

INLINE m4 MATH_CALL m4_add(m4 l, m4 r)
{
    m4 m;
    m.mm0 = _mm256_add_ps(l.mm0, r.mm0);
    m.mm1 = _mm256_add_ps(l.mm1, r.mm1);
    return m;
}

INLINE m4 MATH_CALL m4_sub(m4 l, m4 r)
{
    m4 m;
    m.mm0 = _mm256_sub_ps(l.mm0, r.mm0);
    m.mm1 = _mm256_sub_ps(l.mm1, r.mm1);
    return m;
}

INLINE m4 MATH_CALL m4_transpose(m4 m)
{
    return m4_1(
        m.e00, m.e10, m.e20, m.e30,
        m.e01, m.e11, m.e21, m.e31,
        m.e02, m.e12, m.e22, m.e32,
        m.e03, m.e13, m.e23, m.e33
    );
}

INLINE m4 MATH_CALL m4_mul(m4 l, m4 r)
{
    m4 t = m4_transpose(r);
    return m4_1(
        v4_dot(l.r0, t.r0), v4_dot(l.r0, t.r1), v4_dot(l.r0, t.r2), v4_dot(l.r0, t.r3),
        v4_dot(l.r1, t.r0), v4_dot(l.r1, t.r1), v4_dot(l.r1, t.r2), v4_dot(l.r1, t.r3),
        v4_dot(l.r2, t.r0), v4_dot(l.r2, t.r1), v4_dot(l.r2, t.r2), v4_dot(l.r2, t.r3),
        v4_dot(l.r3, t.r0), v4_dot(l.r3, t.r1), v4_dot(l.r3, t.r2), v4_dot(l.r3, t.r3)
    );
}

INLINE m4 MATH_CALL m4_add_s(f32 l, m4 r)
{
    m4 m;
    __m256 mm = _mm256_set1_ps(l);
    m.mm0 = _mm256_add_ps(mm, r.mm0);
    m.mm1 = _mm256_add_ps(mm, r.mm1);
    return m;
}

INLINE m4 MATH_CALL m4_sub_s(m4 l, f32 r)
{
    m4 m;
    __m256 mm = _mm256_set1_ps(r);
    m.mm0 = _mm256_sub_ps(l.mm0, mm);
    m.mm1 = _mm256_sub_ps(l.mm1, mm);
    return m;
}

INLINE m4 MATH_CALL m4_mul_s(f32 l, m4 r)
{
    m4 m;
    __m256 mm = _mm256_set1_ps(l);
    m.mm0 = _mm256_mul_ps(mm, r.mm0);
    m.mm1 = _mm256_mul_ps(mm, r.mm1);
    return m;
}

INLINE v4 MATH_CALL m4_mul_v(m4 l, v4 r)
{
    return v4_1(
        v4_dot(l.r0, r),
        v4_dot(l.r1, r),
        v4_dot(l.r2, r),
        v4_dot(l.r3, r)
    );
}

INLINE f32 MATH_CALL m4_det(m4 m)
{
    // @Optimize(Roman): maybe there is a way to perform it via mm intrinsics?
    return m.e00 * (m.e11 * (m.e22 * m.e33 - m.e23 * m.e32)
                  + m.e12 * (m.e21 * m.e33 - m.e23 * m.e31)
                  - m.e13 * (m.e21 * m.e32 - m.e22 * m.e31))
         - m.e01 * (m.e10 * (m.e22 * m.e33 - m.e23 * m.e32)
                  + m.e12 * (m.e20 * m.e33 - m.e23 * m.e30)
                  - m.e13 * (m.e20 * m.e32 - m.e22 * m.e30))
         + m.e02 * (m.e10 * (m.e21 * m.e33 - m.e23 * m.e31)
                  + m.e11 * (m.e20 * m.e33 - m.e23 * m.e30)
                  - m.e13 * (m.e20 * m.e31 - m.e21 * m.e30))
         - m.e03 * (m.e10 * (m.e21 * m.e32 - m.e22 * m.e31)
                  + m.e11 * (m.e20 * m.e32 - m.e22 * m.e30)
                  - m.e12 * (m.e20 * m.e31 - m.e21 * m.e30));
}

INLINE m4 MATH_CALL m4_inverse(m4 m)
{
    // @Optimize(Roman): faster solution (gaussian eliminations maybe?)
    f32 c00 = m3_det(m3_1(m.e11, m.e12, m.e13,
                          m.e21, m.e22, m.e23,
                          m.e31, m.e32, m.e33
    ));
    f32 c01 = m3_det(m3_1(m.e10, m.e12, m.e13,
                          m.e20, m.e22, m.e23,
                          m.e30, m.e32, m.e33
    ));
    f32 c02 = m3_det(m3_1(m.e10, m.e11, m.e13,
                          m.e20, m.e21, m.e23,
                          m.e30, m.e31, m.e33
    ));
    f32 c03 = m3_det(m3_1(m.e10, m.e11, m.e12,
                          m.e20, m.e21, m.e22,
                          m.e30, m.e31, m.e32
    ));
    f32 c10 = m3_det(m3_1(m.e01, m.e02, m.e03,
                          m.e21, m.e22, m.e23,
                          m.e31, m.e32, m.e33
    ));
    f32 c11 = m3_det(m3_1(m.e00, m.e02, m.e03,
                          m.e20, m.e22, m.e23,
                          m.e30, m.e32, m.e33
    ));
    f32 c12 = m3_det(m3_1(m.e00, m.e01, m.e03,
                          m.e20, m.e21, m.e23,
                          m.e30, m.e31, m.e33
    ));
    f32 c13 = m3_det(m3_1(m.e00, m.e01, m.e02,
                          m.e20, m.e21, m.e22,
                          m.e30, m.e31, m.e32
    ));
    f32 c20 = m3_det(m3_1(m.e01, m.e02, m.e03,
                          m.e11, m.e12, m.e13,
                          m.e31, m.e32, m.e33
    ));
    f32 c21 = m3_det(m3_1(m.e00, m.e02, m.e03,
                          m.e10, m.e12, m.e13,
                          m.e30, m.e32, m.e33
    ));
    f32 c22 = m3_det(m3_1(m.e00, m.e01, m.e03,
                          m.e10, m.e11, m.e13,
                          m.e30, m.e31, m.e33
    ));
    f32 c23 = m3_det(m3_1(m.e00, m.e01, m.e02,
                          m.e10, m.e11, m.e12,
                          m.e30, m.e31, m.e32
    ));
    f32 c30 = m3_det(m3_1(m.e01, m.e02, m.e03,
                          m.e11, m.e12, m.e13,
                          m.e21, m.e22, m.e23
    ));
    f32 c31 = m3_det(m3_1(m.e00, m.e02, m.e03,
                          m.e10, m.e12, m.e13,
                          m.e20, m.e22, m.e23
    ));
    f32 c32 = m3_det(m3_1(m.e00, m.e01, m.e03,
                          m.e10, m.e11, m.e13,
                          m.e20, m.e21, m.e23
    ));
    f32 c33 = m3_det(m3_1(m.e00, m.e01, m.e02,
                          m.e10, m.e11, m.e12,
                          m.e20, m.e21, m.e22
    ));
    return m4_mul_s(1.0f / m4_det(m), m4_1(
        c00, c01, c02, c03,
        c10, c11, c12, c13,
        c20, c21, c22, c23,
        c30, c31, c32, c33
    ));
}

INLINE m4 MATH_CALL m4_identity()
{
    return m4_1(
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );
}

INLINE m4 MATH_CALL m4_scaling(f32 nx, f32 ny, f32 nz)
{
    return m4_1(
          nx, 0.0f, 0.0f, 0.0f,
        0.0f,   ny, 0.0f, 0.0f,
        0.0f, 0.0f,   nz, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );
}

INLINE m4 MATH_CALL m4_rotation_x(rad angle)
{
    f32 s = sinf(angle);
    f32 c = cosf(angle);
    return m4_1(
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f,    c,   -s, 0.0f,
        0.0f,    s,    c, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );
}

INLINE m4 MATH_CALL m4_rotation_y(rad angle)
{
    f32 s = sinf(angle);
    f32 c = cosf(angle);
    return m4_1(
           c, 0.0f,    s, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
          -s, 0.0f,    c, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );
}

INLINE m4 MATH_CALL m4_rotation_z(rad angle)
{
    f32 s = sinf(angle);
    f32 c = cosf(angle);
    return m4_1(
           c,   -s, 0.0f, 0.0f,
           s,    c, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );
}

INLINE m4 MATH_CALL m4_rotation(v4 v, rad angle)
{
    f32 s = sinf(angle);
    f32 c = cosf(angle);
#if 1
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

    return m4_4(col1, col2, col3, _mm_setr_ps(0.0f, 0.0f, 0.0f, 1.0f));

#else
    return m4_1(
        v.x*v.x*(1-c)+    c, v.x*v.y*(1-c)-v.z*s, v.x*v.z*(1-c)+v.y*s, 0.0f,
        v.x*v.y*(1-c)+v.z*s, v.y*v.y*(1-c)+    c, v.y*v.z*(1-c)-v.x*s, 0.0f,
        v.x*v.z*(1-c)+v.y*s, v.y*v.z*(1-c)+v.x*s, v.z*v.z*(1-c)+    c, 0.0f,
                       0.0f,                0.0f,                0.0f, 1.0f
    );
#endif
}

INLINE m4 MATH_CALL m4_reflection_x()
{
    return m4_1(
        -1.0f, 0.0f, 0.0f, 0.0f,
         0.0f, 1.0f, 0.0f, 0.0f,
         0.0f, 0.0f, 1.0f, 0.0f,
         0.0f, 0.0f, 0.0f, 1.0f
    );
}

INLINE m4 MATH_CALL m4_reflection_y()
{
    return m4_1(
        1.0f,  0.0f, 0.0f, 0.0f,
        0.0f, -1.0f, 0.0f, 0.0f,
        0.0f,  0.0f, 1.0f, 0.0f,
        0.0f,  0.0f, 0.0f, 1.0f
    );
}

INLINE m4 MATH_CALL m4_reflection_z()
{
    return m4_1(
         1.0f, 0.0f,  0.0f, 0.0f,
         0.0f, 1.0f,  0.0f, 0.0f,
         0.0f, 0.0f, -1.0f, 0.0f,
         0.0f, 0.0f,  0.0f, 1.0f
    );
}

INLINE m4 MATH_CALL m4_shearing_x(f32 shy, f32 shz)
{
    return m4_1(
        1.0f, 0.0f, 0.0f, 0.0f,
         shy, 1.0f, 0.0f, 0.0f,
         shz, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );
}

INLINE m4 MATH_CALL m4_shearing_y(f32 shx, f32 shz)
{
    return m4_1(
        1.0f,  shx, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f,  shz, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );
}

INLINE m4 MATH_CALL m4_shearing_z(f32 shx, f32 shy)
{
    return m4_1(
        1.0f, 0.0f,  shx, 0.0f,
        0.0f, 1.0f,  shy, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );
}

INLINE m4 MATH_CALL m4_shearing(f32 shxy, f32 shxz, f32 shyx, f32 shyz, f32 shzx, f32 shzy)
{
    return m4_1(
        1.0f, shyx, shzx, 0.0f,
        shxy, 1.0f, shzy, 0.0f,
        shxz, shyz, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );
}

INLINE m4 MATH_CALL m4_translation(f32 dx, f32 dy, f32 dz)
{
    return m4_1(
        1.0f, 0.0f, 0.0f,   dx,
        0.0f, 1.0f, 0.0f,   dy,
        0.0f, 0.0f, 1.0f,   dz,
        0.0f, 0.0f, 0.0f, 1.0f
    );
}

INLINE m4 MATH_CALL m4_translation_v(v4 dv)
{
    return m4_1(
        1.0f, 0.0f, 0.0f, dv.x,
        0.0f, 1.0f, 0.0f, dv.y,
        0.0f, 0.0f, 1.0f, dv.z,
        0.0f, 0.0f, 0.0f, 1.0f
    );
}

// @TODO(Roman):
// m4_ortho_lh_n0 (near > 0)
// m4_ortho_lh_nn
// m4_ortho_rh_n0 (near > 0)
// m4_ortho_rh_nn
INLINE m4 MATH_CALL m4_ortho_lh_n0(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far)
{
    return m4_1(
        2.0f/(right-left),              0.0f,            0.0f, -(right+left)/(right-left),
                     0.0f, 2.0f/(top-bottom),            0.0f, -(top+bottom)/(top-bottom),
                     0.0f,              0.0f, 1.0f/(far-near),           -near/(far-near),
                     0.0f,              0.0f,            0.0f,                       1.0f
    );
}

// @TODO(Roman):
// m4_persp_lh_n0     (near > 0)
// m4_persp_lh_nn
// m4_persp_rh_n0     (near > 0)
// m4_persp_rh_nn
// m4_persp_lh_n0_fov (near > 0)
// m4_persp_lh_nn_fov
// m4_persp_rh_n0_fov (near > 0)
// m4_persp_rh_nn_fov
INLINE m4 MATH_CALL m4_persp_lh_n0(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far)
{
    return m4_1(
        2.0f*near/(right-left),                   0.0f, (right+left)/(right-left),                   0.0f,
                          0.0f, 2.0f*near/(top-bottom), (top+bottom)/(top-bottom),                   0.0f,
                          0.0f,                   0.0f,            far/(far-near), -(far*near)/(far-near),
                          0.0f,                   0.0f,                      1.0f,                   0.0f
    );
}

INLINE m4 MATH_CALL m4_persp_lh_n0_fov(f32 aspect, deg fov, f32 near, f32 far)
{
    f32 tanfov2 = tanf(fov / 2.0f * f32_PI / 180.0f);
    return m4_1(
        1.0f/(aspect*tanfov2),         0.0f,           0.0f,                   0.0f,
                         0.0f, 1.0f/tanfov2,           0.0f,                   0.0f,
                         0.0f,         0.0f, far/(far-near), -(far*near)/(far-near),
                         0.0f,         0.0f,           1.0f,                   0.0f
    );
}
