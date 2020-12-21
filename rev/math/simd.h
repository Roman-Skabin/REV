//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "math/math.h"

namespace REV::Math
{
    union REV_INTRIN_TYPE xmm
    {
        __m128  f;
        __m128i i;
        __m128d d;

        REV_INLINE xmm(            ) : f(_mm_setzero_ps()) {}
        REV_INLINE xmm(__m128     v) : f(v)                {}
        REV_INLINE xmm(__m128i    v) : i(v)                {}
        REV_INLINE xmm(__m128d    v) : d(v)                {}
        REV_INLINE xmm(const xmm& v) : f(v.f)              {}
        REV_INLINE xmm(xmm&&      v) : f(v.f)              {}

        REV_INLINE REV_VECTORCALL operator __m128()  const { return f; }
        REV_INLINE REV_VECTORCALL operator __m128i() const { return i; }
        REV_INLINE REV_VECTORCALL operator __m128d() const { return d; }

        REV_INLINE xmm& REV_VECTORCALL operator=(__m128     v) { f = v;   return *this; }
        REV_INLINE xmm& REV_VECTORCALL operator=(__m128i    v) { i = v;   return *this; }
        REV_INLINE xmm& REV_VECTORCALL operator=(__m128d    v) { d = v;   return *this; }
        REV_INLINE xmm& REV_VECTORCALL operator=(const xmm& v) { f = v.f; return *this; }
        REV_INLINE xmm& REV_VECTORCALL operator=(xmm&&      v) { f = v.f; return *this; }
    };

    #if REV_ISA >= REV_ISA_AVX
    union REV_INTRIN_TYPE ymm
    {
        __m256  f;
        __m256i i;
        __m256d d;

        REV_INLINE ymm(            ) : f(_mm256_setzero_ps()) {}
        REV_INLINE ymm(__m256     v) : f(v)                   {}
        REV_INLINE ymm(__m256i    v) : i(v)                   {}
        REV_INLINE ymm(__m256d    v) : d(v)                   {}
        REV_INLINE ymm(const ymm& v) : f(v.f)                 {}
        REV_INLINE ymm(ymm&&      v) : f(v.f)                 {}

        REV_INLINE REV_VECTORCALL operator __m256()  const { return f; }
        REV_INLINE REV_VECTORCALL operator __m256i() const { return i; }
        REV_INLINE REV_VECTORCALL operator __m256d() const { return d; }

        REV_INLINE ymm& REV_VECTORCALL operator=(__m256     v) { f = v;   return *this; }
        REV_INLINE ymm& REV_VECTORCALL operator=(__m256i    v) { i = v;   return *this; }
        REV_INLINE ymm& REV_VECTORCALL operator=(__m256d    v) { d = v;   return *this; }
        REV_INLINE ymm& REV_VECTORCALL operator=(const ymm& v) { f = v.f; return *this; }
        REV_INLINE ymm& REV_VECTORCALL operator=(ymm&&      v) { f = v.f; return *this; }
    };
    #endif

    #if REV_ISA >= REV_ISA_AVX512
    union REV_INTRIN_TYPE zmm
    {
        __m512  f;
        __m512i i;
        __m512d d;

        REV_INLINE zmm(            ) : f(_mm512_setzero_ps()) {}
        REV_INLINE zmm(__m512     v) : f(v)                   {}
        REV_INLINE zmm(__m512i    v) : i(v)                   {}
        REV_INLINE zmm(__m512d    v) : d(v)                   {}
        REV_INLINE zmm(const zmm& v) : f(v.f)                 {}
        REV_INLINE zmm(zmm&&      v) : f(v.f)                 {}

        REV_INLINE REV_VECTORCALL operator __m512()  const { return f; }
        REV_INLINE REV_VECTORCALL operator __m512i() const { return i; }
        REV_INLINE REV_VECTORCALL operator __m512d() const { return d; }

