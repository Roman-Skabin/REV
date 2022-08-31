// Copyright (c) 2020-2021, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#include "core/pch.h"
#include "graphics/render_graph.h"
#include "graphics/graphics_api.h"
#include "tools/string_builder.h"
#include "asset_manager/asset_manager.h"

namespace REV
{

RenderGraph::RenderGraph(Allocator *allocator)
    : m_Allocator(allocator),
      m_RenderPasses(allocator),
      m_Logger(ConstString(REV_CSTR_ARGS("RenderGraph")), Logger::TARGET_FILE)
{
}

RenderGraph::~RenderGraph()
{
}

void RenderGraph::Render()
{
    m_RenderPasses.ForEach([](RenderPass& render_pass, u64 chunk_index, u64 elem_global_index, u64 elem_local_index)
    {
        render_pass.Render();
    });
}

}
