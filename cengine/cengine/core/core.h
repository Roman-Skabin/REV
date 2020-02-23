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

#ifdef __cplusplus
    #define CEXTERN extern "C"
#else
    #define CEXTERN extern
#endif

#define MATH_CALL __vectorcall

#if DEVDEBUG
    #define INLINE __declspec(noinline) inline
#else
    #define INLINE __forceinline
#endif

#define global   static
#define internal static
#define local    static

#define _CSTR(x) #x 
#define CSTR(x)  _CSTR(x)

#define _CSTRCAT(a, b) a ## b
#define CSTRCAT(a, b)  _CSTRCAT(a, b)

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

#define KB(x) (  (x) * 1024)
#define MB(x) (KB(x) * 1024)
#define GB(x) (MB(x) * 1024)
#define TB(x) (GB(x) * 1024)

#define BIT(x) (1 << (x))

#define cast(type, x) ((type)(x))

#define ArrayCount(arr) (sizeof(arr) / sizeof(*(arr)))

#define QPF(s64_val) QueryPerformanceFrequency(cast(LARGE_INTEGER *, &(s64_val)))
#define QPC(s64_val) QueryPerformanceCounter(cast(LARGE_INTEGER *, &(s64_val)))

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

//
// Debuging
//

CEXTERN void __cdecl DebugF(const char *const format, ...);

#if DEVDEBUG

    #define DebugStringM(message)                              \
        CSTRCAT(CSTRCAT(CSTRCAT("\nMESSAGE: ", message),       \
                        CSTRCAT("\nFILE: ", __FILE__)),        \
                CSTRCAT(CSTRCAT("\nLINE: ", CSTR(__LINE__)),   \
                        CSTRCAT("\nFUNCTION: ", __FUNCSIG__)))

    #define DebugString(expr)                                  \
        CSTRCAT(CSTRCAT(CSTRCAT("EXPRESSION: ", CSTR(expr)),   \
                        CSTRCAT("\nFILE: ", __FILE__)),        \
                CSTRCAT(CSTRCAT("\nLINE: ", CSTR(__LINE__)),   \
                        CSTRCAT("\nFUNCTION: ", __FUNCSIG__)))

    #define CheckM(expr, message) if (!(expr)) { MessageBoxA(0, DebugStringM(message), "Debug Error!", MB_OK | MB_ICONERROR); __debugbreak(); ExitProcess(1); }
    #define Check(expr)           if (!(expr)) { MessageBoxA(0, DebugString(expr),     "Debug Error!", MB_OK | MB_ICONERROR); __debugbreak(); ExitProcess(1); }
    #define FailedM(message)                   { MessageBoxA(0, DebugStringM(message), "Failed!",      MB_OK | MB_ICONERROR); __debugbreak(); ExitProcess(1); }

    #define DebugResult(ResultType, expr)           { ResultType debug_result = (expr); Check(debug_result);           }
    #define DebugResultM(ResultType, expr, message) { ResultType debug_result = (expr); CheckM(debug_result, message); }

#elif DEBUG

    #define DebugStringM(message)                              \
        CSTRCAT(CSTRCAT(CSTRCAT("\nMESSAGE: ", message),       \
                        CSTRCAT("\nFILE: ", __FILE__)),        \
                CSTRCAT(CSTRCAT("\nLINE: ", CSTR(__LINE__)),   \
                        CSTRCAT("\nFUNCTION: ", __FUNCSIG__)))

    #define DebugString(expr)                                  \
        CSTRCAT(CSTRCAT(CSTRCAT("EXPRESSION: ", CSTR(expr)),   \
                        CSTRCAT("\nFILE: ", __FILE__)),        \
                CSTRCAT(CSTRCAT("\nLINE: ", CSTR(__LINE__)),   \
                        CSTRCAT("\nFUNCTION: ", __FUNCSIG__)))

    #define CheckM(expr, message) if (!(expr)) { MessageBoxA(0, DebugStringM(message), "Debug Error!", MB_OK | MB_ICONERROR); ExitProcess(1); }
    #define Check(expr)           if (!(expr)) { MessageBoxA(0, DebugString(expr),     "Debug Error!", MB_OK | MB_ICONERROR); ExitProcess(1); }
    #define FailedM(message)                   { MessageBoxA(0, DebugStringM(message), "Failed!",      MB_OK | MB_ICONERROR); ExitProcess(1); }

    #define DebugResult(ResultType, expr)           { ResultType debug_result = (expr); Check(debug_result);           }
    #define DebugResultM(ResultType, expr, message) { ResultType debug_result = (expr); CheckM(debug_result, message); }

