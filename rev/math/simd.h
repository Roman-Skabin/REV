//
// Copyright 2020-2021 Roman Skabin
//

#pragma once

#include "math/math.h"

namespace REV::Math
{
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
        REV_INLINE xmm_u(__m128i xmm       ) { _mm_storeu_si128(cast(__m128i *, u64s), xmm);      }
        REV_INLINE xmm_u(__m128d xmm       ) { _mm_storeu_pd(f64s, xmm);                          }
        REV_INLINE xmm_u(const xmm_u& other) { *cast(__m128i *, this) = *cast(__m128i *, &other); }
        REV_INLINE xmm_u(xmm_u&& other     ) { *cast(__m128i *, this) = *cast(__m128i *, &other); }

        REV_INLINE REV_VECTORCALL operator __m128()  const { return _mm_loadu_ps(f32s);                     }
        REV_INLINE REV_VECTORCALL operator __m128i() const { return _mm_lddqu_si128(cast(__m128i *, u64s)); }
        REV_INLINE REV_VECTORCALL operator __m128d() const { return _mm_loadu_pd(f64s);                     }

        REV_INLINE xmm_u& REV_VECTORCALL operator=(__m128  xmm       ) { _mm_storeu_ps(f32s, xmm);                          return *this; }
        REV_INLINE xmm_u& REV_VECTORCALL operator=(__m128i xmm       ) { _mm_storeu_si128(cast(__m128i *, u64s), xmm);      return *this; }
        REV_INLINE xmm_u& REV_VECTORCALL operator=(__m128d xmm       ) { _mm_storeu_pd(f64s, xmm);                          return *this; }
        REV_INLINE xmm_u& REV_VECTORCALL operator=(const xmm_u& other) { *cast(__m128i *, this) = *cast(__m128i *, &other); return *this; }
        REV_INLINE xmm_u& REV_VECTORCALL operator=(xmm_u&& other     ) { *cast(__m128i *, this) = *cast(__m128i *, &other); return *this; }
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
        REV_INLINE ymm_u(__m256i        ymm) { _mm256_storeu_si256(cast(__m256i *, u64s), ymm);   }
        REV_INLINE ymm_u(__m256d        ymm) { _mm256_storeu_pd(f64s, ymm);                       }
        REV_INLINE ymm_u(const ymm_u& other) { *cast(__m256i *, this) = *cast(__m256i *, &other); }
        REV_INLINE ymm_u(ymm_u&& other     ) { *cast(__m256i *, this) = *cast(__m256i *, &other); }

        REV_INLINE REV_VECTORCALL operator __m256()  const { return _mm256_loadu_ps(f32s);                     }
        REV_INLINE REV_VECTORCALL operator __m256i() const { return _mm256_lddqu_si256(cast(__m256i *, u64s)); }
        REV_INLINE REV_VECTORCALL operator __m256d() const { return _mm256_loadu_pd(f64s);                     }

        REV_INLINE ymm_u& REV_VECTORCALL operator=(__m256  ymm) { _mm256_storeu_ps(f32s, ymm);                              return *this; }
        REV_INLINE ymm_u& REV_VECTORCALL operator=(__m256i ymm) { _mm256_storeu_si256(cast(__m256i *, u64s), ymm);          return *this; }
        REV_INLINE ymm_u& REV_VECTORCALL operator=(__m256d ymm) { _mm256_storeu_pd(f64s, ymm);                              return *this; }
        REV_INLINE ymm_u& REV_VECTORCALL operator=(const ymm_u& other) { *cast(__m256i *, this) = *cast(__m256i *, &other); return *this; }
        REV_INLINE ymm_u& REV_VECTORCALL operator=(ymm_u&& other     ) { *cast(__m256i *, this) = *cast(__m256i *, &other); return *this; }
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
        REV_INLINE zmm_u(const zmm_u& other) { *cast(__m512i *, this) = *cast(__m512i *, &other); }
        REV_INLINE zmm_u(zmm_u&& other     ) { *cast(__m512i *, this) = *cast(__m512i *, &other); }

