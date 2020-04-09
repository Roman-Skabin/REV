//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "math/vec.h"

#ifndef ENGINE_DEFINED
#define ENGINE_DEFINED
    typedef struct Engine Engine;
#endif

typedef struct RasterizerOutput
{
    union
    {
        v2s xy;
        struct
        {
            s32 x;
            s32 y;
        };
    };
    f32 z;
} RasterizerOutput;

CEXTERN RasterizerOutput *MATH_CALL RasterizeTriangle(Engine *engine, __m128 p1, __m128 p2, __m128 p3);
