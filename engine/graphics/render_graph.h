// Copyright (c) 2020-2021, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#pragma once

#include "graphics/render_pass.h"
 
namespace REV
{
    struct RenderGraphLevel
    {
        u64              level;
        RenderPassBase **render_passes;
        u64              render_passes_count;
        u64              render_passes_capacity;
    };

    class REV_API RenderGraph final
    {
    public:
        RenderGraph(Allocator *allocator);
        ~RenderGraph();

        void Sort();
        void Render();

    private:
        REV_DELETE_CONSTRS_AND_OPS(RenderGraph);

    private:
        Allocator        *m_Allocator;
        RenderGraphLevel *m_Levels;
        u64               m_LevelsCount;
        u64               m_LevelsCapacity;
    };
}
