//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/pch.h"
#include "core/rtti.hpp"

#pragma warning(disable: 4251) // class 'type1' needs to have dll-interface to be used by clients of class 'type2'

// REV predefined macros:
//     _REV_DEV            - for engine developers, is used to build DLL.
//     _REV_CHECKS_BREAK   - __debugbreak in checks before ExitProcess.
//     _REV_NO_CHECKS      - turns off all checks except REV_FAILED_M
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
    #define REV_EXTERN extern "C"
#else
    #define REV_EXTERN extern
#endif

#define REV_GLOBAL   static
#define REV_INTERNAL static
#define REV_LOCAL    static

#define REV__CSTR(x) #x 
#define REV_CSTR(x)  REV__CSTR(x)

#define REV__CSTRCAT(a, b) a ## b
#define REV_CSTRCAT(a, b)  REV__CSTRCAT(a, b)

#define REV_CSTRLEN(cstr) (sizeof(cstr) - 1)

#define null nullptr

#define REV_INTERFACE struct REV_NOVTABLE

//
// Types
//

namespace Types
{

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

} // Types

using namespace Types;

#if _REV_GLOBAL_TYPES
} // REV

using namespace REV::Types;

namespace REV
{
#endif

#define REV_S8_MIN  0x80i8
#define REV_S16_MIN 0x8000i16
#define REV_S32_MIN 0x80000000i32
#define REV_S64_MIN 0x8000000000000000i64

#define REV_S8_MAX  0x7Fi8
#define REV_S16_MAX 0x7FFFi16
#define REV_S32_MAX 0x7FFFFFFFi32
#define REV_S64_MAX 0x7FFFFFFFFFFFFFFFi64

#define REV_U8_MIN  0x00ui8
#define REV_U16_MIN 0x0000ui16
#define REV_U32_MIN 0x00000000ui32
#define REV_U64_MIN 0x0000000000000000ui64

#define REV_U8_MAX  0xFFui8
#define REV_U16_MAX 0xFFFFui16
#define REV_U32_MAX 0xFFFFFFFFui32
#define REV_U64_MAX 0xFFFFFFFFFFFFFFFFui64

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

namespace Helpers
{

template<typename T, typename = ::REV::RTTI::enable_if_t<::REV::RTTI::is_arithmetic_v<T>>> constexpr REV_INLINE T KB(T x) { return    x  * 1024; }
template<typename T, typename = ::REV::RTTI::enable_if_t<::REV::RTTI::is_arithmetic_v<T>>> constexpr REV_INLINE T MB(T x) { return KB(x) * 1024; }
template<typename T, typename = ::REV::RTTI::enable_if_t<::REV::RTTI::is_arithmetic_v<T>>> constexpr REV_INLINE T GB(T x) { return MB(x) * 1024; }
template<typename T, typename = ::REV::RTTI::enable_if_t<::REV::RTTI::is_arithmetic_v<T>>> constexpr REV_INLINE T TB(T x) { return GB(x) * 1024; }

template<typename T, typename = ::REV::RTTI::enable_if_t<::REV::RTTI::is_integral_v<T>>> constexpr REV_INLINE T BIT(T x) { return 1 << x; }

#pragma warning(suppress: 4172)
template<typename To, typename From> constexpr REV_INLINE To cast(From x) { return (To)x; }

template<typename T, u64 count> constexpr REV_INLINE u64 ArrayCount(T (&)[count]) { return count; }

#define REV_ArrayCountInStruct(_struct, _field) ArrayCount(cast<_struct *>(null)->_field)
#define REV_StructFieldSize(_struct, _field)    sizeof(cast<_struct *>(null)->_field)
#define REV_StructFieldOffset(_struct, _field)  cast<u64>(&(cast<_struct *>(null)->_field))

template<typename T, typename U, typename = RTTI::enable_if_t<RTTI::is_integral_v<T> && RTTI::is_integral_v<U>>>
constexpr REV_INLINE RTTI::max_size_t<T, U> AlignUp(T x, U a = DEFAULT_ALIGNMENT)
{
    return cast<RTTI::max_size_t<T, U>>((x + (a - 1)) & ~(a - 1));
}

template<typename T, typename U, typename = RTTI::enable_if_t<RTTI::is_integral_v<T> && RTTI::is_integral_v<U>>>
constexpr REV_INLINE RTTI::max_size_t<T, U> AlignDown(T x, U a = DEFAULT_ALIGNMENT)
{
    return cast<RTTI::max_size_t<T, U>>(x & ~(a - 1));
}

template<typename T, typename = RTTI::enable_if_t<RTTI::is_integral_v<T>>>
constexpr REV_INLINE bool IsPowOf2(T x)
{
    return x && ((x & (x - 1)) == 0);
}

} // Helpers

using namespace Helpers;

#if _REV_GLOBAL_HELPERS
} // REV

using namespace REV::Helpers;

