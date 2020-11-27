//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/pch.h"
#include "core/rtti.hpp"

#pragma warning(disable: 4251)

enum
{
#if ENGINE_ISA >= ENGINE_ISA_AVX512
    ENGINE_DEFAULT_ALIGNMENT = sizeof(__m512)
#elif ENGINE_ISA >= ENGINE_ISA_AVX
    ENGINE_DEFAULT_ALIGNMENT = sizeof(__m256)
#else
    ENGINE_DEFAULT_ALIGNMENT = sizeof(__m128)
#endif
};

#pragma pack(push, 16)

#ifdef DEBUG
    #undef DEBUG
#endif

#ifdef RELEASE
    #undef RELEASE
#endif

#ifdef _DEBUG
    #define DEBUG   1
    #define RELEASE 0
#else
    #define DEBUG   0
    #define RELEASE 1
#endif

#if _ENGINE_DEV && DEBUG
    #define DEVDEBUG 1
#else
    #define DEVDEBUG 0
#endif

#if _ENGINE_DEV
    #define ENGINE_API ENGINE_EXPORT
#else
    #define ENGINE_API ENGINE_IMPORT
#endif

#ifdef __cplusplus
    #define ENGINE_EXTERN extern "C"
#else
    #define ENGINE_EXTERN extern
#endif

#if DEVDEBUG
    #define INLINE ENGINE_NOINLINE inline
#else
    #define INLINE ENGINE_INLINE
#endif

#define global   static
#define internal static
#define local    static

#define _CSTR(x) #x 
#define CSTR(x)  _CSTR(x)

#define _CSTRCAT(a, b) a ## b
#define CSTRCAT(a, b)  _CSTRCAT(a, b)

#define CSTRLEN(cstr) (sizeof(cstr) - 1)

#ifdef NULL
    #undef NULL
#endif

#define NULL 0
#define null nullptr

#define ENGINE_INTERFACE struct ENGINE_NOVTABLE

//
// Types
//

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

typedef s8  b8;
typedef s16 b16;
typedef s32 b32;
typedef s64 b64;

typedef u8 byte;

#define S8_MIN  0x80i8
#define S16_MIN 0x8000i16
#define S32_MIN 0x80000000i32
#define S64_MIN 0x8000000000000000i64

#define S8_MAX  0x7Fi8
#define S16_MAX 0x7FFFi16
#define S32_MAX 0x7FFFFFFFi32
#define S64_MAX 0x7FFFFFFFFFFFFFFFi64

#define U8_MIN  0x00ui8
#define U16_MIN 0x0000ui16
#define U32_MIN 0x00000000ui32
#define U64_MIN 0x0000000000000000ui64

#define U8_MAX  0xFFui8
#define U16_MAX 0xFFFFui16
#define U32_MAX 0xFFFFFFFFui32
#define U64_MAX 0xFFFFFFFFFFFFFFFFui64

#define F32_MIN -3.402823466e+38f
#define F32_MAX  3.402823466e+38f

#define F64_MIN -1.7976931348623158e+308
#define F64_MAX  1.7976931348623158e+308

#define BYTE_MAX U8_MAX
#define BYTE_MIN U8_MIN

template<typename T, typename = RTTI::enable_if_t<RTTI::is_arithmetic_v<T>>> constexpr T KB(T x) { return    x  * 1024; }
template<typename T, typename = RTTI::enable_if_t<RTTI::is_arithmetic_v<T>>> constexpr T MB(T x) { return KB(x) * 1024; }
template<typename T, typename = RTTI::enable_if_t<RTTI::is_arithmetic_v<T>>> constexpr T GB(T x) { return MB(x) * 1024; }
template<typename T, typename = RTTI::enable_if_t<RTTI::is_arithmetic_v<T>>> constexpr T TB(T x) { return GB(x) * 1024; }

template<typename T, typename = RTTI::enable_if_t<RTTI::is_integral_v<T>>> constexpr T BIT(T x) { return 1 << x; }

