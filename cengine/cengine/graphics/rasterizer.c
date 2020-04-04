//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "graphics/rasterizer.h"
#include "tools/buffer.h"
#include "cengine.h"

typedef struct HashTable
{
    RasterizerOutput *data[PAGE_SIZE];
    // u8                 frames[PAGE_SIZE];
    // Release data[i] if we don't use it for n frames
    u32                   count;
} HashTable;

global HashTable gCache;

typedef struct HashKey
{
    v4 left;
    v4 right;
    v4 middle;
} HashKey;

internal u32 Hash(HashKey *key)
{
    sizeof(HashTable);
    enum { CHUNKS_COUNT = sizeof(HashKey) / sizeof(u32) };

    u32 chunks[CHUNKS_COUNT];
    Check(sizeof(chunks) == sizeof(HashKey));

    CopyMemory(chunks, key, sizeof(HashKey));

    for (u32 i = 1; i < CHUNKS_COUNT; ++i)
    {
        chunks[0] ^= chunks[i];
    }

    return chunks[0] % PAGE_SIZE;
}

internal INLINE void CacheRasterizerOutput(HashKey *key, RasterizerOutput *data)
{
    u32 hash = Hash(key);

    if (!gCache.data[hash])
    {
        ++gCache.count;
    }

    gCache.data[hash] = data;
}

internal INLINE RasterizerOutput *GetCachedRasterizerOutput(HashKey *key)
{
    u32 hash = Hash(key);

    // @TODO(Roman): release data, we don't use for n frames

    return gCache.data[hash];
}

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

internal void RasterizeTriangleSide(RasterizerOutput **triangle, Engine *engine, v4 _lower, v4 _upper)
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

        if (                      0 <= ipoint.x && ipoint.x <  engine->window.size.w
        &&                        0 <= ipoint.y && ipoint.y <  engine->window.size.h
        &&  engine->renderer.zb.near <=  point.z &&  point.z <= engine->renderer.zb.far)
        {
            RasterizerOutput el;
            el.xy = ipoint.xy;
            el.z  = point.z;
            BufferPushBack(*triangle, el);
        }

        x += x_slope;
        z += z_slope;
    }
}

internal RasterizerOutput *RasterizeTriangleDownUpLeftLower(Engine *engine, v4 left, v4 middle, v4 right)
{
    RasterizerOutput *triangle = CreateBuffer(&engine->allocator, CACHE_LINE_SIZE);

    RasterizeTriangleSide(&triangle, engine, middle, left);
    u64 left_line_first = 0;
    u64 left_line_last  = BufferGetCount(triangle) - 1;

    RasterizeTriangleSide(&triangle, engine, middle, right);
    u64 right_line_first = left_line_last + 1;
    u64 right_line_last  = BufferGetCount(triangle) - 1;

    RasterizeTriangleSide(&triangle, engine, left, right);
    u64 upper_line_first = right_line_last + 1;
    u64 upper_line_last  = BufferGetCount(triangle) - 1;

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
            BufferPushBack(triangle, el);

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
            BufferPushBack(triangle, el);

            z += z_slope;
        }

        ++lower_index;
        ++upper_index;
    }

    return triangle;
}

internal RasterizerOutput *RasterizeTriangleDownUpRightLower(Engine *engine, v4 left, v4 middle, v4 right)
{
    RasterizerOutput *triangle = CreateBuffer(&engine->allocator, CACHE_LINE_SIZE);

    RasterizeTriangleSide(&triangle, engine, middle, left);
    u64 left_line_first = 0;
    u64 left_line_last  = BufferGetCount(triangle) - 1;

    RasterizeTriangleSide(&triangle, engine, middle, right);
    u64 right_line_first = left_line_last + 1;
    u64 right_line_last  = BufferGetCount(triangle) - 1;

    RasterizeTriangleSide(&triangle, engine, right, left);
    u64 upper_line_first = right_line_last + 1;
    u64 upper_line_last  = BufferGetCount(triangle) - 1;

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
            BufferPushBack(triangle, el);

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
            BufferPushBack(triangle, el);

            z += z_slope;
        }

        ++lower_index;
        ++upper_index;
    }

    return triangle;
}

internal RasterizerOutput *RasterizeTriangleUpDownLeftLower(Engine *engine, v4 left, v4 middle, v4 right)
{
    RasterizerOutput *triangle = CreateBuffer(&engine->allocator, CACHE_LINE_SIZE);
    
    RasterizeTriangleSide(&triangle, engine, left, middle);
    u64 left_line_first = 0;
    u64 left_line_last  = BufferGetCount(triangle) - 1;

    RasterizeTriangleSide(&triangle, engine, right, middle);
    u64 right_line_first = left_line_last + 1;
    u64 right_line_last  = BufferGetCount(triangle) - 1;

    RasterizeTriangleSide(&triangle, engine, left, right);
    u64 lower_line_first = right_line_last + 1;
    u64 lower_line_last  = BufferGetCount(triangle) - 1;

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
            BufferPushBack(triangle, el);

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
            BufferPushBack(triangle, el);

            z += z_slope;
        }

        ++lower_index;
        ++upper_index;
    }

    return triangle;
}

internal RasterizerOutput *RasterizeTriangleUpDownRightLower(Engine *engine, v4 left, v4 middle, v4 right)
{
    RasterizerOutput *triangle = CreateBuffer(&engine->allocator, CACHE_LINE_SIZE);
    
    RasterizeTriangleSide(&triangle, engine, left, middle);
    u64 left_line_first = 0;
    u64 left_line_last  = BufferGetCount(triangle) - 1;

    RasterizeTriangleSide(&triangle, engine, right, middle);
    u64 right_line_first = left_line_last + 1;
    u64 right_line_last  = BufferGetCount(triangle) - 1;

    RasterizeTriangleSide(&triangle, engine, right, left);
    u64 lower_line_first = right_line_last + 1;
    u64 lower_line_last  = BufferGetCount(triangle) - 1;

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
            BufferPushBack(triangle, el);

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
            BufferPushBack(triangle, el);

            z += z_slope;
        }

        ++lower_index;
        ++upper_index;
    }

    return triangle;
}

RasterizerOutput *RasterizeTriangle(Engine *engine, v4 p1, v4 p2, v4 p3)
{
    v4 *left   = GetLeftPoint(&p1, &p2, &p3);
    v4 *right  = GetRightPoint(&p1, &p2, &p3);
    v4 *middle = GetMiddlePoint(&p1, &p2, &p3, left, right);

    HashKey key = { *left, *right, *middle };
    
    RasterizerOutput *triangle = GetCachedRasterizerOutput(&key);

    if (!triangle)
    {
        if (middle->y <= min(left->y, right->y))
        {
            if (left->y <= right->y)
            {
                triangle = RasterizeTriangleDownUpLeftLower(engine, *left, *middle, *right);
            }
            else
            {
                triangle = RasterizeTriangleDownUpRightLower(engine, *left, *middle, *right);
            }
        }
        else
        {
            if (left->y <= right->y)
            {
                triangle = RasterizeTriangleUpDownLeftLower(engine, *left, *middle, *right);
            }
            else
            {
                triangle = RasterizeTriangleUpDownRightLower(engine, *left, *middle, *right);
            }
        }

        CacheRasterizerOutput(&key, triangle);
    }

    return triangle;
}
