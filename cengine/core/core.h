//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/pch.h"

#if CENGINE_ISA >= CENGINE_ISA_AVX512
    #define CENGINE_DEFAULT_ALIGNMENT sizeof(__m512)
#elif CENGINE_ISA >= CENGINE_ISA_AVX
    #define CENGINE_DEFAULT_ALIGNMENT sizeof(__m256)
#elif CENGINE_ISA >= CENGINE_ISA_SSE
    #define CENGINE_DEFAULT_ALIGNMENT sizeof(__m128)
#else
    #define CENGINE_DEFAULT_ALIGNMENT sizeof(void *)
#endif

#if CENGINE_ISA >= CENGINE_ISA_SSE
    #pragma pack(push, 16)
#else
    #pragma pack(push, 8)
#endif

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

#if _CENGINE_DEV && DEBUG
    #define DEVDEBUG 1
#else
    #define DEVDEBUG 0
#endif

#if _CENGINE_DEV
    #define CENGINE_IMPEXP CENGINE_EXPORT
#else
    #define CENGINE_IMPEXP CENGINE_IMPORT
#endif

#ifdef __cplusplus
    #define CENGINE_EXTERN extern "C"
#else
    #define CENGINE_EXTERN extern
#endif

#define CENGINE_FUN   CENGINE_IMPEXP CENGINE_EXTERN
#define CENGINE_DATA  CENGINE_IMPEXP CENGINE_EXTERN
#define CENGINE_CLASS CENGINE_IMPEXP

#if CENGINE_ISA >= CENGINE_ISA_SSE
    #define MATH_CALL CENGINE_VECTOR_CALL
#else
    #define MATH_CALL
#endif

#if DEVDEBUG
    #define INLINE CENGINE_NOINLINE inline
#else
    #define INLINE CENGINE_INLINE
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
    #undef  NULL
#endif
#define NULL 0
#ifndef __cplusplus
    #define nullptr 0
#endif
#define null nullptr

#ifndef interface
    #ifdef __cplusplus
        #define interface __interface
    #else
        #define interface struct
    #endif
#endif

#define KB(x) (  (x) * 1024)
#define MB(x) (KB(x) * 1024)
#define GB(x) (MB(x) * 1024)
#define TB(x) (GB(x) * 1024)

#define BIT(x) (1 << (x))

#define cast(type, x) ((type)(x))

#define ArrayCount(arr)                     (sizeof(arr) / sizeof(*(arr)))
#define ArrayCountInStruct(_struct, _field) ArrayCount(cast(_struct *, null)->_field)
#define StructFieldSize(_struct, _field)    sizeof(cast(_struct *, null)->_field)
#define StructFieldOffset(_struct, _field)  cast(u64, &(cast(_struct *, null)->_field))

#define ALIGN_UP(x, a)   (((x) + ((a) - 1)) & ~((a) - 1))
#define ALIGN_DOWN(x, a) ( (x)              & ~((a) - 1))
#define IS_POW_2(x)      ((x) && (((x) & ((x) - 1)) == 0))

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

typedef s32 b32;

typedef u8 byte;

#ifndef __cplusplus
    #define true  1
    #define false 0
#endif

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

//
// Debuging
//

typedef enum DEBUG_IN
{
    DEBUG_IN_CONSOLE = 0x1,
    DEBUG_IN_DEBUG   = 0x2,
} DEBUG_IN;

typedef enum MESSAGE_TYPE
{
    MESSAGE_TYPE_ERROR   = MB_ICONERROR,
    MESSAGE_TYPE_WARNING = MB_ICONWARNING,
    MESSAGE_TYPE_INFO    = MB_ICONINFORMATION,
} MESSAGE_TYPE;

CENGINE_FUN void __cdecl DebugF(DEBUG_IN debug_in, const char *format, ...);
CENGINE_FUN void __cdecl MessageF(MESSAGE_TYPE type, const char *format, ...);
CENGINE_FUN void ShowDebugMessage(
    IN       b32         message_is_expr,
    IN       const char *file,
    IN       u64         line,
    IN       const char *function,
    IN       const char *title,
    IN       const char *format,
    OPTIONAL ...
);

#if DEVDEBUG

    #define CheckM(expr, message, ...) if (!(expr)) { ShowDebugMessage(false, __FILE__, __LINE__, __FUNCSIG__, "Debug Error", message,   __VA_ARGS__); __debugbreak(); ExitProcess(1); }
    #define Check(expr)                if (!(expr)) { ShowDebugMessage(true,  __FILE__, __LINE__, __FUNCSIG__, "Debug Error", CSTR(expr)            ); __debugbreak(); ExitProcess(1); }
    #define FailedM(message, ...)                   { ShowDebugMessage(false, __FILE__, __LINE__, __FUNCSIG__, "Failed",      message,   __VA_ARGS__); __debugbreak(); ExitProcess(1); }

    #define DebugResult(expr)                Check(expr)
    #define DebugResultM(expr, message, ...) CheckM(expr, message, __VA_ARGS__)

#elif DEBUG

    #define CheckM(expr, message, ...) if (!(expr)) { ShowDebugMessage(false, __FILE__, __LINE__, __FUNCSIG__, "Debug Error", message,   __VA_ARGS__); ExitProcess(1); }
    #define Check(expr)                if (!(expr)) { ShowDebugMessage(true,  __FILE__, __LINE__, __FUNCSIG__, "Debug Error", CSTR(expr)            ); ExitProcess(1); }
    #define FailedM(message, ...)                   { ShowDebugMessage(false, __FILE__, __LINE__, __FUNCSIG__, "Failed",      message,   __VA_ARGS__); ExitProcess(1); }

    #define DebugResult(expr)                Check(expr)
    #define DebugResultM(expr, message, ...) CheckM(expr, message, __VA_ARGS__)

#else

    #define CheckM(expr, message, ...)
    #define Check(expr)
    #define FailedM(message, ...)      { ShowDebugMessage(false, __FILE__, __LINE__, __FUNCSIG__, "Failed", message, __VA_ARGS__); ExitProcess(1); }

    #define DebugResult(expr)                expr
    #define DebugResultM(expr, message, ...) expr

#endif

#ifdef SUCCEEDED
    #undef SUCCEEDED
#endif
#define SUCCEEDED(hr) ((hr) >= 0)

#ifdef FAILED
    #undef FAILED
#endif
#define FAILED(hr) ((hr) < 0)

//
// Global typedefs
//

typedef struct Engine Engine;

//
// Utils
//

// @NOTE(Roman): annotation for lists
#define LIST

// Declaration examples:
//
// typedef struct <struct_name> <struct_name>;
// struct <struct_name>
// {
//     Extends[D]List(<struct_name>);
//     ... // other fields
// };
#define ExtendsList(Type)              Type *next
#define ExtendsDList(Type) Type *prev; Type *next
