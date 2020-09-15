//
// Copyright 2020 Roman Skabin
//

#pragma once

#pragma warning(push)
#pragma warning(disable: 4005)

//
// Compiler
//

#define ENGINE_COMPILER_MSVC  0
#define ENGINE_COMPILER_GCC   0
#define ENGINE_COMPILER_CLANG 0
#define ENGINE_COMPILER_MINGW 0

#if defined(_MSC_VER)
    #define ENGINE_COMPILER_MSVC  1
#elif defined(__GNUC__)
    #define ENGINE_COMPILER_GCC   1
#elif defined(__clang__)
    #define ENGINE_COMPILER_CLANG 1
#elif defined(__MINGW64__)
    #define ENGINE_COMPILER_MINGW 1
#elif defined(__MINGW32__)
    #error Only 64-bit systems (=> and compilers) are supported!
#else
    #error Unsupported compiler!
#endif

#if !ENGINE_COMPILER_MSVC
    #error Currently only MSVC compiler is supported!
#endif

//
// Platform
//

#ifndef _WIN64
    #error Only 64-bit systems are supported!
#endif

//
// ISA
//

#define ENGINE_ISA_SSE      0x10
#define ENGINE_ISA_SSE2     0x20
#define ENGINE_ISA_SSE3     0x30
#define ENGINE_ISA_SSSE3    0x40
#define ENGINE_ISA_SSE4     0x50
#define ENGINE_ISA_SSE4a    0x60
#define ENGINE_ISA_SSE4_1   0x70
#define ENGINE_ISA_SSE4_2   0x80
#define ENGINE_ISA_AVX      0x90
#define ENGINE_ISA_AVX2     0xA0
#define ENGINE_ISA_AVX512   0xB0

// @NOTE(Roman): OR'd with the ENGINE_ISA_AVX512
#define ENGINE_ISA_AVX512F  0x00
#define ENGINE_ISA_AVX512CD 0x01
#define ENGINE_ISA_AVX512VL 0x02
#define ENGINE_ISA_AVX512DQ 0x04
#define ENGINE_ISA_AVX512BW 0x08

#define ENGINE_ISA 0

#if ENGINE_COMPILER_MSVC
    #if defined(__AVX512BW__) || defined(__AVX512CD__) || defined(__AVX512DQ__) || defined(__AVX512F__) || defined(__AVX512VL__)
        #define ENGINE_ISA ENGINE_ISA_AVX512
        #if defined(__AVX512F__)
            #define ENGINE_ISA (ENGINE_ISA | ENGINE_ISA_AVX512F)
        #endif
        #if defined(__AVX512CD__)
            #define ENGINE_ISA (ENGINE_ISA | ENGINE_ISA_AVX512CD)
        #endif
        #if defined(__AVX512VL__)
            #define ENGINE_ISA (ENGINE_ISA | ENGINE_ISA_AVX512VL)
        #endif
        #if defined(__AVX512DQ__)
            #define ENGINE_ISA (ENGINE_ISA | ENGINE_ISA_AVX512DQ)
        #endif
        #if defined(__AVX512BW__)
            #define ENGINE_ISA (ENGINE_ISA | ENGINE_ISA_AVX512BW)
        #endif
    #elif defined(__AVX2__)
        #define ENGINE_ISA ENGINE_ISA_AVX2
    #elif defined(__AVX__)
        #define ENGINE_ISA ENGINE_ISA_AVX
    #elif _M_IX86_FP == 2
        #define ENGINE_ISA ENGINE_ISA_SSE2
    #elif _M_IX86_FP == 1
        #define ENGINE_ISA ENGINE_ISA_SSE
    #else
        #error Unsupported ISA!
    #endif
#elif ENGINE_COMPILER_GCC
    // @TODO(Roman): Cross compiler ISA defines
#elif ENGINE_COMPILER_CLANG
    // @TODO(Roman): Cross compiler ISA defines
#elif ENGINE_COMPILER_MINGW
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

#if ENGINE_COMPILER_MSVC
    #if ENGINE_ISA >= ENGINE_ISA_AVX512
        #include <immintrin.h>
        #include <ammintrin.h> // AMD specific intrinsics
    #elif ENGINE_ISA >= ENGINE_ISA_AVX2
        #define _ZMMINTRIN_H_INCLUDED // to suppres include zmmintrin from immintrin
        #include <immintrin.h>
        #include <ammintrin.h> // AMD specific intrinsics
    #elif ENGINE_ISA >= ENGINE_ISA_AVX
        #define _ZMMINTRIN_H_INCLUDED // to suppres include zmmintrin from immintrin
        #include <immintrin.h>
        #include <ammintrin.h> // AMD specific intrinsics
    #elif ENGINE_ISA >= ENGINE_ISA_SSE4_2
        #include <wmmintrin.h>
        #include <nmmintrin.h>
        #include <ammintrin.h> // AMD specific intrinsics
    #elif ENGINE_ISA >= ENGINE_ISA_SSE4_1
        #include <smmintrin.h>
        #include <ammintrin.h> // AMD specific intrinsics
    #elif ENGINE_ISA >= ENGINE_ISA_SSSE3
        #include <tmmintrin.h>
        #include <ammintrin.h> // AMD specific intrinsics
    #elif ENGINE_ISA >= ENGINE_ISA_SSE3
        #include <pmmintrin.h>
        #include <ammintrin.h> // AMD specific intrinsics
    #elif ENGINE_ISA >= ENGINE_ISA_SSE2
        #include <emmintrin.h>
        #include <ammintrin.h> // AMD specific intrinsics
    #elif ENGINE_ISA >= ENGINE_ISA_SSE
        #include <xmmintrin.h>
        #include <ammintrin.h> // AMD specific intrinsics
    #endif
#elif ENGINE_COMPILER_GCC
    // @TODO(Roman): Cross platform includes
#elif ENGINE_COMPILER_CLANG
    // @TODO(Roman): Cross platform includes
#elif ENGINE_COMPILER_MINGW
    // @TODO(Roman): Cross platform includes
#endif

//
// Cross compiler utils
//

#if ENGINE_COMPILER_MSVC
    #define ENGINE_EXPORT        __declspec(dllexport)
    #define ENGINE_IMPORT        __declspec(dllimport)
    #define ENGINE_NOINLINE      __declspec(noinline)
    #define ENGINE_INLINE        __forceinline
    #define ENGINE_ALIGN(_bytes) __declspec(align(_bytes))
    #define ENGINE_INTRIN_TYPE   __declspec(intrin_type)
    #define ENGINE_SELECTANY     __declspec(selectany)
    #define ENGINE_NOVTABLE      __declspec(novtable)
#elif ENGINE_COMPILER_GCC || ENGINE_COMPILER_CLANG || ENGINE_COMPILER_MINGW
    #define ENGINE_EXPORT        __attribute__((dllexport))
    #define ENGINE_IMPORT        __attribute__((dllimport))
    #define ENGINE_NOINLINE      __attribute__((noinline))
    #define ENGINE_INLINE        inline  __attribute__((always_inline))
    #define ENGINE_ALIGN(_bytes) __attribute__((aligned(_bytes)))
    #define ENGINE_INTRIN_TYPE
    #define ENGINE_SELECTANY     __attribute__((selectany))
    #define ENGINE_NOVTABLE
#endif

#pragma warning(pop)
