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

        RenderPass& AddRenderPass(const ConstString& name);

        void ResetPasses();

        void Render();

    private:
        Allocator   *m_Allocator;
        RenderGraph  m_RenderGraph;
    };
}
