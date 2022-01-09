// Copyright (c) 2020-2021, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#pragma once

#include "graphics/render_graph.h"

namespace REV
{
    class REV_API ForwardPlusPipeline final
    {
    public:
        ForwardPlusPipeline(Allocator *allocator);
        ~ForwardPlusPipeline();

        RenderPass *EnableRenderPass(RENDER_PASS_KIND kind);
        RenderPass *GetRenderPass(RENDER_PASS_KIND kind);

        void ResetPasses();

        void Render();

    private:
        ConstArray<RenderPass *> CollectEnabledPasses();
    
    private:
        RenderGraph m_RenderGraph;
        RenderPass  m_RenderPasses[RENDER_PASS_KIND_MAX];
    };
}
