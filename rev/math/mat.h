//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "math/vec.h"

#pragma pack(push, 1)
namespace REV::Math
{

//
// m2
//

union REV_INTRIN_TYPE REV_ALIGN(16) m2 final
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

    m2(                                  ) : mm(_mm_setzero_ps())                    {}
    m2(f32 val                           ) : mm(_mm_set_ps1(val))                    {}
    m2(f32 e00, f32 e01, f32 e10, f32 e11) : mm(_mm_setr_ps(e00, e01, e10, e11))     {}
    m2(v2 c1, v2 c2                      ) : mm(_mm_setr_ps(c1.x, c2.x, c1.y, c2.y)) {}
    m2(f32 arr[4]                        ) : mm(_mm_load_ps(arr))                    {}
    m2(__m128 _mm                        ) : mm(_mm)                                 {}
    m2(const m2& m                       ) : mm(m.mm)                                {}
    m2(m2&& m                            ) : mm(m.mm)                                {}

    m2& __vectorcall operator=(f32 val    ) { mm = _mm_set_ps1(val); return *this; }
    m2& __vectorcall operator=(f32 arr[4] ) { mm = _mm_load_ps(arr); return *this; }
    m2& __vectorcall operator=(__m128 _mm ) { mm = _mm;              return *this; }
    m2& __vectorcall operator=(const m2& m) { mm = m.mm;             return *this; }
    m2& __vectorcall operator=(m2&& m     ) { mm = m.mm;             return *this; }

    f32 __vectorcall det() const
    {
        return e00 * e11 - e01 * e10;
    }

    m2 __vectorcall inverse() const
    {
        __m128 vdet = _mm_set_ps1(e00 * e11 - e01 * e10);
        __m128 mult = _mm_setr_ps(1.0f, -1.0f, -1.0f, 1.0f);
        __m128 cof  = _mm_shuffle_ps(mm, mm, cast<s32>(MM_SHUFFLE::WYZX));
        return m2(_mm_div_ps(_mm_mul_ps(cof, mult), vdet));
    }

    m2 __vectorcall transpose() const
    {
        return m2(_mm_shuffle_ps(mm, mm, cast<s32>(MM_SHUFFLE::XZYW)));
    }

    static m2 __vectorcall identity()
    {
        return m2(1.0f, 0.0f,
                  0.0f, 1.0f);
    }

    static m2 __vectorcall scaling(f32 nx, f32 ny)
    {
        return m2(  nx, 0.0f,
                  0.0f,   ny);
    }

    static m2 __vectorcall rotation_z(rad angle)
    {
        f32 s = sinf(angle);
        f32 c = cosf(angle);
        return m2(c, -s,
                  s, c);
    }

    static m2 __vectorcall reflection_x()
    {
        return m2(-1.0f, 0.0f,
                   0.0f, 1.0f);
    }

    static m2 __vectorcall reflection_y()
    {
        return m2(1.0f,  0.0f,
                  0.0f, -1.0f);
    }

    static m2 __vectorcall reflection_xy()
    {
        return m2(-1.0f,  0.0f,
                   0.0f, -1.0f);
    }

    static m2 __vectorcall shearing(f32 shx, f32 shy)
    {
        return m2(1.0f,  shx,
                   shy, 1.0f);
    }

    m2& __vectorcall operator+=(m2 r) { mm = _mm_add_ps(mm, r.mm); return *this; }
    m2& __vectorcall operator-=(m2 r) { mm = _mm_sub_ps(mm, r.mm); return *this; }

    m2& __vectorcall operator*=(m2 r)
    {
        // e00 = l.e00 * r.e00 + l.e01 * r.e10;
        // e01 = l.e00 * r.e01 + l.e01 * r.e11;
        // e10 = l.e10 * r.e00 + l.e11 * r.e10;
        // e11 = l.e10 * r.e01 + l.e11 * r.e11;

        __m128 col_1 = _mm_shuffle_ps(  mm,   mm, cast<s32>(MM_SHUFFLE::XXZZ));
        __m128 col_2 = _mm_shuffle_ps(r.mm, r.mm, cast<s32>(MM_SHUFFLE::XYXY));
        __m128 col_3 = _mm_shuffle_ps(  mm,   mm, cast<s32>(MM_SHUFFLE::YYWW));
        __m128 col_4 = _mm_shuffle_ps(r.mm, r.mm, cast<s32>(MM_SHUFFLE::ZWZW));

        __m128 left  = _mm_mul_ps(col_1, col_2);
        __m128 right = _mm_mul_ps(col_3, col_4);

        mm = _mm_add_ps(left, right);
        return *this;
    }

    m2& __vectorcall operator+=(f32 r) { mm = _mm_add_ps(mm, _mm_set_ps1(r)); return *this; }
    m2& __vectorcall operator-=(f32 r) { mm = _mm_sub_ps(mm, _mm_set_ps1(r)); return *this; }
    m2& __vectorcall operator*=(f32 r) { mm = _mm_mul_ps(mm, _mm_set_ps1(r)); return *this; }
    m2& __vectorcall operator/=(f32 r) { mm = _mm_div_ps(mm, _mm_set_ps1(r)); return *this; }

    v2  operator[](u8 index) const { return index == 0 ? r0 : r1; }
    v2& operator[](u8 index)       { return index == 0 ? r0 : r1; }
};

REV_INLINE m2 __vectorcall operator+(m2 l, m2 r) { return m2(_mm_add_ps(l.mm, r.mm)); }
REV_INLINE m2 __vectorcall operator-(m2 l, m2 r) { return m2(_mm_sub_ps(l.mm, r.mm)); }

REV_INLINE m2 __vectorcall operator*(m2 l, m2 r)
{
    // e00 = l.e00 * r.e00 + l.e01 * r.e10;
    // e01 = l.e00 * r.e01 + l.e01 * r.e11;
    // e10 = l.e10 * r.e00 + l.e11 * r.e10;
    // e11 = l.e10 * r.e01 + l.e11 * r.e11;

    __m128 col_1 = _mm_shuffle_ps(l.mm, l.mm, cast<s32>(MM_SHUFFLE::XXZZ));
    __m128 col_2 = _mm_shuffle_ps(r.mm, r.mm, cast<s32>(MM_SHUFFLE::XYXY));
    __m128 col_3 = _mm_shuffle_ps(l.mm, l.mm, cast<s32>(MM_SHUFFLE::YYWW));
    __m128 col_4 = _mm_shuffle_ps(r.mm, r.mm, cast<s32>(MM_SHUFFLE::ZWZW));

    __m128 left  = _mm_mul_ps(col_1, col_2);
    __m128 right = _mm_mul_ps(col_3, col_4);

    return m2(_mm_add_ps(left, right));
}

REV_INLINE m2 __vectorcall operator+(f32 l, m2 r) { return m2(_mm_add_ps(_mm_set_ps1(l), r.mm)); }
REV_INLINE m2 __vectorcall operator-(m2 l, f32 r) { return m2(_mm_sub_ps(l.mm, _mm_set_ps1(r))); }
REV_INLINE m2 __vectorcall operator*(f32 l, m2 r) { return m2(_mm_mul_ps(_mm_set_ps1(l), r.mm)); }
REV_INLINE m2 __vectorcall operator/(m2 l, f32 r) { return m2(_mm_div_ps(l.mm, _mm_set_ps1(r))); }

REV_INLINE v2 __vectorcall operator*(m2 l, v2 r)
{
    // x = l.e00 * r.x
    //   + l.e01 * r.y;
    // y = l.e10 * r.x
    //   + l.e11 * r.y;

    __m128 vec  = _mm_setr_ps(r.x, r.y, r.x, r.y);
    __m128 cols = _mm_mul_ps(l.mm, vec);
    __m128 vres = _mm_hadd_ps(cols, cols);

    return v2(mm_extract_f32<0>(vres), mm_extract_f32<1>(vres));
}

REV_INLINE bool __vectorcall operator==(m2 l, m2 r)
{
#if REV_ISA >= REV_ISA_AVX512
    return 0b11 == _mm_cmpeq_epi64_mask(*cast<__m128i *>(&l.mm),
                                        *cast<__m128i *>(&r.mm));
#else
    __m128 eq_mask = _mm_cmpeq_ps(l.mm, r.mm);
    __m128 ones    = _mm_cmpeq_ps(eq_mask, eq_mask);
    return _mm_testc_ps(eq_mask, ones);
#endif
}

REV_INLINE bool __vectorcall operator!=(m2 l, m2 r)
{
#if REV_ISA >= REV_ISA_AVX512
    return 0b11 != _mm_cmpeq_epi64_mask(*cast<__m128i *>(&l.mm),
                                        *cast<__m128i *>(&r.mm));
#else
    __m128 eq_mask = _mm_cmpeq_ps(l.mm, r.mm);
    __m128 ones    = _mm_cmpeq_ps(eq_mask, eq_mask);
    return !_mm_testc_ps(eq_mask, ones);
#endif
}

//
// m3
//

// @TODO(Roman): sizeof(m3) == 9*sizeof(f32);

union REV_INTRIN_TYPE m3 final
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
    #if REV_ISA >= REV_ISA_AVX
        __m256 mm0;
        __m128 mm1;
    #else
        __m128 mm0;
        __m128 mm1;
        __m128 mm2;
    #endif
    };

    m3()
    #if REV_ISA >= REV_ISA_AVX
        : mm0(_mm256_setzero_ps()),
          mm1(_mm_setzero_ps())
    #else
        : mm0(_mm_setzero_ps()),
          mm1(_mm_setzero_ps()),
          mm2(_mm_setzero_ps())
    #endif
    {
    }

    m3(f32 val)
    {
        __m128 vval = _mm_setr_ps(val, val, val, 0.0f);
    #if REV_ISA >= REV_ISA_AVX
        mm0 = _mm256_broadcast_ps(&vval);
        mm1 = vval;
    #else
        mm0 = vval;
        mm1 = vval;
        mm2 = vval;
    #endif
    }

    m3(f32 e00, f32 e01, f32 e02,
       f32 e10, f32 e11, f32 e12,
       f32 e20, f32 e21, f32 e22)
    #if REV_ISA >= REV_ISA_AVX
        : mm0(_mm256_setr_ps(e00, e01, e02, 0.0f,
                             e10, e11, e12, 0.0f)),
          mm1(   _mm_setr_ps(e20, e21, e22, 0.0f))
    #else
        : mm0(_mm_setr_ps(e00, e01, e02, 0.0f)),
          mm1(_mm_setr_ps(e10, e11, e12, 0.0f)),
          mm2(_mm_setr_ps(e20, e21, e22, 0.0f))
    #endif
    {
    }

    m3(v3 c1, v3 c2, v3 c3)
    #if REV_ISA >= REV_ISA_AVX
        : mm0(_mm256_setr_ps(c1.x, c2.x, c3.x, 0.0f,
                             c1.y, c2.y, c3.y, 0.0f)),
          mm1(   _mm_setr_ps(c1.z, c2.z, c3.z, 0.0f))
    #else
        : mm0(_mm_setr_ps(c1.x, c2.x, c3.x, 0.0f)),
          mm1(_mm_setr_ps(c1.y, c2.y, c3.y, 0.0f)),
          mm2(_mm_setr_ps(c1.z, c2.z, c3.z, 0.0f))
    #endif
    {
    }

    m3(f32 arr[9])
    #if REV_ISA >= REV_ISA_AVX
        : mm0(_mm256_setr_ps(*arr,   arr[1], arr[2], 0.0f,
                             arr[3], arr[4], arr[5], 0.0f)),
          mm1(   _mm_setr_ps(arr[6], arr[7], arr[8], 0.0f))
    #else
        : mm0(_mm_setr_ps(*arr,   arr[1], arr[2], 0.0f)),
          mm1(_mm_setr_ps(arr[3], arr[4], arr[5], 0.0f)),
          mm2(_mm_setr_ps(arr[6], arr[7], arr[8], 0.0f))
    #endif
    {
    }

