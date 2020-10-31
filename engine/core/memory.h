//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/core.h"

#define MAX_MEMORY 0xFFFFFFFFFFFFF000ui64

enum REGULAR_MEMORY_SPECS
{
    CACHE_LINE_SIZE   = 64,
    PAGE_SIZE         = KB(4),
    STACK_SIZE        = KB(4),
    HEAP_SIZE         = GB(1),
    NTFS_CLUSTER_SIZE = KB(4),
    NTFS_SECTOR_SIZE  = 512,
};

ENGINE_API void memset_f32 (f32  *mem, f32  val, u64 count);
ENGINE_API void memset_f64 (f64  *mem, f64  val, u64 count);
ENGINE_API void memset_char(char *mem, char val, u64 count);
ENGINE_API void memset_s8  (s8   *mem, s8   val, u64 count);
ENGINE_API void memset_s16 (s16  *mem, s16  val, u64 count);
ENGINE_API void memset_s32 (s32  *mem, s32  val, u64 count);
ENGINE_API void memset_s64 (s64  *mem, s64  val, u64 count);

template<typename T, typename = RTTI::enable_if_t<RTTI::cmple(sizeof(T), 8)>>
void memset_any(T *mem, T val, u64 count)
{
    /**/ if constexpr (RTTI::is_same_v<RTTI::remove_cv<T>, f32>) memset_f32(mem, val, count);
    else if constexpr (RTTI::is_same_v<RTTI::remove_cv<T>, f64>) memset_f64(mem, val, count);
    else if constexpr (sizeof(T) == 1)                           memset_s8 (cast<s8  *>(mem), cast<s8 >(val), count);
    else if constexpr (sizeof(T) == 2)                           memset_s16(cast<s16 *>(mem), cast<s16>(val), count);
    else if constexpr (sizeof(T) == 4)                           memset_s32(cast<s32 *>(mem), cast<s32>(val), count);
    else if constexpr (sizeof(T) == 8)                           memset_s64(cast<s64 *>(mem), cast<s64>(val), count);
    else                                                         static_assert(false, "memset_any failed");
}

template<typename T, u64 count, typename = RTTI::enable_if_t<RTTI::cmpg(sizeof(T), 8)>>
void memset_any(T (&arr)[count], const T& val)
{
    T *ptr = arr;
    while (count--) *ptr++ = val;
}

template<typename T, u64 count, typename = RTTI::enable_if_t<RTTI::cmpg(sizeof(T), 8)>>
void memset_any(T (&arr)[count], T&& val)
{
    T *ptr = arr;
    while (count--) *ptr++ = RTTI::move(val);
}

template<typename T, typename = RTTI::enable_if_t<RTTI::cmpg(sizeof(T), 8)>>
void memset_any(T *mem, const T& val, u64 count)
{
    while (count--) *mem++ = val;
}

template<typename T, typename = RTTI::enable_if_t<RTTI::cmpg(sizeof(T), 8)>>
void memset_any(T *mem, T&& val, u64 count)
{
    while (count--) *mem++ = RTTI::move(val);
}

class ENGINE_API Memory final
{
public:
    static Memory *Create(u64 transient_area_capacity, u64 permanent_area_capacity);
    static Memory *Get();

private:
    Memory(u64 transient_area_capacity, u64 permanent_area_capacity);

public:
    ~Memory();

    void *PushToTransientArea(u64 bytes);
    void *PushToTransientAreaAligned(u64 bytes, u64 alignment);
    void  ResetTransientArea();

    void *PushToPermanentArea(u64 bytes);
    void *PushToPermanentAreaAligned(u64 bytes, u64 alignment);

    template<typename T> constexpr T *PushToTA(u64 count = 1)                         { return cast<T *>(PushToTransientArea(count * sizeof(T)));                   }
    template<typename T> constexpr T *PushToTAA(u64 count = 1, u64 alignment = 0) { return cast<T *>(PushToTransientAreaAligned(count * sizeof(T), alignment)); }

    template<typename T> constexpr T *PushToPA(u64 count = 1)                         { return cast<T *>(PushToPermanentArea(count * sizeof(T)));                   }
    template<typename T> constexpr T *PushToPAA(u64 count = 1, u64 alignment = 0) { return cast<T *>(PushToPermanentAreaAligned(count * sizeof(T), alignment)); }

private:
    Memory(const Memory&) = delete;
    Memory(Memory&&)      = delete;

    Memory& operator=(const Memory&) = delete;
    Memory& operator=(Memory&&)      = delete;

private:
    struct ENGINE_API Area final
    {
        byte *base;
        u64   size;
        u64   capacity;

        Area();
        Area(Area&& other) noexcept;

        Area& operator=(Area&& other) noexcept;

        Area(const Area&) = delete;
        Area& operator=(const Area&) = delete;
    };

    Area m_TransientArea;
    Area m_PermanentArea;

    static Memory *s_Memory;
};
