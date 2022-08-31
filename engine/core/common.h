// Copyright (c) 2020-2021, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#pragma once

#include "core/pch.h"

#pragma warning(disable: 4251) // class 'type1' needs to have dll-interface to be used by clients of class 'type2'
#pragma warning(disable: 4200) // nonstandard extension used: zero-sized array in struct/union

// REV predefined macros:
//     _REV_DEV            - for engine developers, is used to build DLL.
//     _REV_CHECKS_BREAK   - REV_DEBUG_BREAK in checks before ExitProcess.
//     _REV_NO_CHECKS      - turns off all checks except REV_ERROR_M
//     _REV_GLOBAL_TYPES   - can use types without REV:: prefix
//     _REV_GLOBAL_HELPERS - can use helpers without REV:: prefix

namespace REV
{

enum
{
#if REV_ISA >= REV_ISA_AVX512
    DEFAULT_ALIGNMENT = sizeof(__m512)
#elif REV_ISA >= REV_ISA_AVX
    DEFAULT_ALIGNMENT = sizeof(__m256)
#else
    DEFAULT_ALIGNMENT = sizeof(__m128)
#endif
};

#ifdef _DEBUG
    #define REV_DEBUG   1
    #define REV_RELEASE 0
#else
    #define REV_DEBUG   0
    #define REV_RELEASE 1
#endif

#if _REV_DEV && REV_DEBUG
    #define REV_DEVDEBUG 1
#else
    #define REV_DEVDEBUG 0
#endif

#if _REV_DEV
    #define REV_API REV_EXPORT
#else
    #define REV_API REV_IMPORT
#endif

#ifdef __cplusplus
    #define REV_EXTERN       extern "C"
    #define REV_EXTERN_START extern "C" {
    #define REV_EXTERN_END   }
#else
    #define REV_EXTERN extern
    #define REV_EXTERN_START
    #define REV_EXTERN_END
#endif

#define REV_GLOBAL   static
#define REV_INTERNAL static
#define REV_LOCAL    static

#define REV__CSTR(x) #x
#define REV_CSTR(x)  REV__CSTR(x)

#define REV__CSTRCAT(a, b) a ## b
#define REV_CSTRCAT(a, b)  REV__CSTRCAT(a, b)

#define REV_CSTRLEN(cstr) (sizeof(cstr) / sizeof(*(cstr)) - sizeof(*(cstr)))

#define REV_CSTR_ARGS(cstr)    (cstr),  REV_CSTRLEN(cstr)
#define REV_CARRAY_ARGS(array) (array), (::REV::ArrayCount(array))

#define null nullptr

#define REV_INTERFACE struct REV_NOVTABLE

//
// Types
//

#if _REV_GLOBAL_TYPES
namespace
{
#endif

static_assert(sizeof(long) == 4);

typedef signed char      s8;
typedef signed short     s16;
typedef signed long      s32;
typedef signed long long s64;

typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned long      u32;
typedef unsigned long long u64;

typedef float  f32;
typedef double f64;

typedef f32 rad;
typedef f32 deg;

typedef f32 rad32;
typedef f32 deg32;

typedef f64 rad64;
typedef f64 deg64;

typedef s8  b8;
typedef s16 b16;
typedef s32 b32;
typedef s64 b64;

typedef u8 byte;

typedef u32 intptr32;
typedef u64 intptr64;

#if _REV_GLOBAL_TYPES
}
#endif

#define REV_S8_MIN  0x80
#define REV_S16_MIN 0x8000
#define REV_S32_MIN 0x8000'0000
#define REV_S64_MIN 0x8000'0000'0000'0000

#define REV_S8_MAX  0x7F
#define REV_S16_MAX 0x7FFF
#define REV_S32_MAX 0x7FFF'FFFF
#define REV_S64_MAX 0x7FFF'FFFF'FFFF'FFFF

#define REV_U8_MIN  0x00
#define REV_U16_MIN 0x0000
#define REV_U32_MIN 0x0000'0000
#define REV_U64_MIN 0x0000'0000'0000'0000

#define REV_U8_MAX  0xFF
#define REV_U16_MAX 0xFFFF
#define REV_U32_MAX 0xFFFF'FFFF
#define REV_U64_MAX 0xFFFF'FFFF'FFFF'FFFF

#define REV_F32_MIN     0hFF7FFFFF
#define REV_F32_MAX     0h7F7FFFFF
#define REV_F32_POS_MIN 0h00800000
#define REV_F32_INF     0h7F800000
#define REV_F32_NEG_INF 0hFF800000

#define REV_F64_MIN     0hFFEFFFFFFFFFFFFF
#define REV_F64_MAX     0h7FEFFFFFFFFFFFFF
#define REV_F64_POS_MIN 0h0010000000000000
#define REV_F64_INF     0h7FF0000000000000
#define REV_F64_NEG_INF 0hFFF0000000000000

#define REV_BYTE_MAX REV_U8_MAX
#define REV_BYTE_MIN REV_U8_MIN

//
// Helpers
//

#define KB(x) ((x) << 10)
#define MB(x) ((x) << 20)
#define GB(x) ((x) << 30)
#define TB(x) ((x) << 40)

#define cast(Type, expr) ((Type)(expr))

#define REV_INVALID_U8_INDEX  REV_U8_MAX
#define REV_INVALID_U16_INDEX REV_U16_MAX
#define REV_INVALID_U32_INDEX REV_U32_MAX
#define REV_INVALID_U64_INDEX REV_U64_MAX

#define REV_INVALID_U8_OFFSET  REV_U8_MAX
#define REV_INVALID_U16_OFFSET REV_U16_MAX
#define REV_INVALID_U32_OFFSET REV_U32_MAX
#define REV_INVALID_U64_OFFSET REV_U64_MAX

#if _REV_GLOBAL_HELPERS
namespace
{
#endif

template<typename T, u64 count> constexpr REV_INLINE u64 ArrayCount(T (&)[count]) { return count; }

#define REV_ArrayCountInStruct(_struct, _field) ArrayCount(cast(_struct *, null)->_field)
#define REV_StructFieldSize(_struct, _field)    sizeof(cast(_struct *, null)->_field)
#define REV_StructFieldOffset(_struct, _field)  cast(u64, &(cast(_struct *, null)->_field))

template<typename T, typename U, typename Ret = RTTI::max_size_t<T, U>, typename = RTTI::enable_if_t<RTTI::is_integral_v<T> && RTTI::is_integral_v<U>>>
constexpr REV_INLINE Ret AlignUp(T x, U a = DEFAULT_ALIGNMENT)
{
    const U mask = a - 1;
    return cast(Ret, (x + mask) & ~mask);
}

template<typename T, typename U, typename Ret = RTTI::max_size_t<T, U>, typename = RTTI::enable_if_t<RTTI::is_integral_v<T> && RTTI::is_integral_v<U>>>
constexpr REV_INLINE Ret AlignDown(T x, U a = DEFAULT_ALIGNMENT)
{
    return cast(Ret, x & ~(a - 1));
}

template<typename T, typename = RTTI::enable_if_t<RTTI::is_integral_v<T>>>
constexpr REV_INLINE bool IsPowOf2(T x)
{
    return x && ((x & (x - 1)) == 0);
}

#if _REV_GLOBAL_HELPERS
}
#endif

#define REV_ENUM_CLASS_OPERATORS(ENUM_CLASS)                                                                                                                                                                                                                                        \
                                                                               constexpr REV_INLINE ENUM_CLASS  operator| (ENUM_CLASS  left, ENUM_CLASS right) { return (ENUM_CLASS )( (RTTI::underlying_type_t<ENUM_CLASS> )left |  (RTTI::underlying_type_t<ENUM_CLASS>)right); } \
                                                                               constexpr REV_INLINE ENUM_CLASS  operator& (ENUM_CLASS  left, ENUM_CLASS right) { return (ENUM_CLASS )( (RTTI::underlying_type_t<ENUM_CLASS> )left &  (RTTI::underlying_type_t<ENUM_CLASS>)right); } \
                                                                               constexpr REV_INLINE ENUM_CLASS  operator^ (ENUM_CLASS  left, ENUM_CLASS right) { return (ENUM_CLASS )( (RTTI::underlying_type_t<ENUM_CLASS> )left ^  (RTTI::underlying_type_t<ENUM_CLASS>)right); } \
                                                                               constexpr REV_INLINE ENUM_CLASS  operator~ (ENUM_CLASS  left                  ) { return (ENUM_CLASS )(~(RTTI::underlying_type_t<ENUM_CLASS> )left                                              ); } \
                                                                               constexpr REV_INLINE ENUM_CLASS& operator|=(ENUM_CLASS& left, ENUM_CLASS right) { return (ENUM_CLASS&)( (RTTI::underlying_type_t<ENUM_CLASS>&)left |= (RTTI::underlying_type_t<ENUM_CLASS>)right); } \
                                                                               constexpr REV_INLINE ENUM_CLASS& operator&=(ENUM_CLASS& left, ENUM_CLASS right) { return (ENUM_CLASS&)( (RTTI::underlying_type_t<ENUM_CLASS>&)left &= (RTTI::underlying_type_t<ENUM_CLASS>)right); } \
                                                                               constexpr REV_INLINE ENUM_CLASS& operator^=(ENUM_CLASS& left, ENUM_CLASS right) { return (ENUM_CLASS&)( (RTTI::underlying_type_t<ENUM_CLASS>&)left ^= (RTTI::underlying_type_t<ENUM_CLASS>)right); } \
    template<typename T, typename = RTTI::enable_if_t<RTTI::is_integral_v<T>>> constexpr REV_INLINE ENUM_CLASS  operator| (ENUM_CLASS  left, T          right) { return (ENUM_CLASS )( (RTTI::underlying_type_t<ENUM_CLASS> )left |  (RTTI::underlying_type_t<ENUM_CLASS>)right); } \
    template<typename T, typename = RTTI::enable_if_t<RTTI::is_integral_v<T>>> constexpr REV_INLINE ENUM_CLASS  operator| (T           left, ENUM_CLASS right) { return (ENUM_CLASS )( (RTTI::underlying_type_t<ENUM_CLASS> )left |  (RTTI::underlying_type_t<ENUM_CLASS>)right); } \
    template<typename T, typename = RTTI::enable_if_t<RTTI::is_integral_v<T>>> constexpr REV_INLINE ENUM_CLASS  operator& (ENUM_CLASS  left, T          right) { return (ENUM_CLASS )( (RTTI::underlying_type_t<ENUM_CLASS> )left &  (RTTI::underlying_type_t<ENUM_CLASS>)right); } \
    template<typename T, typename = RTTI::enable_if_t<RTTI::is_integral_v<T>>> constexpr REV_INLINE ENUM_CLASS  operator& (T           left, ENUM_CLASS right) { return (ENUM_CLASS )( (RTTI::underlying_type_t<ENUM_CLASS> )left &  (RTTI::underlying_type_t<ENUM_CLASS>)right); } \
    template<typename T, typename = RTTI::enable_if_t<RTTI::is_integral_v<T>>> constexpr REV_INLINE ENUM_CLASS  operator^ (ENUM_CLASS  left, T          right) { return (ENUM_CLASS )( (RTTI::underlying_type_t<ENUM_CLASS> )left ^  (RTTI::underlying_type_t<ENUM_CLASS>)right); } \
    template<typename T, typename = RTTI::enable_if_t<RTTI::is_integral_v<T>>> constexpr REV_INLINE ENUM_CLASS  operator^ (T           left, ENUM_CLASS right) { return (ENUM_CLASS )( (RTTI::underlying_type_t<ENUM_CLASS> )left ^  (RTTI::underlying_type_t<ENUM_CLASS>)right); } \
    template<typename T, typename = RTTI::enable_if_t<RTTI::is_integral_v<T>>> constexpr REV_INLINE ENUM_CLASS& operator|=(ENUM_CLASS& left, T          right) { return (ENUM_CLASS&)( (RTTI::underlying_type_t<ENUM_CLASS>&)left |= (RTTI::underlying_type_t<ENUM_CLASS>)right); } \
    template<typename T, typename = RTTI::enable_if_t<RTTI::is_integral_v<T>>> constexpr REV_INLINE ENUM_CLASS& operator|=(T&          left, ENUM_CLASS right) { return (ENUM_CLASS&)( (RTTI::underlying_type_t<ENUM_CLASS>&)left |= (RTTI::underlying_type_t<ENUM_CLASS>)right); } \
    template<typename T, typename = RTTI::enable_if_t<RTTI::is_integral_v<T>>> constexpr REV_INLINE ENUM_CLASS& operator&=(ENUM_CLASS& left, T          right) { return (ENUM_CLASS&)( (RTTI::underlying_type_t<ENUM_CLASS>&)left &= (RTTI::underlying_type_t<ENUM_CLASS>)right); } \
    template<typename T, typename = RTTI::enable_if_t<RTTI::is_integral_v<T>>> constexpr REV_INLINE ENUM_CLASS& operator&=(T&          left, ENUM_CLASS right) { return (ENUM_CLASS&)( (RTTI::underlying_type_t<ENUM_CLASS>&)left &= (RTTI::underlying_type_t<ENUM_CLASS>)right); } \
    template<typename T, typename = RTTI::enable_if_t<RTTI::is_integral_v<T>>> constexpr REV_INLINE ENUM_CLASS& operator^=(ENUM_CLASS& left, T          right) { return (ENUM_CLASS&)( (RTTI::underlying_type_t<ENUM_CLASS>&)left ^= (RTTI::underlying_type_t<ENUM_CLASS>)right); } \
    template<typename T, typename = RTTI::enable_if_t<RTTI::is_integral_v<T>>> constexpr REV_INLINE ENUM_CLASS& operator^=(T&          left, ENUM_CLASS right) { return (ENUM_CLASS&)( (RTTI::underlying_type_t<ENUM_CLASS>&)left ^= (RTTI::underlying_type_t<ENUM_CLASS>)right); } \
    template<typename T, typename = RTTI::enable_if_t<RTTI::is_integral_v<T>>> constexpr REV_INLINE bool        operator==(T           left, ENUM_CLASS right) { return         left  == cast(T, right); } \
    template<typename T, typename = RTTI::enable_if_t<RTTI::is_integral_v<T>>> constexpr REV_INLINE bool        operator!=(T           left, ENUM_CLASS right) { return         left  != cast(T, right); } \
    template<typename T, typename = RTTI::enable_if_t<RTTI::is_integral_v<T>>> constexpr REV_INLINE bool        operator<=(T           left, ENUM_CLASS right) { return         left  <= cast(T, right); } \
    template<typename T, typename = RTTI::enable_if_t<RTTI::is_integral_v<T>>> constexpr REV_INLINE bool        operator>=(T           left, ENUM_CLASS right) { return         left  >= cast(T, right); } \
    template<typename T, typename = RTTI::enable_if_t<RTTI::is_integral_v<T>>> constexpr REV_INLINE bool        operator< (T           left, ENUM_CLASS right) { return         left  <  cast(T, right); } \
    template<typename T, typename = RTTI::enable_if_t<RTTI::is_integral_v<T>>> constexpr REV_INLINE bool        operator> (T           left, ENUM_CLASS right) { return         left  >  cast(T, right); } \
    template<typename T, typename = RTTI::enable_if_t<RTTI::is_integral_v<T>>> constexpr REV_INLINE bool        operator==(ENUM_CLASS  left, T          right) { return cast(T, left) ==         right;  } \
    template<typename T, typename = RTTI::enable_if_t<RTTI::is_integral_v<T>>> constexpr REV_INLINE bool        operator!=(ENUM_CLASS  left, T          right) { return cast(T, left) !=         right;  } \
    template<typename T, typename = RTTI::enable_if_t<RTTI::is_integral_v<T>>> constexpr REV_INLINE bool        operator<=(ENUM_CLASS  left, T          right) { return cast(T, left) <=         right;  } \
    template<typename T, typename = RTTI::enable_if_t<RTTI::is_integral_v<T>>> constexpr REV_INLINE bool        operator>=(ENUM_CLASS  left, T          right) { return cast(T, left) >=         right;  } \
    template<typename T, typename = RTTI::enable_if_t<RTTI::is_integral_v<T>>> constexpr REV_INLINE bool        operator< (ENUM_CLASS  left, T          right) { return cast(T, left) <          right;  } \
    template<typename T, typename = RTTI::enable_if_t<RTTI::is_integral_v<T>>> constexpr REV_INLINE bool        operator> (ENUM_CLASS  left, T          right) { return cast(T, left) >          right;  }

#define REV_ENUM_OPERATORS(ENUM) REV_ENUM_CLASS_OPERATORS(ENUM)

#define REV_INTERFACE_STUFF(Interface)                   \
    protected:                                           \
        REV_INLINE          Interface() {}               \
        REV_INLINE virtual ~Interface() {}               \
    private:                                             \
        Interface(const Interface&)            = delete; \
        Interface(Interface&&)                 = delete; \
        Interface& operator=(const Interface&) = delete; \
        Interface& operator=(Interface&&)      = delete

#define REV_DELETE_CONSTRS_AND_OPS(Class)    \
    Class(const Class&)            = delete; \
    Class(Class&&)                 = delete; \
    Class& operator=(const Class&) = delete; \
    Class& operator=(Class&&)      = delete

#define REV_REMOVE_OOP_STUFF(Class)          \
    Class()                        = delete; \
    ~Class()                       = delete; \
    Class(const Class&)            = delete; \
    Class(Class&&)                 = delete; \
    Class& operator=(const Class&) = delete; \
    Class& operator=(Class&&)      = delete

//
// Debugging
//

#define WIN32_ERROR ERROR
#undef ERROR

enum class DEBUG_COLOR : u16
{
    INFO    = 0x7,
    ERROR   = 0x4,
    WARNING = 0x6,
    SUCCESS = 0xA,
};

REV_API void REV_CDECL PrintDebugMessage(DEBUG_COLOR color, const char *format, ...);
REV_API void REV_CDECL PrintDebugMessage(const char *file, u64 line, DEBUG_COLOR color, bool print_sys_error, bool print_stack_trace, const char *format, ...);

// @NOTE(Roman):      ConstString is pushed on the frame arena, so you don't need to free anything.
// @Important(Roman): Also keep in mind that data will be valid only for 1 frame long.
REV_API class ConstString GetSysErrorMessage(u32 error_code);
REV_API u32 GetSysErrorCode();

#if (REV_DEVDEBUG || defined(_REV_CHECKS_BREAK)) && !defined(_REV_NO_CHECKS)

