//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/core.h"
#include "math/vec.h"

#ifndef ENGINE_DEFINED
#define ENGINE_DEFINED
    typedef struct Engine Engine;
#endif

#define VERTEX_SHADER(name) v4 name(Engine *engine, v4 pos) // output = new pos
typedef VERTEX_SHADER(VertexShader);

#define PIXEL_SHADER(name)  v4 name(Engine *engine, v4 pos) // output = color
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
        v2s  first;
        v2s  last;
    } blending;

    struct
    {
        u32 count;
    } common;
} Renderer;

CEXTERN void SetViewport(Engine *engine, f32 near, f32 far);

CEXTERN void RenderOpaqueTriangle(Engine *engine, v4 p1, v4 p2, v4 p3, VertexShader *VS, PixelShader *PS);
CEXTERN void RenderTranslucentTriangle(Engine *engine, v4 p1, v4 p2, v4 p3, VertexShader *VS, PixelShader *PS);