        REV_INLINE zmm& REV_VECTORCALL operator=(__m512     v) { f = v;   return *this; }
        REV_INLINE zmm& REV_VECTORCALL operator=(__m512i    v) { i = v;   return *this; }
        REV_INLINE zmm& REV_VECTORCALL operator=(__m512d    v) { d = v;   return *this; }
        REV_INLINE zmm& REV_VECTORCALL operator=(const zmm& v) { f = v.f; return *this; }
        REV_INLINE zmm& REV_VECTORCALL operator=(zmm&&      v) { f = v.f; return *this; }
    };
    #endif

    // @NOTE(Roman): We are not able to align v4 to 1 byte because __m128 aligned to 16 bytes, so let's say this is our register.
    // @Important(Roman): This may cause some mem corruptions, so let's see how it will go.
    union REV_INTRIN_TYPE xmm_u
    {
        s8  s8s[16];
        s16 s16s[8];
        s32 s32s[4];
        s64 s64s[2];

        u8  u8s[16];
        u16 u16s[8];
        u32 u32s[4];
        u64 u64s[2];

        f32 f32s[4];
        f64 f64s[2];

        REV_INLINE xmm_u(__m128  xmm       ) { _mm_storeu_ps(f32s, xmm);                          }
        REV_INLINE xmm_u(__m128i xmm       ) { _mm_storeu_si128(cast<__m128i *>(u64s), xmm);      }
        REV_INLINE xmm_u(__m128d xmm       ) { _mm_storeu_pd(f64s, xmm);                          }
        REV_INLINE xmm_u(const xmm_u& other) { *cast<__m128i *>(this) = *cast<__m128i *>(&other); }
        REV_INLINE xmm_u(xmm_u&& other     ) { *cast<__m128i *>(this) = *cast<__m128i *>(&other); }

        REV_INLINE REV_VECTORCALL operator __m128()  const { return _mm_loadu_ps(f32s);                     }
        REV_INLINE REV_VECTORCALL operator __m128i() const { return _mm_lddqu_si128(cast<__m128i *>(u64s)); }
        REV_INLINE REV_VECTORCALL operator __m128d() const { return _mm_loadu_pd(f64s);                     }

        REV_INLINE xmm_u& REV_VECTORCALL operator=(__m128  xmm       ) { _mm_storeu_ps(f32s, xmm);                          return *this; }
        REV_INLINE xmm_u& REV_VECTORCALL operator=(__m128i xmm       ) { _mm_storeu_si128(cast<__m128i *>(u64s), xmm);      return *this; }
        REV_INLINE xmm_u& REV_VECTORCALL operator=(__m128d xmm       ) { _mm_storeu_pd(f64s, xmm);                          return *this; }
        REV_INLINE xmm_u& REV_VECTORCALL operator=(const xmm_u& other) { *cast<__m128i *>(this) = *cast<__m128i *>(&other); return *this; }
        REV_INLINE xmm_u& REV_VECTORCALL operator=(xmm_u&& other     ) { *cast<__m128i *>(this) = *cast<__m128i *>(&other); return *this; }
    };

    #if REV_ISA >= REV_ISA_AVX
    union REV_INTRIN_TYPE ymm_u
    {
        s8  s8s[32];
        s16 s16s[16];
        s32 s32s[8];
        s64 s64s[4];

        u8  u8s[32];
        u16 u16s[16];
        u32 u32s[8];
        u64 u64s[4];

        f32 f32s[8];
        f64 f64s[4];

        REV_INLINE ymm_u(__m256         ymm) { _mm256_storeu_ps(f32s, ymm);                       }
        REV_INLINE ymm_u(__m256i        ymm) { _mm256_storeu_si256(cast<__m256i *>(u64s), ymm);   }
        REV_INLINE ymm_u(__m256d        ymm) { _mm256_storeu_pd(f64s, ymm);                       }
        REV_INLINE ymm_u(const ymm_u& other) { *cast<__m256i *>(this) = *cast<__m256i *>(&other); }
        REV_INLINE ymm_u(ymm_u&& other     ) { *cast<__m256i *>(this) = *cast<__m256i *>(&other); }

