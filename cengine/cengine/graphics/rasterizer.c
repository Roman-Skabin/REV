//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "graphics/rasterizer.h"
#include "tools/buffer.h"
#include "cengine.h"

// @NOTE(Roman): Resets to true in cengine.c on frame start
extern b32 gFrameStart;

typedef struct HashKey
{
    __m128 left;
    __m128 right;
    __m128 middle;
} HashKey;

enum
{
    DATA_COUNT       = PAGE_SIZE,
    CHUNKS_COUNT     = sizeof(HashKey) / sizeof(u32),
    MAX_FRAMES_IDDLE = 120,
};

typedef struct HashTable
{
    RasterizerOutput *data[DATA_COUNT];
    u32               frames[DATA_COUNT];
    u32               count;
} HashTable;

global HashTable gCache;

internal INLINE u32 Hash(HashKey *key)
{
    union
    {
        u32     chunks[CHUNKS_COUNT];
        HashKey key;
    } _;

    _.key = *key;

    for (u32 i = 1; i < CHUNKS_COUNT; ++i)
    {
        _.chunks[0] ^= _.chunks[i];
    }

    return _.chunks[0] % DATA_COUNT;
}

internal INLINE void CacheRasterizerOutput(HashKey *key, RasterizerOutput *data)
{
    u32 hash = Hash(key);

    if (!gCache.data[hash])
    {
        CheckM(gCache.count < DATA_COUNT, "Rasterizer's cache overflow");
        ++gCache.count;
    }

    gCache.data[hash] = data;
}

internal INLINE RasterizerOutput *GetCachedRasterizerOutput(HashKey *key)
{
    if (gCache.count)
    {
        u32 hash = Hash(key);

        gCache.frames[hash] = 0;

        if (gFrameStart)
        {
            for (s32 i = 0; i < DATA_COUNT; ++i)
            {
                if (gCache.data[i])
                {
                    ++gCache.frames[i];

                    if (gCache.frames[i] >= MAX_FRAMES_IDDLE)
                    {
                        gCache.data[i] = 0;
                        gCache.frames[i] = 0;
                        --gCache.count;
                    }
                }
            }

            if (gCache.frames[hash] > 0)
                gCache.frames[hash] = 0;

            gFrameStart = false;
        }

        return gCache.data[hash];
    }
    return 0;
}

internal INLINE __m128 *MATH_CALL GetLeftPoint(__m128 *p1, __m128 *p2, __m128 *p3)
{
    if (p1->m128_f32[0] < p2->m128_f32[0]) return p1->m128_f32[0] < p3->m128_f32[0] ? p1 : p3;
    else                                   return p2->m128_f32[0] < p3->m128_f32[0] ? p2 : p3;
}

internal INLINE __m128 *MATH_CALL GetRightPoint(__m128 *p1, __m128 *p2, __m128 *p3)
{
    if (p1->m128_f32[0] > p2->m128_f32[0]) return p1->m128_f32[0] > p3->m128_f32[0] ? p1 : p3;
    else                                   return p2->m128_f32[0] > p3->m128_f32[0] ? p2 : p3;
}

internal INLINE __m128 *MATH_CALL GetMiddlePoint(__m128 *p1, __m128 *p2, __m128 *p3, __m128 *left, __m128 *right)
{
    if (p1 == left) return p2 == right ? p3 : p2;
    if (p2 == left) return p1 == right ? p3 : p1;
    else            return p1 == right ? p2 : p1;
}

internal void MATH_CALL RasterizeTriangleSide(RasterizerOutput **triangle, Engine *engine, __m128 _lower, __m128 _upper)
{
    __m128i upper = _mm_cvtps_epi32(_upper);
    __m128i lower = _mm_cvtps_epi32(_lower);

    f32 x_slope = 0.0f;
    f32 z_slope = 0.0f;

    f32 d = MMf(_upper, 1) - MMf(_lower, 1);
    if (d)
    {
        x_slope = (MMf(_upper, 0) - MMf(_lower, 0)) / d;
        z_slope = (MMf(_upper, 2) - MMf(_lower, 2)) / d;
    }

    __m128 point = _lower;
    MMf(point, 3) = 1.0f;

    for (s32 y = MMs(lower, 1); y <= MMs(upper, 1); ++y)
    {
        MMf(point, 1) = cast(f32, y);
        __m128i ipoint = _mm_cvtps_epi32(point);

        if (                       0 <= MMs(ipoint, 0) && MMs(ipoint, 0) <  engine->window.size.w
        &&                         0 <= MMs(ipoint, 1) && MMs(ipoint, 1) <  engine->window.size.h
        &&  engine->renderer.zb.near <= MMf( point, 2) && MMf( point, 2) <= engine->renderer.zb.far)
        {
            RasterizerOutput el;
            el.x = MMs(ipoint, 0);
            el.y = MMs(ipoint, 1);
            el.z = MMf( point, 2);
            BufferPushBack(*triangle, el);
        }

        MMf(point, 0) += x_slope;
        MMf(point, 2) += z_slope;
    }
}

