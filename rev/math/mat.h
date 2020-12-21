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

union REV_INTRIN_TYPE m2 final
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
    xmm_u mm;

    REV_INLINE m2(                                  ) : mm(_mm_setzero_ps())                    {}
    REV_INLINE m2(f32 val                           ) : mm(_mm_set_ps1(val))                    {}
    REV_INLINE m2(f32 e00, f32 e01, f32 e10, f32 e11) : mm(_mm_setr_ps(e00, e01, e10, e11))     {}
    REV_INLINE m2(v2 c1, v2 c2                      ) : mm(_mm_setr_ps(c1.x, c2.x, c1.y, c2.y)) {}
    REV_INLINE m2(f32 arr[4]                        ) : mm(_mm_load_ps(arr))                    {}
    REV_INLINE m2(__m128 _mm                        ) : mm(_mm)                                 {}
    REV_INLINE m2(const m2& m                       ) : mm(m.mm)                                {}
    REV_INLINE m2(m2&& m                            ) : mm(m.mm)                                {}

    REV_INLINE m2& REV_VECTORCALL operator=(f32 val    ) { mm = _mm_set_ps1(val); return *this; }
    REV_INLINE m2& REV_VECTORCALL operator=(f32 arr[4] ) { mm = _mm_load_ps(arr); return *this; }
    REV_INLINE m2& REV_VECTORCALL operator=(__m128 _mm ) { mm = _mm;              return *this; }
    REV_INLINE m2& REV_VECTORCALL operator=(const m2& m) { mm = m.mm;             return *this; }
    REV_INLINE m2& REV_VECTORCALL operator=(m2&& m     ) { mm = m.mm;             return *this; }

    REV_INLINE f32 REV_VECTORCALL det() const
    {
        return e00 * e11 - e01 * e10;
    }

    REV_INLINE m2 REV_VECTORCALL inverse() const
    {
        __m128 vdet = _mm_set_ps1(e00 * e11 - e01 * e10);
        __m128 mult = _mm_setr_ps(1.0f, -1.0f, -1.0f, 1.0f);
        __m128 cof  = _mm_shuffle_ps(mm, mm, cast<s32>(MM_SHUFFLE::WYZX));
        return m2(_mm_div_ps(_mm_mul_ps(cof, mult), vdet));
    }

    REV_INLINE m2 REV_VECTORCALL transpose() const
    {
        return m2(_mm_shuffle_ps(mm, mm, cast<s32>(MM_SHUFFLE::XZYW)));
    }

    static REV_INLINE m2 REV_VECTORCALL identity()
    {
        return m2(1.0f, 0.0f,
                  0.0f, 1.0f);
    }

    static REV_INLINE m2 REV_VECTORCALL scaling(f32 nx, f32 ny)
    {
        return m2(  nx, 0.0f,
                  0.0f,   ny);
    }

    static REV_INLINE m2 REV_VECTORCALL rotation_z(rad angle)
    {
        f32 s = sinf(angle);
        f32 c = cosf(angle);
        return m2(c, -s,
                  s, c);
    }

    static REV_INLINE m2 REV_VECTORCALL reflection_x()
    {
        return m2(-1.0f, 0.0f,
                   0.0f, 1.0f);
    }

    static REV_INLINE m2 REV_VECTORCALL reflection_y()
    {
        return m2(1.0f,  0.0f,
                  0.0f, -1.0f);
    }

    static REV_INLINE m2 REV_VECTORCALL reflection_xy()
    {
        return m2(-1.0f,  0.0f,
                   0.0f, -1.0f);
    }

    static REV_INLINE m2 REV_VECTORCALL shearing(f32 shx, f32 shy)
    {
        return m2(1.0f,  shx,
                   shy, 1.0f);
    }

    static REV_INLINE m2 REV_VECTORCALL lerp(m2 start, m2 end, m2 percent)
    {
        return m2(_mm_add_ps(_mm_mul_ps(_mm_sub_ps(end.mm, start.mm), percent.mm), start.mm));
    }

    static REV_INLINE m2 REV_VECTORCALL invlerp(m2 start, m2 end, m2 value) // ret = [0, 1]
    {
        __m128 mm_start = start.mm;
        return m2(_mm_div_ps(_mm_sub_ps(value.mm, mm_start), _mm_sub_ps(end.mm, mm_start)));
    }

    static REV_INLINE m2 REV_VECTORCALL invlerp_n(m2 start, m2 end, m2 value) // ret = [-1, 1]
    {
        __m128 mm_start = start.mm;
        return m2(_mm_mul_ps(_mm_sub_ps(_mm_div_ps(_mm_sub_ps(value.mm, mm_start), _mm_sub_ps(end.mm, mm_start)), _mm_set_ps1(0.5f)), _mm_set_ps1(2.0f)));
    }

    static REV_INLINE m2 REV_VECTORCALL clamp(m2 val, m2 min, m2 max)
    {
        return m2(_mm_max_ps(min.mm, _mm_min_ps(max.mm, val.mm)));
    }

    REV_INLINE m2& REV_VECTORCALL operator+=(m2 r) { mm = _mm_add_ps(mm, r.mm); return *this; }
    REV_INLINE m2& REV_VECTORCALL operator-=(m2 r) { mm = _mm_sub_ps(mm, r.mm); return *this; }

    REV_INLINE m2& REV_VECTORCALL operator*=(m2 r)
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

    REV_INLINE m2& REV_VECTORCALL operator+=(f32 r) { mm = _mm_add_ps(mm, _mm_set_ps1(r)); return *this; }
    REV_INLINE m2& REV_VECTORCALL operator-=(f32 r) { mm = _mm_sub_ps(mm, _mm_set_ps1(r)); return *this; }
    REV_INLINE m2& REV_VECTORCALL operator*=(f32 r) { mm = _mm_mul_ps(mm, _mm_set_ps1(r)); return *this; }
    REV_INLINE m2& REV_VECTORCALL operator/=(f32 r) { mm = _mm_div_ps(mm, _mm_set_ps1(r)); return *this; }

    REV_INLINE v2  operator[](u8 i) const { REV_CHECK(i < 2); return cast<const v2 *>(this)[i]; }
    REV_INLINE v2& operator[](u8 i)       { REV_CHECK(i < 2); return cast<      v2 *>(this)[i]; }
};

REV_INLINE m2 REV_VECTORCALL operator+(m2 l, m2 r) { return m2(_mm_add_ps(l.mm, r.mm)); }
REV_INLINE m2 REV_VECTORCALL operator-(m2 l, m2 r) { return m2(_mm_sub_ps(l.mm, r.mm)); }

REV_INLINE m2 REV_VECTORCALL operator*(m2 l, m2 r)
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

REV_INLINE m2 REV_VECTORCALL operator+(f32 l, m2 r) { return m2(_mm_add_ps(_mm_set_ps1(l), r.mm)); }
REV_INLINE m2 REV_VECTORCALL operator-(m2 l, f32 r) { return m2(_mm_sub_ps(l.mm, _mm_set_ps1(r))); }
REV_INLINE m2 REV_VECTORCALL operator*(f32 l, m2 r) { return m2(_mm_mul_ps(_mm_set_ps1(l), r.mm)); }
REV_INLINE m2 REV_VECTORCALL operator/(m2 l, f32 r) { return m2(_mm_div_ps(l.mm, _mm_set_ps1(r))); }

REV_INLINE v2 REV_VECTORCALL operator*(m2 l, v2 r)
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

REV_INLINE bool REV_VECTORCALL operator==(m2 l, m2 r)
{
#if REV_ISA >= REV_ISA_AVX512
    return 0b1111 == _mm_cmpeq_epi32_mask(_mm_castps_si128(l.mm),
                                          _mm_castps_si128(r.mm));
#else
    return _mm_testc_si128(_mm_castps_si128(_mm_cmpeq_ps(l.mm, r.mm)),
                           _mm_set1_epi64x(REV_U64_MAX));
#endif
}

REV_INLINE bool REV_VECTORCALL operator!=(m2 l, m2 r)
{
#if REV_ISA >= REV_ISA_AVX512
    return _mm_cmpneq_epi32_mask(_mm_castps_si128(l.mm),
                                 _mm_castps_si128(r.mm));
#else
    return _mm_testc_si128(_mm_castps_si128(_mm_cmpneq_ps(l.mm, r.mm)),
                           _mm_set1_epi64x(REV_U64_MAX));
#endif
}

//
// m3
//

union REV_INTRIN_TYPE m3 final
{
    struct
    {
        f32 e00, e01, e02;
        f32 e10, e11, e12;
        f32 e20, e21, e22;
    };
    struct
    {
        v3 r0;
        v3 r1;
        v3 r2;
    };
    f32 e[9];
private:
#if REV_ISA >= REV_ISA_AVX
    ymm_u ymm0;
#endif
    struct
    {
        xmm_u xmm0;
        xmm_u xmm1;
    };
public:

#if REV_ISA >= REV_ISA_AVX
    REV_INLINE m3()
        : ymm0(_mm256_setzero_ps()),
          e22(0.0f)
    {
    }

