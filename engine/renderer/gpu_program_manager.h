//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/core.h"
#include "renderer/gpu_memory_manager.h"
#include "tools/static_string.hpp"

interface ENGINE_IMPEXP IGraphicsProgram
{
    virtual void AttachHullShader(const StaticString<MAX_PATH>& filename)     = 0;
    virtual void AttachDomainShader(const StaticString<MAX_PATH>& filename)   = 0;
    virtual void AttachGeometryShader(const StaticString<MAX_PATH>& filename) = 0;

    virtual void BindResource(IGPUResource *resource) = 0;

    virtual void BindVertexBuffer(IGPUResource *resource) = 0;
    virtual void BindIndexBuffer(IGPUResource *resource)  = 0;

    virtual void DrawVertices() = 0;
    virtual void DrawIndices()  = 0;
};

interface ENGINE_IMPEXP IComputeProgram
{
};

interface ENGINE_IMPEXP IGPUProgramManager
{
    virtual void Destroy() = 0;

    virtual IGraphicsProgram *CreateGraphicsProgram(const StaticString<MAX_PATH>& vs_filename, const StaticString<MAX_PATH>& ps_filename) = 0;

    virtual void SetCurrentGraphicsProgram(IGraphicsProgram *graphics_program) = 0;
};