#if REV_ISA >= REV_ISA_AVX
    m3(__m256 _mm0, __m128 _mm1)
        : mm0(_mm256_blend_ps(_mm0, _mm256_setzero_ps(), 0b10001000)),
          mm1(mm_insert_f32<3>(_mm1, 0.0f))
    {
    }
#endif

    m3(__m128 _mm0, __m128 _mm1, __m128 _mm2)
    #if REV_ISA >= REV_ISA_AVX
        : mm0(_mm256_setr_m128(mm_insert_f32<3>(_mm0, 0.0f), mm_insert_f32<3>(_mm1, 0.0f))),
          mm1(mm_insert_f32<3>(_mm2, 0.0f))
    #else
        : mm0(mm_insert_f32<3>(_mm0, 0.0f)),
          mm1(mm_insert_f32<3>(_mm1, 0.0f)),
          mm2(mm_insert_f32<3>(_mm2, 0.0f))
    #endif
    {
    }

    m3(const m3& m)
    #if REV_ISA >= REV_ISA_AVX
        : mm0(m.mm0),
          mm1(m.mm1)
    #else
        : mm0(m.mm0),
          mm1(m.mm1),
          mm2(m.mm2)
    #endif
    {
    }

    m3(m3&& m)
    #if REV_ISA >= REV_ISA_AVX
        : mm0(m.mm0),
          mm1(m.mm1)
    #else
        : mm0(m.mm0),
          mm1(m.mm1),
          mm2(m.mm2)
    #endif
    {
    }

    m3& __vectorcall operator=(f32 val)
    {
        __m128 vval = _mm_setr_ps(val, val, val, 0.0f);
    #if REV_ISA >= REV_ISA_AVX
        mm0 = _mm256_broadcast_ps(&vval);
        mm1 = vval;
    #else
        mm0 = vval;
        mm1 = vval;
        mm2 = vval;
    #endif
        return *this;
    }

    m3& __vectorcall operator=(f32 arr[9])
    {
    #if REV_ISA >= REV_ISA_AVX
        mm0 = _mm256_setr_ps(*arr,   arr[1], arr[2], 0.0f,
                             arr[3], arr[4], arr[5], 0.0f);
        mm1 =    _mm_setr_ps(arr[6], arr[7], arr[8], 0.0f);
    #else
        mm0 = _mm_setr_ps(*arr,   arr[1], arr[2], 0.0f);
        mm1 = _mm_setr_ps(arr[3], arr[4], arr[5], 0.0f);
        mm2 = _mm_setr_ps(arr[6], arr[7], arr[8], 0.0f);
    #endif
        return *this;
    }

    m3& __vectorcall operator=(const m3& m)
    {
    #if REV_ISA >= REV_ISA_AVX
        mm0 = m.mm0;
        mm1 = m.mm1;
    #else
        mm0 = m.mm0;
        mm1 = m.mm1;
        mm2 = m.mm2;
    #endif
        return *this;
    }

    m3& __vectorcall operator=(m3&& m)
    {
    #if REV_ISA >= REV_ISA_AVX
        mm0 = m.mm0;
        mm1 = m.mm1;
    #else
        mm0 = m.mm0;
        mm1 = m.mm1;
        mm2 = m.mm2;
    #endif
        return *this;
    }

    m3 __vectorcall transpose() const
    {
    #if REV_ISA >= REV_ISA_AVX
        return m3(_mm256_setr_ps(e00, e10, e20, 0.0f,
                                 e01, e11, e21, 0.0f),
                     _mm_setr_ps(e02, e12, e22, 0.0f));
    #else
        return m3(_mm_setr_ps(e00, e10, e20, 0.0f),
                  _mm_setr_ps(e01, e11, e21, 0.0f),
                  _mm_setr_ps(e02, e12, e22, 0.0f));
    #endif
    }

    f32 __vectorcall det() const
    {
        // return e00 * (e11 * e22 - e12 * 21)
        //      - e01 * (e10 * e22 - e12 * 20)
        //      + e02 * (e10 * e21 - e11 * 20);

        __m128 a = _mm_mul_ps(_mm_setr_ps(e11, e10, e10, 0.0f),
                              _mm_setr_ps(e22, e22, e21, 0.0f));

        __m128 b = _mm_mul_ps(_mm_setr_ps(e12, e12, e11, 0.0f),
                              _mm_setr_ps(e21, e20, e20, 0.0f));

        __m128 c = _mm_mul_ps(_mm_setr_ps(e00, e01, e02, 0.0f),
                              _mm_sub_ps(a, b));

        __m128 sub = _mm_hsub_ps(c, c);
        __m128 res = _mm_hadd_ps(sub, sub);

        return _mm_cvtss_f32(res);
    }

    m3 __vectorcall inverse() const
    {
        __m128 col_1 = _mm_sub_ps(_mm_mul_ps(_mm_setr_ps(e11, e12, e10, 0.0f),
                                             _mm_setr_ps(e22, e20, e21, 0.0f)),
                                  _mm_mul_ps(_mm_setr_ps(e12, e10, e11, 0.0f),
                                             _mm_setr_ps(e21, e22, e20, 0.0f)));

        __m128 col_2 = _mm_sub_ps(_mm_mul_ps(_mm_setr_ps(e02, e00, e01, 0.0f),
                                             _mm_setr_ps(e21, e22, e20, 0.0f)),
                                  _mm_mul_ps(_mm_setr_ps(e01, e02, e00, 0.0f),
                                             _mm_setr_ps(e22, e20, e21, 0.0f)));

        __m128 col_3 = _mm_sub_ps(_mm_mul_ps(_mm_setr_ps(e01, e02, e00, 0.0f),
                                             _mm_setr_ps(e12, e10, e01, 0.0f)),
                                  _mm_mul_ps(_mm_setr_ps(e02, e00, e10, 0.0f),
                                             _mm_setr_ps(e11, e12, e11, 0.0f)));

        __m128 vdet = _mm_set_ps1(det());

        col_1 = _mm_div_ps(col_1, vdet);
        col_2 = _mm_div_ps(col_2, vdet);
        col_3 = _mm_div_ps(col_3, vdet);

        return m3(mm_extract_f32<0>(col_1), mm_extract_f32<0>(col_2), mm_extract_f32<0>(col_3),
                  mm_extract_f32<1>(col_1), mm_extract_f32<1>(col_2), mm_extract_f32<1>(col_3),
                  mm_extract_f32<2>(col_1), mm_extract_f32<2>(col_2), mm_extract_f32<2>(col_3));
    }

    static m3 __vectorcall identity()
    {
        return m3(1.0f, 0.0f, 0.0f,
                  0.0f, 1.0f, 0.0f,
                  0.0f, 0.0f, 1.0f);
    }

    static m3 __vectorcall scaling(f32 nx, f32 ny)
    {
        return m3(  nx, 0.0f, 0.0f,
                  0.0f,   ny, 0.0f,
                  0.0f, 0.0f, 1.0f);
    }

    static m3 __vectorcall rotation_z(rad angle)
    {
        f32 s = sinf(angle);
        f32 c = cosf(angle);

        return m3(   c,   -s, 0.0f,
                     s,    c, 0.0f,
                  0.0f, 0.0f, 1.0f);
    }

    static m3 __vectorcall reflection_x()
    {
        return m3(-1.0f, 0.0f, 0.0f,
                   0.0f, 1.0f, 0.0f,
                   0.0f, 0.0f, 1.0f);
    }

    static m3 __vectorcall reflection_y()
    {
        return m3(1.0f,  0.0f, 0.0f,
                  0.0f, -1.0f, 0.0f,
                  0.0f,  0.0f, 1.0f);
    }

    static m3 __vectorcall reflection_xy()
    {
        return m3(-1.0f,  0.0f, 0.0f,
                   0.0f, -1.0f, 0.0f,
                   0.0f,  0.0f, 1.0f);
    }

    static m3 __vectorcall shearing(f32 shx, f32 shy)
    {
        return m3(1.0f,  shx, 0.0f,
                   shy, 1.0f, 0.0f,
                  0.0f, 0.0f, 1.0f);
    }

    static m3 __vectorcall translation(f32 dx, f32 dy)
    {
        return m3(1.0f, 0.0f,   dx,
                  0.0f, 1.0f,   dy,
                  0.0f, 0.0f, 1.0f);
    }

    static m3 __vectorcall translation(v3 v)
    {
        return m3(1.0f, 0.0f,  v.x,
                  0.0f, 1.0f,  v.y,
                  0.0f, 0.0f, 1.0f);
    }

    static m3 __vectorcall ortho(f32 left, f32 right, f32 bottom, f32 top)
    {
        return m3(2.0f/(right-left),              0.0f, -(right+left)/(right-left),
                               0.0f, 2.0f/(top-bottom), -(top+bottom)/(top-bottom),
                               0.0f,              0.0f,                       1.0f);
    }

    m3& __vectorcall operator+=(m3 r)
    {
    #if REV_ISA >= REV_ISA_AVX
        mm0 = _mm256_add_ps(mm0, r.mm0);
        mm1 =    _mm_add_ps(mm1, r.mm1);
    #else
        mm0 = _mm_add_ps(mm0, r.mm0);
        mm1 = _mm_add_ps(mm1, r.mm1);
        mm2 = _mm_add_ps(mm2, r.mm2);
    #endif
        return *this;
    }

    m3& __vectorcall operator-=(m3 r)
    {
    #if REV_ISA >= REV_ISA_AVX
        mm0 = _mm256_sub_ps(mm0, r.mm0);
        mm1 =    _mm_sub_ps(mm1, r.mm1);
    #else
        mm0 = _mm_sub_ps(mm0, r.mm0);
        mm1 = _mm_sub_ps(mm1, r.mm1);
        mm2 = _mm_sub_ps(mm2, r.mm2);
    #endif
        return *this;
    }

    m3& __vectorcall operator*=(m3 r)
    {
        __m128 r_c0 = _mm_setr_ps(r.e00, r.e10, r.e20, 0.0f);
        __m128 r_c1 = _mm_setr_ps(r.e01, r.e11, r.e21, 0.0f);
        __m128 r_c2 = _mm_setr_ps(r.e02, r.e12, r.e22, 0.0f);

    #if REV_ISA >= REV_ISA_AVX
        __m128 l_r0 = _mm_setr_ps(e00, e01, e02, 0.0f);
        __m128 l_r1 = _mm_setr_ps(e10, e11, e12, 0.0f);
        __m128 l_r2 = _mm_setr_ps(e20, e21, e22, 0.0f);

        mm0 = _mm256_setr_ps(_mm_cvtss_f32(_mm_dp_ps(l_r0, r_c0, 0x71)), _mm_cvtss_f32(_mm_dp_ps(l_r0, r_c1, 0x71)), _mm_cvtss_f32(_mm_dp_ps(l_r0, r_c2, 0x71)), 0.0f,
                             _mm_cvtss_f32(_mm_dp_ps(l_r1, r_c0, 0x71)), _mm_cvtss_f32(_mm_dp_ps(l_r1, r_c1, 0x71)), _mm_cvtss_f32(_mm_dp_ps(l_r1, r_c2, 0x71)), 0.0f);
        mm1 =    _mm_setr_ps(_mm_cvtss_f32(_mm_dp_ps(l_r2, r_c0, 0x71)), _mm_cvtss_f32(_mm_dp_ps(l_r2, r_c1, 0x71)), _mm_cvtss_f32(_mm_dp_ps(l_r2, r_c2, 0x71)), 0.0f);
    #else
        mm0 = _mm_setr_ps(_mm_cvtss_f32(_mm_dp_ps(mm0, r_c0, 0x71)), _mm_cvtss_f32(_mm_dp_ps(mm0, r_c1, 0x71)), _mm_cvtss_f32(_mm_dp_ps(mm0, r_c2, 0x71)), 0.0f);
        mm1 = _mm_setr_ps(_mm_cvtss_f32(_mm_dp_ps(mm1, r_c0, 0x71)), _mm_cvtss_f32(_mm_dp_ps(mm1, r_c1, 0x71)), _mm_cvtss_f32(_mm_dp_ps(mm1, r_c2, 0x71)), 0.0f);
        mm2 = _mm_setr_ps(_mm_cvtss_f32(_mm_dp_ps(mm2, r_c0, 0x71)), _mm_cvtss_f32(_mm_dp_ps(mm2, r_c1, 0x71)), _mm_cvtss_f32(_mm_dp_ps(mm2, r_c2, 0x71)), 0.0f);
    #endif

        return *this;
    }
    
    m3& __vectorcall operator+=(f32 r)
    {
        __m128 vval = _mm_setr_ps(r, r, r, 0.0f);
    #if REV_ISA >= REV_ISA_AVX
        mm0 = _mm256_add_ps(mm0, _mm256_broadcast_ps(&vval));
        mm1 =    _mm_add_ps(mm1, vval);
    #else
        mm0 = _mm_add_ps(mm0, vval);
        mm1 = _mm_add_ps(mm1, vval);
        mm2 = _mm_add_ps(mm2, vval);
    #endif
        return *this;
    }

    m3& __vectorcall operator-=(f32 r)
    {
        __m128 vval = _mm_setr_ps(r, r, r, 0.0f);
    #if REV_ISA >= REV_ISA_AVX
        mm0 = _mm256_sub_ps(mm0, _mm256_broadcast_ps(&vval));
        mm1 =    _mm_sub_ps(mm1, vval);
    #else
        mm0 = _mm_sub_ps(mm0, vval);
        mm1 = _mm_sub_ps(mm1, vval);
        mm2 = _mm_sub_ps(mm2, vval);
    #endif
        return *this;
    }

    m3& __vectorcall operator*=(f32 r)
    {
        __m128 vval = _mm_setr_ps(r, r, r, 0.0f);
    #if REV_ISA >= REV_ISA_AVX
        mm0 = _mm256_mul_ps(mm0, _mm256_broadcast_ps(&vval));
        mm1 =    _mm_mul_ps(mm1, vval);
    #else
        mm0 = _mm_mul_ps(mm0, vval);
        mm1 = _mm_mul_ps(mm1, vval);
        mm2 = _mm_mul_ps(mm2, vval);
    #endif
        return *this;
    }

    m3& __vectorcall operator/=(f32 r)
    {
        __m128 vval = _mm_setr_ps(r, r, r, 0.0f);
    #if REV_ISA >= REV_ISA_AVX
        mm0 = _mm256_div_ps(mm0, _mm256_broadcast_ps(&vval));
        mm1 =    _mm_div_ps(mm1, vval);
    #else
        mm0 = _mm_div_ps(mm0, vval);
        mm1 = _mm_div_ps(mm1, vval);
        mm2 = _mm_div_ps(mm2, vval);
    #endif
        return *this;
    }

    #if 0 // @TODO(Roman): Switch to after rewriting data
        v3  operator[](u8 index) const { return cast<v3 *>(this)[index]; }
        v3& operator[](u8 index)       { return cast<v3 *>(this)[index]; }
    #else
        v3  operator[](u8 index) const { return index == 0 ? r0 : index == 1 ? r1 : r2; }
        v3& operator[](u8 index)       { return index == 0 ? r0 : index == 1 ? r1 : r2; }
    #endif
};

