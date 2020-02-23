//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "cengine.h"
#include "math/mat.h"
#include "math/color.h"
#include "graphics/rasterizer.h"

internal INLINE v4 MATH_CALL NormalizePoint(EngineState *state, v4 point)
{
    v2 center = v2_1(state->window.size.w / 2.0f, state->window.size.h / 2.0f);
    point.x = 1.0f / center.x * (point.x - center.x);
    point.y = 1.0f / center.y * (point.y - center.y);
    return point;
}

internal INLINE v2 MATH_CALL AdaptPoint(EngineState *state, v4 point)
{
    v2 center = v2_1(state->window.size.w / 2.0f, state->window.size.h / 2.0f);
    point.x = center.x + point.x * center.x;
    point.y = center.y + point.y * center.y;
    return point.xy;
}

void SetViewport(EngineState *state, f32 near, f32 far)
{
    state->renderer.zb.far  = far;
    state->renderer.zb.near = near;
}

typedef struct RenderTriangleData
{
    v4            p1;
    v4            p2;
    v4            p3;
    VertexShader *VS;
    PixelShader  *PS;
    EngineState  *state;
} RenderTriangleData;

internal WORK_QUEUE_ENTRY_PROC(RenderTriangleMT)
{
    // @TODO(Roman): multi-threaded version
}

void RenderTriangle(EngineState *state, v4 p1, v4 p2, v4 p3, VertexShader *VS, PixelShader *PS)
{
    if (state->renderer.zb.size)
    {
        f32 p1z = p1.z;
        f32 p2z = p2.z;
        f32 p3z = p3.z;

        p1 = VS(state, p1);
        p2 = VS(state, p2);
        p3 = VS(state, p3);

        p1 = v4_normalize_w(p1);
        p2 = v4_normalize_w(p2);
        p3 = v4_normalize_w(p3);

        p1.xy = AdaptPoint(state, p1);
        p2.xy = AdaptPoint(state, p2);
        p3.xy = AdaptPoint(state, p3);

        p1.z = p1z;
        p2.z = p2z;
        p3.z = p3z;

        PUSHED_BUF RasterizerOutput *points = RasterizeTriangle(state, p1, p2, p3);

#if 1
        for (u32 i = 0; i < buf_count(points); ++i)
        {
            f32 *zbuf_z = state->renderer.zb.z
                        + points[i].y * state->window.size.w
                        + points[i].x;

            if (points[i].z < *zbuf_z)
            {
                *zbuf_z = points[i].z;

                u32 *dest_pixel = state->renderer.rt.pixels
                                + points[i].y * state->window.size.w
                                + points[i].x;

                v4 source_pixel = v4_2(v2s_to_v2(points[i].xy), points[i].z, 1.0f);

                v4 source_color = PS(state, NormalizePoint(state, source_pixel));
                v4 dest_color   = hex_to_norm(*dest_pixel);

                *dest_pixel = BlendColor(source_color, dest_color);
            }
        }
#endif
    }
}