    REV_INLINE m3(f32 val)
        : ymm0(_mm256_set1_ps(val)),
          e22(val)
    {
    }

    REV_INLINE m3(f32 e00, f32 e01, f32 e02,
                  f32 e10, f32 e11, f32 e12,
                  f32 e20, f32 e21, f32 e22)
        : ymm0(_mm256_setr_ps(e00, e01, e02,
                              e10, e11, e12,
                              e20, e21)),
          e22(e22)
    {
    }

    REV_INLINE m3(v3 c1, v3 c2, v3 c3)
        : ymm0(_mm256_setr_ps(c1.x, c2.x, c3.x,
                              c1.y, c2.y, c3.y,
                              c1.z, c2.z)),
          e22(c3.z)
    {
    }

    REV_INLINE m3(f32 arr[9])
        : ymm0(_mm256_loadu_ps(arr)),
          e22(arr[8])
    {
    }

    REV_INLINE m3(__m256 _ymm, f32 _e22)
        : ymm0(_ymm),
          e22(_e22)
    {
    }

    REV_INLINE m3(__m128 _xmm0, __m128 _xmm1, f32 _e22)
        : ymm0(_mm256_setr_m128(_xmm0, _xmm1)),
          e22(_e22)
    {
    }

    REV_INLINE m3(const m3& m)
        : ymm0(m.ymm0),
          e22(m.e22)
    {
    }

    REV_INLINE m3(m3&& m)
        : ymm0(m.ymm0),
          e22(m.e22)
    {
    }
#else
    REV_INLINE m3()
        : xmm0(_mm_setzero_ps()),
          xmm1(xmm0),
          e22(0.0f)
    {
    }

    REV_INLINE m3(f32 val)
        : xmm0(_mm256_set1_ps(val)),
          xmm1(xmm0),
          e22(val)
    {
    }

    REV_INLINE m3(f32 e00, f32 e01, f32 e02,
                  f32 e10, f32 e11, f32 e12,
                  f32 e20, f32 e21, f32 e22)
        : xmm0(_mm_setr_ps(e00, e01, e02, e10)),
          xmm1(_mm_setr_ps(e11, e12, e20, e21)),
          e22(e22)
    {
    }

    REV_INLINE m3(v3 c1, v3 c2, v3 c3)
        : xmm0(_mm_setr_ps(c1.x, c2.x, c3.x, c1.y)),
          xmm1(_mm_setr_ps(c2.y, c3.y, c1.z, c2.z)),
          e22(c3.z)
    {
    }

    REV_INLINE m3(f32 arr[9])
        : xmm0(_mm_loadu_ps(arr)),
          xmm1(_mm_loadu_ps(arr + 4)),
          e22(arr[8])
    {
    }

    REV_INLINE m3(__m128 _xmm0, __m128 _xmm1, f32 _e22)
        : xmm0(_xmm0),
          xmm1(_xmm1),
          e22(_e22)
    {
    }

    REV_INLINE m3(const m3& m)
        : xmm0(m.xmm0),
          xmm1(m.xmm1),
          e22(m.e22)
    {
    }

    REV_INLINE m3(m3&& m)
        : xmm0(m.xmm0),
          xmm1(m.xmm1),
          e22(m.e22)
    {
    }
#endif

#if REV_ISA >= REV_ISA_AVX512
    REV_INLINE m3(__m512 _zmm)
        : ymm0(*cast<__m256 *>(_zmm)),
          e22(mm_extract_f32<8>(_zmm))
    {
    }
#endif

#if REV_ISA >= REV_ISA_AVX && 0
    REV_INLINE m3& REV_VECTORCALL operator=(f32 val)
    {
        ymm0 = _mm256_set1_ps(val);
        e22  = val;
        return *this;
    }

    REV_INLINE m3& REV_VECTORCALL operator=(f32 arr[9])
    {
        ymm0 = _mm256_loadu_ps(arr);
        e22  = arr[8];
        return *this;
    }

    REV_INLINE m3& REV_VECTORCALL operator=(const m3& m)
    {
        ymm0 = m.ymm0;
        e22  = m.e22;
        return *this;
    }

    REV_INLINE m3& REV_VECTORCALL operator=(m3&& m)
    {
        ymm0 = m.ymm0;
        e22  = m.e22;
        return *this;
    }
#else
    REV_INLINE m3& REV_VECTORCALL operator=(f32 val)
    {
        xmm0 = _mm_set_ps1(val);
        xmm1 = xmm0;
        e22  = val;
        return *this;
    }

    REV_INLINE m3& REV_VECTORCALL operator=(f32 arr[9])
    {
        xmm0 = _mm_loadu_ps(arr);
        xmm1 = _mm_loadu_ps(arr + 4);
        e22  = arr[8];
        return *this;
    }

    REV_INLINE m3& REV_VECTORCALL operator=(const m3& m)
    {
        xmm0 = m.xmm0;
        xmm1 = m.xmm1;
        e22  = m.e22;
        return *this;
    }

    REV_INLINE m3& REV_VECTORCALL operator=(m3&& m)
    {
        xmm0 = m.xmm0;
        xmm1 = m.xmm1;
        e22  = m.e22;
        return *this;
    }
#endif

    REV_INLINE m3 REV_VECTORCALL transpose() const
    {
        return m3(e00, e10, e20,
                  e01, e11, e21,
                  e02, e12, e22);
    }

    REV_INLINE f32 REV_VECTORCALL det() const
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

    REV_INLINE m3 REV_VECTORCALL inverse() const
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

    static REV_INLINE m3 REV_VECTORCALL identity()
    {
        return m3(1.0f, 0.0f, 0.0f,
                  0.0f, 1.0f, 0.0f,
                  0.0f, 0.0f, 1.0f);
    }

    static REV_INLINE m3 REV_VECTORCALL scaling(f32 nx, f32 ny, f32 nz)
    {
        return m3(  nx, 0.0f, 0.0f,
                  0.0f,   ny, 0.0f,
                  0.0f, 0.0f,   nz);
    }

    static REV_INLINE m3 REV_VECTORCALL scaling(v3 v)
    {
        return m3( v.x, 0.0f, 0.0f,
                  0.0f,  v.y, 0.0f,
                  0.0f, 0.0f,  v.z);
    }

    static REV_INLINE m3 REV_VECTORCALL rotation_x(rad angle)
    {
        f32 s = sinf(angle);
        f32 c = cosf(angle);

        return m3(1.0f, 0.0f, 0.0f,
                  0.0f,    c,   -s,
                  0.0f,    s,    c);
    }

    static REV_INLINE m3 REV_VECTORCALL rotation_y(rad angle)
    {
        f32 s = sinf(angle);
        f32 c = cosf(angle);

        return m3(   c, 0.0f,    s,
                  0.0f, 1.0f, 0.0f,
                    -s, 0.0f,    c);
    }

    static REV_INLINE m3 REV_VECTORCALL rotation_z(rad angle)
    {
        f32 s = sinf(angle);
        f32 c = cosf(angle);

        return m3(   c,   -s, 0.0f,
                     s,    c, 0.0f,
                  0.0f, 0.0f, 1.0f);
    }