REV_INLINE m3 __vectorcall operator+(m3 l, m3 r)
{
#if REV_ISA >= REV_ISA_AVX
    return m3(_mm256_add_ps(l.mm0, r.mm0),
                 _mm_add_ps(l.mm1, r.mm1));
#else
    return m3(_mm_add_ps(l.mm0, r.mm0),
              _mm_add_ps(l.mm1, r.mm1),
              _mm_add_ps(l.mm2, r.mm2));
#endif
}

REV_INLINE m3 __vectorcall operator-(m3 l, m3 r)
{
#if REV_ISA >= REV_ISA_AVX
    return m3(_mm256_sub_ps(l.mm0, r.mm0),
                 _mm_sub_ps(l.mm1, r.mm1));
#else
    return m3(_mm_sub_ps(l.mm0, r.mm0),
              _mm_sub_ps(l.mm1, r.mm1),
              _mm_sub_ps(l.mm2, r.mm2));
#endif
}

REV_INLINE m3 __vectorcall operator*(m3 l, m3 r)
{
    __m128 r_c0 = _mm_setr_ps(r.e00, r.e10, r.e20, 0.0f);
    __m128 r_c1 = _mm_setr_ps(r.e01, r.e11, r.e21, 0.0f);
    __m128 r_c2 = _mm_setr_ps(r.e02, r.e12, r.e22, 0.0f);

#if REV_ISA >= REV_ISA_AVX
    __m128 l_r0 = _mm_setr_ps(l.e00, l.e01, l.e02, 0.0f);
    __m128 l_r1 = _mm_setr_ps(l.e10, l.e11, l.e12, 0.0f);
    __m128 l_r2 = _mm_setr_ps(l.e20, l.e21, l.e22, 0.0f);

    return m3(_mm256_setr_ps(_mm_cvtss_f32(_mm_dp_ps(l_r0, r_c0, 0x71)), _mm_cvtss_f32(_mm_dp_ps(l_r0, r_c1, 0x71)), _mm_cvtss_f32(_mm_dp_ps(l_r0, r_c2, 0x71)), 0.0f,
                             _mm_cvtss_f32(_mm_dp_ps(l_r1, r_c0, 0x71)), _mm_cvtss_f32(_mm_dp_ps(l_r1, r_c1, 0x71)), _mm_cvtss_f32(_mm_dp_ps(l_r1, r_c2, 0x71)), 0.0f),
                 _mm_setr_ps(_mm_cvtss_f32(_mm_dp_ps(l_r2, r_c0, 0x71)), _mm_cvtss_f32(_mm_dp_ps(l_r2, r_c1, 0x71)), _mm_cvtss_f32(_mm_dp_ps(l_r2, r_c2, 0x71)), 0.0f));
#else
    return m3(_mm_setr_ps(_mm_cvtss_f32(_mm_dp_ps(l.mm0, r_c0, 0x71)), _mm_cvtss_f32(_mm_dp_ps(l.mm0, r_c1, 0x71)), _mm_cvtss_f32(_mm_dp_ps(l.mm0, r_c2, 0x71)), 0.0f),
              _mm_setr_ps(_mm_cvtss_f32(_mm_dp_ps(l.mm1, r_c0, 0x71)), _mm_cvtss_f32(_mm_dp_ps(l.mm1, r_c1, 0x71)), _mm_cvtss_f32(_mm_dp_ps(l.mm1, r_c2, 0x71)), 0.0f),
              _mm_setr_ps(_mm_cvtss_f32(_mm_dp_ps(l.mm2, r_c0, 0x71)), _mm_cvtss_f32(_mm_dp_ps(l.mm2, r_c1, 0x71)), _mm_cvtss_f32(_mm_dp_ps(l.mm2, r_c2, 0x71)), 0.0f));
#endif
}

REV_INLINE m3 __vectorcall operator+(f32 l, m3 r)
{
    __m128 vval = _mm_setr_ps(l, l, l, 0.0f);
#if REV_ISA >= REV_ISA_AVX
    return m3(_mm256_add_ps(_mm256_broadcast_ps(&vval), r.mm0),
                 _mm_add_ps(vval,                       r.mm1));
#else
    return m3(_mm_add_ps(vval, r.mm0),
              _mm_add_ps(vval, r.mm1),
              _mm_add_ps(vval, r.mm2));
#endif
}

REV_INLINE m3 __vectorcall operator-(m3 l, f32 r)
{
    __m128 vval = _mm_setr_ps(r, r, r, 0.0f);
#if REV_ISA >= REV_ISA_AVX
    return m3(_mm256_sub_ps(l.mm0, _mm256_broadcast_ps(&vval)),
                 _mm_sub_ps(l.mm1, vval));
#else
    return m3(_mm_sub_ps(l.mm0, vval),
              _mm_sub_ps(l.mm1, vval),
              _mm_sub_ps(l.mm2, vval));
#endif
}

REV_INLINE m3 __vectorcall operator*(f32 l, m3 r)
{
    __m128 vval = _mm_setr_ps(l, l, l, 0.0f);
#if REV_ISA >= REV_ISA_AVX
    return m3(_mm256_mul_ps(_mm256_broadcast_ps(&vval), r.mm0),
                 _mm_mul_ps(vval,                       r.mm1));
#else
    return m3(_mm_mul_ps(vval, r.mm0),
              _mm_mul_ps(vval, r.mm1),
              _mm_mul_ps(vval, r.mm2));
#endif
}

REV_INLINE m3 __vectorcall operator/(m3 l, f32 r)
{
    __m128 vval = _mm_setr_ps(r, r, r, 0.0f);
#if REV_ISA >= REV_ISA_AVX
    return m3(_mm256_div_ps(l.mm0, _mm256_broadcast_ps(&vval)),
                 _mm_div_ps(l.mm1, vval));
#else
    return m3(_mm_div_ps(l.mm0, vval),
              _mm_div_ps(l.mm1, vval),
              _mm_div_ps(l.mm2, vval));
#endif
}

REV_INLINE v3 __vectorcall operator*(m3 l, v3 r)
{
    __m128 l_r0 = _mm_setr_ps(l.e00, l.e01, l.e02, 0.0f);
    __m128 l_r1 = _mm_setr_ps(l.e10, l.e11, l.e12, 0.0f);
    __m128 l_r2 = _mm_setr_ps(l.e20, l.e21, l.e22, 0.0f);

    __m128 r_mm = _mm_setr_ps(r.x, r.y, r.z, 0.0f);

    return v3(_mm_cvtss_f32(_mm_dp_ps(l_r0, r_mm, 0x71)),
              _mm_cvtss_f32(_mm_dp_ps(l_r1, r_mm, 0x71)),
              _mm_cvtss_f32(_mm_dp_ps(l_r2, r_mm, 0x71)));
}

REV_INLINE bool __vectorcall operator==(m3 l, m3 r)
{
#if REV_ISA >= REV_ISA_AVX
    return mm256_equals(&l.mm0, &r.mm0)
        &&    mm_equals(&l.mm1, &r.mm1);
#else
    return mm_equals(&l.mm0, &r.mm0)
        && mm_equals(&l.mm1, &r.mm1)
        && mm_equals(&l.mm2, &r.mm2);
#endif
}

