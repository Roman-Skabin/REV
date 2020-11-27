//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/core.h"
#include "graphics/memory_manager.h"
#include "tools/static_string.hpp"

class GraphicsAPI;

namespace GPU
{
    using GraphicsProgramHandle = u64;
    using ComputeProgramHandle  = u64;

    class ENGINE_API ProgramManager final
    {
    public:
        GraphicsProgramHandle CreateGraphicsProgram(const StaticString<MAX_PATH>& vs_filename, const StaticString<MAX_PATH>& ps_filename);

        void SetCurrentGraphicsProgram(GraphicsProgramHandle graphics_program);

        // @TODO(Roman) @Optimize(Roman): Pass an asset handle. Asset manager.
        void AttachHullShader(GraphicsProgramHandle graphics_program, const StaticString<MAX_PATH>& filename);
        void AttachDomainShader(GraphicsProgramHandle graphics_program, const StaticString<MAX_PATH>& filename);
        void AttachGeometryShader(GraphicsProgramHandle graphics_program, const StaticString<MAX_PATH>& filename);

        void AttachResource(GraphicsProgramHandle graphics_program, ResourceHandle resource_handle);

        void BindVertexBuffer(GraphicsProgramHandle graphics_program, ResourceHandle resource_handle);
        void BindIndexBuffer(GraphicsProgramHandle graphics_program, ResourceHandle resource_handle);

        void DrawVertices(GraphicsProgramHandle graphics_program);
        void DrawIndices(GraphicsProgramHandle graphics_program);

    private:
        ProgramManager()                      = delete;
        ProgramManager(const ProgramManager&) = delete;
        ProgramManager(ProgramManager&&)      = delete;

        ~ProgramManager() = delete;

        ProgramManager& operator=(const ProgramManager&) = delete;
        ProgramManager& operator=(ProgramManager&&)      = delete;

    private:
        #pragma warning(suppress: 4200)
        byte platform[0];

        friend GraphicsAPI;
    };
}
