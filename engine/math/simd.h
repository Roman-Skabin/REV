//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/core.h"

#pragma pack(push, 1)

#if ENGINE_ISA >= ENGINE_ISA_SSE
union ENGINE_INTRIN_TYPE ENGINE_ALIGN(16) xmm
{
    __m128  f;
    __m128i i;
    __m128d d;

    xmm(            ) : f(_mm_setzero_ps()) {}
    xmm(__m128     v) : f(v)                {}
    xmm(__m128i    v) : i(v)                {}
    xmm(__m128d    v) : d(v)                {}
    xmm(const xmm& v) : f(v.f)              {}
    xmm(xmm&&      v) : f(v.f)              {}

    MATH_CALL operator __m128()  const { return f; }
    MATH_CALL operator __m128i() const { return i; }
    MATH_CALL operator __m128d() const { return d; }

    xmm& MATH_CALL operator=(__m128     v) { f = v;   return *this; }
    xmm& MATH_CALL operator=(__m128i    v) { i = v;   return *this; }
    xmm& MATH_CALL operator=(__m128d    v) { d = v;   return *this; }
    xmm& MATH_CALL operator=(const xmm& v) { f = v.f; return *this; }
    xmm& MATH_CALL operator=(xmm&&      v) { f = v.f; return *this; }
};
#endif

#if ENGINE_ISA >= ENGINE_ISA_AVX
union ENGINE_INTRIN_TYPE ENGINE_ALIGN(32) ymm
{
    __m256  f;
    __m256i i;
    __m256d d;

    ymm(            ) : f(_mm256_setzero_ps()) {}
    ymm(__m256     v) : f(v)                   {}
    ymm(__m256i    v) : i(v)                   {}
    ymm(__m256d    v) : d(v)                   {}
    ymm(const ymm& v) : f(v.f)                 {}
    ymm(ymm&&      v) : f(v.f)                 {}

    MATH_CALL operator __m256()  const { return f; }
    MATH_CALL operator __m256i() const { return i; }
    MATH_CALL operator __m256d() const { return d; }

    ymm& MATH_CALL operator=(__m256     v) { f = v;   return *this; }
    ymm& MATH_CALL operator=(__m256i    v) { i = v;   return *this; }
    ymm& MATH_CALL operator=(__m256d    v) { d = v;   return *this; }
    ymm& MATH_CALL operator=(const ymm& v) { f = v.f; return *this; }
    ymm& MATH_CALL operator=(ymm&&      v) { f = v.f; return *this; }
};
#endif

#if ENGINE_ISA >= ENGINE_ISA_AVX512
union ENGINE_INTRIN_TYPE ENGINE_ALIGN(64) zmm
{
    __m512  f;
    __m512i i;
    __m512d d;

    zmm(            ) : f(_mm512_setzero_ps()) {}
    zmm(__m512     v) : f(v)                   {}
    zmm(__m512i    v) : i(v)                   {}
    zmm(__m512d    v) : d(v)                   {}
    zmm(const zmm& v) : f(v.f)                 {}
    zmm(zmm&&      v) : f(v.f)                 {}

    MATH_CALL operator __m512()  const { return f; }
    MATH_CALL operator __m512i() const { return i; }
    MATH_CALL operator __m512d() const { return d; }

    zmm& MATH_CALL operator=(__m512     v) { f = v;   return *this; }
    zmm& MATH_CALL operator=(__m512i    v) { i = v;   return *this; }
    zmm& MATH_CALL operator=(__m512d    v) { d = v;   return *this; }
    zmm& MATH_CALL operator=(const zmm& v) { f = v.f; return *this; }
    zmm& MATH_CALL operator=(zmm&&      v) { f = v.f; return *this; }
};
#endif

#if ENGINE_ISA >= ENGINE_ISA_SSE
    template<typename T, typename MMType, typename = RTTI::enable_if_t<RTTI::is_arithmetic_v<T> && RTTI::is_simd_v<MMType>>>
    constexpr T MATH_CALL MM(MMType mm, u8 index) { return cast<T *>(mm)[index]; }

    constexpr f32 MATH_CALL MMf(__m128  mm, u8 index) { return mm.m128_f32[index];  }
    constexpr s32 MATH_CALL MMi(__m128i mm, u8 index) { return mm.m128i_i32[index]; }
    constexpr u32 MATH_CALL MMu(__m128i mm, u8 index) { return mm.m128i_u32[index]; }

    constexpr f32 MATH_CALL MM128f(__m128  mm, u8 index) { return mm.m128_f32[index];  }
    constexpr s32 MATH_CALL MM128i(__m128i mm, u8 index) { return mm.m128i_i32[index]; }
    constexpr u32 MATH_CALL MM128u(__m128i mm, u8 index) { return mm.m128i_u32[index]; }
