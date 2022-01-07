// Copyright (c) 2020-2021, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#include "core/pch.h"
#include "graphics/render_graph.h"
#include "graphics/graphics_api.h"

namespace REV
{

RenderGraph::RenderGraph(Allocator *allocator)
    : m_Allocator(allocator),
      m_Levels(null),
      m_LevelsCount(0),
      m_LevelsCapacity(0)
{
}

RenderGraph::~RenderGraph()
{
}

void RenderGraph::Sort()
{
}

void RenderGraph::Render()
{
    RenderGraphLevel *levels_end = m_Levels + m_LevelsCount;
    for (RenderGraphLevel *level = m_Levels; level < levels_end; ++level)
    {
        RenderPassBase **render_passes_end = level->render_passes + level->render_passes_count;

        for (RenderPassBase **render_pass = level->render_passes; render_pass < render_passes_end; ++render_pass)
        {
            (*render_pass)->UploadResources();
        }
        for (RenderPassBase **render_pass = level->render_passes; render_pass < render_passes_end; ++render_pass)
        {
            (*render_pass)->Render();
        }
        for (RenderPassBase **render_pass = level->render_passes; render_pass < render_passes_end; ++render_pass)
        {
            (*render_pass)->ReadBackResources();
        }

        GraphicsAPI::GetDeviceContext()->WaitForGPU();
    }
}

}
