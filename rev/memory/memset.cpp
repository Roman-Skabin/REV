//
// Copyright 2020-2021 Roman Skabin
//

#include "core/pch.h"

/*
#include "memory/memlow.h"

namespace REV
{

void REV_VECTORCALL FillMemoryF32(f32 *mem, f32 val, u64 count)
{
#if REV_ISA >= REV_ISA_AVX512
    __m512 *mm512_mem = cast(__m512 *, mem);
    __m512  mm512_val = _mm512_set1_ps(val);

    u64 index = 0;
    while (index + 16 <= count)
    {
        _mm512_storeu_ps(mm512_mem++, mm512_val);
        index += 16;
    }

    __m256 *mm256_mem = cast(__m256 *, mm512_mem);

    if (index + 8 <= count)
    {
        _mm256_storeu_ps(cast(float *, mm256_mem), _mm512_castps512_ps256(mm512_val));
        ++mm256_mem;
        index += 8;
    }

    __m128 *mm128_mem = cast(__m128 *, mm256_mem);

    if (index + 4 <= count)
    {
        _mm_storeu_ps(cast(float *, mm128_mem), _mm512_castps512_ps128(mm512_val));
        ++mm128_mem;
        index += 4;
    }

    if (index < count)
    {
        __stosd(cast(u32 *, mm128_mem), *cast(u32 *, &val), count - index);
    }
#elif REV_ISA >= REV_ISA_AVX
    __m256 *mm256_mem = cast(__m256 *, mem);
    __m256  mm256_val = _mm256_set1_ps(val);

    u64 index = 0;
    while (index + 8 <= count)
    {
        _mm256_storeu_ps(cast(float *, mm256_mem), mm256_val);
        ++mm256_mem;
        index += 8;
    }

    __m128 *mm128_mem = cast(__m128 *, mm256_mem);

    if (index + 4 <= count)
    {
        _mm_storeu_ps(cast(float *, mm128_mem), _mm256_castps256_ps128(mm256_val));
        ++mm128_mem;
        index += 4;
    }

    if (index < count)
    {
        __stosd(cast(u32 *, mm128_mem), *cast(u32 *, &val), count - index);
    }
#else
    __m128 *mm128_mem = cast(__m128 *, mem);
    __m128  mm128_val = _mm_set_ps1(val);

    u64 index = 0;
    while (index + 4 <= count)
    {
        _mm_storeu_ps(cast(float *, mm128_mem), mm128_val);
        ++mm128_mem;
        index += 4;
    }

    if (index < count)
    {
        __stosd(cast(u32 *, mm128_mem), *cast(u32 *, &val), count - index);
    }
#endif
}

void REV_VECTORCALL FillMemoryF64(f64 *mem, f64 val, u64 count)
{
#if REV_ISA >= REV_ISA_AVX512
    __m512d *mm512_mem = cast(__m512d *, mem);
    __m512d  mm512_val = _mm512_set1_pd(val);

    u64 index = 0;
    while (index + 8 <= count)
    {
        _mm512_storeu_pd(mm512_mem++, mm512_val);
        index += 8;
    }

    __m256d *mm256_mem = cast(__m256d *, mm512_mem);

    if (index + 4 <= count)
    {
        _mm256_storeu_pd(cast(double *, mm256_mem), _mm512_castpd512_pd256(mm512_val));
        ++mm256_mem;
        index += 4;
    }

    __m128d *mm128_mem = cast(__m128d *, mm256_mem);

    if (index + 2 <= count)
    {
        _mm_storeu_pd(cast(double *, mm128_mem), _mm512_castpd512_pd128(mm512_val));
        ++mm128_mem;
        index += 2;
    }

    if (index < count)
    {
        __stosq(cast(u64 *, mm128_mem), *cast(u64 *, &val), count - index);
    }
#elif REV_ISA >= REV_ISA_AVX
    __m256d *mm256_mem = cast(__m256d *, mem);
    __m256d  mm256_val = _mm256_set1_pd(val);

    u64 index = 0;
    while (index + 4 <= count)
    {
        _mm256_storeu_pd(cast(double *, mm256_mem), mm256_val);
        ++mm256_mem;
        index += 4;
    }

    __m128d *mm128_mem = cast(__m128d *, mm256_mem);

    if (index + 2 <= count)
    {
        _mm_storeu_pd(cast(double *, mm128_mem), _mm256_castpd256_pd128(mm256_val));
        ++mm128_mem;
        index += 2;
    }

    if (index < count)
    {
        __stosq(cast(u64 *, mm128_mem), *cast(u64 *, &val), count - index);
    }
#else
    __m128d *mm128_mem = cast(__m128d *, mem);
    __m128d  mm128_val = _mm_set1_pd(val);

    u64 index = 0;
    while (index + 2 <= count)
    {
        _mm_storeu_pd(cast(double *, mm128_mem), mm128_val);
        ++mm128_mem;
        index += 2;
    }

    if (index < count)
    {
        __stosq(cast(u64 *, mm128_mem), *cast(u64 *, &val), count - index);
    }
#endif
}

void REV_VECTORCALL FillMemoryU16(u16 *mem, u16 val, u64 count)
{
#if REV_ISA >= REV_ISA_AVX512
    __m512i *mm512_mem = cast(__m512i *, mem);
    __m512i  mm512_val = _mm512_set1_epi16(val);

    u64 index = 0;
    while (index + 32 <= count)
    {
        _mm512_storeu_si512(mm512_mem++, mm512_val);
        index += 32;
    }

    __m256i *mm256_mem = cast(__m256i *, mm512_mem);

    if (index + 16 <= count)
    {
        _mm256_storeu_si256(mm256_mem++, _mm512_castsi512_si256(mm512_val));
        index += 16;
    }

    __m128i *mm128_mem = cast(__m128i *, mm256_mem);

    if (index + 8 <= count)
    {
        _mm_storeu_si128(mm128_mem++, _mm512_castsi512_si128(mm512_val));
        index += 8;
    }

    if (index < count)
    {
        __stosw(cast(u16 *, mm128_mem), val, count - index);
    }
#elif REV_ISA >= REV_ISA_AVX
    __m256i *mm256_mem = cast(__m256i *, mem);
    __m256i  mm256_val = _mm256_set1_epi16(val);

    u64 index = 0;
    while (index + 16 <= count)
    {
        _mm256_storeu_si256(mm256_mem++, mm256_val);
        index += 16;
    }

    __m128i *mm128_mem = cast(__m128i *, mm256_mem);

    if (index + 8 <= count)
    {
        _mm_storeu_si128(mm128_mem++, _mm256_castsi256_si128(mm256_val));
        index += 8;
    }

    if (index < count)
    {
        __stosw(cast(u16 *, mm128_mem), val, count - index);
    }
#else
    __m128i *mm128_mem = cast(__m128i *, mem);
    __m128i  mm128_val = _mm_set1_epi16(val);

    u64 index = 0;
    while (index + 8 <= count)
    {
        _mm_storeu_si128(mm128_mem++, mm128_val);
        index += 8;
    }

    if (index < count)
    {
        __stosw(cast(u16 *, mm128_mem), val, count - index);
    }
#endif
}

void REV_VECTORCALL FillMemoryU32(u32 *mem, u32 val, u64 count)
{
#if REV_ISA >= REV_ISA_AVX512
    __m512i *mm512_mem = cast(__m512i *, mem);
    __m512i  mm512_val = _mm512_set1_epi32(val);

    u64 index = 0;
    while (index + 16 <= count)
    {
        _mm512_storeu_si512(mm512_mem++, mm512_val);
        index += 16;
    }

    __m256i *mm256_mem = cast(__m256i *, mm512_mem);

    if (index + 8 <= count)
    {
        _mm256_storeu_si256(mm256_mem++, _mm512_castsi512_si256(mm512_val));
        index += 8;
    }

    __m128i *mm128_mem = cast(__m128i *, mm256_mem);

    if (index + 4 <= count)
    {
        _mm_storeu_si128(mm128_mem++, _mm512_castsi512_si128(mm512_val));
        index += 4;
    }

    if (index < count)
    {
        __stosd(cast(u32 *, mm128_mem), val, count - index);
    }
#elif REV_ISA >= REV_ISA_AVX
    __m256i *mm256_mem = cast(__m256i *, mem);
    __m256i  mm256_val = _mm256_set1_epi32(val);

    u64 index = 0;
    while (index + 8 <= count)
    {
        _mm256_storeu_si256(mm256_mem++, mm256_val);
        index += 8;
    }

    __m128i *mm128_mem = cast(__m128i *, mm256_mem);

    if (index + 4 <= count)
    {
        _mm_storeu_si128(mm128_mem++, _mm256_castsi256_si128(mm256_val));
        index += 4;
    }

    if (index < count)
    {
        __stosd(cast(u32 *, mm128_mem), val, count - index);
    }
#else
    __m128i *mm128_mem = cast(__m128i *, mem);
    __m128i  mm128_val = _mm_set1_epi32(val);

    u64 index = 0;
    while (index + 4 <= count)
    {
        _mm_storeu_si128(mm128_mem++, mm128_val);
        index += 4;
    }

    if (index < count)
    {
        __stosd(cast(u32 *, mm128_mem), val, count - index);
    }
#endif
}

void REV_VECTORCALL FillMemoryU64(u64 *mem, u64 val, u64 count)
{
#if REV_ISA >= REV_ISA_AVX512
    __m512i *mm512_mem = cast(__m512i *, mem);
    __m512i  mm512_val = _mm512_set1_epi64(val);

    u64 index = 0;
    while (index + 8 <= count)
    {
        _mm512_storeu_si512(mm512_mem++, mm512_val);
        index += 8;
    }

    __m256i *mm256_mem = cast(__m256i *, mm512_mem);

    if (index + 4 <= count)
    {
        _mm256_storeu_si256(mm256_mem++, _mm512_castsi512_si256(mm512_val));
        index += 4;
    }

    __m128i *mm128_mem = cast(__m128i *, mm256_mem);

    if (index + 2 <= count)
    {
        _mm_storeu_si128(mm128_mem++, _mm512_castsi512_si128(mm512_val));
        index += 2;
    }

    if (index < count)
    {
        __stosq(cast(u64 *, mm128_mem), val, count - index);
    }
#elif REV_ISA >= REV_ISA_AVX
    __m256i *mm256_mem = cast(__m256i *, mem);
    __m256i  mm256_val = _mm256_set1_epi64x(val);

    u64 index = 0;
    while (index + 4 <= count)
    {
        _mm256_storeu_si256(mm256_mem++, mm256_val);
        index += 4;
    }

    __m128i *mm128_mem = cast(__m128i *, mm256_mem);

    if (index + 2 <= count)
    {
        _mm_storeu_si128(mm128_mem++, _mm256_castsi256_si128(mm256_val));
        index += 2;
    }

    if (index < count)
    {
        __stosq(cast(u64 *, mm128_mem), val, count - index);
    }
#else
    __m128i *mm128_mem = cast(__m128i *, mem);
    __m128i  mm128_val = _mm_set1_epi64x(val);

    u64 index = 0;
    while (index + 2 <= count)
    {
        _mm_storeu_si128(mm128_mem++, mm128_val);
        index += 2;
    }

    if (index < count)
    {
        __stosq(cast(u64 *, mm128_mem), val, count - index);
    }
#endif
}

}

*/
