//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "graphics/rasterizer.h"
#include "cengine.h"

internal INLINE v4 *GetLeftPoint(v4 *p1, v4 *p2, v4 *p3)
{
    if (p1->x < p2->x) return p1->x < p3->x ? p1 : p3;
    else               return p2->x < p3->x ? p2 : p3;
}

internal INLINE v4 *GetRightPoint(v4 *p1, v4 *p2, v4 *p3)
{
    if (p1->x > p2->x) return p1->x > p3->x ? p1 : p3;
    else               return p2->x > p3->x ? p2 : p3;
}

internal INLINE v4 *GetMiddlePoint(v4 *p1, v4 *p2, v4 *p3, v4 *left, v4 *right)
{
    if (p1 == left) return p2 == right ? p3 : p2;
    if (p2 == left) return p1 == right ? p3 : p1;
    else            return p1 == right ? p2 : p1;
}

internal void RasterizeTriangleSide(
    RasterizerOutput **triangle,
    EngineState       *state,
    v4                 _lower,
    v4                 _upper
)
{
    v4s upper = v4_to_v4s(_upper);
    v4s lower = v4_to_v4s(_lower);

    f32 x_slope = 0.0f;
    f32 z_slope = 0.0f;

    if (_upper.y - _lower.y)
    {
        x_slope = (_upper.x - _lower.x) / (_upper.y - _lower.y);
        z_slope = (_upper.z - _lower.z) / (_upper.y - _lower.y);
    }

    f32 x = _lower.x;
    f32 z = _lower.z;

    for (s32 y = lower.y; y <= upper.y; ++y)
    {
        v4   point = v4_1(x, cast(f32, y), z, 1.0f);
        v4s ipoint = v4_to_v4s(point);

        if (                      0 <= ipoint.x && ipoint.x <  state->window.size.w
        &&                        0 <= ipoint.y && ipoint.y <  state->window.size.h
        &&  state->renderer.zb.near <=  point.z &&  point.z <= state->renderer.zb.far)
        {
            RasterizerOutput el;
            el.xy = ipoint.xy;
            el.z  = point.z;
            buf_push(*triangle, el);
        }

        x += x_slope;
        z += z_slope;
    }
}

internal BUF RasterizerOutput *RasterizeTriangleDownUpLeftLower(EngineState *state, v4 left, v4 middle, v4 right)
{
    BUF RasterizerOutput *triangle = 0;

    RasterizeTriangleSide(&triangle, state, middle, left);
    u64 left_line_first = 0;
    u64 left_line_last  = buf_count(triangle) - 1;

    RasterizeTriangleSide(&triangle, state, middle, right);
    u64 right_line_first = left_line_last + 1;
    u64 right_line_last  = buf_count(triangle) - 1;

    RasterizeTriangleSide(&triangle, state, left, right);
    u64 upper_line_first = right_line_last + 1;
    u64 upper_line_last  = buf_count(triangle) - 1;

    u64 lower_index = 0;
    u64 upper_index = 0;

    for (s32 y = triangle[left_line_first].y; y < triangle[left_line_last].y; ++y)
    {
        f32 z_slope = 0.0f;

        if (triangle[right_line_first + lower_index].x - triangle[left_line_first + lower_index].x)
        {
            z_slope = (triangle[right_line_first + lower_index].z - triangle[left_line_first + lower_index].z)
                    / (triangle[right_line_first + lower_index].x - triangle[left_line_first + lower_index].x);
        }

        f32 z = triangle[left_line_first + lower_index].z;

        for (s32 x = triangle[left_line_first + lower_index].x; x <= triangle[right_line_first + lower_index].x; ++x)
        {
            RasterizerOutput el;
            el.x = x;
            el.y = y;
            el.z = z;
            buf_push(triangle, el);

            z += z_slope;
        }

        ++lower_index;
    }

    for (s32 y = triangle[left_line_last].y; y <= triangle[right_line_last].y; ++y)
    {
        f32 z_slope = 0.0f;

        if (triangle[right_line_first + lower_index].x - triangle[upper_line_first + upper_index].x)
        {
            z_slope = (triangle[right_line_first + lower_index].z - triangle[upper_line_first + upper_index].z)
                    / (triangle[right_line_first + lower_index].x - triangle[upper_line_first + upper_index].x);
        }

        f32 z = triangle[upper_line_first + upper_index].z;

        for (s32 x = triangle[upper_line_first + upper_index].x; x <= triangle[right_line_first + lower_index].x; ++x)
        {
            RasterizerOutput el;
            el.x = x;
            el.y = y;
            el.z = z;
            buf_push(triangle, el);

            z += z_slope;
        }

        ++lower_index;
        ++upper_index;
    }

    return triangle;
}

