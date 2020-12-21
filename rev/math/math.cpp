//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "math/math.h"

namespace REV::Math
{

REV_GLOBAL u64 g_FactTable[21] =
{
    1,                   // fact(0)
    1,                   // fact(1)
    2,                   // fact(2)
    6,                   // fact(3)
    24,                  // fact(4)
    120,                 // fact(5)
    720,                 // fact(6)
    5040,                // fact(7)
    40320,               // fact(8)
    362880,              // fact(9)
    3628800,             // fact(10)
    39916800,            // fact(11)
    479001600,           // fact(12)
    6227020800,          // fact(13)
    87178291200,         // fact(14)
    1307674368000,       // fact(15)
    20922789888000,      // fact(16)
    355687428096000,     // fact(17)
    6402373705728000,    // fact(18)
    121645100408832000,  // fact(19)
    2432902008176640000  // fact(20)
};

u64 fact(u8 value)
{
    return g_FactTable[RTTI::min(value, 20ui8)];
}

// MSVC:                             xmm0     rdx     r8
// GCC:                              xmm0     rdi     rsi
REV_NAKED void REV_VECTORCALL sincos(rad32 a, f32 *s, f32 *c)
{
#if REV_COMPILER_MSVC && 0 // I hate MSVC, C++ and a mysterious reason I can't use inline asembly on x64
    __asm
    {
        push    rbp
        mov     rbp, rsp
        sub     rsp, 4

    #if REV_ISA >= REV_ISA_AVX
        vmovss  dword ptr [rsp], xmm0
    #else
        movss   dword ptr [rsp], xmm0
    #endif

        fld     dword ptr [rsp]
        fsincos
        fstp    dword ptr [r8]
        fstp    dword ptr [rdx]

        mov     rsp, rbp
        pop     rbp
        ret
    }
#elif !REV_COMPILER_MSVC
    __asm("push    rbp");
    __asm("mov     rbp, rsp");
    __asm("sub     rsp, 4");
    #if REV_ISA >= REV_ISA_AVX
    __asm("vmovss  DWORD PTR [rsp], xmm0");
    #else
    __asm("movss   DWORD PTR [rsp], xmm0");
    #endif
    __asm("fld     DWORD PTR [rsp]");
    __asm("fsincos");
    __asm("fstp    DWORD PTR [rsi]");
    __asm("fstp    DWORD PTR [rdi]");
    __asm("mov     rsp, rbp");
    __asm("pop     rbp");
    __asm("ret");
#else
    *s = sinf(a);
    *c = cosf(a);
#endif
}

// MSVC:                             xmm0     rdx     r8
// GCC:                              xmm0     rdi     rsi
REV_NAKED void REV_VECTORCALL sincos(rad64 a, f64 *s, f64 *c)
{
#if REV_COMPILER_MSVC && 0 // I hate MSVC, C++ and a mysterious reason I can't use inline asembly on x64
    __asm
    {
        push    rbp
        mov     rbp, rsp
        sub     rsp, 8

    #if REV_ISA >= REV_ISA_AVX
        vmovsd  qword ptr [rsp], xmm0
    #else
        movsd   qword ptr [rsp], xmm0
    #endif

        fld     qword ptr [rsp]
        fsincos
        fstp    qword ptr [r8]
        fstp    qword ptr [rdx]

        mov     rsp, rbp
        pop     rbp
        ret
    }
#elif !REV_COMPILER_MSVC
    __asm("push    rbp");
    __asm("mov     rbp, rsp");
    __asm("sub     rsp, 8");
    #if REV_ISA >= REV_ISA_AVX
    __asm("vmovsd  QWORD PTR [rsp], xmm0");
    #else
    __asm("movsd   QWORD PTR [rsp], xmm0");
    #endif
    __asm("fld     QWORD PTR [rsp]");
    __asm("fsincos");
    __asm("fstp    QWORD PTR [rsi]");
    __asm("fstp    QWORD PTR [rdi]");
    __asm("mov     rsp, rbp");
    __asm("pop     rbp");
    __asm("ret");
#else
    *s = sin(a);
    *c = cos(a);
#endif
}

}
