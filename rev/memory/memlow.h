//
// Copyright 2020-2021 Roman Skabin
//

#pragma once

#include "core/common.h"

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
    REV_INLINE void FillMemory(T *mem, const T& val, u64 count)
    {
        while (count--) *mem++ = val;
    }

    template<typename T, typename = RTTI::enable_if_t<RTTI::cmpgt(sizeof(T), 8) && RTTI::is_move_assignable_v<T>>>
    REV_INLINE void FillMemory(T *mem, T&& val, u64 count)
    {
        while (count--) *mem++ = RTTI::move(val);
    }

    // @TODO(Roman): ISA wide CopyMemory, MoveMemory, ZeroMemory
    REV_INLINE void CopyMemory(void *dest, const void *src, u64 bytes) { memcpy(dest, src, bytes);                 }
    REV_INLINE void MoveMemory(void *dest, const void *src, u64 bytes) { memmove(dest, src, bytes);                }
    REV_INLINE void ZeroMemory(void *dest, u64 bytes)                  { FillMemoryU8(cast<u8 *>(dest), 0, bytes); }

    enum COMPARE_RESULT : s8
    {
        COMPARE_RESULT_LT = -1,
        COMPARE_RESULT_EQ =  0,
        COMPARE_RESULT_GT =  1
    };
    
    // @TODO(Roman): ISA wide CompareMemory
    REV_INLINE int CompareMemory(const void *left, const void *right, u64 bytes) { return memcmp(left, right, bytes); }

    REV_API COMPARE_RESULT REV_VECTORCALL CompareStrings(const char *left, u64 left_length, const char *right, u64 right_length);
    REV_API COMPARE_RESULT REV_VECTORCALL CompareUnicodeStrings(const wchar_t *left, u64 left_length, const wchar_t *right, u64 right_length);

    // @Important(Roman): Strings must be 16-byte aligned
    REV_API COMPARE_RESULT REV_VECTORCALL CompareStringsAligned(const char *left, u64 left_length, const char *right, u64 right_length);
    REV_API COMPARE_RESULT REV_VECTORCALL CompareUnicodeStringsAligned(const wchar_t *left, u64 left_length, const wchar_t *right, u64 right_length);
}
