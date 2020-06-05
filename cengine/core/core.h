//
// Copyright 2020 Roman Skabin
//

#pragma once

#pragma pack(push, 8)

#include "core/pch.h"

#ifndef _WIN64
    #error Only for 64-bit systems
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

#define MMX    0
#define SSE    1
#define AVX    2
#define AVX2   3
#define AVX512 4

#if __AVX512BW__ || __AVX512CD__ || __AVX512DQ__ || __AVX512F__ || __AVX512VL__
    #define ISA AVX512
    #include <zmmintrin.h>
    #include <immintrin.h>
    #include <ammintrin.h>
#elif __AVX2__
    #define ISA AVX2
    #include <immintrin.h>
    #include <ammintrin.h>
#elif __AVX__
    #define ISA AVX
    #include <immintrin.h>
    #include <ammintrin.h>
#elif _M_IX86_FP > 0
    #define ISA SSE
    #include <wmmintrin.h>
#else
    #define ISA MMX
    #include <mmintrin.h>
#endif

#ifdef __cplusplus
    #define CEXTERN extern "C"
#else
    #define CEXTERN extern
#endif

#if _CENGINE_DEV
    #define CENGINE_IMPEXP __declspec(dllexport)
#else
    #define CENGINE_IMPEXP __declspec(dllimport)
#endif

#define CENGINE_FUN  CENGINE_IMPEXP CEXTERN
#define CENGINE_DATA CENGINE_IMPEXP CEXTERN

#define MATH_CALL __vectorcall

#if DEVDEBUG
    #define INLINE __declspec(noinline) inline
#else
    #define INLINE __forceinline
#endif

#define __align(bytes) __declspec(align(bytes))
#define __intrin_type __declspec(intrin_type)

#define global   static
#define internal static
#define local    static

#define _CSTR(x) #x 
#define CSTR(x)  _CSTR(x)

#define _CSTRCAT(a, b) a ## b
#define CSTRCAT(a, b)  _CSTRCAT(a, b)

#define CSTRLEN(cstr) (sizeof(cstr) - 1)

// No OOP bullshit in this engine!!!
#define class       "No OOP bullshit in this engine!!!"
#define virtual     "No OOP bullshit in this engine!!!"
#define private     "No OOP bullshit in this engine!!!"
#define public      "No OOP bullshit in this engine!!!"
#define protected   "No OOP bullshit in this engine!!!"
#define this        "No OOP bullshit in this engine!!!"
#define __interface "No OOP bullshit in this engine!!!"

#ifdef NULL
    #undef  NULL
    #define NULL 0
#endif

#ifndef __cplusplus
    #define nullptr 0
#endif

#define KB(x) (  (x) * 1024)
#define MB(x) (KB(x) * 1024)
#define GB(x) (MB(x) * 1024)
#define TB(x) (GB(x) * 1024)

#define BIT(x) (1 << (x))

#define cast(type, x) ((type)(x))

#define ArrayCount(arr) (sizeof(arr) / sizeof(*(arr)))

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

#if DEVDEBUG

    #define DebugStringM(message)                                 \
        CSTRCAT(CSTRCAT(CSTRCAT("MESSAGE:\n", message),           \
                        CSTRCAT("\n\nFILE:\n", __FILE__)),        \
                CSTRCAT(CSTRCAT("\n\nLINE:\n", CSTR(__LINE__)),   \
                        CSTRCAT("\n\nFUNCTION:\n", __FUNCSIG__)))

    #define DebugString(expr)                                     \
        CSTRCAT(CSTRCAT(CSTRCAT("EXPRESSION:\n", CSTR(expr)),     \
                        CSTRCAT("\n\nFILE:\n", __FILE__)),        \
                CSTRCAT(CSTRCAT("\n\nLINE:\n", CSTR(__LINE__)),   \
                        CSTRCAT("\n\nFUNCTION:\n", __FUNCSIG__)))

    #define CheckM(expr, message) if (!(expr)) { MessageBoxA(0, DebugStringM(message), "Debug Error!", MB_OK | MB_ICONERROR); __debugbreak(); ExitProcess(1); }
    #define Check(expr)           if (!(expr)) { MessageBoxA(0, DebugString(expr),     "Debug Error!", MB_OK | MB_ICONERROR); __debugbreak(); ExitProcess(1); }
    #define FailedM(message)                   { MessageBoxA(0, DebugStringM(message), "Failed!",      MB_OK | MB_ICONERROR); __debugbreak(); ExitProcess(1); }

    #define DebugResult(expr)           Check(expr)
    #define DebugResultM(expr, message) CheckM(expr, message)

#elif DEBUG

    #define DebugStringM(message)                                 \
        CSTRCAT(CSTRCAT(CSTRCAT("MESSAGE:\n", message),           \
                        CSTRCAT("\n\nFILE:\n", __FILE__)),        \
                CSTRCAT(CSTRCAT("\n\nLINE:\n", CSTR(__LINE__)),   \
                        CSTRCAT("\n\nFUNCTION:\n", __FUNCSIG__)))

    #define DebugString(expr)                                     \
        CSTRCAT(CSTRCAT(CSTRCAT("EXPRESSION:\n", CSTR(expr)),     \
                        CSTRCAT("\n\nFILE:\n", __FILE__)),        \
                CSTRCAT(CSTRCAT("\n\nLINE:\n", CSTR(__LINE__)),   \
                        CSTRCAT("\n\nFUNCTION:\n", __FUNCSIG__)))

    #define CheckM(expr, message) if (!(expr)) { MessageBoxA(0, DebugStringM(message), "Debug Error!", MB_OK | MB_ICONERROR); ExitProcess(1); }
    #define Check(expr)           if (!(expr)) { MessageBoxA(0, DebugString(expr),     "Debug Error!", MB_OK | MB_ICONERROR); ExitProcess(1); }
    #define FailedM(message)                   { MessageBoxA(0, DebugStringM(message), "Failed!",      MB_OK | MB_ICONERROR); ExitProcess(1); }

    #define DebugResult(expr)           Check(expr)
    #define DebugResultM(expr, message) CheckM(expr, message)

#else

    #define DebugStringM(message)                                 \
        CSTRCAT(CSTRCAT(CSTRCAT("MESSAGE:\n", message),           \
                        CSTRCAT("\n\nFILE:\n", __FILE__)),        \
                CSTRCAT(CSTRCAT("\n\nLINE:\n", CSTR(__LINE__)),   \
                        CSTRCAT("\n\nFUNCTION:\n", __FUNCSIG__)))

    #define CheckM(expr, message)
    #define Check(expr)
    #define FailedM(message)      { MessageBoxA(0, DebugStringM(message), "Failed!", MB_OK | MB_ICONERROR); ExitProcess(1); }

    #define DebugResult(expr)           expr
    #define DebugResultM(expr, message) expr

#endif

//
// Global Defines
//

typedef struct Engine Engine;