namespace REV
{
#endif

#define REV_ENUM_CLASS_OPERATORS(ENUM_CLASS)                                                                                                                                                                                                                                                \
                                                                               constexpr REV_INLINE ENUM_CLASS  operator| (ENUM_CLASS  left, ENUM_CLASS right) { return cast<ENUM_CLASS >( cast<RTTI::underlying_type_t<ENUM_CLASS> >(left) |  cast<RTTI::underlying_type_t<ENUM_CLASS>>(right)); } \
                                                                               constexpr REV_INLINE ENUM_CLASS  operator& (ENUM_CLASS  left, ENUM_CLASS right) { return cast<ENUM_CLASS >( cast<RTTI::underlying_type_t<ENUM_CLASS> >(left) &  cast<RTTI::underlying_type_t<ENUM_CLASS>>(right)); } \
                                                                               constexpr REV_INLINE ENUM_CLASS  operator^ (ENUM_CLASS  left, ENUM_CLASS right) { return cast<ENUM_CLASS >( cast<RTTI::underlying_type_t<ENUM_CLASS> >(left) ^  cast<RTTI::underlying_type_t<ENUM_CLASS>>(right)); } \
                                                                               constexpr REV_INLINE ENUM_CLASS  operator~ (ENUM_CLASS  left                  ) { return cast<ENUM_CLASS >(~cast<RTTI::underlying_type_t<ENUM_CLASS> >(left)                                                    ); } \
                                                                               constexpr REV_INLINE ENUM_CLASS& operator|=(ENUM_CLASS& left, ENUM_CLASS right) { return cast<ENUM_CLASS&>( cast<RTTI::underlying_type_t<ENUM_CLASS>&>(left) |= cast<RTTI::underlying_type_t<ENUM_CLASS>>(right)); } \
                                                                               constexpr REV_INLINE ENUM_CLASS& operator&=(ENUM_CLASS& left, ENUM_CLASS right) { return cast<ENUM_CLASS&>( cast<RTTI::underlying_type_t<ENUM_CLASS>&>(left) &= cast<RTTI::underlying_type_t<ENUM_CLASS>>(right)); } \
                                                                               constexpr REV_INLINE ENUM_CLASS& operator^=(ENUM_CLASS& left, ENUM_CLASS right) { return cast<ENUM_CLASS&>( cast<RTTI::underlying_type_t<ENUM_CLASS>&>(left) ^= cast<RTTI::underlying_type_t<ENUM_CLASS>>(right)); } \
    template<typename T, typename = RTTI::enable_if_t<RTTI::is_integral_v<T>>> constexpr REV_INLINE ENUM_CLASS  operator| (ENUM_CLASS  left, T          right) { return cast<ENUM_CLASS >( cast<RTTI::underlying_type_t<ENUM_CLASS> >(left) |  cast<RTTI::underlying_type_t<ENUM_CLASS>>(right)); } \
    template<typename T, typename = RTTI::enable_if_t<RTTI::is_integral_v<T>>> constexpr REV_INLINE ENUM_CLASS  operator| (T           left, ENUM_CLASS right) { return cast<ENUM_CLASS >( cast<RTTI::underlying_type_t<ENUM_CLASS> >(left) |  cast<RTTI::underlying_type_t<ENUM_CLASS>>(right)); } \
    template<typename T, typename = RTTI::enable_if_t<RTTI::is_integral_v<T>>> constexpr REV_INLINE ENUM_CLASS  operator& (ENUM_CLASS  left, T          right) { return cast<ENUM_CLASS >( cast<RTTI::underlying_type_t<ENUM_CLASS> >(left) &  cast<RTTI::underlying_type_t<ENUM_CLASS>>(right)); } \
    template<typename T, typename = RTTI::enable_if_t<RTTI::is_integral_v<T>>> constexpr REV_INLINE ENUM_CLASS  operator& (T           left, ENUM_CLASS right) { return cast<ENUM_CLASS >( cast<RTTI::underlying_type_t<ENUM_CLASS> >(left) &  cast<RTTI::underlying_type_t<ENUM_CLASS>>(right)); } \
    template<typename T, typename = RTTI::enable_if_t<RTTI::is_integral_v<T>>> constexpr REV_INLINE ENUM_CLASS  operator^ (ENUM_CLASS  left, T          right) { return cast<ENUM_CLASS >( cast<RTTI::underlying_type_t<ENUM_CLASS> >(left) ^  cast<RTTI::underlying_type_t<ENUM_CLASS>>(right)); } \
    template<typename T, typename = RTTI::enable_if_t<RTTI::is_integral_v<T>>> constexpr REV_INLINE ENUM_CLASS  operator^ (T           left, ENUM_CLASS right) { return cast<ENUM_CLASS >( cast<RTTI::underlying_type_t<ENUM_CLASS> >(left) ^  cast<RTTI::underlying_type_t<ENUM_CLASS>>(right)); } \
    template<typename T, typename = RTTI::enable_if_t<RTTI::is_integral_v<T>>> constexpr REV_INLINE ENUM_CLASS& operator|=(ENUM_CLASS& left, T          right) { return cast<ENUM_CLASS&>( cast<RTTI::underlying_type_t<ENUM_CLASS>&>(left) |= cast<RTTI::underlying_type_t<ENUM_CLASS>>(right)); } \
    template<typename T, typename = RTTI::enable_if_t<RTTI::is_integral_v<T>>> constexpr REV_INLINE ENUM_CLASS& operator|=(T&          left, ENUM_CLASS right) { return cast<ENUM_CLASS&>( cast<RTTI::underlying_type_t<ENUM_CLASS>&>(left) |= cast<RTTI::underlying_type_t<ENUM_CLASS>>(right)); } \
    template<typename T, typename = RTTI::enable_if_t<RTTI::is_integral_v<T>>> constexpr REV_INLINE ENUM_CLASS& operator&=(ENUM_CLASS& left, T          right) { return cast<ENUM_CLASS&>( cast<RTTI::underlying_type_t<ENUM_CLASS>&>(left) &= cast<RTTI::underlying_type_t<ENUM_CLASS>>(right)); } \
    template<typename T, typename = RTTI::enable_if_t<RTTI::is_integral_v<T>>> constexpr REV_INLINE ENUM_CLASS& operator&=(T&          left, ENUM_CLASS right) { return cast<ENUM_CLASS&>( cast<RTTI::underlying_type_t<ENUM_CLASS>&>(left) &= cast<RTTI::underlying_type_t<ENUM_CLASS>>(right)); } \
    template<typename T, typename = RTTI::enable_if_t<RTTI::is_integral_v<T>>> constexpr REV_INLINE ENUM_CLASS& operator^=(ENUM_CLASS& left, T          right) { return cast<ENUM_CLASS&>( cast<RTTI::underlying_type_t<ENUM_CLASS>&>(left) ^= cast<RTTI::underlying_type_t<ENUM_CLASS>>(right)); } \
    template<typename T, typename = RTTI::enable_if_t<RTTI::is_integral_v<T>>> constexpr REV_INLINE ENUM_CLASS& operator^=(T&          left, ENUM_CLASS right) { return cast<ENUM_CLASS&>( cast<RTTI::underlying_type_t<ENUM_CLASS>&>(left) ^= cast<RTTI::underlying_type_t<ENUM_CLASS>>(right)); }

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

#define REV_DELETE_CONSTRS_AND_OPS(Class)        \
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

REV_API void REV_CDECL DebugF(const char *format, ...);
REV_API void REV_CDECL DebugFC(DEBUG_COLOR color, const char *format, ...);
REV_API void REV_CDECL PrintDebugMessage(const char *file, u64 line, const char *format, ...);

#if (REV_DEVDEBUG || defined(_REV_CHECKS_BREAK)) && !defined(_REV_NO_CHECKS)

