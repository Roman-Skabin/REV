//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "math/math.h"

#pragma pack(push, 1)

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

    __vectorcall operator __m128()  const { return f; }
    __vectorcall operator __m128i() const { return i; }
    __vectorcall operator __m128d() const { return d; }

    xmm& __vectorcall operator=(__m128     v) { f = v;   return *this; }
    xmm& __vectorcall operator=(__m128i    v) { i = v;   return *this; }
    xmm& __vectorcall operator=(__m128d    v) { d = v;   return *this; }
    xmm& __vectorcall operator=(const xmm& v) { f = v.f; return *this; }
    xmm& __vectorcall operator=(xmm&&      v) { f = v.f; return *this; }
};

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

    __vectorcall operator __m256()  const { return f; }
    __vectorcall operator __m256i() const { return i; }
    __vectorcall operator __m256d() const { return d; }

    ymm& __vectorcall operator=(__m256     v) { f = v;   return *this; }
    ymm& __vectorcall operator=(__m256i    v) { i = v;   return *this; }
    ymm& __vectorcall operator=(__m256d    v) { d = v;   return *this; }
    ymm& __vectorcall operator=(const ymm& v) { f = v.f; return *this; }
    ymm& __vectorcall operator=(ymm&&      v) { f = v.f; return *this; }
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

    __vectorcall operator __m512()  const { return f; }
    __vectorcall operator __m512i() const { return i; }
    __vectorcall operator __m512d() const { return d; }

    zmm& __vectorcall operator=(__m512     v) { f = v;   return *this; }
    zmm& __vectorcall operator=(__m512i    v) { i = v;   return *this; }
    zmm& __vectorcall operator=(__m512d    v) { d = v;   return *this; }
    zmm& __vectorcall operator=(const zmm& v) { f = v.f; return *this; }
    zmm& __vectorcall operator=(zmm&&      v) { f = v.f; return *this; }
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

template<typename T, typename U>
INLINE b32 __vectorcall mm_equals(T *a, U *b)
{
#if ENGINE_ISA >= ENGINE_ISA_AVX512
    return 0b11 == _mm_cmpeq_epi64_mask(*cast<__m128i *>(a),
                                        *cast<__m128i *>(b));
#else
    __m128i eq_mask = _mm_cmpeq_epi64(*cast<__m128i *>(a),
                                      *cast<__m128i *>(b));
    __m128i ones = _mm_cmpeq_epi64(eq_mask, eq_mask);
    return _mm_testc_si128(eq_mask, ones);
#endif
}

template<typename T, typename U>
INLINE b32 __vectorcall mm256_equals(T *a, U *b)
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
#else
    __m128i *mm_a      = cast<__m128i *>(a);
    __m128i *mm_b      = cast<__m128i *>(b);
    __m128i  eq_mask_l = _mm_cmpeq_epi64(*(mm_a    ), *(mm_b    ));
    __m128i  eq_mask_h = _mm_cmpeq_epi64(*(mm_a + 1), *(mm_b + 1));
    __m128i  mask      = _mm_and_si128(eq_mask_l, eq_mask_h);
    __m128i  ones      = _mm_cmpeq_epi64(mask, mask);
    return _mm_testc_si128(mask, ones);
#endif
}

template<typename T, typename U>
INLINE b32 __vectorcall mm512_equals(T *a, U *b)
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
#else
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
#endif
}

#undef _MM_EXTRACT_FLOAT
template<u8 index> INLINE f32 __vectorcall mm_extract_f32(__m128  mm) { return reg32(_mm_extract_ps(mm, index)).f; }
template<u8 index> INLINE s32 __vectorcall mm_extract_s32(__m128i mm) { return _mm_extract_epi32(mm, index); }
template<u8 index> INLINE u32 __vectorcall mm_extract_u32(__m128i mm) { return cast<u32>(_mm_extract_epi32(mm, index)); }

template<> INLINE f32 __vectorcall mm_extract_f32<0>(__m128  mm) { return _mm_cvtss_f32(mm); }
template<> INLINE s32 __vectorcall mm_extract_s32<0>(__m128i mm) { return _mm_cvtsi128_si32(mm); }
template<> INLINE u32 __vectorcall mm_extract_u32<0>(__m128i mm) { return cast<u32>(_mm_cvtsi128_si32(mm)); }

