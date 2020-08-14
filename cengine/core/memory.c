//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "core/memory.h"

//
// memset_f32
//

#if CENGINE_ISA >= CENGINE_ISA_AVX512
internal INLINE memset_f32_avx512(f32 *mem, f32 val, u64 count)
{
    __m512 *mm512_mem = cast(__m512 *, mem);
    __m512  mm512_val = _mm512_set1_ps(val);

    u64 index = 0;
    while (index + 16 <= count)
    {
        *mm512_mem++ = mm512_val;
        index += 16;
    }

    mem = cast(f32 *, mm512_mem);

    if (index + 8 <= count)
    {
        *cast(__m256 *, mem)++ = _mm256_set1_ps(val);
        index += 8;
    }

    if (index + 4 <= count)
    {
        *cast(__m128 *, mem)++ = _mm_set_ps1(val);
        index += 4;
    }

    while (index <= count)
    {
        *mem++ = val;
        ++index;
    }
}
#elif CENGINE_ISA >= CENGINE_ISA_AVX
internal INLINE memset_f32_avx(f32 *mem, f32 val, u64 count)
{
    __m256 *mm256_mem = cast(__m256 *, mem);
    __m256  mm256_val = _mm256_set1_ps(val);

    u64 index = 0;
    while (index + 8 <= count)
    {
        *mm256_mem++ = mm256_val;
        index += 8;
    }

    mem = cast(f32 *, mm256_mem);

    if (index + 4 <= count)
    {
        *cast(__m128 *, mem)++ = _mm_set_ps1(val);
        index += 4;
    }

    while (index <= count)
    {
        *mem++ = val;
        ++index;
    }
}
#elif CENGINE_ISA >= CENGINE_ISA_SSE
internal INLINE memset_f32_sse(f32 *mem, f32 val, u64 count)
{
    __m128 *mm128_mem = cast(__m128 *, mem);
    __m128  mm128_val = _mm_set_ps1(val);

    u64 index = 0;
    while (index + 4 <= count)
    {
        *mm128_mem++ = mm128_val;
        index += 4;
    }

    mem = cast(f32 *, mm128_mem);

    while (index <= count)
    {
        *mem++ = val;
        ++index;
    }
}
#endif

void memset_f32(f32 *mem, f32 val, u64 count)
{
#if CENGINE_ISA >= CENGINE_ISA_AVX512
    memset_f32_avx512(mem, val, count);
#elif CENGINE_ISA >= CENGINE_ISA_AVX
    memset_f32_avx(mem, val, count);
#elif CENGINE_ISA >= CENGINE_ISA_SSE
    memset_f32_sse(mem, val, count)
#else
    while (count)
    {
        *mem++ = val;
        --count;
    }
#endif
}

//
// memset_f64
//

#if CENGINE_ISA >= CENGINE_ISA_AVX512
internal INLINE memset_f64_avx512(f64 *mem, f64 val, u64 count)
{
    __m512d *mm512_mem = cast(__m512d *, mem);
    __m512d  mm512_val = _mm512_set1_pd(val);

    u64 index = 0;
    while (index + 8 <= count)
    {
        *mm512_mem++ = mm512_val;
        index += 8;
    }

    mem = cast(f64 *, mm512_mem);

    if (index + 4 <= count)
    {
        *cast(__m256d *, mem)++ = _mm256_set1_pd(val);
        index += 4;
    }

    if (index + 2 <= count)
    {
        *cast(__m128d *, mem)++ = _mm_set1_pd(val);
        index += 2;
    }

    if (index <= count)
    {
        *mem++ = val;
        ++index;
    }
}
#elif CENGINE_ISA >= CENGINE_ISA_AVX
internal INLINE memset_f64_avx(f64 *mem, f64 val, u64 count)
{
    __m256d *mm256_mem = cast(__m256d *, mem);
    __m256d  mm256_val = _mm256_set1_pd(val);

    u64 index = 0;
    while (index + 4 <= count)
    {
        *mm256_mem++ = mm256_val;
        index += 4;
    }

    mem = cast(f64 *, mm256_mem);

    if (index + 2 <= count)
    {
        *cast(__m128d *, mem)++ = _mm_set1_pd(val);
        index += 2;
    }

    if (index <= count)
    {
        *mem++ = val;
        ++index;
    }
}
#elif CENGINE_ISA >= CENGINE_ISA_SSE
internal INLINE memset_f64_sse(f64 *mem, f64 val, u64 count)
{
    __m128d *mm128_mem = cast(__m128d *, mem);
    __m128d  mm128_val = _mm_set1_pd(val);

    u64 index = 0;
    while (index + 2 <= count)
    {
        *mm128_mem++ = mm128_val;
        index += 2;
    }

    mem = cast(f64 *, mm128_mem);

    if (index <= count)
    {
        *mem++ = val;
        ++index;
    }
}
#endif

