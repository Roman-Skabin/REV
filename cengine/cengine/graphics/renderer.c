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

typedef struct RenderTriangleData
{
    v4            p1;
    v4            p2;
    v4            p3;
    VertexShader *VS;
    PixelShader  *PS;
    Engine  *engine;
} RenderTriangleData;

internal WORK_QUEUE_ENTRY_PROC(RenderTriangleMT)
{
    // @TODO(Roman): multi-threaded version
}

void RenderOpaqueTriangle(Engine *engine, v4 p1, v4 p2, v4 p3, VertexShader *VS, PixelShader *PS)
{
    if (engine->renderer.common.count)
    {
        f32 p1z = p1.z;
        f32 p2z = p2.z;
        f32 p3z = p3.z;

        p1 = VS(engine, p1);
        p2 = VS(engine, p2);
        p3 = VS(engine, p3);

        p1 = v4_normalize_w(p1);
        p2 = v4_normalize_w(p2);
        p3 = v4_normalize_w(p3);

        p1.xy = AdaptPoint(engine, p1);
        p2.xy = AdaptPoint(engine, p2);
        p3.xy = AdaptPoint(engine, p3);

        p1.z = p1z;
        p2.z = p2z;
        p3.z = p3z;

        RasterizerOutput *points = RasterizeTriangle(engine, p1, p2, p3);

        for (u32 i = 0; i < BufferGetCount(points); ++i)
        {
            f32 *zbuf_z = engine->renderer.zb.z
                        + points[i].y * engine->window.size.w
                        + points[i].x;

            if (points[i].z < *zbuf_z)
            {
                *zbuf_z = points[i].z;

                u32 *dest_pixel = engine->renderer.rt.pixels
                                + points[i].y * engine->window.size.w
                                + points[i].x;

                v4 source_pixel = v4_2(v2s_to_v2(points[i].xy), points[i].z, 1.0f);
                v4 source_color = PS(engine, NormalizePoint(engine, source_pixel));

                *dest_pixel = norm_to_hex(source_color);
            }
        }
    }
}

void RenderTranslucentTriangle(Engine *engine, v4 p1, v4 p2, v4 p3, VertexShader *VS, PixelShader *PS)
{
    if (engine->renderer.common.count)
    {
        f32 p1z = p1.z;
        f32 p2z = p2.z;
        f32 p3z = p3.z;

        p1 = VS(engine, p1);
        p2 = VS(engine, p2);
        p3 = VS(engine, p3);

        p1 = v4_normalize_w(p1);
        p2 = v4_normalize_w(p2);
        p3 = v4_normalize_w(p3);

        p1.xy = AdaptPoint(engine, p1);
        p2.xy = AdaptPoint(engine, p2);
        p3.xy = AdaptPoint(engine, p3);

        p1.z = p1z;
        p2.z = p2z;
        p3.z = p3z;

        RasterizerOutput *points = RasterizeTriangle(engine, p1, p2, p3);

        for (u32 i = 0; i < BufferGetCount(points); ++i)
        {
            f32 *zbuf_z = engine->renderer.zb.z
                        + points[i].y * engine->window.size.w
                        + points[i].x;

            if (points[i].z < *zbuf_z)
            {
                v4 source_pixel = v4_2(v2s_to_v2(points[i].xy), points[i].z, 1.0f);
                v4 source_color = PS(engine, NormalizePoint(engine, source_pixel));

                BlendOnRender(engine, points[i].x, points[i].y, points[i].z, source_color);
            }
        }
    }
}
