// Copyright (c) 2020-2021, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#pragma once

#include "graphics/render_pass.h"
#include "tools/string.h"
 
namespace REV
{
    struct RenderGraphLayer
    {
        REV_INLINE RenderGraphLayer(Allocator *allocator) : render_passes(allocator) {}

        Array<RenderPass *> render_passes;
    };

    class REV_API RenderGraph final
    {
    public:
        RenderGraph(Allocator *allocator);
        ~RenderGraph();

        void Sort(const ConstArray<RenderPass *>& render_passes);
        void Render();

        String Serialize();

    private:
        REV_DELETE_CONSTRS_AND_OPS(RenderGraph);

    private:
        Allocator               *m_Allocator;
        Array<RenderGraphLayer>  m_Layers;

    public:
        bool                     m_NeedToSort;
    };
}