#else

    #define DebugStringM(message)                              \
        CSTRCAT(CSTRCAT(CSTRCAT("\nMESSAGE: ", message),       \
                        CSTRCAT("\nFILE: ", __FILE__)),        \
                CSTRCAT(CSTRCAT("\nLINE: ", CSTR(__LINE__)),   \
                        CSTRCAT("\nFUNCTION: ", __FUNCSIG__)))

    #define CheckM(expr, message)
    #define Check(expr)
    #define FailedM(message)      { MessageBoxA(0, DebugStringM(message), "Failed!", MB_OK | MB_ICONERROR); ExitProcess(1); }

    #define DebugResult(ResultType, expr)           { (expr); }
    #define DebugResultM(ResultType, expr, message) { (expr); }

#endif

//
// General Purpose Allocator
//

#if DEBUG
    extern u64 gAllocationsPerFrame;
    extern u64 gReAllocationsPerFrame;
    extern u64 gDeAllocationsPerFrame;
#endif

#define ALIGN_UP(x, a)   (((x) + ((a) - 1)) & ~((a) - 1))
#define ALIGN_DOWN(x, a) ( (x)              & ~((a) - 1))
#define IS_POW_2(x)      ((x) && (((x) & ((x)-1)) == 0))

CEXTERN void *  Allocate(u64 bytes);
CEXTERN void *ReAllocate(void *mem, u64 bytes);
CEXTERN void  DeAllocate(void **mem);

CEXTERN void *  AllocateAligned(u64 bytes, u64 alignment);
CEXTERN void *ReAllocateAligned(void *mem, u64 bytes, u64 alignment);
CEXTERN void  DeAllocateAligned(void **mem);

#define   Alloc(Type, count)      cast(Type *, Allocate(sizeof(Type) * (count)))
#define ReAlloc(Type, mem, count) cast(Type *, ReAllocate(mem, sizeof(Type) * (count)))
#define DeAlloc(mem)              DeAllocate(&(mem))

#define   AllocA(Type, count, alignment)      cast(Type *, AllocateAligned(sizeof(Type) * (count), alignment))
#define ReAllocA(Type, mem, count, alignment) cast(Type *, ReAllocateAligned(mem, sizeof(Type) * (count), alignment))
#define DeAllocA(mem)                         DeAllocateAligned(&(mem))

//
// Stretchy Buffers
//

// @NOTE(Roman): annotation, means a variable is a stretchy buffer.
#define BUF

// @NOTE(Roman): annotation, means a variable is a stretchy buffer
//               and it already pushed to the Memory (you can't resize it).
//               Kinda "achieved" buffer.
#define PUSHED_BUF
#define ACHIEVED_BUF PUSHED_BUF

typedef struct BufHdr
{
    u64 count;
    u64 cap;
    u8  buf[0];
} BufHdr;

#define _BUFHDR(b) cast(BufHdr *, cast(u8 *, (b)) - offsetof(BufHdr, buf))

#define buf_free(b) ((b) ? buf_dealloc(b) : 0)

#define buf_count(b) ((b) ? _BUFHDR(b)->count : 0)
#define buf_cap(b)   ((b) ? _BUFHDR(b)->cap   : 0)

#define buf_push(b, el) ((b) = buf_grow(b, buf_count(b) + 1, sizeof(*(b))), (b)[_BUFHDR(b)->count++] = (el))
#define buf_pop(b)      ((b) && buf_count(b) ? (b)[--(_BUFHDR(b)->count)] : 0)

CEXTERN void buf_dealloc(BUF void *b);
CEXTERN BUF void *buf_grow(BUF void *b, u64 new_count, u64 el_size);