#endif

#if ENGINE_ISA >= ENGINE_ISA_AVX
    constexpr f32 MATH_CALL MM256f(__m256  mm, u8 index) { return mm.m256_f32[index];  }
    constexpr s32 MATH_CALL MM256i(__m256i mm, u8 index) { return mm.m256i_i32[index]; }
    constexpr u32 MATH_CALL MM256u(__m256i mm, u8 index) { return mm.m256i_u32[index]; }
#endif

#if ENGINE_ISA >= ENGINE_ISA_AVX512
    constexpr f32 MATH_CALL MM512f(__m512  mm, u8 index) { return mm.m512_f32[index];  }
    constexpr s32 MATH_CALL MM512i(__m512i mm, u8 index) { return mm.m512i_i32[index]; }
    constexpr u32 MATH_CALL MM512u(__m512i mm, u8 index) { return mm.m512i_u32[index]; }
#endif

#if ENGINE_ISA >= ENGINE_ISA_SSE
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
#endif

template<typename T, typename = RTTI::enable_if_t<RTTI::is_arithmetic_v<T>>>
INLINE b32 MATH_CALL mm_equals(T *a, T *b)
{
#if ENGINE_ISA >= ENGINE_ISA_AVX512
    return 0b11 == _mm_cmpeq_epi64_mask(*cast<__m128i *>(a),
                                        *cast<__m128i *>(b));
#elif ENGINE_ISA >= ENGINE_ISA_SSE
    __m128i eq_mask = _mm_cmpeq_epi64(*cast<__m128i *>(a),
                                      *cast<__m128i *>(b));
    __m128i ones = _mm_cmpeq_epi64(eq_mask, eq_mask);
    return _mm_testc_si128(eq_mask, ones);
#else
    u64 *mm_a = cast<u64 *>(a);
    u64 *mm_b = cast<u64 *>(b);
    return *(mm_a + 0) == *(mm_b + 0)
        && *(mm_a + 1) == *(mm_b + 1);
#endif
}

template<typename T, typename = RTTI::enable_if_t<RTTI::is_arithmetic_v<T>>>
INLINE b32 MATH_CALL mm256_equals(T *a, T *b)
{
#if ENGINE_ISA >= ENGINE_ISA_AVX512
    return 0x0F == _mm256_cmpeq_epi64_mask(*cast<__m256i *>(a),
                                           *cast<__m256i *>(b));
#elif ENGINE_ISA >= ENGINE_ISA_AVX2
    __m256i eq_mask = _mm256_cmpeq_epi64(*cast<__m256i *>(a),  
                                         *cast<__m256i *>(b));
    __m256i ones    = _mm256_cmpeq_epi64(eq_mask, eq_mask);
    return _mm256_testc_si256(eq_mask, ones);
#elif ENGINE_ISA >= ENGINE_ISA_AVX
    __m256d eq_mask = _mm256_cmp_pd(*cast<__m256d *>(a),
                                    *cast<__m256d *>(b),
                                    _CMP_EQ_OQ);
    __m256d ones = _mm256_cmp_pd(eq_mask, eq_mask, _CMP_EQ_OQ);
    return _mm256_testc_si256(_mm256_castpd_si256(eq_mask),
                              _mm256_castpd_si256(ones));
#elif ENGINE_ISA >= ENGINE_ISA_SSE
    __m128i *mm_a      = cast<__m128i *>(a);
    __m128i *mm_b      = cast<__m128i *>(b);
    __m128i  eq_mask_l = _mm_cmpeq_epi64(*(mm_a    ), *(mm_b    ));
    __m128i  eq_mask_h = _mm_cmpeq_epi64(*(mm_a + 1), *(mm_b + 1));
    __m128i  mask      = _mm_and_si128(eq_mask_l, eq_mask_h);
    __m128i  ones      = _mm_cmpeq_epi64(mask, mask);
    return _mm_testc_si128(mask, ones);
#else
    u64 *mm_a = cast(u64 *, a);
    u64 *mm_b = cast(u64 *, b);
    return *(mm_a + 0) == *(mm_b + 0)
        && *(mm_a + 1) == *(mm_b + 1)
        && *(mm_a + 2) == *(mm_b + 2)
        && *(mm_a + 3) == *(mm_b + 3);
#endif
}