    #define REV_INFO_M(message, ...)     { ::REV::PrintDebugMessage(__FILE__, __LINE__, DEBUG_COLOR::INFO,    false, false, message, __VA_ARGS__);                                    }
    #define REV_ERROR_M(message, ...)    { ::REV::PrintDebugMessage(__FILE__, __LINE__, DEBUG_COLOR::ERROR,   true,  true,  message, __VA_ARGS__); REV_DEBUG_BREAK(); ExitProcess(1); }
    #define REV_WARNING_M(message, ...)  { ::REV::PrintDebugMessage(__FILE__, __LINE__, DEBUG_COLOR::WARNING, false, false, message, __VA_ARGS__);                                    }
    #define REV_WARNING_MS(message, ...) { ::REV::PrintDebugMessage(__FILE__, __LINE__, DEBUG_COLOR::WARNING, true,  false, message, __VA_ARGS__);                                    }
    #define REV_SUCCESS_M(message, ...)  { ::REV::PrintDebugMessage(__FILE__, __LINE__, DEBUG_COLOR::SUCCESS, false, false, message, __VA_ARGS__);                                    }

    #define REV_CHECK(expr)                 { if (!(expr)) REV_ERROR_M(REV_CSTR(expr))       }
    #define REV_CHECK_M(expr, message, ...) { if (!(expr)) REV_ERROR_M(message, __VA_ARGS__) }

