// Copyright (c) 2020-2021, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#include "core/pch.h"
#include "graphics/shader_manager.h"
#include "graphics/graphics_api.h"
#include "platform/d3d12/d3d12_shader_manager.h"
#include "platform/d3d12/d3d12_common.h"

namespace REV::GPU
{

ShaderHandle ShaderManager::CreateGraphicsShader(
    const ConstString&             shader_cache_filename,
    const ConstArray<AssetHandle>& textures,
    const ConstArray<CBufferDesc>& cbuffers,
    const ConstArray<SamplerDesc>& samplers,
    bool                           _static)
{
    ShaderHandle handle;
    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            handle.index   = cast(D3D12::ShaderManager *, platform)->CreateGraphicsShader(shader_cache_filename, textures, cbuffers, samplers, _static);
            handle._static = _static;
        } break;

        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }
    return handle;
}

void ShaderManager::SetCurrentGraphicsShader(ShaderHandle graphics_shader)
{
    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            D3D12::ShaderManager *shader_manager = cast(D3D12::ShaderManager *, platform);
            shader_manager->SetCurrentGraphicsShader(shader_manager->GetGraphicsShader(graphics_shader));
        } break;

        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }
}

void ShaderManager::BindVertexBuffer(ShaderHandle graphics_shader, ResourceHandle resource_handle)
{
    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            D3D12::ShaderManager *shader_manager = cast(D3D12::ShaderManager *, platform);
            shader_manager->BindVertexBuffer(shader_manager->GetGraphicsShader(graphics_shader), resource_handle);
        } break;

        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }
}

void ShaderManager::BindIndexBuffer(ShaderHandle graphics_shader, ResourceHandle resource_handle)
{
    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            D3D12::ShaderManager *shader_manager = cast(D3D12::ShaderManager *, platform);
            shader_manager->BindIndexBuffer(shader_manager->GetGraphicsShader(graphics_shader), resource_handle);
        } break;

        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }
}

void ShaderManager::Draw(ShaderHandle graphics_shader)
{
    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            D3D12::ShaderManager *shader_manager = cast(D3D12::ShaderManager *, platform);
            shader_manager->Draw(shader_manager->GetGraphicsShader(graphics_shader));
        } break;

        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }
}

CompileShaderResult ShaderManager::CompileShader(const ConstString& code, const ConstString& name, SHADER_KIND kind)
{
    CompileShaderResult result;

    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            D3D12::ShaderManager *shader_manager = cast(D3D12::ShaderManager *, platform);
            Logger               *logger         = &shader_manager->GetLogger();

            ID3DBlob *blob = null;
            switch (kind)
            {
                case SHADER_KIND_VERTEX:
                {
                    blob = shader_manager->CompileShader(code, name.Data(), "VSMain", "vs_5_1");
                    logger->LogSuccess("Vertex shader has been compiled");
                } break;

                case SHADER_KIND_HULL:
                {
                    if (code.Find(REV_CSTR_ARGS("HSMain"), 0) != ConstString::npos)
                    {
                        blob = shader_manager->CompileShader(code, name.Data(), "HSMain", "hs_5_1");
                        logger->LogSuccess("Hull shader has been compiled");
                    }
                    else
                    {
                        logger->LogWarning("Hull shader has not been compiled: Entry point 'HSMain' is not found");
                    }
                } break;

                case SHADER_KIND_DOMAIN:
                {
                    if (code.Find(REV_CSTR_ARGS("DSMain"), 0) != ConstString::npos)
                    {
                        blob = shader_manager->CompileShader(code, name.Data(), "DSMain", "ds_5_1");
                        logger->LogSuccess("Domain shader has been compiled");
                    }
                    else
                    {
                        logger->LogWarning("Domain shader has not been compiled: Entry point 'DSMain' is not found");
                    }
                } break;

                case SHADER_KIND_GEOMETRY:
                {
                    if (code.Find(REV_CSTR_ARGS("GSMain"), 0) != ConstString::npos)
                    {
                        blob = shader_manager->CompileShader(code, name.Data(), "GSMain", "gs_5_1");
                        logger->LogSuccess("Geometry shader has been compiled");
                    }
                    else
                    {
                        logger->LogWarning("Geometry shader has not been compiled: Entry point 'GSMain' is not found");
                    }
                } break;

                case SHADER_KIND_PIXEL:
                {
                    blob = shader_manager->CompileShader(code, name.Data(), "PSMain", "ps_5_1");
                    logger->LogSuccess("Pixel shader has been compiled");
                } break;

                case SHADER_KIND_COMPUTE:
                {
                    REV_ERROR_M("Compute shaders are not supported yet");
                } break;

                default:
                {
                    REV_ERROR_M("Wrong SHADER_KIND: %I32u", kind);
                } break;
            }

            if (blob)
            {
                result.blob     = blob;
                result.bytecode = ConstArray(cast(byte *, blob->GetBufferPointer()), blob->GetBufferSize());
            }
        } break;

        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }

    return result;
}

void ShaderManager::ReleaseCompiledShader(CompileShaderResult& shader_bytecode)
{
    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            D3D12::SafeRelease((ID3DBlob *&)shader_bytecode.blob);
            shader_bytecode.bytecode.~ConstArray();
        } break;

        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }
}

}
