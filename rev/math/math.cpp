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
    return g_FactTable[Math::min(value, 20ui8)];
}

}
