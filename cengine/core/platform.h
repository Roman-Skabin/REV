//
// Copyright 2020 Roman Skabin
//

#pragma once

#pragma warning(push)
#pragma warning(disable: 4005)

//
// Compiler
//

#define CENGINE_COMPILER_MSVC  0
#define CENGINE_COMPILER_GCC   0
#define CENGINE_COMPILER_CLANG 0
#define CENGINE_COMPILER_MINGW 0

#if defined(_MSC_VER)
    #define CENGINE_COMPILER_MSVC  1
#elif defined(__GNUC__)
    #define CENGINE_COMPILER_GCC   1
#elif defined(__clang__)
    #define CENGINE_COMPILER_CLANG 1
#elif defined(__MINGW64__)
    #define CENGINE_COMPILER_MINGW 1
#elif defined(__MINGW32__)
    #error Only 64-bit systems (=> and compilers) supported!
#else
    #error Unsupported compiler!
#endif

#if !CENGINE_COMPILER_MSVC
    #error Currently only MSVC compiler supported!
#endif

//
// Platform
//

#ifndef _WIN64
    #error Only 64-bit systems supported!
#endif

//
// ISA
//

#define CENGINE_ISA_SSE      0x10
#define CENGINE_ISA_SSE2     0x20
#define CENGINE_ISA_SSE3     0x30
#define CENGINE_ISA_SSSE3    0x40
#define CENGINE_ISA_SSE4     0x50
#define CENGINE_ISA_SSE4a    0x60
#define CENGINE_ISA_SSE4_1   0x70
#define CENGINE_ISA_SSE4_2   0x80
#define CENGINE_ISA_AVX      0x90
#define CENGINE_ISA_AVX2     0xA0
#define CENGINE_ISA_AVX512   0xB0

// @NOTE(Roman): OR'd with the CENGINE_ISA_AVX512
#define CENGINE_ISA_AVX512F  0x00
#define CENGINE_ISA_AVX512CD 0x01
#define CENGINE_ISA_AVX512VL 0x02
#define CENGINE_ISA_AVX512DQ 0x04
#define CENGINE_ISA_AVX512BW 0x08

#define CENGINE_ISA 0

#if CENGINE_COMPILER_MSVC
    #if defined(__AVX512BW__) || defined(__AVX512CD__) || defined(__AVX512DQ__) || defined(__AVX512F__) || defined(__AVX512VL__)
        #define CENGINE_ISA CENGINE_ISA_AVX512
        #if defined(__AVX512F__)
            #define CENGINE_ISA (CENGINE_ISA | CENGINE_ISA_AVX512F)
        #endif
        #if defined(__AVX512CD__)
            #define CENGINE_ISA (CENGINE_ISA | CENGINE_ISA_AVX512CD)
        #endif
        #if defined(__AVX512VL__)
            #define CENGINE_ISA (CENGINE_ISA | CENGINE_ISA_AVX512VL)
        #endif
        #if defined(__AVX512DQ__)
            #define CENGINE_ISA (CENGINE_ISA | CENGINE_ISA_AVX512DQ)
        #endif
        #if defined(__AVX512BW__)
            #define CENGINE_ISA (CENGINE_ISA | CENGINE_ISA_AVX512BW)
        #endif
    #elif defined(__AVX2__)
        #define CENGINE_ISA CENGINE_ISA_AVX2
    #elif defined(__AVX__)
        #define CENGINE_ISA CENGINE_ISA_AVX
    #elif _M_IX86_FP == 2
        #define CENGINE_ISA CENGINE_ISA_SSE2
    #elif _M_IX86_FP == 1
        #define CENGINE_ISA CENGINE_ISA_SSE
    #else
        #error Unsupported ISA!
    #endif
#elif CENGINE_COMPILER_GCC
    // @TODO(Roman): Cross compiler ISA defines
#elif CENGINE_COMPILER_CLANG
    // @TODO(Roman): Cross compiler ISA defines
#elif CENGINE_COMPILER_MINGW
    // @TODO(Roman): Cross compiler ISA defines
#endif

//
// Import libs
//

#pragma comment(lib, "User32.lib")      // Windows
#pragma comment(lib, "Ole32.lib")       // Windows
#pragma comment(lib, "mfplat.lib")      // MFAPI
#pragma comment(lib, "mfreadwrite.lib") // MFAPI
#pragma comment(lib, "d3d12.lib")       // DirectX
#pragma comment(lib, "dxgi.lib")        // DirectX
#pragma comment(lib, "d3dcompiler.lib") // DirectX

