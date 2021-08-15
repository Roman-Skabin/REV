//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/common.h"
#include "tools/critical_section.hpp"

namespace REV
{
    enum REGULAR_MEMORY_SPECS
    {
        CACHE_LINE_SIZE   = 64,
        PAGE_SIZE         = KB(4),
        NTFS_CLUSTER_SIZE = KB(4),
        NTFS_SECTOR_SIZE  = 512,
    };

    #undef FillMemory
    #undef CopyMemory
    #undef MoveMemory
    #undef ZeroMemory

    REV_API void REV_VECTORCALL FillMemoryF32 (f32  *mem, f32  val, u64 count);
    REV_API void REV_VECTORCALL FillMemoryF64 (f64  *mem, f64  val, u64 count);
    REV_API void REV_VECTORCALL FillMemoryChar(char *mem, char val, u64 count);
    REV_API void REV_VECTORCALL FillMemoryU8  (u8   *mem, u8   val, u64 count);
    REV_API void REV_VECTORCALL FillMemoryU16 (u16  *mem, u16  val, u64 count);
    REV_API void REV_VECTORCALL FillMemoryU32 (u32  *mem, u32  val, u64 count);
    REV_API void REV_VECTORCALL FillMemoryU64 (u64  *mem, u64  val, u64 count);

    template<typename T, typename = RTTI::enable_if_t<RTTI::cmple(sizeof(T), 8)>>
    REV_INLINE void REV_VECTORCALL FillMemory(T *mem, T val, u64 count)
    {
        /**/ if constexpr (RTTI::is_same_v<RTTI::remove_cv<T>, f32>) FillMemoryF32(mem, val, count);
        else if constexpr (RTTI::is_same_v<RTTI::remove_cv<T>, f64>) FillMemoryF64(mem, val, count);
        else if constexpr (sizeof(T) == 1)                           FillMemoryU8 (cast<u8  *>(mem), *cast<u8  *>(&val), count);
        else if constexpr (sizeof(T) == 2)                           FillMemoryU16(cast<u16 *>(mem), *cast<u16 *>(&val), count);
        else if constexpr (sizeof(T) == 4)                           FillMemoryU32(cast<u32 *>(mem), *cast<u32 *>(&val), count);
        else if constexpr (sizeof(T) == 8)                           FillMemoryU64(cast<u64 *>(mem), *cast<u64 *>(&val), count);
        else                                                         while (count--) *mem++ = val;
    }

    template<typename T, u64 count, typename = RTTI::enable_if_t<RTTI::cmpgt(sizeof(T), 8) && RTTI::is_copy_assignable_v<T>>>
    REV_INLINE void FillMemory(T (&arr)[count], const T& val)
    {
        T *ptr = arr;
        while (count--) *ptr++ = val;
    }

    template<typename T, u64 count, typename = RTTI::enable_if_t<RTTI::cmpgt(sizeof(T), 8) && RTTI::is_move_assignable_v<T>>>
    REV_INLINE void FillMemory(T (&arr)[count], T&& val)
    {
        T *ptr = arr;
        while (count--) *ptr++ = RTTI::move(val);
    }

    template<typename T, typename = RTTI::enable_if_t<RTTI::cmpgt(sizeof(T), 8) && RTTI::is_copy_assignable_v<T>>>
    REV_INLINE void FillMemory(T *mem, u64 count, const T& val)
    {
        while (count--) *mem++ = val;
    }

    template<typename T, typename = RTTI::enable_if_t<RTTI::cmpgt(sizeof(T), 8) && RTTI::is_move_assignable_v<T>>>
    REV_INLINE void FillMemory(T *mem, u64 count, T&& val)
    {
        while (count--) *mem++ = RTTI::move(val);
    }

    // @TODO(Roman): ISA wide CopyMemory, MoveMemory, ZeroMemory, CompareMemory
    REV_INLINE void CopyMemory(void *dest, const void *src, u64 bytes) { memcpy(dest, src, bytes);                 }
    REV_INLINE void MoveMemory(void *dest, const void *src, u64 bytes) { memmove(dest, src, bytes);                }
    REV_INLINE void ZeroMemory(void *dest, u64 bytes)                  { FillMemoryU8(cast<u8 *>(dest), 0, bytes); }

    class REV_API Memory final
    {
    public:
        enum : u64 { MAX = 0xFFFFFFFFFFFFF000ui64 };

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

        template<typename T> REV_INLINE T *PushToTA(u64 count = 1)                     { return cast<T *>(PushToTransientArea(count * sizeof(T)));                   }
        template<typename T> REV_INLINE T *PushToTAA(u64 count = 1, u64 alignment = 0) { return cast<T *>(PushToTransientAreaAligned(count * sizeof(T), alignment)); }

        template<typename T> REV_INLINE T *PushToPA(u64 count = 1)                     { return cast<T *>(PushToPermanentArea(count * sizeof(T)));                   }
        template<typename T> REV_INLINE T *PushToPAA(u64 count = 1, u64 alignment = 0) { return cast<T *>(PushToPermanentAreaAligned(count * sizeof(T), alignment)); }

        template<> REV_INLINE void *PushToTA(u64 bytes)                 { return PushToTransientArea(bytes);                   }
        template<> REV_INLINE void *PushToTAA(u64 bytes, u64 alignment) { return PushToTransientAreaAligned(bytes, alignment); }

        template<> REV_INLINE void *PushToPA(u64 bytes)                 { return PushToPermanentArea(bytes);                   }
        template<> REV_INLINE void *PushToPAA(u64 bytes, u64 alignment) { return PushToPermanentAreaAligned(bytes, alignment); }

    private:
        Memory(const Memory&) = delete;
        Memory(Memory&&)      = delete;

        Memory& operator=(const Memory&) = delete;
        Memory& operator=(Memory&&)      = delete;

    private:
        struct Area
        {
            byte *base     = null;
            u64   size     = 0;
            u64   capacity = 0;
        };

        Area                   m_TransientArea;
        Area                   m_PermanentArea;
        CriticalSection<false> m_CriticalSection;

        static Memory *s_Memory;
    };
}
