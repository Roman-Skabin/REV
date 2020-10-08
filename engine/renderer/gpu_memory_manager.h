//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/core.h"

interface ENGINE_IMPEXP IGPUResource
{
    virtual void SetName(in const char *name, in_opt u32 name_len) = 0;
    virtual void SetData(in const void *data) = 0;
};

interface ENGINE_IMPEXP IGPUMemoryManager
{
    virtual void Destroy() = 0;

    virtual IGPUResource *AllocateVB(in u32 vertex_count, in u32 stride)                 = 0;
    virtual IGPUResource *AllocateIB(in u32 indecies_count)                              = 0;
    virtual IGPUResource *AllocateCB(in u32 bytes, in const char *name, in u32 name_len) = 0;

    virtual void SetGPUResourceData(in const char *name, in_opt u32 name_len, in const void *data) = 0;

    virtual void Reset()   = 0;
    virtual void Release() = 0;
};
