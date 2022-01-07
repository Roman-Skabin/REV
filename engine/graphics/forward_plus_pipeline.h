// Copyright (c) 2020-2021, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#pragma once

#include "graphics/render_pass.h"

namespace REV
{
    class REV_API ForwardPlusPipeline final
    {
    public:
        ForwardPlusPipeline(Allocator *allocator);
        ~ForwardPlusPipeline();

        void AddStaticPass(RenderPassBase *render_pass);
        void AddScenePass(RenderPassBase *render_pass);

        void Render();

    private:
        Array<RenderPassBase *> m_StaticPasses;
        Array<RenderPassBase *> m_ScenePasses;
        RenderGraph             m_RenderGraph;
    };
}
