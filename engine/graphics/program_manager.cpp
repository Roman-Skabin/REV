//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "graphics/program_manager.h"
#include "graphics/graphics_api.h"
#include "platform/d3d12/d3d12_program_manager.h"

namespace GPU
{

GraphicsProgramHandle ProgramManager::CreateGraphicsProgram(const StaticString<MAX_PATH>& vs_filename, const StaticString<MAX_PATH>& ps_filename)
{
    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            return cast<D3D12::ProgramManager *>(platform)->CreateGraphicsProgram(vs_filename, ps_filename);
        } break;

        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }
    return U64_MAX;
}

void ProgramManager::SetCurrentGraphicsProgram(GraphicsProgramHandle graphics_program)
{
    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            D3D12::ProgramManager *program_manager = cast<D3D12::ProgramManager *>(platform);
            program_manager->SetCurrentGraphicsProgram(program_manager->GetGraphicsProgram(graphics_program));
        } break;

        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }
}

void ProgramManager::AttachHullShader(GraphicsProgramHandle graphics_program, const StaticString<MAX_PATH>& filename)
{
    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            D3D12::ProgramManager *program_manager = cast<D3D12::ProgramManager *>(platform);
            program_manager->AttachHullShader(program_manager->GetGraphicsProgram(graphics_program), filename);
        } break;

        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }
}

void ProgramManager::AttachDomainShader(GraphicsProgramHandle graphics_program, const StaticString<MAX_PATH>& filename)
{
    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            D3D12::ProgramManager *program_manager = cast<D3D12::ProgramManager *>(platform);
            program_manager->AttachDomainShader(program_manager->GetGraphicsProgram(graphics_program), filename);
        } break;

        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }
}

void ProgramManager::AttachGeometryShader(GraphicsProgramHandle graphics_program, const StaticString<MAX_PATH>& filename)
{
    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            D3D12::ProgramManager *program_manager = cast<D3D12::ProgramManager *>(platform);
            program_manager->AttachGeometryShader(program_manager->GetGraphicsProgram(graphics_program), filename);
        } break;

        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }
}

void ProgramManager::AttachResource(GraphicsProgramHandle graphics_program, ResourceHandle resource_handle)
{
    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            D3D12::ProgramManager *program_manager = cast<D3D12::ProgramManager *>(platform);
            program_manager->AttachResource(program_manager->GetGraphicsProgram(graphics_program), resource_handle);
        } break;

        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }
}

void ProgramManager::BindVertexBuffer(GraphicsProgramHandle graphics_program, ResourceHandle resource_handle)
{
    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            D3D12::ProgramManager *program_manager = cast<D3D12::ProgramManager *>(platform);
            program_manager->BindVertexBuffer(program_manager->GetGraphicsProgram(graphics_program), resource_handle);
        } break;

        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }
}

void ProgramManager::BindIndexBuffer(GraphicsProgramHandle graphics_program, ResourceHandle resource_handle)
{
    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            D3D12::ProgramManager *program_manager = cast<D3D12::ProgramManager *>(platform);
            program_manager->BindIndexBuffer(program_manager->GetGraphicsProgram(graphics_program), resource_handle);
        } break;

        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }
}

void ProgramManager::DrawVertices(GraphicsProgramHandle graphics_program)
{
    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            D3D12::ProgramManager *program_manager = cast<D3D12::ProgramManager *>(platform);
            program_manager->DrawVertices(program_manager->GetGraphicsProgram(graphics_program));
        } break;

        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }
}

void ProgramManager::DrawIndices(GraphicsProgramHandle graphics_program)
{
    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            D3D12::ProgramManager *program_manager = cast<D3D12::ProgramManager *>(platform);
            program_manager->DrawIndices(program_manager->GetGraphicsProgram(graphics_program));
        } break;

        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }
}

}