#pragma warning(suppress: 4172)
template<typename To, typename From> constexpr To cast(From x) { return (To)x; }

template<typename T, u64 count> constexpr u64 ArrayCount(T (&)[count]) { return count; }

#define ArrayCountInStruct(_struct, _field) ArrayCount(cast<_struct *>(null)->_field)
#define StructFieldSize(_struct, _field)    sizeof(cast<_struct *>(null)->_field)
#define StructFieldOffset(_struct, _field)  cast<u64>(&(cast<_struct *>(null)->_field))

template<typename T, typename U, typename = RTTI::enable_if_t<RTTI::is_integral_v<T> && RTTI::is_integral_v<U>>>
constexpr RTTI::max_size_t<T, U> AlignUp(T x, U a = ENGINE_DEFAULT_ALIGNMENT)
{
    return cast<RTTI::max_size_t<T, U>>((x + (a - 1)) & ~(a - 1));
}

template<typename T, typename U, typename = RTTI::enable_if_t<RTTI::is_integral_v<T> && RTTI::is_integral_v<U>>>
constexpr RTTI::max_size_t<T, U> AlignDown(T x, U a = ENGINE_DEFAULT_ALIGNMENT)
{
    return cast<RTTI::max_size_t<T, U>>(x & ~(a - 1));
}

template<typename T, typename = RTTI::enable_if_t<RTTI::is_integral_v<T>>>
constexpr bool IsPowOf2(T x)
{
    return x && ((x & (x - 1)) == 0);
}

