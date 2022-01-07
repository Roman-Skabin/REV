// Copyright (c) 2020-2021, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#include "core/pch.h"
#include "graphics/forward_plus_pipeline.h"

namespace REV
{

ForwardPlusPipeline::ForwardPlusPipeline(Allocator *allocator)
    : m_StaticPasses(allocator),
      m_ScenePasses(allocator)
{
}

ForwardPlusPipeline::~ForwardPlusPipeline()
{
}

void ForwardPlusPipeline::AddStaticPass(RenderPassBase *render_pass)
{
    REV_CHECK(render_pass);

    RenderPassBase **new_static_render_pass = m_StaticPasses.PushBack();
    *new_static_render_pass = render_pass;
}

void ForwardPlusPipeline::AddScenePass(RenderPassBase *render_pass)
{
    REV_CHECK(render_pass);

    RenderPassBase **new_scene_render_pass = m_ScenePasses.PushBack();
    *new_scene_render_pass = render_pass;
}

void ForwardPlusPipeline::Render()
{
    m_RenderGraph.Render();
}

}