REV_INLINE bool __vectorcall operator!=(m3 l, m3 r)
{
#if REV_ISA >= REV_ISA_AVX
    return !(   mm256_equals(&l.mm0, &r.mm0)
             &&    mm_equals(&l.mm1, &r.mm1));
#else
    return !(   mm_equals(&l.mm0, &r.mm0)
             && mm_equals(&l.mm1, &r.mm1)
             && mm_equals(&l.mm2, &r.mm2));
#endif
}

//
// m4
//

union REV_INTRIN_TYPE REV_ALIGN(32) m4 final
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
#if REV_ISA >= REV_ISA_AVX512
    __m512 zmm;
#endif
    struct
    {
    #if REV_ISA >= REV_ISA_AVX
        __m256 ymm0;
        __m256 ymm1;
    #endif
    };
    struct
    {
        __m128 xmm0;
        __m128 xmm1;
        __m128 xmm2;
        __m128 xmm3;
    };

    m4()
    #if REV_ISA >= REV_ISA_AVX512
        : zmm(_mm512_setzero_ps())
    #elif REV_ISA >= REV_ISA_AVX
        : ymm0(_mm256_setzero_ps()),
          ymm1(ymm0)
    #else
        : xmm0(_mm_setzero_ps()),
          xmm1(xmm0),
          xmm2(xmm0),
          xmm3(xmm0))
    #endif
    {
    }

    m4(f32 val)
    #if REV_ISA >= REV_ISA_AVX512
        : zmm(_mm512_set1_ps(val))
    #elif REV_ISA >= REV_ISA_AVX
        : ymm0(_mm256_set1_ps(val)),
          ymm1(ymm0)
    #else
        : xmm0(_mm_set_ps1(val)),
          xmm1(xmm0),
          xmm2(xmm0),
          xmm3(xmm0)
    #endif
    {
    }

    m4(f32 e00, f32 e01, f32 e02, f32 e03,
       f32 e10, f32 e11, f32 e12, f32 e13,
       f32 e20, f32 e21, f32 e22, f32 e23,
       f32 e30, f32 e31, f32 e32, f32 e33)
    #if REV_ISA >= REV_ISA_AVX512
        : zmm(_mm512_setr_ps(e00, e01, e02, e03,
                               e10, e11, e12, e13,
                               e20, e21, e22, e23,
                               e30, e31, e32, e33))
    #elif REV_ISA >= REV_ISA_AVX
        : ymm0(_mm256_setr_ps(e00, e01, e02, e03,
                              e10, e11, e12, e13)),
          ymm1(_mm256_setr_ps(e20, e21, e22, e23,
                              e30, e31, e32, e33))
    #else
        : xmm0(_mm_setr_ps(e00, e01, e02, e03)),
          xmm1(_mm_setr_ps(e10, e11, e12, e13)),
          xmm2(_mm_setr_ps(e20, e21, e22, e23)),
          xmm3(_mm_setr_ps(e30, e31, e32, e33))
    #endif
    {
    }

    m4(v4 c1, v4 c2, v4 c3, v4 c4)
    #if REV_ISA >= REV_ISA_AVX512
        : zmm(_mm512_setr_ps(c1.x, c2.x, c3.x, c4.x,
                             c1.y, c2.y, c3.y, c4.y,
                             c1.z, c2.z, c3.z, c4.z,
                             c1.w, c2.w, c3.w, c4.w))
    #elif REV_ISA >= REV_ISA_AVX
        : ymm0(_mm256_setr_ps(c1.x, c2.x, c3.x, c4.x,
                              c1.y, c2.y, c3.y, c4.y)),
          ymm1(_mm256_setr_ps(c1.z, c2.z, c3.z, c4.z,
                              c1.w, c2.w, c3.w, c4.w))
    #else
        : xmm0(_mm_setr_ps(c1.x, c2.x, c3.x, c4.x)),
          xmm1(_mm_setr_ps(c1.y, c2.y, c3.y, c4.y)),
          xmm2(_mm_setr_ps(c1.z, c2.z, c3.z, c4.z)),
          xmm3(_mm_setr_ps(c1.w, c2.w, c3.w, c4.w))
    #endif
    {
    }

    m4(f32 arr[16])
    #if REV_ISA >= REV_ISA_AVX512
        : zmm(_mm512_load_ps(arr))
    #elif REV_ISA >= REV_ISA_AVX
        : ymm0(_mm256_load_ps(arr)),
          ymm1(_mm256_load_ps(arr + 8))
    #else
        : xmm0(_mm_load_ps(arr)),
          xmm1(_mm_load_ps(arr + 4)),
          xmm2(_mm_load_ps(arr + 8)),
          xmm3(_mm_load_ps(arr + 12))
    #endif
    {
    }

#if REV_ISA >= REV_ISA_AVX512
    m4(__m512 mm)
        : zmm(mm)
    {
    }
#endif

#if REV_ISA >= REV_ISA_AVX
    m4(__m256 _mm0, __m256 _mm1)
        : ymm0(_mm0),
          ymm1(_mm1)
    {
    }