#define ENUM_CLASS_OPERATORS(ENUM_CLASS)                                                                                                                                                                                                                                                 \
                                                                               constexpr ENUM_CLASS  operator| (ENUM_CLASS  left, ENUM_CLASS right) { return cast<ENUM_CLASS >( cast<RTTI::underlying_type_t<ENUM_CLASS> >(left) |  cast<RTTI::underlying_type_t<ENUM_CLASS>>(right)); } \
                                                                               constexpr ENUM_CLASS  operator& (ENUM_CLASS  left, ENUM_CLASS right) { return cast<ENUM_CLASS >( cast<RTTI::underlying_type_t<ENUM_CLASS> >(left) &  cast<RTTI::underlying_type_t<ENUM_CLASS>>(right)); } \
                                                                               constexpr ENUM_CLASS  operator^ (ENUM_CLASS  left, ENUM_CLASS right) { return cast<ENUM_CLASS >( cast<RTTI::underlying_type_t<ENUM_CLASS> >(left) ^  cast<RTTI::underlying_type_t<ENUM_CLASS>>(right)); } \
                                                                               constexpr ENUM_CLASS  operator~ (ENUM_CLASS  left                  ) { return cast<ENUM_CLASS >(~cast<RTTI::underlying_type_t<ENUM_CLASS> >(left)                                                    ); } \
                                                                               constexpr ENUM_CLASS& operator|=(ENUM_CLASS& left, ENUM_CLASS right) { return cast<ENUM_CLASS&>( cast<RTTI::underlying_type_t<ENUM_CLASS>&>(left) |= cast<RTTI::underlying_type_t<ENUM_CLASS>>(right)); } \
                                                                               constexpr ENUM_CLASS& operator&=(ENUM_CLASS& left, ENUM_CLASS right) { return cast<ENUM_CLASS&>( cast<RTTI::underlying_type_t<ENUM_CLASS>&>(left) &= cast<RTTI::underlying_type_t<ENUM_CLASS>>(right)); } \
                                                                               constexpr ENUM_CLASS& operator^=(ENUM_CLASS& left, ENUM_CLASS right) { return cast<ENUM_CLASS&>( cast<RTTI::underlying_type_t<ENUM_CLASS>&>(left) ^= cast<RTTI::underlying_type_t<ENUM_CLASS>>(right)); } \
    template<typename T, typename = RTTI::enable_if_t<RTTI::is_integral_v<T>>> constexpr ENUM_CLASS  operator| (ENUM_CLASS  left, T          right) { return cast<ENUM_CLASS >( cast<RTTI::underlying_type_t<ENUM_CLASS> >(left) |  cast<RTTI::underlying_type_t<ENUM_CLASS>>(right)); } \
    template<typename T, typename = RTTI::enable_if_t<RTTI::is_integral_v<T>>> constexpr ENUM_CLASS  operator| (T           left, ENUM_CLASS right) { return cast<ENUM_CLASS >( cast<RTTI::underlying_type_t<ENUM_CLASS> >(left) |  cast<RTTI::underlying_type_t<ENUM_CLASS>>(right)); } \
    template<typename T, typename = RTTI::enable_if_t<RTTI::is_integral_v<T>>> constexpr ENUM_CLASS  operator& (ENUM_CLASS  left, T          right) { return cast<ENUM_CLASS >( cast<RTTI::underlying_type_t<ENUM_CLASS> >(left) &  cast<RTTI::underlying_type_t<ENUM_CLASS>>(right)); } \
    template<typename T, typename = RTTI::enable_if_t<RTTI::is_integral_v<T>>> constexpr ENUM_CLASS  operator& (T           left, ENUM_CLASS right) { return cast<ENUM_CLASS >( cast<RTTI::underlying_type_t<ENUM_CLASS> >(left) &  cast<RTTI::underlying_type_t<ENUM_CLASS>>(right)); } \
    template<typename T, typename = RTTI::enable_if_t<RTTI::is_integral_v<T>>> constexpr ENUM_CLASS  operator^ (ENUM_CLASS  left, T          right) { return cast<ENUM_CLASS >( cast<RTTI::underlying_type_t<ENUM_CLASS> >(left) ^  cast<RTTI::underlying_type_t<ENUM_CLASS>>(right)); } \
    template<typename T, typename = RTTI::enable_if_t<RTTI::is_integral_v<T>>> constexpr ENUM_CLASS  operator^ (T           left, ENUM_CLASS right) { return cast<ENUM_CLASS >( cast<RTTI::underlying_type_t<ENUM_CLASS> >(left) ^  cast<RTTI::underlying_type_t<ENUM_CLASS>>(right)); } \
    template<typename T, typename = RTTI::enable_if_t<RTTI::is_integral_v<T>>> constexpr ENUM_CLASS& operator|=(ENUM_CLASS& left, T          right) { return cast<ENUM_CLASS&>( cast<RTTI::underlying_type_t<ENUM_CLASS>&>(left) |= cast<RTTI::underlying_type_t<ENUM_CLASS>>(right)); } \
    template<typename T, typename = RTTI::enable_if_t<RTTI::is_integral_v<T>>> constexpr ENUM_CLASS& operator|=(T&          left, ENUM_CLASS right) { return cast<ENUM_CLASS&>( cast<RTTI::underlying_type_t<ENUM_CLASS>&>(left) |= cast<RTTI::underlying_type_t<ENUM_CLASS>>(right)); } \
    template<typename T, typename = RTTI::enable_if_t<RTTI::is_integral_v<T>>> constexpr ENUM_CLASS& operator&=(ENUM_CLASS& left, T          right) { return cast<ENUM_CLASS&>( cast<RTTI::underlying_type_t<ENUM_CLASS>&>(left) &= cast<RTTI::underlying_type_t<ENUM_CLASS>>(right)); } \
    template<typename T, typename = RTTI::enable_if_t<RTTI::is_integral_v<T>>> constexpr ENUM_CLASS& operator&=(T&          left, ENUM_CLASS right) { return cast<ENUM_CLASS&>( cast<RTTI::underlying_type_t<ENUM_CLASS>&>(left) &= cast<RTTI::underlying_type_t<ENUM_CLASS>>(right)); } \
    template<typename T, typename = RTTI::enable_if_t<RTTI::is_integral_v<T>>> constexpr ENUM_CLASS& operator^=(ENUM_CLASS& left, T          right) { return cast<ENUM_CLASS&>( cast<RTTI::underlying_type_t<ENUM_CLASS>&>(left) ^= cast<RTTI::underlying_type_t<ENUM_CLASS>>(right)); } \
    template<typename T, typename = RTTI::enable_if_t<RTTI::is_integral_v<T>>> constexpr ENUM_CLASS& operator^=(T&          left, ENUM_CLASS right) { return cast<ENUM_CLASS&>( cast<RTTI::underlying_type_t<ENUM_CLASS>&>(left) ^= cast<RTTI::underlying_type_t<ENUM_CLASS>>(right)); }