        REV_INLINE REV_VECTORCALL operator __m256()  const { return _mm256_loadu_ps(f32s);                     }
        REV_INLINE REV_VECTORCALL operator __m256i() const { return _mm256_lddqu_si256(cast<__m256i *>(u64s)); }
        REV_INLINE REV_VECTORCALL operator __m256d() const { return _mm256_loadu_pd(f64s);                     }

        REV_INLINE ymm_u& REV_VECTORCALL operator=(__m256  ymm) { _mm256_storeu_ps(f32s, ymm);                              return *this; }
        REV_INLINE ymm_u& REV_VECTORCALL operator=(__m256i ymm) { _mm256_storeu_si256(cast<__m256i *>(u64s), ymm);          return *this; }
        REV_INLINE ymm_u& REV_VECTORCALL operator=(__m256d ymm) { _mm256_storeu_pd(f64s, ymm);                              return *this; }
        REV_INLINE ymm_u& REV_VECTORCALL operator=(const ymm_u& other) { *cast<__m256i *>(this) = *cast<__m256i *>(&other); return *this; }
        REV_INLINE ymm_u& REV_VECTORCALL operator=(ymm_u&& other     ) { *cast<__m256i *>(this) = *cast<__m256i *>(&other); return *this; }
    };
    #endif

    #if REV_ISA >= REV_ISA_AVX512
    union REV_INTRIN_TYPE zmm_u
    {
        s8  s8s[64];
        s16 s16s[32];
        s32 s32s[16];
        s64 s64s[8];

        u8  u8s[64];
        u16 u16s[32];
        u32 u32s[16];
        u64 u64s[8];

        f32 f32s[16];
        f64 f64s[8];

        REV_INLINE zmm_u(__m512  zmm       ) { _mm512_storeu_ps(f32s, zmm);                       }
        REV_INLINE zmm_u(__m512i zmm       ) { _mm512_storeu_epi64(u64s, zmm);                    }
        REV_INLINE zmm_u(__m512d zmm       ) { _mm512_storeu_pd(f64s, zmm);                       }
        REV_INLINE zmm_u(const zmm_u& other) { *cast<__m512i *>(this) = *cast<__m512i *>(&other); }
        REV_INLINE zmm_u(zmm_u&& other     ) { *cast<__m512i *>(this) = *cast<__m512i *>(&other); }

        REV_INLINE REV_VECTORCALL operator __m512()  const { return _mm512_loadu_ps(f32s);    }
        REV_INLINE REV_VECTORCALL operator __m512i() const { return _mm512_loadu_epi32(u64s); }
        REV_INLINE REV_VECTORCALL operator __m512d() const { return _mm512_loadu_pd(f64s);    }

        REV_INLINE zmm_u& REV_VECTORCALL operator=(__m512  zmm       ) { _mm512_storeu_ps(f32s, zmm);                       return *this; }
        REV_INLINE zmm_u& REV_VECTORCALL operator=(__m512i zmm       ) { _mm512_storeu_epi64(u64s, zmm);                    return *this; }
        REV_INLINE zmm_u& REV_VECTORCALL operator=(__m512d zmm       ) { _mm512_storeu_pd(f64s, zmm);                       return *this; }
        REV_INLINE zmm_u& REV_VECTORCALL operator=(const zmm_u& other) { *cast<__m512i *>(this) = *cast<__m512i *>(&other); return *this; }
        REV_INLINE zmm_u& REV_VECTORCALL operator=(zmm_u&& other     ) { *cast<__m512i *>(this) = *cast<__m512i *>(&other); return *this; }
    };
    #endif