#endif

    m4(__m128 _mm0, __m128 _mm1, __m128 _mm2, __m128 _mm3)
        : xmm0(_mm0),
          xmm1(_mm1),
          xmm2(_mm2),
          xmm3(_mm3)
    {
    }

    m4(const m4& m)
    #if REV_ISA >= REV_ISA_AVX512
        : zmm(m.zmm)
    #elif REV_ISA >= REV_ISA_AVX
        : ymm0(m.ymm0),
          ymm1(m.ymm1)
    #else
        : xmm0(m.xmm0),
          xmm1(m.xmm1),
          xmm2(m.xmm2),
          xmm3(m.xmm3)
    #endif
    {
    }

    m4(m4&& m)
    #if REV_ISA >= REV_ISA_AVX512
        : zmm(m.zmm)
    #elif REV_ISA >= REV_ISA_AVX
        : ymm0(m.ymm0),
          ymm1(m.ymm1)
    #else
        : xmm0(m.xmm0),
          xmm1(m.xmm1),
          xmm2(m.xmm2),
          xmm3(m.xmm3)
    #endif
    {
    }

    m4& __vectorcall operator=(f32 val)
    {
    #if REV_ISA >= REV_ISA_AVX512
        zmm = _mm512_set1_ps(val);
    #elif REV_ISA >= REV_ISA_AVX
        ymm0 = _mm256_set1_ps(val);
        ymm1 = ymm0;
    #else
        xmm0 = _mm_set_ps1(val);
        xmm1 = xmm0;
        xmm2 = xmm0;
        xmm3 = xmm0;
    #endif
        return *this;
    }

    m4& __vectorcall operator=(f32 arr[16])
    {
    #if REV_ISA >= REV_ISA_AVX512
        zmm = _mm512_load_ps(arr);
    #elif REV_ISA >= REV_ISA_AVX
        ymm0 = _mm256_load_ps(arr);
        ymm1 = _mm256_load_ps(arr + 8);
    #else
        xmm0 = _mm_load_ps(arr);
        xmm1 = _mm_load_ps(arr + 4);
        xmm2 = _mm_load_ps(arr + 8);
        xmm3 = _mm_load_ps(arr + 12);
    #endif
        return *this;
    }

    m4& __vectorcall operator=(const m4& m)
    {
    #if REV_ISA >= REV_ISA_AVX512
        zmm = m.zmm;
    #elif REV_ISA >= REV_ISA_AVX
        ymm0 = m.ymm0;
        ymm1 = m.ymm1;
    #else
        xmm0 = m.xmm0;
        xmm1 = m.xmm1;
        xmm2 = m.xmm2;
        xmm3 = m.xmm3;
    #endif
        return *this;
    }

    m4& __vectorcall operator=(m4&& m)
    {
    #if REV_ISA >= REV_ISA_AVX512
        zmm = m.zmm;
    #elif REV_ISA >= REV_ISA_AVX
        ymm0 = m.ymm0;
        ymm1 = m.ymm1;
    #else
        xmm0 = m.xmm0;
        xmm1 = m.xmm1;
        xmm2 = m.xmm2;
        xmm3 = m.xmm3;
    #endif
        return *this;
    }

    m4 __vectorcall transpose() const
    {
        return m4(e00, e10, e20, e30,
                  e01, e11, e21, e31,
                  e02, e12, e22, e32,
                  e03, e13, e23, e33);
    }

    f32 __vectorcall det() const
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
    #if REV_ISA >= REV_ISA_AVX512
        __m512 first  = _mm512_setr_ps(e22, e21, e21, e22, e20, e20, e21, e20, e20, e21, e20, e20, 0.0f, 0.0f, 0.0f, 0.0f);
        __m512 second = _mm512_setr_ps(e33, e33, e32, e33, e33, e32, e33, e33, e31, e32, e32, e31, 0.0f, 0.0f, 0.0f, 0.0f);
        __m512 third  = _mm512_setr_ps(e23, e23, e22, e23, e23, e22, e23, e23, e21, e22, e22, e21, 0.0f, 0.0f, 0.0f, 0.0f);
        __m512 forth  = _mm512_setr_ps(e32, e31, e31, e32, e30, e30, e31, e30, e30, e31, e30, e30, 0.0f, 0.0f, 0.0f, 0.0f);
        __m512 fifth  = _mm512_setr_ps(e11, e12, e13, e10, e12, e13, e10, e11, e13, e10, e11, e12, 0.0f, 0.0f, 0.0f, 0.0f);

        __m512 inner = _mm512_mul_ps(fifth, _mm512_sub_ps(_mm512_mul_ps(first, second), _mm512_mul_ps(third, forth)));

        __m128 right_1 = _mm_setr_ps(mm_extract_f32<0>(inner), mm_extract_f32<3>(inner), mm_extract_f32<6>(inner), mm_extract_f32<9>(inner));
        __m128 right_2 = _mm_setr_ps(mm_extract_f32<1>(inner), mm_extract_f32<4>(inner), mm_extract_f32<7>(inner), mm_extract_f32<10>(inner));
        __m128 right_3 = _mm_setr_ps(mm_extract_f32<2>(inner), mm_extract_f32<5>(inner), mm_extract_f32<8>(inner), mm_extract_f32<11>(inner));

        __m128 left  = _mm_setr_ps(e00, e01, e02, e03);
        __m128 right = _mm_add_ps(_mm_sub_ps(right_1, right_2), right_3);

        __m128 res = _mm_mul_ps(left, right);
        res = _mm_hsub_ps(res, res);
        res = _mm_hadd_ps(res, res);

        return mm_extract_f32<0>(res);
    #elif REV_ISA >= REV_ISA_AVX
        __m256 first_1 = _mm256_setr_ps(e22, e21, e21, e22, e20, e20, e21, e20);
        __m128 first_2 = _mm_setr_ps(e20, e21, e20, e20);

        __m256 second_1 = _mm256_setr_ps(e33, e33, e32, e33, e33, e32, e33, e33);
        __m128 second_2 = _mm_setr_ps(e31, e32, e32, e31);

        __m256 third_1 = _mm256_setr_ps(e23, e23, e22, e23, e23, e22, e23, e23);
        __m128 third_2 = _mm_setr_ps(e21, e22, e22, e21);

        __m256 forth_1 = _mm256_setr_ps(e32, e31, e31, e32, e30, e30, e31, e30);
        __m128 forth_2 = _mm_setr_ps(e30, e31, e30, e30);

        __m256 fifth_1 = _mm256_setr_ps(e11, e12, e13, e10, e12, e13, e10, e11);
        __m128 fifth_2 = _mm_setr_ps(e13, e10, e11, e12);

        __m256 inner_1 = _mm256_mul_ps(fifth_1, _mm256_sub_ps(_mm256_mul_ps(first_1, second_1), _mm256_mul_ps(third_1, forth_1)));
        __m128 inner_2 =    _mm_mul_ps(fifth_2,    _mm_sub_ps(   _mm_mul_ps(first_2, second_2),    _mm_mul_ps(third_2, forth_2)));

        __m128 right_1 = _mm_setr_ps(mm_extract_f32<0>(inner_1), mm_extract_f32<3>(inner_1), mm_extract_f32<6>(inner_1), mm_extract_f32<1>(inner_2));
        __m128 right_2 = _mm_setr_ps(mm_extract_f32<1>(inner_1), mm_extract_f32<4>(inner_1), mm_extract_f32<7>(inner_1), mm_extract_f32<2>(inner_2));
        __m128 right_3 = _mm_setr_ps(mm_extract_f32<2>(inner_1), mm_extract_f32<5>(inner_1), mm_extract_f32<0>(inner_2), mm_extract_f32<3>(inner_2));

        __m128 left  = _mm_setr_ps(e00, e01, e02, e03);
        __m128 right = _mm_add_ps(_mm_sub_ps(right_1, right_2), right_3);

        __m128 res = _mm_mul_ps(left, right);
        res = _mm_hsub_ps(res, res);
        res = _mm_hadd_ps(res, res);

        return mm_extract_f32<0>(res);
    #else
        __m128 first_1 = _mm_setr_ps(e22, e21, e21, e22);
        __m128 first_2 = _mm_setr_ps(e20, e20, e21, e20);
        __m128 first_3 = _mm_setr_ps(e20, e21, e20, e20);

        __m128 second_1 = _mm_setr_ps(e33, e33, e32, e33);
        __m128 second_2 = _mm_setr_ps(e33, e32, e33, e33);
        __m128 second_3 = _mm_setr_ps(e31, e32, e32, e31);

        __m128 third_1 = _mm_setr_ps(e23, e23, e22, e23);
        __m128 third_2 = _mm_setr_ps(e23, e22, e23, e23);
        __m128 third_3 = _mm_setr_ps(e21, e22, e22, e21);

        __m128 forth_1 = _mm_setr_ps(e32, e31, e31, e32);
        __m128 forth_2 = _mm_setr_ps(e30, e30, e31, e30);
        __m128 forth_3 = _mm_setr_ps(e30, e31, e30, e30);

        __m128 fifth_1 = _mm_setr_ps(e11, e12, e13, e10);
        __m128 fifth_2 = _mm_setr_ps(e12, e13, e10, e11);
        __m128 fifth_3 = _mm_setr_ps(e13, e10, e11, e12);

        __m128 inner_1 = _mm_mul_ps(fifth_1, _mm_sub_ps(_mm_mul_ps(first_1, second_1), _mm_mul_ps(third_1, forth_1)));
        __m128 inner_2 = _mm_mul_ps(fifth_2, _mm_sub_ps(_mm_mul_ps(first_2, second_2), _mm_mul_ps(third_2, forth_2)));
        __m128 inner_3 = _mm_mul_ps(fifth_3, _mm_sub_ps(_mm_mul_ps(first_3, second_3), _mm_mul_ps(third_3, forth_3)));

        __m128 right_1 = _mm_setr_ps(mm_extract_f32<0>(inner_1), mm_extract_f32<3>(inner_1), mm_extract_f32<2>(subres_2), mm_extract_f32<1>(inner_3));
        __m128 right_2 = _mm_setr_ps(mm_extract_f32<1>(inner_1), mm_extract_f32<0>(inner_2), mm_extract_f32<3>(subres_2), mm_extract_f32<2>(inner_3));
        __m128 right_3 = _mm_setr_ps(mm_extract_f32<2>(inner_1), mm_extract_f32<1>(inner_2), mm_extract_f32<0>(subres_3), mm_extract_f32<3>(inner_3));

        __m128 left  = _mm_setr_ps(e00, e01, e02, e03);
        __m128 right = _mm_add_ps(_mm_sub_ps(right_1, right_2), right_3);

        __m128 res = _mm_mul_ps(left, right);
        res = _mm_hsub_ps(res, res);
        res = _mm_hadd_ps(res, res);

        return mm_extract_f32<0>(res);
    #endif
    }

    m4 __vectorcall inverse() const
    {
        f32 c00 = m3(e11, e12, e13, e21, e22, e23, e31, e32, e33).det();
        f32 c01 = m3(e10, e12, e13, e20, e22, e23, e30, e32, e33).det();
        f32 c02 = m3(e10, e11, e13, e20, e21, e23, e30, e31, e33).det();
        f32 c03 = m3(e10, e11, e12, e20, e21, e22, e30, e31, e32).det();
        f32 c10 = m3(e01, e02, e03, e21, e22, e23, e31, e32, e33).det();
        f32 c11 = m3(e00, e02, e03, e20, e22, e23, e30, e32, e33).det();
        f32 c12 = m3(e00, e01, e03, e20, e21, e23, e30, e31, e33).det();
        f32 c13 = m3(e00, e01, e02, e20, e21, e22, e30, e31, e32).det();
        f32 c20 = m3(e01, e02, e03, e11, e12, e13, e31, e32, e33).det();
        f32 c21 = m3(e00, e02, e03, e10, e12, e13, e30, e32, e33).det();
        f32 c22 = m3(e00, e01, e03, e10, e11, e13, e30, e31, e33).det();
        f32 c23 = m3(e00, e01, e02, e10, e11, e12, e30, e31, e32).det();
        f32 c30 = m3(e01, e02, e03, e11, e12, e13, e21, e22, e23).det();
        f32 c31 = m3(e00, e02, e03, e10, e12, e13, e20, e22, e23).det();
        f32 c32 = m3(e00, e01, e03, e10, e11, e13, e20, e21, e23).det();
        f32 c33 = m3(e00, e01, e02, e10, e11, e12, e20, e21, e22).det();

        m4 cof(c00, c01, c02, c03,
               c10, c11, c12, c13,
               c20, c21, c22, c23,
               c30, c31, c32, c33);
        
    #if REV_ISA >= REV_ISA_AVX512
        return m4(_mm512_div_ps(cof.zmm, _mm512_set1_ps(det())));
    #elif REV_ISA >= REV_ISA_AVX
        __m256 vdet = _mm256_set1_ps(det());
        return m4(_mm256_div_ps(cof.ymm0, vdet),
                  _mm256_div_ps(cof.ymm1, vdet));
    #else
        __m128 vdet = _mm_set_ps1(det());
        return m4(_mm_div_ps(cof.xmm0, vdet),
                  _mm_div_ps(cof.xmm1, vdet),
                  _mm_div_ps(cof.xmm2, vdet),
                  _mm_div_ps(cof.xmm3, vdet));
    #endif
    }

    static m4 __vectorcall identity()
    {
        return m4(1.0f, 0.0f, 0.0f, 0.0f,
                  0.0f, 1.0f, 0.0f, 0.0f,
                  0.0f, 0.0f, 1.0f, 0.0f,
                  0.0f, 0.0f, 0.0f, 1.0f);
    }

    static m4 __vectorcall scaling(f32 nx, f32 ny, f32 nz)
    {
        return m4(  nx, 0.0f, 0.0f, 0.0f,
                  0.0f,   ny, 0.0f, 0.0f,
                  0.0f, 0.0f,   nz, 0.0f,
                  0.0f, 0.0f, 0.0f, 1.0f);
    }

    static m4 __vectorcall rotation_x(rad angle)
    {
        f32 s = sinf(angle);
        f32 c = cosf(angle);

        return m4(1.0f, 0.0f, 0.0f, 0.0f,
                  0.0f,    c,   -s, 0.0f,
                  0.0f,    s,    c, 0.0f,
                  0.0f, 0.0f, 0.0f, 1.0f);
    }

    static m4 __vectorcall rotation_y(rad angle)
    {
        f32 s = sinf(angle);
        f32 c = cosf(angle);

        return m4(   c, 0.0f,    s, 0.0f,
                  0.0f, 1.0f, 0.0f, 0.0f,
                    -s, 0.0f,    c, 0.0f,
                  0.0f, 0.0f, 0.0f, 1.0f);
    }

    static m4 __vectorcall rotation_z(rad angle)
    {
        f32 s = sinf(angle);
        f32 c = cosf(angle);

        return m4(   c,   -s, 0.0f, 0.0f,
                     s,    c, 0.0f, 0.0f,
                  0.0f, 0.0f, 1.0f, 0.0f,
                  0.0f, 0.0f, 0.0f, 1.0f);
    }

    static m4 __vectorcall rotation(v4 v, rad angle)
    {
        // x*x*(1.0f-c)+  c     x*y*(1.0f-c)-z*s     x*z*(1.0-c)+y*s     0.0f
        // y*x*(1.0f-c)+z*s     y*y*(1.0f-c)+  c     y*z*(1.0-c)-x*s     0.0f
        // z*x*(1.0f-c)-y*s     z*y*(1.0f-c)+x*s     z*z*(1.0-c)+  c     0.0f
        //             0.0f                 0.0f                0.0f     1.0f

        f32 s = sinf(angle);
        f32 c = cosf(angle);

        f32 invc = 1.0f - c;

    #if REV_ISA >= REV_ISA_AVX512
        __m512 first  = _mm512_setr_ps( v.x,  v.y,  v.z, 0.0f,  v.x,  v.y,  v.z, 0.0f,  v.x,  v.y,  v.z, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
        __m512 second = _mm512_setr_ps( v.x,  v.x,  v.x, 0.0f,  v.y,  v.y,  v.y, 0.0f,  v.z,  v.z,  v.z, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
        __m512 third  = _mm512_setr_ps(invc, invc, invc, 0.0f, invc, invc, invc, 0.0f, invc, invc, invc, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
        __m512 forth  = _mm512_setr_ps(1.0f,  v.z, -v.y, 0.0f, -v.z, 1.0f,  v.x, 0.0f,  v.y, -v.x, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
        __m512 fifth  = _mm512_setr_ps(   c,    s,    s, 0.0f,    s,    c,    s, 0.0f,    s,    s,    c, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);

        __m512 res = _mm512_add_ps(_mm512_mul_ps(_mm512_mul_ps(first, second), third), _mm512_mul_ps(forth, fifth));

        return m4(mm_extract_f32<0>(res), mm_extract_f32<4>(res), mm_extract_f32< 8>(res), 0.0f,
                  mm_extract_f32<1>(res), mm_extract_f32<5>(res), mm_extract_f32< 9>(res), 0.0f,
                  mm_extract_f32<2>(res), mm_extract_f32<6>(res), mm_extract_f32<10>(res), 0.0f,
                                    0.0f,                   0.0f,                    0.0f, 1.0f);

    #elif REV_ISA >= REV_ISA_AVX
        __m256 first_1 = _mm256_setr_ps(v.x, v.y, v.z, 0.0f,  v.x,  v.y,  v.z, 0.0f);
        __m256 first_2 = _mm256_setr_ps(v.x, v.y, v.z, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);

        __m256 second_1 = _mm256_setr_ps(v.x, v.x, v.x, 0.0f,  v.y,  v.y,  v.y, 0.0f);
        __m256 second_2 = _mm256_setr_ps(v.z, v.z, v.z, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);

        __m256 third_1 = _mm256_setr_ps(invc, invc, invc, 0.0f, invc, invc, invc, 0.0f);
        __m256 third_2 = _mm256_setr_ps(invc, invc, invc, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);

        __m256 forth_1 = _mm256_setr_ps(1.0f,  v.z, -v.y, 0.0f, -v.z, 1.0f,  v.x, 0.0f);
        __m256 forth_2 = _mm256_setr_ps( v.y, -v.x, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);

        __m256 fifth_1 = _mm256_setr_ps(   c,    s,    s, 0.0f,    s,    c,    s, 0.0f);
        __m256 fifth_2 = _mm256_setr_ps(   s,    s,    c, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);

        __m256 res_1 = _mm256_add_ps(_mm256_mul_ps(_mm256_mul_ps(first_1, second_1), third_1), _mm256_mul_ps(forth_1, fifth_1));
        __m256 res_2 = _mm256_add_ps(_mm256_mul_ps(_mm256_mul_ps(first_2, second_2), third_2), _mm256_mul_ps(forth_2, fifth_2));

        return m4(mm_extract_f32<0>(res_1), mm_extract_f32<4>(res_1), mm_extract_f32<0>(res_2), 0.0f,
                  mm_extract_f32<1>(res_1), mm_extract_f32<5>(res_1), mm_extract_f32<1>(res_2), 0.0f,
                  mm_extract_f32<2>(res_1), mm_extract_f32<6>(res_1), mm_extract_f32<2>(res_2), 0.0f,
                                      0.0f,                     0.0f,                     0.0f, 1.0f);
    #else
        __m128 first_1 = _mm_setr_ps(v.x, v.y, v.z, 0.0f);
        __m128 first_2 = first_1;
        __m128 first_3 = first_1;

        __m128 second_1 = _mm_set_ps(v.x, v.x, v.x, 0.0f);
        __m128 second_2 = _mm_set_ps(v.y, v.y, v.y, 0.0f);
        __m128 second_3 = _mm_set_ps(v.z, v.z, v.z, 0.0f);

        __m128 third_1 = _mm_setr_ps(invc, invc, invc, 0.0f);
        __m128 third_2 = third_1;
        __m128 third_3 = third_1;

        __m128 forth_1 = _mm_setr_ps(1.0f,  v.z, -v.y, 0.0f);
        __m128 forth_2 = _mm_setr_ps(-v.z, 1.0f,  v.x, 0.0f);
        __m128 forth_3 = _mm_setr_ps( v.y, -v.x, 1.0f, 0.0f);

        __m128 fifth_1 = _mm_setr_ps(c, s, s, 0.0f);
        __m128 fifth_2 = _mm_setr_ps(s, c, s, 0.0f);
        __m128 fifth_3 = _mm_setr_ps(s, s, c, 0.0f);

        __m128 res_1 = _mm_add_ps(_mm_mul_ps(_mm_mul_ps(first_1, second_1), third_1), _mm_mul_ps(forth_1, fifth_1));
        __m128 res_2 = _mm_add_ps(_mm_mul_ps(_mm_mul_ps(first_2, second_2), third_2), _mm_mul_ps(forth_2, fifth_2));
        __m128 res_3 = _mm_add_ps(_mm_mul_ps(_mm_mul_ps(first_3, second_3), third_3), _mm_mul_ps(forth_3, fifth_3));

        return m4(mm_extract_f32<0>(res_1), mm_extract_f32<0>(res_2), mm_extract_f32<0>(res_3), 0.0f,
                  mm_extract_f32<1>(res_1), mm_extract_f32<1>(res_2), mm_extract_f32<1>(res_3), 0.0f,
                  mm_extract_f32<2>(res_1), mm_extract_f32<2>(res_2), mm_extract_f32<2>(res_3), 0.0f,
                                      0.0f,                     0.0f,                     0.0f, 1.0f);
    #endif
    }

    static m4 __vectorcall reflection_x()
    {
        return m4(-1.0f, 0.0f, 0.0f, 0.0f,
                   0.0f, 1.0f, 0.0f, 0.0f,
                   0.0f, 0.0f, 1.0f, 0.0f,
                   0.0f, 0.0f, 0.0f, 1.0f);
    }

    static m4 __vectorcall reflection_y()
    {
        return m4(1.0f,  0.0f, 0.0f, 0.0f,
                  0.0f, -1.0f, 0.0f, 0.0f,
                  0.0f,  0.0f, 1.0f, 0.0f,
                  0.0f,  0.0f, 0.0f, 1.0f);
    }

    static m4 __vectorcall reflection_z()
    {
        return m4(1.0f, 0.0f,  0.0f, 0.0f,
                  0.0f, 1.0f,  0.0f, 0.0f,
                  0.0f, 0.0f, -1.0f, 0.0f,
                  0.0f, 0.0f,  0.0f, 1.0f);
    }

    static m4 __vectorcall reflection_xy()
    {
        return m4(-1.0f,  0.0f, 0.0f, 0.0f,
                   0.0f, -1.0f, 0.0f, 0.0f,
                   0.0f,  0.0f, 1.0f, 0.0f,
                   0.0f,  0.0f, 0.0f, 1.0f);
    }

    static m4 __vectorcall reflection_xz()
    {
        return m4(-1.0f, 0.0f,  0.0f, 0.0f,
                   0.0f, 1.0f,  0.0f, 0.0f,
                   0.0f, 0.0f, -1.0f, 0.0f,
                   0.0f, 0.0f,  0.0f, 1.0f);
    }

    static m4 __vectorcall reflection_yz()
    {
        return m4(1.0f,  0.0f,  0.0f, 0.0f,
                  0.0f, -1.0f,  0.0f, 0.0f,
                  0.0f,  0.0f, -1.0f, 0.0f,
                  0.0f,  0.0f,  0.0f, 1.0f);
    }

    static m4 __vectorcall reflection_xyz()
    {
        return m4(-1.0f,  0.0f,  0.0f, 0.0f,
                   0.0f, -1.0f,  0.0f, 0.0f,
                   0.0f,  0.0f, -1.0f, 0.0f,
                   0.0f,  0.0f,  0.0f, 1.0f);
    }

    static m4 __vectorcall shearing_x(f32 shy, f32 shz)
    {
        return m4(1.0f, 0.0f, 0.0f, 0.0f,
                   shy, 1.0f, 0.0f, 0.0f,
                   shz, 0.0f, 1.0f, 0.0f,
                  0.0f, 0.0f, 0.0f, 1.0f);
    }

    static m4 __vectorcall shearing_y(f32 shx, f32 shz)
    {
        return m4(1.0f,  shx, 0.0f, 0.0f,
                  0.0f, 1.0f, 0.0f, 0.0f,
                  0.0f,  shz, 1.0f, 0.0f,
                  0.0f, 0.0f, 0.0f, 1.0f);
    }

    static m4 __vectorcall shearing_z(f32 shx, f32 shy)
    {
        return m4(1.0f, 0.0f,  shx, 0.0f,
                  0.0f, 1.0f,  shy, 0.0f,
                  0.0f, 0.0f, 1.0f, 0.0f,
                  0.0f, 0.0f, 0.0f, 1.0f);
    }

    static m4 __vectorcall shearing(f32 shxy, f32 shxz, f32 shyx, f32 shyz, f32 shzx, f32 shzy)
    {
        return m4(1.0f, shyx, shzx, 0.0f,
                  shxy, 1.0f, shzy, 0.0f,
                  shxz, shyz, 1.0f, 0.0f,
                  0.0f, 0.0f, 0.0f, 1.0f);
    }

    static m4 __vectorcall translation(f32 dx, f32 dy, f32 dz)
    {
        return m4(1.0f, 0.0f, 0.0f,   dx,
                  0.0f, 1.0f, 0.0f,   dy,
                  0.0f, 0.0f, 1.0f,   dz,
                  0.0f, 0.0f, 0.0f, 1.0f);
    }

    static m4 __vectorcall translation(v4 dv)
    {
        return m4(1.0f, 0.0f, 0.0f, dv.x,
                  0.0f, 1.0f, 0.0f, dv.y,
                  0.0f, 0.0f, 1.0f, dv.z,
                  0.0f, 0.0f, 0.0f, 1.0f);
    }

    static m4 __vectorcall ortho_lh(f32 left, f32 right, f32 bottom, f32 top, f32 _near, f32 _far) // returns z = [-1; 1]
    {
        return m4(2.0f/(right-left),              0.0f,              0.0f, (-right-left)/(right-left),
                               0.0f, 2.0f/(top-bottom),              0.0f, (-top-bottom)/(top-bottom),
                               0.0f,              0.0f, 2.0f/(_far-_near), (-_far-_near)/(_far-_near),
                               0.0f,              0.0f,              0.0f,                       1.0f);
    }

    static m4 __vectorcall ortho_rh(f32 left, f32 right, f32 bottom, f32 top, f32 _near, f32 _far) // returns z = [-1; 1]
    {
        return m4(2.0f/(right-left),              0.0f,               0.0f, (-right-left)/(right-left),
                               0.0f, 2.0f/(top-bottom),               0.0f, (-top-bottom)/(top-bottom),
                               0.0f,              0.0f, -2.0f/(_far-_near),  (_far+_near)/(_far-_near),
                               0.0f,              0.0f,               0.0f,                       1.0f);
    }

    static m4 __vectorcall persp_lh(f32 left, f32 right, f32 bottom, f32 top, f32 _near, f32 _far) // returns z = [-1; 1]
    {
        return m4(2.0f*_near/(right-left),                    0.0f,                      0.0f,                            0.0f,
                                     0.0f, 2.0f*_near/(top-bottom),                      0.0f,                            0.0f,
                                     0.0f,                    0.0f, (_far+_near)/(_far-_near), -(2.0f*_near*_far)/(_far-_near),
                                     0.0f,                    0.0f,                      1.0f,                            0.0f);
    }

    static m4 __vectorcall persp_lh(f32 aspect, deg fov, f32 _near, f32 _far) // returns z = [-1; 1]
    {
        f32 tanfov2 = tanf(fov / 2.0f * g_f32_PI / 180.0f);

        return m4(1.0f/(aspect*tanfov2),         0.0f,                      0.0f,                            0.0f,
                                   0.0f, 1.0f/tanfov2,                      0.0f,                            0.0f,
                                   0.0f,         0.0f, (_far+_near)/(_far-_near), -(2.0f*_near*_far)/(_far-_near),
                                   0.0f,         0.0f,                      1.0f,                            0.0f);
    }

    static m4 __vectorcall persp_rh(f32 left, f32 right, f32 bottom, f32 top, f32 _near, f32 _far) // returns z = [-1; 1]
    {
        return m4(2.0f*_near/(right-left),                    0.0f,                      0.0f,                           0.0f,
                                     0.0f, 2.0f*_near/(top-bottom),                      0.0f,                           0.0f,
                                     0.0f,                    0.0f, (_far+_near)/(_far-_near), (2.0f*_near*_far)/(_far-_near),
                                     0.0f,                    0.0f,                     -1.0f,                           0.0f);
    }

    static m4 __vectorcall persp_rh(f32 aspect, deg fov, f32 _near, f32 _far) // returns z = [-1; 1]
    {
        f32 tanfov2 = tanf(fov / 2.0f * g_f32_PI / 180.0f);
        
        return m4(1.0f/(aspect*tanfov2),         0.0f,                      0.0f,                           0.0f,
                                   0.0f, 1.0f/tanfov2,                      0.0f,                           0.0f,
                                   0.0f,         0.0f, (_far+_near)/(_far-_near), (2.0f*_near*_far)/(_far-_near),
                                   0.0f,         0.0f,                     -1.0f,                           0.0f);
    }

    m4& __vectorcall operator+=(m4 r)
    {
    #if REV_ISA >= REV_ISA_AVX512
        zmm = _mm512_add_ps(zmm, r.zmm);
    #elif REV_ISA >= REV_ISA_AVX
        ymm0 = _mm256_add_ps(ymm0, r.ymm0);
        ymm1 = _mm256_add_ps(ymm1, r.ymm1);
    #else
        xmm0 = _mm_add_ps(xmm0, r.xmm0);
        xmm1 = _mm_add_ps(xmm1, r.xmm1);
        xmm2 = _mm_add_ps(xmm2, r.xmm2);
        xmm3 = _mm_add_ps(xmm3, r.xmm3);
    #endif
        return *this;
    }

    m4& __vectorcall operator-=(m4 r)
    {
    #if REV_ISA >= REV_ISA_AVX512
        zmm = _mm512_sub_ps(zmm, r.zmm);
    #elif REV_ISA >= REV_ISA_AVX
        ymm0 = _mm256_sub_ps(ymm0, r.ymm0);
        ymm1 = _mm256_sub_ps(ymm1, r.ymm1);
    #else
        xmm0 = _mm_sub_ps(xmm0, r.xmm0);
        xmm1 = _mm_sub_ps(xmm1, r.xmm1);
        xmm2 = _mm_sub_ps(xmm2, r.xmm2);
        xmm2 = _mm_sub_ps(xmm3, r.xmm3);
    #endif
        return *this;
    }

    m4& __vectorcall operator*=(m4 r)
    {
        __m128 r_c0 = _mm_setr_ps(r.e00, r.e10, r.e20, r.e30);
        __m128 r_c1 = _mm_setr_ps(r.e01, r.e11, r.e21, r.e31);
        __m128 r_c2 = _mm_setr_ps(r.e02, r.e12, r.e22, r.e32);
        __m128 r_c3 = _mm_setr_ps(r.e03, r.e13, r.e23, r.e33);

    #if REV_ISA >= REV_ISA_AVX512
        zmm = _mm512_setr_ps(_mm_cvtss_f32(_mm_dp_ps(xmm0, r_c0, 0xF1)), _mm_cvtss_f32(_mm_dp_ps(xmm0, r_c1, 0xF1)), _mm_cvtss_f32(_mm_dp_ps(xmm0, r_c2, 0xF1)), _mm_cvtss_f32(_mm_dp_ps(xmm0, r_c3, 0xF1)),
                             _mm_cvtss_f32(_mm_dp_ps(xmm1, r_c0, 0xF1)), _mm_cvtss_f32(_mm_dp_ps(xmm1, r_c1, 0xF1)), _mm_cvtss_f32(_mm_dp_ps(xmm1, r_c2, 0xF1)), _mm_cvtss_f32(_mm_dp_ps(xmm1, r_c3, 0xF1)),
                             _mm_cvtss_f32(_mm_dp_ps(xmm2, r_c0, 0xF1)), _mm_cvtss_f32(_mm_dp_ps(xmm2, r_c1, 0xF1)), _mm_cvtss_f32(_mm_dp_ps(xmm2, r_c2, 0xF1)), _mm_cvtss_f32(_mm_dp_ps(xmm2, r_c3, 0xF1)),
                             _mm_cvtss_f32(_mm_dp_ps(xmm3, r_c0, 0xF1)), _mm_cvtss_f32(_mm_dp_ps(xmm3, r_c1, 0xF1)), _mm_cvtss_f32(_mm_dp_ps(xmm3, r_c2, 0xF1)), _mm_cvtss_f32(_mm_dp_ps(xmm3, r_c3, 0xF1)));
    #elif REV_ISA >= REV_ISA_AVX
        ymm0 = _mm256_setr_ps(_mm_cvtss_f32(_mm_dp_ps(xmm0, r_c0, 0xF1)), _mm_cvtss_f32(_mm_dp_ps(xmm0, r_c1, 0xF1)), _mm_cvtss_f32(_mm_dp_ps(xmm0, r_c2, 0xF1)), _mm_cvtss_f32(_mm_dp_ps(xmm0, r_c3, 0xF1)),
                              _mm_cvtss_f32(_mm_dp_ps(xmm1, r_c0, 0xF1)), _mm_cvtss_f32(_mm_dp_ps(xmm1, r_c1, 0xF1)), _mm_cvtss_f32(_mm_dp_ps(xmm1, r_c2, 0xF1)), _mm_cvtss_f32(_mm_dp_ps(xmm1, r_c3, 0xF1)));
        ymm1 = _mm256_setr_ps(_mm_cvtss_f32(_mm_dp_ps(xmm2, r_c0, 0xF1)), _mm_cvtss_f32(_mm_dp_ps(xmm2, r_c1, 0xF1)), _mm_cvtss_f32(_mm_dp_ps(xmm2, r_c2, 0xF1)), _mm_cvtss_f32(_mm_dp_ps(xmm2, r_c3, 0xF1)),
                              _mm_cvtss_f32(_mm_dp_ps(xmm3, r_c0, 0xF1)), _mm_cvtss_f32(_mm_dp_ps(xmm3, r_c1, 0xF1)), _mm_cvtss_f32(_mm_dp_ps(xmm3, r_c2, 0xF1)), _mm_cvtss_f32(_mm_dp_ps(xmm3, r_c3, 0xF1)));
    #else
        xmm0 = _mm_setr_ps(_mm_cvtss_f32(_mm_dp_ps(xmm0, r_c0, 0xF1)), _mm_cvtss_f32(_mm_dp_ps(xmm0, r_c1, 0xF1)), _mm_cvtss_f32(_mm_dp_ps(xmm0, r_c2, 0xF1)), _mm_cvtss_f32(_mm_dp_ps(xmm0, r_c3, 0xF1)));
        xmm1 = _mm_setr_ps(_mm_cvtss_f32(_mm_dp_ps(xmm1, r_c0, 0xF1)), _mm_cvtss_f32(_mm_dp_ps(xmm1, r_c1, 0xF1)), _mm_cvtss_f32(_mm_dp_ps(xmm1, r_c2, 0xF1)), _mm_cvtss_f32(_mm_dp_ps(xmm1, r_c3, 0xF1)));
        xmm2 = _mm_setr_ps(_mm_cvtss_f32(_mm_dp_ps(xmm2, r_c0, 0xF1)), _mm_cvtss_f32(_mm_dp_ps(xmm2, r_c1, 0xF1)), _mm_cvtss_f32(_mm_dp_ps(xmm2, r_c2, 0xF1)), _mm_cvtss_f32(_mm_dp_ps(xmm2, r_c3, 0xF1)));
        xmm2 = _mm_setr_ps(_mm_cvtss_f32(_mm_dp_ps(xmm3, r_c0, 0xF1)), _mm_cvtss_f32(_mm_dp_ps(xmm3, r_c1, 0xF1)), _mm_cvtss_f32(_mm_dp_ps(xmm3, r_c2, 0xF1)), _mm_cvtss_f32(_mm_dp_ps(xmm3, r_c3, 0xF1)));
    #endif

        return *this;
    }
    
    m4& __vectorcall operator+=(f32 r)
    {
    #if REV_ISA >= REV_ISA_AVX512
        zmm = _mm512_add_ps(zmm, _mm512_set1_ps(r));
    #elif REV_ISA >= REV_ISA_AVX
        __m256 vval = _mm256_set1_ps(r);
        ymm0 = _mm256_add_ps(ymm0, vval);
        ymm1 = _mm256_add_ps(ymm1, vval);
    #else
        __m128 vval = _mm_set_ps1(r);
        xmm0 = _mm_add_ps(xmm0, vval);
        xmm1 = _mm_add_ps(xmm1, vval);
        xmm2 = _mm_add_ps(xmm2, vval);
        xmm3 = _mm_add_ps(xmm3, vval);
    #endif
        return *this;
    }

    m4& __vectorcall operator-=(f32 r)
    {
    #if REV_ISA >= REV_ISA_AVX512
        zmm = _mm512_sub_ps(zmm, _mm512_set1_ps(r));
    #elif REV_ISA >= REV_ISA_AVX
        __m256 vval = _mm256_set1_ps(r);
        ymm0 = _mm256_sub_ps(ymm0, vval);
        ymm1 = _mm256_sub_ps(ymm1, vval);
    #else
        __m128 vval = _mm_set_ps1(r);
        xmm0 = _mm_sub_ps(xmm0, vval);
        xmm1 = _mm_sub_ps(xmm1, vval);
        xmm2 = _mm_sub_ps(xmm2, vval);
        xmm3 = _mm_sub_ps(xmm3, vval);
    #endif
        return *this;
    }

    m4& __vectorcall operator*=(f32 r)
    {
    #if REV_ISA >= REV_ISA_AVX512
        zmm = _mm512_mul_ps(zmm, _mm512_set1_ps(r));
    #elif REV_ISA >= REV_ISA_AVX
        __m256 vval = _mm256_set1_ps(r);
        ymm0 = _mm256_mul_ps(ymm0, vval);
        ymm1 = _mm256_mul_ps(ymm1, vval);
    #else
        __m128 vval = _mm_set_ps1(r);
        xmm0 = _mm_mul_ps(xmm0, vval);
        xmm1 = _mm_mul_ps(xmm1, vval);
        xmm2 = _mm_mul_ps(xmm2, vval);
        xmm3 = _mm_mul_ps(xmm3, vval);
    #endif
        return *this;
    }

    m4& __vectorcall operator/=(f32 r)
    {
    #if REV_ISA >= REV_ISA_AVX512
        zmm = _mm512_div_ps(zmm, _mm512_set1_ps(r));
    #elif REV_ISA >= REV_ISA_AVX
        __m256 vval = _mm256_set1_ps(r);
        ymm0 = _mm256_div_ps(ymm0, vval);
        ymm1 = _mm256_div_ps(ymm1, vval);
    #else
        __m128 vval = _mm_set_ps1(r);
        xmm0 = _mm_div_ps(xmm0, vval);
        xmm1 = _mm_div_ps(xmm1, vval);
        xmm2 = _mm_div_ps(xmm2, vval);
        xmm3 = _mm_div_ps(xmm3, vval);
    #endif
        return *this;
    }

    v4  operator[](u8 index) const { return cast<v4 *>(this)[index]; }
    v4& operator[](u8 index)       { return cast<v4 *>(this)[index]; }
};

REV_INLINE m4 __vectorcall operator+(m4 l, m4 r)
{
#if REV_ISA >= REV_ISA_AVX512
    return m4(_mm512_add_ps(l.zmm, r.zmm));
#elif REV_ISA >= REV_ISA_AVX
    return m4(_mm256_add_ps(l.ymm0, r.ymm0),
              _mm256_add_ps(l.ymm1, r.ymm1));
#else
    return m4(_mm_add_ps(l.xmm0, r.xmm0),
              _mm_add_ps(l.xmm1, r.xmm1),
              _mm_add_ps(l.xmm2, r.xmm2),
              _mm_add_ps(l.xmm3, r.xmm3));
#endif
}

REV_INLINE m4 __vectorcall operator-(m4 l, m4 r)
{
#if REV_ISA >= REV_ISA_AVX512
    return m4(_mm512_sub_ps(l.zmm, r.zmm));
#elif REV_ISA >= REV_ISA_AVX
    return m4(_mm256_sub_ps(l.ymm0, r.ymm0),
              _mm256_sub_ps(l.ymm1, r.ymm1));
#else
    return m4(_mm_sub_ps(l.xmm0, r.xmm0),
              _mm_sub_ps(l.xmm1, r.xmm1),
              _mm_sub_ps(l.xmm2, r.xmm2),
              _mm_sub_ps(l.xmm3, r.xmm3));
#endif
}

REV_INLINE m4 __vectorcall operator*(m4 l, m4 r)
{
    __m128 r_c0 = _mm_setr_ps(r.e00, r.e10, r.e20, r.e30);
    __m128 r_c1 = _mm_setr_ps(r.e01, r.e11, r.e21, r.e31);
    __m128 r_c2 = _mm_setr_ps(r.e02, r.e12, r.e22, r.e32);
    __m128 r_c3 = _mm_setr_ps(r.e03, r.e13, r.e23, r.e33);

    return m4(_mm_cvtss_f32(_mm_dp_ps(l.xmm0, r_c0, 0xF1)), _mm_cvtss_f32(_mm_dp_ps(l.xmm0, r_c1, 0xF1)), _mm_cvtss_f32(_mm_dp_ps(l.xmm0, r_c2, 0xF1)), _mm_cvtss_f32(_mm_dp_ps(l.xmm0, r_c3, 0xF1)),
              _mm_cvtss_f32(_mm_dp_ps(l.xmm1, r_c0, 0xF1)), _mm_cvtss_f32(_mm_dp_ps(l.xmm1, r_c1, 0xF1)), _mm_cvtss_f32(_mm_dp_ps(l.xmm1, r_c2, 0xF1)), _mm_cvtss_f32(_mm_dp_ps(l.xmm1, r_c3, 0xF1)),
              _mm_cvtss_f32(_mm_dp_ps(l.xmm2, r_c0, 0xF1)), _mm_cvtss_f32(_mm_dp_ps(l.xmm2, r_c1, 0xF1)), _mm_cvtss_f32(_mm_dp_ps(l.xmm2, r_c2, 0xF1)), _mm_cvtss_f32(_mm_dp_ps(l.xmm2, r_c3, 0xF1)),
              _mm_cvtss_f32(_mm_dp_ps(l.xmm3, r_c0, 0xF1)), _mm_cvtss_f32(_mm_dp_ps(l.xmm3, r_c1, 0xF1)), _mm_cvtss_f32(_mm_dp_ps(l.xmm3, r_c2, 0xF1)), _mm_cvtss_f32(_mm_dp_ps(l.xmm3, r_c3, 0xF1)));
}

REV_INLINE m4 __vectorcall operator+(f32 l, m4 r)
{
#if REV_ISA >= REV_ISA_AVX512
    return m4(_mm512_add_ps(_mm512_set1_ps(l), r.zmm));
#elif REV_ISA >= REV_ISA_AVX
    __m256 vval = _mm256_set1_ps(l);
    return m4(_mm256_add_ps(vval, r.ymm0),
              _mm256_add_ps(vval, r.ymm1));
#else
    __m128 vval = _mm_set_ps1(l);
    return m4(_mm_add_ps(vval, r.xmm0),
              _mm_add_ps(vval, r.xmm1),
              _mm_add_ps(vval, r.xmm2),
              _mm_add_ps(vval, r.xmm3));
#endif
}

REV_INLINE m4 __vectorcall operator-(m4 l, f32 r)
{
#if REV_ISA >= REV_ISA_AVX512
    return m4(_mm512_sub_ps(l.zmm, _mm512_set1_ps(r)));
#elif REV_ISA >= REV_ISA_AVX
    __m256 vval = _mm256_set1_ps(r);
    return m4(_mm256_sub_ps(l.ymm0, vval),
              _mm256_sub_ps(l.ymm1, vval));
#else
    __m128 vval = _mm_set_ps1(r);
    return m4(_mm_sub_ps(l.xmm0, vval),
              _mm_sub_ps(l.xmm1, vval),
              _mm_sub_ps(l.xmm2, vval),
              _mm_sub_ps(l.xmm3, vval));
#endif
}

REV_INLINE m4 __vectorcall operator*(f32 l, m4 r)
{
#if REV_ISA >= REV_ISA_AVX512
    return m4(_mm512_mul_ps(_mm512_set1_ps(l), r.zmm));
#elif REV_ISA >= REV_ISA_AVX
    __m256 vval = _mm256_set1_ps(l);
    return m4(_mm256_mul_ps(vval, r.ymm0),
              _mm256_mul_ps(vval, r.ymm1));
#else
    __m128 vval = _mm_set_ps1(l);
    return m4(_mm_mul_ps(vval, r.xmm0),
              _mm_mul_ps(vval, r.xmm1),
              _mm_mul_ps(vval, r.xmm2),
              _mm_mul_ps(vval, r.xmm3));
#endif
}

REV_INLINE m4 __vectorcall operator/(m4 l, f32 r)
{
#if REV_ISA >= REV_ISA_AVX512
    return m4(_mm512_div_ps(l.zmm, _mm512_set1_ps(r)));
#elif REV_ISA >= REV_ISA_AVX
    __m256 vval = _mm256_set1_ps(r);
    return m4(_mm256_div_ps(l.ymm0, vval),
              _mm256_div_ps(l.ymm1, vval));
#else
    __m128 vval = _mm_set_ps1(r);
    return m4(_mm_div_ps(l.xmm0, vval),
              _mm_div_ps(l.xmm1, vval),
              _mm_div_ps(l.xmm2, vval),
              _mm_div_ps(l.xmm3, vval));
#endif
}

REV_INLINE v4 __vectorcall operator*(m4 l, v4 r)
{
    return v4(_mm_cvtss_f32(_mm_dp_ps(l.xmm0, r.mm, 0xF1)),
              _mm_cvtss_f32(_mm_dp_ps(l.xmm1, r.mm, 0xF1)),
              _mm_cvtss_f32(_mm_dp_ps(l.xmm2, r.mm, 0xF1)),
              _mm_cvtss_f32(_mm_dp_ps(l.xmm3, r.mm, 0xF1)));
}

REV_INLINE bool __vectorcall operator==(m4 l, m4 r)
{
    return mm512_equals(&l, &r);
}

REV_INLINE bool __vectorcall operator!=(m4 l, m4 r)
{
    return !mm512_equals(&l, &r);
}

// @TODO(Roman): Move to the camera class.

REV_INLINE m4 __vectorcall CameraToWorldLH(v4 camera, v4 target, v4 up)
{
    __m128 z_axis     = _mm_sub_ps(target.mm, camera.mm);
    __m128 z_axis_len = _mm_sqrt_ps(_mm_dp_ps(z_axis, z_axis, 0x7F));
           z_axis     = _mm_div_ps(z_axis, z_axis_len);

    __m128 x_axis     = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps( up.mm,  up.mm, cast<s32>(MM_SHUFFLE::YZXW)),
                                              _mm_shuffle_ps(z_axis, z_axis, cast<s32>(MM_SHUFFLE::ZXYW))),
                                   _mm_mul_ps(_mm_shuffle_ps( up.mm,  up.mm, cast<s32>(MM_SHUFFLE::ZXYW)),
                                              _mm_shuffle_ps(z_axis, z_axis, cast<s32>(MM_SHUFFLE::YZXW))));
    __m128 x_axis_len = _mm_sqrt_ps(_mm_dp_ps(x_axis, x_axis, 0x7F));
           x_axis     = _mm_div_ps(x_axis, x_axis_len);

    __m128 y_axis     = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(z_axis, z_axis, cast<s32>(MM_SHUFFLE::YZXW)),
                                              _mm_shuffle_ps(x_axis, x_axis, cast<s32>(MM_SHUFFLE::ZXYW))),
                                   _mm_mul_ps(_mm_shuffle_ps(z_axis, z_axis, cast<s32>(MM_SHUFFLE::ZXYW)),
                                              _mm_shuffle_ps(x_axis, x_axis, cast<s32>(MM_SHUFFLE::YZXW))));

    __m128 translate  = _mm_setr_ps(-_mm_cvtss_f32(_mm_dp_ps(x_axis, camera.mm, 0x71)),
                                    -_mm_cvtss_f32(_mm_dp_ps(y_axis, camera.mm, 0x71)),
                                    -_mm_cvtss_f32(_mm_dp_ps(z_axis, camera.mm, 0x71)),
                                     0.0f);

    return m4(mm_extract_f32<0>(x_axis), mm_extract_f32<1>(x_axis), mm_extract_f32<2>(x_axis), mm_extract_f32<0>(translate),
              mm_extract_f32<0>(y_axis), mm_extract_f32<1>(y_axis), mm_extract_f32<2>(y_axis), mm_extract_f32<1>(translate),
              mm_extract_f32<0>(z_axis), mm_extract_f32<1>(z_axis), mm_extract_f32<2>(z_axis), mm_extract_f32<2>(translate),
                                   0.0f,                      0.0f,                      0.0f,                         1.0f);
}