    #define REV_DEBUG_RESULT(expr)                 REV_CHECK(expr)
    #define REV_DEBUG_RESULT_M(expr, message, ...) REV_CHECK_M(expr, message, __VA_ARGS__)

#elif !defined(_REV_NO_CHECKS)

    #define REV_INFO_M(message, ...)     { ::REV::PrintDebugMessage(__FILE__, __LINE__, DEBUG_COLOR::INFO,    false, false, message, __VA_ARGS__);                 }
    #define REV_ERROR_M(message, ...)    { ::REV::PrintDebugMessage(__FILE__, __LINE__, DEBUG_COLOR::ERROR,   true,  true,  message, __VA_ARGS__); ExitProcess(1); }
    #define REV_WARNING_M(message, ...)  { ::REV::PrintDebugMessage(__FILE__, __LINE__, DEBUG_COLOR::WARNING, false, false, message, __VA_ARGS__);                 }
    #define REV_WARNING_MS(message, ...) { ::REV::PrintDebugMessage(__FILE__, __LINE__, DEBUG_COLOR::WARNING, true,  false, message, __VA_ARGS__);                 }
    #define REV_SUCCESS_M(message, ...)  { ::REV::PrintDebugMessage(__FILE__, __LINE__, DEBUG_COLOR::SUCCESS, false, false, message, __VA_ARGS__);                 }

