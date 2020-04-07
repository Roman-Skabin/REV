//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "cengine.h"
#include "math/mat.h"
#include "math/color.h"
#include "graphics/rasterizer.h"
#include "tools/buffer.h"

internal INLINE v4 MATH_CALL NormalizePoint(Engine *engine, v4 point)
{
    v2 center = v2_1(engine->window.size.w / 2.0f, engine->window.size.h / 2.0f);
    point.x = 1.0f / center.x * (point.x - center.x);
    point.y = 1.0f / center.y * (point.y - center.y);
    return point;
}

internal INLINE v2 MATH_CALL AdaptPoint(Engine *engine, v4 point)
{
    v2 center = v2_1(engine->window.size.w / 2.0f, engine->window.size.h / 2.0f);
    point.x = center.x + point.x * center.x;
    point.y = center.y + point.y * center.y;
    return point.xy;
}

void SetViewport(Engine *engine, f32 near, f32 far)
{
    engine->renderer.zb.far  = far;
    engine->renderer.zb.near = near;
}

typedef struct RenderData
{
    Engine           *engine;
    RasterizerOutput *points;
    PixelShader      *PS;
    volatile u64      i;
} RenderData;

internal WORK_QUEUE_ENTRY_PROC(RenderOpaqueTriangle)
{
    RenderData *data = arg;

    while (!data->points)
    {
        // Wait till our triangle will be rasterized.
    }

    u64 old_i = data->i;
    u64 new_i = old_i + 1;

    u64 points_count = BufferGetCount(data->points);
    while (old_i < points_count)
    {
        if (old_i == _InterlockedCompareExchange64(&data->i, new_i, old_i))
        {
            f32 z = data->points[old_i].z;

            u32 offset = data->points[old_i].y * data->engine->window.size.w
                       + data->points[old_i].x;

            f32 *zbuf_z = data->engine->renderer.zb.z + offset;

            if (z < *zbuf_z)
            {
                *zbuf_z = z;

                v4 source_pixel = v4_2(v2s_to_v2(data->points[old_i].xy), z, 1.0f);
                v4 source_color = data->PS(data->engine, NormalizePoint(data->engine, source_pixel));
                source_color.a = 1.0f;

                data->engine->renderer.rt.pixels[offset] = norm_to_hex(source_color);
            }

            old_i = data->i;
            new_i = old_i + 1;
        }
    }
}

internal WORK_QUEUE_ENTRY_PROC(RenderTranslucentTriangle)
{
    RenderData *data = arg;

    while (!data->points)
    {
        // Wait till our triangle will be rasterized.
    }

    u64 old_i = data->i;
    u64 new_i = old_i + 1;

    u64 points_count = BufferGetCount(data->points);
    while (old_i < points_count)
    {
        if (old_i == _InterlockedCompareExchange64(&data->i, new_i, old_i))
        {
            s32 x = data->points[old_i].x;
            s32 y = data->points[old_i].y;
            f32 z = data->points[old_i].z;

            f32 *zbuf_z = data->engine->renderer.zb.z
                        + y * data->engine->window.size.w
                        + x;

            if (z < *zbuf_z)
            {
                if (data->engine->renderer.blending.first.x > x) data->engine->renderer.blending.first.x = x;
                if (data->engine->renderer.blending.first.y > y) data->engine->renderer.blending.first.y = y;
                if (data->engine->renderer.blending.last.x  < x) data->engine->renderer.blending.last.x = x;
                if (data->engine->renderer.blending.last.y  < y) data->engine->renderer.blending.last.y = y;

                v4 source_pixel = v4_2(v2s_to_v2(data->points[old_i].xy), z, 1.0f);
                v4 source_color = data->PS(data->engine, NormalizePoint(data->engine, source_pixel));

                CheckM(source_color.a < 1.0f, "alpha >= 1.0f, use RenderOpaqueTriangle instead");

                BlendOnRender(data->engine, x, y, z, source_color);
            }

            old_i = data->i;
            new_i = old_i + 1;
        }
    }
}

void DrawOpaqueTriangles(Engine *engine, Triangle **triangles, u64 triangles_count)
{
    if (engine->renderer.common.count)
    {
        for (u64 i = 0; i < triangles_count; ++i)
        {
            Triangle *triangle = triangles[i];

            v4 p1 = triangle->VS(engine, triangle->p1);
            v4 p2 = triangle->VS(engine, triangle->p2);
            v4 p3 = triangle->VS(engine, triangle->p3);

            p1 = v4_normalize_w(p1);
            p2 = v4_normalize_w(p2);
            p3 = v4_normalize_w(p3);

            p1.xy = AdaptPoint(engine, p1);
            p2.xy = AdaptPoint(engine, p2);
            p3.xy = AdaptPoint(engine, p3);

            p1.z = triangle->p1.z;
            p2.z = triangle->p2.z;
            p3.z = triangle->p3.z;

            triangle->private_data = RasterizeTriangle(engine, p1, p2, p3);
        }

        for (u64 i = 0; i < triangles_count; ++i)
        {
            Triangle *triangle = triangles[i];

            RenderData *data = PushToTA(RenderData, engine->memory, 1);
            data->engine     = engine;
            data->points     = triangle->private_data;
            data->PS         = triangle->PS;

            AddWorkQueueEntry(engine->queue, RenderOpaqueTriangle, data);
        }
    }
}

void DrawTranslucentTriangles(Engine *engine, Triangle **triangles, u64 triangles_count)
{
    if (engine->renderer.common.count)
    {
        for (u64 i = 0; i < triangles_count; ++i)
        {
            Triangle *triangle = triangles[i];

            v4 p1 = triangle->VS(engine, triangle->p1);
            v4 p2 = triangle->VS(engine, triangle->p2);
            v4 p3 = triangle->VS(engine, triangle->p3);

            p1 = v4_normalize_w(p1);
            p2 = v4_normalize_w(p2);
            p3 = v4_normalize_w(p3);

            p1.xy = AdaptPoint(engine, p1);
            p2.xy = AdaptPoint(engine, p2);
            p3.xy = AdaptPoint(engine, p3);

            p1.z = triangle->p1.z;
            p2.z = triangle->p2.z;
            p3.z = triangle->p3.z;

            triangle->private_data = RasterizeTriangle(engine, p1, p2, p3);
        }

        for (u64 i = 0; i < triangles_count; ++i)
        {
            Triangle *triangle = triangles[i];

            RenderData *data = PushToTA(RenderData, engine->memory, 1);
            data->engine     = engine;
            data->points     = triangle->private_data;
            data->PS         = triangle->PS;

            AddWorkQueueEntry(engine->queue, RenderTranslucentTriangle, data);
        }
    }
}