REV_INLINE m4 __vectorcall CameraToWorldRH(v4 camera, v4 target, v4 up)
{
    __m128 z_axis     = _mm_sub_ps(camera.mm, target.mm);
    __m128 z_axis_len = _mm_sqrt_ps(_mm_dp_ps(z_axis, z_axis, 0x7F));
           z_axis     = _mm_div_ps(z_axis, z_axis_len);

    __m128 x_axis     = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps( up.mm,  up.mm, cast<s32>(MM_SHUFFLE::YZXW)),
                                              _mm_shuffle_ps(z_axis, z_axis, cast<s32>(MM_SHUFFLE::ZXYW))),
                                   _mm_mul_ps(_mm_shuffle_ps( up.mm,  up.mm, cast<s32>(MM_SHUFFLE::ZXYW)),
                                              _mm_shuffle_ps(z_axis, z_axis, cast<s32>(MM_SHUFFLE::YZXW))));
    __m128 x_axis_len = _mm_sqrt_ps(_mm_dp_ps(x_axis, x_axis, 0x7F));
           x_axis     = _mm_div_ps(x_axis, x_axis_len);

    __m128 y_axis     = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(z_axis, z_axis, cast<s32>(MM_SHUFFLE::YZXW)),
                                              _mm_shuffle_ps(x_axis, x_axis, cast<s32>(MM_SHUFFLE::ZXYW))),
                                   _mm_mul_ps(_mm_shuffle_ps(z_axis, z_axis, cast<s32>(MM_SHUFFLE::ZXYW)),
                                              _mm_shuffle_ps(x_axis, x_axis, cast<s32>(MM_SHUFFLE::YZXW))));

    __m128 translate  = _mm_setr_ps(-_mm_cvtss_f32(_mm_dp_ps(x_axis, camera.mm, 0x71)),
                                    -_mm_cvtss_f32(_mm_dp_ps(y_axis, camera.mm, 0x71)),
                                    -_mm_cvtss_f32(_mm_dp_ps(z_axis, camera.mm, 0x71)),
                                     0.0f);

    return m4(mm_extract_f32<0>(x_axis), mm_extract_f32<1>(x_axis), mm_extract_f32<2>(x_axis), mm_extract_f32<0>(translate),
              mm_extract_f32<0>(y_axis), mm_extract_f32<1>(y_axis), mm_extract_f32<2>(y_axis), mm_extract_f32<1>(translate),
              mm_extract_f32<0>(z_axis), mm_extract_f32<1>(z_axis), mm_extract_f32<2>(z_axis), mm_extract_f32<2>(translate),
                                   0.0f,                      0.0f,                      0.0f,                         1.0f);
}

}
#pragma pack(pop)