    #undef _MM_SHUFFLE
    enum class MM_SHUFFLE // for _mm_shuffle_*
    {
        XXXX, YXXX, ZXXX, WXXX,
        XYXX, YYXX, ZYXX, WYXX,
        XZXX, YZXX, ZZXX, WZXX,
        XWXX, YWXX, ZWXX, WWXX,
        XXYX, YXYX, ZXYX, WXYX,
        XYYX, YYYX, ZYYX, WYYX,
        XZYX, YZYX, ZZYX, WZYX,
        XWYX, YWYX, ZWYX, WWYX,
        XXZX, YXZX, ZXZX, WXZX,
        XYZX, YYZX, ZYZX, WYZX,
        XZZX, YZZX, ZZZX, WZZX,
        XWZX, YWZX, ZWZX, WWZX,
        XXWX, YXWX, ZXWX, WXWX,
        XYWX, YYWX, ZYWX, WYWX,
        XZWX, YZWX, ZZWX, WZWX,
        XWWX, YWWX, ZWWX, WWWX,

        XXXY, YXXY, ZXXY, WXXY,
        XYXY, YYXY, ZYXY, WYXY,
        XZXY, YZXY, ZZXY, WZXY,
        XWXY, YWXY, ZWXY, WWXY,
        XXYY, YXYY, ZXYY, WXYY,
        XYYY, YYYY, ZYYY, WYYY,
        XZYY, YZYY, ZZYY, WZYY,
        XWYY, YWYY, ZWYY, WWYY,
        XXZY, YXZY, ZXZY, WXZY,
        XYZY, YYZY, ZYZY, WYZY,
        XZZY, YZZY, ZZZY, WZZY,
        XWZY, YWZY, ZWZY, WWZY,
        XXWY, YXWY, ZXWY, WXWY,
        XYWY, YYWY, ZYWY, WYWY,
        XZWY, YZWY, ZZWY, WZWY,
        XWWY, YWWY, ZWWY, WWWY,

        XXXZ, YXXZ, ZXXZ, WXXZ,
        XYXZ, YYXZ, ZYXZ, WYXZ,
        XZXZ, YZXZ, ZZXZ, WZXZ,
        XWXZ, YWXZ, ZWXZ, WWXZ,
        XXYZ, YXYZ, ZXYZ, WXYZ,
        XYYZ, YYYZ, ZYYZ, WYYZ,
        XZYZ, YZYZ, ZZYZ, WZYZ,
        XWYZ, YWYZ, ZWYZ, WWYZ,
        XXZZ, YXZZ, ZXZZ, WXZZ,
        XYZZ, YYZZ, ZYZZ, WYZZ,
        XZZZ, YZZZ, ZZZZ, WZZZ,
        XWZZ, YWZZ, ZWZZ, WWZZ,
        XXWZ, YXWZ, ZXWZ, WXWZ,
        XYWZ, YYWZ, ZYWZ, WYWZ,
        XZWZ, YZWZ, ZZWZ, WZWZ,
        XWWZ, YWWZ, ZWWZ, WWWZ,

        XXXW, YXXW, ZXXW, WXXW,
        XYXW, YYXW, ZYXW, WYXW,
        XZXW, YZXW, ZZXW, WZXW,
        XWXW, YWXW, ZWXW, WWXW,
        XXYW, YXYW, ZXYW, WXYW,
        XYYW, YYYW, ZYYW, WYYW,
        XZYW, YZYW, ZZYW, WZYW,
        XWYW, YWYW, ZWYW, WWYW,
        XXZW, YXZW, ZXZW, WXZW,
        XYZW, YYZW, ZYZW, WYZW,
        XZZW, YZZW, ZZZW, WZZW,
        XWZW, YWZW, ZWZW, WWZW,
        XXWW, YXWW, ZXWW, WXWW,
        XYWW, YYWW, ZYWW, WYWW,
        XZWW, YZWW, ZZWW, WZWW,
        XWWW, YWWW, ZWWW, WWWW
    };

