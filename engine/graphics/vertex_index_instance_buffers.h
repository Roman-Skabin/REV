// Copyright (c) 2020-2021, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#pragma once

#include "math/vec.h"

#pragma pack(push, 1)

namespace REV
{
    //
    // Index Buffers
    //
    typedef u16 Index16;
    typedef u32 Index32;

    //
    // Vertex Buffers
    //
    struct REV_ALIGN(1) VertexPos
    {
        Math::v4 position;
    };

    struct REV_ALIGN(1) VertexPosTex
    {
        Math::v4 position;
        Math::v2 tex_coord;
    };

    struct REV_ALIGN(1) VertexPosNormTex
    {
        Math::v4 position;
        Math::v4 normal;
        Math::v2 tex_coord;
    };

    struct REV_ALIGN(1) VertexPosNormTanTex
    {
        Math::v4 position;
        Math::v4 normal;
        Math::v4 tangent;
        Math::v2 tex_coord;
    };

    struct REV_ALIGN(1) VertexPosNormBinormTex
    {
        Math::v4 position;
        Math::v4 normal;
        Math::v4 binormal;
        Math::v2 tex_coord;
    };

    struct REV_ALIGN(1) VertexPosNormTanBinormTex
    {
        Math::v4 position;
        Math::v4 normal;
        Math::v4 tangent;
        Math::v4 binormal;
        Math::v2 tex_coord;
    };

    //
    // Instance Buffers
    //
    struct REV_ALIGN(1) InstancePos
    {
        Math::v4 position;
    };

    struct REV_ALIGN(1) InstancePosColor
    {
        Math::v4 position;
        Math::v4 color;
    };
}

#pragma pack(pop)
