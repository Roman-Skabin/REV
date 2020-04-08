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
        volatile v2s first;
        volatile v2s last;
    } blending;

    struct
    {
        u32 count;
    } common;
} Renderer;

CEXTERN void SetViewport(Engine *engine, f32 near, f32 far);

typedef struct Triangle
{
    _In_  v4                p1;
    _In_  v4                p2;
    _In_  v4                p3;
    _In_  VertexShader     *VS;
    _In_  PixelShader      *PS;
    void                   *private_data; // FOR INTERNAL USE ONLY. DO NOT TOUCH!!!
} Triangle;

CEXTERN void DrawOpaqueTriangles(Engine *engine, Triangle **triangles, u64 triangles_count);
CEXTERN void DrawTranslucentTriangles(Engine *engine, Triangle **triangles, u64 triangles_count);