    REV_INLINE bool REV_VECTORCALL mm_equals(const void *a, const void *b)
    {
    #if REV_ISA >= REV_ISA_AVX512
        return 0b11 == _mm_cmpeq_epi64_mask(*cast<__m128i *>(a),
                                            *cast<__m128i *>(b));
    #else
        return _mm_testc_si128(_mm_cmpeq_epi64(*cast<__m128i *>(a),
                                               *cast<__m128i *>(b)),
                               _mm_set1_epi64x(REV_U64_MAX));
    #endif
    }

    REV_INLINE bool REV_VECTORCALL mm_not_equals(const void *a, const void *b)
    {
    #if REV_ISA >= REV_ISA_AVX512
        return _mm_cmpneq_epi64_mask(*cast<__m128i *>(a),
                                     *cast<__m128i *>(b));
    #else
        __m128i cmp  = _mm_cmpeq_epi64(*cast<__m128i *>(a),
                                       *cast<__m128i *>(b));
        __m128i mask = _mm_set1_epi64x(REV_U64_MAX);
        return _mm_testc_si128(_mm_andnot_si128(cmp, mask), mask);
    #endif
    }

    REV_INLINE bool REV_VECTORCALL mm256_equals(const void *a, const void *b)
    {
    #if REV_ISA >= REV_ISA_AVX512
        return 0b1111 == _mm256_cmpeq_epi64_mask(*cast<__m256i *>(a),
                                                 *cast<__m256i *>(b));
    #elif REV_ISA >= REV_ISA_AVX2
        return _mm256_testc_si256(_mm256_cmpeq_epi64(*cast<__m256i *>(a),
                                                     *cast<__m256i *>(b)),
                                  _mm256_set1_epi64x(REV_U64_MAX));
    #elif REV_ISA >= REV_ISA_AVX
        return _mm256_testc_si256(_mm256_castpd_si256(_mm256_cmp_pd(*cast<__m256d *>(a),
                                                                    *cast<__m256d *>(b),
                                                                    _CMP_EQ_OQ)),
                                  _mm256_set1_epi64x(REV_U64_MAX));
    #else
        __m128i *mm_a = cast<__m128i *>(a);
        __m128i *mm_b = cast<__m128i *>(b);
        __m128i  cmpl = _mm_cmpeq_epi64(mm_a[0], mm_b[0]);
        __m128i  cmph = _mm_cmpeq_epi64(mm_a[1], mm_b[1]);
        __m128i  mask = _mm_set1_epi64x(REV_U64_MAX);
        return _mm_testc_si128(cmpl, mask)
            && _mm_testc_si128(cmph, mask);
    #endif
    }

    REV_INLINE bool REV_VECTORCALL mm256_not_equals(const void *a, const void *b)
    {
    #if REV_ISA >= REV_ISA_AVX512
        return _mm256_cmpneq_epi64_mask(*cast<__m256i *>(a),
                                        *cast<__m256i *>(b));
    #elif REV_ISA >= REV_ISA_AVX2
        __m256i cmp  = _mm256_cmpeq_epi64(*cast<__m256i *>(a),
                                          *cast<__m256i *>(b));
        __m256i mask = _mm256_set1_epi64x(REV_U64_MAX);
        return _mm256_testc_si256(_mm256_andnot_si256(cmp, mask), mask);
    #elif REV_ISA >= REV_ISA_AVX
        return _mm256_testc_si256(_mm256_castpd_si256(_mm256_cmp_pd(*cast<__m256d *>(a),
                                                                    *cast<__m256d *>(b),
                                                                    _CMP_NEQ_OQ)),
                                  _mm256_set1_epi64x(REV_U64_MAX));
    #else
        __m128i *mm_a = cast<__m128i *>(a);
        __m128i *mm_b = cast<__m128i *>(b);
        __m128i  cmpl = _mm_cmpeq_epi64(mm_a[0], mm_b[0]);
        __m128i  cmph = _mm_cmpeq_epi64(mm_a[1], mm_b[1]);
        __m128i  mask = _mm_set1_epi64x(REV_U64_MAX);
        return _mm_testc_si128(_mm_andnot_si128(cmpl, mask), mask)
            || _mm_testc_si128(_mm_andnot_si128(cmph, mask), mask);
    #endif
    }