void memset_f64(f64 *mem, f64 val, u64 count)
{
#if CENGINE_ISA >= CENGINE_ISA_AVX512
    memset_f64_avx512(mem, val, count);
#elif CENGINE_ISA >= CENGINE_ISA_AVX
    memset_f64_avx(mem, val, count);
#elif CENGINE_ISA >= CENGINE_ISA_SSE
    memset_f64_sse(mem, val, count)
#else
    while (count)
    {
        *mem++ = val;
        --count;
    }
#endif
}

//
// Memory
//

void CreateMemory(
    in  u64     transient_area_cap,
    in  u64     permanent_area_cap,
    out Memory *memory)
{
    Check(memory);
    CheckM(transient_area_cap, "Transient area capacity must be greater than 0");
    CheckM(permanent_area_cap, "Permanent area capacity must be greater than 0");

    transient_area_cap = ALIGN_UP(transient_area_cap, PAGE_SIZE);
    permanent_area_cap = ALIGN_UP(permanent_area_cap, PAGE_SIZE);
    Check(   transient_area_cap <= MAX_MEMORY
          && permanent_area_cap <= MAX_MEMORY
          && transient_area_cap <= MAX_MEMORY - permanent_area_cap
          && permanent_area_cap <= MAX_MEMORY - transient_area_cap);

    u64 memory_cap = transient_area_cap + permanent_area_cap;

    memory->transient.cap = transient_area_cap;
    memory->permanent.cap = permanent_area_cap;

    memory->transient.base = cast(byte *, VirtualAlloc(null, memory_cap, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
    CheckM(memory->transient.base, "Physical memory overflow");

    memory->permanent.base = memory->transient.base + memory->transient.cap;

    memory->transient.size = 0;
    memory->permanent.size = 0;
}

void DestroyMemory(
    in Memory *memory)
{
    Check(memory);

    if (memory->transient.base)
    {
        DebugResult(VirtualFree(memory->transient.base, 0, MEM_RELEASE));
    }

    ZeroMemory(memory, sizeof(Memory));
}

void *PushToTransientArea(
    in Memory *memory,
    in u64     bytes)
{
    Check(memory);
    CheckM(bytes, "0 bytes has been passed");

    CheckM(bytes <= memory->transient.cap - memory->transient.size,
           "Transient area overflow.\n"
           "Bytes to allocate: %I64u.\n"
           "Remain transient area capacity: %I64u.",
           bytes,
           memory->transient.cap - memory->transient.size);

    byte *retptr = memory->transient.base + memory->transient.size;
    memory->transient.size += bytes;

    return retptr;
}

void *PushToTransientAreaAligned(
    in Memory *memory,
    in u64     bytes,
    in u64     alignment)
{
    Check(memory);
    CheckM(bytes, "0 bytes was passed");

    if (alignment < CENGINE_DEFAULT_ALIGNMENT) alignment = CENGINE_DEFAULT_ALIGNMENT;
    CheckM(IS_POW_2(alignment), "Alignment must be power of 2");

    u64 aligned_bytes = ALIGN_UP(bytes, alignment);

    CheckM(bytes <= memory->transient.cap - memory->transient.size,
           "Transient area overflow.\n"
           "Bytes: %I64u. Alignment: %I64u. Aligned bytes: %I64u.\n"
           "Remain transient area capacity: %I64u.",
           bytes, alignment, aligned_bytes,
           memory->transient.cap - memory->transient.size);

    byte *retptr = memory->transient.base + memory->transient.size;
    memory->transient.size += aligned_bytes;

    return retptr;
}

void ResetTransientArea(
    in Memory *memory)
{
    Check(memory);
    ZeroMemory(memory->transient.base, memory->transient.size);
    memory->transient.size = 0;
}

void *PushToPermanentArea(
    in Memory *memory,
    in u64     bytes)
{
    Check(memory);
    CheckM(bytes, "0 bytes was passed");

    CheckM(bytes <= memory->permanent.cap - memory->permanent.size,
           "Permanent area overflow.\n"
           "Bytes to allocate: %I64u.\n"
           "Remain transient area capacity: %I64u.",
           bytes,
           memory->permanent.cap - memory->permanent.size);

    byte *retptr = memory->permanent.base + memory->permanent.size;
    memory->permanent.size += bytes;

    return retptr;
}

void *PushToPermanentAreaAligned(
    in Memory *memory,
    in u64     bytes,
    in u64     alignment)
{
    Check(memory);
    CheckM(bytes, "0 bytes was passed");

    if (alignment < CENGINE_DEFAULT_ALIGNMENT) alignment = CENGINE_DEFAULT_ALIGNMENT;
    CheckM(IS_POW_2(alignment), "Alignment must be power of 2");
    
    u64 aligned_bytes = ALIGN_UP(bytes, alignment);

    CheckM(bytes <= memory->permanent.cap - memory->permanent.size,
           "Transient area overflow.\n"
           "Bytes: %I64u. Alignment: %I64u. Aligned bytes: %I64u.\n"
           "Remain transient area capacity: %I64u.",
           bytes, alignment, aligned_bytes,
           memory->permanent.cap - memory->permanent.size);

    byte *retptr = memory->permanent.base + memory->permanent.size;
    memory->permanent.size += aligned_bytes;

    return retptr;
}
