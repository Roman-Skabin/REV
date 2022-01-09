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

RenderPass::RenderPass(Allocator *allocator, RENDER_PASS_KIND kind)
    : m_SubPasses(allocator),
      m_Kind(kind),
      m_Flags(RENDER_PASS_FLAG_NONE)
{
}

RenderPass::~RenderPass()
{
}

void RenderPass::LoadResources()
{
    GPU::ShaderManager *shader_manager = GraphicsAPI::GetShaderManager();
    GPU::MemoryManager *memory_manager = GraphicsAPI::GetMemoryManager();

    for (RenderSubPass& subpass : m_SubPasses)
    {
        ConstArray<GPU::ResourceHandle> resources = shader_manager->GetLoadableResources(subpass.shader_handle);
        memory_manager->LoadResources(resources);
    }
}

void RenderPass::StoreResources()
{
    GPU::ShaderManager *shader_manager = GraphicsAPI::GetShaderManager();
    GPU::MemoryManager *memory_manager = GraphicsAPI::GetMemoryManager();

    for (RenderSubPass& subpass : m_SubPasses)
    {
        ConstArray<GPU::ResourceHandle> resources = shader_manager->GetStorableResources(subpass.shader_handle);
        memory_manager->StoreResources(resources);
    }
}

void RenderPass::Render()
{
    GPU::ShaderManager *shader_manager = GraphicsAPI::GetShaderManager();

    for (RenderSubPass& subpass : m_SubPasses)
    {
        shader_manager->SetCurrentGraphicsShader(subpass.shader_handle);

        shader_manager->BindVertexBuffer(subpass.shader_handle, subpass.vbuffer_handle);
        shader_manager->BindIndexBuffer(subpass.shader_handle, subpass.ibuffer_handle);

        shader_manager->Draw(subpass.shader_handle);
    }
}

void RenderPass::AddSubPass(
    const GPU::ShaderHandle&   shader_handle,
    const GPU::ResourceHandle& vbuffer_handle,
    const GPU::ResourceHandle& ibuffer_handle)
{
    RenderSubPass *subpass = m_SubPasses.PushBack();

    subpass->shader_handle  = shader_handle;
    subpass->vbuffer_handle = vbuffer_handle;
    subpass->ibuffer_handle = ibuffer_handle;
}

}