    REV_INLINE bool REV_VECTORCALL mm512_equals(const void *a, const void *b)
    {
    #if REV_ISA >= REV_ISA_AVX512
        return 0xFF == _mm512_cmpeq_epi64_mask(*cast<__m512i *>(a),
                                               *cast<__m512i *>(b));
    #elif REV_ISA >= REV_ISA_AVX2
        __m256i *mm_a = cast<__m256i *>(a);
        __m256i *mm_b = cast<__m256i *>(b);
        __m256i  cmpl = _mm256_cmpeq_epi64(mm_a[0], mm_b[0]);
        __m256i  cmph = _mm256_cmpeq_epi64(mm_a[1], mm_b[1]);
        __m256i  mask = _mm256_set1_epi64x(REV_U64_MAX);
        return _mm256_testc_si256(cmpl, mask)
            && _mm256_testc_si256(cmph, mask);
    #elif REV_ISA >= REV_ISA_AVX
        __m256d *mm_a = cast<__m256d *>(a);
        __m256d *mm_b = cast<__m256d *>(b);
        __m256i  cmpl = _mm256_castpd_si256(_mm256_cmp_pd(mm_a[0], mm_b[0], _CMP_EQ_OQ));
        __m256i  cmph = _mm256_castpd_si256(_mm256_cmp_pd(mm_a[1], mm_b[1], _CMP_EQ_OQ));
        __m256i  mask = _mm256_set1_epi64x(REV_U64_MAX);
        return _mm256_testc_si256(cmpl, mask)
            && _mm256_testc_si256(cmph, mask);
    #else
        __m128i *mm_a = cast<__m128i *>(a);
        __m128i *mm_b = cast<__m128i *>(b);
        __m128i  cmp0 = _mm_cmpeq_epi64(mm_a[0], mm_b[0]);
        __m128i  cmp1 = _mm_cmpeq_epi64(mm_a[1], mm_b[1]);
        __m128i  cmp2 = _mm_cmpeq_epi64(mm_a[2], mm_b[2]);
        __m128i  cmp3 = _mm_cmpeq_epi64(mm_a[3], mm_b[3]);
        __m128i  mask = _mm_set1_epi64x(REV_U64_MAX);
        return _mm_testc_si128(cmp0, mask)
            && _mm_testc_si128(cmp1, mask)
            && _mm_testc_si128(cmp2, mask)
            && _mm_testc_si128(cmp3, mask);
    #endif
    }