#if ENGINE_ISA >= ENGINE_ISA_AVX
    template<u8 index> INLINE f32 __vectorcall mm_extract_f32(__m256  mm) { return reg32(_mm256_extract_epi32(ymm(mm).i, index)).f; }
    template<u8 index> INLINE s32 __vectorcall mm_extract_s32(__m256i mm) { return _mm256_extract_epi32(mm, index); }
    template<u8 index> INLINE u32 __vectorcall mm_extract_u32(__m256i mm) { return cast<u32>(_mm256_extract_epi32(mm, index)); }

    template<> INLINE f32 __vectorcall mm_extract_f32<0>(__m256  mm) { return _mm_cvtss_f32(_mm256_castps256_ps128(mm)); }
    template<> INLINE s32 __vectorcall mm_extract_s32<0>(__m256i mm) { return _mm_cvtsi128_si32(_mm256_castsi256_si128(mm)); }
    template<> INLINE u32 __vectorcall mm_extract_u32<0>(__m256i mm) { return cast<u32>(_mm_cvtsi128_si32(_mm256_castsi256_si128(mm))); }
#endif

#if ENGINE_ISA >= ENGINE_ISA_AVX512
    template<u8 index> INLINE f32 __vectorcall mm_extract_f32(__m512  mm) { return reg32(_mm_extract_ps(_mm512_extractf32x4_ps(mm, index / 4), index % 4)).f; }
    template<u8 index> INLINE s32 __vectorcall mm_extract_s32(__m512i mm) { return _mm_extract_epi32(_mm512_extracti32x4_epi32(mm, index / 4), index % 4); }
    template<u8 index> INLINE u32 __vectorcall mm_extract_u32(__m512i mm) { return cast<u32>(_mm_extract_epi32(_mm512_extracti32x4_epi32(mm, index / 4), index % 4)); }

    template<> INLINE f32 __vectorcall mm_extract_f32<0>(__m512  mm) { return _mm512_cvtss_f32(mm); }
    template<> INLINE s32 __vectorcall mm_extract_s32<0>(__m512i mm) { return _mm512_cvtsi512_si32(mm); }
    template<> INLINE u32 __vectorcall mm_extract_u32<0>(__m512i mm) { return cast<u32>(_mm512_cvtsi512_si32(mm)); }
#endif

template<u8 index> INLINE __m128  __vectorcall mm_insert_f32(__m128  mm, f32 val) { return xmm(_mm_insert_epi32(xmm(mm).i, reg32(val).i, index)).f; }
template<u8 index> INLINE __m128i __vectorcall mm_insert_s32(__m128i mm, s32 val) { return _mm_insert_epi32(mm, val, index); }
template<u8 index> INLINE __m128i __vectorcall mm_insert_u32(__m128i mm, u32 val) { return _mm_insert_epi32(mm, val, index); }

#if ENGINE_ISA >= ENGINE_ISA_AVX
    template<u8 index> INLINE __m256  __vectorcall mm_insert_f32(__m256  mm, f32 val) { return ymm(_mm256_insert_epi32(ymm(mm).i, reg32(val).i, index)).f; }
    template<u8 index> INLINE __m256i __vectorcall mm_insert_s32(__m256i mm, s32 val) { return _mm256_insert_epi32(mm, val, index); }
    template<u8 index> INLINE __m256i __vectorcall mm_insert_u32(__m256i mm, u32 val) { return _mm256_insert_epi32(mm, val, index); }
#endif

#if ENGINE_ISA >= ENGINE_ISA_AVX512
    template<u8 index>
    INLINE __m512 __vectorcall mm_insert_f32(__m512 mm, f32 val)
    {
        __m128 paste = _mm512_extractf32x4_ps(mm, index / 4);
        paste = xmm(_mm_insert_epi32(xmm(paste).i, reg32(val).i, index % 4)).f;
        return _mm512_insertf32x4(mm, paste, index / 4);
    }

    template<u8 index>
    INLINE __m512i __vectorcall mm_insert_s32(__m512i mm, s32 val)
    {
        __m128i paste = _mm512_extracti32x4_epi32(mm, index / 4);
        paste = _mm_insert_epi32(paste, val, index % 4);
        return _mm512_inserti32x4(mm, paste, index / 4);
    }

    template<u8 index>
    INLINE __m512i __vectorcall mm_insert_u32(__m512i mm, u32 val)
    {
        __m128i paste = _mm512_extracti32x4_epi32(mm, index / 4);
        paste = _mm_insert_epi32(paste, val, index % 4);
        return _mm512_inserti32x4(mm, paste, index / 4);
    }
#endif

#pragma pack(pop)
