//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/core.h"
#include "tools/static_string.hpp"

interface ENGINE_IMPEXP IGPUResource
{
    virtual void SetName(const StaticString<64>& name) = 0;
    virtual void SetData(const void *data) = 0;
    virtual void SetDataImmediate(const void *data) = 0;
};

interface ENGINE_IMPEXP IGPUMemoryManager
{
    virtual void Destroy() = 0;

    virtual IGPUResource *AllocateVB(u32 vertex_count, u32 stride)            = 0;
    virtual IGPUResource *AllocateIB(u32 indecies_count)                      = 0; // stride = sizeof(u32)
    virtual IGPUResource *AllocateCB(u32 bytes, const StaticString<64>& name) = 0;

    virtual void SetGPUResourceData(const StaticString<64>& name, const void *data)          = 0;
    virtual void SetGPUResourceDataImmediate(const StaticString<64>& name, const void *data) = 0;

    virtual void Reset()   = 0;
    virtual void Release() = 0;
};