//
// Debugging
//

enum class DEBUG_IN
{
    CONSOLE = BIT(0),
    WINDBG  = BIT(1),
};
ENUM_CLASS_OPERATORS(DEBUG_IN)

#define WIN32_ERROR ERROR
#undef ERROR

enum class MESSAGE_TYPE
{
    ERROR   = MB_ICONERROR,
    WARNING = MB_ICONWARNING,
    INFO    = MB_ICONINFORMATION,
};
ENUM_CLASS_OPERATORS(MESSAGE_TYPE)

enum class DEBUG_COLOR : u16
{
    INFO    = 0x7,
    ERROR   = 0x4,
    WARNING = 0x6,
    SUCCESS = 0xA,
};

ENGINE_API void __cdecl DebugF(DEBUG_IN debug_in, const char *format, ...);
ENGINE_API void __cdecl DebugFC(DEBUG_IN debug_in, DEBUG_COLOR color, const char *format, ...);
ENGINE_API void __cdecl MessageF(MESSAGE_TYPE type, const char *format, ...);
ENGINE_API void __cdecl ShowDebugMessage(
    b32         message_is_expr,
    const char *file,
    u64         line,
    const char *function,
    const char *title,
    const char *format,
    ...
);

#if (DEVDEBUG || defined(_ENGINE_CHECKS_BREAK)) && !defined(_ENGINE_NO_CHECKS)

    #define CheckM(expr, message, ...) { if (!(expr)) { ShowDebugMessage(false, __FILE__, __LINE__, __FUNCSIG__, "Debug Error", message,   __VA_ARGS__); __debugbreak(); ExitProcess(1); } }
    #define Check(expr)                { if (!(expr)) { ShowDebugMessage(true,  __FILE__, __LINE__, __FUNCSIG__, "Debug Error", CSTR(expr)            ); __debugbreak(); ExitProcess(1); } }
    #define FailedM(message, ...)      {                ShowDebugMessage(false, __FILE__, __LINE__, __FUNCSIG__, "Failed",      message,   __VA_ARGS__); __debugbreak(); ExitProcess(1);   }

    #define DebugResult(expr)                Check(expr)
    #define DebugResultM(expr, message, ...) CheckM(expr, message, __VA_ARGS__)

#elif !defined(_ENGINE_NO_CHECKS)

    #define CheckM(expr, message, ...) { if (!(expr)) { ShowDebugMessage(false, __FILE__, __LINE__, __FUNCSIG__, "Debug Error", message,   __VA_ARGS__); ExitProcess(1); } }
    #define Check(expr)                { if (!(expr)) { ShowDebugMessage(true,  __FILE__, __LINE__, __FUNCSIG__, "Debug Error", CSTR(expr)            ); ExitProcess(1); } }
    #define FailedM(message, ...)      {                ShowDebugMessage(false, __FILE__, __LINE__, __FUNCSIG__, "Failed",      message,   __VA_ARGS__); ExitProcess(1);   }

    #define DebugResult(expr)                Check(expr)
    #define DebugResultM(expr, message, ...) CheckM(expr, message, __VA_ARGS__)

#else

    #define CheckM(expr, message, ...)
    #define Check(expr)
    #define FailedM(message, ...)      { ShowDebugMessage(false, __FILE__, __LINE__, __FUNCSIG__, "Failed", message, __VA_ARGS__); ExitProcess(1); }

    #define DebugResult(expr)                { expr }
    #define DebugResultM(expr, message, ...) { expr }

#endif
