//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/core.h"

interface ENGINE_IMPEXP IGPUResource
{
};

interface ENGINE_IMPEXP IGPUMemoryManager
{
    virtual void Destroy() = 0;
};
