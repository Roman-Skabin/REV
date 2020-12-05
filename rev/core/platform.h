//
// Copyright 2020 Roman Skabin
//

#pragma once

#pragma warning(push)
#pragma warning(disable: 4005) // macro redefinition

//
// Compiler
//

#define REV_COMPILER_MSVC  0
#define REV_COMPILER_GCC   0
#define REV_COMPILER_CLANG 0
#define REV_COMPILER_MINGW 0

#if defined(_MSC_VER)
    #define REV_COMPILER_MSVC  1
#elif defined(__GNUC__)
    #define REV_COMPILER_GCC   1
#elif defined(__clang__)
    #define REV_COMPILER_CLANG 1
#elif defined(__MINGW64__)
    #define REV_COMPILER_MINGW 1
#elif defined(__MINGW32__)
    #error Only 64-bit systems (=> and compilers) are supported!
#else
    #error Unsupported compiler!
#endif

#if !REV_COMPILER_MSVC
    #error Currently only MSVC compiler is supported!
#endif

//
// Platform
//

#define REV_PLATFORM_WIN64   0
#define REV_PLATFORM_OSX_X64 0
#define REV_PLATFORM_LINUX   0

#if defined(_WIN64)
    #define REV_PLATFORM_WIN64 1
#elif defined(__APPLE__)
    #include <TargetConditionals.h>
    #if defined(TARGET_OS_IPHONE)
        #error iOS is not supported!
    #elif defined(TARGET_OS_MAC)
        #if TARGET_CPU_X86_64
            #define REV_PLATFORM_OSX_X64 1
        #else
            #error Unsupported OSX platform!
        #endif
    #else
        #error Unsupported Apple platform!
    #endif
#elif defined(__linux__)
    #define REV_PLATFORM_LINUX 1
#else
    #error Unsupported platform!
#endif

#if !REV_PLATFORM_WIN64
    #error Currently only WIN64 platform is supported!
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
#define REV_ISA_SSE    0
#define REV_ISA_AVX    1
#define REV_ISA_AVX2   2
#define REV_ISA_AVX512 3

#if REV_COMPILER_MSVC
    #if defined(__AVX512BW__) || defined(__AVX512CD__) || defined(__AVX512DQ__) || defined(__AVX512F__) || defined(__AVX512VL__)
        #define REV_ISA REV_ISA_AVX512
    #elif defined(__AVX2__)
        #define REV_ISA REV_ISA_AVX2
    #elif defined(__AVX__)
        #define REV_ISA REV_ISA_AVX
    #else
        #define REV_ISA REV_ISA_SSE
    #endif
#elif REV_COMPILER_GCC
    // @TODO(Roman): Compiler-specific ISA defines
#elif REV_COMPILER_CLANG
    // @TODO(Roman): Compiler-specific ISA defines
#elif REV_COMPILER_MINGW
    // @TODO(Roman): Compiler-specific ISA defines
#endif

//
// Import libs
//

// UPD: moved to core/pch.cpp

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

#if REV_PLATFORM_WIN64
    #include <Windows.h>
    #undef GetMessage

    #include <Xinput.h>
//  #include <audiopolicy.h>
//  #include <mmdeviceapi.h>
//  #include <AudioClient.h>
//  #include <mfidl.h>
//  #include <mfapi.h>
//  #include <mfreadwrite.h>
#elif REV_PLATFORM_OSX_X64
    // @TODO(Roman): Platform-specific includes
#elif REV_PLATFORM_LINUX
    // @TODO(Roman): Platform-specific includes
#endif

#if REV_COMPILER_MSVC
    #if REV_ISA >= REV_ISA_AVX512
        #include <immintrin.h>
        #include <ammintrin.h> // AMD specific intrinsics
    #elif REV_ISA >= REV_ISA_AVX2
        #define _ZMMINTRIN_H_INCLUDED // to suppres include zmmintrin from immintrin
        #include <immintrin.h>
        #include <ammintrin.h> // AMD specific intrinsics
    #elif REV_ISA >= REV_ISA_AVX
        #define _ZMMINTRIN_H_INCLUDED // to suppres include zmmintrin from immintrin
        #include <immintrin.h>
        #include <ammintrin.h> // AMD specific intrinsics
    #else
        #include <wmmintrin.h>
        #include <ammintrin.h> // AMD specific intrinsics
    #endif
    #include <intrin.h>
#elif REV_COMPILER_GCC
    // @TODO(Roman): Compiler-specific includes
#elif REV_COMPILER_CLANG
    // @TODO(Roman): Compiler-specific includes
#elif REV_COMPILER_MINGW
    // @TODO(Roman): Compiler-specific includes
#endif

//
// Compiler-specific utils
//

#if REV_COMPILER_MSVC
    #define REV_EXPORT        __declspec(dllexport)
    #define REV_IMPORT        __declspec(dllimport)
    #define REV_NOINLINE      __declspec(noinline)
    #define REV_FINLINE       __forceinline
    #define REV_ALIGN(_bytes) __declspec(align(_bytes))
    #define REV_INTRIN_TYPE   __declspec(intrin_type)
    #define REV_SELECTANY     __declspec(selectany)
    #define REV_NOVTABLE      __declspec(novtable)
#elif REV_COMPILER_GCC || REV_COMPILER_CLANG || REV_COMPILER_MINGW
    #define REV_EXPORT        __attribute__((dllexport))
    #define REV_IMPORT        __attribute__((dllimport))
    #define REV_NOINLINE      __attribute__((noinline))
    #define REV_FINLINE       inline  __attribute__((always_inline))
    #define REV_ALIGN(_bytes) __attribute__((aligned(_bytes)))
    #define REV_INTRIN_TYPE
    #define REV_SELECTANY     __attribute__((selectany))
    #define REV_NOVTABLE
#endif

#pragma warning(pop)