internal RasterizerOutput *MATH_CALL RasterizeTriangleDownUpLeftLower(Engine *engine, __m128 left, __m128 middle, __m128 right)
{
    RasterizerOutput *triangle = CreateBuffer(&engine->allocator, CACHE_LINE_SIZE);

    RasterizeTriangleSide(&triangle, engine, middle, left);
    RasterizerOutput *left_line_first = triangle;
    RasterizerOutput *left_line_last  = triangle + BufferGetCount(triangle) - 1;

    RasterizeTriangleSide(&triangle, engine, middle, right);
    RasterizerOutput *right_line_last = triangle + BufferGetCount(triangle) - 1;

    RasterizeTriangleSide(&triangle, engine, left, right);

    RasterizerOutput *left_line_point  = left_line_first;
    RasterizerOutput *right_line_point = left_line_last + 1;
    RasterizerOutput *upper_line_point = right_line_last + 1;

    for (s32 y = left_line_first->y; y < left_line_last->y; ++y)
    {
        f32 z_slope = 0.0f;

        s32 d = right_line_point->x - left_line_point->x;
        if (d)
        {
            z_slope = (right_line_point->z - left_line_point->z) / d;
        }

        f32 z = left_line_point->z;

        for (s32 x = left_line_point->x + 1; x < right_line_point->x; ++x)
        {
            RasterizerOutput el;
            el.x = x;
            el.y = y;
            el.z = z;
            BufferPushBack(triangle, el);

            z += z_slope;
        }

        ++left_line_point;
        ++right_line_point;
    }

    for (s32 y = left_line_last->y; y <= right_line_last->y; ++y)
    {
        f32 z_slope = 0.0f;

        s32 d = right_line_point->x - upper_line_point->x;
        if (d)
        {
            z_slope = (right_line_point->z - upper_line_point->z) / d;
        }

        f32 z = upper_line_point->z;

        for (s32 x = upper_line_point->x + 1; x < right_line_point->x; ++x)
        {
            RasterizerOutput el;
            el.x = x;
            el.y = y;
            el.z = z;
            BufferPushBack(triangle, el);

            z += z_slope;
        }

        ++right_line_point;
        ++upper_line_point;
    }

    return triangle;
}

internal RasterizerOutput *MATH_CALL RasterizeTriangleDownUpRightLower(Engine *engine, __m128 left, __m128 middle, __m128 right)
{
    RasterizerOutput *triangle = CreateBuffer(&engine->allocator, CACHE_LINE_SIZE);

    RasterizeTriangleSide(&triangle, engine, middle, left);
    RasterizerOutput *left_line_last = triangle + BufferGetCount(triangle) - 1;

    RasterizeTriangleSide(&triangle, engine, middle, right);
    RasterizerOutput *right_line_first = left_line_last + 1;
    RasterizerOutput *right_line_last  = triangle + BufferGetCount(triangle) - 1;

    RasterizeTriangleSide(&triangle, engine, right, left);

    RasterizerOutput *left_line_point  = triangle;
    RasterizerOutput *right_line_point = right_line_first;
    RasterizerOutput *upper_line_point = right_line_last + 1;

    for (s32 y = right_line_first->y; y < right_line_last->y; ++y)
    {
        f32 z_slope = 0.0f;

        s32 d = right_line_point->x - left_line_point->x;
        if (d)
        {
            z_slope = (right_line_point->z - left_line_point->z) / d;
        }

        f32 z = left_line_point->z;

        for (s32 x = left_line_point->x + 1; x < right_line_point->x; ++x)
        {
            RasterizerOutput el;
            el.x = x;
            el.y = y;
            el.z = z;
            BufferPushBack(triangle, el);

            z += z_slope;
        }

        ++left_line_point;
        ++right_line_point;
    }

    for (s32 y = right_line_last->y; y <= left_line_last->y; ++y)
    {
        f32 z_slope = 0.0f;

        s32 d = upper_line_point->x - left_line_point->x;
        if (d)
        {
            z_slope = (upper_line_point->z - left_line_point->z) / d;
        }

        f32 z = left_line_point->z;

        for (s32 x = left_line_point->x + 1; x < upper_line_point->x; ++x)
        {
            RasterizerOutput el;
            el.x = x;
            el.y = y;
            el.z = z;
            BufferPushBack(triangle, el);

            z += z_slope;
        }

        ++left_line_point;
        ++upper_line_point;
    }

    return triangle;
}

