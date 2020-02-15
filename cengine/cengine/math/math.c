//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "math/math.h"

global u64 gFactTable[21] =
{
    1,                   // __fact(0)
    1,                   // __fact(1)
    2,                   // __fact(2)
    6,                   // __fact(3)
    24,                  // __fact(4)
    120,                 // __fact(5)
    720,                 // __fact(6)
    5040,                // __fact(7)
    40320,               // __fact(8)
    362880,              // __fact(9)
    3628800,             // __fact(10)
    39916800,            // __fact(11)
    479001600,           // __fact(12)
    6227020800,          // __fact(13)
    87178291200,         // __fact(14)
    1307674368000,       // __fact(15)
    20922789888000,      // __fact(16)
    355687428096000,     // __fact(17)
    6402373705728000,    // __fact(18)
    121645100408832000,  // __fact(19)
    2432902008176640000  // __fact(20)
};

u64 __fastcall fact(u8 value)
{
    return value <= 20 ? gFactTable[value] : 0;
}