template<typename T, typename = RTTI::enable_if_t<RTTI::is_arithmetic_v<T>>>
INLINE b32 MATH_CALL mm512_equals(T *a, T *b)
{
#if ENGINE_ISA >= ENGINE_ISA_AVX512
    return 0xFF == _mm512_cmpeq_epi64_mask(*cast<__m512i *>(a),
                                           *cast<__m512i *>(b));
#elif ENGINE_ISA >= ENGINE_ISA_AVX2
    __m256i *mm_a      = cast<__m256i *>(a);
    __m256i *mm_b      = cast<__m256i *>(b);
    __m256i  eq_mask_l = _mm256_cmpeq_epi64(*(mm_a    ), *(mm_b    ));
    __m256i  eq_mask_h = _mm256_cmpeq_epi64(*(mm_a + 1), *(mm_b + 1));
    __m256i  mask      = _mm256_and_si256(eq_mask_l, eq_mask_h);
    __m256i  ones      = _mm256_cmpeq_epi64(mask, mask);
    return _mm256_testc_si256(mask, ones);
#elif ENGINE_ISA >= ENGINE_ISA_AVX
    __m256d *mm_a      = cast<__m256d *>(a);
    __m256d *mm_b      = cast<__m256d *>(b);
    __m256d  eq_mask_l = _mm256_cmp_pd(*(mm_a    ), *(mm_b    ), _CMP_EQ_OQ);
    __m256d  eq_mask_h = _mm256_cmp_pd(*(mm_a + 1), *(mm_b + 1), _CMP_EQ_OQ);
    __m256d  mask      = _mm256_and_pd(eq_mask_l, eq_mask_h);
    __m256d  ones      = _mm256_cmp_pd(mask, mask, _CMP_EQ_OQ);
    return _mm256_testc_si256(_mm256_castpd_si256(mask),
                              _mm256_castpd_si256(ones));
#elif ENGINE_ISA >= ENGINE_ISA_SSE
    __m128i *mm_a      = cast<__m128i *>(a);
    __m128i *mm_b      = cast<__m128i *>(b);
    __m128i  eq_mask_0 = _mm_cmpeq_epi64(*(mm_a    ), *(mm_b    ));
    __m128i  eq_mask_1 = _mm_cmpeq_epi64(*(mm_a + 1), *(mm_b + 1));
    __m128i  eq_mask_2 = _mm_cmpeq_epi64(*(mm_a + 2), *(mm_b + 2));
    __m128i  eq_mask_3 = _mm_cmpeq_epi64(*(mm_a + 3), *(mm_b + 3));
    __m128i  mask_l    = _mm_and_si128(eq_mask_0, eq_mask_1);
    __m128i  mask_h    = _mm_and_si128(eq_mask_2, eq_mask_3);
    __m128i  mask      = _mm_and_si128(mask_l, mask_h);
    __m128i  ones      = _mm_cmpeq_epi64(mask, mask);
    return _mm_testc_si128(mask, ones);
#else
    u64 *mm_a = cast<u64 *>(a);
    u64 *mm_b = cast<u64 *>(b);
    return *(mm_a + 0) == *(mm_b + 0)
        && *(mm_a + 1) == *(mm_b + 1)
        && *(mm_a + 2) == *(mm_b + 2)
        && *(mm_a + 3) == *(mm_b + 3)
        && *(mm_a + 4) == *(mm_b + 4)
        && *(mm_a + 5) == *(mm_b + 5)
        && *(mm_a + 6) == *(mm_b + 6)
        && *(mm_a + 7) == *(mm_b + 7);
#endif
}

#if ENGINE_ISA >= ENGINE_ISA_SSE
    #undef _MM_EXTRACT_FLOAT
    template<u8 index>
    INLINE f32 MATH_CALL mm_extract_ps(__m128 mm)
    {
        return reg32(_mm_extract_ps(mm, index)).f;
    }
#endif

#if ENGINE_ISA >= ENGINE_ISA_AVX
    template<u8 index>
    INLINE f32 MATH_CALL mm256_extract_ps(__m256 mm256)
    {
        return reg32(_mm256_extract_epi32(ymm(mm256).i, index)).f;
    }
#endif

#if ENGINE_ISA >= ENGINE_ISA_SSE
    template<u8 index>
    INLINE __m128 MATH_CALL mm_insert_f32(__m128 mm128, f32 val)
    {
        return xmm(_mm_insert_epi32(xmm(mm128).i, reg32(val).i, index)).f;
    }
#endif

#if ENGINE_ISA >= ENGINE_ISA_AVX
    template<u8 index>
    INLINE __m256 MATH_CALL mm256_insert_f32(__m256 mm256, f32 val)
    {
        return ymm(_mm256_insert_epi32(ymm(mm256).i, reg32(val).i, index)).f;
    }
#endif

#pragma pack(pop)
