//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "core/memory.h"

namespace REV
{

//
// memset_f32
//

#if REV_ISA >= REV_ISA_AVX512
REV_INTERNAL REV_INLINE void memset_f32_avx512(f32 *mem, f32 val, u64 count)
{
    __m512 *mm512_mem = cast<__m512 *>(mem);
    __m512  mm512_val = _mm512_set1_ps(val);

    u64 index = 0;
    while (index + 16 <= count)
    {
        *mm512_mem++ = mm512_val;
        index += 16;
    }

    __m256 *mm256_mem = cast<__m256 *>(mm512_mem);

    if (index + 8 <= count)
    {
        *mm256_mem++ = _mm256_set1_ps(val);
        index += 8;
    }

    __m128 *mm128_mem = cast<__m128 *>(mm256_mem);

    if (index + 4 <= count)
    {
        *mm128_mem++ = _mm_set_ps1(val);
        index += 4;
    }

    mem = cast<f32 *>(mm128_mem);

    while (index < count)
    {
        *mem++ = val;
        ++index;
    }
}
#elif REV_ISA >= REV_ISA_AVX
REV_INTERNAL REV_INLINE void memset_f32_avx(f32 *mem, f32 val, u64 count)
{
    __m256 *mm256_mem = cast<__m256 *>(mem);
    __m256  mm256_val = _mm256_set1_ps(val);

    u64 index = 0;
    while (index + 8 <= count)
    {
        *mm256_mem++ = mm256_val;
        index += 8;
    }

    __m128 *mm128_mem = cast<__m128 *>(mm256_mem);

    if (index + 4 <= count)
    {
        *mm128_mem++ = _mm_set_ps1(val);
        index += 4;
    }

    mem = cast<f32 *>(mm128_mem);

    while (index < count)
    {
        *mem++ = val;
        ++index;
    }
}
#else
REV_INTERNAL REV_INLINE void memset_f32_sse(f32 *mem, f32 val, u64 count)
{
    __m128 *mm128_mem = cast<__m128 *>(mem);
    __m128  mm128_val = _mm_set_ps1(val);

    u64 index = 0;
    while (index + 4 <= count)
    {
        *mm128_mem++ = mm128_val;
        index += 4;
    }

    mem = cast<f32 *>(mm128_mem);

    while (index < count)
    {
        *mem++ = val;
        ++index;
    }
}
#endif

void memset_f32(f32 *mem, f32 val, u64 count)
{
#if REV_ISA >= REV_ISA_AVX512
    memset_f32_avx512(mem, val, count);
#elif REV_ISA >= REV_ISA_AVX
    memset_f32_avx(mem, val, count);
#else
    memset_f32_sse(mem, val, count);
#endif
}

//
// memset_f64
//

#if REV_ISA >= REV_ISA_AVX512
REV_INTERNAL REV_INLINE void memset_f64_avx512(f64 *mem, f64 val, u64 count)
{
    __m512d *mm512_mem = cast<__m512d *>(mem);
    __m512d  mm512_val = _mm512_set1_pd(val);

    u64 index = 0;
    while (index + 8 <= count)
    {
        *mm512_mem++ = mm512_val;
        index += 8;
    }

    __m256d *mm256_mem = cast<__m256d *>(mm512_mem);

    if (index + 4 <= count)
    {
        *mm256_mem++ = _mm256_set1_pd(val);
        index += 4;
    }

    __m128d *mm128_mem = cast<__m128d *>(mm256_mem);

    if (index + 2 <= count)
    {
        *mm128_mem++ = _mm_set1_pd(val);
        index += 2;
    }

    mem = cast<f64 *>(mm128_mem);

    if (index < count)
    {
        *mem++ = val;
        ++index;
    }
}
#elif REV_ISA >= REV_ISA_AVX
REV_INTERNAL REV_INLINE void memset_f64_avx(f64 *mem, f64 val, u64 count)
{
    __m256d *mm256_mem = cast<__m256d *>(mem);
    __m256d  mm256_val = _mm256_set1_pd(val);

    u64 index = 0;
    while (index + 4 <= count)
    {
        *mm256_mem++ = mm256_val;
        index += 4;
    }

    __m128d *mm128_mem = cast<__m128d *>(mm256_mem);

    if (index + 2 <= count)
    {
        *mm128_mem++ = _mm_set1_pd(val);
        index += 2;
    }

    mem = cast<f64 *>(mm128_mem);

    if (index < count)
    {
        *mem++ = val;
        ++index;
    }
}
#else
REV_INTERNAL REV_INLINE void memset_f64_sse(f64 *mem, f64 val, u64 count)
{
    __m128d *mm128_mem = cast<__m128d *>(mem);
    __m128d  mm128_val = _mm_set1_pd(val);

    u64 index = 0;
    while (index + 2 <= count)
    {
        *mm128_mem++ = mm128_val;
        index += 2;
    }

    mem = cast<f64 *>(mm128_mem);

    if (index < count)
    {
        *mem++ = val;
        ++index;
    }
}
#endif

void memset_f64(f64 *mem, f64 val, u64 count)
{
#if REV_ISA >= REV_ISA_AVX512
    memset_f64_avx512(mem, val, count);
#elif REV_ISA >= REV_ISA_AVX
    memset_f64_avx(mem, val, count);
#else
    memset_f64_sse(mem, val, count)
#endif
}

//
// memset_char
//

#if REV_ISA >= REV_ISA_AVX512
REV_INTERNAL REV_INLINE void memset_char_avx512(char *mem, char val, u64 count)
{
    __m512i *mm512_mem = cast<__m512i *>(mem);
    __m512i  mm512_val = _mm512_set1_epi8(val);

    u64 index = 0;
    while (index + 64 <= count)
    {
        *mm512_mem++ = mm512_val;
        index += 64;
    }

    __m256i *mm256_mem = cast<__m256i *>(mm512_mem);

    if (index + 32 <= count)
    {
        *mm256_mem++ = _mm256_set1_epi8(val);
        index += 32;
    }

    __m128i *mm128_mem = cast<__m128i *>(mm256_mem);

    if (index + 16 <= count)
    {
        *mm128_mem++ = _mm_set1_epi8(val);
        index += 16;
    }

    mem = cast<char *>(mm128_mem);

    while (index < count)
    {
        *mem++ = val;
        ++index;
    }
}
#elif REV_ISA >= REV_ISA_AVX
REV_INTERNAL REV_INLINE void memset_char_avx(char *mem, char val, u64 count)
{
    __m256i *mm256_mem = cast<__m256i *>(mem);
    __m256i  mm256_val = _mm256_set1_epi8(val);

    u64 index = 0;
    while (index + 32 <= count)
    {
        *mm256_mem++ = mm256_val;
        index += 32;
    }

    __m128i *mm128_mem = cast<__m128i *>(mm256_mem);

    if (index + 16 <= count)
    {
        *mm128_mem++ = _mm_set1_epi8(val);
        index += 16;
    }

    mem = cast<char *>(mm128_mem);

    while (index < count)
    {
        *mem++ = val;
        ++index;
    }
}
#else
REV_INTERNAL REV_INLINE void memset_char_sse(char *mem, char val, u64 count)
{
    __m128i *mm128_mem = cast<__m128i *>(mem);
    __m128i  mm128_val = _mm_set1_epi8(val);

    u64 index = 0;
    while (index + 16 <= count)
    {
        *mm128_mem++ = mm128_val;
        index += 16;
    }

    mem = cast<char *>(mm128_mem);

    while (index < count)
    {
        *mem++ = val;
        ++index;
    }
}
#endif

void memset_char(char *mem, char val, u64 count)
{
#if REV_ISA >= REV_ISA_AVX512
    memset_char_avx512(mem, val, count);
#elif REV_ISA >= REV_ISA_AVX
    memset_char_avx(mem, val, count);
#else
    memset_char_sse(mem, val, count)
#endif
}

//
// memset_s8
//

#if REV_ISA >= REV_ISA_AVX512
REV_INTERNAL REV_INLINE void memset_s8_avx512(s8 *mem, s8 val, u64 count)
{
    __m512i *mm512_mem = cast<__m512i *>(mem);
    __m512i  mm512_val = _mm512_set1_epi8(val);

    u64 index = 0;
    while (index + 64 <= count)
    {
        *mm512_mem++ = mm512_val;
        index += 64;
    }

    __m256i *mm256_mem = cast<__m256i *>(mm512_mem);

    if (index + 32 <= count)
    {
        *mm256_mem++ = _mm256_set1_epi8(val);
        index += 32;
    }

    __m128i *mm128_mem = cast<__m128i *>(mm256_mem);

    if (index + 16 <= count)
    {
        *mm128_mem++ = _mm_set1_epi8(val);
        index += 16;
    }

    mem = cast<s8 *>(mm128_mem);

    while (index < count)
    {
        *mem++ = val;
        ++index;
    }
}
#elif REV_ISA >= REV_ISA_AVX
REV_INTERNAL REV_INLINE void memset_s8_avx(s8 *mem, s8 val, u64 count)
{
    __m256i *mm256_mem = cast<__m256i *>(mem);
    __m256i  mm256_val = _mm256_set1_epi8(val);

    u64 index = 0;
    while (index + 32 <= count)
    {
        *mm256_mem++ = mm256_val;
        index += 32;
    }

    __m128i *mm128_mem = cast<__m128i *>(mm256_mem);

    if (index + 16 <= count)
    {
        *mm128_mem++ = _mm_set1_epi8(val);
        index += 16;
    }

    mem = cast<s8 *>(mm128_mem);

    while (index < count)
    {
        *mem++ = val;
        ++index;
    }
}
#else
REV_INTERNAL REV_INLINE void memset_s8_sse(s8 *mem, s8 val, u64 count)
{
    __m128i *mm128_mem = cast<__m128i *>(mem);
    __m128i  mm128_val = _mm_set1_epi8(val);

    u64 index = 0;
    while (index + 16 <= count)
    {
        *mm128_mem++ = mm128_val;
        index += 16;
    }

    mem = cast<s8 *>(mm128_mem);

    while (index < count)
    {
        *mem++ = val;
        ++index;
    }
}
#endif

void memset_s8(s8 *mem, s8 val, u64 count)
{
#if REV_ISA >= REV_ISA_AVX512
    memset_s8_avx512(mem, val, count);
#elif REV_ISA >= REV_ISA_AVX
    memset_s8_avx(mem, val, count);
#else
    memset_s8_sse(mem, val, count)
#endif
}

//
// memset_s16
//

#if REV_ISA >= REV_ISA_AVX512
REV_INTERNAL REV_INLINE void memset_s16_avx512(s16 *mem, s16 val, u64 count)
{
    __m512i *mm512_mem = cast<__m512i *>(mem);
    __m512i  mm512_val = _mm512_set1_epi16(val);

    u64 index = 0;
    while (index + 32 <= count)
    {
        *mm512_mem++ = mm512_val;
        index += 32;
    }

    __m256i *mm256_mem = cast<__m256i *>(mm512_mem);

    if (index + 16 <= count)
    {
        *mm256_mem++ = _mm256_set1_epi16(val);
        index += 16;
    }

    __m128i *mm128_mem = cast<__m128i *>(mm256_mem);

    if (index + 8 <= count)
    {
        *mm128_mem++ = _mm_set1_epi16(val);
        index += 8;
    }

    mem = cast<s16 *>(mm128_mem);

    while (index < count)
    {
        *mem++ = val;
        ++index;
    }
}
#elif REV_ISA >= REV_ISA_AVX
REV_INTERNAL REV_INLINE void memset_s16_avx(s16 *mem, s16 val, u64 count)
{
    __m256i *mm256_mem = cast<__m256i *>(mem);
    __m256i  mm256_val = _mm256_set1_epi16(val);

    u64 index = 0;
    while (index + 16 <= count)
    {
        *mm256_mem++ = mm256_val;
        index += 16;
    }

    __m128i *mm128_mem = cast<__m128i *>(mm256_mem);

    if (index + 8 <= count)
    {
        *mm128_mem++ = _mm_set1_epi16(val);
        index += 8;
    }

    mem = cast<s16 *>(mm128_mem);

    while (index < count)
    {
        *mem++ = val;
        ++index;
    }
}
#else
REV_INTERNAL REV_INLINE void memset_s16_sse(s16 *mem, s16 val, u64 count)
{
    __m128i *mm128_mem = cast<__m128i *>(mem);
    __m128i  mm128_val = _mm_set1_epi16(val);

    u64 index = 0;
    while (index + 8 <= count)
    {
        *mm128_mem++ = mm128_val;
        index += 8;
    }

    mem = cast<s16 *>(mm128_mem);

    while (index < count)
    {
        *mem++ = val;
        ++index;
    }
}
#endif

void memset_s16(s16 *mem, s16 val, u64 count)
{
#if REV_ISA >= REV_ISA_AVX512
    memset_s16_avx512(mem, val, count);
#elif REV_ISA >= REV_ISA_AVX
    memset_s16_avx(mem, val, count);
#else
    memset_s16_sse(mem, val, count)
#endif
}

//
// memset_s32
//

#if REV_ISA >= REV_ISA_AVX512
REV_INTERNAL REV_INLINE void memset_s32_avx512(s32 *mem, s32 val, u64 count)
{
    __m512i *mm512_mem = cast<__m512i *>(mem);
    __m512i  mm512_val = _mm512_set1_epi32(val);

    u64 index = 0;
    while (index + 16 <= count)
    {
        *mm512_mem++ = mm512_val;
        index += 16;
    }

    __m256i *mm256_mem = cast<__m256i *>(mm512_mem);

    if (index + 8 <= count)
    {
        *mm256_mem++ = _mm256_set1_epi32(val);
        index += 8;
    }

    __m128i *mm128_mem = cast<__m128i *>(mm256_mem);

    if (index + 4 <= count)
    {
        *mm128_mem++ = _mm_set1_epi32(val);
        index += 4;
    }

    mem = cast<s32 *>(mm128_mem);

    while (index < count)
    {
        *mem++ = val;
        ++index;
    }
}
#elif REV_ISA >= REV_ISA_AVX
REV_INTERNAL REV_INLINE void memset_s32_avx(s32 *mem, s32 val, u64 count)
{
    __m256i *mm256_mem = cast<__m256i *>(mem);
    __m256i  mm256_val = _mm256_set1_epi32(val);

    u64 index = 0;
    while (index + 8 <= count)
    {
        *mm256_mem++ = mm256_val;
        index += 8;
    }

    __m128i *mm128_mem = cast<__m128i *>(mm256_mem);

    if (index + 4 <= count)
    {
        *mm128_mem++ = _mm_set1_epi32(val);
        index += 4;
    }

    mem = cast<s32 *>(mm128_mem);

    while (index < count)
    {
        *mem++ = val;
        ++index;
    }
}
#else
REV_INTERNAL REV_INLINE void memset_s32_sse(s32 *mem, s32 val, u64 count)
{
    __m128i *mm128_mem = cast<__m128i *>(mem);
    __m128i  mm128_val = _mm_set1_epi32(val);

    u64 index = 0;
    while (index + 4 <= count)
    {
        *mm128_mem++ = mm128_val;
        index += 4;
    }

    mem = cast<s32 *>(mm128_mem);

    while (index < count)
    {
        *mem++ = val;
        ++index;
    }
}
#endif

void memset_s32(s32 *mem, s32 val, u64 count)
{
#if REV_ISA >= REV_ISA_AVX512
    memset_s32_avx512(mem, val, count);
#elif REV_ISA >= REV_ISA_AVX
    memset_s32_avx(mem, val, count);
#else
    memset_s32_sse(mem, val, count)
#endif
}

//
// memset_s64
//

#if REV_ISA >= REV_ISA_AVX512
REV_INTERNAL REV_INLINE void memset_s64_avx512(s64 *mem, s64 val, u64 count)
{
    __m512i *mm512_mem = cast<__m512i *>(mem);
    __m512i  mm512_val = _mm512_set1_epi64(val);

    u64 index = 0;
    while (index + 8 <= count)
    {
        *mm512_mem++ = mm512_val;
        index += 8;
    }

    __m256i *mm256_mem = cast<__m256i *>(mm512_mem);

    if (index + 4 <= count)
    {
        *mm256_mem++ = _mm256_set1_epi64x(val);
        index += 4;
    }

    __m128i *mm128_mem = cast<__m128i *>(mm256_mem);

    if (index + 2 <= count)
    {
        *mm128_mem++ = _mm_set1_epi64x(val);
        index += 2;
    }

    mem = cast<s64 *>(mm128_mem);

    if (index < count)
    {
        *mem++ = val;
        ++index;
    }
}
#elif REV_ISA >= REV_ISA_AVX
REV_INTERNAL REV_INLINE void memset_s64_avx(s64 *mem, s64 val, u64 count)
{
    __m256i *mm256_mem = cast<__m256i *>(mem);
    __m256i  mm256_val = _mm256_set1_epi64x(val);

    u64 index = 0;
    while (index + 4 <= count)
    {
        *mm256_mem++ = mm256_val;
        index += 4;
    }

    __m128i *mm128_mem = cast<__m128i *>(mm256_mem);

    if (index + 2 <= count)
    {
        *mm128_mem++ = _mm_set1_epi64x(val);
        index += 2;
    }

    mem = cast<s64 *>(mm128_mem);

    if (index < count)
    {
        *mem++ = val;
        ++index;
    }
}
#else
REV_INTERNAL REV_INLINE void memset_s64_sse(s64 *mem, s64 val, u64 count)
{
    __m128i *mm128_mem = cast<__m128i *>(mem);
    __m128i  mm128_val = _mm_set1_epi64x(val);

    u64 index = 0;
    while (index + 2 <= count)
    {
        *mm128_mem++ = mm128_val;
        index += 2;
    }

    mem = cast<s64 *>(mm128_mem);

    if (index < count)
    {
        *mem++ = val;
        ++index;
    }
}
#endif

void memset_s64(s64 *mem, s64 val, u64 count)
{
#if REV_ISA >= REV_ISA_AVX512
    memset_s64_avx512(mem, val, count);
#elif REV_ISA >= REV_ISA_AVX
    memset_s64_avx(mem, val, count);
#else
    memset_s64_sse(mem, val, count)
#endif
}

}
