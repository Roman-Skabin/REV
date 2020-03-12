//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/core.h"
#include "math/vec.h"

#ifndef ENGINE_STATE_DEFINED
#define ENGINE_STATE_DEFINED
    typedef struct EngineState EngineState;
#endif

#define VERTEX_SHADER(name) v4 name(EngineState *state, v4 pos) // output = new pos
#define PIXEL_SHADER(name)  v4 name(EngineState *state, v4 pos) // output = color

typedef VERTEX_SHADER(VertexShader);
typedef PIXEL_SHADER(PixelShader);

typedef struct Renderer
{
    struct
    {
        u32        *pixels;
        BITMAPINFO  info;
    } rt;

    struct
    {
        f32 *z;
        f32 near;
        f32 far;
    } zb;

    struct
    {
        v4  *sum;
        f32 *mul;
    } blending;

    struct
    {
        u32 count;
    } common;
} Renderer;

CEXTERN void SetViewport(EngineState *state, f32 near, f32 far);

CEXTERN void RenderOpaqueTriangle(EngineState *state, v4 p1, v4 p2, v4 p3, VertexShader *VS, PixelShader *PS);
CEXTERN void RenderTranslucentTriangle(EngineState *state, v4 p1, v4 p2, v4 p3, VertexShader *VS, PixelShader *PS);
