// Copyright (c) 2020-2021, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#pragma once

//
// Platform header with all the necessary predefines
//

#include "core/platform.h"

//
// STD includes
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <corecrt_math.h>
#include <vcruntime_typeinfo.h>
#include <vcruntime_new.h>
#include <functional>

//
// Stable headers
//

#include "core/rtti.hpp"
#include "core/key_codes.h"

//
// Platform independent intrinsics activation
//

#pragma intrinsic(abs)
#pragma intrinsic(fabs)
#pragma intrinsic(labs)
#pragma intrinsic(fmod)
#pragma intrinsic(pow)
#pragma intrinsic(sqrt)

#pragma intrinsic(sin)
#pragma intrinsic(cos)
#pragma intrinsic(tan)
#pragma intrinsic(sinf)
#pragma intrinsic(cosf)
#pragma intrinsic(tanf)

#pragma intrinsic(asin)
#pragma intrinsic(acos)
#pragma intrinsic(atan)
#pragma intrinsic(asinf)
#pragma intrinsic(acosf)
#pragma intrinsic(atanf)

#pragma intrinsic(sinh)
#pragma intrinsic(cosh)
#pragma intrinsic(tanh)
#pragma intrinsic(sinhf)
#pragma intrinsic(coshf)
#pragma intrinsic(tanhf)

#pragma intrinsic(exp)
#pragma intrinsic(log)
#pragma intrinsic(log10)
#pragma intrinsic(atan2)
#pragma intrinsic(expf)
#pragma intrinsic(logf)
#pragma intrinsic(log10f)
#pragma intrinsic(atan2f)

#pragma intrinsic(strcmp)
#pragma intrinsic(strcpy)
#pragma intrinsic(strlen)
#pragma intrinsic(_strset)
#pragma intrinsic(strcat)

#pragma intrinsic(memcmp)
#pragma intrinsic(memcpy)
#pragma intrinsic(memset)
