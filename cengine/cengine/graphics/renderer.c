//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "graphics/renderer.h"
#include "cengine.h"

void SetVSync(Engine *engine, b32 enable)
{
    if (engine->renderer.vsync != enable)
        engine->renderer.vsync = enable ? 1 : 0;
}

void DrawOpaqueTriangles(Engine *engine, Triangle **triangles, u64 triangles_count)
{
}

void DrawTranslucentTriangles(Engine *engine, Triangle **triangles, u64 count)
{
}

void DrawOpaqueRects(Engine *engine, Rect **rects, u64 count)
{
}

void DrawTranslucentRects(Engine *engine, Rect **rects, u64 count)
{
}

void DrawOpaqueMeshes(Engine *engine, Mesh **meshes, u64 count)
{
}

void DrawTranslucentMeshes(Engine *engine, Mesh **meshes, u64 count)
{
}