internal BUF RasterizerOutput *RasterizeTriangleDownUpRightLower(EngineState *state, v4 left, v4 middle, v4 right)
{
    BUF RasterizerOutput *triangle = 0;

    RasterizeTriangleSide(&triangle, state, middle, left);
    u64 left_line_first = 0;
    u64 left_line_last  = buf_count(triangle) - 1;

    RasterizeTriangleSide(&triangle, state, middle, right);
    u64 right_line_first = left_line_last + 1;
    u64 right_line_last  = buf_count(triangle) - 1;

    RasterizeTriangleSide(&triangle, state, right, left);
    u64 upper_line_first = right_line_last + 1;
    u64 upper_line_last  = buf_count(triangle) - 1;

    u64 lower_index = 0;
    u64 upper_index = 0;

    for (s32 y = triangle[right_line_first].y; y < triangle[right_line_last].y; ++y)
    {
        f32 z_slope = 0.0f;

        if (triangle[right_line_first + lower_index].x - triangle[left_line_first + lower_index].x)
        {
            z_slope = (triangle[right_line_first + lower_index].z - triangle[left_line_first + lower_index].z)
                    / (triangle[right_line_first + lower_index].x - triangle[left_line_first + lower_index].x);
        }

        f32 z = triangle[left_line_first + lower_index].z;

        for (s32 x = triangle[left_line_first + lower_index].x; x <= triangle[right_line_first + lower_index].x; ++x)
        {
            RasterizerOutput el;
            el.x = x;
            el.y = y;
            el.z = z;
            buf_push(triangle, el);

            z += z_slope;
        }

        ++lower_index;
    }

    for (s32 y = triangle[right_line_last].y; y <= triangle[left_line_last].y; ++y)
    {
        f32 z_slope = 0.0f;

        if (triangle[upper_line_first + upper_index].x - triangle[left_line_first + lower_index].x)
        {
            z_slope = (triangle[upper_line_first + upper_index].z - triangle[left_line_first + lower_index].z)
                    / (triangle[upper_line_first + upper_index].x - triangle[left_line_first + lower_index].x);
        }

        f32 z = triangle[left_line_first + lower_index].z;

        for (s32 x = triangle[left_line_first + lower_index].x; x <= triangle[upper_line_first + upper_index].x; ++x)
        {
            RasterizerOutput el;
            el.x = x;
            el.y = y;
            el.z = z;
            buf_push(triangle, el);

            z += z_slope;
        }

        ++lower_index;
        ++upper_index;
    }

    return triangle;
}

internal BUF RasterizerOutput *RasterizeTriangleUpDownLeftLower(EngineState *state, v4 left, v4 middle, v4 right)
{
    BUF RasterizerOutput *triangle = 0;
    
    RasterizeTriangleSide(&triangle, state, left, middle);
    u64 left_line_first = 0;
    u64 left_line_last  = buf_count(triangle) - 1;

    RasterizeTriangleSide(&triangle, state, right, middle);
    u64 right_line_first = left_line_last + 1;
    u64 right_line_last  = buf_count(triangle) - 1;

    RasterizeTriangleSide(&triangle, state, left, right);
    u64 lower_line_first = right_line_last + 1;
    u64 lower_line_last  = buf_count(triangle) - 1;

    u64 upper_index = 0;
    u64 lower_index = 0;

    for (s32 y = triangle[right_line_last].y; y > triangle[right_line_first].y; --y)
    {
        u64 right_index = right_line_last - upper_index;
        u64 left_index  = left_line_last  - upper_index;

        f32 z_slope = 0.0f;

        if (triangle[right_index].x - triangle[left_index].x)
        {
            z_slope = (triangle[right_index].z - triangle[left_index].z)
                    / (triangle[right_index].x - triangle[left_index].x);
        }

        f32 z = triangle[left_index].z;

        for (s32 x = triangle[left_index].x; x <= triangle[right_index].x; ++x)
        {
            RasterizerOutput el;
            el.x = x;
            el.y = y;
            el.z = z;
            buf_push(triangle, el);

            z += z_slope;
        }

        ++upper_index;
    }

    for (s32 y = triangle[right_line_first].y; y >= triangle[left_line_first].y; --y)
    {
        u64 lower_line_index = lower_line_last - lower_index;
        u64 left_index       = left_line_last  - upper_index;

        f32 z_slope = 0.0f;

        if (triangle[lower_line_index].x - triangle[left_index].x)
        {
            z_slope = (triangle[lower_line_index].z - triangle[left_index].z)
                    / (triangle[lower_line_index].x - triangle[left_index].x);
        }

        f32 z = triangle[left_index].z;

        for (s32 x = triangle[left_index].x; x <= triangle[lower_line_index].x; ++x)
        {
            RasterizerOutput el;
            el.x = x;
            el.y = y;
            el.z = z;
            buf_push(triangle, el);

            z += z_slope;
        }

        ++lower_index;
        ++upper_index;
    }

    return triangle;
}

