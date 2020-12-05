//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/common.h"
#include "tools/static_string.hpp"

class GraphicsAPI;

namespace REV::GPU
{
    enum class RESOURCE_KIND : u64
    {
        UNKNOWN,
        VB,
        IB,
        CB,
        SR,
        UA,
    };

    struct ResourceHandle
    {
        u64           index;
        RESOURCE_KIND kind;

        constexpr ResourceHandle(u64 index = REV_U64_MAX, RESOURCE_KIND kind = RESOURCE_KIND::UNKNOWN) : index(index), kind(kind) {}
    };

    class REV_API MemoryManager final
    {
    public:
        ResourceHandle AllocateVertexBuffer(u32 vertex_count, u32 vertex_stride, const StaticString<64>& name = null);
        ResourceHandle AllocateIndexBuffer(u32 index_count, const StaticString<64>& name = null); // stride = sizeof(u32)
        ResourceHandle AllocateConstantBuffer(u32 bytes, const StaticString<64>& name /* required */);

        void SetResoucreData(ResourceHandle resource, const void *data);

        void SetResourceDataImmediate(ResourceHandle resource, const void *data);

        void StartImmediateExecution();
        void EndImmediateExecution();

        void FreeMemory();

    private:
        MemoryManager()                     = delete;
        MemoryManager(const MemoryManager&) = delete;
        MemoryManager(MemoryManager&&)      = delete;

        ~MemoryManager() = delete;

        MemoryManager& operator=(const MemoryManager&) = delete;
        MemoryManager& operator=(MemoryManager&&)      = delete;

    private:
        #pragma warning(suppress: 4200)
        byte platform[0];

        friend GraphicsAPI;
    };
}
