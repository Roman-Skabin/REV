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

internal BUF RasterizerOutput *RasterizeTriangleSide(EngineState *state, v4 _lower, v4 _upper)
{
    BUF RasterizerOutput *output = 0;

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
            buf_push(output, el);
        }

        x += x_slope;
        z += z_slope;
    }

    return output;
}

internal BUF RasterizerOutput *RasterizeTriangleDownUpLeftLower(EngineState *state, v4 left, v4 middle, v4 right)
{
    BUF RasterizerOutput *triangle = 0;

    BUF RasterizerOutput *left_line  = RasterizeTriangleSide(state, middle, left);
    BUF RasterizerOutput *right_line = RasterizeTriangleSide(state, middle, right);
    BUF RasterizerOutput *upper_line = RasterizeTriangleSide(state, left, right);

    u32 lower_index = 0;
    u32 upper_index = 0;

    s32 upper_left_y  = left_line[buf_count(left_line) - 1].y;
    s32 upper_right_y = right_line[buf_count(right_line) - 1].y;

    for (s32 y = left_line[0].y; y < upper_left_y; ++y)
    {
        f32 z_slope = 0.0f;

        if (right_line[lower_index].x - left_line[lower_index].x)
        {
            z_slope = (right_line[lower_index].z - left_line[lower_index].z)
                    / (right_line[lower_index].x - left_line[lower_index].x);
        }

        f32 z = left_line[lower_index].z;

        for (s32 x = left_line[lower_index].x; x <= right_line[lower_index].x; ++x)
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

    for (s32 y = upper_left_y; y <= upper_right_y; ++y)
    {
        f32 z_slope = 0.0f;

        if (right_line[lower_index].x - upper_line[upper_index].x)
        {
            z_slope = (right_line[lower_index].z - upper_line[upper_index].z)
                    / (right_line[lower_index].x - upper_line[upper_index].x);
        }

        f32 z = upper_line[upper_index].z;

        for (s32 x = upper_line[upper_index].x; x <= right_line[lower_index].x; ++x)
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

    buf_free(upper_line);
    buf_free(right_line);
    buf_free(left_line);

    return triangle;
}

internal BUF RasterizerOutput *RasterizeTriangleDownUpRightLower(EngineState *state, v4 left, v4 middle, v4 right)
{
    BUF RasterizerOutput *triangle = 0;

    BUF RasterizerOutput *left_line  = RasterizeTriangleSide(state, middle, left);
    BUF RasterizerOutput *right_line = RasterizeTriangleSide(state, middle, right);
    BUF RasterizerOutput *upper_line = RasterizeTriangleSide(state, right, left);

    u32 lower_index = 0;
    u32 upper_index = 0;

    s32 upper_right_y = right_line[buf_count(right_line) - 1].y;
    s32 upper_left_y  = left_line[buf_count(left_line) - 1].y;

    for (s32 y = right_line[0].y; y < upper_right_y; ++y)
    {
        f32 z_slope = 0.0f;

        if (right_line[lower_index].x - left_line[lower_index].x)
        {
            z_slope = (right_line[lower_index].z - left_line[lower_index].z)
                    / (right_line[lower_index].x - left_line[lower_index].x);
        }

        f32 z = left_line[lower_index].z;

        for (s32 x = left_line[lower_index].x; x <= right_line[lower_index].x; ++x)
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

    for (s32 y = upper_right_y; y <= upper_left_y; ++y)
    {
        f32 z_slope = 0.0f;

        if (upper_line[upper_index].x - left_line[lower_index].x)
        {
            z_slope = (upper_line[upper_index].z - left_line[lower_index].z)
                    / (upper_line[upper_index].x - left_line[lower_index].x);
        }

        f32 z = left_line[lower_index].z;

        for (s32 x = left_line[lower_index].x; x <= upper_line[upper_index].x; ++x)
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

    buf_free(upper_line);
    buf_free(right_line);
    buf_free(left_line);

    return triangle;
}

internal BUF RasterizerOutput *RasterizeTriangleUpDownLeftLower(EngineState *state, v4 left, v4 middle, v4 right)
{
    BUF RasterizerOutput *triangle = 0;

    BUF RasterizerOutput *left_line  = RasterizeTriangleSide(state, left, middle);
    BUF RasterizerOutput *right_line = RasterizeTriangleSide(state, right, middle);
    BUF RasterizerOutput *lower_line = RasterizeTriangleSide(state, left, right);

    u32 upper_index = 1;
    u32 lower_index = 1;

    for (s32 y = right_line[buf_count(right_line) - 1].y; y > right_line[0].y; --y)
    {
        u32 right_index = buf_count(right_line) - upper_index;
        u32 left_index  = buf_count(left_line)  - upper_index;

        f32 z_slope = 0.0f;

        if (right_line[right_index].x - left_line[left_index].x)
        {
            z_slope = (right_line[right_index].z - left_line[left_index].z)
                    / (right_line[right_index].x - left_line[left_index].x);
        }

        f32 z = left_line[left_index].z;

        for (s32 x = left_line[left_index].x; x <= right_line[right_index].x; ++x)
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

    for (s32 y = right_line[0].y; y >= left_line[0].y; --y)
    {
        u32 lower_line_index = buf_count(lower_line) - lower_index;
        u32 left_index       = buf_count(left_line)  - upper_index;

        f32 z_slope = 0.0f;

        if (lower_line[lower_line_index].x - left_line[left_index].x)
        {
            z_slope = (lower_line[lower_line_index].z - left_line[left_index].z)
                    / (lower_line[lower_line_index].x - left_line[left_index].x);
        }

        f32 z = left_line[left_index].z;

        for (s32 x = left_line[left_index].x; x <= lower_line[lower_line_index].x; ++x)
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

    buf_free(lower_line);
    buf_free(right_line);
    buf_free(left_line);

    return triangle;
}

internal BUF RasterizerOutput *RasterizeTriangleUpDownRightLower(EngineState *state, v4 left, v4 middle, v4 right)
{
    BUF RasterizerOutput *triangle = 0;

    BUF RasterizerOutput *left_line  = RasterizeTriangleSide(state, left, middle);
    BUF RasterizerOutput *right_line = RasterizeTriangleSide(state, right, middle);
    BUF RasterizerOutput *lower_line = RasterizeTriangleSide(state, right, left);

    u32 upper_index = 1;
    u32 lower_index = 1;

    for (s32 y = left_line[buf_count(right_line) - 1].y; y > left_line[0].y; --y)
    {
        u32 left_index  = buf_count(left_line)  - upper_index;
        u32 right_index = buf_count(right_line) - upper_index;

        f32 z_slope = 0.0f;

        if (right_line[right_index].x - left_line[left_index].x)
        {
            z_slope = (right_line[right_index].z - left_line[left_index].z)
                    / (right_line[right_index].x - left_line[left_index].x);
        }

        f32 z = left_line[left_index].z;

        for (s32 x = left_line[left_index].x; x <= right_line[right_index].x; ++x)
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

    for (s32 y = left_line[0].y; y >= lower_line[0].y; --y)
    {
        u32 right_index      = buf_count(right_line) - upper_index;
        u32 lower_line_index = buf_count(lower_line) - lower_index;

        f32 z_slope = 0.0f;

        if (right_line[right_index].x - lower_line[lower_line_index].x)
        {
            z_slope = (right_line[right_index].z - lower_line[lower_line_index].z)
                    / (right_line[right_index].x - lower_line[lower_line_index].x);
        }

        f32 z = lower_line[lower_line_index].z;

        for (s32 x = lower_line[lower_line_index].x; x <= right_line[right_index].x; ++x)
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

    buf_free(lower_line);
    buf_free(right_line);
    buf_free(left_line);

    return triangle;
}

BUF RasterizerOutput *RasterizeTriangle(EngineState *state, v4 p1, v4 p2, v4 p3)
{
    v4 *left   = GetLeftPoint(&p1, &p2, &p3);
    v4 *right  = GetRightPoint(&p1, &p2, &p3);
    v4 *middle = GetMiddlePoint(&p1, &p2, &p3, left, right);

    if (middle->y <= min(left->y, right->y))
    {
        if (left->y <= right->y)
        {
            return RasterizeTriangleDownUpLeftLower(state, *left, *middle, *right);
        }
        else
        {
            return RasterizeTriangleDownUpRightLower(state, *left, *middle, *right);
        }
    }
    else
    {
        if (left->y <= right->y)
        {
            return RasterizeTriangleUpDownLeftLower(state, *left, *middle, *right);
        }
        else
        {
            return RasterizeTriangleUpDownRightLower(state, *left, *middle, *right);
        }
    }
}