internal RasterizerOutput *MATH_CALL RasterizeTriangleUpDownLeftLower(Engine *engine, __m128 left, __m128 middle, __m128 right)
{
    RasterizerOutput *triangle = CreateBuffer(&engine->allocator, CACHE_LINE_SIZE);
    
    RasterizeTriangleSide(&triangle, engine, left, middle);
    RasterizerOutput *left_line_last = triangle + BufferGetCount(triangle) - 1;

    RasterizeTriangleSide(&triangle, engine, right, middle);
    RasterizerOutput *right_line_first = left_line_last + 1;
    RasterizerOutput *right_line_last  = triangle + BufferGetCount(triangle) - 1;

    RasterizeTriangleSide(&triangle, engine, left, right);

    RasterizerOutput *left_line_point  = left_line_last;
    RasterizerOutput *right_line_point = right_line_last;
    RasterizerOutput *lower_line_point = triangle + BufferGetCount(triangle) - 1;

    for (s32 y = right_line_last->y; y > right_line_first->y; --y)
    {
        f32 z_slope = 0.0f;

        s32 d = right_line_point->x - left_line_point->x;
        if (d)
        {
            z_slope = (right_line_point->z - left_line_point->z) / d;
        }

        f32 z = left_line_point->z;

        for (s32 x = left_line_point->x + 1; x < right_line_point->x; ++x)
        {
            RasterizerOutput el;
            el.x = x;
            el.y = y;
            el.z = z;
            BufferPushBack(triangle, el);

            z += z_slope;
        }

        --right_line_point;
        --left_line_point;
    }

    for (s32 y = right_line_first->y; y >= triangle->y; --y)
    {
        f32 z_slope = 0.0f;

        s32 d = lower_line_point->x - left_line_point->x;
        if (d)
        {
            z_slope = (lower_line_point->z - left_line_point->z) / d;
        }

        f32 z = left_line_point->z;

        for (s32 x = left_line_point->x + 1; x < lower_line_point->x; ++x)
        {
            RasterizerOutput el;
            el.x = x;
            el.y = y;
            el.z = z;
            BufferPushBack(triangle, el);

            z += z_slope;
        }

        --lower_line_point;
        --left_line_point;
    }

    return triangle;
}

internal RasterizerOutput *MATH_CALL RasterizeTriangleUpDownRightLower(Engine *engine, __m128 left, __m128 middle, __m128 right)
{
    RasterizerOutput *triangle = CreateBuffer(&engine->allocator, CACHE_LINE_SIZE);
    
    RasterizeTriangleSide(&triangle, engine, left, middle);
    RasterizerOutput *left_line_first = triangle;
    RasterizerOutput *left_line_last  = triangle + BufferGetCount(triangle) - 1;

    RasterizeTriangleSide(&triangle, engine, right, middle);
    RasterizerOutput *right_line_last = triangle + BufferGetCount(triangle) - 1;

    RasterizeTriangleSide(&triangle, engine, right, left);
    RasterizerOutput *lower_line_first = right_line_last + 1;

    RasterizerOutput *left_line_point  = left_line_last;
    RasterizerOutput *right_line_point = right_line_last;
    RasterizerOutput *lower_line_point = triangle + BufferGetCount(triangle) - 1;

    for (s32 y = left_line_last->y; y > left_line_first->y; --y)
    {
        f32 z_slope = 0.0f;

        s32 d = right_line_point->x - left_line_point->x;
        if (d)
        {
            z_slope = (right_line_point->z - left_line_point->z) / d;
        }

        f32 z = left_line_point->z;

        for (s32 x = left_line_point->x + 1; x < right_line_point->x; ++x)
        {
            RasterizerOutput el;
            el.x = x;
            el.y = y;
            el.z = z;
            BufferPushBack(triangle, el);

            z += z_slope;
        }

        --left_line_point;
        --right_line_point;
    }

    for (s32 y = left_line_first->y; y >= lower_line_first->y; --y)
    {
        f32 z_slope = 0.0f;

        s32 d = right_line_point->x - lower_line_point->x;
        if (d)
        {
            z_slope = (right_line_point->z - lower_line_point->z) / d;
        }

        f32 z = lower_line_point->z;

        for (s32 x = lower_line_point->x + 1; x < right_line_point->x; ++x)
        {
            RasterizerOutput el;
            el.x = x;
            el.y = y;
            el.z = z;
            BufferPushBack(triangle, el);

            z += z_slope;
        }

        --lower_line_point;
        --right_line_point;
    }

    return triangle;
}

RasterizerOutput *MATH_CALL RasterizeTriangle(Engine *engine, __m128 p1, __m128 p2, __m128 p3)
{
    __m128 *left   = GetLeftPoint(&p1, &p2, &p3);
    __m128 *right  = GetRightPoint(&p1, &p2, &p3);
    __m128 *middle = GetMiddlePoint(&p1, &p2, &p3, left, right);

    HashKey key = { *left, *right, *middle };
    
    RasterizerOutput *triangle = GetCachedRasterizerOutput(&key);

    if (!triangle)
    {
        if (middle->m128_f32[1] <= __min(left->m128_f32[1], right->m128_f32[1]))
        {
            if (left->m128_f32[1] <= right->m128_f32[1])
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
            if (left->m128_f32[1] <= right->m128_f32[1])
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
