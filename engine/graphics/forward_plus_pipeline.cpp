// Copyright (c) 2020-2021, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#include "core/pch.h"
#include "graphics/forward_plus_pipeline.h"
#include "memory/memory.h"

namespace REV
{

ForwardPlusPipeline::ForwardPlusPipeline(Allocator *allocator)
    : m_RenderGraph(allocator)
{
    for (u32 i = 0; i < RENDER_PASS_KIND_MAX; ++i)
    {
        new (m_RenderPasses + i) RenderPass(allocator, i);
    }
}

ForwardPlusPipeline::~ForwardPlusPipeline()
{
}

RenderPass *ForwardPlusPipeline::EnableRenderPass(RENDER_PASS_KIND kind)
{
    RenderPass *render_pass = GetRenderPass(kind);
    render_pass->Enable();
    m_RenderGraph.m_NeedToSort = true;
    return render_pass;
}

RenderPass *ForwardPlusPipeline::GetRenderPass(RENDER_PASS_KIND kind)
{
    REV_CHECK(kind < RENDER_PASS_KIND_MAX);
    RenderPass *render_pass = m_RenderPasses + kind;
    REV_CHECK(render_pass->Kind() == kind);
    return render_pass;
}

void ForwardPlusPipeline::ResetPasses()
{
    RenderPass *render_passes_end = m_RenderPasses + RENDER_PASS_KIND_MAX;
    for (RenderPass *render_pass = m_RenderPasses; render_pass < render_passes_end; ++render_pass)
    {
        render_pass->ResetSubPasses();
        render_pass->Disable();
    }
}

void ForwardPlusPipeline::Render()
{
    if (m_RenderGraph.m_NeedToSort)
    {
        ConstArray<RenderPass *> enabled_passes = CollectEnabledPasses();
        m_RenderGraph.Sort(enabled_passes);
    }

    m_RenderGraph.Render();
}

ConstArray<RenderPass *> ForwardPlusPipeline::CollectEnabledPasses()
{
    u64 render_passes_count = 0;

    RenderPass *render_passes_end = m_RenderPasses + RENDER_PASS_KIND_MAX;
    for (RenderPass *render_pass = m_RenderPasses; render_pass < render_passes_end; ++render_pass)
    {
        if (render_pass->Enabled())
        {
            ++render_passes_count;
        }
    }

    RenderPass **render_passes    = Memory::Get()->PushToFA<RenderPass *>(render_passes_count);
    RenderPass **render_passes_it = render_passes;

    for (RenderPass *render_pass = m_RenderPasses; render_pass < render_passes_end; ++render_pass)
    {
        if (render_pass->Enabled())
        {
            *render_passes_it++ = render_pass;
        }
    }

    return ConstArray(render_passes, render_passes_count);
}

}