    static REV_INLINE m3 REV_VECTORCALL rotation(v3 v, rad angle)
    {
        // x*x*(1.0f-c)+  c     x*y*(1.0f-c)-z*s     x*z*(1.0-c)+y*s
        // y*x*(1.0f-c)+z*s     y*y*(1.0f-c)+  c     y*z*(1.0-c)-x*s
        // z*x*(1.0f-c)-y*s     z*y*(1.0f-c)+x*s     z*z*(1.0-c)+  c

        f32 s = sinf(angle);
        f32 c = cosf(angle);

        f32 invc = 1.0f - c;

    #if REV_ISA >= REV_ISA_AVX512
        __m512 first  = _mm512_setr_ps( v.x,  v.y,  v.z,  v.x,  v.y,  v.z,  v.x,  v.y,  v.z, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        __m512 second = _mm512_setr_ps( v.x,  v.x,  v.x,  v.y,  v.y,  v.y,  v.z,  v.z,  v.z, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        __m512 third  = _mm512_setr_ps(invc, invc, invc, invc, invc, invc, invc, invc, invc, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        __m512 forth  = _mm512_setr_ps(1.0f,  v.z, -v.y, -v.z, 1.0f,  v.x,  v.y, -v.x, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        __m512 fifth  = _mm512_setr_ps(   c,    s,    s,    s,    c,    s,    s,    s,    c, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);

        __m512 res = _mm512_add_ps(_mm512_mul_ps(_mm512_mul_ps(first, second), third), _mm512_mul_ps(forth, fifth));

        return m3(mm_extract_f32<0>(res), mm_extract_f32<3>(res), mm_extract_f32<6>(res),
                  mm_extract_f32<1>(res), mm_extract_f32<4>(res), mm_extract_f32<7>(res),
                  mm_extract_f32<2>(res), mm_extract_f32<5>(res), mm_extract_f32<8>(res));
    #elif REV_ISA >= REV_ISA_AVX
        __m256 first  = _mm256_setr_ps(v.x, v.y, v.z, v.x, v.y, v.z, v.x, v.y);
        __m256 second = _mm256_setr_ps(v.x, v.x, v.x, v.y, v.y, v.y, v.z, v.z);
        __m256 third  = _mm256_setr_ps(invc, invc, invc, invc, invc, invc, invc, invc);
        __m256 forth  = _mm256_setr_ps(1.0f, v.z, -v.y, -v.z, 1.0f, v.x, v.y, -v.x);
        __m256 fifth  = _mm256_setr_ps(c, s, s, s, c, s, s, s);

        __m256 res = _mm256_add_ps(_mm256_mul_ps(_mm256_mul_ps(first, second), third), _mm256_mul_ps(forth, fifth));

        return m3(mm_extract_f32<0>(res), mm_extract_f32<3>(res), mm_extract_f32<6>(res),
                  mm_extract_f32<1>(res), mm_extract_f32<4>(res), mm_extract_f32<7>(res),
                  mm_extract_f32<2>(res), mm_extract_f32<5>(res),  v.z * v.z * invc + c);
    #else
        __m128 first_1 = _mm_setr_ps(v.x, v.y, v.z, v.x);
        __m128 first_2 = _mm_setr_ps(v.y, v.z, v.x, v.y);

        __m128 second_1 = _mm_set_ps(v.x, v.x, v.x, v.y);
        __m128 second_2 = _mm_set_ps(v.y, v.y, v.z, v.z);

        __m128 third_1 = _mm_set1_ps(invc);
        __m128 third_2 = third_1;

        __m128 forth_1 = _mm_setr_ps(1.0f, v.z, -v.y, -v.z);
        __m128 forth_2 = _mm_setr_ps(1.0f, v.x,  v.y, -v.x);

        __m128 fifth_1 = _mm_setr_ps(c, s, s, s);
        __m128 fifth_2 = _mm_setr_ps(c, s, s, s);

        __m128 res_1 = _mm_add_ps(_mm_mul_ps(_mm_mul_ps(first_1, second_1), third_1), _mm_mul_ps(forth_1, fifth_1));
        __m128 res_2 = _mm_add_ps(_mm_mul_ps(_mm_mul_ps(first_2, second_2), third_2), _mm_mul_ps(forth_2, fifth_2));

        return m3(mm_extract_f32<0>(res_1), mm_extract_f32<3>(res_1), mm_extract_f32<2>(res_2),
                  mm_extract_f32<1>(res_1), mm_extract_f32<0>(res_2), mm_extract_f32<3>(res_2),
                  mm_extract_f32<2>(res_1), mm_extract_f32<1>(res_2),    v.z * v.z * invc + c);
    #endif
    }

    static REV_INLINE m3 REV_VECTORCALL reflection_x()
    {
        return m3(-1.0f, 0.0f, 0.0f,
                   0.0f, 1.0f, 0.0f,
                   0.0f, 0.0f, 1.0f);
    }

    static REV_INLINE m3 REV_VECTORCALL reflection_y()
    {
        return m3(1.0f,  0.0f, 0.0f,
                  0.0f, -1.0f, 0.0f,
                  0.0f,  0.0f, 1.0f);
    }

    static REV_INLINE m3 REV_VECTORCALL reflection_xy()
    {
        return m3(-1.0f,  0.0f, 0.0f,
                   0.0f, -1.0f, 0.0f,
                   0.0f,  0.0f, 1.0f);
    }

    static REV_INLINE m3 REV_VECTORCALL reflection_xz()
    {
        return m3(-1.0f, 0.0f,  0.0f,
                   0.0f, 1.0f,  0.0f,
                   0.0f, 0.0f, -1.0f);
    }

    static REV_INLINE m3 REV_VECTORCALL reflection_yz()
    {
        return m3(1.0f,  0.0f,  0.0f,
                  0.0f, -1.0f,  0.0f,
                  0.0f,  0.0f, -1.0f);
    }

    static REV_INLINE m3 REV_VECTORCALL reflection_xyz()
    {
        return m3(-1.0f,  0.0f,  0.0f,
                   0.0f, -1.0f,  0.0f,
                   0.0f,  0.0f, -1.0f);
    }

    static REV_INLINE m3 REV_VECTORCALL shearing_x(f32 shy, f32 shz)
    {
        return m3(1.0f, 0.0f, 0.0f,
                   shy, 1.0f, 0.0f,
                   shz, 0.0f, 1.0f);
    }

    static REV_INLINE m3 REV_VECTORCALL shearing_y(f32 shx, f32 shz)
    {
        return m3(1.0f,  shx, 0.0f,
                  0.0f, 1.0f, 0.0f,
                  0.0f,  shz, 1.0f);
    }

    static REV_INLINE m3 REV_VECTORCALL shearing_z(f32 shx, f32 shy)
    {
        return m3(1.0f, 0.0f, shx,
                  0.0f, 1.0f, shy,
                  0.0f, 0.0f, 1.0f);
    }

    static REV_INLINE m3 REV_VECTORCALL shearing(f32 shxy, f32 shxz, f32 shyx, f32 shyz, f32 shzx, f32 shzy)
    {
        return m3(1.0f, shyx, shzx,
                  shxy, 1.0f, shzy,
                  shxz, shyz, 1.0f);
    }

    static REV_INLINE m3 REV_VECTORCALL translation(f32 dx, f32 dy, f32 dz)
    {
        return m3(1.0f, 0.0f, dx,
                  0.0f, 1.0f, dy,
                  0.0f, 0.0f, dz);
    }

    static REV_INLINE m3 REV_VECTORCALL translation(v3 v)
    {
        return m3(1.0f, 0.0f, v.x,
                  0.0f, 1.0f, v.y,
                  0.0f, 0.0f, v.z);
    }

    static REV_INLINE m3 REV_VECTORCALL ortho_2d(f32 left, f32 right, f32 bottom, f32 top)
    {
        return m3(2.0f/(right-left),              0.0f, -(right+left)/(right-left),
                               0.0f, 2.0f/(top-bottom), -(top+bottom)/(top-bottom),
                               0.0f,              0.0f,                       1.0f);
    }

    static REV_INLINE m3 REV_VECTORCALL lerp(m3 start, m3 end, m3 percent)
    {
    #if REV_ISA >= REV_ISA_AVX
        __m256 start_ymm = start.ymm0;
        return m3(_mm256_add_ps(_mm256_mul_ps(_mm256_sub_ps(end.ymm0, start_ymm), percent.ymm0), start_ymm),
                  (end.e22 - start.e22) * percent.e22 + start.e22);
    #else
        __m128 start_xmm0 = start.xmm0;
        __m128 start_xmm1 = start.xmm1;
        return m3(_mm_add_ps(_mm_mul_ps(_mm_sub_ps(end.xmm0, start_xmm0), percent.xmm0), start_xmm0),
                  _mm_add_ps(_mm_mul_ps(_mm_sub_ps(end.xmm1, start_xmm1), percent.xmm1), start_xmm1),
                  (end.e22 - start.e22) * percent.e22 + start.e22);
    #endif
    }

    static REV_INLINE m3 REV_VECTORCALL invlerp(m3 start, m3 end, m3 value) // ret = [0, 1]
    {
    #if REV_ISA >= REV_ISA_AVX
        __m256 start_ymm0 = start.ymm0;
        return m3(_mm256_div_ps(_mm256_sub_ps(value.ymm0, start_ymm0), _mm256_sub_ps(end.ymm0, start_ymm0)),
                  (value.e22 - start.e22) / (end.e22 - start.e22));
    #else
        __m128 start_xmm0 = start.xmm0;
        __m128 start_xmm1 = start.xmm1;
        return m3(_mm_div_ps(_mm_sub_ps(value.xmm0, start_xmm0), _mm_sub_ps(end.xmm0, start_xmm0)),
                  _mm_div_ps(_mm_sub_ps(value.xmm1, start_xmm1), _mm_sub_ps(end.xmm1, start_xmm1)),
                  (value.e22 - start.e22) / (end.e22 - start.e22));
    #endif
    }

    static REV_INLINE m3 REV_VECTORCALL invlerp_n(m3 start, m3 end, m3 value) // ret = [-1, 1]
    {
    #if REV_ISA >= REV_ISA_AVX
        __m256 start_ymm0 = start.ymm0;
        return m3(_mm256_mul_ps(_mm256_sub_ps(_mm256_div_ps(_mm256_sub_ps(value.ymm0, start_ymm0), _mm256_sub_ps(end.ymm0, start_ymm0)), _mm256_set1_ps(0.5f)), _mm256_set1_ps(2.0f)),
                  ((value.e22 - start.e22) / (end.e22 - start.e22) - 0.5f) * 2.0f);
    #else
        __m128 start_xmm0 = start.xmm0;
        __m128 start_xmm1 = start.xmm1;
        __m128 half       = _mm_set1_ps(0.5f);
        __m128 two        = _mm_set1_ps(2.0f);
    
        return m3(_mm_mul_ps(_mm_sub_ps(_mm_div_ps(_mm_sub_ps(value.xmm0, start_xmm0), _mm_sub_ps(end.xmm0, start_xmm0)), half), two),
                  _mm_mul_ps(_mm_sub_ps(_mm_div_ps(_mm_sub_ps(value.xmm1, start_xmm1), _mm_sub_ps(end.xmm1, start_xmm1)), half), two),
                  ((value.e22 - start.e22) / (end.e22 - start.e22) - 0.5f) * 2.0f);
    #endif
    }

    static REV_INLINE m3 REV_VECTORCALL clamp(m3 val, m3 min, m3 max)
    {
    #if REV_ISA >= REV_ISA_AVX
        return m3(_mm256_max_ps(min.ymm0, _mm256_min_ps(max.ymm0, val.ymm0)),
                  RTTI::max(min.e22, RTTI::min(max.e22, val.e22)));
    #else
        return m3(_mm_max_ps(min.xmm0, _mm_min_ps(max.xmm0, val.xmm0)),
                  _mm_max_ps(min.xmm1, _mm_min_ps(max.xmm1, val.xmm1)),
                  RTTI::max(min.e22, RTTI::min(max.e22, val.e22)));
    #endif
    }

    REV_INLINE m3& REV_VECTORCALL operator+=(m3 r)
    {
    #if REV_ISA >= REV_ISA_AVX
        ymm0  = _mm256_add_ps(ymm0, r.ymm0);
        e22  += r.e22;
    #else
        xmm0  = _mm_add_ps(xmm0, r.xmm0);
        xmm1  = _mm_add_ps(xmm1, r.xmm1);
        e22  += r.e22;
    #endif
        return *this;
    }

    REV_INLINE m3& REV_VECTORCALL operator-=(m3 r)
    {
    #if REV_ISA >= REV_ISA_AVX
        ymm0  = _mm256_sub_ps(ymm0, r.ymm0);
        e22  -= r.e22;
    #else
        xmm0  = _mm_sub_ps(xmm0, r.xmm0);
        xmm1  = _mm_sub_ps(xmm1, r.xmm1);
        e22  -= r.e22;
    #endif
        return *this;
    }

    REV_INLINE m3& REV_VECTORCALL operator*=(m3 r)
    {
        __m128 l_r0 = _mm_setr_ps(e00, e01, e02, 0.0f);
        __m128 l_r1 = _mm_setr_ps(e10, e11, e12, 0.0f);
        __m128 l_r2 = _mm_setr_ps(e20, e21, e22, 0.0f);

        __m128 r_c0 = _mm_setr_ps(r.e00, r.e10, r.e20, 0.0f);
        __m128 r_c1 = _mm_setr_ps(r.e01, r.e11, r.e21, 0.0f);
        __m128 r_c2 = _mm_setr_ps(r.e02, r.e12, r.e22, 0.0f);

        r0 = _mm_or_ps(_mm_or_ps(_mm_dp_ps(l_r0, r_c0, 0b0111'0001), _mm_dp_ps(l_r0, r_c1, 0b0111'0010)), _mm_dp_ps(l_r0, r_c2, 0b0111'0100));
        r1 = _mm_or_ps(_mm_or_ps(_mm_dp_ps(l_r1, r_c0, 0b0111'0001), _mm_dp_ps(l_r1, r_c1, 0b0111'0010)), _mm_dp_ps(l_r1, r_c2, 0b0111'0100));
        r2 = _mm_or_ps(_mm_or_ps(_mm_dp_ps(l_r2, r_c0, 0b0111'0001), _mm_dp_ps(l_r2, r_c1, 0b0111'0010)), _mm_dp_ps(l_r2, r_c2, 0b0111'0100));

        return *this;
    }

    REV_INLINE m3& REV_VECTORCALL operator+=(f32 r)
    {
    #if REV_ISA >= REV_ISA_AVX
        ymm0  = _mm256_add_ps(ymm0, _mm256_set1_ps(r));
        e22  += r;
    #else
        __m128 vval = _mm_set1_ps(r);
        xmm0  = _mm_add_ps(xmm0, vval);
        xmm1  = _mm_add_ps(xmm1, vval);
        e22  += r;
    #endif
        return *this;
    }

    REV_INLINE m3& REV_VECTORCALL operator-=(f32 r)
    {
    #if REV_ISA >= REV_ISA_AVX
        ymm0  = _mm256_sub_ps(ymm0, _mm256_set1_ps(r));
        e22  -= r;
    #else
        __m128 vval = _mm_set1_ps(r);
        xmm0  = _mm_add_ps(xmm0, vval);
        xmm1  = _mm_add_ps(xmm1, vval);
        e22  -= r;
    #endif
        return *this;
    }

    REV_INLINE m3& REV_VECTORCALL operator*=(f32 r)
    {
    #if REV_ISA >= REV_ISA_AVX
        ymm0  = _mm256_mul_ps(ymm0, _mm256_set1_ps(r));
        e22  -= r;
    #else
        __m128 vval = _mm_set1_ps(r);
        xmm0  = _mm_mul_ps(xmm0, vval);
        xmm1  = _mm_mul_ps(xmm1, vval);
        e22  -= r;
    #endif
        return *this;
    }

    REV_INLINE m3& REV_VECTORCALL operator/=(f32 r)
    {
    #if REV_ISA >= REV_ISA_AVX
        ymm0  = _mm256_div_ps(ymm0, _mm256_set1_ps(r));
        e22  -= r;
    #else
        __m128 vval = _mm_set1_ps(r);
        xmm0  = _mm_div_ps(xmm0, vval);
        xmm1  = _mm_div_ps(xmm1, vval);
        e22  -= r;
    #endif
        return *this;
    }

    REV_INLINE v3  operator[](u8 i) const { REV_CHECK(i < 3); return cast<v3 *>(this)[i]; }
    REV_INLINE v3& operator[](u8 i)       { REV_CHECK(i < 3); return cast<v3 *>(this)[i]; }

    friend REV_INLINE m3 REV_VECTORCALL operator+(m3 l, m3 r);
    friend REV_INLINE m3 REV_VECTORCALL operator-(m3 l, m3 r);
    friend REV_INLINE m3 REV_VECTORCALL operator*(m3 l, m3 r);

    friend REV_INLINE m3 REV_VECTORCALL operator+(f32 l, m3  r);
    friend REV_INLINE m3 REV_VECTORCALL operator-(m3  l, f32 r);
    friend REV_INLINE m3 REV_VECTORCALL operator*(f32 l, m3  r);
    friend REV_INLINE m3 REV_VECTORCALL operator/(m3  l, f32 r);

    friend REV_INLINE bool REV_VECTORCALL operator==(m3 l, m3 r);
    friend REV_INLINE bool REV_VECTORCALL operator!=(m3 l, m3 r);
};

REV_INLINE m3 REV_VECTORCALL operator+(m3 l, m3 r)
{
#if REV_ISA >= REV_ISA_AVX
    return m3(_mm256_add_ps(l.ymm0, r.ymm0),
              l.e22 + r.e22);
#else
    return m3(_mm_add_ps(l.xmm0, r.xmm0),
              _mm_add_ps(l.xmm1, r.xmm1),
              l.e22 + r.e22);
#endif
}

REV_INLINE m3 REV_VECTORCALL operator-(m3 l, m3 r)
{
#if REV_ISA >= REV_ISA_AVX
    return m3(_mm256_sub_ps(l.ymm0, r.ymm0),
              l.e22 + r.e22);
#else
    return m3(_mm_sub_ps(l.xmm0, r.xmm0),
              _mm_sub_ps(l.xmm1, r.xmm1),
              l.e22 + r.e22);
#endif
}

REV_INLINE m3 REV_VECTORCALL operator*(m3 l, m3 r)
{
    __m128 l_r0 = _mm_setr_ps(l.e00, l.e01, l.e02, 0.0f);
    __m128 l_r1 = _mm_setr_ps(l.e10, l.e11, l.e12, 0.0f);
    __m128 l_r2 = _mm_setr_ps(l.e20, l.e21, l.e22, 0.0f);

    __m128 r_c0 = _mm_setr_ps(r.e00, r.e10, r.e20, 0.0f);
    __m128 r_c1 = _mm_setr_ps(r.e01, r.e11, r.e21, 0.0f);
    __m128 r_c2 = _mm_setr_ps(r.e02, r.e12, r.e22, 0.0f);

    return m3(_mm_or_ps(_mm_or_ps(_mm_dp_ps(l_r0, r_c0, 0b0111'0001), _mm_dp_ps(l_r0, r_c1, 0b0111'0010)), _mm_dp_ps(l_r0, r_c2, 0b0111'0100)),
              _mm_or_ps(_mm_or_ps(_mm_dp_ps(l_r1, r_c0, 0b0111'0001), _mm_dp_ps(l_r1, r_c1, 0b0111'0010)), _mm_dp_ps(l_r1, r_c2, 0b0111'0100)),
              _mm_or_ps(_mm_or_ps(_mm_dp_ps(l_r2, r_c0, 0b0111'0001), _mm_dp_ps(l_r2, r_c1, 0b0111'0010)), _mm_dp_ps(l_r2, r_c2, 0b0111'0100)));
}

REV_INLINE m3 REV_VECTORCALL operator+(f32 l, m3 r)
{
#if REV_ISA >= REV_ISA_AVX
    return m3(_mm256_add_ps(_mm256_set1_ps(l), r.ymm0),
              l + r.e22);
#else
    __m128 vval = _mm_set1_ps(l);
    return m3(_mm_add_ps(vval, r.xmm0),
              _mm_add_ps(vval, r.xmm1),
              l + r.e22);
#endif
}

REV_INLINE m3 REV_VECTORCALL operator-(m3 l, f32 r)
{
#if REV_ISA >= REV_ISA_AVX
    return m3(_mm256_sub_ps(l.ymm0, _mm256_set1_ps(r)),
              l.e22 + r);
#else
    __m128 vval = _mm_set1_ps(r);
    return m3(_mm_sub_ps(l.xmm0, vval),
              _mm_sub_ps(l.xmm1, vval),
              l.e22 + r);
#endif
}

REV_INLINE m3 REV_VECTORCALL operator*(f32 l, m3 r)
{
#if REV_ISA >= REV_ISA_AVX
    return m3(_mm256_mul_ps(_mm256_set1_ps(l), r.ymm0),
              l + r.e22);
#else
    __m128 vval = _mm_set1_ps(l);
    return m3(_mm_mul_ps(vval, r.xmm0),
              _mm_mul_ps(vval, r.xmm1),
              l + r.e22);
#endif
}

REV_INLINE m3 REV_VECTORCALL operator/(m3 l, f32 r)
{
#if REV_ISA >= REV_ISA_AVX
    return m3(_mm256_div_ps(l.ymm0, _mm256_set1_ps(r)),
              l.e22 + r);
#else
    __m128 vval = _mm_set1_ps(r);
    return m3(_mm_div_ps(l.xmm0, vval),
              _mm_div_ps(l.xmm1, vval),
              l.e22 + r);
#endif
}

REV_INLINE v3 REV_VECTORCALL operator*(m3 l, v3 r)
{
    __m128 l_r0 = _mm_setr_ps(l.e00, l.e01, l.e02, 0.0f);
    __m128 l_r1 = _mm_setr_ps(l.e10, l.e11, l.e12, 0.0f);
    __m128 l_r2 = _mm_setr_ps(l.e20, l.e21, l.e22, 0.0f);

    __m128 r_mm = _mm_setr_ps(r.x, r.y, r.z, 0.0f);

    return v3(_mm_or_ps(_mm_or_ps(
        _mm_dp_ps(l_r0, r_mm, 0b0111'0001),
        _mm_dp_ps(l_r1, r_mm, 0b0111'0010)),
        _mm_dp_ps(l_r2, r_mm, 0b0111'0100)));
}

REV_INLINE bool REV_VECTORCALL operator==(m3 l, m3 r)
{
#if REV_ISA >= REV_ISA_AVX512
    __m512 mm_l = _mm512_setr_ps(l.e00, l.e01, l.e02, 0.0f,
                                 l.e10, l.e11, l.e12, 0.0f,
                                 l.e20, l.e21, l.e22, 0.0f,
                                 0.0f,  0.0f,  0.0f,  0.0f);

    __m512 mm_r = _mm512_setr_ps(r.e00, r.e01, r.e02, 0.0f,
                                 r.e10, r.e11, r.e12, 0.0f,
                                 r.e20, r.e21, r.e22, 0.0f,
                                 0.0f,  0.0f,  0.0f,  0.0f);

    return _mm512_cmpeq_ps_mask(mm_l, mm_r) == 0xFFFF;
#elif REV_ISA >= REV_ISA_AVX
    return _mm256_testc_si256(_mm256_castps_si256(_mm256_cmp_ps(l.ymm0, r.ymm0, _CMP_EQ_OQ)),
                              _mm256_set1_epi64x(REV_U64_MAX))
        && l.e22 == r.e22;
#else
    __m128i cmp0 = _mm_castps_si128(_mm_cmpeq_ps(l.xmm0, r.xmm0));
    __m128i cmp1 = _mm_castps_si128(_mm_cmpeq_ps(l.xmm1, r.xmm1));
    __m128i mask = _mm_set1_epi64x(REV_U64_MAX);

    return _mm_testc_si128(cmp0, mask)
        && _mm_testc_si128(cmp1, mask)
        && l.e22 == r.e22;
#endif
}

REV_INLINE bool REV_VECTORCALL operator!=(m3 l, m3 r)
{
#if REV_ISA >= REV_ISA_AVX512
    __m512 mm_l = _mm512_setr_ps(l.e00, l.e01, l.e02, 0.0f,
                                 l.e10, l.e11, l.e12, 0.0f,
                                 l.e20, l.e21, l.e22, 0.0f,
                                 0.0f,  0.0f,  0.0f,  0.0f);

    __m512 mm_r = _mm512_setr_ps(r.e00, r.e01, r.e02, 0.0f,
                                 r.e10, r.e11, r.e12, 0.0f,
                                 r.e20, r.e21, r.e22, 0.0f,
                                 0.0f,  0.0f,  0.0f,  0.0f);

    return _mm512_cmpneq_ps_mask(mm_l, mm_r);
#elif REV_ISA >= REV_ISA_AVX
    return _mm256_testc_si256(_mm256_castps_si256(_mm256_cmp_ps(l.ymm0, r.ymm0, _CMP_NEQ_OQ)),
                              _mm256_set1_epi64x(REV_U64_MAX))
        || l.e22 != r.e22;
#else
    __m128i cmp0 = _mm_castps_si128(_mm_cmpneq_ps(l.xmm0, r.xmm0));
    __m128i cmp1 = _mm_castps_si128(_mm_cmpneq_ps(l.xmm1, r.xmm1));
    __m128i mask = _mm_set1_epi64x(REV_U64_MAX);

    return _mm_testc_si128(cmp0, mask)
        || _mm_testc_si128(cmp1, mask)
        || l.e22 != r.e22;
#endif
}

//
// m4
//

union REV_INTRIN_TYPE m4 final
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
        xmm_u xmm0;
        xmm_u xmm1;
        xmm_u xmm2;
        xmm_u xmm3;
    };
#if REV_ISA >= REV_ISA_AVX
    struct
    {
        ymm_u ymm0;
        ymm_u ymm1;
    };
#endif
#if REV_ISA >= REV_ISA_AVX512
    zmm_u zmm;
#endif

    REV_INLINE m4()
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

    REV_INLINE m4(f32 val)
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

    REV_INLINE m4(f32 e00, f32 e01, f32 e02, f32 e03,
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

    REV_INLINE m4(v4 c1, v4 c2, v4 c3, v4 c4)
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

    REV_INLINE m4(f32 arr[16])
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
    REV_INLINE m4(__m512 mm)
        : zmm(mm)
    {
    }
#endif

#if REV_ISA >= REV_ISA_AVX
    REV_INLINE m4(__m256 _mm0, __m256 _mm1)
        : ymm0(_mm0),
          ymm1(_mm1)
    {
    }
#endif

    REV_INLINE m4(__m128 _mm0, __m128 _mm1, __m128 _mm2, __m128 _mm3)
        : xmm0(_mm0),
          xmm1(_mm1),
          xmm2(_mm2),
          xmm3(_mm3)
    {
    }

    REV_INLINE m4(const m4& m)
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

    REV_INLINE m4(m4&& m)
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

    REV_INLINE m4& REV_VECTORCALL operator=(f32 val)
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

    REV_INLINE m4& REV_VECTORCALL operator=(f32 arr[16])
    {
    #if REV_ISA >= REV_ISA_AVX512
        zmm = _mm512_loadu_ps(arr);
    #elif REV_ISA >= REV_ISA_AVX
        ymm0 = _mm256_loadu_ps(arr);
        ymm1 = _mm256_loadu_ps(arr + 8);
    #else
        xmm0 = _mm_loadu_ps(arr);
        xmm1 = _mm_loadu_ps(arr + 4);
        xmm2 = _mm_loadu_ps(arr + 8);
        xmm3 = _mm_loadu_ps(arr + 12);
    #endif
        return *this;
    }

    REV_INLINE m4& REV_VECTORCALL operator=(const m4& m)
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

    REV_INLINE m4& REV_VECTORCALL operator=(m4&& m)
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

    REV_INLINE m4 REV_VECTORCALL transpose() const
    {
        return m4(e00, e10, e20, e30,
                  e01, e11, e21, e31,
                  e02, e12, e22, e32,
                  e03, e13, e23, e33);
    }

    REV_INLINE f32 REV_VECTORCALL det() const
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

        return _mm_cvtss_f32(res);
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

        return _mm_cvtss_f32(res);
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

        return _mm_cvtss_f32(res);
    #endif
    }

    REV_INLINE m4 REV_VECTORCALL inverse() const
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

    static REV_INLINE m4 REV_VECTORCALL identity()
    {
        return m4(1.0f, 0.0f, 0.0f, 0.0f,
                  0.0f, 1.0f, 0.0f, 0.0f,
                  0.0f, 0.0f, 1.0f, 0.0f,
                  0.0f, 0.0f, 0.0f, 1.0f);
    }

    static REV_INLINE m4 REV_VECTORCALL scaling(f32 nx, f32 ny, f32 nz)
    {
        return m4(  nx, 0.0f, 0.0f, 0.0f,
                  0.0f,   ny, 0.0f, 0.0f,
                  0.0f, 0.0f,   nz, 0.0f,
                  0.0f, 0.0f, 0.0f, 1.0f);
    }

    static REV_INLINE m4 REV_VECTORCALL rotation_x(rad angle)
    {
        f32 s = sinf(angle);
        f32 c = cosf(angle);

        return m4(1.0f, 0.0f, 0.0f, 0.0f,
                  0.0f,    c,   -s, 0.0f,
                  0.0f,    s,    c, 0.0f,
                  0.0f, 0.0f, 0.0f, 1.0f);
    }

    static REV_INLINE m4 REV_VECTORCALL rotation_y(rad angle)
    {
        f32 s = sinf(angle);
        f32 c = cosf(angle);

        return m4(   c, 0.0f,    s, 0.0f,
                  0.0f, 1.0f, 0.0f, 0.0f,
                    -s, 0.0f,    c, 0.0f,
                  0.0f, 0.0f, 0.0f, 1.0f);
    }

    static REV_INLINE m4 REV_VECTORCALL rotation_z(rad angle)
    {
        f32 s = sinf(angle);
        f32 c = cosf(angle);

        return m4(   c,   -s, 0.0f, 0.0f,
                     s,    c, 0.0f, 0.0f,
                  0.0f, 0.0f, 1.0f, 0.0f,
                  0.0f, 0.0f, 0.0f, 1.0f);
    }

    static REV_INLINE m4 REV_VECTORCALL rotation(v4 v, rad angle)
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

    static REV_INLINE m4 REV_VECTORCALL reflection_x()
    {
        return m4(-1.0f, 0.0f, 0.0f, 0.0f,
                   0.0f, 1.0f, 0.0f, 0.0f,
                   0.0f, 0.0f, 1.0f, 0.0f,
                   0.0f, 0.0f, 0.0f, 1.0f);
    }

    static REV_INLINE m4 REV_VECTORCALL reflection_y()
    {
        return m4(1.0f,  0.0f, 0.0f, 0.0f,
                  0.0f, -1.0f, 0.0f, 0.0f,
                  0.0f,  0.0f, 1.0f, 0.0f,
                  0.0f,  0.0f, 0.0f, 1.0f);
    }

    static REV_INLINE m4 REV_VECTORCALL reflection_z()
    {
        return m4(1.0f, 0.0f,  0.0f, 0.0f,
                  0.0f, 1.0f,  0.0f, 0.0f,
                  0.0f, 0.0f, -1.0f, 0.0f,
                  0.0f, 0.0f,  0.0f, 1.0f);
    }

    static REV_INLINE m4 REV_VECTORCALL reflection_xy()
    {
        return m4(-1.0f,  0.0f, 0.0f, 0.0f,
                   0.0f, -1.0f, 0.0f, 0.0f,
                   0.0f,  0.0f, 1.0f, 0.0f,
                   0.0f,  0.0f, 0.0f, 1.0f);
    }

    static REV_INLINE m4 REV_VECTORCALL reflection_xz()
    {
        return m4(-1.0f, 0.0f,  0.0f, 0.0f,
                   0.0f, 1.0f,  0.0f, 0.0f,
                   0.0f, 0.0f, -1.0f, 0.0f,
                   0.0f, 0.0f,  0.0f, 1.0f);
    }

    static REV_INLINE m4 REV_VECTORCALL reflection_yz()
    {
        return m4(1.0f,  0.0f,  0.0f, 0.0f,
                  0.0f, -1.0f,  0.0f, 0.0f,
                  0.0f,  0.0f, -1.0f, 0.0f,
                  0.0f,  0.0f,  0.0f, 1.0f);
    }

    static REV_INLINE m4 REV_VECTORCALL reflection_xyz()
    {
        return m4(-1.0f,  0.0f,  0.0f, 0.0f,
                   0.0f, -1.0f,  0.0f, 0.0f,
                   0.0f,  0.0f, -1.0f, 0.0f,
                   0.0f,  0.0f,  0.0f, 1.0f);
    }

    static REV_INLINE m4 REV_VECTORCALL shearing_x(f32 shy, f32 shz)
    {
        return m4(1.0f, 0.0f, 0.0f, 0.0f,
                   shy, 1.0f, 0.0f, 0.0f,
                   shz, 0.0f, 1.0f, 0.0f,
                  0.0f, 0.0f, 0.0f, 1.0f);
    }

    static REV_INLINE m4 REV_VECTORCALL shearing_y(f32 shx, f32 shz)
    {
        return m4(1.0f,  shx, 0.0f, 0.0f,
                  0.0f, 1.0f, 0.0f, 0.0f,
                  0.0f,  shz, 1.0f, 0.0f,
                  0.0f, 0.0f, 0.0f, 1.0f);
    }

    static REV_INLINE m4 REV_VECTORCALL shearing_z(f32 shx, f32 shy)
    {
        return m4(1.0f, 0.0f,  shx, 0.0f,
                  0.0f, 1.0f,  shy, 0.0f,
                  0.0f, 0.0f, 1.0f, 0.0f,
                  0.0f, 0.0f, 0.0f, 1.0f);
    }

    static REV_INLINE m4 REV_VECTORCALL shearing(f32 shxy, f32 shxz, f32 shyx, f32 shyz, f32 shzx, f32 shzy)
    {
        return m4(1.0f, shyx, shzx, 0.0f,
                  shxy, 1.0f, shzy, 0.0f,
                  shxz, shyz, 1.0f, 0.0f,
                  0.0f, 0.0f, 0.0f, 1.0f);
    }

    static REV_INLINE m4 REV_VECTORCALL translation(f32 dx, f32 dy, f32 dz)
    {
        return m4(1.0f, 0.0f, 0.0f,   dx,
                  0.0f, 1.0f, 0.0f,   dy,
                  0.0f, 0.0f, 1.0f,   dz,
                  0.0f, 0.0f, 0.0f, 1.0f);
    }

    static REV_INLINE m4 REV_VECTORCALL translation(v4 dv)
    {
        return m4(1.0f, 0.0f, 0.0f, dv.x,
                  0.0f, 1.0f, 0.0f, dv.y,
                  0.0f, 0.0f, 1.0f, dv.z,
                  0.0f, 0.0f, 0.0f, 1.0f);
    }

    static REV_INLINE m4 REV_VECTORCALL ortho_lh(f32 left, f32 right, f32 bottom, f32 top, f32 _near, f32 _far) // returns z = [-1; 1]
    {
        return m4(2.0f/(right-left),              0.0f,              0.0f, (-right-left)/(right-left),
                               0.0f, 2.0f/(top-bottom),              0.0f, (-top-bottom)/(top-bottom),
                               0.0f,              0.0f, 2.0f/(_far-_near), (-_far-_near)/(_far-_near),
                               0.0f,              0.0f,              0.0f,                       1.0f);
    }

    static REV_INLINE m4 REV_VECTORCALL ortho_rh(f32 left, f32 right, f32 bottom, f32 top, f32 _near, f32 _far) // returns z = [-1; 1]
    {
        return m4(2.0f/(right-left),              0.0f,               0.0f, (-right-left)/(right-left),
                               0.0f, 2.0f/(top-bottom),               0.0f, (-top-bottom)/(top-bottom),
                               0.0f,              0.0f, -2.0f/(_far-_near),  (_far+_near)/(_far-_near),
                               0.0f,              0.0f,               0.0f,                       1.0f);
    }

    static REV_INLINE m4 REV_VECTORCALL persp_lh(f32 left, f32 right, f32 bottom, f32 top, f32 _near, f32 _far) // returns z = [-1; 1]
    {
        return m4(2.0f*_near/(right-left),                    0.0f,                      0.0f,                            0.0f,
                                     0.0f, 2.0f*_near/(top-bottom),                      0.0f,                            0.0f,
                                     0.0f,                    0.0f, (_far+_near)/(_far-_near), -(2.0f*_near*_far)/(_far-_near),
                                     0.0f,                    0.0f,                      1.0f,                            0.0f);
    }

    static REV_INLINE m4 REV_VECTORCALL persp_lh(f32 aspect, deg fov, f32 _near, f32 _far) // returns z = [-1; 1]
    {
        f32 tanfov2 = tanf(fov / 2.0f * g_f32_PI / 180.0f);

        return m4(1.0f/(aspect*tanfov2),         0.0f,                      0.0f,                            0.0f,
                                   0.0f, 1.0f/tanfov2,                      0.0f,                            0.0f,
                                   0.0f,         0.0f, (_far+_near)/(_far-_near), -(2.0f*_near*_far)/(_far-_near),
                                   0.0f,         0.0f,                      1.0f,                            0.0f);
    }

    static REV_INLINE m4 REV_VECTORCALL persp_rh(f32 left, f32 right, f32 bottom, f32 top, f32 _near, f32 _far) // returns z = [-1; 1]
    {
        return m4(2.0f*_near/(right-left),                    0.0f,                      0.0f,                           0.0f,
                                     0.0f, 2.0f*_near/(top-bottom),                      0.0f,                           0.0f,
                                     0.0f,                    0.0f, (_far+_near)/(_far-_near), (2.0f*_near*_far)/(_far-_near),
                                     0.0f,                    0.0f,                     -1.0f,                           0.0f);
    }

    static REV_INLINE m4 REV_VECTORCALL persp_rh(f32 aspect, deg fov, f32 _near, f32 _far) // returns z = [-1; 1]
    {
        f32 tanfov2 = tanf(fov / 2.0f * g_f32_PI / 180.0f);
        
        return m4(1.0f/(aspect*tanfov2),         0.0f,                      0.0f,                           0.0f,
                                   0.0f, 1.0f/tanfov2,                      0.0f,                           0.0f,
                                   0.0f,         0.0f, (_far+_near)/(_far-_near), (2.0f*_near*_far)/(_far-_near),
                                   0.0f,         0.0f,                     -1.0f,                           0.0f);
    }

    static REV_INLINE m4 REV_VECTORCALL lerp(m4 start, m4 end, m4 percent)
    {
    #if REV_ISA >= REV_ISA_AVX512
        return m4(_mm512_add_ps(_mm512_mul_ps(_mm512_sub_ps(end.zmm, start.zmm), percent.zmm), start.zmm));
    #elif REV_ISA >= REV_ISA_AVX
        return m4(_mm256_add_ps(_mm256_mul_ps(_mm256_sub_ps(end.ymm0, start.ymm0), percent.ymm0), start.ymm0),
                  _mm256_add_ps(_mm256_mul_ps(_mm256_sub_ps(end.ymm1, start.ymm1), percent.ymm1), start.ymm1));
    #else
        return m4(_mm_add_ps(_mm_mul_ps(_mm_sub_ps(end.ymm0, start.ymm0), percent.ymm0), start.ymm0),
                  _mm_add_ps(_mm_mul_ps(_mm_sub_ps(end.ymm1, start.ymm1), percent.ymm1), start.ymm1),
                  _mm_add_ps(_mm_mul_ps(_mm_sub_ps(end.ymm2, start.ymm2), percent.ymm2), start.ymm2),
                  _mm_add_ps(_mm_mul_ps(_mm_sub_ps(end.ymm3, start.ymm3), percent.ymm3), start.ymm3));
    #endif
    }

    static REV_INLINE m4 REV_VECTORCALL invlerp(m4 start, m4 end, m4 value) // ret = [0, 1]
    {
    #if REV_ISA >= REV_ISA_AVX512
        return m4(_mm512_div_ps(_mm512_sub_ps(value.zmm, start.zmm), _mm512_sub_ps(end.zmm, start.zmm)));
    #elif REV_ISA >= REV_ISA_AVX
        return m4(_mm256_div_ps(_mm256_sub_ps(value.ymm0, start.ymm0), _mm256_sub_ps(end.ymm0, start.ymm0)),
                  _mm256_div_ps(_mm256_sub_ps(value.ymm1, start.ymm1), _mm256_sub_ps(end.ymm1, start.ymm1)));
    #else
        return m4(_mm_div_ps(_mm_sub_ps(value.xmm0, start.xmm0), _mm_sub_ps(end.xmm0, start.xmm0)),
                  _mm_div_ps(_mm_sub_ps(value.xmm1, start.xmm1), _mm_sub_ps(end.xmm1, start.xmm1)),
                  _mm_div_ps(_mm_sub_ps(value.xmm2, start.xmm2), _mm_sub_ps(end.xmm2, start.xmm2)),
                  _mm_div_ps(_mm_sub_ps(value.xmm3, start.xmm3), _mm_sub_ps(end.xmm3, start.xmm3)));
    #endif
    }

    static REV_INLINE m4 REV_VECTORCALL invlerp_n(m4 start, m4 end, m4 value) // ret = [-1, 1]
    {
    #if REV_ISA >= REV_ISA_AVX512
        return m4(_mm512_mul_ps(_mm512_sub_ps(_mm512_div_ps(_mm512_sub_ps(value.zmm, start.zmm), _mm512_sub_ps(end.zmm, start.zmm)), _mm512_set1_ps(0.5f)), _mm512_set1_ps(2.0f)));
    #elif REV_ISA >= REV_ISA_AVX
        __m256 half = _mm256_set1_ps(0.5f);
        __m256 two  = _mm256_set1_ps(2.0f);
        return m4(_mm256_mul_ps(_mm256_sub_ps(_mm256_div_ps(_mm256_sub_ps(value.ymm0, start.ymm0), _mm256_sub_ps(end.ymm0, start.ymm0)), half), two),
                  _mm256_mul_ps(_mm256_sub_ps(_mm256_div_ps(_mm256_sub_ps(value.ymm1, start.ymm1), _mm256_sub_ps(end.ymm1, start.ymm1)), half), two));
    #else
        __m128 half = _mm_set1_ps(0.5f);
        __m128 two  = _mm_set1_ps(2.0f);
        return m4(_mm_mul_ps(_mm_sub_ps(_mm_div_ps(_mm_sub_ps(value.xmm0, start.xmm0), _mm_sub_ps(end.xmm0, start.xmm0)), half), two),
                  _mm_mul_ps(_mm_sub_ps(_mm_div_ps(_mm_sub_ps(value.xmm1, start.xmm1), _mm_sub_ps(end.xmm1, start.xmm1)), half), two),
                  _mm_mul_ps(_mm_sub_ps(_mm_div_ps(_mm_sub_ps(value.xmm2, start.xmm2), _mm_sub_ps(end.xmm2, start.xmm2)), half), two),
                  _mm_mul_ps(_mm_sub_ps(_mm_div_ps(_mm_sub_ps(value.xmm3, start.xmm3), _mm_sub_ps(end.xmm3, start.xmm3)), half), two));
    #endif
    }

    static REV_INLINE m4 REV_VECTORCALL clamp(m4 val, m4 min, m4 max)
    {
    #if REV_ISA >= REV_ISA_AVX512
        return m4(_mm512_max_ps(min.zmm, _mm512_min_ps(max.zmm, val.zmm)));
    #elif REV_ISA >= REV_ISA_AVX
        return m4(_mm256_max_ps(min.ymm0, _mm256_min_ps(max.ymm0, val.ymm0)),
                  _mm256_max_ps(min.ymm1, _mm256_min_ps(max.ymm1, val.ymm1)));
    #else
        return m4(_mm_max_ps(min.xmm0, _mm_min_ps(max.xmm0, val.xmm0)),
                  _mm_max_ps(min.xmm1, _mm_min_ps(max.xmm1, val.xmm1)),
                  _mm_max_ps(min.xmm2, _mm_min_ps(max.xmm2, val.xmm2)),
                  _mm_max_ps(min.xmm3, _mm_min_ps(max.xmm3, val.xmm3)));
    #endif
    }

    REV_INLINE m4& REV_VECTORCALL operator+=(m4 r)
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

    REV_INLINE m4& REV_VECTORCALL operator-=(m4 r)
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

    REV_INLINE m4& REV_VECTORCALL operator*=(m4 r)
    {
        __m128 r_c0 = _mm_setr_ps(r.e00, r.e10, r.e20, r.e30);
        __m128 r_c1 = _mm_setr_ps(r.e01, r.e11, r.e21, r.e31);
        __m128 r_c2 = _mm_setr_ps(r.e02, r.e12, r.e22, r.e32);
        __m128 r_c3 = _mm_setr_ps(r.e03, r.e13, r.e23, r.e33);

        xmm0 = _mm_or_ps(_mm_or_ps(_mm_dp_ps(xmm0, r_c0, 0b1111'0001), _mm_dp_ps(xmm0, r_c1, 0b1111'0010)), _mm_or_ps(_mm_dp_ps(xmm0, r_c2, 0b1111'0100), _mm_dp_ps(xmm0, r_c3, 0b1111'1000)));
        xmm1 = _mm_or_ps(_mm_or_ps(_mm_dp_ps(xmm1, r_c0, 0b1111'0001), _mm_dp_ps(xmm1, r_c1, 0b1111'0010)), _mm_or_ps(_mm_dp_ps(xmm1, r_c2, 0b1111'0100), _mm_dp_ps(xmm1, r_c3, 0b1111'1000)));
        xmm2 = _mm_or_ps(_mm_or_ps(_mm_dp_ps(xmm2, r_c0, 0b1111'0001), _mm_dp_ps(xmm2, r_c1, 0b1111'0010)), _mm_or_ps(_mm_dp_ps(xmm2, r_c2, 0b1111'0100), _mm_dp_ps(xmm2, r_c3, 0b1111'1000)));
        xmm3 = _mm_or_ps(_mm_or_ps(_mm_dp_ps(xmm3, r_c0, 0b1111'0001), _mm_dp_ps(xmm3, r_c1, 0b1111'0010)), _mm_or_ps(_mm_dp_ps(xmm3, r_c2, 0b1111'0100), _mm_dp_ps(xmm3, r_c3, 0b1111'1000)));

        return *this;
    }

    REV_INLINE m4& REV_VECTORCALL operator+=(f32 r)
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

    REV_INLINE m4& REV_VECTORCALL operator-=(f32 r)
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

    REV_INLINE m4& REV_VECTORCALL operator*=(f32 r)
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

    REV_INLINE m4& REV_VECTORCALL operator/=(f32 r)
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

    REV_INLINE v4  operator[](u8 i) const { REV_CHECK(i < 4); return cast<v4 *>(this)[i]; }
    REV_INLINE v4& operator[](u8 i)       { REV_CHECK(i < 4); return cast<v4 *>(this)[i]; }
};

REV_INLINE m4 REV_VECTORCALL operator+(m4 l, m4 r)
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

REV_INLINE m4 REV_VECTORCALL operator-(m4 l, m4 r)
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

REV_INLINE m4 REV_VECTORCALL operator*(m4 l, m4 r)
{
    __m128 r_c0 = _mm_setr_ps(r.e00, r.e10, r.e20, r.e30);
    __m128 r_c1 = _mm_setr_ps(r.e01, r.e11, r.e21, r.e31);
    __m128 r_c2 = _mm_setr_ps(r.e02, r.e12, r.e22, r.e32);
    __m128 r_c3 = _mm_setr_ps(r.e03, r.e13, r.e23, r.e33);

    return m4(_mm_or_ps(_mm_or_ps(_mm_dp_ps(l.xmm0, r_c0, 0b1111'0001), _mm_dp_ps(l.xmm0, r_c1, 0b1111'0010)), _mm_or_ps(_mm_dp_ps(l.xmm0, r_c2, 0b1111'0100), _mm_dp_ps(l.xmm0, r_c3, 0b1111'1000))),
              _mm_or_ps(_mm_or_ps(_mm_dp_ps(l.xmm1, r_c0, 0b1111'0001), _mm_dp_ps(l.xmm1, r_c1, 0b1111'0010)), _mm_or_ps(_mm_dp_ps(l.xmm1, r_c2, 0b1111'0100), _mm_dp_ps(l.xmm1, r_c3, 0b1111'1000))),
              _mm_or_ps(_mm_or_ps(_mm_dp_ps(l.xmm2, r_c0, 0b1111'0001), _mm_dp_ps(l.xmm2, r_c1, 0b1111'0010)), _mm_or_ps(_mm_dp_ps(l.xmm2, r_c2, 0b1111'0100), _mm_dp_ps(l.xmm2, r_c3, 0b1111'1000))),
              _mm_or_ps(_mm_or_ps(_mm_dp_ps(l.xmm3, r_c0, 0b1111'0001), _mm_dp_ps(l.xmm3, r_c1, 0b1111'0010)), _mm_or_ps(_mm_dp_ps(l.xmm3, r_c2, 0b1111'0100), _mm_dp_ps(l.xmm3, r_c3, 0b1111'1000))));
}

REV_INLINE m4 REV_VECTORCALL operator+(f32 l, m4 r)
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

REV_INLINE m4 REV_VECTORCALL operator-(m4 l, f32 r)
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

REV_INLINE m4 REV_VECTORCALL operator*(f32 l, m4 r)
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

REV_INLINE m4 REV_VECTORCALL operator/(m4 l, f32 r)
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

REV_INLINE v4 REV_VECTORCALL operator*(m4 l, v4 r)
{
    return v4(_mm_or_ps(_mm_or_ps(_mm_dp_ps(l.xmm0, r.mm, 0b1111'0001),
                                  _mm_dp_ps(l.xmm1, r.mm, 0b1111'0010)),
                        _mm_or_ps(_mm_dp_ps(l.xmm2, r.mm, 0b1111'0100),
                                  _mm_dp_ps(l.xmm3, r.mm, 0b1111'1000))));
}

REV_INLINE bool REV_VECTORCALL operator==(m4 l, m4 r)
{
#if REV_ISA >= REV_ISA_AVX512
    return _mm512_cmpeq_ps_mask(l.zmm, r.zmm) == 0xFFFF;
#elif REV_ISA >= REV_ISA_AVX2
    __m256i cmp0 = _mm256_cmpeq_epi32(_mm256_castps_si256(l.ymm0), _mm256_castps_si256(r.ymm0));
    __m256i cmp1 = _mm256_cmpeq_epi32(_mm256_castps_si256(l.ymm1), _mm256_castps_si256(r.ymm1));
    __m256i mask = _mm256_set1_epi64x(REV_U64_MAX);
    return _mm256_testc_si256(cmp0, mask)
        && _mm256_testc_si256(cmp1, mask);
#elif REV_ISA >= REV_ISA_AVX
    __m256i cmp0 = _mm256_castps_si256(_mm256_cmp_ps(l.ymm0, r.ymm0, _CMP_EQ_OQ));
    __m256i cmp1 = _mm256_castps_si256(_mm256_cmp_ps(l.ymm1, r.ymm1, _CMP_EQ_OQ));
    __m256i mask = _mm256_set1_epi64x(REV_U64_MAX);
    return _mm256_testc_si256(cmp0, mask)
        && _mm256_testc_si256(cmp1, mask);
#else
    __m128i cmp0 = _mm256_castps_si256(_mm_cmpeq_ps(l.xmm0, r.xmm0));
    __m128i cmp1 = _mm256_castps_si256(_mm_cmpeq_ps(l.xmm1, r.xmm1));
    __m128i cmp2 = _mm256_castps_si256(_mm_cmpeq_ps(l.xmm2, r.xmm2));
    __m128i cmp3 = _mm256_castps_si256(_mm_cmpeq_ps(l.xmm3, r.xmm3));
    __m128i mask = _mm_set1_epi64x(REV_U64_MAX);
    return _mm_testc_si128(cmp0, mask)
        && _mm_testc_si128(cmp1, mask)
        && _mm_testc_si128(cmp2, mask)
        && _mm_testc_si128(cmp3, mask);
#endif
}

REV_INLINE bool REV_VECTORCALL operator!=(m4 l, m4 r)
{
#if REV_ISA >= REV_ISA_AVX512
    return _mm512_cmpneq_ps_mask(l.zmm, r.zmm);
#elif REV_ISA >= REV_ISA_AVX
    __m256i cmp0 = _mm256_castps_si256(_mm256_cmp_ps(l.ymm0, r.ymm0, _CMP_NEQ_OQ));
    __m256i cmp1 = _mm256_castps_si256(_mm256_cmp_ps(l.ymm1, r.ymm1, _CMP_NEQ_OQ));
    __m256i mask = _mm256_set1_epi64x(REV_U64_MAX);
    return _mm256_testc_si256(cmp0, mask)
        || _mm256_testc_si256(cmp1, mask);
#else
    __m128i cmp0 = _mm256_castps_si256(_mm_cmpneq_ps(l.xmm0, r.xmm0));
    __m128i cmp1 = _mm256_castps_si256(_mm_cmpneq_ps(l.xmm1, r.xmm1));
    __m128i cmp2 = _mm256_castps_si256(_mm_cmpneq_ps(l.xmm2, r.xmm2));
    __m128i cmp3 = _mm256_castps_si256(_mm_cmpneq_ps(l.xmm3, r.xmm3));
    __m128i mask = _mm_set1_epi64x(REV_U64_MAX);
    return _mm_testc_si128(cmp0, mask)
        || _mm_testc_si128(cmp1, mask)
        || _mm_testc_si128(cmp2, mask)
        || _mm_testc_si128(cmp3, mask);
#endif
}

// @TODO(Roman): Move to the camera class or something.

REV_INLINE m4 REV_VECTORCALL LookAtLH(v4 camera, v4 target, v4 up)
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

REV_INLINE m4 REV_VECTORCALL LookAtRH(v4 camera, v4 target, v4 up)
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
