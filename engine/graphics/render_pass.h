// Copyright (c) 2020-2021, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#pragma once

#include "graphics/shader_manager.h"

namespace REV
{
    enum RENDER_PASS_KIND : u32
    {
        RENDER_PASS_KIND_NONE,

        // Common
        RENDER_PASS_KIND_OPAQUE,
        RENDER_PASS_KIND_TRANSPARENT,
        RENDER_PASS_KIND_SEA,
        RENDER_PASS_KIND_SKYBOX,
        RENDER_PASS_KIND_DEPTH,
        RENDER_PASS_KIND_UI,

        // Forward+ pipeline
        RENDER_PASS_KIND_COMPUTE_LIGHT_GRID,

        // @TODO(Roman): More passes
    };

    class REV_API RenderPassBase
    {
    protected:
        RenderPassBase(Allocator *allocator, RENDER_PASS_KIND kind);
        virtual ~RenderPassBase();

    public:
        void UploadResources();
        void ReadBackResources();

        virtual void Render() = 0;

    private:
        REV_DELETE_CONSTRS_AND_OPS(RenderPassBase);

    private:
        Array<GPU::ShaderHandle> m_Shaders;
        RENDER_PASS_KIND         m_Kind;
    };
}
