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
      m_Levels(allocator),
      m_NeedToSort(false)
{
}

RenderGraph::~RenderGraph()
{
}

void RenderGraph::Sort(const ConstArray<RenderPass *>& render_passes)
{
    // @Cleanup(Roman): Temp, do actual sorting
    for (RenderPass *render_pass : render_passes)
    {
        RenderGraphLayer  *layer = new (m_Layers.PushBack()) RenderGraphLayer(m_Allocator);
        RenderPass       **pass  = layer->render_passes.PushBack();

        *pass = render_pass;
    }

    m_NeedToSort = false;

    REV_LOCAL bool serialized = false;
    if (!serialized)
    {
        String serialized_graph = Serialize();
        REV_INFO_M("\n%.*s", serialized_graph.Length(), serialized_graph.Data());
        serialized = true;
    }
}

void RenderGraph::Render()
{
    REV_CHECK_M(!m_NeedToSort, "Render graph is not sorted");

    for (RenderGraphLayer& layer : m_Layers)
    {
        for (RenderPass *render_pass : layer.render_passes)
            render_pass->LoadResources();

        for (RenderPass *render_pass : layer.render_passes)
            render_pass->Render();

        for (RenderPass *render_pass : layer.render_passes)
            render_pass->StoreResources();

        GraphicsAPI::GetDeviceContext()->WaitForGPU();
    }
}

String RenderGraph::Serialize()
{
    /*
    Graph:
    |---- Layer 0:
    |     |---- Pass "Name":
    |     |     |---- Input:
    |     |     |     |---- Buffer:  "Name"
    |     |     |     |---- Texture: "Name"
    |     |     |---- Output:
    |     |     |     |---- Render Target: "Name"
    |     |     |     |---- Render Target: "Name"
    |     |     |     |---- Depth Target: "Name"
    |     |     |---- SubPass 0:
    |     |     |     |---- Shader: "Name"
    |     |     |     |---- Vertex Buffer: "Name"
    |     |     |     |---- Index Buffer: "Name"
    |     |     |---- SubPass 1:
    |     |           |---- Shader: "Name"
    |     |           |---- Vertex Buffer: "Name"
    |     |           |---- Index Buffer: "Name"
    |     |---- Pass "Name":
    |           |---- Input:
    |           |     |---- Buffer:  "Name"
    |           |     |---- Texture: "Name"
    |           |---- Output:
    |           |     |---- Render Target: "Name"
    |           |     |---- Render Target: "Name"
    |           |     |---- Depth Target: "Name"
    |           |---- SubPass 0:
    |           |     |---- Shader: "Name"
    |           |     |---- Vertex Buffer: "Name"
    |           |     |---- Index Buffer: "Name"
    |           |---- SubPass 1:
    |                 |---- Shader: "Name"
    |                 |---- Vertex Buffer: "Name"
    |                 |---- Index Buffer: "Name"
    |---- Layer 1:
          |---- Pass "Name":
          |     |---- Input:
          |     |     |---- Buffer:  "Name"
          |     |     |---- Texture: "Name"
          |     |---- Output:
          |     |     |---- Render Target: "Name"
          |     |     |---- Render Target: "Name"
          |     |     |---- Depth Target: "Name"
          |     |---- SubPass 0:
          |     |     |---- Shader: "Name"
          |     |     |---- Vertex Buffer: "Name"
          |     |     |---- Index Buffer: "Name"
          |     |---- SubPass 1:
          |           |---- Shader: "Name"
          |           |---- Vertex Buffer: "Name"
          |           |---- Index Buffer: "Name"
          |---- Pass "Name":
                |---- Input:
                |     |---- Buffer:  "Name"
                |     |---- Texture: "Name"
                |---- Output:
                |     |---- Render Target: "Name"
                |     |---- Render Target: "Name"
                |     |---- Depth Target: "Name"
                |---- SubPass 0:
                |     |---- Shader: "Name"
                |     |---- Vertex Buffer: "Name"
                |     |---- Index Buffer: "Name"
                |---- SubPass 1:
                      |---- Shader: "Name"
                      |---- Vertex Buffer: "Name"
                      |---- Index Buffer: "Name"
    */

    GPU::MemoryManager *memory_manager = GraphicsAPI::GetMemoryManager();
    AssetManager       *asset_manager  = AssetManager::Get();
    
    StringBuilder builder(m_Allocator, 1024);

    builder.BuildLn("Render Graph");

    u64 layer_index   = 0;
    for (RenderGraphLayer& layer : m_Layers)
    {
        builder.BuildLn("|---- Layer ", layer_index, ':');

        for (RenderPass& pass : layer.render_passes)
        {
            builder.BuildLn("|     |---- Pass \"", pass.KindName(), "\":");

            builder.BuildLn("|     |     |---- Input:");
            for (const GPU::ResourceHandle& resource : pass.Input())
            {
                if (GPU::MemoryManager::IsBuffer(resource))
                {
                    builder.BuildLn("|     |     |     |---- Buffer: \"", memory_manager->GetBufferName(resource), '"');
                }
                else if (GPU::MemoryManager::IsTexture(resource))
                {
                    builder.BuildLn("|     |     |     |---- Texture: \"", memory_manager->GetTextureName(resource), '"');
                }
            }

            builder.BuildLn("|     |     |---- Output:");
            for (const GPU::ResourceHandle& resource : pass.Output())
            {
                if (resource.flags & GPU::RESOURCE_FLAG_RENDER_TARGET)
                {
                    builder.BuildLn("|     |     |     |---- Render target: \"", memory_manager->GetTextureName(resource), '"');
                }
                else if (resource.flags & GPU::RESOURCE_FLAG_DEPTH_STENCIL)
                {
                    builder.BuildLn("|     |     |     |---- Depth stencil: \"", memory_manager->GetTextureName(resource), '"');
                }
            }

            u64 subpass_index = 0;
            for (const RenderSubPass& subpass : pass.SubPasses())
            {
                Asset *shader_asset = asset_manager->GetAsset(subpass.shader_handle);
                REV_CHECK(shader_asset);

                builder.BuildLn("|     |---- SubPass ", subpass_index++, ':');
                builder.BuildLn("|     |     |     |---- Shader: \"", shader_asset->name.ToConstString(), '"');
                builder.BuildLn("|     |     |     |---- Vertex Buffer: \"", memory_manager->GetBufferName(subpass.vbuffer_handle), '"');
                builder.BuildLn("|     |     |     |---- Index Buffer: \"", memory_manager->GetBufferName(subpass.ibuffer_handle), '"');
            }
        }
    }

    return builder.ToString();
}

}
