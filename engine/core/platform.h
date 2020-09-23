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

// @NOTE(Roman): On x64 CPUs we wanna see at least nothing but SSE.
//               And I don't care is it SSE 4.2 or just SSE.
//               https://store.steampowered.com/hwsurvey says
//               there are 97.58% CPUs supported SSE 4.2 (9/16/2020).
//               So if your CPU does not support SSE 4.2
//               some instractions will be illegal probably.
#define ENGINE_ISA_SSE    0
#define ENGINE_ISA_AVX    1
#define ENGINE_ISA_AVX2   2
#define ENGINE_ISA_AVX512 3

#if ENGINE_COMPILER_MSVC
    #if defined(__AVX512BW__) || defined(__AVX512CD__) || defined(__AVX512DQ__) || defined(__AVX512F__) || defined(__AVX512VL__)
        #define ENGINE_ISA ENGINE_ISA_AVX512
    #elif defined(__AVX2__)
        #define ENGINE_ISA ENGINE_ISA_AVX2
    #elif defined(__AVX__)
        #define ENGINE_ISA ENGINE_ISA_AVX
    #else
        #define ENGINE_ISA ENGINE_ISA_SSE
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
    #else
        #include <wmmintrin.h>
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
