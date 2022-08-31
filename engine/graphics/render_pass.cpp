// Copyright (c) 2020-2021, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#include "core/pch.h"
#include "graphics/render_pass.h"
#include "graphics/graphics_api.h"

namespace REV
{

RenderPass::RenderPass(const ConstString& name)
    : m_Name(name)
{
}

RenderPass::~RenderPass()
{
}

void RenderPass::Render()
{
    ShaderManager *shader_manager = GraphicsAPI::GetShaderManager();

    for (RenderSubPass& subpass : m_SubPasses)
    {
        shader_manager->SetCurrentGraphicsShader(subpass.shader_handle);

        shader_manager->BindVertexBuffer(subpass.shader_handle, subpass.vbuffer_handle);
        shader_manager->BindIndexBuffer(subpass.shader_handle, subpass.ibuffer_handle);

        shader_manager->Draw(subpass.shader_handle);
    }
}

}