    REV_INLINE bool REV_VECTORCALL mm512_not_equals(const void *a, const void *b)
    {
    #if REV_ISA >= REV_ISA_AVX512
        return _mm512_cmpneq_epi64_mask(*cast<__m512i *>(a),
                                        *cast<__m512i *>(b));
    #elif REV_ISA >= REV_ISA_AVX2
        __m256i *mm_a = cast<__m256i *>(a);
        __m256i *mm_b = cast<__m256i *>(b);
        __m256i  cmpl = _mm256_cmpeq_epi64(mm_a[0], mm_b[0]);
        __m256i  cmph = _mm256_cmpeq_epi64(mm_a[1], mm_b[1]);
        __m256i  mask = _mm256_set1_epi64x(REV_U64_MAX);
        return _mm256_testc_si256(_mm256_andnot_si256(cmpl, mask), mask)
            || _mm256_testc_si256(_mm256_andnot_si256(cmph, mask), mask);
    #elif REV_ISA >= REV_ISA_AVX
        __m256d *mm_a = cast<__m256d *>(a);
        __m256d *mm_b = cast<__m256d *>(b);
        __m256i  cmpl = _mm256_castpd_si256(_mm256_cmp_pd(mm_a[0], mm_b[0], _CMP_NEQ_OQ));
        __m256i  cmph = _mm256_castpd_si256(_mm256_cmp_pd(mm_a[1], mm_b[1], _CMP_NEQ_OQ));
        __m256i  mask = _mm256_set1_epi64x(REV_U64_MAX);
        return _mm256_testc_si256(cmpl, mask)
            || _mm256_testc_si256(cmph, mask);
    #else
        __m128i *mm_a = cast<__m128i *>(a);
        __m128i *mm_b = cast<__m128i *>(b);
        __m128i  cmp0 = _mm_cmpeq_epi64(mm_a[0], mm_b[0]);
        __m128i  cmp1 = _mm_cmpeq_epi64(mm_a[1], mm_b[1]);
        __m128i  cmp2 = _mm_cmpeq_epi64(mm_a[2], mm_b[2]);
        __m128i  cmp3 = _mm_cmpeq_epi64(mm_a[3], mm_b[3]);
        __m128i  mask = _mm_set1_epi64x(REV_U64_MAX);
        return _mm_testc_si128(_mm_andnot_si128(cmp0, mask), mask)
            || _mm_testc_si128(_mm_andnot_si128(cmp1, mask), mask)
            || _mm_testc_si128(_mm_andnot_si128(cmp2, mask), mask)
            || _mm_testc_si128(_mm_andnot_si128(cmp3, mask), mask);
    #endif
    }

    #undef _MM_EXTRACT_FLOAT
    template<u8 index> REV_INLINE f32 REV_VECTORCALL mm_extract_f32(__m128  mm) { return reg32(_mm_extract_ps(mm, index)).f; }
    template<u8 index> REV_INLINE s32 REV_VECTORCALL mm_extract_s32(__m128i mm) { return _mm_extract_epi32(mm, index); }
    template<u8 index> REV_INLINE u32 REV_VECTORCALL mm_extract_u32(__m128i mm) { return cast<u32>(_mm_extract_epi32(mm, index)); }

    template<> REV_INLINE f32 REV_VECTORCALL mm_extract_f32<0>(__m128  mm) { return _mm_cvtss_f32(mm); }
    template<> REV_INLINE s32 REV_VECTORCALL mm_extract_s32<0>(__m128i mm) { return _mm_cvtsi128_si32(mm); }
    template<> REV_INLINE u32 REV_VECTORCALL mm_extract_u32<0>(__m128i mm) { return cast<u32>(_mm_cvtsi128_si32(mm)); }

    #if REV_ISA >= REV_ISA_AVX
        template<u8 index> REV_INLINE f32 REV_VECTORCALL mm_extract_f32(__m256  mm) { return reg32(_mm256_extract_epi32(ymm(mm).i, index)).f; }
        template<u8 index> REV_INLINE s32 REV_VECTORCALL mm_extract_s32(__m256i mm) { return _mm256_extract_epi32(mm, index); }
        template<u8 index> REV_INLINE u32 REV_VECTORCALL mm_extract_u32(__m256i mm) { return cast<u32>(_mm256_extract_epi32(mm, index)); }

        template<> REV_INLINE f32 REV_VECTORCALL mm_extract_f32<0>(__m256  mm) { return _mm_cvtss_f32(_mm256_castps256_ps128(mm)); }
        template<> REV_INLINE s32 REV_VECTORCALL mm_extract_s32<0>(__m256i mm) { return _mm_cvtsi128_si32(_mm256_castsi256_si128(mm)); }
        template<> REV_INLINE u32 REV_VECTORCALL mm_extract_u32<0>(__m256i mm) { return cast<u32>(_mm_cvtsi128_si32(_mm256_castsi256_si128(mm))); }
    #endif