internal BUF RasterizerOutput *RasterizeTriangleUpDownRightLower(EngineState *state, v4 left, v4 middle, v4 right)
{
    BUF RasterizerOutput *triangle = 0;
    
    RasterizeTriangleSide(&triangle, state, left, middle);
    u64 left_line_first = 0;
    u64 left_line_last  = buf_count(triangle) - 1;

    RasterizeTriangleSide(&triangle, state, right, middle);
    u64 right_line_first = left_line_last + 1;
    u64 right_line_last  = buf_count(triangle) - 1;

    RasterizeTriangleSide(&triangle, state, right, left);
    u64 lower_line_first = right_line_last + 1;
    u64 lower_line_last  = buf_count(triangle) - 1;

    u64 upper_index = 0;
    u64 lower_index = 0;

    for (s32 y = triangle[left_line_last].y; y > triangle[left_line_first].y; --y)
    {
        u64 left_index  = left_line_last  - upper_index;
        u64 right_index = right_line_last - upper_index;

        f32 z_slope = 0.0f;

        if (triangle[right_index].x - triangle[left_index].x)
        {
            z_slope = (triangle[right_index].z - triangle[left_index].z)
                    / (triangle[right_index].x - triangle[left_index].x);
        }

        f32 z = triangle[left_index].z;

        for (s32 x = triangle[left_index].x; x <= triangle[right_index].x; ++x)
        {
            RasterizerOutput el;
            el.x = x;
            el.y = y;
            el.z = z;
            buf_push(triangle, el);

            z += z_slope;
        }

        ++upper_index;
    }

    for (s32 y = triangle[left_line_first].y; y >= triangle[lower_line_first].y; --y)
    {
        u64 right_index      = right_line_last - upper_index;
        u64 lower_line_index = lower_line_last - lower_index;

        f32 z_slope = 0.0f;

        if (triangle[right_index].x - triangle[lower_line_index].x)
        {
            z_slope = (triangle[right_index].z - triangle[lower_line_index].z)
                    / (triangle[right_index].x - triangle[lower_line_index].x);
        }

        f32 z = triangle[lower_line_index].z;

        for (s32 x = triangle[lower_line_index].x; x <= triangle[right_index].x; ++x)
        {
            RasterizerOutput el;
            el.x = x;
            el.y = y;
            el.z = z;
            buf_push(triangle, el);

            z += z_slope;
        }

        ++lower_index;
        ++upper_index;
    }

    return triangle;
}

PUSHED_BUF RasterizerOutput *RasterizeTriangle(EngineState *state, v4 p1, v4 p2, v4 p3)
{
    v4 *left   = GetLeftPoint(&p1, &p2, &p3);
    v4 *right  = GetRightPoint(&p1, &p2, &p3);
    v4 *middle = GetMiddlePoint(&p1, &p2, &p3, left, right);

    BUF RasterizerOutput *triangle = 0;

    if (middle->y <= min(left->y, right->y))
    {
        if (left->y <= right->y)
        {
            triangle = RasterizeTriangleDownUpLeftLower(state, *left, *middle, *right);
        }
        else
        {
            triangle = RasterizeTriangleDownUpRightLower(state, *left, *middle, *right);
        }
    }
    else
    {
        if (left->y <= right->y)
        {
            triangle = RasterizeTriangleUpDownLeftLower(state, *left, *middle, *right);
        }
        else
        {
            triangle = RasterizeTriangleUpDownRightLower(state, *left, *middle, *right);
        }
    }

    BufHdr *pushed_triangle = PushToTransientArea(&state->memory, offsetof(BufHdr, buf) + buf_count(triangle) * sizeof(RasterizerOutput));
    CopyMemory(pushed_triangle, _BUFHDR(triangle), offsetof(BufHdr, buf) + buf_count(triangle) * sizeof(RasterizerOutput));
    buf_free(triangle);

    return cast(RasterizerOutput *, pushed_triangle->buf);
}
