// Copyright (c) 2020-2021, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#pragma once

#include "graphics/render_pass.h"
#include "tools/string.h"
#include "tools/logger.h"

namespace REV
{
    class REV_API RenderGraph final
    {
    public:
        RenderGraph(Allocator *allocator);
        ~RenderGraph();

        void Render();

    private:
        REV_DELETE_CONSTRS_AND_OPS(RenderGraph);

    private:
        Allocator             *m_Allocator;
        ChunkList<RenderPass>  m_RenderPasses;
        Logger                 m_Logger;
    };
}