//
// Predefines
//

#define WIN32_LEAN_AND_MEAN     1 // Windows.h
#define VC_EXTRALEAN            1 // Windows.h
#define NOMINMAX                1 // Windows.h
#define INITGUID                1 // DirectX, WASAPI, MFAPI
#define CINTERFACE              1 // DirectX
#define _CRT_SECURE_NO_WARNINGS 1 // CRT

//
// STD includes
//

#include <Windows.h>
#undef GetMessage

#include <Xinput.h>
#include <audiopolicy.h>
#include <mmdeviceapi.h>
#include <AudioClient.h>
#include <mfidl.h>
#include <mfapi.h>
#include <mfreadwrite.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <d3dcompiler.h>

#undef near
#undef far

#if CENGINE_COMPILER_MSVC
    #if CENGINE_ISA >= CENGINE_ISA_AVX512
        #include <immintrin.h>
        #include <ammintrin.h> // AMD specific intrinsics
    #elif CENGINE_ISA >= CENGINE_ISA_AVX2
        #define _ZMMINTRIN_H_INCLUDED // to suppres include zmmintrin from immintrin
        #include <immintrin.h>
        #include <ammintrin.h> // AMD specific intrinsics
    #elif CENGINE_ISA >= CENGINE_ISA_AVX
        #define _ZMMINTRIN_H_INCLUDED // to suppres include zmmintrin from immintrin
        #include <immintrin.h>
        #include <ammintrin.h> // AMD specific intrinsics
    #elif CENGINE_ISA >= CENGINE_ISA_SSE4_2
        #include <wmmintrin.h>
        #include <nmmintrin.h>
        #include <ammintrin.h> // AMD specific intrinsics
    #elif CENGINE_ISA >= CENGINE_ISA_SSE4_1
        #include <smmintrin.h>
        #include <ammintrin.h> // AMD specific intrinsics
    #elif CENGINE_ISA >= CENGINE_ISA_SSSE3
        #include <tmmintrin.h>
        #include <ammintrin.h> // AMD specific intrinsics
    #elif CENGINE_ISA >= CENGINE_ISA_SSE3
        #include <pmmintrin.h>
        #include <ammintrin.h> // AMD specific intrinsics
    #elif CENGINE_ISA >= CENGINE_ISA_SSE2
        #include <emmintrin.h>
        #include <ammintrin.h> // AMD specific intrinsics
    #elif CENGINE_ISA >= CENGINE_ISA_SSE
        #include <xmmintrin.h>
        #include <ammintrin.h> // AMD specific intrinsics
    #endif
#elif CENGINE_COMPILER_GCC
    // @TODO(Roman): Cross platform includes
#elif CENGINE_COMPILER_CLANG
    // @TODO(Roman): Cross platform includes
#elif CENGINE_COMPILER_MINGW
    // @TODO(Roman): Cross platform includes
#endif

//
// Cross compiler utils
//

#if CENGINE_COMPILER_MSVC
    #define CENGINE_EXPORT        __declspec(dllexport)
    #define CENGINE_IMPORT        __declspec(dllimport)
    #define CENGINE_NOINLINE      __declspec(noinline)
    #define CENGINE_VECTOR_CALL   __vectorcall
    #define CENGINE_INLINE        __forceinline
    #define CENGINE_ALIGN(_bytes) __declspec(align(_bytes))
    #define CENGINE_INTRIN_TYPE   __declspec(intrin_type)
    #define CENGINE_SELECTANY     __declspec(selectany)
#elif CENGINE_COMPILER_GCC || CENGINE_COMPILER_CLANG || CENGINE_COMPILER_MINGW
    #define CENGINE_EXPORT        __attribute__((dllexport))
    #define CENGINE_IMPORT        __attribute__((dllimport))
    #define CENGINE_NOINLINE      __attribute__((noinline))
    #define CENGINE_VECTOR_CALL   __vectorcall
    #define CENGINE_INLINE        inline  __attribute__((always_inline))
    #define CENGINE_ALIGN(_bytes) __attribute__((aligned(_bytes)))
    #define CENGINE_INTRIN_TYPE
    #define CENGINE_SELECTANY     __attribute__((selectany))
#endif

#pragma warning(pop)
