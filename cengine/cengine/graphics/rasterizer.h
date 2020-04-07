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

CEXTERN RasterizerOutput *RasterizeTriangle(Engine *engine, v4 p1, v4 p2, v4 p3);