    #if REV_ISA >= REV_ISA_AVX512
        template<u8 index> REV_INLINE f32 REV_VECTORCALL mm_extract_f32(__m512  mm) { return reg32(_mm_extract_ps(_mm512_extractf32x4_ps(mm, index / 4), index % 4)).f; }
        template<u8 index> REV_INLINE s32 REV_VECTORCALL mm_extract_s32(__m512i mm) { return _mm_extract_epi32(_mm512_extracti32x4_epi32(mm, index / 4), index % 4); }
        template<u8 index> REV_INLINE u32 REV_VECTORCALL mm_extract_u32(__m512i mm) { return cast<u32>(_mm_extract_epi32(_mm512_extracti32x4_epi32(mm, index / 4), index % 4)); }

        template<> REV_INLINE f32 REV_VECTORCALL mm_extract_f32<0>(__m512  mm) { return _mm512_cvtss_f32(mm); }
        template<> REV_INLINE s32 REV_VECTORCALL mm_extract_s32<0>(__m512i mm) { return _mm512_cvtsi512_si32(mm); }
        template<> REV_INLINE u32 REV_VECTORCALL mm_extract_u32<0>(__m512i mm) { return cast<u32>(_mm512_cvtsi512_si32(mm)); }
    #endif

    template<u8 index> REV_INLINE __m128  REV_VECTORCALL mm_insert_f32(__m128  mm, f32 val) { return xmm(_mm_insert_epi32(xmm(mm).i, reg32(val).i, index)).f; }
    template<u8 index> REV_INLINE __m128i REV_VECTORCALL mm_insert_s32(__m128i mm, s32 val) { return _mm_insert_epi32(mm, val, index); }
    template<u8 index> REV_INLINE __m128i REV_VECTORCALL mm_insert_u32(__m128i mm, u32 val) { return _mm_insert_epi32(mm, val, index); }

    #if REV_ISA >= REV_ISA_AVX
        template<u8 index> REV_INLINE __m256  REV_VECTORCALL mm_insert_f32(__m256  mm, f32 val) { return ymm(_mm256_insert_epi32(ymm(mm).i, reg32(val).i, index)).f; }
        template<u8 index> REV_INLINE __m256i REV_VECTORCALL mm_insert_s32(__m256i mm, s32 val) { return _mm256_insert_epi32(mm, val, index); }
        template<u8 index> REV_INLINE __m256i REV_VECTORCALL mm_insert_u32(__m256i mm, u32 val) { return _mm256_insert_epi32(mm, val, index); }
    #endif

    #if REV_ISA >= REV_ISA_AVX512
        template<u8 index>
        REV_INLINE __m512 REV_VECTORCALL mm_insert_f32(__m512 mm, f32 val)
        {
            __m128 paste = _mm512_extractf32x4_ps(mm, index / 4);
            paste = xmm(_mm_insert_epi32(xmm(paste).i, reg32(val).i, index % 4)).f;
            return _mm512_insertf32x4(mm, paste, index / 4);
        }

        template<u8 index>
        REV_INLINE __m512i REV_VECTORCALL mm_insert_s32(__m512i mm, s32 val)
        {
            __m128i paste = _mm512_extracti32x4_epi32(mm, index / 4);
            paste = _mm_insert_epi32(paste, val, index % 4);
            return _mm512_inserti32x4(mm, paste, index / 4);
        }

        template<u8 index>
        REV_INLINE __m512i REV_VECTORCALL mm_insert_u32(__m512i mm, u32 val)
        {
            __m128i paste = _mm512_extracti32x4_epi32(mm, index / 4);
            paste = _mm_insert_epi32(paste, val, index % 4);
            return _mm512_inserti32x4(mm, paste, index / 4);
        }
    #endif
}