    #define REV_CHECK(expr)                 { if (!(expr)) REV_ERROR_M(REV_CSTR(expr))       }
    #define REV_CHECK_M(expr, message, ...) { if (!(expr)) REV_ERROR_M(message, __VA_ARGS__) }

    #define REV_DEBUG_RESULT(expr)                 REV_CHECK(expr)
    #define REV_DEBUG_RESULT_M(expr, message, ...) REV_CHECK_M(expr, message, __VA_ARGS__)

#else

    #define REV_INFO_M(message, ...)     { ::REV::PrintDebugMessage(__FILE__, __LINE__, DEBUG_COLOR::INFO,    false, false, message, __VA_ARGS__);                 }
    #define REV_ERROR_M(message, ...)    { ::REV::PrintDebugMessage(__FILE__, __LINE__, DEBUG_COLOR::ERROR,   true,  true,  message, __VA_ARGS__); ExitProcess(1); }
    #define REV_WARNING_M(message, ...)  { ::REV::PrintDebugMessage(__FILE__, __LINE__, DEBUG_COLOR::WARNING, false, false, message, __VA_ARGS__);                 }
    #define REV_WARNING_MS(message, ...) { ::REV::PrintDebugMessage(__FILE__, __LINE__, DEBUG_COLOR::WARNING, true,  false, message, __VA_ARGS__);                 }
    #define REV_SUCCESS_M(message, ...)  { ::REV::PrintDebugMessage(__FILE__, __LINE__, DEBUG_COLOR::SUCCESS, false, false, message, __VA_ARGS__);                 }

    #define REV_CHECK(expr)                 {}
    #define REV_CHECK_M(expr, message, ...) {}

    #define REV_DEBUG_RESULT(expr)                 { expr; }
    #define REV_DEBUG_RESULT_M(expr, message, ...) { expr; }

#endif

} // namespace REV