    #define REV_CHECK_M(expr, message, ...) { if (!(expr)) { ::REV::PrintDebugMessage(__FILE__, __LINE__, message,   __VA_ARGS__); __debugbreak(); ExitProcess(1); } }
    #define REV_CHECK(expr)                 { if (!(expr)) { ::REV::PrintDebugMessage(__FILE__, __LINE__, REV_CSTR(expr)        ); __debugbreak(); ExitProcess(1); } }
    #define REV_FAILED_M(message, ...)      {                ::REV::PrintDebugMessage(__FILE__, __LINE__, message,   __VA_ARGS__); __debugbreak(); ExitProcess(1);   }
    #define REV_FAILED()                    {                ::REV::PrintDebugMessage(__FILE__, __LINE__, null                  ); __debugbreak(); ExitProcess(1);   }

    #define REV_DEBUG_RESULT(expr)                 REV_CHECK(expr)
    #define REV_DEBUG_RESULT_M(expr, message, ...) REV_CHECK_M(expr, message, __VA_ARGS__)

#elif !defined(_REV_NO_CHECKS)

    #define REV_CHECK_M(expr, message, ...) { if (!(expr)) { ::REV::PrintDebugMessage(__FILE__, __LINE__, message,   __VA_ARGS__); ExitProcess(1); } }
    #define REV_CHECK(expr)                 { if (!(expr)) { ::REV::PrintDebugMessage(__FILE__, __LINE__, REV_CSTR(expr)        ); ExitProcess(1); } }
    #define REV_FAILED_M(message, ...)      {                ::REV::PrintDebugMessage(__FILE__, __LINE__, message,   __VA_ARGS__); ExitProcess(1);   }
    #define REV_FAILED()                    {                ::REV::PrintDebugMessage(__FILE__, __LINE__, null                  ); ExitProcess(1);   }

    #define REV_DEBUG_RESULT(expr)                 REV_CHECK(expr)
    #define REV_DEBUG_RESULT_M(expr, message, ...) REV_CHECK_M(expr, message, __VA_ARGS__)

#else

    #define REV_CHECK_M(expr, message, ...)
    #define REV_CHECK(expr)
    #define REV_FAILED_M(message, ...)      { ::REV::PrintDebugMessage(__FILE__, __LINE__, message, __VA_ARGS__); ExitProcess(1); }
    #define REV_FAILED()                    { ::REV::PrintDebugMessage(__FILE__, __LINE__, null                ); ExitProcess(1); }

    #define REV_DEBUG_RESULT(expr)                 { expr }
    #define REV_DEBUG_RESULT_M(expr, message, ...) { expr }

#endif

}