        REV_INLINE REV_VECTORCALL operator __m512()  const { return _mm512_loadu_ps(f32s);    }
        REV_INLINE REV_VECTORCALL operator __m512i() const { return _mm512_loadu_epi32(u64s); }
        REV_INLINE REV_VECTORCALL operator __m512d() const { return _mm512_loadu_pd(f64s);    }

        REV_INLINE zmm_u& REV_VECTORCALL operator=(__m512  zmm       ) { _mm512_storeu_ps(f32s, zmm);                       return *this; }
        REV_INLINE zmm_u& REV_VECTORCALL operator=(__m512i zmm       ) { _mm512_storeu_epi64(u64s, zmm);                    return *this; }
        REV_INLINE zmm_u& REV_VECTORCALL operator=(__m512d zmm       ) { _mm512_storeu_pd(f64s, zmm);                       return *this; }
        REV_INLINE zmm_u& REV_VECTORCALL operator=(const zmm_u& other) { *cast(__m512i *, this) = *cast(__m512i *, &other); return *this; }
        REV_INLINE zmm_u& REV_VECTORCALL operator=(zmm_u&& other     ) { *cast(__m512i *, this) = *cast(__m512i *, &other); return *this; }
    };
    #endif

    #undef _MM_SHUFFLE
    // @NOTE(Roman): In reversed order: enum[X, Y, Z, W] = register[W, Z, Y, X]
    enum MM_SHUFFLE : int // for _mm_shuffle_*
    {
        MM_SHUFFLE_XXXX, MM_SHUFFLE_YXXX, MM_SHUFFLE_ZXXX, MM_SHUFFLE_WXXX,
        MM_SHUFFLE_XYXX, MM_SHUFFLE_YYXX, MM_SHUFFLE_ZYXX, MM_SHUFFLE_WYXX,
        MM_SHUFFLE_XZXX, MM_SHUFFLE_YZXX, MM_SHUFFLE_ZZXX, MM_SHUFFLE_WZXX,
        MM_SHUFFLE_XWXX, MM_SHUFFLE_YWXX, MM_SHUFFLE_ZWXX, MM_SHUFFLE_WWXX,
        MM_SHUFFLE_XXYX, MM_SHUFFLE_YXYX, MM_SHUFFLE_ZXYX, MM_SHUFFLE_WXYX,
        MM_SHUFFLE_XYYX, MM_SHUFFLE_YYYX, MM_SHUFFLE_ZYYX, MM_SHUFFLE_WYYX,
        MM_SHUFFLE_XZYX, MM_SHUFFLE_YZYX, MM_SHUFFLE_ZZYX, MM_SHUFFLE_WZYX,
        MM_SHUFFLE_XWYX, MM_SHUFFLE_YWYX, MM_SHUFFLE_ZWYX, MM_SHUFFLE_WWYX,
        MM_SHUFFLE_XXZX, MM_SHUFFLE_YXZX, MM_SHUFFLE_ZXZX, MM_SHUFFLE_WXZX,
        MM_SHUFFLE_XYZX, MM_SHUFFLE_YYZX, MM_SHUFFLE_ZYZX, MM_SHUFFLE_WYZX,
        MM_SHUFFLE_XZZX, MM_SHUFFLE_YZZX, MM_SHUFFLE_ZZZX, MM_SHUFFLE_WZZX,
        MM_SHUFFLE_XWZX, MM_SHUFFLE_YWZX, MM_SHUFFLE_ZWZX, MM_SHUFFLE_WWZX,
        MM_SHUFFLE_XXWX, MM_SHUFFLE_YXWX, MM_SHUFFLE_ZXWX, MM_SHUFFLE_WXWX,
        MM_SHUFFLE_XYWX, MM_SHUFFLE_YYWX, MM_SHUFFLE_ZYWX, MM_SHUFFLE_WYWX,
        MM_SHUFFLE_XZWX, MM_SHUFFLE_YZWX, MM_SHUFFLE_ZZWX, MM_SHUFFLE_WZWX,
        MM_SHUFFLE_XWWX, MM_SHUFFLE_YWWX, MM_SHUFFLE_ZWWX, MM_SHUFFLE_WWWX,

        MM_SHUFFLE_XXXY, MM_SHUFFLE_YXXY, MM_SHUFFLE_ZXXY, MM_SHUFFLE_WXXY,
        MM_SHUFFLE_XYXY, MM_SHUFFLE_YYXY, MM_SHUFFLE_ZYXY, MM_SHUFFLE_WYXY,
        MM_SHUFFLE_XZXY, MM_SHUFFLE_YZXY, MM_SHUFFLE_ZZXY, MM_SHUFFLE_WZXY,
        MM_SHUFFLE_XWXY, MM_SHUFFLE_YWXY, MM_SHUFFLE_ZWXY, MM_SHUFFLE_WWXY,
        MM_SHUFFLE_XXYY, MM_SHUFFLE_YXYY, MM_SHUFFLE_ZXYY, MM_SHUFFLE_WXYY,
        MM_SHUFFLE_XYYY, MM_SHUFFLE_YYYY, MM_SHUFFLE_ZYYY, MM_SHUFFLE_WYYY,
        MM_SHUFFLE_XZYY, MM_SHUFFLE_YZYY, MM_SHUFFLE_ZZYY, MM_SHUFFLE_WZYY,
        MM_SHUFFLE_XWYY, MM_SHUFFLE_YWYY, MM_SHUFFLE_ZWYY, MM_SHUFFLE_WWYY,
        MM_SHUFFLE_XXZY, MM_SHUFFLE_YXZY, MM_SHUFFLE_ZXZY, MM_SHUFFLE_WXZY,
        MM_SHUFFLE_XYZY, MM_SHUFFLE_YYZY, MM_SHUFFLE_ZYZY, MM_SHUFFLE_WYZY,
        MM_SHUFFLE_XZZY, MM_SHUFFLE_YZZY, MM_SHUFFLE_ZZZY, MM_SHUFFLE_WZZY,
        MM_SHUFFLE_XWZY, MM_SHUFFLE_YWZY, MM_SHUFFLE_ZWZY, MM_SHUFFLE_WWZY,
        MM_SHUFFLE_XXWY, MM_SHUFFLE_YXWY, MM_SHUFFLE_ZXWY, MM_SHUFFLE_WXWY,
        MM_SHUFFLE_XYWY, MM_SHUFFLE_YYWY, MM_SHUFFLE_ZYWY, MM_SHUFFLE_WYWY,
        MM_SHUFFLE_XZWY, MM_SHUFFLE_YZWY, MM_SHUFFLE_ZZWY, MM_SHUFFLE_WZWY,
        MM_SHUFFLE_XWWY, MM_SHUFFLE_YWWY, MM_SHUFFLE_ZWWY, MM_SHUFFLE_WWWY,

        MM_SHUFFLE_XXXZ, MM_SHUFFLE_YXXZ, MM_SHUFFLE_ZXXZ, MM_SHUFFLE_WXXZ,
        MM_SHUFFLE_XYXZ, MM_SHUFFLE_YYXZ, MM_SHUFFLE_ZYXZ, MM_SHUFFLE_WYXZ,
        MM_SHUFFLE_XZXZ, MM_SHUFFLE_YZXZ, MM_SHUFFLE_ZZXZ, MM_SHUFFLE_WZXZ,
        MM_SHUFFLE_XWXZ, MM_SHUFFLE_YWXZ, MM_SHUFFLE_ZWXZ, MM_SHUFFLE_WWXZ,
        MM_SHUFFLE_XXYZ, MM_SHUFFLE_YXYZ, MM_SHUFFLE_ZXYZ, MM_SHUFFLE_WXYZ,
        MM_SHUFFLE_XYYZ, MM_SHUFFLE_YYYZ, MM_SHUFFLE_ZYYZ, MM_SHUFFLE_WYYZ,
        MM_SHUFFLE_XZYZ, MM_SHUFFLE_YZYZ, MM_SHUFFLE_ZZYZ, MM_SHUFFLE_WZYZ,
        MM_SHUFFLE_XWYZ, MM_SHUFFLE_YWYZ, MM_SHUFFLE_ZWYZ, MM_SHUFFLE_WWYZ,
        MM_SHUFFLE_XXZZ, MM_SHUFFLE_YXZZ, MM_SHUFFLE_ZXZZ, MM_SHUFFLE_WXZZ,
        MM_SHUFFLE_XYZZ, MM_SHUFFLE_YYZZ, MM_SHUFFLE_ZYZZ, MM_SHUFFLE_WYZZ,
        MM_SHUFFLE_XZZZ, MM_SHUFFLE_YZZZ, MM_SHUFFLE_ZZZZ, MM_SHUFFLE_WZZZ,
        MM_SHUFFLE_XWZZ, MM_SHUFFLE_YWZZ, MM_SHUFFLE_ZWZZ, MM_SHUFFLE_WWZZ,
        MM_SHUFFLE_XXWZ, MM_SHUFFLE_YXWZ, MM_SHUFFLE_ZXWZ, MM_SHUFFLE_WXWZ,
        MM_SHUFFLE_XYWZ, MM_SHUFFLE_YYWZ, MM_SHUFFLE_ZYWZ, MM_SHUFFLE_WYWZ,
        MM_SHUFFLE_XZWZ, MM_SHUFFLE_YZWZ, MM_SHUFFLE_ZZWZ, MM_SHUFFLE_WZWZ,
        MM_SHUFFLE_XWWZ, MM_SHUFFLE_YWWZ, MM_SHUFFLE_ZWWZ, MM_SHUFFLE_WWWZ,

        MM_SHUFFLE_XXXW, MM_SHUFFLE_YXXW, MM_SHUFFLE_ZXXW, MM_SHUFFLE_WXXW,
        MM_SHUFFLE_XYXW, MM_SHUFFLE_YYXW, MM_SHUFFLE_ZYXW, MM_SHUFFLE_WYXW,
        MM_SHUFFLE_XZXW, MM_SHUFFLE_YZXW, MM_SHUFFLE_ZZXW, MM_SHUFFLE_WZXW,
        MM_SHUFFLE_XWXW, MM_SHUFFLE_YWXW, MM_SHUFFLE_ZWXW, MM_SHUFFLE_WWXW,
        MM_SHUFFLE_XXYW, MM_SHUFFLE_YXYW, MM_SHUFFLE_ZXYW, MM_SHUFFLE_WXYW,
        MM_SHUFFLE_XYYW, MM_SHUFFLE_YYYW, MM_SHUFFLE_ZYYW, MM_SHUFFLE_WYYW,
        MM_SHUFFLE_XZYW, MM_SHUFFLE_YZYW, MM_SHUFFLE_ZZYW, MM_SHUFFLE_WZYW,
        MM_SHUFFLE_XWYW, MM_SHUFFLE_YWYW, MM_SHUFFLE_ZWYW, MM_SHUFFLE_WWYW,
        MM_SHUFFLE_XXZW, MM_SHUFFLE_YXZW, MM_SHUFFLE_ZXZW, MM_SHUFFLE_WXZW,
        MM_SHUFFLE_XYZW, MM_SHUFFLE_YYZW, MM_SHUFFLE_ZYZW, MM_SHUFFLE_WYZW,
        MM_SHUFFLE_XZZW, MM_SHUFFLE_YZZW, MM_SHUFFLE_ZZZW, MM_SHUFFLE_WZZW,
        MM_SHUFFLE_XWZW, MM_SHUFFLE_YWZW, MM_SHUFFLE_ZWZW, MM_SHUFFLE_WWZW,
        MM_SHUFFLE_XXWW, MM_SHUFFLE_YXWW, MM_SHUFFLE_ZXWW, MM_SHUFFLE_WXWW,
        MM_SHUFFLE_XYWW, MM_SHUFFLE_YYWW, MM_SHUFFLE_ZYWW, MM_SHUFFLE_WYWW,
        MM_SHUFFLE_XZWW, MM_SHUFFLE_YZWW, MM_SHUFFLE_ZZWW, MM_SHUFFLE_WZWW,
        MM_SHUFFLE_XWWW, MM_SHUFFLE_YWWW, MM_SHUFFLE_ZWWW, MM_SHUFFLE_WWWW
    };

    enum MM_SIGN : int // for _mm*_sign_*
    {
        MM_SIGN_NEG  = -1,
        MM_SIGN_ZERO =  0,
        MM_SIGN_POS  =  1
    };

    // @NOTE(Roman): In reversed order: enum[X, Y, Z, W] = register[W, Z, Y, X]
    enum MM_BLEND : int // for _mm_blend_[ps, epi32]
    {
        MM_BLEND_AAAA, MM_BLEND_BAAA,
        MM_BLEND_ABAA, MM_BLEND_BBAA,
        MM_BLEND_AABA, MM_BLEND_BABA,
        MM_BLEND_ABBA, MM_BLEND_BBBA,

        MM_BLEND_AAAB, MM_BLEND_BAAB,
        MM_BLEND_ABAB, MM_BLEND_BBAB,
        MM_BLEND_AABB, MM_BLEND_BABB,
        MM_BLEND_ABBB, MM_BLEND_BBBB
    };

    REV_INLINE bool REV_VECTORCALL mm_equals(const void *a, const void *b)
    {
    #if REV_ISA >= REV_ISA_AVX512
        return 0b11 == _mm_cmpeq_epi64_mask(*cast(__m128i *, a),
                                            *cast(__m128i *, b));
    #else
        return _mm_testc_si128(_mm_cmpeq_epi64(*cast(__m128i *, a),
                                               *cast(__m128i *, b)),
                               _mm_set1_epi64x(REV_U64_MAX));
    #endif
    }

    REV_INLINE bool REV_VECTORCALL mm_not_equals(const void *a, const void *b)
    {
    #if REV_ISA >= REV_ISA_AVX512
        return _mm_cmpneq_epi64_mask(*cast(__m128i *, a),
                                     *cast(__m128i *, b));
    #else
        __m128i cmp  = _mm_cmpeq_epi64(*cast(__m128i *, a),
                                       *cast(__m128i *, b));
        __m128i mask = _mm_set1_epi64x(REV_U64_MAX);
        return _mm_testc_si128(_mm_andnot_si128(cmp, mask), mask);
    #endif
    }

    REV_INLINE bool REV_VECTORCALL mm256_equals(const void *a, const void *b)
    {
    #if REV_ISA >= REV_ISA_AVX512
        return 0b1111 == _mm256_cmpeq_epi64_mask(*cast(__m256i *, a),
                                                 *cast(__m256i *, b));
    #elif REV_ISA >= REV_ISA_AVX2
        return _mm256_testc_si256(_mm256_cmpeq_epi64(*cast(__m256i *, a),
                                                     *cast(__m256i *, b)),
                                  _mm256_set1_epi64x(REV_U64_MAX));
    #elif REV_ISA >= REV_ISA_AVX
        return _mm256_testc_si256(_mm256_castpd_si256(_mm256_cmp_pd(*cast(__m256d *, a),
                                                                    *cast(__m256d *, b),
                                                                    _CMP_EQ_OQ)),
                                  _mm256_set1_epi64x(REV_U64_MAX));
    #else
        __m128i *mm_a = cast(__m128i *, a);
        __m128i *mm_b = cast(__m128i *, b);
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
        return _mm256_cmpneq_epi64_mask(*cast(__m256i *, a),
                                        *cast(__m256i *, b));
    #elif REV_ISA >= REV_ISA_AVX2
        __m256i cmp  = _mm256_cmpeq_epi64(*cast(__m256i *, a),
                                          *cast(__m256i *, b));
        __m256i mask = _mm256_set1_epi64x(REV_U64_MAX);
        return _mm256_testc_si256(_mm256_andnot_si256(cmp, mask), mask);
    #elif REV_ISA >= REV_ISA_AVX
        return _mm256_testc_si256(_mm256_castpd_si256(_mm256_cmp_pd(*cast(__m256d *, a),
                                                                    *cast(__m256d *, b),
                                                                    _CMP_NEQ_OQ)),
                                  _mm256_set1_epi64x(REV_U64_MAX));
    #else
        __m128i *mm_a = cast(__m128i *, a);
        __m128i *mm_b = cast(__m128i *, b);
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
        return 0xFF == _mm512_cmpeq_epi64_mask(*cast(__m512i *, a),
                                               *cast(__m512i *, b));
    #elif REV_ISA >= REV_ISA_AVX2
        __m256i *mm_a = cast(__m256i *, a);
        __m256i *mm_b = cast(__m256i *, b);
        __m256i  cmpl = _mm256_cmpeq_epi64(mm_a[0], mm_b[0]);
        __m256i  cmph = _mm256_cmpeq_epi64(mm_a[1], mm_b[1]);
        __m256i  mask = _mm256_set1_epi64x(REV_U64_MAX);
        return _mm256_testc_si256(cmpl, mask)
            && _mm256_testc_si256(cmph, mask);
    #elif REV_ISA >= REV_ISA_AVX
        __m256d *mm_a = cast(__m256d *, a);
        __m256d *mm_b = cast(__m256d *, b);
        __m256i  cmpl = _mm256_castpd_si256(_mm256_cmp_pd(mm_a[0], mm_b[0], _CMP_EQ_OQ));
        __m256i  cmph = _mm256_castpd_si256(_mm256_cmp_pd(mm_a[1], mm_b[1], _CMP_EQ_OQ));
        __m256i  mask = _mm256_set1_epi64x(REV_U64_MAX);
        return _mm256_testc_si256(cmpl, mask)
            && _mm256_testc_si256(cmph, mask);
    #else
        __m128i *mm_a = cast(__m128i *, a);
        __m128i *mm_b = cast(__m128i *, b);
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
        return _mm512_cmpneq_epi64_mask(*cast(__m512i *, a),
                                        *cast(__m512i *, b));
    #elif REV_ISA >= REV_ISA_AVX2
        __m256i *mm_a = cast(__m256i *, a);
        __m256i *mm_b = cast(__m256i *, b);
        __m256i  cmpl = _mm256_cmpeq_epi64(mm_a[0], mm_b[0]);
        __m256i  cmph = _mm256_cmpeq_epi64(mm_a[1], mm_b[1]);
        __m256i  mask = _mm256_set1_epi64x(REV_U64_MAX);
        return _mm256_testc_si256(_mm256_andnot_si256(cmpl, mask), mask)
            || _mm256_testc_si256(_mm256_andnot_si256(cmph, mask), mask);
    #elif REV_ISA >= REV_ISA_AVX
        __m256d *mm_a = cast(__m256d *, a);
        __m256d *mm_b = cast(__m256d *, b);
        __m256i  cmpl = _mm256_castpd_si256(_mm256_cmp_pd(mm_a[0], mm_b[0], _CMP_NEQ_OQ));
        __m256i  cmph = _mm256_castpd_si256(_mm256_cmp_pd(mm_a[1], mm_b[1], _CMP_NEQ_OQ));
        __m256i  mask = _mm256_set1_epi64x(REV_U64_MAX);
        return _mm256_testc_si256(cmpl, mask)
            || _mm256_testc_si256(cmph, mask);
    #else
        __m128i *mm_a = cast(__m128i *, a);
        __m128i *mm_b = cast(__m128i *, b);
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
    template<u8 index> REV_INLINE f32 REV_VECTORCALL mm_extract_f32(__m128  mm) { int i = _mm_extract_ps(mm, index); return *cast(f32 *, &i); }
    template<u8 index> REV_INLINE s32 REV_VECTORCALL mm_extract_s32(__m128i mm) { return _mm_extract_epi32(mm, index); }
    template<u8 index> REV_INLINE u32 REV_VECTORCALL mm_extract_u32(__m128i mm) { return cast(u32, _mm_extract_epi32(mm, index)); }

    template<> REV_INLINE f32 REV_VECTORCALL mm_extract_f32<0>(__m128  mm) { return _mm_cvtss_f32(mm); }
    template<> REV_INLINE s32 REV_VECTORCALL mm_extract_s32<0>(__m128i mm) { return _mm_cvtsi128_si32(mm); }
    template<> REV_INLINE u32 REV_VECTORCALL mm_extract_u32<0>(__m128i mm) { return cast(u32, _mm_cvtsi128_si32(mm)); }

    #if REV_ISA >= REV_ISA_AVX
        template<u8 index> REV_INLINE f32 REV_VECTORCALL mm_extract_f32(__m256  mm) { int i = _mm256_extract_epi32(_mm256_castps_si256(mm), index); return *cast(f32 *, &i); }
        template<u8 index> REV_INLINE s32 REV_VECTORCALL mm_extract_s32(__m256i mm) { return _mm256_extract_epi32(mm, index); }
        template<u8 index> REV_INLINE u32 REV_VECTORCALL mm_extract_u32(__m256i mm) { return cast(u32, _mm256_extract_epi32(mm, index)); }

        template<> REV_INLINE f32 REV_VECTORCALL mm_extract_f32<0>(__m256  mm) { return _mm_cvtss_f32(_mm256_castps256_ps128(mm)); }
        template<> REV_INLINE s32 REV_VECTORCALL mm_extract_s32<0>(__m256i mm) { return _mm_cvtsi128_si32(_mm256_castsi256_si128(mm)); }
        template<> REV_INLINE u32 REV_VECTORCALL mm_extract_u32<0>(__m256i mm) { return cast(u32, _mm_cvtsi128_si32(_mm256_castsi256_si128(mm))); }
    #endif

    #if REV_ISA >= REV_ISA_AVX512
        template<u8 index> REV_INLINE f32 REV_VECTORCALL mm_extract_f32(__m512  mm) { int i = _mm_extract_ps(_mm512_extractf32x4_ps(mm, index / 4), index % 4); return *cast(f32 *, &i); }
        template<u8 index> REV_INLINE s32 REV_VECTORCALL mm_extract_s32(__m512i mm) { return _mm_extract_epi32(_mm512_extracti32x4_epi32(mm, index / 4), index % 4); }
        template<u8 index> REV_INLINE u32 REV_VECTORCALL mm_extract_u32(__m512i mm) { return cast(u32, _mm_extract_epi32(_mm512_extracti32x4_epi32(mm, index / 4), index % 4)); }

        template<> REV_INLINE f32 REV_VECTORCALL mm_extract_f32<0>(__m512  mm) { return _mm512_cvtss_f32(mm); }
        template<> REV_INLINE s32 REV_VECTORCALL mm_extract_s32<0>(__m512i mm) { return _mm512_cvtsi512_si32(mm); }
        template<> REV_INLINE u32 REV_VECTORCALL mm_extract_u32<0>(__m512i mm) { return cast(u32, _mm512_cvtsi512_si32(mm)); }
    #endif

    template<u8 index> REV_INLINE __m128  REV_VECTORCALL mm_insert_f32(__m128  mm, f32 val) { return _mm_insert_ps(mm, _mm_set_ss(val), index); }
    template<u8 index> REV_INLINE __m128i REV_VECTORCALL mm_insert_s32(__m128i mm, s32 val) { return _mm_insert_epi32(mm, val, index); }
    template<u8 index> REV_INLINE __m128i REV_VECTORCALL mm_insert_u32(__m128i mm, u32 val) { return _mm_insert_epi32(mm, val, index); }

    #if REV_ISA >= REV_ISA_AVX
        template<u8 index> REV_INLINE __m256  REV_VECTORCALL mm_insert_f32(__m256  mm, f32 val) { return _mm256_insertf128_ps(mm, _mm_insert_ps(_mm256_extractf128_ps(mm, index / 4), _mm_set_ss(val), index % 4), index / 4); }
        template<u8 index> REV_INLINE __m256i REV_VECTORCALL mm_insert_s32(__m256i mm, s32 val) { return _mm256_insert_epi32(mm, val, index); }
        template<u8 index> REV_INLINE __m256i REV_VECTORCALL mm_insert_u32(__m256i mm, u32 val) { return _mm256_insert_epi32(mm, val, index); }
    #endif

    #if REV_ISA >= REV_ISA_AVX512
        template<u8 index> REV_INLINE __m512  REV_VECTORCALL mm_insert_f32(__m512  mm, f32 val) { return _mm512_insertf32x4(mm, _mm_insert_ps(_mm512_extractf32x4_ps(mm, index / 4), _mm_set_ss(val), index % 4), index / 4); }
        template<u8 index> REV_INLINE __m512i REV_VECTORCALL mm_insert_s32(__m512i mm, s32 val) { return _mm512_inserti32x4(mm, _mm_insert_epi32(_mm512_extracti32x4_epi32(mm, index / 4), val, index % 4), index / 4); }
        template<u8 index> REV_INLINE __m512i REV_VECTORCALL mm_insert_u32(__m512i mm, u32 val) { return _mm512_inserti32x4(mm, _mm_insert_epi32(_mm512_extracti32x4_epi32(mm, index / 4), val, index % 4), index / 4); }
    #endif

    REV_INLINE __m128 REV_VECTORCALL mm_cvtepu32_ps(__m128i mm)
    {
        __m128i half_mmi = _mm_srli_epi32(mm, 1);
        __m128i saved_1  = _mm_and_si128(mm, _mm_set1_epi32(1));
        __m128  half_mmf = _mm_cvtepi32_ps(half_mmi);
        __m128  saved_1f = _mm_cvtepi32_ps(saved_1);
        return _mm_add_ps(_mm_add_ps(half_mmf, half_mmf), saved_1f);
    }

    REV_INLINE __m128d REV_VECTORCALL mm_cvtepu64_pd(__m128i mm)
    {
        __m128i half_mmi = _mm_srli_epi64(mm, 1);
        __m128i saved_1  = _mm_and_si128(mm, _mm_set1_epi64x(1));
        __m128d zero     = _mm_setzero_pd();

        __m128d hmd_0    = _mm_cvtsi64_sd(zero, _mm_cvtsi128_si64x(half_mmi));
        __m128d hmd_1    = _mm_cvtsi64_sd(zero, _mm_extract_epi64(half_mmi, 1));
        __m128d half_mmd = _mm_shuffle_pd(hmd_0, hmd_1, 0);

        __m128d s1d_0    = _mm_cvtsi64_sd(zero, _mm_cvtsi128_si64x(saved_1));
        __m128d s1d_1    = _mm_cvtsi64_sd(zero, _mm_extract_epi64(saved_1, 1));
        __m128d saved_1d = _mm_shuffle_pd(s1d_0, s1d_1, 0);

        return _mm_add_pd(_mm_add_pd(half_mmd, half_mmd), saved_1d);
    }

    REV_INLINE __m128d REV_VECTORCALL mm_cvtu64_sd(__m128i mm)
    {
        __m128i half_mmi = _mm_srli_epi64(mm, 1);
        __m128i saved_1  = _mm_and_si128(mm, _mm_cvtsi64_si128(1));
        __m128d zero     = _mm_setzero_pd();

        __m128d half_mmd = _mm_cvtsi64_sd(zero, _mm_cvtsi128_si64x(half_mmi));
        __m128d saved_1d = _mm_cvtsi64_sd(zero, _mm_cvtsi128_si64x(saved_1));

        return _mm_add_pd(_mm_add_pd(half_mmd, half_mmd), saved_1d);
    }
}
